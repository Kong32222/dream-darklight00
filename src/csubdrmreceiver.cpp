#include "csubdrmreceiver.h"

void CSubDRMReceiver::process()
{

    bool bFrameToSend = false;
    bool bEnoughData  = true;
    /* Input - from upstream RSCI or input and demodulation from sound card / file */
    if (pUpstreamRSCI->GetInEnabled())
    {
        RSIPacketBuf.Clear();
        pUpstreamRSCI->ReadData(Parameters, RSIPacketBuf);

        if (RSIPacketBuf.GetFillLevel() > 0)
        {

            time_keeper = time(nullptr);
            bHasEnoughData = DecodeRSIMDI.ProcessData(Parameters, RSIPacketBuf,
                                                      FACDecBuf, SDCDecBuf, MSCDecBuf);

            PlotManager.UpdateParamHistoriesRSIIn();
            bFrameToSend = true;

        }
        else
        {
            time_t now = time(nullptr);
            if ((now - time_keeper) > 2)
            {
                Parameters.ReceiveStatus.InterfaceI.SetStatus(NOT_PRESENT);
                Parameters.ReceiveStatus.InterfaceO.SetStatus(NOT_PRESENT);
                Parameters.ReceiveStatus.TSync.SetStatus(NOT_PRESENT);
                Parameters.ReceiveStatus.FSync.SetStatus(NOT_PRESENT);
                Parameters.ReceiveStatus.FAC.SetStatus(NOT_PRESENT);
                Parameters.ReceiveStatus.SDC.SetStatus(NOT_PRESENT);
                Parameters.ReceiveStatus.SLAudio.SetStatus(NOT_PRESENT);
            }
        }
    }
    else
    {
        //非网络
    }

    if(0)//查看打印消息的
    {

        //    for(size_t i = 0 ;i < Parameters.iNumServices; i++)
        //    {
        //        // 参数中的成员
        //        int iLenPartA = Parameters. Stream[i].iLenPartA;
        //        int iLenPartB = Parameters. Stream[i].iLenPartB;
        //        /*
        //        rx 流0:Stream.iLenPartA = 0  ;Stream.iLenPartB = 669
        //        rx 流1:Stream.iLenPartA = 0  ;Stream.iLenPartB = 69

        //        rx 流0:Stream.iLenPartA = 0  ;Stream.iLenPartB = 669
        //        rx 流1:Stream.iLenPartA = 0  ;Stream.iLenPartB = 69
        //        */
        //        cout<<"rx 流"<<i << ":Stream.iLenPartA = "<< iLenPartA
        //           << "  ;Stream.iLenPartB = "<< iLenPartB<<endl;
        //    }
    }




    switch (eReceiverMode)
    {
    case RM_DRM:
    {
        //如果 上面没有收到数据 直接跳过
        if(bHasEnoughData == false) {
            break;
        }

        // FAC | FACDecBuf size 72  经过ProcessData函数 填充程度size 变成 0 | FACUseBuf 大小一直累加(如果不消费)
        bHasEnoughDataFAC = SplitFAC.ProcessData(Parameters, FACDecBuf, FACUseBuf,
                                                 FACSendBuf);

        /* if we have an SDC block, make a copy and keep it until the next frame is to be sent */
        if (SDCDecBuf.GetFillLevel() == Parameters.iNumSDCBitsPerSFrame)
        {
            bHasEnoughDataSDC = SplitSDC.ProcessData(Parameters, SDCDecBuf, SDCUseBuf,
                                                     SDCSendBuf);
        }

        bool bHasData= false;


        //查看复用队列是否有数据
        if (!availableQueue.empty())
        {
            pEQueuebuff = availableQueue.front();

            availableQueue.pop();
        }
        else
        {
            cout<<"availableQueue== empty()" <<endl;
            return;
        }
        if(pEQueuebuff == nullptr)
        {
            cout<<"pEQueuebuff == nullptr" <<endl;
            return;
        }

        for (int i = 0; i < int(MSCDecBuf.size()); i++)
        {
            if(! SplitMSC[i].getDoinitflag())
            {
                int size=((*pEQueuebuff))[i].QueryWriteBuffer()->size();
                if(size<SplitMSC[i].GetOutputBlockSize())
                    ((*pEQueuebuff))[i].Init(SplitMSC[i].GetOutputBlockSize());

            }

            bHasData |= SplitMSC[i].ProcessData(Parameters, MSCDecBuf[i],
                                                MSCUseBuf[i], (*pEQueuebuff)[i]);

        }
        this->inUseQueue.push(pEQueuebuff);

        //
        bHasEnoughData = false;

        break;
    }

    case RM_AM:
        SplitAudio.ProcessData(Parameters, AMAudioBuf, AudSoDecBuf, AMSoEncBuf);
        break;
    case RM_FM:
        SplitAudio.ProcessData(Parameters, AMAudioBuf, AudSoDecBuf, AMSoEncBuf);
        break;
    case RM_NONE:
        break;
    }

    // cout<<"FAC buff" << ": "<<" size= "<< FACSendBuf.GetFillLevel() <<endl;  //###?

    //    /* Decoding 可以显示出 节目需要的解码信息 */
    while (bEnoughData) // TODO break if stop requested
    {
        touchWatchdogFile();

        /* Init flag */
        //进入函数 bEnoughData:true ，每次先变成 假 ，如果还有当前流有数据， 设置成true；
        // 比如两个流 就会循环两次
        bEnoughData = false;
        // Write output I/Q file
        if (WriteIQFile.IsRecording())
        {
            if (WriteIQFile.WriteData(Parameters, IQRecordDataBuf))
            {
                bEnoughData = true;
            }
        }

        switch (eReceiverMode)
        {
        case RM_DRM:
            UtilizeDRM(bEnoughData);

            break;
        case RM_AM:
            UtilizeAM(bEnoughData);
            break;
        case RM_FM:
            UtilizeFM(bEnoughData);
            break;
        case RM_NONE:
            break;
        }
    }
    /*
            server num = 2
        **/
    // 超帧

    //发送给 发射机ok -----


}

void CSubDRMReceiver::LoadSettings()
{
    if (pSettings == nullptr) return;
    CSettings& s = *pSettings;

    CDRMReceiver::LoadSettings();   // 调用父类

    /* Upstream RSCI if any  Receiver */
    string str = s.Get("Receiver", "rsiin");

    if(str.empty()) { std::cout<<"sub  UDP  error"<<std::endl;}
    else { DEBUG_COUT(" UDP = "<<QString::fromStdString(str)); }

    if (str == "")
    {
        /* Input from file if any */
        str = s.Get("command", string("fileio"));

        if (str == "") {
            /* Sound In device */
            str = s.Get("Receiver", "snddevin", string());
            std::cout<<"snddevin.str = "<< str << std::endl;
            if(str == "") {
                vector<string> vn,vd;
                EnumerateInputs(vn, vd, str);
            }
        }
    }
    SetInputDevice(str);   //TODO:  如果多个子接收机 IP应该是从外部获取的(而不是从文件中读取)

}




//void CSubDRMReceiver::putInUseBuffer(CSingleBuffer<_BINARY> &pushBuff)
//{

//}
