/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik & BBC
 * Copyright (c) 2001-2019
 *
 * Author(s):
 * Volker Fischer, Julian Cable, Mark J Fine
 *
 * Description:
 *
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

#include "DataIO.h"
#include <iomanip>
#include <time.h>
#ifdef QT_MULTIMEDIA_LIB
# include <QAudioOutput>
# include <QAudioInput>
# include <QSet>
#endif
//#include "sound/sound.h"
#include "sound/soundinterfacefactory.h"

/* Implementation *************************************************************/
/******************************************************************************\
* MSC data                                                                    *
\******************************************************************************/
/* Transmitter -------------------------------------------------------------- */
void CReadData::ProcessDataInternal(CParameter& Parameters)
{
    /* Get data from sound interface */
    if (pSound == nullptr)
    {
#ifdef QT_MULTIMEDIA_LIB
        if(pIODevice)
        {
            qint64 n = 2 * vecsSoundBuffer.Size();
            cout<<"n:"<<n <<endl;
            qint64 bytesRead = pIODevice->read(
                        reinterpret_cast<char*>(&vecsSoundBuffer[0]), n);
            if (bytesRead > 0)
            {
                //bytesRead =  15360
                cout<<"处理读取到的音频数据:"<<bytesRead <<endl; // 处理读取到的音频数据
            }
            else
            {
                cout<<"没有读取到数据，可能是静音"<<endl; // 没有读取到数据，可能是静音
            }
        }
        else
        {
            cout<<"pIODevice == null ;return"<<endl; // 处理读取到的音频数据
            return;
        }

#else
        return;
#endif
    }
    else
    {
        pSound->Read(vecsSoundBuffer, Parameters);
    }
    /* Write data to output buffer */
    for (int i = 0; i < iOutputBlockSize; i++)
        (*pvecOutputData)[i] = vecsSoundBuffer[i];

    /* Update level meter */
    SignalLevelMeter.Update((*pvecOutputData));
}

void CReadData::InitInternal(CParameter& Parameters)
{
    /* Get audio sample rate */
    iSampleRate = Parameters.GetAudSampleRate();

    /* Define block-size for output, an audio frame always corresponds
       to 400 ms. We use always stereo blocks */
    iOutputBlockSize = (int) ((_REAL) iSampleRate *
                              (_REAL) 0.4 /* 400 ms */ * 2 /* stereo */);
    //iOutputBlockSize 48000 * 0.4 * 2 =  38400
    DEBUG_COUT("iOutputBlockSize = " << iOutputBlockSize);
    /* Init sound interface and intermediate buffer */
    if(pSound) pSound->Init(iSampleRate, iOutputBlockSize, false);
    vecsSoundBuffer.Init(iOutputBlockSize);

    /* Init level meter */
    SignalLevelMeter.Init(0);
}

void CReadData::Stop()
{
#ifdef QT_MULTIMEDIA_LIB
    if(pIODevice!=nullptr) pIODevice->close();
#endif
    if(pSound!=nullptr) pSound->Close();
}

