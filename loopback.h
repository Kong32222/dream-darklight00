#ifndef LOOPBACK_H
#define LOOPBACK_H
#include "creceivedata.h"
#include "ringbuff.h"
#if 0

#endif
// 回环
// Loopback类继承自CReceiverModul，用于处理音频信号回环
class Loopback : public CReceiverModul<_REAL, _REAL>
{
public:
    // 构造函数：初始化Loopback对象
    Loopback();

    // 析构函数：销毁Loopback对象
    virtual ~Loopback(){}
    // 将给定的频率值进行转换，可选择是否反转频率
    _REAL ConvertFrequency(_REAL rFrequency, bool bInvert=false) const;

    // 获取输入信号的频谱数据及其刻度
    void GetInputSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);

    // 设置是否翻转频谱
    void SetFlippedSpectrum(const bool bNewF) {
        bFippedSpectrum = bNewF;
    }

    // 获取当前频谱翻转状态
    bool GetFlippedSpectrum() {
        return bFippedSpectrum;
    }

    // 清除输入数据，将输入数据缓冲区重置为零
    void ClearInputData() {
        mutexInpData.Lock(); // 加锁保护缓冲区
        vecrInpData.Init(INPUT_DATA_VECTOR_SIZE, 0.0); // 初始化输入数据缓冲区
        mutexInpData.Unlock(); // 解锁缓冲区
    }

    // 停止音频处理
    void Stop();


    // 设置输入通道选择
    void SetInChanSel(const EInChanSel eNS)
    {
        eInChanSelection = eNS;
    }

    // 获取当前输入通道选择
    EInChanSel GetInChanSel()
    {
        /* 返回左声道或其他选择的通道 */
        return eInChanSelection;
    }

    // 获取输入信号的功率谱密度(PSD)
    void GetInputPSD(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale,
                     const int iLenPSDAvEachBlock = LEN_PSD_AV_EACH_BLOCK,
                     const int iNumAvBlocksPSD = NUM_AV_BLOCKS_PSD,
                     const int iPSDOverlap = 0);

protected:
    CSignalLevelMeter		SignalLevelMeter;   // 信号电平计，用于监控输入信号的电平

    RingBuffer* pRingbuff = nullptr;            // 环形缓冲区，用于从接收器获取数据



    CVector<_SAMPLE>		vecsSoundBuffer;    // 声音缓冲区

    /* 访问 vecrInpData 缓冲区时必须在互斥锁中进行 */
    CShiftRegister<_REAL>	vecrInpData;        // 输入数据的移位寄存器
    CMutex                  mutexInpData;       // 保护输入数据的互斥锁

    int                     iSampleRate;        // 采样率
    bool                    bFippedSpectrum;    // 是否翻转频谱

    int                     iUpscaleRatio;      // 上采样率
    int                     iDownscaleRatio;    // 下采样率
    std::vector<float>		vecf_B, vecf_YL, vecf_YR, vecf_ZL, vecf_ZR;  // 滤波器系数和状态变量

    EInChanSel			eInChanSelection;    // 输入通道选择

    CVector<_REAL>		vecrReHist;           // 实部历史数据
    CVector<_REAL>		vecrImHist;           // 虚部历史数据
    _COMPLEX			cCurExp;             // 当前复数指数
    _COMPLEX			cExpStep;            // 复数指数步长
    int					iPhase;              // 相位
    SpectrumAnalyser            spectrumAnalyser;  // 频谱分析器

    // Hilbert变换滤波器，计算信号的Hilbert变换
    _REAL HilbertFilt(const _REAL rRe, const _REAL rIm);

    /* OPH: 用于计数帧内符号数量以生成RSCI输出 */
    int							iFreeSymbolCounter;

    // 内部初始化函数，用于配置参数
    virtual void InitInternal(CParameter& Parameters);

    // 内部数据处理函数，处理输入参数
    virtual void ProcessDataInternal(CParameter& Parameters);

    // 2倍插值滤波器函数，处理多通道数据
    void InterpFIR_2X(const int channels, _SAMPLE* X, std::vector<float>& Z, std::vector<float>& Y, std::vector<float>& B);

    // 2倍抽取滤波器函数，处理多通道数据
    void DecimFIR_2X(const int channels, _SAMPLE* X, std::vector<float>& Z, std::vector<float>& Y, std::vector<float>& B);

    // 发送RSCI数据，输出处理结果
    void emitRSCIData(CParameter& Parameters);
};


#endif // LOOPBACK_H
