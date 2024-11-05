/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	DRM-transmitter
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

#include "DrmTransmitter.h"
#include <sstream>
#ifdef QT_MULTIMEDIA_LIB
# include <QAudioDeviceInfo>
#endif

#include "messagequeue.h"

#include "receivermanager.h"
std::vector<unsigned char> convertToCharStream(CSingleBuffer<_COMPLEX>& buffer);


/* Implementation *************************************************************/
void CDRMTransmitter::Run()
{

    //获取 复用器 ；复用流
    CMSCmultiplexer* pMSCmultiplexer = this->GetMSCmultiplexer();   // 发射机中的复用器
    static CSingleBuffer<_BINARY>* MSCBuf = NULL;       // 复用流

    MSCBuf = GetMSCmultiplexer()->GetMuxBuf();          // MSCBuf 复用流
    // MSCBuf = &AudSrcBuf;                              // 发射机中的 AudSrcBuf

    Parameters.reconfigFlag = true;   // #####
    for (;;)
    {
#if 0
        /* MSC 源码****************************************************************/
        /* Read the source signal */
        ReadData.ReadData(Parameters, DataBuf);

        /* Audio source encoder */
        AudioSourceEncoder.ProcessData(Parameters, DataBuf, AudSrcBuf);

        /* MLC-encoder */
        MSCMLCEncoder.ProcessData(Parameters, AudSrcBuf, MLCEncBuf);
#else

        CSubDRMReceiver* pSubDRMReceiver = nullptr;
        //  组装流  遍历"源"
        if (pReceiverManager_m == nullptr)
        {
            //
            cout << " 管理类 指针为空" << endl;
        }
        else
        {
            //选择子接收机 数据源
            pSubDRMReceiver = pReceiverManager_m->getSubReceiver(1);
            if (pSubDRMReceiver == nullptr)
            {
                //
                cout << "子发射机类 指针为空" << endl;
                continue;
            }
            else
            {
                //
                // pSubDRMReceiver->resizeVloopBuff(16*1024);
            }
        }

        // 填写 发射机 "参数类"
        // FAC
        if (pSubDRMReceiver->bHasEnoughDataFAC)  //正常 打印出是: 1
        {
            CUtilizeFACData* pUtilizeFACData = pSubDRMReceiver->GetFAC();  // 根据FAC填写参数

            bool ret = pUtilizeFACData->WriteData(this->Parameters,                   // 发射机参数类
                                                  *(pSubDRMReceiver->GetFACSendBuf()));// 子接收机 FAC buff
            // cout << "FAC 给参数类填写:" << (ret == true ? "成功" : "不成功") << endl;

            pSubDRMReceiver->bHasEnoughDataFAC = false;
        }

        // SDC
        if (pSubDRMReceiver->bHasEnoughDataSDC)
        {

            CUtilizeSDCData* pUtilizeSDCData = pSubDRMReceiver->GetSDC(); // 根据SDC填写参数
            bool sdc_ret = pUtilizeSDCData->WriteData(this->Parameters,                  // 发射机参数类
                                                      *(pSubDRMReceiver->GetSDCSendBuf()));//子接收机 SDC buff

            // cout << "SDC 给参数类填写:" << (sdc_ret == true ? "成功" : "不成功") << endl;

            pSubDRMReceiver->bHasEnoughDataSDC = false;

            //是否重新配置
            if (pSubDRMReceiver->GetParameters()->reconfigFlag)
            {
                //填写完 参数后, 重新初始化 发射机
                Init();
                cout<<" 重新配置-pSubDRMReceiver->GetParameters()- "<<endl;
                pSubDRMReceiver->GetParameters()->reconfigFlag = false;

            }
            if (Parameters.reconfigFlag)
            {
                Init();
                cout<<" 重新配置-Parameters.reconfigFlag- "<<endl;
                Parameters.reconfigFlag = false;
            }
        }

        // MSC 数据复用

        std::vector<CSingleBuffer<_BINARY>>* pMSCUseBuf = nullptr;
        if (MSCBuf->GetRequestFlag())  //
            if (!pSubDRMReceiver->inUseQueue.empty())
            {

                pMSCUseBuf =  pSubDRMReceiver->inUseQueue.front() ;
                pSubDRMReceiver->inUseQueue.pop();
                if(pMSCUseBuf == nullptr)
                {
                    cout<<"tx:pMSCUseBuf == nullptr"<<endl;
                    continue;
                }
                // 流复用

                //---复用器初始化---------   (发射机中的)
                pMSCmultiplexer->InitInternal(Parameters);
                // MSC
                if(1)
                {
                    MSCBuf->Clear(); //复用流buffer 清空 ， 把新的 MSC数据 填入复用流buff中


                    for (int i = 0; i < MAX_NUM_STREAMS; i++)
                    {
                        // 子接收机 每个流的 ID
                        pMSCmultiplexer->streamID = i;
                        // 每个流的 input输入个数
                        int OutputBlockSize = pMSCmultiplexer->m_veciOutputBlockSize[i];

                        pMSCmultiplexer->setInputBlockSize(OutputBlockSize);
                        pMSCmultiplexer->setOutputBlockSize(OutputBlockSize);

                        // int sizemsc = ((*pMSCUseBuf))[i].GetFillLevel();
                        // 0:  size msc = 117744 ;TODO:接收机先跑起来， 就会put 很多filllevel，实际数据应该是 5352(现象)
                        //                        if (sizemsc)
                        //cout <<"tx:"<< i << ": " << "MSC  size = " << sizemsc << endl;

                        // 复用
                        pMSCmultiplexer->ProcessData(Parameters,
                                                     (*pMSCUseBuf)[i], // 子接收机 MSC buff
                                                     *MSCBuf);         // 发射机中复用器 的复用流

                        //  str0 消费个数 5,352    str1: 552
                        MSCBuf->SetRequestFlag(true);
                    }


                    MSCBuf->SetRequestFlag(false);
                }
                //目前现象是 (所有流的数据个数加在一起)
                cout << "复用后:MSCBuf(复用流)->GetFillLevel: " << MSCBuf->GetFillLevel() << std::endl;

                // pSubDRMReceiver->bHasEnoughDataMSC--;
                pSubDRMReceiver->availableQueue.push( pMSCUseBuf);
            }

        // 6000.0000000Hz;  输出信号类型 CTransmitData::OF_REAL_VAL OF_IQ_NEG
        //configureIFSettings(0.0000000, CTransmitData::OF_IQ_NEG);

        // cout<<"tx drm Mode: "<< this->Parameters.GetWaveMode()<<endl;
        /* MLC-encoder *********************************************************/
        // 表J.1: EEP SM 模式A 中每一复用帧的输入比特数L: 5904
        // 模式 A  频谱占用3       insize: 5904   (bit )
        // 每个复用帧中的MSC单元数  outsize: 2959  (QAM cell num)
        static bool encoder = false;
        if (MLCEncBuf.GetRequestFlag())
        {
            encoder = true;
            cout<<"MSCMLCEncoder InputBlockSize: "
            <<MSCMLCEncoder.GetInputBlockSize()<<endl;
            cout<<"MSCBuf->GetFillLevel(): "
            << MSCBuf->GetFillLevel()<<endl;
            // MSCMLCEncoder OutputBlockSize: 2959
            cout<<"MSCMLCEncoder OutputBlockSize: "
            <<MSCMLCEncoder.GetOutputBlockSize()<<endl;

        }
        MSCMLCEncoder.ProcessData(Parameters, *MSCBuf, MLCEncBuf);
        if (encoder)
        {
            // cout<< "Encoder: MSCBuf->GetFillLevel(): " << MSCBuf->GetFillLevel()<<endl;
            encoder = false;
        }


#endif ///

        /* Convolutional interleaver */
        //        if(MLCEncBuf.GetFillLevel()!= 0)
        //            cout<<"MLCEncBuf GetFillLevel()"<< MLCEncBuf.GetFillLevel()<<std::endl;
        //        SymbInterleaver InputBlockSize: 2959
        //        SymbInterleaver OutputBlockSize: 2959

        //        cout<<"SymbInterleaver InputBlockSize: " <<SymbInterleaver.GetInputBlockSize()<<endl;
        //        cout<<"SymbInterleaver OutputBlockSize: "<<SymbInterleaver.GetOutputBlockSize()<<endl;
        //        cout<<"IntlBuf.GetRequestFlag: "<< IntlBuf.GetRequestFlag() << endl;
        SymbInterleaver.ProcessData(Parameters, MLCEncBuf, IntlBuf);


        /* FAC ****************************************************************/
        // 模式 A  频谱占用3   input size:72
        //                   outsize:65        QAM cell num
        GenerateFACData.ReadData(Parameters, GenFACDataBuf);

        //        if(GenFACDataBuf.GetFillLevel()!= 0)
        //            cout<<"GenFACDataBuf GetFillLevel()"<< GenFACDataBuf.GetFillLevel()<<std::endl;
        FACMLCEncoder.ProcessData(Parameters, GenFACDataBuf, FACMapBuf);


        /* SDC ****************************************************************/
        GenerateSDCData.ReadData(Parameters, GenSDCDataBuf);
        // 模式 A  频谱占用3   insize:399      表J.21: 模式 A  每个SDC块的输入比特数L
        //                   outsize: 405    表30：SDC  QAM单元数   QAM cell num
        //        if(GenSDCDataBuf.GetFillLevel()!= 0)
        //        {   // 打印
        //            cout<<"GenSDCDataBuf GetFillLevel()"<< GenSDCDataBuf.GetFillLevel()<<std::endl;
        //        }
        SDCMLCEncoder.ProcessData(Parameters, GenSDCDataBuf, SDCMapBuf);


        /* Mapping of the MSC, FAC, SDC and pilots on the carriers ************/
        // sizein1: 207(当前符号中 MSC 的数)
        // sizein2:FAC sizein3:SDC
        // sizeout: 229   -114 + 114 + 1 =  229
        //        if(CarMapBuf.GetRequestFlag())
        //        {
        //            static int falg_msc =0 ;
        //            static int falg_msc_ed = 0 ;
        //            falg_msc = IntlBuf.GetFillLevel() ;
        //            if(falg_msc != falg_msc_ed)
        //            {
        //                falg_msc = falg_msc_ed;
        //                cout<<"inputsize (MSC) = "<< OFDMCellMapping.GetInputBlockSize()<< endl;
        //                cout<<"inputsize (FAC) = "<< OFDMCellMapping.getInputBlockSize2()<< endl;
        //                cout<<"inputsize (SDC) = "<< OFDMCellMapping.getInputBlockSize3()<< endl;

        //                cout<<"IntlBuf.GetFillLevel(MSC): "<<IntlBuf.GetFillLevel()<<endl;
        //                cout<<"FACMapBuf.GetFillLevel(FAC): "<<FACMapBuf.GetFillLevel()<<endl;
        //                cout<<"SDCMapBuf.GetFillLevel(SDC): "<<SDCMapBuf.GetFillLevel()<<endl;

        //            }

        //        }
        OFDMCellMapping.ProcessData(Parameters, IntlBuf, FACMapBuf, SDCMapBuf, CarMapBuf);


        /* OFDM-modulation ****************************************************/

        //        if(CarMapBuf.GetFillLevel()!= 0)
        //        {
        //            static int i = 0;
        //            cout<< i++ <<":CarMapBuf GetFillLevel(载波数)"
        //                << CarMapBuf.GetFillLevel()<<std::endl;    //  229

        //        }
        OFDMModulation.ProcessData(Parameters, CarMapBuf, OFDMModBuf);

        /* Soft stop **********************************************************/
        if (CanSoftStopExit())
            break;

        // 没有声音 第一种原因是: MSC数据不足,符号生成
        /* Transmit the signal ************************************************/
        TransmitData.WriteData(Parameters, OFDMModBuf);

    }
}