void CReadData::Enumerate(std::vector<std::string>& names,
                          std::vector<std::string>& descriptions,
                          string& defaultInput)
{
#ifdef QT_MULTIMEDIA_LIB
    // 发射机的 输入端
    QString def = QAudioDeviceInfo::defaultInputDevice().deviceName();
    defaultInput = def.toStdString();

    DEBUG_COUT("defaultInput = "<<QString::fromStdString( defaultInput));

    // 输入设备
    QSet<QString> s;
    foreach(const QAudioDeviceInfo& di,
            QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
    {
        s.insert(di.deviceName());
    }

    names.clear();
    descriptions.clear();
    // 从遍历到的 set容器中 string --到内部内存
    int  i = 0;
    foreach(const QString n, s)
    {
        i++;
        names.push_back(n.toStdString());
        descriptions.push_back(""); // 描述
    }
#else
    if(pSound==nullptr)
        pSound = CSoundInterfaceFactory::CreateSoundInInterface();
    pSound->Enumerate(names, descriptions, defaultInput);
#endif
    //  cout << "default input is " << defaultInput << endl;

    DEBUG_COUT("default input is= " <<
               QString::fromStdString( defaultInput));

}

void CReadData::SetSoundInterface(string device)
{
    soundDevice = device;

    //设备来自 文件

    // qDebug()<< "SetSoundInterface: Input device = ["<< QString::fromStdString(device) << "]"<<endl;

#ifdef QT_MULTIMEDIA_LIB
    QAudioFormat format;
    if(iSampleRate==0) iSampleRate = 48000; // TODO get order of initialisation correct
    format.setSampleRate(iSampleRate);
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setChannelCount(2); // TODO
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setCodec("audio/pcm");

    // 获取可用音频输入设备列表
    QList<QAudioDeviceInfo> audioInputs =
            QAudioDeviceInfo::availableDevices(QAudio::AudioInput);

    // 打印设备数量- 11
    //DEBUG_COUT("可用音频输入设备的数量:" << audioInputs.size());

    //用于获取当前系统上可用的音频输入设备列表
    foreach(const QAudioDeviceInfo& di,
            QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
    {
        // 检查当前设备的名称是否与目标设备名称匹配
        //qDebug()<<"input设备的名称:"<< di.deviceName() <<endl;
        if(device == di.deviceName().toStdString())
        {
            // 获取最接近的音频格式:
            QAudioFormat nearestFormat = di.nearestFormat(format);

            // 创建并启动音频输入:
            QAudioInput* pAudioInput = new QAudioInput(di, nearestFormat);
            pIODevice = pAudioInput->start();
            if(pAudioInput->error()==QAudio::NoError)
            {
                if(pIODevice->open(QIODevice::ReadOnly))
                {
                    qDebug("audio input open");
                }
                else {
                    qDebug("else audio input open");
                }
            }
            else
            {
                qDebug("Can't open audio input");
            }
        }
        break;
    }

#else
    if(pSound != nullptr) {
        delete pSound;
        pSound = nullptr;
    }
    pSound = CSoundInterfaceFactory::CreateSoundInInterface();
    pSound->SetDev(device);
#endif

}


/* Receiver ----------------------------------------------------------------- */
CWriteData::CWriteData() :
    #ifdef QT_MULTIMEDIA_LIB
    pAudioOutput(nullptr), pIODevice(nullptr),
    #endif
    pSound(nullptr), /* Sound interface */
    bMuteAudio(false), bDoWriteWaveFile(false),
    bSoundBlocking(false), bNewSoundBlocking(false),
    eOutChanSel(CS_BOTH_BOTH), rMixNormConst(MIX_OUT_CHAN_NORM_CONST),
    iAudSampleRate(0), iNumSmpls4AudioSprectrum(0), iNumBlocksAvAudioSpec(0),
    iMaxAudioFrequency(MAX_SPEC_AUDIO_FREQUENCY)
{
    /* Constructor */
}

void CWriteData::Stop()
{
#ifdef QT_MULTIMEDIA_LIB
    if(pAudioOutput!=nullptr) pIODevice->close();
#endif
    if(pSound!=nullptr) pSound->Close();
}

void CWriteData::Enumerate(std::vector<std::string>& names, std::vector<std::string>& descriptions, string& defaultOutput)
{
#ifdef QT_MULTIMEDIA_LIB
    QString def = QAudioDeviceInfo::defaultOutputDevice().deviceName();
    defaultOutput = def.toStdString();
    QSet<QString> s;
    foreach(const QAudioDeviceInfo& di, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
    {
        s.insert(di.deviceName());
    }
    names.clear();
    descriptions.clear();
    foreach(const QString n, s) {
        names.push_back(n.toStdString());
        descriptions.push_back("");
    }
#else
    if(pSound==nullptr) pSound = CSoundInterfaceFactory::CreateSoundOutInterface();
    pSound->Enumerate(names, descriptions, defaultOutput);
#endif
}

void CWriteData::SetSoundInterface(string device)
{
    // 接收机
    soundDevice = device;

#ifdef QT_MULTIMEDIA_LIB
    QAudioFormat format;
    if(iAudSampleRate==0) iAudSampleRate = 48000; // TODO get order of initialisation correct
    format.setSampleRate(iAudSampleRate);
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setChannelCount(2); // TODO
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setCodec("audio/pcm");

    if(pAudioOutput != nullptr && pAudioOutput->state() == QAudio::ActiveState)
    {
        pAudioOutput->stop();
        std::cout<<"pAudioOutput stop !!! "<< pAudioOutput << std::endl;
        delete pAudioOutput;
        pAudioOutput = nullptr;
    }
    // 遍历可用的音频输出设备
    foreach(const QAudioDeviceInfo& di,
            QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
    {
        if(device == di.deviceName().toStdString())
        {
            QAudioFormat nearestFormat = di.nearestFormat(format);
            pAudioOutput = new QAudioOutput(di, nearestFormat);
            break;
        }
    }

    if(pAudioOutput == nullptr) {
        // OUT put
        qDebug("Can't find audio output %s", device.c_str());
    }
#else
    if(pSound != nullptr) {
        delete pSound;
        pSound = nullptr;
    }
    pSound = CSoundInterfaceFactory::CreateSoundOutInterface();
    pSound->SetDev(device);
#endif

}

void CWriteData::ProcessDataInternal(CParameter& Parameters)
{
    int i;

    /* Calculate size of each individual audio channel */
    const int iHalfBlSi = iInputBlockSize / 2;

    switch (eOutChanSel)
    {
    case CS_BOTH_BOTH:
        /* left -> left, right -> right (vector sizes might not be the
           same -> use for-loop for copying) */
        for (i = 0; i < iInputBlockSize; i++)
            vecsTmpAudData[i] = (*pvecInputData)[i]; /* Just copy data */
        break;

    case CS_LEFT_LEFT:
        /* left -> left, right muted */
        for (i = 0; i < iHalfBlSi; i++)
        {
            vecsTmpAudData[2 * i] = (*pvecInputData)[2 * i];
            vecsTmpAudData[2 * i + 1] = 0; /* mute */
        }
        break;

    case CS_RIGHT_RIGHT:
        /* left muted, right -> right */
        for (i = 0; i < iHalfBlSi; i++)
        {
            vecsTmpAudData[2 * i] = 0; /* mute */
            vecsTmpAudData[2 * i + 1] = (*pvecInputData)[2 * i + 1];
        }
        break;

    case CS_LEFT_MIX:
        /* left -> mix, right muted */
        for (i = 0; i < iHalfBlSi; i++)
        {
            /* Mix left and right channel together. Prevent overflow! First,
               copy recorded data from "short" in "_REAL" type variables */
            const _REAL rLeftChan = (*pvecInputData)[2 * i];
            const _REAL rRightChan = (*pvecInputData)[2 * i + 1];

            vecsTmpAudData[2 * i] =
                    Real2Sample((rLeftChan + rRightChan) * rMixNormConst);

            vecsTmpAudData[2 * i + 1] = 0; /* mute */
        }
        break;

    case CS_RIGHT_MIX:
        /* left muted, right -> mix */
        for (i = 0; i < iHalfBlSi; i++)
        {
            /* Mix left and right channel together. Prevent overflow! First,
               copy recorded data from "short" in "_REAL" type variables */
            const _REAL rLeftChan = (*pvecInputData)[2 * i];
            const _REAL rRightChan = (*pvecInputData)[2 * i + 1];

            vecsTmpAudData[2 * i] = 0; /* mute */
            vecsTmpAudData[2 * i + 1] =
                    Real2Sample((rLeftChan + rRightChan) * rMixNormConst);
        }
        break;
    }

    if (bMuteAudio)
    {
        /* Clear both channels if muted */
        for (i = 0; i < iInputBlockSize; i++)
            vecsTmpAudData[i] = 0;
    }

    /* Put data to sound card interface. Show sound card state on GUI */

#ifdef QT_MULTIMEDIA_LIB
    bool bBad = true;
    if(pIODevice)
    {
        //cout<<"output dev: pIODevice = ["<< pIODevice <<"]" << endl;
        int n = 2 * vecsTmpAudData.Size(); // bytes to write 2 = sizeof(_SAMPLE)
        char* buf = reinterpret_cast<char*>(&vecsTmpAudData[0]);
        qint64 w = 0;
        while(n > 0)
        {
            int bytesToWrite = pAudioOutput->bytesFree();
            if(bytesToWrite == 0) {
                pIODevice->waitForBytesWritten(-1);
            }
            if(n < bytesToWrite) {
                bytesToWrite = n;
            }
            //接受机 往输出设备 写入数据

            w = pIODevice->write(buf, bytesToWrite);
            if(w != 0)
            {
                //                DEBUG_COUT("接受机 n = "<< n
                //                           << " ,w = " << w);
            }

            n -= w;
            buf += w;
        }
        bBad = false;
    }
#else
    const bool bBad = pSound->Write(vecsTmpAudData);
#endif
    Parameters.Lock();
    Parameters.ReceiveStatus.InterfaceO.SetStatus(bBad ? DATA_ERROR : RX_OK); /* Yellow light */
    Parameters.Unlock();

    /* Write data as wave in file */
    if (bDoWriteWaveFile)
    {
        /* Write audio data to file only if it is not zero */
        bool bDoNotWrite = true;
        for (i = 0; i < iInputBlockSize; i++)
        {
            if ((*pvecInputData)[i] != 0)
                bDoNotWrite = false;
        }

        if (bDoNotWrite == false)
        {
            for (i = 0; i < iInputBlockSize; i += 2)
            {
                WaveFileAudio.AddStereoSample((*pvecInputData)[i] /* left */,
                                              (*pvecInputData)[i + 1] /* right */);
            }
        }
    }

    /* Store data in buffer for spectrum calculation
    将数据存储在频谱计算缓冲区中  38400  */
    //std::cout<<"iInputBlockSize = " << iInputBlockSize << std::endl;
    vecsOutputData.AddEnd((*pvecInputData), iInputBlockSize);
}

void CWriteData::InitInternal(CParameter& Parameters)
{
    /* Get audio sample rate */
    iAudSampleRate = Parameters.GetAudSampleRate();

    /* Set maximum audio frequency */
    iMaxAudioFrequency = MAX_SPEC_AUDIO_FREQUENCY;
    if (iMaxAudioFrequency > iAudSampleRate/2)
        iMaxAudioFrequency = iAudSampleRate/2;

    /* Length of vector for audio spectrum. We use a power-of-two length to
       make the FFT work more efficiently, need to be scaled from sample rate to
       keep the same frequency resolution */
    iNumSmpls4AudioSprectrum = ADJ_FOR_SRATE(1024, iAudSampleRate);

    /* Number of blocks for averaging the audio spectrum */
    iNumBlocksAvAudioSpec = int(ceil(_REAL(iAudSampleRate *
                                           TIME_AV_AUDIO_SPECT_MS)
                                     / 1000.0 / _REAL(iNumSmpls4AudioSprectrum)));

    /* Inits for audio spectrum plotting */
    vecsOutputData.Init(iNumBlocksAvAudioSpec * iNumSmpls4AudioSprectrum * 2 /* stereo */, 0); /* Init with zeros */
    FftPlan.Init(iNumSmpls4AudioSprectrum);
    veccFFTInput.Init(iNumSmpls4AudioSprectrum);
    veccFFTOutput.Init(iNumSmpls4AudioSprectrum);
    vecrAudioWindowFunction.Init(iNumSmpls4AudioSprectrum);

    /* An audio frame always corresponds to 400 ms.
       We use always stereo blocks
        一个音频帧总是对应于400毫秒。  我们总是使用立体声块 */
    const int iAudFrameSize = int(_REAL(iAudSampleRate) * 0.4 /* 400 ms */);

    /* Check if blocking behaviour of sound interface shall be changed */
    if (bNewSoundBlocking != bSoundBlocking)
        bSoundBlocking = bNewSoundBlocking;

    /* Init sound interface with blocking or non-blocking behaviour */
#ifdef QT_MULTIMEDIA_LIB
    if(pAudioOutput != nullptr)
    {
        // 1. 设置缓冲区大小 (空间为两帧 * 字节每个样本 * 立体声)
        pAudioOutput->setBufferSize(2 * 2 * 2 * iAudFrameSize); // room for two frames * bytes per sample * stereo
        // 2. 启动音频输出  并且音频输出设备会自动检测到并播放这些数据。
        pIODevice = pAudioOutput->start();
        // buffer size = 153600
        // period size = 3840
        /// soundDevice = alsa_output.usb-JU_Jiu_2023-02-20-0000-0000-0000-00.analog-stereo
        cerr << "buffer size " << pAudioOutput->bufferSize() << endl;
        cerr << "period size " << pAudioOutput->periodSize() << endl;
        if(pAudioOutput->error() != QAudio::NoError)
        {
            qDebug("Can't open audio output");
        }
    }

#endif
    if(pSound!=nullptr) pSound->Init(iAudSampleRate, iAudFrameSize * 2 /* stereo */, bSoundBlocking); // might be a sound file

    /* Init intermediate buffer needed for different channel selections */
    vecsTmpAudData.Init(iAudFrameSize * 2 /* stereo */);

    /* Inits for audio spectrum plot */
    vecrAudioWindowFunction = Hann(iNumSmpls4AudioSprectrum);
    vecsOutputData.Reset(0); /* Reset audio data storage vector */

    /* Define block-size for input (stereo input) */
    iInputBlockSize = iAudFrameSize * 2 /* stereo */;
}

void CWriteData::StartWriteWaveFile(const string& strFileName)
{
    /* No Lock(), Unlock() needed here */
    if (bDoWriteWaveFile == false)
    {
        WaveFileAudio.Open(strFileName, iAudSampleRate);
        bDoWriteWaveFile = true;
    }
}

void CWriteData::StopWriteWaveFile()
{
    Lock();

    WaveFileAudio.Close();
    bDoWriteWaveFile = false;

    Unlock();
}

void CWriteData::GetAudioSpec(CVector<_REAL>& vecrData,
                              CVector<_REAL>& vecrScale)
{
    if (iAudSampleRate == 0)
    {
        /* Init output vectors to zero data */
        vecrData.Init(0, (_REAL) 0.0);
        vecrScale.Init(0, (_REAL) 0.0);
        return;
    }

    /* Real input signal -> symmetrical spectrum -> use only half of spectrum */
    const _REAL rLenPowSpec = _REAL(iNumSmpls4AudioSprectrum) * iMaxAudioFrequency / iAudSampleRate;
    const int iLenPowSpec = int(rLenPowSpec);

    /* Init output vectors */
    vecrData.Init(iLenPowSpec, (_REAL) 0.0);
    vecrScale.Init(iLenPowSpec, (_REAL) 0.0);

    int i;

    /* Lock resources */
    Lock();

    /* Init vector storing the average spectrum with zeros */
    CVector<_REAL> veccAvSpectrum(iLenPowSpec, (_REAL) 0.0);

    int iCurPosInStream = 0;
    for (i = 0; i < iNumBlocksAvAudioSpec; i++)
    {
        int j;

        /* Mix both channels */
        for (j = 0; j < iNumSmpls4AudioSprectrum; j++)
        {
            int jj =  2*(iCurPosInStream + j);
            veccFFTInput[j] = _REAL(vecsOutputData[jj] + vecsOutputData[jj + 1]) / 2;
        }

        /* Apply window function */
        veccFFTInput *= vecrAudioWindowFunction;

        /* Calculate Fourier transformation to get the spectrum */
        veccFFTOutput = Fft(veccFFTInput, FftPlan);

        /* Average power (using power of this tap) */
        for (j = 0; j < iLenPowSpec; j++)
            veccAvSpectrum[j] += SqMag(veccFFTOutput[j]);

        iCurPosInStream += iNumSmpls4AudioSprectrum;
    }

    /* Calculate norm constant and scale factor */
    const _REAL rNormData = (_REAL) iNumSmpls4AudioSprectrum *
            iNumSmpls4AudioSprectrum * _MAXSHORT * _MAXSHORT *
            iNumBlocksAvAudioSpec;

    /* Define scale factor for audio data */
    const _REAL rFactorScale = _REAL(iMaxAudioFrequency) / iLenPowSpec / 1000;

    /* Apply the normalization (due to the FFT) */
    for (i = 0; i < iLenPowSpec; i++)
    {
        const _REAL rNormPowSpec = veccAvSpectrum[i] / rNormData;

        if (rNormPowSpec > 0)
            vecrData[i] = 10.0 * log10(rNormPowSpec);
        else
            vecrData[i] = RET_VAL_LOG_0;

        vecrScale[i] = _REAL(i) * rFactorScale;
    }

    /* Release resources */
    Unlock();
}


/******************************************************************************\
* FAC data                                                                    *
\******************************************************************************/
/* Transmitter */
void CGenerateFACData::ProcessDataInternal(CParameter& TransmParam)
{
    FACTransmit.FACParam(pvecOutputData, TransmParam);
}

void CGenerateFACData::InitInternal(CParameter& TransmParam)
{
    FACTransmit.Init(TransmParam);

    /* Define block-size for output */
    iOutputBlockSize = NUM_FAC_BITS_PER_BLOCK;
}

/* Receiver */
void CUtilizeFACData::ProcessDataInternal(CParameter& Parameters)
{
    /* Do not use received FAC data in case of simulation */
    if (bSyncInput == false)
    {
        bCRCOk = FACReceive.FACParam(pvecInputData, Parameters);
        /* Set FAC status for RSCI, log file & GUI */
        if (bCRCOk)
            Parameters.ReceiveStatus.FAC.SetStatus(RX_OK);
        else
            Parameters.ReceiveStatus.FAC.SetStatus(CRC_ERROR);
    }

    if ((bSyncInput) || (bCRCOk == false))
    {
        /* If FAC CRC check failed we should increase the frame-counter
           manually. If only FAC data was corrupted, the others can still
           decode if they have the right frame number. In case of simulation
           no FAC data is used, we have to increase the counter here
            如果FAC CRC检查失败，我们应该增加帧计数器
            手动。如果只有FAC数据被损坏，其他数据仍然可以
            解码，如果他们有正确的帧数。在模拟情况下
            没有使用FAC数据，我们必须在这里增加计数器
            */
        Parameters.iFrameIDReceiv++;

        if (Parameters.iFrameIDReceiv == NUM_FRAMES_IN_SUPERFRAME)
            Parameters.iFrameIDReceiv = 0;
    }
}

void CUtilizeFACData::InitInternal(CParameter& Parameters)
{

    // This should be in FAC class in an Init() routine which has to be defined, this
    // would be cleaner code! TODO
    /* Init frame ID so that a "0" comes after increasing the init value once
        初始化帧ID，使初始化值增加一次后出现  0 */
    Parameters.iFrameIDReceiv = NUM_FRAMES_IN_SUPERFRAME - 1;

    /* Reset flag */
    bCRCOk = false;

    /* Define block-size for input */
    iInputBlockSize = NUM_FAC_BITS_PER_BLOCK;
}


/******************************************************************************\
* SDC data                                                                    *
\******************************************************************************/
/* Transmitter */
void CGenerateSDCData::ProcessDataInternal(CParameter& TransmParam)
{
    SDCTransmit.SDCParam(pvecOutputData, TransmParam);
}

void CGenerateSDCData::InitInternal(CParameter& TransmParam)
{
    /* Define block-size for output */
    iOutputBlockSize = TransmParam.iNumSDCBitsPerSFrame;
}

/* Receiver */
void CUtilizeSDCData::ProcessDataInternal(CParameter& Parameters)
{
    //    bool bSDCOK = false;

    /* Decode SDC block and return CRC status */
    CSDCReceive::ERetStatus eStatus = SDCReceive.SDCParam(pvecInputData, Parameters);

    Parameters.Lock();
    switch (eStatus)
    {
    case CSDCReceive::SR_OK:
        Parameters.ReceiveStatus.SDC.SetStatus(RX_OK);
        //        bSDCOK = true;
        break;

    case CSDCReceive::SR_BAD_CRC:
        /* SDC block depends on only a few parameters: robustness mode,
           DRM bandwidth and coding scheme (can be 4 or 16 QAM). If we
           initialize these parameters with resonable parameters it might
           be possible that these are the correct parameters. Therefore
           try to decode SDC even in case FAC wasn't decoded. That might
           speed up the DRM signal acqisition. But quite often it is the
           case that the parameters are not correct. In this case do not
           show a red light if SDC CRC was not ok */
        if (bFirstBlock == false)
            Parameters.ReceiveStatus.SDC.SetStatus(CRC_ERROR);
        break;

    case CSDCReceive::SR_BAD_DATA:
        /* CRC was ok but data seems to be incorrect */
        Parameters.ReceiveStatus.SDC.SetStatus(DATA_ERROR);
        break;
    }
    Parameters.Unlock();

    /* Reset "first block" flag */
    bFirstBlock = false;
}

void CUtilizeSDCData::InitInternal(CParameter& Parameters)
{
    /* Init "first block" flag */
    bFirstBlock = true;

    /* Define block-size for input */
    iInputBlockSize = Parameters.iNumSDCBitsPerSFrame;
}


/* CWriteIQFile : module for writing an IQ or IF file */

CWriteIQFile::CWriteIQFile() : pFile(0), iFrequency(0), bIsRecording(false), bChangeReceived(false)
{
}

CWriteIQFile::~CWriteIQFile()
{
    if (pFile != 0)
        fclose(pFile);
}

void CWriteIQFile::StartRecording(CParameter&)
{
    bIsRecording = true;
    bChangeReceived = true;
}

void CWriteIQFile::OpenFile(CParameter& Parameters)
{
    iFrequency = Parameters.GetFrequency();

    /* Get current UTC time */
    time_t ltime;
    time(&ltime);
    struct tm* gmtCur = gmtime(&ltime);

    stringstream filename;
    filename << Parameters.GetDataDirectory();
    filename << Parameters.sReceiverID << "_";
    filename << setw(4) << setfill('0') << gmtCur->tm_year + 1900 << "-" << setw(2) << setfill('0')<< gmtCur->tm_mon + 1;
    filename << "-" << setw(2) << setfill('0')<< gmtCur->tm_mday << "_";
    filename << setw(2) << setfill('0') << gmtCur->tm_hour << "-" << setw(2) << setfill('0')<< gmtCur->tm_min;
    filename << "-" << setw(2) << setfill('0')<< gmtCur->tm_sec << "_";
    filename << setw(8) << setfill('0') << (iFrequency*1000) << ".iq" << (Parameters.GetSigSampleRate()/1000);

    pFile = fopen(filename.str().c_str(), "wb");

}

void CWriteIQFile::StopRecording()
{
    bIsRecording = false;
    bChangeReceived = true;
}

void CWriteIQFile::NewFrequency(CParameter &)
{
}

void CWriteIQFile::InitInternal(CParameter& Parameters)
{
    /* Get parameters from info class */
    const int iSymbolBlockSize = Parameters.CellMappingTable.iSymbolBlockSize;

    iInputBlockSize = iSymbolBlockSize;

    /* Init temporary vector for filter input and output */
    rvecInpTmp.Init(iSymbolBlockSize);
    cvecHilbert.Init(iSymbolBlockSize);

    /* Init mixer */
    Mixer.Init(iSymbolBlockSize);

    /* Inits for Hilbert and DC filter -------------------------------------- */
    /* Hilbert filter block length is the same as input block length */
    iHilFiltBlLen = iSymbolBlockSize;

    /* Init state vector for filtering with zeros */
    rvecZReal.Init(iHilFiltBlLen, (CReal) 0.0);
    rvecZImag.Init(iHilFiltBlLen, (CReal) 0.0);

    /* "+ 1" because of the Nyquist frequency (filter in frequency domain) */
    cvecBReal.Init(iHilFiltBlLen + 1);
    cvecBImag.Init(iHilFiltBlLen + 1);

    /* FFT plans are initialized with the long length */
    FftPlansHilFilt.Init(iHilFiltBlLen * 2);

    /* Set up the band-pass filter */

    /* Set internal parameter */
    const CReal rBPNormBW = CReal(0.4);

    /* Actual prototype filter design */
    CRealVector vecrFilter(iHilFiltBlLen);
    vecrFilter = FirLP(rBPNormBW, Nuttallwin(iHilFiltBlLen));

    /* Assume the IQ will be centred on a quarter of the soundcard sampling rate (e.g. 12kHz @ 48kHz) */
    const CReal rBPNormCentOffset = CReal(0.25);

    /* Set filter coefficients ---------------------------------------------- */
    /* Not really necessary since bandwidth will never be changed */
    const CReal rStartPhase = (CReal) iHilFiltBlLen * crPi * rBPNormCentOffset;

    /* Copy actual filter coefficients. It is important to initialize the
       vectors with zeros because we also do a zero-padding */
    CRealVector rvecBReal(2 * iHilFiltBlLen, (CReal) 0.0);
    CRealVector rvecBImag(2 * iHilFiltBlLen, (CReal) 0.0);
    for (int i = 0; i < iHilFiltBlLen; i++)
    {
        rvecBReal[i] = vecrFilter[i] *
                Cos((CReal) 2.0 * crPi * rBPNormCentOffset * i - rStartPhase);

        rvecBImag[i] = vecrFilter[i] *
                Sin((CReal) 2.0 * crPi * rBPNormCentOffset * i - rStartPhase);
    }

    /* Transformation in frequency domain for fft filter */
    cvecBReal = rfft(rvecBReal, FftPlansHilFilt);
    cvecBImag = rfft(rvecBImag, FftPlansHilFilt);

}

void CWriteIQFile::ProcessDataInternal(CParameter& Parameters)
{
    int i;

    if (bChangeReceived) // file is open but we want to start a new one
    {
        bChangeReceived = false;
        if (pFile != nullptr)
        {
            fclose(pFile);
        }
        pFile = nullptr;
    }

    // is recording switched on?
    if (!bIsRecording)
    {
        if (pFile != nullptr)
        {
            fclose(pFile); // close file if currently open
            pFile = nullptr;
        }
        return;
    }


    // Has the frequency changed? If so, close any open file (a new one will be opened)
    int iNewFrequency = Parameters.GetFrequency();

    if (iNewFrequency != iFrequency)
    {
        iFrequency = iNewFrequency;
        // If file is currently open, close it
        if (pFile != nullptr)
        {
            fclose(pFile);
            pFile = nullptr;
        }
    }
    // Now open the file with correct name if it isn't currently open
    if (!pFile)
    {
        OpenFile(Parameters);
    }

    /* Band-pass filter and mixer ------------------------------------------- */
    /* Copy CVector data in CMatlibVector */
    for (i = 0; i < iInputBlockSize; i++)
        rvecInpTmp[i] = (*pvecInputData)[i];

    /* Cut out a spectrum part of desired bandwidth */
    cvecHilbert = CComplexVector(
                FftFilt(cvecBReal, rvecInpTmp, rvecZReal, FftPlansHilFilt),
                FftFilt(cvecBImag, rvecInpTmp, rvecZImag, FftPlansHilFilt));

    /* Mix it down to zero frequency */
    Mixer.SetMixFreq(CReal(0.25));
    Mixer.Process(cvecHilbert);

    /* Write to the file */

    _SAMPLE re, im;
    _BYTE bytes[4];

    CReal rScale = CReal(1.0);
    for (i=0; i<iInputBlockSize; i++)
    {
        re = _SAMPLE(cvecHilbert[i].real() * rScale);
        im = _SAMPLE(cvecHilbert[i].imag() * rScale);
        bytes[0] = _BYTE(re & 0xFF);
        bytes[1] = _BYTE((re>>8) & 0xFF);
        bytes[2] = _BYTE(im & 0xFF);
        bytes[3] = _BYTE((im>>8) & 0xFF);

        fwrite(bytes, 4, sizeof(_BYTE), pFile);
    }
}
void CMSCReadData::ReadData(CParameter &Parameter, CSingleBuffer<_BINARY> &mscOutputBuffer)
{
    this->m_InputSize = mscOutputBuffer.QueryWriteBuffer()->size();
    // 复用流 准备好了
    if(Parameter.multiplexerOK)
    {
        // 获取标识位
        if (mscOutputBuffer.GetRequestFlag())
        {
            // pVecOutput = mscOutputBuffer.QueryWriteBuffer();
            pVecOutput = mscOutputBuffer.Get(this->m_InputSize);
            this->InitInternal(Parameter);
            // processInternal
            this->ProcessDataInternal(Parameter);

            //ok
            // mscOutputBuffer.Put(this->outputSize);

            mscOutputBuffer.SetRequestFlag(false);
        }
        else
        {
            std::cout<<"复用流 准备好了, 但是没有请求 "<<std::endl;
        }

        Parameter.multiplexerOK = false;
    }


}


// my
void CMSCReadData::ReadData(CParameter &Parameter, CSingleBuffer<_BINARY> &mscInputBuffer,
                            CSingleBuffer<_BINARY> &mscOutputBuffer)
{


    // 复用流 准备好了
    if(Parameter.multiplexerOK)
    {

        // 获取标识位
        if (mscOutputBuffer.GetRequestFlag())
        {
            pvecInput = mscInputBuffer.QueryWriteBuffer();
            m_InputSize = pvecInput->size();


            this->InitInternal(Parameter);
            // 真正的处理在 process
            this->ProcessDataInternal(Parameter);

            //ok
            mscOutputBuffer.Put(this->outputSize);

            mscOutputBuffer.SetRequestFlag(false);
        }


        Parameter.multiplexerOK = false;
    }
}

void CMSCReadData::InitInternal(CParameter &TransmParam)
{
    //

}

void CMSCReadData::ProcessDataInternal(CParameter &TransmParam)
{
    TransmParam.Lock();
#if 0

    if(!m_bInit)
    {
        // output buff 初始化
        CVectorEx<_BINARY>* pMSCBuf = this->m_MSCBuf.QueryWriteBuffer();
        m_InputSize = pMSCBuf->capacity() ;
        pMSCBuf->resize(m_InputSize);
        std::cout<<" m_InputSize = "<< m_InputSize << std::endl;
        m_bInit = true;
    }

    //MSC 复用流
    CVector<_BINARY>* vecInputStr =  pvecInput;

    // 内部复用流(准备 解调使用)
    CVector<_BINARY>* pvecOutputData = m_MSCBuf.QueryWriteBuffer();;


    const int iStreamLen = vecInputStr->size();

    const int iLen = pvecOutputData->size();
    const int iLen2 = pvecOutputData->capacity();
    if(iLen == 0 )
    {
        pvecOutputData->resize(iStreamLen);
    }
    // Len = 0,  StreamLen:5904
    std::cout<<"iLen = "<<iLen <<",iLen2 = "<<iLen2 << ", iStreamLen:" << iStreamLen <<std::endl;


    //
    if (iLen >= iStreamLen)
    {
        // Copy data
        vecInputStr->ResetBitAccess();
        pvecOutputData->ResetBitAccess();

        int j = 0;


        for (j = 0; j < iStreamLen / SIZEOF__BYTE; j++)
        {
            pvecOutputData->Enqueue(vecInputStr->Separate(SIZEOF__BYTE), SIZEOF__BYTE);
        }

        std::cout<< " (循环了次数) j = "<< j <<std::endl;
        // 使能
        m_MSCBuf.SetRequestFlag(true);

        // 记录大小
        this->outputSize = iStreamLen;
        std::cout<< " this->outputSize = "<< this->outputSize <<std::endl;
    }

#else
    int iLen = pVecOutput->size();
    int Size = pVecOutput->Size();
    int capacity = pVecOutput->capacity();
    // iLen = 5904,  Size = 0,  iStreamLen:5904
    this->outputSize = iLen;
    std::cout<<" iLen = "<<iLen <<",Size = " << Size <<",iStreamLen:" << capacity <<std::endl;

#endif

    TransmParam.Unlock();
}




