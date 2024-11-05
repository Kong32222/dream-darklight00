#include "messagequeue.h"


//拷贝MSC【0-3】流
int Copy_stream(std::vector<CSingleBuffer<_BINARY>>* pMSCUseBuf,
                CMSCmultiplexer* pMSCmultiplexer)
{

    // 检查指针是否有效
    if (pMSCUseBuf)
    {
        // output 发射机 容器
        std::vector<CSingleBuffer<_BINARY>>& MSCUseBuf = pMSCmultiplexer->MSCUseBuf;

        // 遍历两个容器的 每个成员MSC
        for (int i = 0; i < 4/*MAX_NUM_STREAMS*/; ++i)
        {
            // 访问 接收机 pMSCUseBuf 中的第i 个成员
            CSingleBuffer<_BINARY>& singleBuffer1 = (*pMSCUseBuf)[i];
            CVectorEx<_BINARY>* InputMSCBuf = singleBuffer1.QueryWriteBuffer();

            int InputMSCBufLength = InputMSCBuf->Size();
            // cout<<"InputMSCBufLength = "<< InputMSCBufLength << endl; //DEBUG()

            // 访问 发射机 MSCUseBuf 中的第 i 个成员
            CSingleBuffer<_BINARY>* singleBuffer2 = &(MSCUseBuf[i]);
            CVectorEx<_BINARY>* OutputMSCBuf = singleBuffer2->QueryWriteBuffer();

            if((int)OutputMSCBuf->capacity() < InputMSCBufLength)
            {
                /* Copy stream data */
                cout << "InputMSCBufLength = " << InputMSCBufLength << endl;
                InputMSCBuf->ResetBitAccess();
                OutputMSCBuf->ResetBitAccess();

                OutputMSCBuf->Init(InputMSCBufLength);

            }
            else
                OutputMSCBuf->Init(static_cast<int>(OutputMSCBuf->capacity()));


            for (int i = 0; i < InputMSCBufLength / SIZEOF__BYTE; i++)
            {
                OutputMSCBuf->Enqueue(InputMSCBuf->Separate(SIZEOF__BYTE),
                                      SIZEOF__BYTE);
            }
            //veciOutputBlockSize[i] = iStreamLen;

        }
    }
    return 0;
}

//拷贝MSC【0-3】流
int Copy_stream(std::vector<CSingleBuffer<_BINARY>>* pMSCUseBuf,
                std::vector<CSingleBuffer<_BINARY>>* pTxMSCUseBuf)
{
    //////

    // 检查 源接收机 指针是否有效
    if (pMSCUseBuf)
    {
        // 遍历两个容器的 每个成员MSC
        for (int i = 0; i <  MAX_NUM_STREAMS ; ++i)
        {
            // 访问 接收机 pMSCUseBuf 中的第i 个成员
            CSingleBuffer<_BINARY>* singleBufferInput = &(*pMSCUseBuf)[i];
            CVectorEx<_BINARY>* InputMSCBuf = singleBufferInput->QueryWriteBuffer();

            // 打印的效果 : 5904
//            std::cout<<"Input MSCBuf Size= "<<InputMSCBuf->Size()
//                      <<" , size = "<<InputMSCBuf->size()
//                      <<", capacity = "<<InputMSCBuf->capacity()<<std::endl;
            int InputMSCBufLength = InputMSCBuf->Size();
            // cout<<"InputMSCBufLength = "<< InputMSCBufLength << endl; //DEBUG()

            // 访问 发射机 MSCUseBuf 中的第 i 个成员
            CSingleBuffer<_BINARY>* singleBufferOutput = &(*pTxMSCUseBuf)[i];
            CVectorEx<_BINARY>* OutputMSCBuf = singleBufferOutput->QueryWriteBuffer();

            /// capacity !!
            if((int)OutputMSCBuf->capacity() >= InputMSCBufLength)
            {
                OutputMSCBuf->clear();

                /* Copy stream data */
                //cout << "InputMSCBufLength = " << InputMSCBufLength << endl;
                InputMSCBuf->ResetBitAccess();
                OutputMSCBuf->ResetBitAccess();
                for (int i = 0; i < InputMSCBufLength / SIZEOF__BYTE; i++)
                {
                    OutputMSCBuf->Enqueue(InputMSCBuf->Separate(SIZEOF__BYTE),
                                          SIZEOF__BYTE);
                }

                singleBufferOutput->Put(InputMSCBufLength);
            }
            else
            {
               return -1;
            }
            //veciOutputBlockSize[i] = iStreamLen;

        }
    }
    return 0;
}


