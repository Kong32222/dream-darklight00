#include "loopback.h"
#include "Parameter.h"
#ifdef QT_MULTIMEDIA_LIB
# include <QSet>
# include <QThread>
# include <QTimer>
# include <QEventLoop>
#else
# include "sound/soundinterfacefactory.h"
#endif
#include "sound/audiofilein.h"
#include "util/FileTyper.h"
#include "IQInputFilter.h"
#include "UpsampleFilter.h"
#include "matlib/MatlibSigProToolbox.h"
#include "tuner.h"
const static int SineTable[] = { 0, 1, 0, -1, 0 };
const static _REAL PSDWindowGain = 0.39638; /* power gain of the Hamming window */


//如果你将 inline 函数写在 .cpp 文件中，它只能在这个 .cpp 文件中使用。
// 想在多个 .cpp 文件中使用这个内联函数，通常需要将其放在头文件中。
inline _REAL sample2real(_SAMPLE s) {
    return _REAL(s);
}

/// 回环
Loopback::Loopback():
    vecrInpData(INPUT_DATA_VECTOR_SIZE, 0.0),
    bFippedSpectrum(false),
    eInChanSelection(CS_IQ_POS),  //  CS_MIX_CHAN    CS_IQ_POS  CS_IQ_POS_ZERO
    iPhase(0),spectrumAnalyser()
{
    this->SetInitFlag();  //  可以初始化模块 bDoInit = true;
}

_REAL Loopback::ConvertFrequency(_REAL rFrequency, bool bInvert) const
{
    const int iInvert = bInvert ? -1 : 1;

    if (eInChanSelection == CS_IQ_POS_SPLIT ||
            eInChanSelection == CS_IQ_NEG_SPLIT)
        rFrequency -= iSampleRate / 4 * iInvert;
    else if (eInChanSelection == CS_IQ_POS_ZERO ||
             eInChanSelection == CS_IQ_NEG_ZERO)
        rFrequency -= VIRTUAL_INTERMED_FREQ * iInvert;

    return rFrequency;
}

void Loopback::GetInputSpec(CVector<_REAL> &vecrData, CVector<_REAL> &vecrScale)
{
    spectrumAnalyser.setNegativeFrequency(eInChanSelection == CS_IQ_POS_SPLIT || eInChanSelection == CS_IQ_NEG_SPLIT);
    spectrumAnalyser.setOffsetFrequency((eInChanSelection == CS_IQ_POS_ZERO) || (eInChanSelection == CS_IQ_NEG_ZERO));
    mutexInpData.Lock();
    spectrumAnalyser.CalculateSpectrum(vecrInpData, NUM_SMPLS_4_INPUT_SPECTRUM);
    mutexInpData.Unlock();
    /* The calibration factor of 18.49 was determined experimentaly,
       give 0 dB for a full scale sine wave input (0 dBFS) */

    const _REAL rNormData = pow(_REAL(_MAXSHORT) * _REAL(NUM_SMPLS_4_INPUT_SPECTRUM),2) / 18.49;
    spectrumAnalyser.PSD2LogPSD(rNormData, vecrData, vecrScale);
}

void Loopback::Stop()
{
    //#ifdef QT_MULTIMEDIA_LIB
    //    if(pIODevice!=nullptr) {
    //        pIODevice->close();
    //        pIODevice = nullptr;
    //    }
    //    if(pAudioInput != nullptr) {
    //        pAudioInput->stop();
    //        delete pAudioInput;
    //        pAudioInput = nullptr;
    //    }
    //#endif
    //    if(pSound!=nullptr) {
    //        pSound->Close();
    //        pSound = nullptr;
    //    }
}

void Loopback::GetInputPSD(CVector<_REAL> &vecrData, CVector<_REAL> &vecrScale,
                           const int iLenPSDAvEachBlock, const int iNumAvBlocksPSD,
                           const int iPSDOverlap)
{
    spectrumAnalyser.setNegativeFrequency(eInChanSelection == CS_IQ_POS_SPLIT || eInChanSelection == CS_IQ_NEG_SPLIT);
    spectrumAnalyser.setOffsetFrequency((eInChanSelection == CS_IQ_POS_ZERO) || (eInChanSelection == CS_IQ_NEG_ZERO));
    mutexInpData.Lock();
    spectrumAnalyser.CalculateLinearPSD(vecrInpData, iLenPSDAvEachBlock, iNumAvBlocksPSD, iPSDOverlap);
    mutexInpData.Unlock();

    const _REAL rNormData =  pow(_REAL(_MAXSHORT) * _REAL(iLenPSDAvEachBlock), 2) * _REAL(iNumAvBlocksPSD) * PSDWindowGain;

    spectrumAnalyser.PSD2LogPSD(rNormData, vecrData, vecrScale);
}

