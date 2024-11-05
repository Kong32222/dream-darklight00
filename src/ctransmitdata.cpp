/******************************************************************************\
 * BBC and Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2019
 *
 * Author(s):
 * Volker Fischer, Julian Cable
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

#include "ctransmitdata.h"
#include "Parameter.h"
#include "IQInputFilter.h"
#ifdef QT_MULTIMEDIA_LIB
# include <QSet>
# include <QAudioOutput>
#else
# include "sound/soundinterfacefactory.h"
#endif
using namespace std;

CTransmitData::CTransmitData() :
    pFileTransmitter(nullptr),  //IQ file
    #ifdef QT_MULTIMEDIA_LIB
    pIODevice(nullptr),
    #endif
    pSound(nullptr),
    eOutputFormat(OF_REAL_VAL),  // OF_REAL_VAL(实数) ; OF_IQ_POS
    rDefCarOffset(VIRTUAL_INTERMED_FREQ),
    //   test/TransmittedData.IQ
    strOutFileName("/home/linux/Dream_dir/2024_9_11/TransmittedData.IQ"),
    bUseSoundcard(true),  //默认是 使用声卡 true;
    bAmplified(false), bHighQualityIQ(false)
{

}

void CTransmitData::Stop()
{
#ifdef QT_MULTIMEDIA_LIB
    if (pIODevice != nullptr) pIODevice->close();
#endif
    if (pSound != nullptr) pSound->Close();
}

/*
 这个 Enumerate 函数的作用是列举系统中的音频输出设备，
 并将设备名称、描述和默认输出设备的信息存储到传入的参数中

*/
void CTransmitData::Enumerate(vector<string>& names,
                              vector<string>& descriptions,
                              string& defaultOutput)
{
    DEBUG_COUT("发射机遍历输出设备 Enumerate");
#ifdef QT_MULTIMEDIA_LIB
    // 用于存储设备名称，确保唯一性
    QSet<QString> s;

    // 获取默认输出设备的名称
    QString def = QAudioDeviceInfo::defaultOutputDevice().deviceName();
    // 将默认输出设备名称转换为 std::string 并存储到 defaultOutput 中
    defaultOutput = def.toStdString();
    cerr << "default output device is " << defaultOutput << endl;

    // 遍历所有可用的音频输出设备，并将它们的名称插入到 QSet 中
    foreach(const QAudioDeviceInfo & di,
            QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
    {
        s.insert(di.deviceName());
    }

    names.clear();
    descriptions.clear();

    //// 遍历 QSet 中的设备名称，将它们添加到 names 向量中
    //// 如果设备是默认设备，描述中添加 "default"，否则添加空字符串
    int i = 0;
    foreach(const QString n, s)
    {
        i++;
        //cerr << "have output device " << n.toStdString() << endl;
        names.push_back(n.toStdString());
        if (n == def)
        {
            descriptions.push_back("default");
        }
        else
        {
            descriptions.push_back("");
        }
    }
#else
    if (pSound == nullptr) pSound = CSoundInterfaceFactory::CreateSoundOutInterface();
    pSound->Enumerate(names, descriptions, defaultOutput);
#endif
    cout << "默认输出是: " << defaultOutput << endl;
}

void CTransmitData::SetSoundInterface(string device)
{
    //发射机
    soundDevice = device;

#ifdef QT_MULTIMEDIA_LIB
    QAudioFormat format;
    if (iSampleRate == 0) iSampleRate = 48000; // TODO get initialisation order right
    format.setSampleRate(iSampleRate);
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setChannelCount(2); // TODO
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setCodec("audio/pcm");
    foreach(const QAudioDeviceInfo & di,
            QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
    {
        if (device == di.deviceName().toStdString())
        {
            QAudioFormat nearestFormat = di.nearestFormat(format);
            QAudioOutput* pAudioOutput = new QAudioOutput(di, nearestFormat);
            if (pAudioOutput->error() != QAudio::NoError)
            {
                qDebug("Can't open audio output");
            }
            else
            {
                pAudioOutput->setBufferSize(1000000);
                pIODevice = pAudioOutput->start();
                // cout : "default"
                //                DEBUG_COUT("发射机输出设备 IODevice string = [" <<
                //                    QString::fromStdString(GetSoundInterface()) << "]");
            }



            if (pAudioOutput->error() != QAudio::NoError)
            {
                qDebug("Can't open audio output");
            }
        }
    }
#else
    if (pSound != nullptr)
    {
        delete pSound;
        pSound = nullptr;
    }
    pSound = CSoundInterfaceFactory::CreateSoundOutInterface();
    pSound->SetDev(device);
#endif
}
/****
并根据不同的输出格式将数据处理并存储到输出缓冲区 vecsDataOut 中。下面对其主要步骤进行中文解析：
函数功能概述：
    该函数的主要功能是处理输入信号数据，并根据设定的输出格式，将数据转换为相应的输出信号格式，
输出到声音卡的两个声道中。
    处理过程中应用了带通滤波器和希尔伯特变换，并支持多种不同的输出格式。




***/
void CTransmitData::ProcessDataInternal(CParameter&)
{
    int i;

    /* Apply bandpass filter 该行代码对输入信号 pvecInputData 应用了带通滤波器 */
    BPFilter.Process(*pvecInputData);

    /* Convert vector type. Fill vector with symbols (collect them)
        信号转换及采集：
        这里循环遍历输入数据，每次取两个采样点处理。
    iCurIndex 是当前数据块在输出缓冲区中的索引，cInputData 则表示输入数据的一个复数采样点。
    */
    const int iNs2 = iInputBlockSize * 2;   //iNs2=  1280
    // OFDM iInputBlockSize = 640 ;根据 FFT 计算出
    // cout<<"OFDM iInputBlockSize = "<<iInputBlockSize <<endl;
    for (i = 0; i < iNs2; i += 2)
    {
        const int iCurIndex = iBlockCnt * iNs2 + i;
        _COMPLEX cInputData = (*pvecInputData)[i / 2];

        /**如果启用了高质量 IQ 模式并且输出格式不是实数信号格式，则对输入数据 cInputData 进行希尔伯特变换。*/
        if (bHighQualityIQ && eOutputFormat != OF_REAL_VAL)
            HilbertFilt(cInputData);

        /**复数数据的处理：
         * 输入数据 cInputData 是复数形式的信号，这里将其分为实部和虚部，
         * 并通过   Real2Sample 转换为输出格式
        sCurOutReal 表示信号的实部，sCurOutImag 表示信号的虚部。
        */

        /* Imaginary, real */
        const _SAMPLE sCurOutReal =
                Real2Sample(cInputData.real() * rNormFactor);

        const _SAMPLE sCurOutImag =
                Real2Sample(cInputData.imag() * rNormFactor);

        //包络和相位计算： 除了实部和虚部，还计算了输入信号的包络和相位：
        //表示信号幅度  CurOutPhase 是信号的相位角。
        /* Envelope, phase */
        const _SAMPLE sCurOutEnv =
                Real2Sample(Abs(cInputData) * 256.0);
        const _SAMPLE sCurOutPhase = /* 2^15 / pi / 2 -> approx. 5000 */
                Real2Sample(Angle(cInputData) * 5000.0);

        //根据输出格式选择相应的处理方式  0

        // eOutputFormat = OF_IQ_POS;   //  I/Q 信号

        switch (eOutputFormat)   //  0
        {
        case OF_REAL_VAL:  //将实数值作为输出，输出到左右声道（两个声道输出相同的信号）。
            /* Use real valued signal as output for both sound card channels */
            vecsDataOut[iCurIndex] = vecsDataOut[iCurIndex + 1] = sCurOutReal;
            break;

        case OF_IQ_POS: // 将实部输出到左声道，虚部输出到右声道，形成 I/Q 信号。
            /* Send inphase and quadrature (I / Q) signal to stereo sound card
               output. I: left channel, Q: right channel */
            vecsDataOut[iCurIndex] = sCurOutReal;
            vecsDataOut[iCurIndex + 1] = sCurOutImag;
            break;

        case OF_IQ_NEG: // 将虚部输出到左声道，实部输出到右声道，形成倒置的 I/Q 信号。
            /* Send inphase and quadrature (I / Q) signal to stereo sound card
               output. I: right channel, Q: left channel */
            vecsDataOut[iCurIndex] = sCurOutImag;
            vecsDataOut[iCurIndex + 1] = sCurOutReal;
            break;

        case OF_EP: // 将包络输出到左声道，相位输出到右声道。
            /* Send envelope and phase signal to stereo sound card
               output. Envelope: left channel, Phase: right channel */
            vecsDataOut[iCurIndex] = sCurOutEnv;
            vecsDataOut[iCurIndex + 1] = sCurOutPhase;
            break;
        }
    }

    //是否往主接收机中   发射数据
    if(0)
    {
        const int iCurIndex = iBlockCnt * iNs2;  //size = 1280;
        short* pvecsDataOut = &vecsDataOut[iCurIndex];
        int size = m_Loopback.pRingbuff->ring_buffer_write(pvecsDataOut, iNs2);
        if (size < 0)
            cout << "写入环形缓冲区 error (= " << size << endl;  // 环形缓冲区 满了
        else
            cout << "写入环形缓冲区 size (= " << size << endl;  // 正常情况下

    }

    std::cout << "tx : symbol = " << iBlockCnt << " iNumBlocks = " << iNumBlocks << std::endl;
    cout << "-----------" << endl;

    iBlockCnt++;

    if (iBlockCnt == iNumBlocks)
    {
        // 写入环形缓冲区----
        qint64 n = vecsDataOut.Size(); //cout<< "n = "<< n << endl;  //19,200

        short* pvecsDataOut = &vecsDataOut[0];
        int size = m_Loopback.pRingbuff->ring_buffer_write(pvecsDataOut, n);
        if(size < 0) {
            cout << "; 写入环形缓冲区 error = "<<size <<endl;
        } else {
            cout << "; 写入环形缓冲区 size = "<<size <<endl;
        }

        FlushData();

    }

}

void CTransmitData::FlushData()
{
    int i;


    /* Zero the remain of the buffer, if incomplete */
    if (iBlockCnt != iNumBlocks)
    {
        const int iSize = vecsDataOut.Size();
        const int iStart = iSize * iBlockCnt / iNumBlocks;
        for (i = iStart; i < iSize; i++)
            vecsDataOut[i] = 0;
    }

    iBlockCnt = 0;

    if (bUseSoundcard)
    {
        /* Write data to sound card. Must be a blocking function */

#ifdef QT_MULTIMEDIA_LIB
        bool bBad = true;
        // 输出设备 "default"
        if (pIODevice)
        {
            // DEBUG_COUT("发射机 sound Device  = " << QString::fromStdString(soundDevice));

            qint64 n = 2 * vecsDataOut.Size();

#if 0
            // 发射机
            qint64 m = pIODevice->write(reinterpret_cast<char*>
                                        (&vecsDataOut[0]), n);

            //  TransmitData n =  76800, m =  4408
            // DEBUG_COUT("TransmitData n = "<< n << " ,m = " << m);
            if (m == n)
            {
                bBad = false;
            }
#else

            qint64 w = 0;

            int datalangth = 0;
            char* buf = reinterpret_cast<char*>(&vecsDataOut[0]);
            cout<<"tx: OFDM out N = "<<n <<endl;  //76800
            while (n > 0)
            {
                if (n == 0)
                {
                    pIODevice->waitForBytesWritten(-1);
                }
                //  往输出设备 写入数据
                w = pIODevice->write(buf, n);  // n = buffSize();
                datalangth += w;
                n -= w;
                buf += w;
            }

            if (datalangth == (2 * vecsDataOut.Size()))
            {
                bBad = false;
            }

#endif
        }
        if (bBad)
        {
            // 写入声卡有问
            cerr << "problem writing to sound card" << endl;
            //DEBUG_COUT("problem writing to sound card");
        }
#else
        pSound->Write(vecsDataOut);
#endif
    }
    else
    {
        /* Write data to file */
        for (i = 0; i < iBigBlockSize; i++)
        {
#ifdef FILE_DRM_USING_RAW_DATA
            const short sOut = vecsDataOut[i];

            /* Write 2 bytes, 1 piece */
            fwrite((const void*) &sOut, size_t(2), size_t(1),
                   pFileTransmitter);
#else
            /* This can be read with Matlab "load" command  这可以用Matlab的“load”命令读取*/
            fprintf(pFileTransmitter, "%d\n", vecsDataOut[i]);
#endif
        }

        /* Flush the file buffer */
        fflush(pFileTransmitter);
    }
}

void CTransmitData::InitInternal(CParameter& Parameters)
{
    /*
        float*	pCurFilt;
        int		iNumTapsTransmFilt;
        CReal	rNormCurFreqOffset;
    */

    /* Get signal sample rate */
    iSampleRate = Parameters.GetSigSampleRate();
    cout<<"tx : iSampleRate = "<<iSampleRate <<endl; // tx : iSampleRate = 24000

    /* Define symbol block-size */
    const int iSymbolBlockSize = Parameters.CellMappingTable.iSymbolBlockSize;

    /* Init vector for storing a complete DRM frame number of OFDM symbols */
    iBlockCnt = 0;
    Parameters.Lock();
    iNumBlocks = Parameters.CellMappingTable.iNumSymPerFrame;
    ESpecOcc eSpecOcc = Parameters.GetSpectrumOccup();
    Parameters.Unlock();
    iBigBlockSize = iSymbolBlockSize * 2 /* Stereo */ * iNumBlocks;

    /* Init I/Q history */
    vecrReHist.Init(NUM_TAPS_IQ_INPUT_FILT_HQ, 0.0);

    vecsDataOut.Init(iBigBlockSize);

    if (pFileTransmitter != nullptr)
    {
        fclose(pFileTransmitter);
    }

    if (bUseSoundcard)
    {
        /* Init sound interface */
        if (pSound != nullptr) pSound->Init(iSampleRate, iBigBlockSize, true);
    }
    else
    {

        /* Open file for writing data for transmitting */
#ifdef FILE_DRM_USING_RAW_DATA
        pFileTransmitter = fopen(strOutFileName.c_str(), "wb");
#else
        //IQ file
        pFileTransmitter = fopen(strOutFileName.c_str(), "w");
        if (pFileTransmitter != nullptr)
        {
            cout << "pFileTransmitter = " << strOutFileName.c_str() << endl;
        }
#endif

        /* Check for error */
        if (pFileTransmitter == nullptr)
            throw CGenErr("The file " + strOutFileName + " cannot be created.");
    }


    /* Init bandpass filter object */
    BPFilter.Init(iSampleRate, iSymbolBlockSize, rDefCarOffset, eSpecOcc, CDRMBandpassFilt::FT_TRANSMITTER);

    /* All robustness modes and spectrum occupancies should have the same output
       power. Calculate the normaization factor based on the average power of
       symbol (the number 3000 was obtained through output tests) */
    rNormFactor = 3000.0 / Sqrt(Parameters.CellMappingTable.rAvPowPerSymbol);

    /* Apply amplification factor, 4.0 = +12dB
       (the maximum without clipping, obtained through output tests) */
    rNormFactor *= bAmplified ? 4.0 : 1.0;

    /* Define block-size for input */
    iInputBlockSize = iSymbolBlockSize;
}

CTransmitData::~CTransmitData()
{
    /* Close file */
    if (pFileTransmitter != nullptr)
        fclose(pFileTransmitter);
}

void CTransmitData::HilbertFilt(_COMPLEX& vecData)
{
    int i;

    /* Move old data */
    for (i = 0; i < NUM_TAPS_IQ_INPUT_FILT_HQ - 1; i++)
        vecrReHist[i] = vecrReHist[i + 1];

    vecrReHist[NUM_TAPS_IQ_INPUT_FILT_HQ - 1] = vecData.real();

    /* Filter */
    _REAL rSum = 0.0;
    for (i = 1; i < NUM_TAPS_IQ_INPUT_FILT_HQ; i += 2)
        rSum += _REAL(fHilFiltIQ_HQ[i]) * vecrReHist[i];

    vecData = _COMPLEX(vecrReHist[IQ_INP_HIL_FILT_DELAY_HQ], -rSum);
}
