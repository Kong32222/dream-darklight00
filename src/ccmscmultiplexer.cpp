#include "ccmscmultiplexer.h"

/* ********新函数 复用  ********/
void CMSCmultiplexer::ProcessDataInternal(CParameter& Parameters)
{
    Parameters.Lock();              // 不能多次锁
    // 计算 高低保护 (复用)
    MultiplexerData1(this->pvecInputData, this->pvecOutputData, this->StreamPos);
    Parameters.Unlock();

}

/* my复用 初始化函数 */
void CMSCmultiplexer::InitInternal(CParameter& Parameters)
{

    Parameters.Lock();
    for (size_t i = 0; i < MAX_NUM_STREAMS; i++)
    {
        StreamPos[i] = GetStreamPos(Parameters, i);

        // 记录每个流中 高低保护个数
        m_veciOutputBlockSize[i] = StreamPos[i].iLenHigh + StreamPos[i].iLenLow;

    }

    /* Set input output block size */
    //TODO: 修改成 一个数; (所有流加起来的 Size)
    int MSCsize = 0 ;
    for(size_t i = 0; i < MAX_NUM_STREAMS; i++)
    {
        MSCsize +=m_veciOutputBlockSize[i];
    }
    // cout<<"MSCsize = "<< MSCsize<<endl;  //5904
    this->MuxBuf.Init(MSCsize*2);  //以防万一: size * 2   ;  更保险的是: 16 * 1024
    Parameters.Unlock();
}



//void CMSCmultiplexer::MultiplexerData(std::vector<CSingleBuffer<_BINARY>>* pMSCUseBuf,
//    CSingleBuffer<_BINARY>* pMuxBuf, SStreamPos* StreamPos)
//{
//    // static int count = 0;
//    /* ************  安全检测   ********************************************  */
//    if (pMSCUseBuf == nullptr) {
//        std::cerr << "Error: Input data pointer is null." << std::endl;
//        return;
//    }
//    if (pMuxBuf == nullptr){
//        std::cerr << "Error: Output buffer pointer is null." << std::endl;
//        return;
//    }
//    if (StreamPos == nullptr){
//        std::cerr << "Error: Stream position pointer is null." << std::endl;
//        return;
//    }


//    /* *************    计算长度   ******************************* */
//    const int tempID = this->streamID;

//    CSingleBuffer<_BINARY>& InputBuf = (*pMSCUseBuf)[tempID];


//    const int  LenLow     = this->StreamPos[tempID].iLenLow;
//    const int  LenHigh    = this->StreamPos[tempID].iLenHigh;
//    const int  OffsetLow  = this->StreamPos[tempID].iOffsetLow;
//    const int  OffsetHigh = this->StreamPos[tempID].iOffsetHigh;



//    /* ************* 流中没有数据size = 0 函数返回 ******************************* */
//    if ((LenLow + LenHigh) <= 0)
//    {
//        // std::cout << "流" << tempID << "没有数据. return " << std::endl;
//        return;
//    }
//    else
//    {

//    }



//    //1.  输入迭代器 用于拷贝
//    CVectorEx<_BINARY>* pDataBuff = InputBuf.QueryWriteBuffer();
//    std::vector<_BINARY>::iterator It_VecInput = pDataBuff->begin();


//    //2. 输出迭代器 用于拷贝
//    CVectorEx<_BINARY>* pMuxBuff = pMuxBuf->QueryWriteBuffer();
//    vector<_BINARY>::iterator it_VecOutput = pMuxBuff->begin();

//    if(pMuxBuff->size() == 0 || pMuxBuff->Size()==0 || pMuxBuff->capacity()==0 )
//    {
////        std::cerr<<"pMuxBuff size = "  << pMuxBuff->size()
////                <<", pMuxBuff Size = " << pMuxBuff->Size()
////               <<", pMuxBuff capacity" << pMuxBuff->capacity()<<std::endl;
//        return;
//    }

//    /*****************   拷贝 高保护 ************************/

//    for (int i = 0; i < LenHigh; i++)
//    {
//         it_VecOutput[i + OffsetHigh] = It_VecInput[i];
//        //*(it_VecOutput + i + OffsetHigh) = *(It_VecInput + i);
//    }

//    /* *****************   拷贝 低保护 ************************ */
//    int  i = 0;
//    for (i = 0; i < LenLow; i++)
//    {
//        it_VecOutput[i + OffsetLow] = It_VecInput[i + LenHigh];
//        //*(it_VecOutput + i + OffsetLow) = *(It_VecInput + i + LenHigh);
//    }

