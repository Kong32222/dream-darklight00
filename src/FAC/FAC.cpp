/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	FAC
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

#include "FAC.h"


/* Implementation *************************************************************/
/******************************************************************************\
* CFACTransmit																   *
\******************************************************************************/
void CFACTransmit::FACParam(CVector<_BINARY>* pbiFACData, CParameter& Parameter)
{
    int			iCurShortID;

    /* Reset enqueue function */
    (*pbiFACData).ResetBitAccess();

    /* Put FAC parameters on stream */
    /* Channel parameters --------------------------------------------------- */
    /* Base/Enhancement flag, set it to base which is decodable by all DRM
       receivers */
    (*pbiFACData).Enqueue(0 /* 0 */, 1);

    /* Identity */
    /* Manage index of FAC block in super-frame 管理超帧 中FAC块的索引 */
    cout<<"tx: fac Parameter.iFrameIDTransm index:"<<Parameter.iFrameIDTransm << endl;
    switch (Parameter.iFrameIDTransm)
    {
    case 0:
        /* Assuming AFS is valid (AFS not used here), if AFS is not valid, the
           parameter must be 3 (11) */
        (*pbiFACData).Enqueue(3 /* 11 */, 2);
        break;

    case 1:
        (*pbiFACData).Enqueue(1 /* 01 */, 2);
        break;

    case 2:
        (*pbiFACData).Enqueue(2 /* 10 */, 2);
        break;
    }
    cout<<"tx: fac Parameter.GetSpectrumOccup():"<<Parameter.GetSpectrumOccup()<< endl;

    /* Spectrum occupancy */
    switch (Parameter.GetSpectrumOccup())
    {
    case SO_0:
        (*pbiFACData).Enqueue(0 /* 0000 */, 4);
        break;

    case SO_1:
        (*pbiFACData).Enqueue(1 /* 0001 */, 4);
        break;

    case SO_2:
        (*pbiFACData).Enqueue(2 /* 0010 */, 4);
        break;

    case SO_3:
        (*pbiFACData).Enqueue(3 /* 0011 */, 4);
        break;

    case SO_4:
        (*pbiFACData).Enqueue(4 /* 0100 */, 4);
        break;

    case SO_5:
        (*pbiFACData).Enqueue(5 /* 0101 */, 4);
        break;
    }

    /* Interleaver depth flag */
    cout<<"tx: fac Parameter.eSymbolInterlMode:"<<Parameter.eSymbolInterlMode<< endl;

    switch (Parameter.eSymbolInterlMode)
    {
    case CParameter::SI_LONG:
        (*pbiFACData).Enqueue(0 /* 0 */, 1);
        break;

    case CParameter::SI_SHORT:
        (*pbiFACData).Enqueue(1 /* 1 */, 1);
        break;
    }
    cout<<"tx: fac Parameter.eMSCCodingScheme:"<<Parameter.eMSCCodingScheme<< endl;

    /* MSC mode */
    switch (Parameter.eMSCCodingScheme)
    {
    case CS_3_SM:
        (*pbiFACData).Enqueue(0 /* 00 */, 2);
        break;

    case CS_3_HMMIX:
        (*pbiFACData).Enqueue(1 /* 01 */, 2);
        break;

    case CS_3_HMSYM:
        (*pbiFACData).Enqueue(2 /* 10 */, 2);
        break;

    case CS_2_SM:
        (*pbiFACData).Enqueue(3 /* 11 */, 2);
        break;

    default:
        break;
    }
    cout<<"tx: fac Parameter.eSDCCodingScheme:"<<Parameter.eSDCCodingScheme<< endl;

    /* SDC mode */
    switch (Parameter.eSDCCodingScheme)
    {
    case CS_2_SM:
        (*pbiFACData).Enqueue(0 /* 0 */, 1);
        break;

    case CS_1_SM:
        (*pbiFACData).Enqueue(1 /* 1 */, 1);
        break;

    default:
        break;
    }

    /* Number of services */
    cout<<"tx fac Number of services: "
       <<iTableNumOfServices[Parameter.iNumAudioService]
         [Parameter.iNumDataService]
            <<"; Parameter.iNumAudioService: "<<Parameter.iNumAudioService
            <<"; Parameter.iNumDataService: "<<Parameter.iNumDataService<<endl;

    /* Use Table */
    (*pbiFACData).Enqueue(iTableNumOfServices[Parameter.iNumAudioService]
            [Parameter.iNumDataService], 4);

    /* Reconfiguration index (not used) */
    (*pbiFACData).Enqueue((uint32_t) 0, 3);

    /* rfu */
    (*pbiFACData).Enqueue((uint32_t) 0, 2);

    /* Service parameters --------------------------------------------------- */
    /* Transmit service-information of service signalled in the FAC-repetition
       array */
    iCurShortID = FACRepetition[FACRepetitionCounter];
     cout<<"tx fac FACRepetitionCounter= "<< FACRepetitionCounter <<endl;
    cout<<"tx fac iCurShortID = "<<iCurShortID <<endl;
    FACRepetitionCounter++;
    if (FACRepetitionCounter == FACNumRep)
        FACRepetitionCounter = 0;

    /* Service identifier */
     cout<<"tx fac iServiceID= "<< Parameter.Service[iCurShortID].iServiceID <<endl;
    (*pbiFACData).Enqueue(Parameter.Service[iCurShortID].iServiceID, 24);


    /* Short ID */
    (*pbiFACData).Enqueue((uint32_t) iCurShortID, 2);
    cout<<"tx fac Parameter eCAIndication= "<<
          Parameter.Service[iCurShortID].eCAIndication <<endl;

    /* CA indication */
    switch (Parameter.Service[iCurShortID].eCAIndication)
    {
    case CService::CA_NOT_USED:
        (*pbiFACData).Enqueue(0 /* 0 */, 1);
        break;

    case CService::CA_USED:
        (*pbiFACData).Enqueue(1 /* 1 */, 1);
        break;
    }

    cout<<"tx fac Parameter.Service[iCurShortID].iLanguage= "<<
          Parameter.Service[iCurShortID].iLanguage <<endl;
    /* Language */
    (*pbiFACData).Enqueue(
                (uint32_t) Parameter.Service[iCurShortID].iLanguage, 4);

    /* Audio/Data flag */
    cout<<"tx fac Audio/Data flag = "<<
          Parameter.Service[iCurShortID].eAudDataFlag<<endl;
    switch (Parameter.Service[iCurShortID].eAudDataFlag)
    {
    case CService::SF_AUDIO:
        (*pbiFACData).Enqueue(0 /* 0 */, 1);
        break;

    case CService::SF_DATA:
        (*pbiFACData).Enqueue(1 /* 1 */, 1);
    }

    cout<<"tx fac Parameter.Service[iCurShortID].iServiceDescr = "<<
          Parameter.Service[iCurShortID].iServiceDescr  <<endl;
    /* Service descriptor */
    (*pbiFACData).Enqueue(
                (uint32_t) Parameter.Service[iCurShortID].iServiceDescr, 5);

    /* Rfa */
    (*pbiFACData).Enqueue(uint32_t(0), 7);

    /* CRC ------------------------------------------------------------------ */
    /* Calculate the CRC and put at the end of the stream */
    CRCObject.Reset(8);

    (*pbiFACData).ResetBitAccess();

    for (int i = 0; i < NUM_FAC_BITS_PER_BLOCK / SIZEOF__BYTE - 1; i++)
        CRCObject.AddByte((_BYTE) (*pbiFACData).Separate(SIZEOF__BYTE));

    /* Now, pointer in "enqueue"-function is back at the same place,
       add CRC */
    (*pbiFACData).Enqueue(CRCObject.GetCRC(), 8);
}
void CFACTransmit::Init(CParameter& Parameter)
{
    set<int>	actServ;

    /* Get active services */
    Parameter.GetActiveServices(actServ);
    const size_t iTotNumServices = actServ.size();

    /* Check how many audio and data services present */
    vector<int>	veciAudioServ;
    vector<int>	veciDataServ;
    size_t		iNumAudio = 0;
    size_t		iNumData = 0;

    for (set<int>::iterator i = actServ.begin(); i!=actServ.end(); i++)
    {
        if (Parameter.Service[*i].eAudDataFlag == CService::SF_AUDIO)
        {
            veciAudioServ.push_back(*i);
            iNumAudio++;
        }
        else
        {
            veciDataServ.push_back(*i);
            iNumData++;
        }
    }

    /* Now check special cases which are defined in 6.3.6-------------------- */
    /* If we have only data or only audio services. When all services are of
       the same type (e.g. all audio or all data) then the services shall be
       signalled sequentially */
    if ((iNumAudio == iTotNumServices) || (iNumData == iTotNumServices))
    {
        /* Init repetition vector */
        FACNumRep = iTotNumServices;
        FACRepetition.resize(0);

        for (set<int>::iterator i = actServ.begin(); i!=actServ.end(); i++)
            FACRepetition.push_back(*i);
    }
    else
    {
        /* Special cases according to Table 60 (Service parameter repetition
           patterns for mixtures of audio and data services) */
        if (iNumAudio == 1)
        {
            if (iNumData == 1)
            {
                /* Init repetion vector */
                FACNumRep = 5;
                FACRepetition.resize(FACNumRep);

                /* A1A1A1A1D1 */
                FACRepetition[0] = veciAudioServ[0];
                FACRepetition[1] = veciAudioServ[0];
                FACRepetition[2] = veciAudioServ[0];
                FACRepetition[3] = veciAudioServ[0];
                FACRepetition[4] = veciDataServ[0];
            }
            else if (iNumData == 2)
            {
                /* Init repetion vector */
                FACNumRep = 10;
                FACRepetition.resize(FACNumRep);

                /* A1A1A1A1D1A1A1A1A1D2 */
                FACRepetition[0] = veciAudioServ[0];
                FACRepetition[1] = veciAudioServ[0];
                FACRepetition[2] = veciAudioServ[0];
                FACRepetition[3] = veciAudioServ[0];
                FACRepetition[4] = veciDataServ[0];
                FACRepetition[5] = veciAudioServ[0];
                FACRepetition[6] = veciAudioServ[0];
                FACRepetition[7] = veciAudioServ[0];
                FACRepetition[8] = veciAudioServ[0];
                FACRepetition[9] = veciDataServ[1];
            }
            else /* iNumData == 3 */
            {
                /* Init repetion vector */
                FACNumRep = 15;
                FACRepetition.resize(FACNumRep);

                /* A1A1A1A1D1A1A1A1A1D2A1A1A1A1D3 */
                FACRepetition[0] = veciAudioServ[0];
                FACRepetition[1] = veciAudioServ[0];
                FACRepetition[2] = veciAudioServ[0];
                FACRepetition[3] = veciAudioServ[0];
                FACRepetition[4] = veciDataServ[0];
                FACRepetition[5] = veciAudioServ[0];
                FACRepetition[6] = veciAudioServ[0];
                FACRepetition[7] = veciAudioServ[0];
                FACRepetition[8] = veciAudioServ[0];
                FACRepetition[9] = veciDataServ[1];
                FACRepetition[10] = veciAudioServ[0];
                FACRepetition[11] = veciAudioServ[0];
                FACRepetition[12] = veciAudioServ[0];
                FACRepetition[13] = veciAudioServ[0];
                FACRepetition[14] = veciDataServ[2];
            }
        }
        else if (iNumAudio == 2)
        {
            if (iNumData == 1)
            {
                /* Init repetion vector */
                FACNumRep = 5;
                FACRepetition.resize(FACNumRep);

                /* A1A2A1A2D1 */
                FACRepetition[0] = veciAudioServ[0];
                FACRepetition[1] = veciAudioServ[1];
                FACRepetition[2] = veciAudioServ[0];
                FACRepetition[3] = veciAudioServ[1];
                FACRepetition[4] = veciDataServ[0];
            }
            else /* iNumData == 2 */
            {
                /* Init repetion vector */
                FACNumRep = 10;
                FACRepetition.resize(FACNumRep);

                /* A1A2A1A2D1A1A2A1A2D2 */
                FACRepetition[0] = veciAudioServ[0];
                FACRepetition[1] = veciAudioServ[1];
                FACRepetition[2] = veciAudioServ[0];
                FACRepetition[3] = veciAudioServ[1];
                FACRepetition[4] = veciDataServ[0];
                FACRepetition[5] = veciAudioServ[0];
                FACRepetition[6] = veciAudioServ[1];
                FACRepetition[7] = veciAudioServ[0];
                FACRepetition[8] = veciAudioServ[1];
                FACRepetition[9] = veciDataServ[1];
            }
        }
        else /* iNumAudio == 3 */
        {
            /* Init repetion vector */
            FACNumRep = 7;
            FACRepetition.resize(FACNumRep);

            /* A1A2A3A1A2A3D1 */
            FACRepetition[0] = veciAudioServ[0];
            FACRepetition[1] = veciAudioServ[1];
            FACRepetition[2] = veciAudioServ[2];
            FACRepetition[3] = veciAudioServ[0];
            FACRepetition[4] = veciAudioServ[1];
            FACRepetition[5] = veciAudioServ[2];
            FACRepetition[6] = veciDataServ[0];
        }
    }
}