//to  char
std::vector<unsigned char> convertToCharStream(CSingleBuffer<_COMPLEX>& buffer)
{
    std::vector<unsigned char> charStream;
    CVectorEx<_COMPLEX>* tempbuff = buffer.QueryWriteBuffer();
    // cout<<"sizeof(_COMPLEX) = "<< sizeof(_COMPLEX) << endl;  //sizeof(_COMPLEX) = 16

    if (tempbuff == nullptr || tempbuff->empty())
    {
        cout << "tempbuff == nullptr || tempbuff->empty()" << endl;
        return charStream;  // 如果缓冲区为空，直接返回空的字节流
    }
    // cout<<"tempbuff->size() = "<<tempbuff->size()<< endl;  //640
    size_t dataSize = tempbuff->size() * sizeof(_COMPLEX);
    charStream.resize(dataSize);  // 640
    std::memcpy(charStream.data(), tempbuff->data(), dataSize);
    return charStream;
}


bool CDRMTransmitter::CanSoftStopExit()
{
    /* Set new symbol flag */
    const bool bNewSymbol = OFDMModBuf.GetFillLevel() != 0;

    if (bNewSymbol)
    {
        /* Number of symbol by frame */
        const int iSymbolPerFrame = Parameters.CellMappingTable.iNumSymPerFrame;

        /* Set stop requested flag  eRunState*/
        //使用信号槽

        //      const bool bStopRequested = false; //原来的

        //Parameters.eRunState != CParameter::RUNNING;

#if true
        /* Flavour 1: Stop at the frame boundary (worst case delay one frame) */
        /* The soft stop is always started at the beginning of a new frame */
        //        DEBUG_COUT("bStopRequested: " << *(this->peRunState_m)
        //                   << "iSoftStopSymbolCount(符号数量): " <<iSoftStopSymbolCount);
        if (((*(this->peRunState_m) != 1/*CParameter::RUNNING*/)
             && iSoftStopSymbolCount == 0) ||
                iSoftStopSymbolCount < 0)
        {
            DEBUG_COUT("准备关闭...");
            /* Data in OFDM buffer are set to zero */
            OFDMModBuf.QueryWriteBuffer()->Reset(_COMPLEX());
            //void InitSoftStop() { iSoftStopSymbolCount=0; }

            /* The zeroing will continue until the frame end */
            if (--iSoftStopSymbolCount < -iSymbolPerFrame)
                return true; /* End of frame reached, signal that loop exit must be done */
        }
#else
        /* Flavour 2: Stop at the symbol boundary (worst case delay two frame) */
        /* Check if stop is requested */
        if (bStopRequested || iSoftStopSymbolCount < 0)
        {
            /* Reset the counter if positif */
            if (iSoftStopSymbolCount > 0)
                iSoftStopSymbolCount = 0;

            /* Data in OFDM buffer are set to zero */
            OFDMModBuf.QueryWriteBuffer()->Reset(_COMPLEX());

            /* Zeroing only this symbol, the next symbol will be an exiting one */
            if (--iSoftStopSymbolCount < -1)
            {
                TransmitData.FlushData();
                return true; /* Signal that a loop exit must be done */
            }
#endif
            else
            {
                /* Update the symbol counter to keep track of frame beginning */
                if (++iSoftStopSymbolCount >= iSymbolPerFrame)
                {
                    iSoftStopSymbolCount = 0;
                    // std::cout<<"++iSoftStopSymbolCount = "<<iSoftStopSymbolCount <<std::endl;
                }

            }
        }
        return false; /* Signal to continue the normal operation */
    }

    void CDRMTransmitter::EnumerateInputs(vector<string>&names, vector<string>&descriptions, string & defaultInput)
    {
        ReadData.Enumerate(names, descriptions, defaultInput);
    }

    void CDRMTransmitter::EnumerateOutputs(vector<string>&names,
                                           vector<string>&descriptions,
                                           string & defaultOutput)
    {
        TransmitData.Enumerate(names, descriptions, defaultOutput);
    }

    void CDRMTransmitter::doSetInputDevice()
    {
        if (indev.empty())
        {
            indev = "default";
        }
        else
        {
            qDebug() << "发射机: doSetInputDevice() " << QString::fromStdString(indev) << endl;
        }
        ReadData.SetSoundInterface(indev);
    }

    void CDRMTransmitter::doSetOutputDevice()
    {
        if (outdev.empty())
        {
            outdev = "default";
        }
        else
        {
            qDebug() << "发射机: doSetOutputDevice() " << QString::fromStdString(outdev) << endl;
        }

        TransmitData.SetSoundInterface(outdev);
    }

    void CDRMTransmitter::SetInputDevice(string device)
    {
        qDebug() << "发射机更改 Input设备: SetInputDevice = " << QString::fromStdString(device);
        indev = device;
        ReadData.SetSoundInterface(indev);
    }

    void CDRMTransmitter::SetOutputDevice(string device)
    {
        outdev = device;
        DEBUG_COUT("发射机更改输出设备: ");
        DEBUG_COUT("outdev = " << QString::fromStdString(device));
        TransmitData.SetSoundInterface(outdev);
    }

    CSettings* CDRMTransmitter::GetSettings()
    {
        return pSettings;
    }

    void CDRMTransmitter::Init()
    {

        /* Fetch new sample rate if any */
        Parameters.FetchNewSampleRate();  //##### 检查在哪设置 ，是否需要设置?

        /* Init cell mapping table 根据鲁棒模式以及带宽初始化QAM映射表 */
        //        cout<<"tx:Parameters.GetWaveMode() = "<<Parameters.GetWaveMode()
        //           <<" tx:Parameters.GetSpectrumOccup() = "<<Parameters.GetSpectrumOccup()
        //           << endl;
        Parameters.InitCellMapTable(Parameters.GetWaveMode(), Parameters.GetSpectrumOccup());

        /* Defines number of cells, important! */
        OFDMCellMapping.Init(Parameters, CarMapBuf);

        /* Defines number of SDC bits per super-frame */
        SDCMLCEncoder.Init(Parameters, SDCMapBuf);

        MSCMLCEncoder.Init(Parameters, MLCEncBuf);
        SymbInterleaver.Init(Parameters, IntlBuf);
        GenerateFACData.Init(Parameters, GenFACDataBuf);
        FACMLCEncoder.Init(Parameters, FACMapBuf);
        GenerateSDCData.Init(Parameters, GenSDCDataBuf);
        OFDMModulation.Init(Parameters, OFDMModBuf);
        AudioSourceEncoder.Init(Parameters, AudSrcBuf);
        ReadData.Init(Parameters, DataBuf);
        TransmitData.Init(Parameters);

        /* (Re)Initialization of the buffers */
        CarMapBuf.Clear();
        SDCMapBuf.Clear();
        MLCEncBuf.Clear();
        IntlBuf.Clear();
        GenFACDataBuf.Clear();
        FACMapBuf.Clear();
        GenSDCDataBuf.Clear();
        OFDMModBuf.Clear();
        AudSrcBuf.Clear();
        DataBuf.Clear();

        /* Initialize the soft stop */
        InitSoftStop();

#ifdef QT_MULTIMEDIA_LIB
        doSetInputDevice();
        cout << "outdev[" << outdev << "]" << endl;  // 默认
        outdev = TransmitData.GetSoundInterface();
        doSetOutputDevice();
#endif
    }

    CDRMTransmitter::~CDRMTransmitter()
    {}

    CDRMTransmitter::CDRMTransmitter(CSettings * nPsettings) : CDRMTransceiver(),
        ReadData(), TransmitData(),
        rDefCarOffset(VIRTUAL_INTERMED_FREQ),
        // UEP only works with Dream receiver, FIXME! -> disabled for now
        bUseUEP(false), Parameters(*(new CParameter())),
        pSettings(nPsettings)
    {

        indev = "default";
        outdev = "default";

        /* Init streams */
        Parameters.ResetServicesStreams();

        /* Init frame ID counter (index) */
        Parameters.iFrameIDTransm = 0;

        /* Init transmission of current time */
        Parameters.eTransmitCurrentTime = CParameter::CT_OFF;
        Parameters.bValidUTCOffsetAndSense = false;

        /**************************************************************************/
        /* Robustness mode and spectrum occupancy. Available transmission modes:
       RM_ROBUSTNESS_MODE_A: Gaussian channels, with minor fading,
       RM_ROBUSTNESS_MODE_B: Time and frequency selective channels, with longer
       delay spread,
       RM_ROBUSTNESS_MODE_C: As robustness mode B, but with higher Doppler
       spread,
       RM_ROBUSTNESS_MODE_D: As robustness mode B, but with severe delay and
       Doppler spread.
       Available bandwidths:
       SO_0: 4.5 kHz, SO_1: 5 kHz, SO_2: 9 kHz, SO_3: 10 kHz, SO_4: 18 kHz,
       SO_5: 20 kHz */
        Parameters.InitCellMapTable(RM_ROBUSTNESS_MODE_B, SO_3);

        /* Protection levels for MSC. Depend on the modulation scheme. Look at
       TableMLC.h, iCodRateCombMSC16SM, iCodRateCombMSC64SM,
       iCodRateCombMSC64HMsym, iCodRateCombMSC64HMmix for available numbers */
        Parameters.MSCPrLe.iPartA = 0;
        Parameters.MSCPrLe.iPartB = 1;
        Parameters.MSCPrLe.iHierarch = 0;

        /* Either one audio or one data service can be chosen */
        bool bIsAudio = true;

        /* In the current version only one service and one stream is supported. The
       stream IDs must be 0 in both cases */
        if (bIsAudio)
        {
            /* Audio */
            Parameters.SetNumOfServices(1, 0);
            Parameters.SetCurSelAudioService(0);

            CAudioParam AudioParam;

            AudioParam.iStreamID = 0;

            /* Text message */
            AudioParam.bTextflag = true;

            Parameters.SetAudioParam(0, AudioParam);

            Parameters.SetAudDataFlag(0, CService::SF_AUDIO);

            /* Programme Type code (see TableFAC.h, "strTableProgTypCod[]") */
            Parameters.Service[0].iServiceDescr = 15; /* 15 -> other music */

            Parameters.SetCurSelAudioService(0);
        }
        else
        {
            /* Data */
            Parameters.SetNumOfServices(0, 1);
            Parameters.SetCurSelDataService(0);

            Parameters.SetAudDataFlag(0, CService::SF_DATA);

            CDataParam DataParam;

            DataParam.iStreamID = 0;

            /* Init SlideShow application */
            DataParam.iPacketLen = 45; /* TEST */
            DataParam.eDataUnitInd = CDataParam::DU_DATA_UNITS;
            DataParam.eAppDomain = CDataParam::AD_DAB_SPEC_APP;
            Parameters.SetDataParam(0, DataParam);

            /* The value 0 indicates that the application details are provided
           solely by SDC data entity type 5 */
            Parameters.Service[0].iServiceDescr = 0;
        }

        /* Init service parameters, 24 bit unsigned integer number */
        Parameters.Service[0].iServiceID = 0;

        /* Service label data. Up to 16 bytes defining the label using UTF-8
       coding */
        Parameters.Service[0].strLabel = "Dream Test";

        /* Language (see TableFAC.h, "strTableLanguageCode[]") */
        Parameters.Service[0].iLanguage = 5; /* 5 -> english */

        /* Interleaver mode of MSC service. Long interleaving (2 s): SI_LONG,
       short interleaving (400 ms): SI_SHORT */
        Parameters.eSymbolInterlMode = CParameter::SI_LONG;

        /* MSC modulation scheme. Available modes:
       16-QAM standard mapping (SM): CS_2_SM,
       64-QAM standard mapping (SM): CS_3_SM,
       64-QAM symmetrical hierarchical mapping (HMsym): CS_3_HMSYM,
       64-QAM mixture of the previous two mappings (HMmix): CS_3_HMMIX */
        Parameters.eMSCCodingScheme = CS_3_SM;

        /* SDC modulation scheme. Available modes:
       4-QAM standard mapping (SM): CS_1_SM,
       16-QAM standard mapping (SM): CS_2_SM */
        Parameters.eSDCCodingScheme = CS_2_SM;

        /* Set desired intermedia frequency (IF) in Hertz */
        SetCarOffset(VIRTUAL_INTERMED_FREQ); /* Default: "VIRTUAL_INTERMED_FREQ" */

        if (bUseUEP)
        {
            // TEST
            Parameters.SetStreamLen(0, 80, 0);
        }
        else
        {
            /* Length of part B is set automatically (equal error protection (EEP),
           if "= 0"). Sets the number of bytes, should not exceed total number
           of bytes available in MSC block */
            Parameters.SetStreamLen(0, 0, 0);
        }
    }

    void CDRMTransmitter::LoadSettings()
    {
        if (pSettings == nullptr) return;
        CSettings& s = *pSettings;

        const char* Transmitter = "Transmitter";
        std::ostringstream oss;
        string value, service;

        /* Sound card audio sample rate */
        Parameters.SetNewAudSampleRate(s.Get(Transmitter, "samplerateaud", int(DEFAULT_SOUNDCRD_SAMPLE_RATE)));

        /* Sound card signal sample rate */
        Parameters.SetNewSoundcardSigSampleRate(s.Get(Transmitter, "sampleratesig", int(DEFAULT_SOUNDCRD_SAMPLE_RATE)));

        /* Fetch new sample rate if any */
        Parameters.FetchNewSampleRate();

        /* Sound card input device id */
        ReadData.SetSoundInterface(s.Get(Transmitter, "snddevin", string()));

        /* Sound card output device id */
        TransmitData.SetSoundInterface(s.Get(Transmitter, "snddevout", string()));
#if 0 // TODO
        /* Sound clock drift adjustment 声音调整时钟漂移 */
        bool bEnabled = s.Get(Transmitter, "sndclkadj", int(0));
        ((CSoundOutPulse*) pSoundOutInterface)->EnableClockDriftAdj(bEnabled);
#endif
        /* Robustness mode and spectrum occupancy */
        ERobMode eRobustnessMode = RM_ROBUSTNESS_MODE_B;
        ESpecOcc eSpectOccup = SO_3;
        /* Robustness mode */
        value = s.Get(Transmitter, "robustness", string("RM_ROBUSTNESS_MODE_B"));
        if (value == "RM_ROBUSTNESS_MODE_A") { eRobustnessMode = RM_ROBUSTNESS_MODE_A; }
        else if (value == "RM_ROBUSTNESS_MODE_B") { eRobustnessMode = RM_ROBUSTNESS_MODE_B; }
        else if (value == "RM_ROBUSTNESS_MODE_C") { eRobustnessMode = RM_ROBUSTNESS_MODE_C; }
        else if (value == "RM_ROBUSTNESS_MODE_D") { eRobustnessMode = RM_ROBUSTNESS_MODE_D; }
        /* Spectrum occupancy */
        value = s.Get(Transmitter, "spectocc", string("SO_3"));
        if (value == "SO_0") { eSpectOccup = SO_0; }
        else if (value == "SO_1") { eSpectOccup = SO_1; }
        else if (value == "SO_2") { eSpectOccup = SO_2; }
        else if (value == "SO_3") { eSpectOccup = SO_3; }
        else if (value == "SO_4") { eSpectOccup = SO_4; }
        else if (value == "SO_5") { eSpectOccup = SO_5; }
        Parameters.InitCellMapTable(eRobustnessMode, eSpectOccup);

        /* Protection level for MSC */
        Parameters.MSCPrLe.iPartB = s.Get(Transmitter, "protlevel", int(1));

        /* Interleaver mode of MSC service */
        value = s.Get(Transmitter, "interleaver", string("SI_LONG"));
        if (value == "SI_SHORT") { Parameters.eSymbolInterlMode = CParameter::SI_SHORT; }
        else if (value == "SI_LONG") { Parameters.eSymbolInterlMode = CParameter::SI_LONG; }

        /* MSC modulation scheme */
        value = s.Get(Transmitter, "msc", string("CS_3_SM"));
        if (value == "CS_2_SM") { Parameters.eMSCCodingScheme = CS_2_SM; }
        else if (value == "CS_3_SM") { Parameters.eMSCCodingScheme = CS_3_SM; }
        else if (value == "CS_3_HMSYM") { Parameters.eMSCCodingScheme = CS_3_HMSYM; }
        else if (value == "CS_3_HMMIX") { Parameters.eMSCCodingScheme = CS_3_HMMIX; }

        /* SDC modulation scheme */
        value = s.Get(Transmitter, "sdc", string("CS_2_SM"));
        if (value == "CS_1_SM") { Parameters.eSDCCodingScheme = CS_1_SM; }
        else if (value == "CS_2_SM") { Parameters.eSDCCodingScheme = CS_2_SM; }

        //-------------
        /* IF frequency */
        SetCarOffset(s.Get(Transmitter, "iffreq", double(GetCarOffset())));
        /* IF format */
        value = s.Get(Transmitter, "ifformat", string("OF_REAL_VAL"));
        // cout<<"IF format = "<<value<<endl;
        if (value == "OF_REAL_VAL") { GetTransData()->SetIQOutput(CTransmitData::OF_REAL_VAL); }
        else if (value == "OF_IQ_POS") { GetTransData()->SetIQOutput(CTransmitData::OF_IQ_POS); }
        else if (value == "OF_IQ_NEG") { GetTransData()->SetIQOutput(CTransmitData::OF_IQ_NEG); }
        else if (value == "OF_EP") { GetTransData()->SetIQOutput(CTransmitData::OF_EP); }

        /* IF high quality I/Q */
        GetTransData()->SetHighQualityIQ(s.Get(Transmitter, "hqiq", int(1)));
        /* IF amplified output 中频放大输出 */
        GetTransData()->SetAmplifiedOutput(s.Get(Transmitter, "ifamp", int(1)));

        /* Transmission of current time */
        value = s.Get(Transmitter, "currenttime", string("CT_OFF"));//时间设置为关闭状态 Off 不使用任何时间设置 。
        if (value == "CT_OFF") { Parameters.eTransmitCurrentTime = CParameter::CT_OFF; }
        else if (value == "CT_LOCAL") { Parameters.eTransmitCurrentTime = CParameter::CT_LOCAL; }
        if (value == "CT_UTC") { Parameters.eTransmitCurrentTime = CParameter::CT_UTC; }
        else if (value == "CT_UTC_OFFSET") { Parameters.eTransmitCurrentTime = CParameter::CT_UTC_OFFSET; }
        //-------------

        /**********************/
        /* Service parameters 1 */
        for (int i = 0; i < 1 /*MAX_NUM_SERVICES*/; i++) // TODO
        {
            oss.str("");
            oss.clear(); // 清除状态标志

            oss << Transmitter << " Service " << i + 1;
            service = oss.str();
            //DEBUG_COUT("service["<<QString::fromStdString( service) << "]"); //根据文件中的键值对
            CService& Service = Parameters.Service[i];

            /* Service ID */
            Service.iServiceID = s.Get(service, "id", int(Service.iServiceID));
            // DEBUG_COUT("根据文件中的键值对 Service ID["<< Service.iServiceID << "]"); //根据文件中的键值对
            /* Service label data */
            Service.strLabel = s.Get(service, "label", string(Service.strLabel));

            /* Service description */
            Service.iServiceDescr = s.Get(service, "description", int(Service.iServiceDescr));

            /* Language  3 中文 */
            Service.iLanguage = s.Get(service, "language", int(Service.iLanguage));

            /* Audio codec */
            value = s.Get(service, "codec", string("AAC"));
            if (value == "AAC") { Service.AudioParam.eAudioCoding = CAudioParam::AC_AAC; }
            else if (value == "Opus") { Service.AudioParam.eAudioCoding = CAudioParam::AC_OPUS; }

            /* Opus Codec Channels */
            value = s.Get(service, "Opus_Channels", string("OC_STEREO"));
            if (value == "OC_MONO") { Service.AudioParam.eOPUSChan = CAudioParam::OC_MONO; }
            else if (value == "OC_STEREO") { Service.AudioParam.eOPUSChan = CAudioParam::OC_STEREO; }

            /* Opus Codec Bandwith */
            value = s.Get(service, "Opus_Bandwith", string("OB_FB"));
            if (value == "OB_NB") { Service.AudioParam.eOPUSBandwidth = CAudioParam::OB_NB; }
            else if (value == "OB_MB") { Service.AudioParam.eOPUSBandwidth = CAudioParam::OB_MB; }
            else if (value == "OB_WB") { Service.AudioParam.eOPUSBandwidth = CAudioParam::OB_WB; }
            else if (value == "OB_SWB") { Service.AudioParam.eOPUSBandwidth = CAudioParam::OB_SWB; }
            else if (value == "OB_FB") { Service.AudioParam.eOPUSBandwidth = CAudioParam::OB_FB; }

            /* Opus Forward Error Correction */
            value = s.Get(service, "Opus_FEC", string("0"));
            if (value == "0") { Service.AudioParam.bOPUSForwardErrorCorrection = false; }
            else if (value == "1") { Service.AudioParam.bOPUSForwardErrorCorrection = true; }

            /* Opus encoder signal type */
            value = s.Get(service, "Opus_Signal", string("OG_MUSIC"));
            if (value == "OG_VOICE") { Service.AudioParam.eOPUSSignal = CAudioParam::OG_VOICE; }
            else if (value == "OG_MUSIC") { Service.AudioParam.eOPUSSignal = CAudioParam::OG_MUSIC; }

            /* Opus encoder intended application */
            value = s.Get(service, "Opus_Application", string("OA_AUDIO"));
            if (value == "OA_VOIP") { Service.AudioParam.eOPUSApplication = CAudioParam::OA_VOIP; }
            else if (value == "OA_AUDIO") { Service.AudioParam.eOPUSApplication = CAudioParam::OA_AUDIO; }
        }
    }

    void CDRMTransmitter::SaveSettings()
    {
        if (pSettings == nullptr) return;
        CSettings& s = *pSettings;

        const char* Transmitter = "Transmitter";
        std::ostringstream oss;
        string value, service;

        /* Fetch new sample rate if any */
        Parameters.FetchNewSampleRate();

        /* Sound card audio sample rate */
        s.Put(Transmitter, "samplerateaud", Parameters.GetAudSampleRate());

        /* Sound card signal sample rate */
        s.Put(Transmitter, "sampleratesig", Parameters.GetSigSampleRate());

        /* Sound card input device id */
        s.Put(Transmitter, "snddevin", ReadData.GetSoundInterface());

        /* Sound card output device id */
        s.Put(Transmitter, "snddevout", TransmitData.GetSoundInterface());
#if 0 // TODO
        /* Sound clock drift adjustment */
        s.Put(Transmitter, "sndclkadj",
              int(((CSoundOutPulse*) pSoundOutInterface)
                  ->IsClockDriftAdjEnabled()));
#endif
        /* Robustness mode */
        switch (Parameters.GetWaveMode())
        {
        case RM_ROBUSTNESS_MODE_A: value = "RM_ROBUSTNESS_MODE_A"; break;
        case RM_ROBUSTNESS_MODE_B: value = "RM_ROBUSTNESS_MODE_B"; break;
        case RM_ROBUSTNESS_MODE_C: value = "RM_ROBUSTNESS_MODE_C"; break;
        case RM_ROBUSTNESS_MODE_D: value = "RM_ROBUSTNESS_MODE_D"; break;
        default: value = "";
        }
        s.Put(Transmitter, "robustness", value);

        /* Spectrum occupancy */
        switch (Parameters.GetSpectrumOccup())
        {
        case SO_0: value = "SO_0"; break;
        case SO_1: value = "SO_1"; break;
        case SO_2: value = "SO_2"; break;
        case SO_3: value = "SO_3"; break;
        case SO_4: value = "SO_4"; break;
        case SO_5: value = "SO_5"; break;
        default: value = "";
        }
        s.Put(Transmitter, "spectocc", value);

        /* Protection level for MSC */
        s.Put(Transmitter, "protlevel", int(Parameters.MSCPrLe.iPartB));

        /* Interleaver mode of MSC service */
        switch (Parameters.eSymbolInterlMode)
        {
        case CParameter::SI_SHORT: value = "SI_SHORT"; break;
        case CParameter::SI_LONG:  value = "SI_LONG";  break;
        default: value = "";
        }
        s.Put(Transmitter, "interleaver", value);

        /* MSC modulation scheme */
        switch (Parameters.eMSCCodingScheme)
        {
        case CS_2_SM:    value = "CS_2_SM";    break;
        case CS_3_SM:    value = "CS_3_SM";    break;
        case CS_3_HMSYM: value = "CS_3_HMSYM"; break;
        case CS_3_HMMIX: value = "CS_3_HMMIX"; break;
        default: value = "";
        }
        s.Put(Transmitter, "msc", value);

        /* SDC modulation scheme */
        switch (Parameters.eSDCCodingScheme)
        {
        case CS_1_SM: value = "CS_1_SM"; break;
        case CS_2_SM: value = "CS_2_SM"; break;
        default: value = "";
        }
        s.Put(Transmitter, "sdc", value);

        /* IF frequency */
        s.Put(Transmitter, "iffreq", double(GetCarOffset()));

        /* IF format */
        switch (GetTransData()->GetIQOutput())
        {
        case CTransmitData::OF_REAL_VAL: value = "OF_REAL_VAL"; break;
        case CTransmitData::OF_IQ_POS:   value = "OF_IQ_POS";   break;
        case CTransmitData::OF_IQ_NEG:   value = "OF_IQ_NEG";   break;
        case CTransmitData::OF_EP:       value = "OF_EP";       break;
        default: value = "";
        }
        s.Put(Transmitter, "ifformat", value);

        /* IF high quality I/Q */
        s.Put(Transmitter, "hqiq", int(GetTransData()->GetHighQualityIQ()));

        /* IF amplified output */
        s.Put(Transmitter, "ifamp", int(GetTransData()->GetAmplifiedOutput()));

        /* Transmission of current time */
        switch (Parameters.eTransmitCurrentTime)
        {
        case CParameter::CT_OFF:        value = "CT_OFF";        break;
        case CParameter::CT_LOCAL:      value = "CT_LOCAL";      break;
        case CParameter::CT_UTC:        value = "CT_UTC";        break;
        case CParameter::CT_UTC_OFFSET: value = "CT_UTC_OFFSET"; break;
        default: value = "";
        }
        s.Put(Transmitter, "currenttime", value);

        /**********************/
        /* Service parameters */
        for (int i = 0; i < 1/*MAX_NUM_SERVICES*/; i++) // TODO
        {
            oss << Transmitter << " Service " << i + 1;
            service = oss.str();

            CService& Service = Parameters.Service[i];

            /* Service ID */
            s.Put(service, "id", int(Service.iServiceID));

            /* Service label data */
            s.Put(service, "label", string(Service.strLabel));

            /* Service description */
            s.Put(service, "description", int(Service.iServiceDescr));

            /* Language */
            s.Put(service, "language", int(Service.iLanguage));

            /* Audio codec */
            switch (Service.AudioParam.eAudioCoding)
            {
            case CAudioParam::AC_AAC:  value = "AAC";  break;
            case CAudioParam::AC_OPUS: value = "Opus"; break;
            default: value = "";
            }
            s.Put(service, "codec", value);

            /* Opus Codec Channels */
            switch (Service.AudioParam.eOPUSChan)
            {
            case CAudioParam::OC_MONO:   value = "OC_MONO";   break;
            case CAudioParam::OC_STEREO: value = "OC_STEREO"; break;
            default: value = "";
            }
            s.Put(service, "Opus_Channels", value);

            /* Opus Codec Bandwith */
            switch (Service.AudioParam.eOPUSBandwidth)
            {
            case CAudioParam::OB_NB:  value = "OB_NB";  break;
            case CAudioParam::OB_MB:  value = "OB_MB";  break;
            case CAudioParam::OB_WB:  value = "OB_WB";  break;
            case CAudioParam::OB_SWB: value = "OB_SWB"; break;
            case CAudioParam::OB_FB:  value = "OB_FB";  break;
            default: value = "";
            }
            s.Put(service, "Opus_Bandwith", value);

            /* Opus Forward Error Correction */
            value = Service.AudioParam.bOPUSForwardErrorCorrection ? "1" : "0";
            s.Put(service, "Opus_FEC", value);

            /* Opus encoder signal type */
            switch (Service.AudioParam.eOPUSSignal)
            {
            case CAudioParam::OG_VOICE: value = "OG_VOICE"; break;
            case CAudioParam::OG_MUSIC: value = "OG_MUSIC"; break;
            default: value = "";
            }
            s.Put(service, "Opus_Signal", value);

            /* Opus encoder intended application */
            switch (Service.AudioParam.eOPUSApplication)
            {
            case CAudioParam::OA_VOIP:  value = "OA_VOIP";  break;
            case CAudioParam::OA_AUDIO: value = "OA_AUDIO"; break;
            default: value = "";
            }
            s.Put(service, "Opus_Application", value);
        }
    }