_REAL Loopback::HilbertFilt(const _REAL rRe, const _REAL rIm)
{
    /*
        Hilbert filter for I / Q input data. This code is based on code written
        by Cesco (HB9TLK)
    */

    /* Move old data */
    for (int i = 0; i < NUM_TAPS_IQ_INPUT_FILT - 1; i++)
    {
        vecrReHist[i] = vecrReHist[i + 1];
        vecrImHist[i] = vecrImHist[i + 1];
    }

    vecrReHist[NUM_TAPS_IQ_INPUT_FILT - 1] = rRe;
    vecrImHist[NUM_TAPS_IQ_INPUT_FILT - 1] = rIm;

    /* Filter */
    _REAL rSum = 0.0;
    for (unsigned i = 1; i < NUM_TAPS_IQ_INPUT_FILT; i += 2)
        rSum += _REAL(fHilFiltIQ[i]) * vecrImHist[int(i)];

    return (rSum + vecrReHist[IQ_INP_HIL_FILT_DELAY]) / 2;
}

void Loopback::InitInternal(CParameter& Parameters)
{

    /* Init sound interface. Set it to one symbol. The sound card interface
           has to taken care about the buffering data of a whole MSC block.
           Use stereo input (* 2) */

    //#ifdef QT_MULTIMEDIA_LIB
    // 把Qt 声音 去除;
    //#endif

    Parameters.Lock();
    /* We define iOutputBlockSize as half the iSymbolBlockSize because
       if a positive frequency offset is present in drm signal,
       after some time a buffer overflow occur in the output buffer of
       InputResample.ProcessData()
            我们将iOutputBlockSize定义为iSymbolBlockSize的一半，因为
        如果DRM信号中存在正频偏，的输出缓冲区发生缓冲区溢出 */

    /* Define output block-sizeiOutputBlockSize  定义为 iSymbolBlockSize 的一半，
     *  以避免在存在频偏时输出缓冲区发生溢出。  */
    iOutputBlockSize = Parameters.CellMappingTable.iSymbolBlockSize / 2;  //
    iMaxOutputBlockSize = iOutputBlockSize * 2;

    /* Get signal sample rate  从参数中获取信号采样率、上采样和下采样的比率。1 1 */
    iSampleRate = Parameters.GetSigSampleRate();
    iUpscaleRatio = Parameters.GetSigUpscaleRatio();  //  比值
    iDownscaleRatio = Parameters.GetSigDownscaleRatio();
    Parameters.Unlock();

    // 因为4在二进制中是100，所以它的倍数在二进制表示中，从最低位开始数起的两位总是00
    //检查输出块的对齐情况。如果 iOutputBlockSize 不是 4 的倍数（即没有对齐），则输出一个警告信息
    const int iOutputBlockAlignment = iOutputBlockSize & 3;
    if (iOutputBlockAlignment) {
        fprintf(stderr, "CReceiveData::InitInternal(): iOutputBlockAlignment = %d\n",
                iOutputBlockAlignment);
    }

    try
    {

        bool bChanged = false;
        int wantedBufferSize = iOutputBlockSize * 2 *
                iDownscaleRatio / iUpscaleRatio; // samples

        // 初始化声音设备或音频输入
        /*
        这里根据 QT_MULTIMEDIA_LIB 的定义选择初始化声音设备或音频输入设备。
        通过调整采样率（上采样或下采样）和所需缓冲区大小来初始化声音设备（pSound）或音频输入（pAudioInput）。
        如果 pAudioInput 存在，设置缓冲区大小并启动音频输入设备，同时处理启动失败的情况。
        bChanged 表示声音设备的初始化状态，如果发生了变化，则重新初始化。
        **/

        /* 清理输入数据： Clear input data buffer on change samplerate change */
        if (bChanged)
            ClearInputData();

        /* 滤波器的初始化（上采样或下采样）：Init 2X upscaler if enabled

        根据 iUpscaleRatio 或 iDownscaleRatio 初始化上采样或下采样滤波器。
        滤波器的 taps 被定义为接近 4 字节对齐的值，确保滤波器的系数被正确加载。
        如果采样率或设备发生变化（bChanged），重置滤波器状态。
        */
        if (iUpscaleRatio > 1)
        {
            const int taps = (NUM_TAPS_UPSAMPLE_FILT + 3) & ~3;
            vecf_B.resize(taps, 0.0f);
            for (unsigned i = 0; i < NUM_TAPS_UPSAMPLE_FILT; i++)
                vecf_B[i] = float(dUpsampleFilt[i] * iUpscaleRatio);
            if (bChanged)
            {
                vecf_ZL.resize(0);
                vecf_ZR.resize(0);
            }
            vecf_ZL.resize(unsigned(iOutputBlockSize + taps) / 2, 0.0f);
            vecf_ZR.resize(unsigned(iOutputBlockSize + taps) / 2, 0.0f);
            vecf_YL.resize(unsigned(iOutputBlockSize));
            vecf_YR.resize(unsigned(iOutputBlockSize));
        }
        else if (iDownscaleRatio > 1)
        {
            const int taps = (NUM_TAPS_DOWNSAMPLE_FILT + 3) & ~3;
            vecf_B.resize(taps, 0.0f);
            for (unsigned i = 0; i < NUM_TAPS_DOWNSAMPLE_FILT; i++)
                vecf_B[i] = float(dDownsampleFilt[i] / iDownscaleRatio);
            if (bChanged)
            {
                vecf_ZL.resize(0);
                vecf_ZR.resize(0);
            }
            vecf_ZL.resize(unsigned(iOutputBlockSize * 2 + taps), 0.0f);
            vecf_ZR.resize(unsigned(iOutputBlockSize *2 + taps), 0.0f);
            vecf_YL.resize(unsigned(iOutputBlockSize));
            vecf_YR.resize(unsigned(iOutputBlockSize));
        }
        else
        {
            vecf_B.resize(0);
            vecf_YL.resize(0);
            vecf_YR.resize(0);
            vecf_ZL.resize(0);
            vecf_ZR.resize(0);
        }

        /* Init buffer size for taking stereo input
         * 初始化音频缓冲区，分配的大小是根据输出块大小和采样比率调整后的值 */
        // 目前:iOutputBlockSize * 2 = 2,560;
        // iDownscaleRatio / iUpscaleRatio = 1
        // cout<<"iDownscaleRatio / iUpscaleRatio = "<<iDownscaleRatio / iUpscaleRatio<<endl;
        vecsSoundBuffer.Init(iOutputBlockSize * 2 *
                             iDownscaleRatio / iUpscaleRatio);

        /* Init signal meter */
        SignalLevelMeter.Init(0);

        /* Inits for I / Q input, only if it is not already
           to keep the history intact I/Q 输入的初始化
        初始化用于 I/Q 输入信号的历史数据缓冲区（实部和虚部），
        只在尺寸不匹配或发生设备变化时重新初始化  *******/
        if (vecrReHist.Size() != NUM_TAPS_IQ_INPUT_FILT || bChanged)
        {
            vecrReHist.Init(NUM_TAPS_IQ_INPUT_FILT, 0.0);
            vecrImHist.Init(NUM_TAPS_IQ_INPUT_FILT, 0.0);
        }

        /* Start with phase null (can be arbitrarily chosen) */
        cCurExp = 1.0;

        /* Set rotation vector to mix signal from zero frequency to virtual
           intermediate frequency */
        const _REAL rNormCurFreqOffsetIQ = 2.0 * crPi * VIRTUAL_INTERMED_FREQ
                                                            / _REAL(iSampleRate);

        cout<<"iSampleRate :"<<iSampleRate<<endl; //48000
        //rNormCurFreqOffsetIQ = 0.78539816339744828  pei/4
        cout<<"loop rx: rNormCurFreqOffsetIQ"<<rNormCurFreqOffsetIQ <<endl;
        cExpStep = _COMPLEX(cos(rNormCurFreqOffsetIQ), sin(rNormCurFreqOffsetIQ));

        //  0.707107; 0.707107
        cout<<" "<<cos(rNormCurFreqOffsetIQ)<<"; "<< sin(rNormCurFreqOffsetIQ)<<endl;

        /* 符号计数器的初始化：  OPH: init free-running symbol counter*/
        iFreeSymbolCounter = 0;

    }
    catch (CGenErr GenErr)
    {

    }
    catch (string strError)
    {

    }
    /**
    该函数的核心任务是：

    初始化或重新初始化音频设备（或文件）输入。
    设定上采样、下采样的滤波器参数。
    根据系统参数（如采样率和符号块大小）调整缓冲区和相关处理逻辑。
    清理输入数据并准备好接收和处理新的数据。**/

}


