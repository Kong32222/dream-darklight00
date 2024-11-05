#ifndef CTRX_H
#define CTRX_H

#include <QThread>
#include "../DrmTransceiver.h"
#include <vector>
#include "../Soundbar/soundbar.h"

class CTRx : public QThread, public CDRMTransceiver
{
    Q_OBJECT
public:
    explicit CTRx(QThread *parent = nullptr);
    virtual int GetFrequency()=0;

public slots:
    //重写了
    void SetInputDevice(QString InputDev)
    {
        qDebug()<< InputDev <<"emit OutputDeviceChanged(dev) no ";

    }
    void SetOutputDevice(QString OutputDev)
    {
        qDebug()<< OutputDev <<":emit OutputDeviceChanged(output_dev) no";
    }

signals:
    void InputDeviceChanged(const std::string&);
    void OutputDeviceChanged(const std::string&);

    void OutputDeviceChanged(QString); //my
    void soundUpscaleRatioChanged(int);

public slots:
    virtual void SetFrequency(int)=0;
    virtual void SetSoundSignalUpscale(int)
    {

    }

    //自己添加的
    void SetOutputDevice_(QString dev);
};

#endif // CTRX_H
