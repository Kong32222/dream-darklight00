#ifndef CMAINDRMRECEIVER_H
#define CMAINDRMRECEIVER_H
#include "../DrmReceiver.h"
#include "managementsource.h"
#include "globalvariables_.h"

#include <array>
#include <unistd.h>
#include <QHash>

#include "../messagequeue.h"
#include <QHostAddress>

#include "src/channelmanager.h"
#include "../loopback.h"


//定时器
void timerFunction();


class ManagementSource;

class Loopback; // 前向声明

class CMainDRMReceiver: public CDRMReceiver
{

public:
    CMainDRMReceiver(CSettings* pSettings = nullptr);
    ~CMainDRMReceiver();

    enum DataSourceType {
        Network = 0,
        File,
        SoundCard,
        Loopback
    };

    void process() override;
    void LoadSettings() override;


    void sendMessageToManagement(const std::string& ip, uint16_t port,
                                 CSingleBuffer<_BINARY>& data);


    int configureStream(std::vector<CSingleBuffer<_BINARY>>& src,
                         const vector<int> &srcrows,
                         std::vector<CSingleBuffer<_BINARY>>& dest,
                         const vector<int> &destrows);
    void checkTimerouts();

    void startTimer()
    {
        std::thread(&CMainDRMReceiver::checkTimerouts,this).detach();
    }
    void printClientLastActive(const QHash<QPair<QHostAddress, quint16>, QDateTime>&
                               clientLastActive);
public:
    std::vector<CSingleBuffer<_BINARY> >   m_MSCmonitorBuf;   // 监测 MSC  buff
    std::vector<CSingleBuffer<_BINARY> > MSCBuff;             // 复用流 , 并Split


    std::array<ManagementSource, 16> managementSources;     //  管理16个接收机子线程

    int Threadsused;                                        //  记录 接收机子线程 个数

    CDRMTransmitter* pDRMTransmitter = nullptr;              // 发射机 指针

    ChannelManager m_ChannelManager;                     // 数据通道复用管理类

    DataSourceType m_dataSourceType = DataSourceType::Loopback;
    class Loopback m_LoopBack;                            // 必须加class

    QHash<QPair<QHostAddress, quint16>, QDateTime> clientLastActive;
    int ProcessAudioFromCharArray(unsigned char* inputData,
                                   const size_t dataSize,
                                   CVectorEx<_REAL>* Ringbuff);

private:


};

#endif // CMAINDRMRECEIVER_H
