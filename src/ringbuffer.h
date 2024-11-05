#ifndef RINGBUFFER_H
#define RINGBUFFER_H
//new

#include <arpa/inet.h>    // uint8_t
#include <iostream>
#include <mutex>
#include <string.h>

#define RING_BUFFER_SIZE (76800 * 6)

class RingBuffer
{
public:

    RingBuffer();
    ~RingBuffer();

    void    ring_buffer_init();
    int     ring_buffer_read(short* dest, int size);
    int     ring_buffer_write(short* data, int size);
    int     Getsize(){return data_size; }// 已用size
    short*  Getbuff(){return buffer;}


    short buffer[RING_BUFFER_SIZE];
    int write_index;     // 写的 头 head
    int read_index;      // 读取 尾 tail
    int data_size;       // 已用size
    std::mutex Mutex;
private:

};

#endif // RINGBUFFER_H
