#ifndef CHANNELMANAGER_H
#define CHANNELMANAGER_H

#include <iostream>
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <algorithm>
#include <string>
#include "DrmReceiver.h"

using namespace std;

// 数据包类型定义
using DataPacket = vector<_BINARY>;

//using DataPacket = CSingleBuffer<_BINARY>;

// 数据源 基类接口，代表一个通用的数据源
class IDataSource {
public:
    virtual ~IDataSource() = default;

    // 添加一个通道
    virtual void addChannel(int channelId, const DataPacket& data) = 0;

    // 移除一个通道
    virtual void removeChannel(int channelId) = 0;

    // 获取通道数据
    virtual bool getChannelData(int channelId, DataPacket& data) const = 0;

    // 实时更新通道数据
    virtual void updateChannelData(int channelId, const DataPacket& data) = 0;

    // 获取数据源的访问信息
    virtual string getAccessInfo() const = 0;

    // 获取通道当前数据长度
    virtual size_t getCurrentDataLength(int channelId) const = 0;

    // 获取通道总的数据容量大小
    virtual size_t getTotalCapacity(int channelId) const = 0;

    //TODO 实现获取通道数据的逻辑
    virtual std::vector<_BINARY> getChannelData(int channelId) const = 0;
};

// IP流数据源类
class IPStreamDataSource : public IDataSource {
public:
    IPStreamDataSource(const string& ip) : ipAddress(ip) {}

    //添加信道
    void addChannel(int channelId, const DataPacket& data) override {
        lock_guard<mutex> guard(mtx);
        channels[channelId] = data;
    }

    void removeChannel(int channelId) override {
        lock_guard<mutex> guard(mtx);
        channels.erase(channelId);
    }

    bool getChannelData(int channelId, DataPacket& data) const override {
        lock_guard<mutex> guard(mtx);
        auto it = channels.find(channelId);
        if (it != channels.end()) {
            data = it->second;
            return true;
        }
        return false;
    }

    void updateChannelData(int channelId, const DataPacket& data) override {
        lock_guard<mutex> guard(mtx);
        channels[channelId] = data;
    }

    string getAccessInfo() const override {
        return  ipAddress;
    }

    size_t getCurrentDataLength(int channelId) const override {
        lock_guard<mutex> guard(mtx);
        auto it = channels.find(channelId);
        if (it != channels.end())
        {
            // return it->second.size();

        }
        return 0;
    }

    size_t getTotalCapacity(int channelId) const override {
        lock_guard<mutex> guard(mtx);
        auto it = channels.find(channelId);
        if (it != channels.end())
        {

            return it->second.capacity();

        }
        return 0;
    }

    DataPacket getChannelData(int channelId) const override
    {
        lock_guard<mutex> guard(mtx);
        auto it = channels.find(channelId);
        if (it != channels.end())
        {
            return it->second;
        }
        else
        {
            std::cout<<" Channel ID not found "<<std::endl;
        }

        return DataPacket();    // 不太好 size = 0
    }
    CDRMReceiver m_DRMReceiver;    //父类 接收机 处理接下来的解码流程
private:
    string ipAddress;                  // 唯一标识
    map<int, DataPacket> channels;    // 1. 下标索引, 2. 存储流服务数据
    mutable mutex mtx;
};

// 硬件接收设备数据源类
class HardwareDeviceDataSource : public IDataSource {
public:
    HardwareDeviceDataSource(const string& driver) : driverName(driver) {}

    void addChannel(int channelId, const DataPacket& data) override {
        lock_guard<mutex> guard(mtx);
        channels[channelId] = data;
    }

    void removeChannel(int channelId) override {
        lock_guard<mutex> guard(mtx);
        channels.erase(channelId);
    }

    bool getChannelData(int channelId, DataPacket& data) const override {
        lock_guard<mutex> guard(mtx);
        auto it = channels.find(channelId);
        if (it != channels.end()) {
            data = it->second;
            return true;
        }
        return false;
    }

    void updateChannelData(int channelId, const DataPacket& data) override {
        lock_guard<mutex> guard(mtx);
        channels[channelId] = data;
    }

    string getAccessInfo() const override {
        return "Driver: " + driverName;
    }

    size_t getCurrentDataLength(int channelId) const override {
        lock_guard<mutex> guard(mtx);
        auto it = channels.find(channelId);
        if (it != channels.end()) {
            return it->second.size();
        }
        return 0;
    }

    size_t getTotalCapacity(int channelId) const override {
        lock_guard<mutex> guard(mtx);
        auto it = channels.find(channelId);
        if (it != channels.end()) {
            return it->second.capacity();
        }
        return 0;
    }

private:
    string driverName;
    map<int, DataPacket> channels;
    mutable mutex mtx;
};

// 本地文件数据源类
class LocalFileDataSource : public IDataSource {
public:
    LocalFileDataSource(const string& path) : filePath(path) {}

    void addChannel(int channelId, const DataPacket& data) override {
        lock_guard<mutex> guard(mtx);
        channels[channelId] = data;
    }

