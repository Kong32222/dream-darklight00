/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	MSC audio/data demultiplexer
 *
 *
 * - (6.2.3.1) Multiplex frames (DRM standard):
 * The multiplex frames are built by placing the logical frames from each
 * non-hierarchical stream together. The logical frames consist, in general, of
 * two parts each with a separate protection level. The multiplex frame is
 * constructed by taking the data from the higher protected part of the logical
 * frame from the lowest numbered stream (stream 0 when hierarchical modulation
 * is not used, or stream 1 when hierarchical modulation is used) and placing
 * it at the start of the multiplex frame. Next the data from the higher
 * protected part of the logical frame from the next lowest numbered stream is
 * appended and so on until all streams have been transferred. The data from
 * the lower protected part of the logical frame from the lowest numbered
 * stream (stream 0 when hierarchical modulation is not used, or stream 1 when
 * hierarchical modulation is used) is then appended, followed by the data from
 * the lower protected part of the logical frame from the next lowest numbered
 * stream, and so on until all streams have been transferred. The higher
 * protected part is designated part A and the lower protected part is
 * designated part B in the multiplex description. The multiplex frame will be
 * larger than or equal to the sum of the logical frames from which it is
 * formed. The remainder, if any, of the multiplex frame shall be filled with
 * 0s. These bits shall be ignored by the receiver.
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "MSCMultiplexer.h"
#include <iostream>
using namespace std;

/* Implementation *************************************************************/
void CMSCDemultiplexer::ProcessDataInternal(CParameter&)
{
//    for(int i =0 ;i < MAX_NUM_STREAMS;i++)
//    {
//        cout<<"StreamPos["<<i<<"].iLenLow = "<<StreamPos[i].iLenLow<<endl;
//        cout<<"StreamPos["<<i<<"].iLenHigh = "<<StreamPos[i].iLenHigh<<endl;
//        cout<<"StreamPos["<<i<<"].iOffsetLow = "<<StreamPos[i].iOffsetLow<<endl;
//        cout<<"StreamPos["<<i<<"].iOffsetHigh = "<<StreamPos[i].iOffsetHigh<<endl;
//    }

    for (size_t i=0; i<MAX_NUM_STREAMS; i++)
        ExtractData(*pvecInputData, *vecpvecOutputData[i], StreamPos[i]);


}

void CMSCDemultiplexer::InitInternal(CParameter& Parameters)
{
    Parameters.Lock();
    for (size_t i=0; i<MAX_NUM_STREAMS; i++)
    {
        StreamPos[i] = GetStreamPos(Parameters, i);
        veciOutputBlockSize[i] = StreamPos[i].iLenHigh + StreamPos[i].iLenLow;
    }
    /* Set input block size */
    iInputBlockSize = Parameters.iNumDecodedBitsMSC;
    Parameters.Unlock();
}

void CMSCDemultiplexer::ExtractData(CVectorEx<_BINARY>& vecIn,
                                    CVectorEx<_BINARY>& vecOut,
                                    SStreamPos& StrPos)
{
    int i;

    /* Higher protected part */
    for (i = 0; i < StrPos.iLenHigh; i++)
        vecOut[i] = vecIn[i + StrPos.iOffsetHigh];

    /* Lower protected part */
    for (i = 0; i < StrPos.iLenLow; i++)
        vecOut[i + StrPos.iLenHigh] = vecIn[i + StrPos.iOffsetLow];
}