int CopyParameter( CParameter* pParameter_recv , CParameter* pParameter_trans)
{
    if(pParameter_recv == NULL || pParameter_trans == NULL)
    {
        DEBUG_COUT("Parameter_recv is NULL || pParameter_trans is NULL");
        return -1;
    }
    //是否是主源
    if(1)
    {
        // DEBUG_COUT("主源");

        /* 修改 FAC参数   */

        //鲁棒模式
        pParameter_trans->SetWaveMode(pParameter_recv->GetWaveMode());
        //频谱占用
        pParameter_trans->SetSpectrumOccup(pParameter_recv->GetSpectrumOccup());
        // 交织长度
        pParameter_trans->SetInterleaverDepth(pParameter_recv->GetInterleaverDepth());
        //sdc  msc 映射模式
        pParameter_trans->SetSDCCodingScheme(pParameter_recv->GetSDCCodingScheme());
        pParameter_trans->SetMSCCodingScheme(pParameter_recv->GetMSCCodingScheme());

    }
    else
    {
        /* 次源 */
        if (1)
        {
            DEBUG_COUT("次源");
            /* 如果没有没有主源 ，需要根据默认参数配置--
                 防止程序刚刚运行时，主源没有连接，或者是断开）*/

        }
    }
    /* 综合  */


    /*  FAC ----------------------------------------------------------------- */
    // 增强模式标识   0

    // 当前复用帧帧号    2比特    超帧id   0 1 2
    pParameter_trans->iFrameIDTransm = pParameter_recv->iFrameIDTransm;

    // 再配置索引  3比特 见节 6.4.6.   默认是 000
    // Rfu	   2比特                  默认是 00

    // 业务标识 音频服务 数据服务
    // 综合多个源头，再设置， 从主源开始排，
    // 活跃的次元，调度表
    pParameter_trans->iNumDataService = pParameter_recv->iNumDataService;
    pParameter_trans->iNumAudioService = pParameter_recv->iNumAudioService;
    pParameter_trans->iNumServices = pParameter_recv->GetTotNumServices();

    // 短标识
    pParameter_trans->iFacShortID = pParameter_recv->GetFacShortID();
    //服务唯一标识  24比特
    pParameter_trans->iServiceID = pParameter_recv->GetServiceID();

    // SetServiceID ( fac短标识 ,唯一标识 24比特  )
    pParameter_trans->SetServiceID(
                pParameter_recv->iFacShortID, pParameter_recv->iServiceID);

    // CA indication
    pParameter_trans->Service[pParameter_trans->iFacShortID].eCAIndication
            = pParameter_recv->Service[pParameter_recv->iFacShortID].eCAIndication;

    // Language
    pParameter_trans->Service[pParameter_trans->iFacShortID].iLanguage
            = pParameter_recv->Service[pParameter_recv->iFacShortID].iLanguage;

    // Audio/Data flag
    pParameter_trans->Service[pParameter_trans->iFacShortID].eAudDataFlag
            = pParameter_recv->Service[pParameter_recv->iFacShortID].eAudDataFlag;

    //Service descriptor  业务描述标识 5
    pParameter_trans->Service[pParameter_trans->iFacShortID].iServiceDescr
            = pParameter_recv->Service[pParameter_recv->iFacShortID].iServiceDescr;


    // Rfa  0  7 比特

    // CRC  8比特


    /* SDC 参数--------------------------------------------------------------------*/

    // SDC 比特总数
    pParameter_trans->iNumSDCBitsPerSFrame = pParameter_recv->iNumSDCBitsPerSFrame;
    // AFS 索引  4比特  填入 1

    /* ----------type 0 -------  */

    // 数据体长度       7比特   给出了数据实体部分的【字节数】  （头 - 4） /8
    // 版本标识         1比特   控制接收机对实体数据的管理           0
    // 数据实体类型     4比特   对应数据实体类型的标志序号           00

    //A 保护等级
    pParameter_trans->MSCPrLe.iPartA = pParameter_recv->MSCPrLe.iPartA;
    //B 保护等级
    pParameter_trans->MSCPrLe.iPartB = pParameter_recv->MSCPrLe.iPartB;
    //iHierarch  分层 分等级
    pParameter_trans->MSCPrLe.iHierarch = pParameter_recv->MSCPrLe.iHierarch;
    // A的长度 B的长度
    for (size_t i = 0; i < pParameter_recv->GetTotNumServices(); i++)
    {
        pParameter_trans->Stream[i].iLenPartA = pParameter_recv->Stream[i].iLenPartA;
        pParameter_trans->Stream[i].iLenPartB = pParameter_recv->Stream[i].iLenPartB;
    }

    /*  type 9  < 音频信息数据实体>   ------ */
    // 设置总位数  20 + (xHE_AAC_config.size() * 8 )
    // for (size_t i = 0; i < pParameter_recv->GetTotNumServices(); i++){ 2个 }
    pParameter_trans->Service[pParameter_trans->iFacShortID].AudioParam.eAudioCoding
            = pParameter_recv->Service[pParameter_recv->iFacShortID].AudioParam.eAudioCoding;

    // xHE_AAC_config
    pParameter_trans->Service[pParameter_trans->iFacShortID].AudioParam.xHE_AAC_config
            = pParameter_recv->Service[pParameter_recv->iFacShortID].AudioParam.xHE_AAC_config;

    // 数据体长度       7比特  (总位数 -  4) / 8
    // 版本标识         1比特   0
    // 数据实体类型     4比特   9
    // 短标识           2比特     0 (猜测是 流0 )
    // 流 Id            2比特
    pParameter_trans->Service[pParameter_trans->iFacShortID].AudioParam.iStreamID
            = pParameter_recv->Service[pParameter_recv->iFacShortID].AudioParam.iStreamID;
    //实体的其余部分  CAudioParam()
    // 音频编码                    2比特
    // SBR标志                     1比特
    //音频模式                     2比特
    //音频采样率                   3比特
    //文本标志                     1比特
    //增强标志                     1比特
    //编码区域                     5比特     AC_AAC  AC_xHE_AAC 3位 ，rfa 2位
    //该 1-比特, 4-比特 和 5-比特区域留作将来定义，定义前置0 (xHE_AAC_config[i], 8)
    pParameter_trans->SetAudioParam(pParameter_trans->iFacShortID,
                                    pParameter_recv->GetAudioParam(pParameter_recv->GetFacShortID()));

    /* SDC type1 strLabel */
    pParameter_trans->Service[pParameter_trans->iFacShortID].strLabel
            = pParameter_recv->Service[pParameter_recv->iFacShortID].strLabel;
    //短标识: 该短标识与FAC提供的业务标识参数信息相关。   2 位
    //Rfu：这2比特位留作将来使用，在被定义之前应置为0。   2 位
    //标记：LenLabel
    /* 数据头 */
    //  数据体长度         7比特   lenth
    //  版本标识           1比特    0
    //  数据实体类型       4比特   Type b01

    /* Actual body */
    //Short Id            2比特  ServiceID  流0
    //rfu                 2比特    00
    //标记 标签           n比特  Parameter.Service[ServiceID].strLabel[i];


    /*type8 时间*/
    //
    pParameter_trans->bValidUTCOffsetAndSense
            = pParameter_recv->bValidUTCOffsetAndSense;
    // Parameter.eTransmitCurrentTime
    pParameter_trans->eTransmitCurrentTime
            = pParameter_recv->eTransmitCurrentTime;
    //Parameter.iYear, Parameter.iMonth, Parameter.iDay  17位
    //Parameter.iUTCHour                        5
    //Parameter.iUTCMin                         6
    //Parameter.iUTCSense,                      1
    //Parameter.iUTCOff,                        5
    // size大小
    // pParameter_trans->CellMappingTable.iSymbolBlockSize
    //     = pParameter_recv->CellMappingTable.iSymbolBlockSize;

    //pParameter_trans->CellMappingTable = pParameter_recv->CellMappingTable;

    // MSC 的输出比特数
    int streamlen = 0;
    int veciOutputBlockSize = 0;
    for(size_t i= 0; i< 4 ; i++)
    {
        streamlen= pParameter_recv->Stream[i].iLenPartA;
        streamlen += pParameter_recv->Stream[i].iLenPartB;
        //veciMaxOutputBlockSize[i] = 16384;
        veciOutputBlockSize += streamlen * SIZEOF__BYTE;
    }

    // pParameter_trans-> NumDecodedBitsMSC = pParameter_recv->iNumDecodedBitsMSC;
    pParameter_trans->iNumDecodedBitsMSC = veciOutputBlockSize;

    return 0;
}

