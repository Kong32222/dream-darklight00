#include "receivermanager.h"


std::vector<CSubDRMReceiver*> ReceiverManager::LoopReceivers()
{
    //C++11 及以上版本：支持 移动语义，当返回局部 std::vector 时，
    // 编译器会自动优化为移动操作，而不是拷贝，从而提高性能。

    // 创建一个 vector 来保存指向 CSubDRMReceiver 的指针
    std::vector<CSubDRMReceiver*> receiverPointers;

    // 遍历所有的接收器
    for (const auto& pair : receivers)
    {
        // 打印索引号
        // std::cout << "Receiver ID: " << pair.first << std::endl;

        // 获取 CSubDRMReceiver* 指针
        CSubDRMReceiver* temp = getSubReceiver(pair.first);
        if (temp != nullptr)
        {
            // 将指针保存到 vector 中
            receiverPointers.push_back(temp);
        }
    }

    // 返回指针 vector
    return receiverPointers;
}
