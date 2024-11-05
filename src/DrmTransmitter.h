/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See DrmTransmitter.cpp
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

#ifndef __DRMTRANSM_H
#define __DRMTRANSM_H

#include "Soundbar/soundbar.h"

#include <iostream>
#include "util/Buffer.h"
#include "Parameter.h"
#include "DataIO.h"
#include "mlc/MLC.h"
#include "interleaver/SymbolInterleaver.h"
#include "ofdmcellmapping/OFDMCellMapping.h"
#include "OFDM.h"
#include "ctransmitdata.h"
#include "sourcedecoders/AudioSourceEncoder.h"
#include "sound/soundinterface.h"
#include "DrmTransceiver.h"
#include "Parameter.h"
#include <unistd.h>

#include "ccmscmultiplexer.h"


class ReceiverManager;

/* Classes ********************************************************************/
class CDRMTransmitter : public CDRMTransceiver
{
public:
    CDRMTransmitter(CSettings* pSettings=nullptr);
    virtual ~CDRMTransmitter();

    void LoadSettings();
    void SaveSettings();
    void Init();

    CAudioSourceEncoder*	GetAudSrcEnc() {
        return &AudioSourceEncoder;
    }
    CTransmitData*			GetTransData() {
        return &TransmitData;
    }
    CReadData*				GetReadData() {
        return &ReadData;
    }

    void SetCarOffset(const _REAL rNewCarOffset)
    {
        /* Has to be set in OFDM modulation and transmitter filter module
            必须设置在OFDM调制和发射机滤波模块  */
        OFDMModulation.SetCarOffset(rNewCarOffset);
        TransmitData.SetCarOffset(rNewCarOffset);
        rDefCarOffset = rNewCarOffset;
    }
    _REAL GetCarOffset()
    {
        return rDefCarOffset;
    }
    void GetInputDevice(std::string& s) { s = indev; }
    void GetOutputDevice(std::string& s) { s = outdev; }
    void EnumerateInputs(std::vector<string>& names, std::vector<string>& descriptions, std::string& defaultInput);
    void EnumerateOutputs(std::vector<string>& names, std::vector<string>& descriptions, std::string& defaultOutput);
    void SetInputDevice(std::string);
    void SetOutputDevice(std::string);
    void doSetInputDevice();
    void doSetOutputDevice();
    virtual bool				IsReceiver() const { return false; }
    virtual bool				IsTransmitter() const { return true; }
    virtual CSettings*				GetSettings() ;
    virtual void					SetSettings(CSettings* pNewSettings) { pSettings = pNewSettings; }
    virtual CParameter*				GetParameters() {return &Parameters; }
    void Run();
    void Close() {ReadData.Stop(); TransmitData.Stop(); }


    ERunState* peRunState_m;

    //返回复用器的指针
    CMSCmultiplexer* GetMSCmultiplexer()
    {
        return &MSCmultiplexer;
    }
    ReceiverManager* pReceiverManager_m;    //内部成员


    // 自定义
    void configureIFSettings(_REAL frequency, CTransmitData::EOutFormat format,
                             bool highQuality  = true,
                             bool amplifiedOutput = true,
                             CParameter::ECurTime Time = CParameter::ECurTime::CT_OFF )
    {
        //IF 通常指的是 Intermediate Frequency（中间频率）
        SetCarOffset(frequency);                         /* IF frequency  6000.0000000 */
        GetTransData()->SetIQOutput(format);             /* IF format  IQ复数信号包含幅度和相位信息
                                                                , real valued */

        GetTransData()->SetHighQualityIQ(highQuality);   /* IF high quality I/Q ,默认是 1 */
        /* IF amplified output  */
        GetTransData()->SetAmplifiedOutput(amplifiedOutput); // 中频放大输出, 默认是 1

        Parameters.eTransmitCurrentTime = Time;  // CT_OFF
    }
protected:
    bool CanSoftStopExit();
    void InitSoftStop() { iSoftStopSymbolCount=0; }
    int GetSoftStopSymbolCount(){return iSoftStopSymbolCount; }

    // my复用器  自定义
    CMSCmultiplexer MSCmultiplexer;


    std::string indev  ;
    std::string outdev ;
    /* Buffers */
    CSingleBuffer<_SAMPLE>	DataBuf;
    CSingleBuffer<_BINARY>	AudSrcBuf;

    CSingleBuffer<_COMPLEX>	MLCEncBuf;
    CCyclicBuffer<_COMPLEX>	IntlBuf;

    CSingleBuffer<_BINARY>	GenFACDataBuf;
    CCyclicBuffer<_COMPLEX>	FACMapBuf;

    CSingleBuffer<_BINARY>	GenSDCDataBuf;
    CCyclicBuffer<_COMPLEX>	SDCMapBuf;

    CSingleBuffer<_COMPLEX>	CarMapBuf;
    CSingleBuffer<_COMPLEX>	OFDMModBuf;

    /* Modules */
    CMSCReadData            MSCReadData;


    CReadData				ReadData;
    CAudioSourceEncoder		AudioSourceEncoder;
    CMSCMLCEncoder			MSCMLCEncoder;
    CSymbInterleaver		SymbInterleaver;
    CGenerateFACData		GenerateFACData;
    CFACMLCEncoder			FACMLCEncoder;
    CGenerateSDCData		GenerateSDCData;
    CSDCMLCEncoder			SDCMLCEncoder;
    COFDMCellMapping		OFDMCellMapping;
    COFDMModulation			OFDMModulation;
    CTransmitData			TransmitData;

    _REAL					rDefCarOffset;
    bool				    bUseUEP;
    int						iSoftStopSymbolCount;
    CParameter&             Parameters;
    CSettings*              pSettings;
};


#endif // !defined(DRMTRANSM_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