void Print_stream(std::vector<CSingleBuffer<_BINARY>>* pMSCUseBuf)
{
    // !!用来打印!! --遍历 ppMSCUseBuf 中的每个 CSingleBuffer 对象
    for (CSingleBuffer<_BINARY>& singleBuffer : *pMSCUseBuf)
    {
        // 获取 QueryWriteBuffer 返回的指针
        CVectorEx<_BINARY>* vecMSCBUF = singleBuffer.QueryWriteBuffer();

        // 检查指针是否有效
        if (vecMSCBUF)
        {
            // 打印数据
            int MSCDATASIZE = vecMSCBUF->size();

            cout<<"MSC 数据的字节个数:" << (MSCDATASIZE / SIZEOF__BYTE)<<endl;
            for (int i = 0; i < MSCDATASIZE / SIZEOF__BYTE; ++i)
            {
                _BINARY byte = 0; // 初始化一个字节
                for (int j = 0; j < 8; j++) // 每次合并8个比特为一个字节
                {
                    // 将当前8个比特按位合并到一个字节中
                    byte |= ((*vecMSCBUF)[i * 8 + j] << (7 - j));
                }
                // 打印每个字节的十六进制表示
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                          << static_cast<int>(byte) << " ";

                // 控制格式
                if ((i + 1) % 8 == 0)
                {
                    std::cout << "  ";
                }
                if ((i + 1) % 32 == 0)
                {
                    std::cout << std::endl;
                }
            }
            std::cout << std::endl;
        }
        else
        {
            std::cout << "无效的 ppMSCUseBuf 指针!" << std::endl;
        }
    }

}
void Print_stream1(CSingleBuffer<_BINARY>* pMSCUseBuf)
{
    // 获取 QueryWriteBuffer 返回的指针
    CVectorEx<_BINARY>* vecMSCBUF = pMSCUseBuf->QueryWriteBuffer();

    // 检查指针是否有效
    if (vecMSCBUF)
    {
        // 打印数据
        // vecMSCBUF->resize(vecMSCBUF->capacity());  // 不需要resize
        int MSCDATASIZE = vecMSCBUF->size();

#if 0
        for (int i = 0; i < MSCDATASIZE / SIZEOF__BYTE; ++i)
        {
            _BINARY byte = 0; // 初始化一个字节
            for (int j = 0; j < 8; j++) // 每次合并8个比特为一个字节
            {
                // 将当前8个比特按位合并到一个字节中
                byte |= ((*vecMSCBUF)[i * 8 + j] << (7 - j));
            }
            // 打印每个字节的十六进制表示
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(byte) << " ";

            // 控制格式
            if ((i + 1) % 8 == 0)
            {
                std::cout << "  ";
            }
            if ((i + 1) % 32 == 0)
            {
                std::cout << std::endl;
            }
        }
        std::cout << std::endl;
#else
        // 尝试访问一个超出范围的元素
        try
        {
            for (int i = 0; i < MSCDATASIZE; ++i)
            {
                _BINARY byte = 0; // 初始化一个字节
                byte = vecMSCBUF->at(i); // 使用 at() 方法进行边界检查
                // 打印每个字节的
                std::cout << static_cast<int>(byte) << " ";

                // 控制格式
                if ((i + 1) % 8 == 0)
                {
                    std::cout << "  ";
                }
                if ((i + 1) % 32 == 0)
                {
                    std::cout << std::endl;
                }
            }
            std::cout << std::endl;
        }
        catch (const std::out_of_range& e)
        {
            std::cerr << "超出范围 error: " << e.what() << std::endl;
            exit(-1);
        }
        if(MSCDATASIZE != 0)
        {
            cout<< "MSC复用流数据的个数:" << MSCDATASIZE  <<endl;
        }
#endif
    }
    else
    {
        std::cout<<"检查指针无效 "<<std::endl;
    }


}