//    return;
//}

void CMSCmultiplexer::MultiplexerData1(CVectorEx<_BINARY>* pVecInputData ,
                                       CVectorEx<_BINARY>* pMuxBuf,
                                       CMSCmultiplexer::SStreamPos *StreamPos)
{

    /* ************  安全检测   ********************************************  */
    if (pMuxBuf == nullptr){
        std::cerr << "Error: Output buffer pointer is null." << std::endl;
        return;
    }
    if (StreamPos == nullptr){
        std::cerr << "Error: Stream position pointer is null." << std::endl;
        return;
    }


    /* *************    计算长度   ******************************* */
    // cout<<streamID<<";tempID = "<<tempID<< endl;
    const int  LenLow     = this->StreamPos[streamID].iLenLow;
    const int  LenHigh    = this->StreamPos[streamID].iLenHigh;
    const int  OffsetLow  = this->StreamPos[streamID].iOffsetLow;
    const int  OffsetHigh = this->StreamPos[streamID].iOffsetHigh;


    /* ************* 流中没有数据size = 0 函数返回 ******************************* */
    if ((LenLow + LenHigh) <= 0)
    {
        // DEBUG_COUT("流" << streamID << "没有数据. return ");
        return;
    }
    {
        //检查debug
        //        DEBUG_COUT("--------");
        //        DEBUG_COUT("流" << streamID << ":LenHigh:\t"<<LenHigh);
        //        DEBUG_COUT("流" << streamID << ":LenLow:\t"<<LenLow);
        //        DEBUG_COUT("流" << streamID << ":OffsetLow:\t"<<OffsetLow);
        //        DEBUG_COUT("流" << streamID << ":OffsetHigh:\t"<<OffsetHigh);
    }



    //1.  输入迭代器 用于拷贝
    CVectorEx<_BINARY>* pDataBuff = pVecInputData;
    std::vector<_BINARY>::iterator It_VecInput = pDataBuff->begin();


    //2. 输出迭代器 用于拷贝
    CVectorEx<_BINARY>* pMuxBuff = pMuxBuf ;
    vector<_BINARY>::iterator it_VecOutput = pMuxBuff->begin();


    /*****************   拷贝 高保护 ************************/

    for (int i = 0; i < LenHigh; i++)
    {
        it_VecOutput[i + OffsetHigh] = It_VecInput[i];
        //*(it_VecOutput + i + OffsetHigh) = *(It_VecInput + i);
    }

    /* *****************   拷贝 低保护 ************************ */
    int  i = 0;
    for (i = 0; i < LenLow; i++)
    {
        it_VecOutput[i + OffsetLow] = It_VecInput[i + LenHigh];
        //*(it_VecOutput + i + OffsetLow) = *(It_VecInput + i + LenHigh);
    }
    // static int count = 0;
    // std::cout<<count++<< ":拷贝的个数i = "<< i <<std::endl;  //上面如果( 低保护 和高保护):没有数据 提前return

    return;
}



/*
 1.初始化位置信息为零。
 2.如果流ID不等于STREAM_ID_NOT_USED（即流ID被使用），则进行以下操作：
    获取流的高低保护部分的长度（以字节为单位），并将其存储在StPos.iLenHigh和StPos.iLenLow中。
    计算低保护部分的起始偏移量，需要考虑所有高保护部分的长度。这部分将会对StPos.iOffsetLow进行赋值。
    计算实际的流起始位置，需要考虑所有比当前流ID小的流的高保护部分和低保护部分的长度。
 这部分将会对StPos.iOffsetHigh 和 StPos.iOffsetLow进行赋值.
    如果使用了分层调制（hierarchical modulation），则会进行特殊处理。
 如果当前流ID为0，则数据位于传入数据块的开头，否则会将所有偏移量加上分层帧的长度。
 3.进行可能性检查，如果发现参数值不合理，则将所有位置信息都设置为零。
*/