//读取数据给 &vecsSoundBuffer[0]  type:short
void Loopback::ProcessDataInternal(CParameter& Parameters)
{

    int i;
    /* OPH: update free-running symbol counter */
    Parameters.Lock();
    iFreeSymbolCounter++;
    if (iFreeSymbolCounter >= Parameters.CellMappingTable.iNumSymPerFrame * 2)
        /* x2 because iOutputBlockSize=iSymbolBlockSize/2 */
    {
        iFreeSymbolCounter = 0;
        /* calculate the PSD once per frame for the RSI output */
        if (Parameters.bMeasurePSD) {
            this->emitRSCIData(Parameters);
        }
    }
    Parameters.Unlock();

    bool bBad = true;
    /**Read data:  现象目前size = 1,280 ***/
    int size = vecsSoundBuffer.Size() ;  //
//    cout<<"SoundBuffer size = "<<size <<endl; // 1,280
    // char *p = reinterpret_cast<char*>(&vecsSoundBuffer[0]);
    short* p = &vecsSoundBuffer[0];


    //cout<<"接收机ringbuff:size = "<< size <<endl;  //n = 1280
    //cout<<"pRingbuff usr size = "<<pRingbuff->Getsize()<<endl;
    //
    do {
        int r = pRingbuff->ring_buffer_read(p, size);

        if(r > 0) {
            p += r;
            size -= r;
        }
        else {

            QThread::msleep(100);
            // int RingbuffSize = pRingbuff->Getsize();
            // 5:接收机 read sleep ; 1280, r = -1
            //cout <<":接收机 read sleep ; " <<RingbuffSize <<", r = " << r <<endl;  // n = 2560
            // return;
        }
    } while (size > 0);


    // output size = 640; iUpscaleRatio = 1;iDownscaleRatio = 1
    //    cout<< "; output size = "<<iOutputBlockSize
    //        << "; iUpscaleRatio = "<<iUpscaleRatio
    //        << "; iDownscaleRatio = "<<iDownscaleRatio <<endl;
    // cout<< "接收机 read OK " << endl;  // size = 2560
    /*****/
    // InterfaceI 指示灯
//    Parameters.Lock();
//    Parameters.ReceiveStatus.InterfaceI.
//            SetStatus(bBad ? CRC_ERROR : RX_OK); /* Red light */
//    Parameters.Unlock();

//    if(bBad)
//        return;

    /* Get data from sound interface. The read function must be a
       blocking function! */

    /* Upscale if ratio greater than one */
    if (iUpscaleRatio > 1)
    {
        /* The actual upscaling, currently only 2X is supported */
        InterpFIR_2X(2, &vecsSoundBuffer[0], vecf_ZL, vecf_YL, vecf_B);
        InterpFIR_2X(2, &vecsSoundBuffer[1], vecf_ZR, vecf_YR, vecf_B);
    }
    else if (iDownscaleRatio > 1)
    {
        /* The actual downscaling, currently only 2X is supported */
        DecimFIR_2X(2, &vecsSoundBuffer[0], vecf_ZL, vecf_YL, vecf_B);
        DecimFIR_2X(2, &vecsSoundBuffer[1], vecf_ZR, vecf_YR, vecf_B);
    }

    if (iUpscaleRatio > 1 || iDownscaleRatio >1)
    {

        /* Write data to output buffer. Do not set the switch command inside
           the for-loop for efficiency reasons */
        switch (eInChanSelection)
        {
        case CS_LEFT_CHAN:
            for (i = 0; i < iOutputBlockSize; i++)
                (*pvecOutputData)[i] = _REAL(vecf_YL[unsigned(i)]);
            break;

        case CS_RIGHT_CHAN:
            for (i = 0; i < iOutputBlockSize; i++)
                (*pvecOutputData)[i] = _REAL(vecf_YR[unsigned(i)]);
            break;

        case CS_MIX_CHAN:
            for (i = 0; i < iOutputBlockSize; i++)
            {
                /* Mix left and right channel together */
                (*pvecOutputData)[i] = _REAL(vecf_YL[unsigned(i)] + vecf_YR[unsigned(i)]) / 2.0;
            }
            break;

        case CS_SUB_CHAN:
            for (i = 0; i < iOutputBlockSize; i++)
            {
                /* Subtract right channel from left */
                (*pvecOutputData)[i] = _REAL(vecf_YL[unsigned(i)] - vecf_YR[unsigned(i)]) / 2.0;
            }
            break;

            /* I / Q input */
        case CS_IQ_POS:

            for (i = 0; i < iOutputBlockSize; i++)
            {
                (*pvecOutputData)[i] = HilbertFilt(_REAL(vecf_YL[unsigned(i)]), _REAL(vecf_YR[unsigned(i)]));
            }
            break;

        case CS_IQ_NEG:
            for (i = 0; i < iOutputBlockSize; i++)
            {
                (*pvecOutputData)[i] = HilbertFilt(_REAL(vecf_YR[unsigned(i)]), _REAL(vecf_YL[unsigned(i)]));
            }
            break;

        case CS_IQ_POS_ZERO:
            for (i = 0; i < iOutputBlockSize; i++)
            {
                /* Shift signal to vitual intermediate frequency before applying
                   the Hilbert filtering */
                _COMPLEX cCurSig = _COMPLEX(_REAL(vecf_YL[unsigned(i)]), _REAL(vecf_YR[unsigned(i)]));

                cCurSig *= cCurExp;

                /* Rotate exp-pointer on step further by complex multiplication
                   with precalculated rotation vector cExpStep */
                cCurExp *= cExpStep;

                (*pvecOutputData)[i] =
                        HilbertFilt(cCurSig.real(), cCurSig.imag());
            }
            break;

        case CS_IQ_NEG_ZERO:
            for (i = 0; i < iOutputBlockSize; i++)
            {
                /* Shift signal to vitual intermediate frequency before applying
                   the Hilbert filtering */
                _COMPLEX cCurSig = _COMPLEX(_REAL(vecf_YR[unsigned(i)]), _REAL(vecf_YL[unsigned(i)]));

                cCurSig *= cCurExp;

                /* Rotate exp-pointer on step further by complex multiplication
                   with precalculated rotation vector cExpStep */
                cCurExp *= cExpStep;

                (*pvecOutputData)[i] =
                        HilbertFilt(cCurSig.real(), cCurSig.imag());
            }
            break;

        case CS_IQ_POS_SPLIT:
            for (i = 0; i < iOutputBlockSize; i += 4)
            {
                (*pvecOutputData)[i + 0] =  _REAL(vecf_YL[unsigned(i + 0)]);
                (*pvecOutputData)[i + 1] = _REAL(-vecf_YR[unsigned(i + 1)]);
                (*pvecOutputData)[i + 2] = _REAL(-vecf_YL[unsigned(i + 2)]);
                (*pvecOutputData)[i + 3] =  _REAL(vecf_YR[unsigned(i + 3)]);
            }
            break;

        case CS_IQ_NEG_SPLIT:
            for (i = 0; i < iOutputBlockSize; i += 4)
            {
                (*pvecOutputData)[i + 0] =  _REAL(vecf_YR[unsigned(i + 0)]);
                (*pvecOutputData)[i + 1] = _REAL(-vecf_YL[unsigned(i + 1)]);
                (*pvecOutputData)[i + 2] = _REAL(-vecf_YR[unsigned(i + 2)]);
                (*pvecOutputData)[i + 3] =  _REAL(vecf_YL[unsigned(i + 3)]);
            }
            break;
        }
    }
    /* Upscale ratio equal to one */
    else
    {
        /* Write data to output buffer. Do not set the switch command inside
           the for-loop for efficiency reasons */

        eInChanSelection = CS_IQ_POS_ZERO;

        switch (eInChanSelection)
        {
        case CS_LEFT_CHAN:
            for (i = 0; i < iOutputBlockSize; i++)
                (*pvecOutputData)[i] = sample2real(vecsSoundBuffer[2 * i]);
            break;

        case CS_RIGHT_CHAN:
            for (i = 0; i < iOutputBlockSize; i++)
                (*pvecOutputData)[i] = sample2real(vecsSoundBuffer[2 * i + 1]);
            break;

        case CS_MIX_CHAN:
            for (i = 0; i < iOutputBlockSize; i++)
            {
                /* Mix left and right channel together */
                const _REAL rLeftChan = sample2real(vecsSoundBuffer[2 * i]);
                const _REAL rRightChan = sample2real(vecsSoundBuffer[2 * i + 1]);
                (*pvecOutputData)[i] = (rLeftChan + rRightChan) / 2;
            }
            break;

        case CS_SUB_CHAN:
            for (i = 0; i < iOutputBlockSize; i++)
            {
                /* Subtract right channel from left */
                const _REAL rLeftChan = sample2real(vecsSoundBuffer[2 * i]);
                const _REAL rRightChan = sample2real(vecsSoundBuffer[2 * i + 1]);
                (*pvecOutputData)[i] = (rLeftChan - rRightChan) / 2;
            }
            break;

            /* I / Q input */
        case CS_IQ_POS:
            //iOutputBlockSize: 640
            cout<<"---------------\n read I / Q input:CS_IQ_POS ,iOutputBlockSize: "<<iOutputBlockSize << endl;

            for (i = 0; i < iOutputBlockSize; i++)
            {
                (*pvecOutputData)[i] =
                        HilbertFilt(sample2real(vecsSoundBuffer[2 * i]),
                        sample2real(vecsSoundBuffer[2 * i + 1]));
            }
            break;

        case CS_IQ_NEG:
            for (i = 0; i < iOutputBlockSize; i++)
            {
                (*pvecOutputData)[i] =
                        HilbertFilt(sample2real(vecsSoundBuffer[2 * i + 1]),
                        sample2real(vecsSoundBuffer[2 * i]));
            }
            break;

        case CS_IQ_POS_ZERO:
            cout<<"rx: CS_IQ_POS_ZERO---"<<eInChanSelection<< endl;
            for (i = 0; i < iOutputBlockSize; i++)
            {
                /* Shift signal to vitual intermediate frequency before applying
                   the Hilbert filtering */
                _COMPLEX cCurSig = _COMPLEX(sample2real(vecsSoundBuffer[2 * i]),
                        sample2real(vecsSoundBuffer[2 * i + 1]));

                cCurSig *= cCurExp;

                /* Rotate exp-pointer on step further by complex multiplication
                   with precalculated rotation vector cExpStep */
                cCurExp *= cExpStep;

                (*pvecOutputData)[i] =
                        HilbertFilt(cCurSig.real(), cCurSig.imag());
            }
            break;

        case CS_IQ_NEG_ZERO:
            cout<<"rx: CS_IQ_NEG_ZERO---"<<CS_IQ_NEG_ZERO<< endl;
            for (i = 0; i < iOutputBlockSize; i++)
            {
                /* Shift signal to vitual intermediate frequency before applying
                   the Hilbert filtering */
                _COMPLEX cCurSig = _COMPLEX(sample2real(vecsSoundBuffer[2 * i + 1]),
                        sample2real(vecsSoundBuffer[2 * i]));

                cCurSig *= cCurExp;

                /* Rotate exp-pointer on step further by complex multiplication
                   with precalculated rotation vector cExpStep */
                cCurExp *= cExpStep;

                (*pvecOutputData)[i] =
                        HilbertFilt(cCurSig.real(), cCurSig.imag());
            }
            break;

        case CS_IQ_POS_SPLIT: /* Require twice the bandwidth */
            for (i = 0; i < iOutputBlockSize; i++)
            {
                iPhase = (iPhase + 1) & 3;
                _REAL rValue = vecsSoundBuffer[2 * i]     * /*COS*/SineTable[iPhase + 1] -
                        vecsSoundBuffer[2 * i + 1] * /*SIN*/SineTable[iPhase];
                (*pvecOutputData)[i] = rValue;
            }
            break;

        case CS_IQ_NEG_SPLIT: /* Require twice the bandwidth */
            for (i = 0; i < iOutputBlockSize; i++)
            {
                iPhase = (iPhase + 1) & 3;
                _REAL rValue = vecsSoundBuffer[2 * i + 1] * /*COS*/SineTable[iPhase + 1] -
                        vecsSoundBuffer[2 * i]     * /*SIN*/SineTable[iPhase];
                (*pvecOutputData)[i] = rValue;
            }
            break;
        }
    }

    /* Flip spectrum if necessary ------------------------------------------- */
    if (bFippedSpectrum)
    {
        /* Since iOutputBlockSize is always even we can do some opt. here */
        for (i = 0; i < iOutputBlockSize; i+=2)
        {
            /* We flip the spectrum by using the mirror spectrum at the negative
               frequencys. If we shift by half of the sample frequency, we can
               do the shift without the need of a Hilbert transformation */
            (*pvecOutputData)[i] = -(*pvecOutputData)[i];
        }
    }

    /* Copy data in buffer for spectrum calculation 在缓冲区中复制数据以进行频谱计算 */
    mutexInpData.Lock();
    vecrInpData.AddEnd((*pvecOutputData), iOutputBlockSize);
    mutexInpData.Unlock();

    /* Update level meter */
    SignalLevelMeter.Update((*pvecOutputData));
    Parameters.Lock();
    Parameters.SetIFSignalLevel(SignalLevelMeter.Level());
    Parameters.Unlock();
}