CMSCDemultiplexer::SStreamPos CMSCDemultiplexer::GetStreamPos(CParameter& Param,
                                                              const int iStreamID)
{
    CMSCDemultiplexer::SStreamPos	StPos;

    /* Init positions with zeros (needed if an error occurs) */
    StPos.iOffsetLow = 0;
    StPos.iOffsetHigh = 0;
    StPos.iLenLow = 0;
    StPos.iLenHigh = 0;

    if (iStreamID != STREAM_ID_NOT_USED)
    {
        /* Length of higher and lower protected part of audio stream (number
           of bits) */
        StPos.iLenHigh = Param.Stream[iStreamID].iLenPartA * SIZEOF__BYTE;
        StPos.iLenLow = Param.Stream[iStreamID].iLenPartB *	SIZEOF__BYTE;


        /* Byte-offset of higher and lower protected part of audio stream --- */
        /* Get active streams */
        set<int> actStreams;
        Param.GetActiveStreams(actStreams);

        /* Get start offset for lower protected parts in stream. Since lower
           protected part comes after the higher protected part, the offset
           must be shifted initially by all higher protected part lengths
           (iLenPartA of all streams are added) 6.2.3.1 */
        StPos.iOffsetLow = 0;
        set<int>::iterator i;
        for (i = actStreams.begin(); i!=actStreams.end(); i++)
        {
            StPos.iOffsetLow += Param.Stream[*i].iLenPartA * SIZEOF__BYTE;
        }

        /* Real start position of the streams */
        StPos.iOffsetHigh = 0;
        for (i = actStreams.begin(); i!=actStreams.end(); i++)
        {
            if (*i < iStreamID)
            {
                StPos.iOffsetHigh += Param.Stream[*i].iLenPartA * SIZEOF__BYTE;
                StPos.iOffsetLow += Param.Stream[*i].iLenPartB * SIZEOF__BYTE;
            }
        }

        /* Special case if hierarchical modulation is used */
        if (((Param.eMSCCodingScheme == CS_3_HMSYM) ||
             (Param.eMSCCodingScheme == CS_3_HMMIX)))
        {
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

        /* Possibility check ------------------------------------------------ */
        /* Test, if parameters have possible values */
        if ((StPos.iOffsetHigh + StPos.iLenHigh > Param.iNumDecodedBitsMSC) ||
                (StPos.iOffsetLow + StPos.iLenLow > Param.iNumDecodedBitsMSC))
        {
            /* Something is wrong, set everything to zero */
            StPos.iOffsetLow = 0;
            StPos.iOffsetHigh = 0;
            StPos.iLenLow = 0;
            StPos.iLenHigh = 0;
        }
    }

    return StPos;
}

//CMSCmultiplexer::SStreamPos CMSCmultiplexer::GetStreamPos(CParameter &Param,
//                                                          const int iStreamID)
//{
//    CMSCmultiplexer::SStreamPos	StPos;


//    /* Init positions with zeros (needed if an error occurs) */
//    StPos.iOffsetLow = 0;
//    StPos.iOffsetHigh = 0;
//    StPos.iLenLow = 0;
//    StPos.iLenHigh = 0;

//    if (iStreamID != STREAM_ID_NOT_USED)
//    {
//        /* Length of higher and lower protected part of audio stream (number of bits)
//              Stream是一个Cstream类型的数组中的int类型(iLenPartA)* 8  iLenPartA == 多少字节* * bit ==
//             StPos.iLenHigh  单位是bit*8 == [总位数]
//         */

//        // 输入数据的 高、低保护数据长度
//        StPos.iLenHigh = Param.Stream[iStreamID].iLenPartA * SIZEOF__BYTE;
//        StPos.iLenLow = Param.Stream[iStreamID].iLenPartB * SIZEOF__BYTE;


//        /* Byte-offset of higher and lower protected part of audio stream --- */
//        /* Get active streams */
//        set<int> actStreams;
//        Param.GetActiveStreams(actStreams);  //尾插 ： actStr.insert(Service[i].AudioParam.iStreamID );

//        /* Get start offset for lower protected parts in stream. Since lower
//            protected part comes after the higher protected part, the offset
//            must be shifted initially by all higher protected part lengths
//            (iLenPartA of all streams are added) 6.2.3.1
//            获取流中较低受保护部件的启动偏移量。因为低受保护部分位于高保护部分之后，即偏移量
//             必须首先移动所有较高的保护部件长度 (添加所有流的iLenPartA) */
//        StPos.iOffsetLow = 0;
//        set<int>::iterator i;

//        //计算 低保护的起始点
//        for (i = actStreams.begin(); i != actStreams.end(); i++)
//        {
//            StPos.iOffsetLow += Param.Stream[*i].iLenPartA * SIZEOF__BYTE;
//        }

//        /* Real start position of the streams 流的实际起始位置 */
//        StPos.iOffsetHigh = 0;

//        //复加 高、低保护的偏移量
//        for (i = actStreams.begin(); i != actStreams.end(); i++)
//        {
//            if (*i < iStreamID)
//            {
//                //设置 高保护 低保护的偏移量
//                StPos.iOffsetHigh += Param.Stream[*i].iLenPartA * SIZEOF__BYTE;
//                StPos.iOffsetLow += Param.Stream[*i].iLenPartB * SIZEOF__BYTE;
//            }
//        }

//        /* Special case if hierarchical modulation is used
//          使用分层调制的特殊情况 */
//        if (((Param.eMSCCodingScheme == CS_3_HMSYM) ||
//             (Param.eMSCCodingScheme == CS_3_HMMIX)))
//        {
//            std::cout << "使用分层调制的特殊情况" << std::endl;

//            if (iStreamID == 0)
//            {
//                /* Hierarchical channel is selected. Data is at the beginning
//                    of incoming data block */
//                StPos.iOffsetLow = 0;
//            }
//            else
//            {
//                /* Shift all offsets by the length of the hierarchical frame. We
//                    cannot use the information about the length in
//                    "Stream[0].iLenPartB", because the real length of the frame
//                    is longer or equal to the length in "Stream[0].iLenPartB" */
//                StPos.iOffsetHigh += Param.iNumBitsHierarchFrameTotal;
//                StPos.iOffsetLow += Param.iNumBitsHierarchFrameTotal -
//                        /* We have to subtract this because we added it in the
//                                for loop above which we do not need here */
//                        Param.Stream[0].iLenPartB * SIZEOF__BYTE;
//            }
//        }

//        /* Possibility check  可能性检查 ------------------------------------------------ */
//        /* Test, if parameters have possible values 测试参数是否有可能的值 */
//        if ((StPos.iOffsetHigh + StPos.iLenHigh > Param.iNumDecodedBitsMSC) ||
//                (StPos.iOffsetLow + StPos.iLenLow > Param.iNumDecodedBitsMSC))
//        {



//            std::cout << "重要---StPos.iOffsetHigh + StPos.iLenHigh = " << StPos.iOffsetHigh + StPos.iLenHigh << std::endl;
//            std::cout << "重要---StPos.iOffsetLow + StPos.iLenLow = " << StPos.iOffsetLow + StPos.iLenLow << std::endl;

//            /* Something is wrong, set everything to zero
//              出了什么问题，把一切都设为零*/
//            StPos.iOffsetLow = 0;
//            StPos.iOffsetHigh = 0;
//            StPos.iLenLow = 0;
//            StPos.iLenHigh = 0;
//        }

//    }

//    return StPos;
//}

//bool CMSCmultiplexer::ProcessData(CParameter &Parameter,
//                      std::vector<CSingleBuffer<_BINARY>>& vecpvecInputData,
//                      CSingleBuffer<_BINARY>& OutputBuffer)
//{
//    bool InitFlag = true;
//    int steamSize = (int)vecpvecInputData.size();
//    for (this->streamID = 0; this->streamID < steamSize ; this->streamID++)
//    {
//        std::cout << "流 = " << streamID << std::endl;

//        if (OutputBuffer.GetRequestFlag())
//        {

//            this->pvecInputData =
//                    vecpvecInputData[this->streamID].Get(this->iInputBlockSize);


//            this->pvecOutputData = OutputBuffer.QueryWriteBuffer();

//            /* Copy extended data from std::vectors */
//            (*(this->pvecOutputData)).SetExData((*(this->pvecInputData)).GetExData());

//            /* Call the underlying processing-routine */
//            if (InitFlag)
//            {
//                this->InitInternal(&Parameter);
//                InitFlag = false;
//            }

//            this->ProcessDataInternal(&Parameter);

//            /* Write processed data from internal memory in transfer-buffer */
//            OutputBuffer.Put(this->iOutputBlockSize);  //  累加

//            /* Data was provided, clear data request  */

//        }
//        else
//        {
//            std::cout << "NOT OutputBuffer.GetRequestFlag()-----" << std::endl;
//        }
//    }

//    OutputBuffer.SetRequestFlag(false);
//    return true;

//}



//void CMSCmultiplexer::MultiplexerData(std::vector<CSingleBuffer<_BINARY>>* pMSCUseBuf,
//                                      CSingleBuffer<_BINARY>* pMuxBuf,
//                                      SStreamPos* StreamPos)
//{
//    if (pMSCUseBuf == nullptr)
//     {   // 检查输入数据指针是否为空
//         std::cerr << "Error: Input data pointer is null." << std::endl;
//         return;
//     }

//     if (pMuxBuf == nullptr)
//     {   // 检查输出缓冲区指针是否为空
//         std::cerr << "Error: Output buffer pointer is null." << std::endl;
//         return;
//     }

//     if (StreamPos == nullptr)
//     {   // 检查流位置指针是否为空
//         std::cerr << "Error: Stream position pointer is null." << std::endl;
//         return;
//     }

//     const int tempID = this->streamID;
//     /*
//             是首先使用 [] 运算符获取 pMSCUseBuf 指针指向的向量中的第一个元素，
//             然后使用 * 运算符对获取到的元素进行解引用，从而得到该元素的值。
//             接着，& 运算符用于获取该元素的地址，将该地址赋值给指针 pInputBuf。
//         */
//     CSingleBuffer<_BINARY>& InputBuf = (*pMSCUseBuf)[tempID];


//     if (pMSCUseBuf == nullptr)
//     {
//         std::cerr << "Error: pMSCUseBuf is null." << std::endl;
//         return;
//     }


//     const int  LenLow = this->StreamPos[tempID].iLenLow;
//     const int  LenHigh = this->StreamPos[tempID].iLenHigh;
//     const int  OffsetLow = this->StreamPos[tempID].iOffsetLow;
//     const int  OffsetHigh = this->StreamPos[tempID].iOffsetHigh;
//     if ((LenLow + LenHigh) <= 0)
//     {
//         // "流" << tempID << "没有数据 " << std::endl;
//         return;
//     }
//     else
//     {
//         std::cout << "流" << tempID << std::endl;
//         std::cout << ": #StPos.iLenHigh = " << LenHigh << std::endl;
//         std::cout << " #StPos.iOffsetHigh = " << OffsetHigh << std::endl;
//         std::cout << " #StPos.iLenLow = " << LenLow << std::endl;
//         std::cout << " #StPos.iOffsetLow = " << OffsetLow << std::endl;
//     }

//     //迭代器  (*vecOfVecsPtr)[i][j]
//     CVectorEx<_BINARY>* pDataBuff = InputBuf.QueryWriteBuffer();
//     int DataBufLen = pDataBuff->capacity();
//     (void) DataBufLen;
//     std::cout << "pInputBuf QueryWriteBuffer()  = " << pDataBuff << std::endl;
//     std::cout << "pInputBuf  size =  = " << pDataBuff->size() << std::endl;
//     std::cout << "pInputBuf  Size =  = " << pDataBuff->Size() << std::endl;
//     std::vector<_BINARY>::iterator It_VecInput = pDataBuff->begin();

//     CVectorEx<_BINARY>* pMuxBuff = pMuxBuf->QueryWriteBuffer();
//     std::cout << "pMuxBuff QueryWriteBuffer()  = " << pMuxBuff << std::endl;
//     std::cout << "pMuxBuff  size =  = " << pMuxBuff->size() << std::endl;
//     std::cout << "pMuxBuff  Size =  = " << pMuxBuff->Size() << std::endl;

//     vector<_BINARY>::iterator it_VecOutput = pMuxBuff->begin();

//     // mutex

//     /* 高位 */
//     std::cout << " 拷贝 高位" << std::endl;
//     for (int i = 0; i < LenHigh; i++)
//     {
//         it_VecOutput[i + OffsetHigh] = It_VecInput[i];
//     }

//     /* 低位 */
//     std::cout << " 拷贝 低位" << std::endl;
//     int i = 0;
//     for (i = 0; i < LenLow; i++)
//     {
//         it_VecOutput[i + OffsetLow] = It_VecInput[i + LenHigh];
//     }

//     std::cout << "i循环了 = " << i << " 次数，全部拷贝 完成" << std::endl;
//}

//void CMSCmultiplexer::InitInternal(CParameter *Parameters)
//{
//    (void)Parameters;
//    std::cout << "指针版本-InitInternal()" << std::endl;

//    // 计算 复用流
//    int Allsize = 0;

//    //计算 输入缓冲区的大小
//    for (int i = 0;i < MAX_NUM_STREAMS;i++)
//    {
//        Allsize += this->MSCUseBuf[i].QueryWriteBuffer()->size();
//    }

//    // 输出缓冲区赋值
//    this->pvecOutputData->resize(Allsize);
//    int MuxBuffLen = pvecOutputData->capacity();
//    pvecOutputData->Init(MuxBuffLen);
//    pvecOutputData->ResetBitAccess();

//    std::cout << " pvecOutputData.size = " << pvecOutputData->size() << std::endl;
//    std::cout << " pvecOutputData.capacity = " << pvecOutputData->capacity() << std::endl;

//    // 清空 ppMuxBuf中的数据，但保留容量不变
//    this->pvecOutputData->erase(this->pvecOutputData->begin(), this->pvecOutputData->end());

//}

//void CMSCmultiplexer::ProcessDataInternal(CParameter *Parameters)
//{
//    std::cout << "指针版本-ProcessDataInternal()" << std::endl;
//    //  安全检测
//    if (Parameters == nullptr)
//    {
//        std::cout << "Parameters null " << std::endl;
//        return;
//    }

//    MultiplexerData(&MSCUseBuf, &MuxBuf, StreamPos);

//    //
//    this->iOutputBlockSize = StreamPos->iLenLow + StreamPos->iLenHigh;
//}