    void removeChannel(int channelId) override {
        lock_guard<mutex> guard(mtx);
        channels.erase(channelId);
    }

    bool getChannelData(int channelId, DataPacket& data) const override {
        lock_guard<mutex> guard(mtx);
        auto it = channels.find(channelId);
        if (it != channels.end()) {
            data = it->second;
            return true;
        }
        return false;
    }

    void updateChannelData(int channelId, const DataPacket& data) override {
        lock_guard<mutex> guard(mtx);
        channels[channelId] = data;
    }

    string getAccessInfo() const override {
        return "File Path: " + filePath;
    }

    size_t getCurrentDataLength(int channelId) const override {
        lock_guard<mutex> guard(mtx);
        auto it = channels.find(channelId);
        if (it != channels.end()) {
            return it->second.size();
        }
        return 0;
    }

    size_t getTotalCapacity(int channelId) const override {
        lock_guard<mutex> guard(mtx);
        auto it = channels.find(channelId);
        if (it != channels.end()) {
            return it->second.capacity();
        }
        return 0;
    }

private:
    string filePath;
    map<int, DataPacket> channels;
    mutable mutex mtx;
};

// 声音信号数据源类
class AudioSignalDataSource : public IDataSource {
public:
    AudioSignalDataSource(const string& device) : deviceName(device) {}

    void addChannel(int channelId, const DataPacket& data) override {
        lock_guard<mutex> guard(mtx);
        channels[channelId] = data;
    }

    void removeChannel(int channelId) override {
        lock_guard<mutex> guard(mtx);
        channels.erase(channelId);
    }

    bool getChannelData(int channelId, DataPacket& data) const override {
        lock_guard<mutex> guard(mtx);
        auto it = channels.find(channelId);
        if (it != channels.end()) {
            data = it->second;
            return true;
        }
        return false;
    }

    void updateChannelData(int channelId, const DataPacket& data) override {
        lock_guard<mutex> guard(mtx);
        channels[channelId] = data;
    }

    string getAccessInfo() const override {
        return "Device: " + deviceName;
    }

    size_t getCurrentDataLength(int channelId) const override {
        lock_guard<mutex> guard(mtx);
        auto it = channels.find(channelId);
        if (it != channels.end()) {
            return it->second.size();
        }
        return 0;
    }

    size_t getTotalCapacity(int channelId) const override {
        lock_guard<mutex> guard(mtx);
        auto it = channels.find(channelId);
        if (it != channels.end()) {
            return it->second.capacity();
        }
        return 0;
    }

private:
    string deviceName;
    map<int, DataPacket> channels;
    mutable mutex mtx;
};

// 数据通道复用管理类
class ChannelManager {
public:
    ChannelManager(){}
    ~ChannelManager(){}
    // 添加一个数据源
    void addDataSource(const string& sourceName, shared_ptr<IDataSource> dataSource) {
        lock_guard<mutex> guard(mtx);
        m_dataSources[sourceName] = dataSource; //添加到map 容器中
    }

    // 移除一个数据源
    void removeDataSource(const string& sourceName) {
        lock_guard<mutex> guard(mtx);
        m_dataSources.erase(sourceName);
    }

    // TODO   配置通道映射
    void configureMapping(const map<string, vector<int>>& newMapping) {
        lock_guard<mutex> guard(mtx);
         channelMapping = newMapping;
    }

    // 获取复用后的通道数据
    vector<DataPacket> getMergedData() const
    {
        lock_guard<mutex> guard(mtx);
        vector<DataPacket> mergedData;   //TODO 类型不对

        // 遍历通道映射，合并数据
        for (const auto& sourcePair : channelMapping)
        {
            const auto& sourceName = sourcePair.first;
            const auto& channelIds = sourcePair.second;

            auto sourceIt = m_dataSources.find(sourceName);
            if (sourceIt != m_dataSources.end())
            {
                auto& dataSource = sourceIt->second;
                for (int channelId : channelIds)
                {
                    DataPacket data;
                    if (dataSource->getChannelData(channelId, data))
                    {
                        mergedData.push_back(data);
                    }
                }
            }
        }

        return mergedData;
    }

    // 检查码率限制
    bool checkAndLimitBitrate(size_t maxBitrate) const {
        lock_guard<mutex> guard(mtx);
        size_t totalBitrate = 0;

        for (const auto& sourcePair : channelMapping) {
            const auto& sourceName = sourcePair.first;
            const auto& channelIds = sourcePair.second;

            auto sourceIt = m_dataSources.find(sourceName);
            if (sourceIt != m_dataSources.end()) {
                auto& dataSource = sourceIt->second;
                for (int channelId : channelIds) {
                    totalBitrate += dataSource->getCurrentDataLength(channelId);
                }
            }
        }

        return totalBitrate <= maxBitrate;
    }