void Loopback::InterpFIR_2X(const int channels, _SAMPLE *X, std::vector<float> &Z, std::vector<float> &Y, std::vector<float> &B)
{
    /*
        2X interpolating filter. When combined with CS_IQ_POS_SPLIT or CS_IQ_NEG_SPLIT
        input data mode, convert I/Q input to full bandwidth, code by David Flamand
    */
    int i, j;
    const int B_len = int(B.size());
    const int Z_len = int(Z.size());
    const int Y_len = int(Y.size());
    const int Y_len_2 = Y_len / 2;
    float *B_beg_ptr = &B[0];
    float *Z_beg_ptr = &Z[0];
    float *Y_ptr = &Y[0];
    float *B_end_ptr, *B_ptr, *Z_ptr;
    float y0, y1, y2, y3;

    /* Check for size and alignment requirement */
    if ((B_len & 3) || (Z_len != (B_len/2 + Y_len_2)) || (Y_len & 1))
        return;

    /* Copy the old history at the end */
    for (i = B_len/2-1; i >= 0; i--)
        Z_beg_ptr[Y_len_2 + i] = Z_beg_ptr[i];

    /* Copy the new sample at the beginning of the history */
    for (i = 0, j = 0; i < Y_len_2; i++, j+=channels)
        Z_beg_ptr[Y_len_2 - i - 1] = X[j];

    /* The actual lowpass filtering using FIR */
    for (i = Y_len_2-1; i >= 0; i--)
    {
        B_end_ptr  = B_beg_ptr + B_len;
        B_ptr      = B_beg_ptr;
        Z_ptr      = Z_beg_ptr + i;
        y0 = y1 = y2 = y3 = 0.0f;
        while (B_ptr != B_end_ptr)
        {
            y0 = y0 + B_ptr[0] * Z_ptr[0];
            y1 = y1 + B_ptr[1] * Z_ptr[0];
            y2 = y2 + B_ptr[2] * Z_ptr[1];
            y3 = y3 + B_ptr[3] * Z_ptr[1];
            B_ptr += 4;
            Z_ptr += 2;
        }
        *Y_ptr++ = y0 + y2;
        *Y_ptr++ = y1 + y3;
    }
}

