/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See MSCMultiplexer.cpp
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

#if !defined(MSCMUX_H__3B0BA660_JLBVEOUB239485BB2B_23E7A0D31912__INCLUDED_)
#define MSCMUX_H__3B0BA660_JLBVEOUB239485BB2B_23E7A0D31912__INCLUDED_

#include "GlobalDefinitions.h"
#include "Parameter.h"
#include "util/Buffer.h"
#include "util/Modul.h"
#include "util/Vector.h"

/* Classes ********************************************************************/
class CMSCDemultiplexer : public CReceiverModul<_BINARY, _BINARY>
{
public:
    CMSCDemultiplexer() {}
    virtual ~CMSCDemultiplexer() {}

protected:
    struct SStreamPos
    {
        int	iOffsetLow;
        int	iOffsetHigh;
        int	iLenLow;
        int	iLenHigh;
    };

    SStreamPos			StreamPos[4];

    SStreamPos GetStreamPos(CParameter& Param, const int iStreamID);
    void ExtractData(CVectorEx<_BINARY>& vecIn, CVectorEx<_BINARY>& vecOut,
                     SStreamPos& StrPos);

    virtual void InitInternal(CParameter& Parameters);
    virtual void ProcessDataInternal(CParameter& Parameters);
};

///*  复用  新类  CTransmitterModul  */
//class CMSCmultiplexer : public CTransmitterModul<_BINARY, _BINARY>
//{

//public:
//    CMSCmultiplexer() :MSCUseBuf(4), m_VecOutputSize(0){ }

//    virtual ~CMSCmultiplexer() {}
//    int streamID;       // 流的 索引值

//    void AddOutputSize(const int size)
//    {
//        m_VecOutputSize += size;
//    }

//    struct SStreamPos
//    {
//        int	iOffsetLow;
//        int	iOffsetHigh;
//        int	iLenLow;
//        int	iLenHigh;
//    };

//    SStreamPos StreamPos[4];
//    // msc 流  现在已经有数据了
//    CSingleBuffer<_BINARY>* GetMSCUseBuf(int index)
//    {
//        return &MSCUseBuf[index];
//    }

//    std::vector<CSingleBuffer<_BINARY>> MSCUseBuf; //4流

//    // 内部成员 迭代器指针
//    typename std::vector<CSingleBuffer<_BINARY>>::iterator m_iterator;

//    // 复用流
//    CSingleBuffer<_BINARY>* GetMuxBuf(){ return &MuxBuf;}
//    CSingleBuffer<_BINARY>	MuxBuf;




//    SStreamPos GetStreamPos(CParameter& Param, const int iStreamID);

//    //传 发射机的 Parameter ;
//    bool ProcessData(CParameter& Parameter,
//                             std::vector<CSingleBuffer<_BINARY>>& vecpvecInputData,
//                             CSingleBuffer<_BINARY>& vecOutputBuffer);

//    // 复用数据流 --传输帧 (指针参数)
//    void MultiplexerData(std::vector<CSingleBuffer<_BINARY>>* pVecInputData,
//        CSingleBuffer<_BINARY>* pMuxBuf,
//        SStreamPos* StreamPos);

//    virtual void InitInternal(CParameter* Parameters);
//    virtual void ProcessDataInternal(CParameter* Parameters);


//private:
//    int  m_VecOutputSize;  //记录 输出缓冲区的size()
//};



#endif // !defined(MSCMUX_H__3B0BA660_JLBVEOUB239485BB2B_23E7A0D31912__INCLUDED_)