    // 打印所有数据源的访问信息
    void printAllAccessInfo() const {
        lock_guard<mutex> guard(mtx);

        for (const auto& sourcePair : m_dataSources) {
            const auto& sourceName = sourcePair.first;
            const auto& dataSource = sourcePair.second;
            cout << sourceName << ": " << dataSource->getAccessInfo() << endl;
        }
    }

    // 根据访问信息查找数据源
    shared_ptr<IDataSource> findDataSourceByAccessInfo(const string& accessInfo) const
    {
        lock_guard<mutex> guard(mtx);

        for (const auto& sourcePair : m_dataSources)
        {
            const auto& dataSource = sourcePair.second;
            // ip 端口 匹配一致
            if (dataSource->getAccessInfo() == accessInfo) {
                return dataSource;
            }
        }

        return nullptr;  // 如果未找到，返回空指针
    }

    //
    void copy_mux_stream(DataPacket& data, CSingleBuffer<_BINARY>& MSCMuxBuf)
    {
        // 输入
        DataPacket::iterator inputIt = data.begin();

        // 输出
        CVectorEx<_BINARY>* OutputMSCMuxBuf =  MSCMuxBuf.QueryWriteBuffer();
        vector<_BINARY>::iterator it_VecOutput = OutputMSCMuxBuf->begin();


        const int inputLen = data.size();
        const int iStreamLen = OutputMSCMuxBuf->Size();

        if (inputLen >= iStreamLen)
        {
            /* Copy data */
            for (int i = 0; i < inputLen ; i++)
            {
                it_VecOutput[i] = inputIt[i];
            }
            // veciOutputBlockSize[i] = iStreamLen;
        }

    }


    //拷贝 复用的服务中的业务
    void copyDataToBuffer(std::vector<CSingleBuffer<_BINARY>>& MSCMuxBuf)
    {
        std::lock_guard<std::mutex> guard(mtx);
        std::map<std::string, std::shared_ptr<IDataSource>>::iterator sourceIt ;
        //根据 遍历映射表:
        for (auto& sourcePair : channelMapping)
        {
            //<string, vector<int>>
            string sourceName = sourcePair.first;
            vector<int> channelIds = sourcePair.second;

            // 找到需要复用 服务流（业务）
            sourceIt = m_dataSources.find(sourceName); //管理源中 Name 是否存在
            if (sourceIt != m_dataSources.end())
            {
                std::shared_ptr<IDataSource>& dataSource = sourceIt->second;  // auto&
                for (int serverId: channelIds)
                {
                    DataPacket data = dataSource->getChannelData(serverId);
                    if (!data.empty())
                    {
                        // 位置
                        //MSCDecBuf.emplace_back(data);
                        copy_mux_stream(data, MSCMuxBuf[0]);

                    }
                }
            }
        }
    }


private:
    map<string, shared_ptr<IDataSource>> m_dataSources;   // 第一个 当成索引使用，第二个是实际数据; (IDataSource基类)
    map<string, vector<int>> channelMapping;               //配置 复用服务业务   int: 服务下标
    mutable mutex mtx;     //如果不加上  mutable  报错  ,不能加锁 释放锁

};

#if 0
int mainChannelManager()
{
    auto ipStreamDataSource = make_shared<IPStreamDataSource>("192.168.0.1");
    ipStreamDataSource->addChannel(1, {'a', 'b', 'c'});
    ipStreamDataSource->addChannel(2, {'d', 'e', 'f'});

    auto hardwareDeviceDataSource = make_shared<HardwareDeviceDataSource>("hw_driver");
    hardwareDeviceDataSource->addChannel(1, {'x', 'y', 'z'});

    auto localFileDataSource = make_shared<LocalFileDataSource>("path/to/file");
    localFileDataSource->addChannel(1, {'1', '2', '3'});
    localFileDataSource->addChannel(2, {'4', '5', '6'});

    auto audioSignalDataSource = make_shared<AudioSignalDataSource>("audio_device");
    audioSignalDataSource->addChannel(1, {'7', '8', '9'});
    audioSignalDataSource->addChannel(2, {'0', 'a', 'b'});

    ChannelManager manager;
    manager.addDataSource("IPStream", ipStreamDataSource);
    manager.addDataSource("HardwareDevice", hardwareDeviceDataSource);
    manager.addDataSource("LocalFile", localFileDataSource);
    manager.addDataSource("AudioSignal", audioSignalDataSource);

    manager.configureMapping({
                                 {"IPStream", {1, 2}},
                                 {"HardwareDevice", {1}},
                                 {"LocalFile", {2}},
                                 {"AudioSignal", {1, 2}}
                             });

    cout << "Merged Data: " << endl;
    auto mergedData = manager.getMergedData();
    for (const auto& data : mergedData) {
        for (char c : data) {
            cout << c << ' ';
        }
        cout << endl;
    }

    cout << "Check Bitrate Limit: " << manager.checkAndLimitBitrate(10) << endl;

    cout << "All Access Info: " << endl;
    manager.printAllAccessInfo();

    return 0;
}
#endif

#endif // CHANNELMANAGER_H
