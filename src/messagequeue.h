#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H
#include "DrmReceiver.h"
#include "DrmTransmitter.h"
#include "Soundbar/soundbar.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <cstring>
#include <cerrno>

class CMessageQueue
{
public:
    CMessageQueue();
    CMessageQueue(const std::string& path);
    ~CMessageQueue();

    bool SendMessage(long type, CDRMReceiver* pDRMReceiver);    //传入参数
    // bool ReceiveMessage(long type, CDRMReceiver*& pDRMReceiver); //穿出参数
    CDRMReceiver* ReceiveMessage(long type );

    // 清空消息队列
    void ClearMessageQueue();

    key_t msq_key;
    int msq_id;
};


class RecvToMulti_t
{
public:
    long mytype;
    CDRMReceiver* pDRMReceiver;         // 接受机的 指针
};


// 接收机给复用器子线程 传参
typedef struct RECIVER_ADD_transmit
{
    CDRMReceiver* pDRMReceiver;         // 接受机的 指针
    CDRMTransmitter* pDRMTransmitter;   // 发射机的 指针

}RecvAddtransmit_t;

//打印流
void Print_stream(std::vector<CSingleBuffer<_BINARY>>* pMSCUseBuf);

//拷贝MSC 其中一个流
void Print_stream1( CSingleBuffer<_BINARY>* pMSCUseBuf);


//拷贝MSC【0-3】流
int Copy_stream(std::vector<CSingleBuffer<_BINARY>>* pMSCUseBuf,
                CMSCmultiplexer* pMSCmultiplexer);
//拷贝MSC【0-3】第二个版本
int Copy_stream(std::vector<CSingleBuffer<_BINARY>>* pMSCUseBuf,
                std::vector<CSingleBuffer<_BINARY>>* pTxMSCUseBuf);

//拷贝MSC【0-3】第三个版本 3
int CopyStream(std::vector<CSingleBuffer<_BINARY> >* pMSCUseBuf,
               std::vector<CSingleBuffer<_BINARY> >* pTxMSCUseBuf,CParameter* pParameter_recv);
//复制FAC的参数
int CopyParameter( CParameter* pParameter_recv , CParameter* pParameter_trans);



#endif // MESSAGEQUEUE_H

