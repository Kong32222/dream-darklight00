#ifndef CCMSCMULTIPLEXER_H
#define CCMSCMULTIPLEXER_H

#include <stdio.h>

#include "GlobalDefinitions.h"
#include "Parameter.h"
#include "util/Buffer.h"
#include "util/Modul.h"
#include "util/Vector.h"
#include <iomanip>


/*  复用  新类  CTransmitterModul  */
class CMSCmultiplexer : public CTransmitterModul<_BINARY, _BINARY>
{
public:
    // 初始化列表中字段的顺序与它们在类定义中的声明顺序一致
    CMSCmultiplexer() :m_veciOutputBlockSize(4), MSCUseBuf(4), m_VecOutputSize(0){}

    virtual ~CMSCmultiplexer() {}
    int streamID = 0;       // 流的 索引值

    void AddOutputSize(const int size)
    {
        m_VecOutputSize += size;
    }

    struct SStreamPos
    {
        int	iOffsetLow;
        int	iOffsetHigh;
        int	iLenLow;
        int	iLenHigh;
    };
    // my
    void setInputBlockSize(int inputsize )
    {
        this->iInputBlockSize = inputsize;
    }

    void setOutputBlockSize(int size )
    {
        this->iOutputBlockSize = size;
    }


    SStreamPos StreamPos[4];
    vector <int> m_veciOutputBlockSize;   //记录每个流中 高低保护个数

    // msc 流  现在已经有数据了
    CSingleBuffer<_BINARY>* GetMSCUseBuf(int index){return &MSCUseBuf[index]; }

    std::vector<CSingleBuffer<_BINARY>>* GetMSCuUseBuf_All() { return &MSCUseBuf; }

    int GetMSCBitStreamNum(){ return m_MSCBitStreamNum; }

    //msc流  4
    std::vector<CSingleBuffer<_BINARY>> MSCUseBuf;

    // 内部成员 迭代器指针
    typename std::vector<CSingleBuffer<_BINARY>>::iterator m_iterator;


    // 复用流
    CSingleBuffer<_BINARY>* GetMuxBuf() {return &MuxBuf;}
    CSingleBuffer<_BINARY> MuxBuf;

    // 计算 高低保护
    SStreamPos GetStreamPos(CParameter& Param, const int iStreamID);

    //  复用数据流
    void MultiplexerData1(CVectorEx<_BINARY>* pVecInputData,
        CVectorEx<_BINARY>* pMuxBuf,
        SStreamPos* StreamPos);

    //
    virtual void ProcessDataInternal(CParameter& Parameters);
    virtual void InitInternal(CParameter& Parameters);


private:
    int  m_VecOutputSize;      // 记录 输出缓冲区的size()
    int  m_MSCBitStreamNum= 0; // MSC 比特流的总大小
    int  m_Filllevel;
};




//  复用数据流 --传输帧 (指针参数)  1参数. MSC所有流  2参数. 复用流  3. 高低保护参数
//void MultiplexerData(std::vector<CSingleBuffer<_BINARY>>* pVecInputData,
//    CSingleBuffer<_BINARY>* pMuxBuf,
//    SStreamPos* StreamPos);
#endif // CCMSCMULTIPLEXER_H
