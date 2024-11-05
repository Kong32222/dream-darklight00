#include "cmaindrmreceiver.h"

//CMainDRMReceiver::CMainDRMReceiver()
//{

//}

CMainDRMReceiver::CMainDRMReceiver(CSettings *pSettings)
    :CDRMReceiver(pSettings),
      m_MSCmonitorBuf(MAX_NUM_STREAMS),
      m_LoopBack()

{
    ipList.clear();          // 清空所有连接的源
    startTimer();            // 定时器: 启动检测所有子接收机
    this->Threadsused = 0;   // 子接收机个数

}



void CMainDRMReceiver::checkTimerouts()
{
    while (true)
    {
        QDateTime currentTime = QDateTime::currentDateTime();
        QList<QPair<QHostAddress, quint16>> toRemove;

        int timerOUT = 5;  // 5s
        // 遍历所有客户端，检查是否超时
        for (auto it = clientLastActive.begin(); it != clientLastActive.end() ; ++it)
        {
            // 超过5 秒未发送消息
            if (it.value().secsTo(currentTime) >  timerOUT)
            {
                QString message1 = "Client " + it.key().first.toString()
                        + " : ["+ QString::number(it.key().second ) + "] has timed out.";

                // qDebug()<< message1;
            }
            else
            {
                // qDebug()<< "NO timed out";
            }
        }
        // 移除超时的客户端
        for (const auto &client : toRemove)
        {
            QString entry = QString("remove IP: [%1], Port: [%2]")
                    .arg(client.first.toString()) .arg(client.second);

            qDebug()<< entry;

            clientLastActive.remove(client);
        }
        // printf  ALL  client
        // printClientLastActive(this->clientLastActive);

        sleep(1);
    }
}

void CMainDRMReceiver::printClientLastActive(const QHash<QPair<QHostAddress, quint16>, QDateTime> &clientLastActive)
{
    // 遍历QHash
    for (auto it = clientLastActive.begin(); it != clientLastActive.end(); ++it)
    {
        // 获取键（QPair<QHostAddress, quint16>）
        QPair<QHostAddress, quint16> key = it.key();
        // 获取值（QDateTime）
        QDateTime lastActive = it.value();
        // 打印IP地址、端口和时间
        qDebug() << "IP:" << key.first.toString()
                 << "Port:"<< key.second
                 << "Last Active:" << lastActive.toString(Qt::ISODate);
    }
}

int CMainDRMReceiver::ProcessAudioFromCharArray(unsigned char* inputData,
                                                const size_t dataSize,
                                                CVectorEx<_REAL>* outputData)
{

    // 确保输入数据的长度是偶数，因为每两个char将合并成一个short
    if (dataSize % 2 != 0 )
    {
        // cout<<"确保输入数据的长度是偶数 "<< endl;
        return -1;
    }
    if(dataSize < 0)
    {
        // cout<<" dataSize = 0 ;数据不完整，处理错误 "<< endl;
        return -2;
    }
    size_t i = 0;
    // 每两个char构成一个short
    for (i = 0; i < dataSize; i += 2)
    {

        // 结合两个8位char为一个16位short
        short sample = (static_cast<short>(inputData[i + 1]) << 8)
                | (static_cast<unsigned char>(inputData[i]));

        // 保存转换后的short值到输出数据
        (*outputData)[i] = sample;

    }
    return dataSize;
}

CMainDRMReceiver::~CMainDRMReceiver()
{
    // 在析构函数中释放每个 managementSources 中的资源
    for (auto& ms : managementSources)
    {
        ms.stopThread();
    }
}

