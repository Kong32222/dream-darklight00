#ifndef RECEIVERMANAGER_H
#define RECEIVERMANAGER_H

#include "DrmReceiver.h"           // 基类   接收机
#include "csubdrmreceiver.h"       // 子     接收机
#include "./MDI/cmaindrmreceiver.h"// 主源   接收机
#include "DrmTransceiver.h"

class ReceiverManager
{
public:

    // 添加一个新的 CDRMReceiver 实例 (基类)
    void addMainReceiver(CDRMReceiver* pDRMReceiver, int id)
    {
        // 将传入的裸指针包装为 std::unique_ptr 并存储到 receivers 容器中
        receivers[id] = std::unique_ptr<CDRMReceiver>(pDRMReceiver);
    }


    // 添加一个新的 CSubDRMReceiver 实例 (子类)，并通过构造函数传参
    void addSubReceiver(const std::string& sourceIP,
                        const std::string& localIP,
                        int port, int id,
                        CSettings& settings
                        ) //CDRMTransmitter& transmitter
    {
        auto subReceiver = std::make_unique<CSubDRMReceiver>(&settings);
        // 设置子类的特殊成员
        subReceiver->sourceIP = sourceIP;
        subReceiver->localIP = localIP;
        subReceiver->localPort = port;
        //它封装了一个指针。你可以通过 get() 方法获取内部指针;  打印实例的地址
        // SubDRMReceiver 地址: 0x55c21c358e60
        //        std::cout << "SubDRMReceiver 地址: "
        //                  << static_cast<void*>(subReceiver.get()) << std::endl;

        receivers[id] = std::move(subReceiver); // 存储子类到容器中
    }

    // 添加 CMainDRMReceiver 实例（子类）
    //const std::string& sourceIP, const std::string& localIP, int port,

    void addMainReceiver( int id)
    {
        auto mainReceiver = std::make_unique<CMainDRMReceiver>();
        //        mainReceiver->sourceIP = sourceIP;
        //        mainReceiver->localIP = localIP;
        //        mainReceiver->localPort = port;
        receivers[id] = std::move(mainReceiver);
    }

    void addMainReceiver(CMainDRMReceiver* mainDRMReceiver, int id)
    {
        // 将传入的裸指针包装为 std::unique_ptr 并存储到 receivers 容器中
        receivers[id] = std::unique_ptr<CDRMReceiver>(mainDRMReceiver);
    }

    // 删除指定 ID 的接收器实例
    void removeReceiver(int id)
    {
        auto it = receivers.find(id);
        if (it != receivers.end()) {
            receivers.erase(it); // 从容器中移除
        } else {
            std::cerr << "Receiver with ID " << id << " not found!" << std::endl;
        }
    }

    // 检索指定 ID 的接收器实例（父类指针）
    CDRMReceiver* getReceiver(int id)
    {
        auto it = receivers.find(id);
        if (it != receivers.end()) {
            return it->second.get(); // 返回指向 CDRMReceiver 实例的指针
        } else {
            std::cerr << "Receiver with ID " << id << " not found!" << std::endl;
            return nullptr;
        }
    }

    // 检索指定 ID 的 CSubDRMReceiver 实例（子类）
    CSubDRMReceiver* getSubReceiver(int id)
    {
        auto it = receivers.find(id);
        if (it != receivers.end())
        {
            return dynamic_cast<CSubDRMReceiver*>(it->second.get());
        }
        else
        {
            std::cerr << "Receiver with ID " << id << " not found!" << std::endl;
            return nullptr;
        }
    }

    /* 检索指定 ID 的接收器实例，并检查是否为子类
     *   getSubReceiver 的目的是在 receivers 容器中通过给定的 ID 检索出某个接收器对象，
     *   并检查该接收器对象是否是 CSubDRMReceiver 类型（即子类对象）
     *   dynamic_cast：dynamic_cast 是 C++ 中用于安全类型转换的操作符。
     *   它能够检查一个指针所指向的对象是否是某个特定的派生类类型。
     *   如果类型转换成功（即该对象是 CSubDRMReceiver 类型），则返回有效指针；
     *  如果失败，则返回 nullptr
     */
    CMainDRMReceiver* getMainReceiver(int id)
    {
        auto it = receivers.find(id);
        if (it != receivers.end())
        {
            return dynamic_cast<CMainDRMReceiver*>(it->second.get());
        }
        else
        {
            std::cerr << "Receiver with ID " << id << " not found!" << std::endl;
            return nullptr;
        }
    }

    // 打印所有接收器的信息（用于调试）
    void printReceivers() const
    {
        for (const auto& pair : receivers)
        {
            // 打印索引号
            std::cout << "Receiver ID: " << pair.first << std::endl;
        }
    }

    std::vector<CSubDRMReceiver*> LoopReceivers();

private:
    // 使用 std::map 管理基类指针，支持多态
    std::map<int, std::unique_ptr<CDRMReceiver>> receivers;
};





#endif // RECEIVERMANAGER_H