CMessageQueue::CMessageQueue()
{
    const std::string path = "/home/linux/code_wode";
    // 获取线程之间通信的键值
    msq_key = ftok(path.c_str(), 'M');
    if (msq_key == -1)
    {
        perror("ftok:");
        throw std::runtime_error("Failed to get message queue key");
    }

    // 创建或获取消息队列
    msq_id = msgget(msq_key, IPC_CREAT | 0666);
    if (msq_id == -1)
    {
        perror("msgget:");
        throw std::runtime_error("Failed to create or get message queue");
    }
}

CMessageQueue::CMessageQueue(const std::string& path)
{
    // 获取线程之间通信的键值
    msq_key = ftok(path.c_str(), 'M');
    if (msq_key == -1)
    {
        perror("ftok:");
        throw std::runtime_error("Failed to get message queue key");
    }

    // 创建或获取消息队列
    msq_id = msgget(msq_key, IPC_CREAT | 0666);
    if (msq_id == -1)
    {
        perror("msgget:");
        throw std::runtime_error("Failed to create or get message queue");
    }
}

CMessageQueue::~CMessageQueue() {
    // 删除消息队列
    if (msgctl(msq_id, IPC_RMID, nullptr) == -1)
    {
        perror("msgctl:");
    }
}


bool CMessageQueue::SendMessage(long type, CDRMReceiver *pDRMReceiver)
{
    //
    RecvToMulti_t tempRecv;
    tempRecv.mytype = type;
    tempRecv.pDRMReceiver = pDRMReceiver; /*传入的接收机源指针*/
    //

    if (msgsnd(msq_id, &tempRecv, sizeof(tempRecv.pDRMReceiver), 0) == -1)
    {
        perror("msgsnd: ");
        return false;
    }
    return true;
}

