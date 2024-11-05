#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <arpa/inet.h>    // uint8_t
#include <QIODevice>
#include <iostream>


#define RING_BUFFER_SIZE (76800 * 3)



class RingBuffer : public QIODevice {
public:

    explicit RingBuffer(size_t bufferSize, QObject* parent = nullptr)
        : QIODevice(parent), buffer(bufferSize), head(0), tail(0), size(bufferSize), dataAvailable(0) {
        open(QIODevice::ReadWrite);
    }
    QIODevice* start()
    {
        // Return the ring buffer as the IO device
        return this;
    }
    qint64 readData(char* data, qint64 maxSize) override {
        if (dataAvailable == 0) {
            return 0; // No data to read
        }

        qint64 bytesRead = 0;
        while (bytesRead < maxSize && dataAvailable > 0) {
            data[bytesRead] = buffer[head];
            head = (head + 1) % size;
            bytesRead++;
            dataAvailable--;
        }
        return bytesRead;
    }

    qint64 writeData(const char* data, qint64 maxSize) override {
        qint64 bytesWritten = 0;
        while (bytesWritten < maxSize && dataAvailable < size) {
            buffer[tail] = data[bytesWritten];
            tail = (tail + 1) % size;
            bytesWritten++;
            dataAvailable++;
        }
        return bytesWritten;
    }

    bool isBufferFull() const {
        return dataAvailable == size;
    }

    bool isBufferEmpty() const {
        return dataAvailable == 0;
    }

private:
    std::vector<char> buffer;
    size_t head;
    size_t tail;
    size_t size;
    size_t dataAvailable;
};


#endif // RINGBUFFER_H
