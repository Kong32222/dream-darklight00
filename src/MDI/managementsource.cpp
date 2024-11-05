#include "managementsource.h"


ManagementSource::ManagementSource():
    workerThread()
{
    //DEBUG_COUT("ManagementSource构造 ");  // 16个


}

void ManagementSource::pushMessage(CSingleBuffer<_BINARY>& data)
{
    CVectorEx<_BINARY>* pData = data.QueryWriteBuffer();

    CVectorEx<_BINARY>* m_Data = this->message.QueryWriteBuffer();

    // Copy stream data
    int  MDIBufLength = pData->Size();
    // m_Data->Init(AFBufLength);

    pData->ResetBitAccess();
    m_Data->ResetBitAccess();

    for (int i = 0; i < MDIBufLength / SIZEOF__BYTE; i++)
    {
        m_Data->Enqueue(pData->Separate(SIZEOF__BYTE),
                        SIZEOF__BYTE);
    }
    MDIBufLength = m_Data->Size();

    // 将数据添加到队列中

    // message
    //queueCondVar.notify_one(); // 通知线程有新数据到达
}

//bool ManagementSource::popMessage()
//{

//}

bool ManagementSource::matches(const string &ip, uint16_t port) const
{
    if (clients.ip == ip && clients.port == port)
    {
        return true;
    }

    return false;
}

void ManagementSource::stopThread()
{


}

void ManagementSource::startThread(const IPAddress& Clients)
{
    this->clients = Clients;
    workerThread = std::thread(ReceiveThread, this);
    workerThread.detach();      // 将线程设置为分离态
}

//
void ReceiveThread(ManagementSource *managementSource)
{
    //DRMReceiver DRMReceiver(&Settings);

    CSettings Settings;
    // 加载新的文件 source.ini
    Settings.LoadIni(SOURCE_INIT_FILE_NAME);

    //managementSource->receiver = new CDRMReceiver(&Settings);
    pid_t threadID = syscall(SYS_gettid); // 获取线程ID
    DEBUG_COUT("thread ID: "<< threadID);

    while (true)
    {
        DEBUG_COUT("thread ID: "<< threadID);
        // readdata(适配各种数据源), 数据从主接收机 传过来
        // managementSource->receiver->process(managementSource->message);

        // CParameter* parameter = managementSource->receiver->GetParameters();


    }

    return;
}



