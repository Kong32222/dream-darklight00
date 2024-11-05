#include "ringbuffer.h"

//new


RingBuffer::RingBuffer()
{

}

RingBuffer::~RingBuffer()
{

}

void RingBuffer::ring_buffer_init()
{

    memset(this->buffer, 0, sizeof(short) * RING_BUFFER_SIZE);
    this->write_index = 0;
    this->read_index = 0;
    this->data_size = 0;
}

int RingBuffer::ring_buffer_read(short *dest, int size)
{

    if (size > (this->data_size))
    {
        /*
         * std::cout<<"(this->data_size数据不足 ) = "<< this <<"; "
         * <<(this->data_size) << std::endl;
            */

        return -1;  // 数据不足
    }

    std::lock_guard<std::mutex> lock(Mutex);  //加锁
    for (int i = 0; i < size; i++)
    {
        dest[i] = this->buffer[this->read_index];
        this->read_index = (this->read_index + 1) % (RING_BUFFER_SIZE);
    }

    this->data_size -= size;


    return size;
}

int RingBuffer::ring_buffer_write(short *data, int size)
{
    // 38400
    if (size > ((RING_BUFFER_SIZE) -(this->data_size)))
    {
        // std::cout<<"环形缓冲区 写不下 ; this->data_size = "<<this->data_size<<std::endl;
        return -1;  // 环形缓冲区 写不下
    }
    std::lock_guard<std::mutex> lock(Mutex);

    for (int i = 0; i < size; i++)
    {
        this->buffer[this->write_index] = data[i];
        this->write_index = (this->write_index + 1) % (RING_BUFFER_SIZE);
    }
    this->data_size += size;

    return size;
}