CDRMReceiver* CMessageQueue::ReceiveMessage(long type)
{

    RecvToMulti_t tempRecv;

    ssize_t msgsz = msgrcv(msq_id, &tempRecv, sizeof(tempRecv.pDRMReceiver), type, 0);
    if (msgsz == -1)
    {
        if (errno != ENOMSG)
        {
            perror("msgrcv:");
        }
        return NULL;
    }


    return tempRecv.pDRMReceiver;    /*传入的接收机源指针*/
}

void CMessageQueue::ClearMessageQueue()
{
    RecvToMulti_t tempMsg;

    ///参数0表示希望接收任意类型的消息
    while (msgrcv(msq_id, &tempMsg, sizeof(tempMsg.pDRMReceiver), 0, IPC_NOWAIT) != -1)
    {
        // 清空消息队列中的消息
    }

    // 检查是否读取所有消息时发生错误
    if (errno != ENOMSG)
    {
        perror("msgrcv:");
        throw std::runtime_error("Failed to clear message queue");
    }
}


int CopyStream(std::vector<CSingleBuffer<_BINARY> >* pMSCUseBuf,
               std::vector<CSingleBuffer<_BINARY> >* pTxMSCUseBuf,CParameter* pParameter_recv)
{
    for(int j  =0;j < MAX_NUM_STREAMS ;j++)
    {
        int iInputBlockSize = pParameter_recv->GetStreamLen(j) * SIZEOF__BYTE;

        CSingleBuffer<_BINARY>& singleBuffer1 = (*pMSCUseBuf)[j];
        CVectorEx<_BINARY>* InputMSCBuf = singleBuffer1.QueryWriteBuffer();

        CSingleBuffer<_BINARY>& singleBuffer2 = (*pTxMSCUseBuf)[j];
        CVectorEx<_BINARY>* OutputMSCBuf = singleBuffer2.QueryWriteBuffer();

        for (int i = 0; i < iInputBlockSize; i++)
        {
            _BINARY n = (*(InputMSCBuf))[i];
            (*OutputMSCBuf)[i] = n;
        }
    }

//    * 设置大小 : this->iInputBlockSize = p.GetStreamLen(iStreamID) * SIZEOF__BYTE;
//    *
//       for (int i = 0; i < this->iInputBlockSize; i++)
//       {
//           TInput n = (*(this->pvecInputData))[i];
//           (*this->pvecOutputData)[i] = n;
//           (*this->pvecOutputData2)[i] = n;
//       }
    return 0;
}