/******************************************************************************\
* CFACReceive																   *
\******************************************************************************/
bool CFACReceive::FACParam(CVector<_BINARY>* pbiFACData,
                           CParameter& Parameter)
{
    /*
        First get new data from incoming data stream, then check if the new
        parameter differs from the old data stored in the receiver. If yes, init
        the modules to the new parameter
    */
    uint32_t	iTempServiceID;
    int			iTempShortID;

    /* CRC ------------------------------------------------------------------ */
    /* 1: Check the CRC of this data block */
    CRCObject.Reset(8);

    (*pbiFACData).ResetBitAccess();

    for (int i = 0; i < NUM_FAC_BITS_PER_BLOCK / SIZEOF__BYTE - 1; i++)
        CRCObject.AddByte((_BYTE) (*pbiFACData).Separate(SIZEOF__BYTE));

    bool permissive = Parameter.lenient_RSCI;

    //    cout<<" permissive = [ "<< permissive << "], CheckCRC: "
    // << CRCObject.CheckCRC((*pbiFACData).Separate(8))<<endl;   // printf: 1

    if (permissive || CRCObject.CheckCRC((*pbiFACData).Separate(8)))
    {
        /* CRC-check successful, extract data from FAC-stream */
        /* Reset separation function */
        (*pbiFACData).ResetBitAccess();

        Parameter.Lock();

        /* Channel parameters -----------------------------------------------
            信道参数如下：
            一共20比特
            -增强模式标识                                       1比特
            -当前复用帧帧号及AFS索引有效标识（超帧有关）            2比特
            -频谱占有 	                                      4比特
            -交织深度标记位	                                  1比特
            - MSC映射模式                                      2比特
            - SDC映射模式	                                     1比特
            -业务种类及数量	                                 4比特
            -再配置索引	                                     3比特
            -Rfu	                                         2比特  //FAC
        */
        /* Base/Enhancement flag (not used) */
        (*pbiFACData).Separate(1);

        /* Identity */
        switch ((*pbiFACData).Separate(2))
        {
        case 0: /* 00 */
            Parameter.iFrameIDReceiv = 0;
            break;

        case 1: /* 01 */
            Parameter.iFrameIDReceiv = 1;
            break;

        case 2: /* 10 */
            Parameter.iFrameIDReceiv = 2;
            break;

        case 3: /* 11 */
            Parameter.iFrameIDReceiv = 0;
            break;
        }

        /* Spectrum occupancy */
        switch ((*pbiFACData).Separate(4))
        {
        case 0: /* 0000 */
            Parameter.SetSpectrumOccup(SO_0);
            break;

        case 1: /* 0001 */
            Parameter.SetSpectrumOccup(SO_1);
            break;

        case 2: /* 0010 */
            Parameter.SetSpectrumOccup(SO_2);
            break;

        case 3: /* 0011 */
            Parameter.SetSpectrumOccup(SO_3);
            break;

        case 4: /* 0100 */
            Parameter.SetSpectrumOccup(SO_4);
            break;

        case 5: /* 0101 */
            Parameter.SetSpectrumOccup(SO_5);
            break;
        }

        /* Interleaver depth flag */
        switch ((*pbiFACData).Separate(1))
        {
        case 0: /* 0 */
            Parameter.SetInterleaverDepth(CParameter::SI_LONG);
            break;

        case 1: /* 1 */
            Parameter.SetInterleaverDepth(CParameter::SI_SHORT);
            break;
        }

        /* MSC mode
            MSC映射模式：此标识指出了MSC使用的QAM映射模式。
            00：64-QAM，非等级调制。
            01: 64-QAM,   I轴等级调制。
            10: 64-QAM,   I&Q轴等级调制。
            11: 16-QAM,   非等级调制。

        */
        switch ((*pbiFACData).Separate(2))
        {
        case 0: /* 00 */
            Parameter.SetMSCCodingScheme(CS_3_SM);
            break;

        case 1: /* 01 */
            Parameter.SetMSCCodingScheme(CS_3_HMMIX);
            break;

        case 2: /* 10 */
            Parameter.SetMSCCodingScheme(CS_3_HMSYM);
            break;

        case 3: /* 11 */

            Parameter.SetMSCCodingScheme(CS_2_SM);
            break;
        }

        /* SDC mode
            SDC映射模式：此标识指出了SDC使用的QAM映射模式。
            0: 16-QAM.
            1: 4-QAM.
        */
        switch ((*pbiFACData).Separate(1))
        {
        case 0: /* 0 */
            Parameter.SetSDCCodingScheme(CS_2_SM);
            break;

        case 1: /* 1 */
            Parameter.SetSDCCodingScheme(CS_1_SM);
            break;
        }

        /* Number of services
            业务种类及数量：此标识指出了参与复用的业务组成。
            0000:   4 音频业务
            0001:   1 数据业务.
            0010:   2 数据业务
            0011:   3 数据业务
            0100:   1 音频业务.
            0101:   1音频业务和1数据业务.
            0110:   1音频业务和2数据业务
            0111:   1音频业务和3数据业务
            1000:   2音频业务.
            1001:   2音频业务和1数据业务
            1010:   2音频业务和2数据业务
            1011:  保留
            1100:   3 音频业务
            1101:   3音频业务和1数据业务.
            1110:   保留
            1111:   4 数据业务.

             */
        /* Search table for entry */
        int iNumServTabEntry = (*pbiFACData).Separate(4);

        //音频 数据 个数 iNumServTabEntry = 5  0101

        for (int i = 0; i < 5; i++)
        {
            for (int j = 0; j < 5; j++)
            {
                if (iNumServTabEntry == iTableNumOfServices[i][j])
                {
                    Parameter.SetNumOfServices(i, j);
                    // 一个 音频  一个data
                    // cout<<"SetNumOfServices(i, j): = "<<i <<" "<<j <<endl;
                }
            }
        }


        /* Reconfiguration index (not used, yet)
        重新配置索引：此标识指出了复用重新配置状况和时间。
        该标识如果是非零值，则代表新的配置生效前需要传送携带原来配置信息的传输超帧的数目。
        见节 6.4.6.
        */
        (*pbiFACData).Separate(3);

        /* rfu */
        /* Do not use rfu */
        (*pbiFACData).Separate(2);


        /* Service parameters -----------------------------------------------
            业务参数如下：
            44比特
            - 业务标识		            24 比特
            - 短标识	(stream下标,流的标识) 2  比特
            - 音频条件接收标识		        1  比特
            - 语言标识			        4  比特
            - 音频/数据标识   	        1  比特
            - 业务描述标识                5  比特
            - 数据条件接收标识		        1  比特
            - Rfa				        6  比特          // FAC

            */
        /* Service identifier */
        iTempServiceID = (*pbiFACData).Separate(24);

        /* Short ID (the short ID is the index of the service-array) */

        /* 一个音频 一个文本  和文档中 描述是一样的
        iCurShortID = 0
        iCurShortID = 0
        iCurShortID = 0
        iCurShortID = 0
        iCurShortID = 1

        iCurShortID = 0
        iCurShortID = 0
        iCurShortID = 0
        iCurShortID = 0
        iCurShortID = 1
        */
        /*
        短标识：此标识指出该业务的短标识，并且用作SDC中的参考。
        短标识存在于整个业务持续期，并且在多路复用重新配置中保持不变。

        在整个服务过程中，Short Id 都会保持不变。
        即使系统进行了多路复用的重配置（例如改变数据流的排列或信道的分配），Short Id 仍然不变。
        这确保了在服务变化或配置更新时，系统仍能正确识别和追踪特定的服务。
        换句话说，无论传输或配置如何变化，Short Id 为该服务提供了一个持续且稳定的引用点，
        使得系统不会“失去”对该服务的跟踪。

        */
        iTempShortID = (*pbiFACData).Separate(2);
        // cout<<"iCurShortID = "<< iTempShortID << endl;

        //自定义
        Parameter.iFacShortID = iTempShortID;
        Parameter.iServiceID = iTempServiceID;

        /* Set service identifier */
        Parameter.SetServiceID(iTempShortID, iTempServiceID);

        /* CA indication
        音频条件接收标识：此标识指出该音频业务是否使用条件接收。
            0：音频流未使用CA系统（或是该业务不包含音频流）
            1：音频流使用CA系统。
            注1：具体细节见SDC数据实体类型2。
            接收机在播放音频流之前首先检测“音频CA标记”位。如果“音频CA标记”设置为1，
            那么未获得CA授权的接收机将无法解码该音频流。

        */
        switch ((*pbiFACData).Separate(1))
        {
        case 0: /* 0 */
            Parameter.Service[iTempShortID].eCAIndication =
                    CService::CA_NOT_USED;
            break;

        case 1: /* 1 */
            Parameter.Service[iTempShortID].eCAIndication =
                    CService::CA_USED;
            break;
        }

        /* Language */
        Parameter.Service[iTempShortID].iLanguage = (*pbiFACData).Separate(4);

        /* Audio/Data flag
            音频/数据标识：此标识指出该业务是音频业务还是数据业务。
            0: 音频业务
            1: 数据业务.
        */
        switch ((*pbiFACData).Separate(1))
        {
        case 0: /* 0 */
            Parameter.SetAudDataFlag(iTempShortID, CService::SF_AUDIO);
            break;

        case 1: /* 1 */
            Parameter.SetAudDataFlag(iTempShortID, CService::SF_DATA);
            break;
        }

        /* Service descriptor
        业务描述标识：该标识的含义依赖于“音频/数据标记”的值。
        音频/数据标识为 0 ：该标识表示节目类型
        音频/数据标识为 1： 该标识表示应用ID(数据ID)
        不管“音频/数据标识”为何值，值31（5比特 所有位均置1）都表示标准接收机应该跳过该业务，
        继续搜索其它业务。
          注3：这将允许工程测试信号通过接收机时被忽略。

        */
        Parameter.Service[iTempShortID].iServiceDescr =
                (*pbiFACData).Separate(5);

        Parameter.Unlock();

        /* Rfa */
        /* Do not use Rfa
            数据条件接收标识：此标识指出该数据业务是否使用条件接收。
                0：数据流或数据子流未使用CA系统（或是该业务不包含数据流或数据子流）。
                1：数据流或数据子流使用CA系统
            注4：详情见SDC数据实体类型2。
            接收机在显示业务的数据流 / 子流之前都要检测“数据CA标记”位。
            如果“数据CA标记”设置为1，未获得CA授权的接收机无法解码该数据流/子流。
            Rfa：这6比特位留作将来追加定义，在被定义之前应被置为0。

        */
        (*pbiFACData).Separate(7);

        /* CRC is ok, return true */
        return true;
    }
    else
    {
        /* Data is corrupted, do not use it. Return failure as false */
        return false;
    }
}