void CMainDRMReceiver::process()
{

    bool bFrameToSend = false;
    bool bEnoughData = true;

    //38400/2 = 19200;  48000 * 0.4 =  19200 得出每帧的样本数

    // IP流数据源类   内部 有 子接受机成员
    shared_ptr<IPStreamDataSource> ipStreamDataSource = nullptr;
    string IPAddressPort;


    //下面可以展开
    /* Input - from upstream RSCI or input and demodulation from sound card / file */
    {
#if 0
        if (pUpstreamRSCI->GetInEnabled())
        {

            RSIPacketBuf.Clear();
            pUpstreamRSCI->ReadData(Parameters, RSIPacketBuf);

#if 1   /************* 检查 接收机源  ************************/
            CPacketSource* pSource = pUpstreamRSCI->Getsource();  //获取 发送者IP端口: 添加到客户端列表

            // 数据源 基类接口 (子接收机)
            shared_ptr<IDataSource> Sourceptr = nullptr;

            // 查看是否存在.
            Sourceptr = m_ChannelManager.findDataSourceByAccessInfo(pSource->sendIP);

            // 1. 检查是否存在 ,等于空 则表明是第一次发送信息
            if(Sourceptr == nullptr)
            {
                // new 数据源    ipStreamDataSource-> 当前数据源
                ipStreamDataSource = make_shared<IPStreamDataSource>(pSource->sendIP);

                // m_DRMReceiver-> LoadSetttings();


                // 初始化 当前源的接收机
                ipStreamDataSource->m_DRMReceiver.InitReceiverMode();
                ipStreamDataSource->m_DRMReceiver.SetInStartMode();

                // 添加到管理数据源列表中
                IPAddressPort = pSource->sendIP + std::to_string(pSource->sendPort);
                m_ChannelManager.addDataSource( IPAddressPort, ipStreamDataSource);

            }
            else
            {
                // 将 IDataSource 转换为 IPStreamDataSource  将基类指针转换为派生类指针
                ipStreamDataSource = dynamic_pointer_cast<IPStreamDataSource>(Sourceptr);

                // 如果转换失败，ipStreamDataSource 会是 nullptr
                if (ipStreamDataSource == nullptr)
                {
                    cout << "Error: The data source found is not of type IPStreamDataSource." << endl;
                }

                // 更新客户端的最后通讯时间
                QHostAddress senderIp;
                quint16 senderPort;
                senderIp.setAddress(QString::fromStdString(pSource->sendIP));
                senderPort = pSource->sendPort;
                clientLastActive[QPair<QHostAddress, quint16>(senderIp, senderPort)]
                        = QDateTime::currentDateTime();
            }

#endif
            if (RSIPacketBuf.GetFillLevel() > 0)
            {

                time_keeper = time(nullptr);
                // 换成 当前源的接收机 以及参数 及缓冲区;
                CDecodeRSIMDI* DecodeRSIMDICurrent =
                        ipStreamDataSource->m_DRMReceiver.GetDecodeRSIMDI();

                DecodeRSIMDICurrent->ProcessData((*ipStreamDataSource->m_DRMReceiver.GetParameters()),
                                                 RSIPacketBuf,
                                                 (*ipStreamDataSource->m_DRMReceiver.GetFACDecBuf()),
                                                 (*ipStreamDataSource->m_DRMReceiver.GetSDCDecBuf()),
                                                 (*ipStreamDataSource->m_DRMReceiver.GetMSCDecBuf()));

                DecodeRSIMDI.ProcessData(Parameters, RSIPacketBuf, FACDecBuf, SDCDecBuf,
                                         MSCDecBuf);

                PlotManager.UpdateParamHistoriesRSIIn();
                bFrameToSend = true;
            }
            else
            {
                time_t now = time(nullptr);
                if ((now - time_keeper) > 2)
                {
                    CParameter& ParameterCurrent = *ipStreamDataSource->m_DRMReceiver.GetParameters();

                    // (当前源的接收机)超时检测  Parameters(主接收机) ParameterCurrent
                    ParameterCurrent.ReceiveStatus.InterfaceI.SetStatus(NOT_PRESENT);
                    ParameterCurrent.ReceiveStatus.InterfaceO.SetStatus(NOT_PRESENT);
                    ParameterCurrent.ReceiveStatus.TSync.SetStatus(NOT_PRESENT);
                    ParameterCurrent.ReceiveStatus.FSync.SetStatus(NOT_PRESENT);
                    ParameterCurrent.ReceiveStatus.FAC.SetStatus(NOT_PRESENT);
                    ParameterCurrent.ReceiveStatus.SDC.SetStatus(NOT_PRESENT);
                    ParameterCurrent.ReceiveStatus.SLAudio.SetStatus(NOT_PRESENT);

                    // (原来)超时检测
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

            if (WriteIQFile.IsRecording())
            {
                /* Receive data in RecDataBuf */
                // 文件 Decode 换成 当前源的接收机 以及参数 及缓冲区;
                ReceiveData.ReadData(Parameters, RecDataBuf);

                /* Split samples, one output to the demodulation, another for IQ recording */
                if (SplitForIQRecord.ProcessData(Parameters, RecDataBuf, DemodDataBuf, IQRecordDataBuf))
                {
                    bEnoughData = true;
                }
            }
            else
            {
                /* No I/Q recording then receive data directly in DemodDataBuf */
                ReceiveData.ReadData(Parameters, DemodDataBuf);
            }

            // DemodulateDRM 增加一个接收机的 RecDataBuf / DemodDataBuf 多个接收来源，走解调环节
            switch (eReceiverMode)
            {
            case RM_DRM:
                DemodulateDRM(bEnoughData);
                DecodeDRM(bEnoughData, bFrameToSend);
                break;
            case RM_AM:
                DemodulateAM(bEnoughData);
                DecodeAM(bEnoughData);
                break;
            case RM_FM:
                DemodulateFM(bEnoughData);
                DecodeFM(bEnoughData);
                break;
            case RM_NONE:
                break;
            }
        }

#endif   // readdata
    }

    //------------------------- 回环
    if(this->m_dataSourceType == DataSourceType::Loopback)
    {
        // m_LoopBack  OutputBlockSize: 640
        // cout<<"m_LoopBack OutputBlockSize: "<<m_LoopBack.GetOutputBlockSize()<<endl;
        m_LoopBack.ReadData(Parameters, DemodDataBuf);
        DemodulateDRM(bEnoughData);         //输入源是: DemodDataBuf
        DecodeDRM(bEnoughData, bFrameToSend);

    }

#if 0
    // 配置 ipStreamDataSource   MSCBuff
    std::vector<CSingleBuffer<_BINARY>>& subMSCDecBuf =
            *(ipStreamDataSource->m_DRMReceiver.GetMSCDecBuf());

    // CVectorEx<_BINARY>* xx= subMSCDecBuf[0].QueryWriteBuffer();
    // cout<< "subMSCDecBuf size = " << xx->size() << endl;   //5904


    // concatenateRows
    vector<int> Srcstream;
    Srcstream.push_back(0);// 按照puch的顺序 配置流
    Srcstream.push_back(1);// 按照puch的顺序 配置流
    Srcstream.push_back(2);// 按照puch的顺序 配置流
    Srcstream.push_back(3);// 按照puch的顺序 配置流

    vector<int> monitorStream;
    monitorStream.push_back(0);//组装要监测的流
    monitorStream.push_back(1);//组装要监测的流
    monitorStream.push_back(2);//组装要监测的流
    monitorStream.push_back(3);//组装要监测的流


    // configure
    configureStream(subMSCDecBuf,    /**选择 intput buff MSC 流 **/
                    Srcstream,       /**选择 intput MSC 流的服务下标顺序 **/
                    m_MSCmonitorBuf, /**选择 output buff MSC 流 **/
                    monitorStream);  /**选择 output MSC 流的服务下标顺序 **/

#endif // 配置 源

    /* Split the data for downstream RSCI and local processing. TODO make this conditional */

    switch (eReceiverMode)
    {
    case RM_DRM:
    {
        // int facsize = FACDecBuf.GetFillLevel();// 0
        SplitFAC.ProcessData(Parameters, FACDecBuf, FACUseBuf, FACSendBuf);

        /* if we have an SDC block, make a copy and keep it until the next frame is to be sent */
        if (SDCDecBuf.GetFillLevel() == Parameters.iNumSDCBitsPerSFrame)
        {
            SplitSDC.ProcessData(Parameters, SDCDecBuf, SDCUseBuf, SDCSendBuf);
        }

        for (int i = 0; i < int(MSCDecBuf.size()); i++)
        {
            SplitMSC[i].ProcessData(Parameters, MSCDecBuf[i], MSCUseBuf[i],
                                    MSCSendBuf[i]);     //源码
        }
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


    /* Decoding  平常 等于1  bEnoughData == 1 */
    while (bEnoughData) // TODO break if stop requested
    {
        touchWatchdogFile();

        /* Init flag */
        bEnoughData = false;

        // Write output I/Q file
        if (WriteIQFile.IsRecording())
        {
            if (WriteIQFile.WriteData(Parameters, IQRecordDataBuf))
            {
                bEnoughData = true;
            }
        }

        switch (eReceiverMode)  // 0 == RM_DRM
        {
        case RM_DRM:
            UtilizeDRM(bEnoughData);  // Decode  xxUseBuf

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


#if 0
    /* Output to downstream RSCI */
    if (downstreamRSCI.GetOutEnabled())
    {
        switch (eReceiverMode)
        {
        case RM_DRM:
            if (Parameters.eAcquiState == AS_NO_SIGNAL)
            {
                /* we will get one of these between each FAC block, and occasionally we */
                /* might get two, so don't start generating free-wheeling RSCI until we've. */
                /* had three in a row */
                if (FreqSyncAcq.GetUnlockedFrameBoundary())
                {
                    if (iUnlockedCount < MAX_UNLOCKED_COUNT)
                        iUnlockedCount++;
                    else
                        downstreamRSCI.SendUnlockedFrame(Parameters);
                }
            }
            else if (bFrameToSend)
            {
                downstreamRSCI.SendLockedFrame(Parameters, FACSendBuf, SDCSendBuf, MSCSendBuf);
                iUnlockedCount = 0;
                bFrameToSend = false;
            }
            break;
        case RM_AM:
        case RM_FM:
            /* Encode audio for RSI output */
            if (AudioSourceEncoder.ProcessData(Parameters, AMSoEncBuf, MSCSendBuf[0]))
                bFrameToSend = true;

            if (bFrameToSend)
                downstreamRSCI.SendAMFrame(Parameters, MSCSendBuf[0]);
            break;
        case RM_NONE:
            break;
        }
    }
    /* Check for RSCI commands */
    if (downstreamRSCI.GetInEnabled())
    {
        downstreamRSCI.poll();
    }

#endif
    /* Play and/or save the audio */
    if (iAudioStreamID != STREAM_ID_NOT_USED || (eReceiverMode == RM_AM) ||
            (eReceiverMode == RM_FM))
    {
           AudSoDecBuf.Get(WriteData.GetInputBlockSize());
//        if (WriteData.WriteData(Parameters, AudSoDecBuf))
//        {
//            bEnoughData = true;
//        }
    }

}

void CMainDRMReceiver::LoadSettings()
{

    if (pSettings == nullptr) return;
    CSettings& s = *pSettings;

    CDRMReceiver::LoadSettings();  //调用了父类

    string str = s.Get("Receiver", "rsiin");  // Receiver
    if(str.empty())
    {
        std::cout<<"CMainDRMReceiver UDP  error"<<std::endl;
    }
    else
    {
        DEBUG_COUT("CMainDRMReceiver UDP = "<<QString::fromStdString(str));
    }

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
    //SetInputDevice(str);

    // CDRMReceiver::LoadSettings();  //上面 调用了父类

}


int CMainDRMReceiver::configureStream(std::vector<CSingleBuffer<_BINARY>>& src,
                                      const vector<int> &srcrows,
                                      std::vector<CSingleBuffer<_BINARY>>& dest,
                                      const vector<int> &destrows)
{
    // m_MSCmonitorBuf; 监测 MSC BUFF
    // MSC buff size = 4
    if(dest.size() < src.size())
    {
        cout<<"error: dest.size() < src.size()"<<dest.size() <<", "<< src.size()<<endl;
        return -1;
    }


    std::vector<int>::const_iterator destIndex = destrows.begin(); // 控制第几个
    for (int row : srcrows)
    {
        if(row < 0 || row >= (int)src.size())
        {
            cerr<<"row error"<<endl;
            return -2;
        }
        // MSC BUFF
        CSingleBuffer<_BINARY>* singleBufferInput = &src[row];
        CVectorEx<_BINARY>* InputMSCBuf = singleBufferInput->QueryWriteBuffer();

        int InputMSCBufLength = InputMSCBuf->Size();
        int iOutputBlockSize = InputMSCBufLength * SIZEOF__BYTE;


        // 监测 MSC BUFF
        CSingleBuffer<_BINARY>* singleBufferOutput = &dest[(*destIndex++)];

        singleBufferOutput->Init(iOutputBlockSize);
        CVectorEx<_BINARY>* OutputMSCBuf = singleBufferOutput->QueryWriteBuffer();

        // capacity
        if((int)OutputMSCBuf->capacity() >= InputMSCBufLength)
        {

            /* Copy stream data */
            InputMSCBuf->ResetBitAccess();
            OutputMSCBuf->ResetBitAccess();
            for (int i = 0; i < InputMSCBufLength / SIZEOF__BYTE; i++)
            {
                OutputMSCBuf->Enqueue(InputMSCBuf->Separate(SIZEOF__BYTE),
                                      SIZEOF__BYTE);
            }

        }
        else
        {
            cerr<<"error: 输出MSC的大小 小于 输入的MSC大小 "<<endl;
            return -1;
        }


    }

    return 0;
}


// 主接收机 往 子接收机 发送消息
void CMainDRMReceiver::sendMessageToManagement(const string &ip, uint16_t port,
                                               CSingleBuffer<_BINARY>& data)
{

    for (int i = 0; i < Threadsused; ++i)
    {
        if (managementSources[i].matches(ip, port))
        {
            DEBUG_COUT("找到对应的IP 把对应的数据 传给子接收机中的buff中 ip = "
                       << QString::fromStdString(ip)
                       << "port :"<< port );

            // 把对应的数据 传给子接收机中的buff中
            managementSources[i].pushMessage(data);
            break;
        }
    }
}



void timerFunction()
{
    DEBUG_COUT("检测所有子接收机 timer!!!!");

    while (true)
    {
        //当前线程休眠 1 分钟（60,000 毫秒）。这行代码使函数每 1 分钟检查一次 ipList
        //std::this_thread::sleep_for(std::chrono::minutes(1));
        //获取当前时间
        auto now = std::time(nullptr);

        // 使用 std::remove_if 算法遍历 ipList，并标记所有符合条件的元素
        ipList.erase(std::remove_if(ipList.begin(), ipList.end(), [now](const IPAddress& addr)
        {
            // 判断 now 和 addr.lastActiveTime 之间的时间差是否超过 30 秒
            return std::difftime(now, addr.lastActiveTime) > 30;

        }), ipList.end());

        printIPList();
        sleep(20);

    }

}