void Loopback::DecimFIR_2X(const int channels, _SAMPLE *X, std::vector<float> &Z, std::vector<float> &Y, std::vector<float> &B)
{
    /*
        2X decimating filter.
    */
    int i, j;
    const int B_len = int(B.size());
    const int Z_len = int(Z.size());
    const int Y_len = int(Y.size());
    const int Y_len_2 = Y_len * 2;
    float *B_beg_ptr = &B[0];
    float *Z_beg_ptr = &Z[0];
    float *Y_ptr = &Y[0];
    float *B_end_ptr, *B_ptr, *Z_ptr;
    float y0, y1, y2, y3;

    /* Check for size and alignment requirement */
    if ((B_len & 3) || Z_len != (B_len + Y_len_2))
        return;

    /* Copy the old history at the end */
    for (i = B_len-1; i >= 0; i--)
        Z_beg_ptr[Y_len_2 + i] = Z_beg_ptr[i];

    /* Copy the new sample at the beginning of the history */
    for (i = 0, j = 0; i < Y_len_2; i++, j+=channels)
        Z_beg_ptr[Y_len_2 - i - 1] = X[j];

    /* The actual lowpass filtering using FIR */
    for (i = Y_len_2-2; i >= 0; i-=2)
    {
        B_end_ptr  = B_beg_ptr + B_len;
        B_ptr      = B_beg_ptr;
        Z_ptr      = Z_beg_ptr + i;
        y0 = y1 = y2 = y3 = 0.0f;
        while (B_ptr != B_end_ptr)
        {
            y0 = y0 + B_ptr[0] * Z_ptr[0];
            y1 = y1 + B_ptr[1] * Z_ptr[1];
            y2 = y2 + B_ptr[2] * Z_ptr[2];
            y3 = y3 + B_ptr[3] * Z_ptr[3];

            B_ptr += 4;
            Z_ptr += 4;
        }
        *Y_ptr++ = y0 + y1 + y2 + y3;
    }
}

