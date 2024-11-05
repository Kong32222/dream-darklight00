#ifndef CSUBDRMRECEIVER_H
#define CSUBDRMRECEIVER_H
#include "DrmReceiver.h"
#include <thread>
#include <QDebug>
#include <condition_variable> //C++
#include <mutex>   // C++

// 子接收机 :只负责接收数据 不处理数据
class CSubDRMReceiver: public CDRMReceiver
{
public:
    //CSubDRMReceiver();
    CSubDRMReceiver(CSettings* pSettings = nullptr):
        CDRMReceiver(pSettings),
        VloopBuff(MAX_NUM_STREAMS)  // 4个流
    {
        // resizeVloopBuff(16*1024);
    }

    void process() override;
    void LoadSettings() override;


    // 用于设置退出标志的函数
    void requestStop() {
        shouldExit.store(true);
    }

    // 用于检查退出标志的函数
    bool isStopRequested() const {
        return shouldExit.load(); // 原子操作读取标志位
    }

    // 线程启动
    void pthread_ReadData_start( )
    {
        //使用 this 是必要的 尤其是在调用成员函数时。这是因为成员函数的调用需要一个特定的对象实例。
        std::thread(&CSubDRMReceiver::SubDRM_ReadData, this).detach();

    }

    // 读取数据的主循环（子线程中的代码）
    void* SubDRM_ReadData()
    {


        // 可以调用 LoadSettings() 或其他成员函数
        try
        {
            cout<<"子线程已启动"<< endl;
            // 初始化 模块
            InitReceiverMode();
            SetInStartMode();
            /* Set run flag so that the thread can work */
            queueSize = 20;

            for (size_t i = 0; i < queueSize; ++i)
            {

                Qelement[i].resize(MAX_NUM_STREAMS);
                availableQueue.push(&Qelement[i]);
                //inUseQueue
            }
            while (!isStopRequested())
            {

                // 数据读取 逻辑
                updatePosition();
                process();

                static bool flag_c =true;
                if(flag_c)
                {
                    std::lock_guard<std::mutex> lock(this->m_mutex);
                    m_ready = true;
                    this->m_condition.notify_one();  // 通知主线程
                    flag_c = false;
                }

                // 检查退出标志
                if (isStopRequested()) {
                    std::cout << "Exiting SubDRMReceiver thread." << std::endl;
                    break; // 安全退出
                }
            }

            CloseSoundInterfaces();


        }
        catch (CGenErr GenErr)
        {
            qDebug("%s", GenErr.strError.c_str());
        }
        catch (string strError)
        {
            qDebug("%s", strError.c_str());
        }
        qDebug("sub Receiver working thread complete  ");   // sub 工作线程完成
        // 完成清理操作

        return nullptr;
    }

public:


    //
    void resizeVloopBuff(int size)  //16 * 1024
    {
        //static int resizeFlag = 1;
        if(1)
        {
            // 初始化 4个环形缓冲区
            int i = 0;
            for( i = 0; i < MAX_NUM_STREAMS; i++)
            {
                if(VloopBuff[i].GetvecInOutBuffersize()<size)
                    VloopBuff[i].resizeVecbuff(size);

            }

        }
    }

    std::vector<CCyclicBuffer<_BINARY>>	 VloopBuff;

    mutex SplitMSCmutex;

    bool bHasEnoughData = false;
    bool bHasEnoughDataFAC = false;
    bool bHasEnoughDataSDC = false;
    int  bHasEnoughDataMSC = 0;
    //

    vector<CSingleBuffer<_BINARY>>* pEQueuebuff = nullptr;

    std::string sourceIP;
    std::string localIP;
    int localPort;
    std::atomic<bool> shouldExit; // 原子布尔变量，保证线程安全

    vector<CSingleBuffer<_BINARY>>  Qelement[20];
    std::queue<vector<CSingleBuffer<_BINARY>>*> availableQueue;  // 空闲队列
    std::queue<vector<CSingleBuffer<_BINARY>>*> inUseQueue;      // 使用中的队列
    size_t queueSize;

    //
    std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_ready = false;


};

#endif // CSUBDRMRECEIVER_H
