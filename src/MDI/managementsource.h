#ifndef MANAGEMENTSOURCE_H
#define MANAGEMENTSOURCE_H

#include <string>
#include <vector>
#include <algorithm>
#include "../DrmReceiver.h"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <algorithm>

#include <../Soundbar/soundbar.h>

#include <unistd.h>
#include <sys/syscall.h> // for syscall()
#include <ctime>
class CDRMReceiver;



// 定义包含 IP 和端口号的结构体
typedef struct IPAddress
{
    std::string ip;
    uint16_t port;
    std::time_t lastActiveTime; // 记录最后活跃时间

    // 自定义比较运算符，用于在容器中查找重复项
    bool operator==(const IPAddress& other) const
    {
        return (ip == other.ip && port == other.port);
    }

}IPAddress;

struct ThreadData
{
    IPAddress address;
    // 其他需要传递的数据

    // vec
};


class ManagementSource;
//管理源的类
void ReceiveThread(ManagementSource* source);




class ManagementSource
{
public:
    ManagementSource();

    ~ManagementSource() { stopThread(); }

    // 线程安全的消息队列操作
    void pushMessage(CSingleBuffer<_BINARY>& data);
    // bool popMessage();

    void startThread(const IPAddress& Clients);      // 启动线程进行处理
    void stopThread();                               // 停止子线程的函数

    // 检查是否匹配给定的 IP 和端口
    bool matches(const std::string& ip, uint16_t port) const;

public:

    IPAddress clients;                               // 子接收机自己的 IP , port
    CSingleBuffer<_BINARY> message;                  //
    CDRMReceiver receiver;                           // 使用 实例化
    std::thread workerThread;                        // 线程对象

};




#endif // MANAGEMENTSOURCE_H