void Loopback::emitRSCIData(CParameter &Parameters)
{
    /* Init the constants for scale and normalization */
    spectrumAnalyser.setNegativeFrequency(eInChanSelection == CS_IQ_POS_SPLIT || eInChanSelection == CS_IQ_NEG_SPLIT);
    spectrumAnalyser.setOffsetFrequency((eInChanSelection == CS_IQ_POS_ZERO) || (eInChanSelection == CS_IQ_NEG_ZERO));
    mutexInpData.Lock();
    spectrumAnalyser.CalculateLinearPSD(vecrInpData, LEN_PSD_AV_EACH_BLOCK_RSI, NUM_AV_BLOCKS_PSD_RSI, PSD_OVERLAP_RSI);
    mutexInpData.Unlock();


    const _REAL rNormData =  pow(_REAL(_MAXSHORT) * _REAL(LEN_PSD_AV_EACH_BLOCK_RSI), 2) * _REAL(NUM_AV_BLOCKS_PSD_RSI) * PSDWindowGain;

    CVector<_REAL>		vecrData;
    CVector<_REAL>		vecrScale;
    spectrumAnalyser.PSD2LogPSD(rNormData, vecrData, vecrScale);

    /* Data required for rpsd tag */
    /* extract the values from -8kHz to +8kHz/18kHz relative to 12kHz, i.e. 4kHz to 20kHz */
    /*const int startBin = 4000.0 * LEN_PSD_AV_EACH_BLOCK_RSI /iSampleRate;
    const int endBin = 20000.0 * LEN_PSD_AV_EACH_BLOCK_RSI /iSampleRate;*/
    /* The above calculation doesn't round in the way FhG expect. Probably better to specify directly */

    /* For 20k mode, we need -8/+18, which is more than the Nyquist rate of 24kHz. */
    /* Assume nominal freq = 7kHz (i.e. 2k to 22k) and pad with zeroes (roughly 1kHz each side) */

    int iStartBin = 22;
    int iEndBin = 106;
    int iVecSize = iEndBin - iStartBin + 1; //85

    //_REAL rIFCentreFrequency = Parameters.FrontEndParameters.rIFCentreFreq;

    ESpecOcc eSpecOcc = Parameters.GetSpectrumOccup();
    if (eSpecOcc == SO_4 || eSpecOcc == SO_5)
    {
        iStartBin = 0;
        iEndBin = 127;
        iVecSize = 139;
    }
    /* Line up the the middle of the vector with the quarter-Nyquist bin of FFT */
    int iStartIndex = iStartBin - (LEN_PSD_AV_EACH_BLOCK_RSI/4) + (iVecSize-1)/2;

    /* Fill with zeros to start with */
    Parameters.vecrPSD.Init(iVecSize, 0.0);

    for (int i=iStartIndex, j=iStartBin; j<=iEndBin; i++,j++)
        Parameters.vecrPSD[i] = vecrData[j];

    spectrumAnalyser.CalculateSigStrengthCorrection(Parameters, vecrData);

    spectrumAnalyser.CalculatePSDInterferenceTag(Parameters, vecrData);
}