// 发射机 my  赋值了一份
CMSCmultiplexer::SStreamPos CMSCmultiplexer::GetStreamPos(CParameter& Param,
                                                          const int iStreamID)
{
    CMSCmultiplexer::SStreamPos	StPos;


    /* Init positions with zeros (needed if an error occurs) */
    StPos.iOffsetLow = 0;
    StPos.iOffsetHigh = 0;
    StPos.iLenLow = 0;
    StPos.iLenHigh = 0;

    if (iStreamID != STREAM_ID_NOT_USED)
    {
        /* Length of higher and lower protected part of audio stream (number of bits)
             Stream是一个Cstream类型的数组中的int类型(iLenPartA)* 8  iLenPartA == 多少字节* * bit ==
            StPos.iLenHigh  单位是bit*8 == [总位数]
        */

        // 输入数据的 高、低保护数据长度
        StPos.iLenHigh = Param.Stream[iStreamID].iLenPartA * SIZEOF__BYTE;
        StPos.iLenLow = Param.Stream[iStreamID].iLenPartB * SIZEOF__BYTE;


        /* Byte-offset of higher and lower protected part of audio stream --- */
        /* Get active streams */
        set<int> actStreams;
        Param.GetActiveStreams(actStreams);  //尾插 ： actStr.insert(Service[i].AudioParam.iStreamID );

        /* Get start offset for lower protected parts in stream. Since lower
           protected part comes after the higher protected part, the offset
           must be shifted initially by all higher protected part lengths
           (iLenPartA of all streams are added) 6.2.3.1
           获取流中较低受保护部件的启动偏移量。因为低受保护部分位于高保护部分之后，即偏移量
            必须首先移动所有较高的保护部件长度 (添加所有流的iLenPartA) */
        StPos.iOffsetLow = 0;
        set<int>::iterator i;

        //计算 低保护的起始点
        for (i = actStreams.begin(); i != actStreams.end(); i++)
        {
            StPos.iOffsetLow += Param.Stream[*i].iLenPartA * SIZEOF__BYTE;
        }

        /* Real start position of the streams 流的实际起始位置 */
        StPos.iOffsetHigh = 0;

        //复加 高、低保护的偏移量
        for (i = actStreams.begin(); i != actStreams.end(); i++)
        {
            if (*i < iStreamID)
            {
                //设置 高保护 低保护的偏移量
                StPos.iOffsetHigh += Param.Stream[*i].iLenPartA * SIZEOF__BYTE;
                StPos.iOffsetLow += Param.Stream[*i].iLenPartB * SIZEOF__BYTE;
            }
        }

        /* Special case if hierarchical modulation is used
         使用分层调制的特殊情况 */
        if (((Param.eMSCCodingScheme == CS_3_HMSYM) ||
             (Param.eMSCCodingScheme == CS_3_HMMIX)))
        {
            std::cout << "使用分层调制的特殊情况" << std::endl;

            if (iStreamID == 0)
            {
                /* Hierarchical channel is selected. Data is at the beginning
                   of incoming data block */
                StPos.iOffsetLow = 0;
            }
            else
            {
                /* Shift all offsets by the length of the hierarchical frame. We
                   cannot use the information about the length in
                   "Stream[0].iLenPartB", because the real length of the frame
                   is longer or equal to the length in "Stream[0].iLenPartB" */
                StPos.iOffsetHigh += Param.iNumBitsHierarchFrameTotal;
                StPos.iOffsetLow += Param.iNumBitsHierarchFrameTotal -
                        /* We have to subtract this because we added it in the
                                                                               for loop above which we do not need here */
                        Param.Stream[0].iLenPartB * SIZEOF__BYTE;
            }
        }

        /* Possibility check  可能性检查 ------------------------------------------------ */
        /* Test, if parameters have possible values 测试参数是否有可能的值 */
        if ((StPos.iOffsetHigh + StPos.iLenHigh > Param.iNumDecodedBitsMSC) ||
                (StPos.iOffsetLow + StPos.iLenLow > Param.iNumDecodedBitsMSC))
        {

            std::cout << "可能性检查 可能出错 " << std::endl;
            std::cout << "重要---Param.NumDecodedBitsMSC = " << Param.iNumDecodedBitsMSC << std::endl;

            std::cout << "重要---StPos.iOffsetHigh + StPos.iLenHigh = " << StPos.iOffsetHigh + StPos.iLenHigh << std::endl;
            std::cout << "重要---StPos.iOffsetLow + StPos.iLenLow = " <<
                         StPos.iOffsetLow + StPos.iLenLow << std::endl;

            /* Something is wrong, set everything to zero
             出了什么问题，把一切都设为零*/
            StPos.iOffsetLow = 0;
            StPos.iOffsetHigh = 0;
            StPos.iLenLow = 0;
            StPos.iLenHigh = 0;
        }

    }

    return StPos;
}
