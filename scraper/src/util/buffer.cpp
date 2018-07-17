#include "buffer.h"

DynamicBuffer::DynamicBuffer()
{
    this->bufferSize = 1;
    this->bufferLength = 0;
    this->p_Buffer = new char[bufferSize];
    this->p_Buffer[0] = '\0';
    this->maxSize = 0;
}

DynamicBuffer::DynamicBuffer(size_t size)
{
    this->bufferSize = size+1;
    this->bufferLength = 0;
    this->p_Buffer = new char[bufferSize];
    this->p_Buffer[bufferLength] = '\0';
    this->maxSize = 0;
}

DynamicBuffer::DynamicBuffer(const std::string &str)
{
    this->bufferSize = str.size();
    this->bufferLength = str.size();
    this->p_Buffer = new char[bufferSize];
    std::strcpy(p_Buffer, str.c_str());
}

DynamicBuffer::~DynamicBuffer()
{
    delete []this->p_Buffer;
}

size_t DynamicBuffer::setMax(size_t max)
{
    if (max < this->size())
    {
        std::cerr << "Cannot set maximum size of buffer lower than its size" << std::endl;
        return this->size();
    }
    this->maxSize = max;
    return max;
}

size_t DynamicBuffer::enlarge(size_t newSize)
{
    if (maxSize && newSize >= maxSize)
    {
        std::cerr << "Cannot extend buffer! Maximum size exceeded" << std::endl;
        return 0;
    }
    if (newSize <= bufferSize)
    {
        std::cerr << "Given size is less or equal to current size" << std::endl;
        return 0;
    }
    bufferSize = newSize + 1;
    char* newBuffer = new char[bufferSize];
    std::strcpy(newBuffer, p_Buffer);
    delete []p_Buffer;
    p_Buffer = newBuffer;
    return bufferSize;
}

int DynamicBuffer::write(char c)
{
    if (maxSize && bufferLength >= maxSize)
    {
        std::cerr << "Cannot extend buffer! Maximum size reached" << std::endl;
        return -1;
    }

    if (++bufferLength >= bufferSize)
    {
        bufferSize = bufferLength + 1;
        char* newBuffer = new char[bufferSize];
        std::strcpy(newBuffer, p_Buffer);
        delete []p_Buffer;
        p_Buffer = newBuffer;
    }
    p_Buffer[bufferLength-1] = c;
    p_Buffer[bufferLength] = '\0';
    return bufferLength;
}

int DynamicBuffer::write(std::string str)
{
    size_t newLength = str.size() + bufferLength;
    if (maxSize && newLength >= maxSize)
    {
        std::cerr << "Cannot extend buffer! Maximum size reached." << std::endl;
        return -1;
    }

    if (newLength > bufferSize)
    {
        bufferSize = newLength + 1;
        char* newBuffer = new char[bufferSize];
        std::strcpy(newBuffer, p_Buffer);
        std::strcat(newBuffer, str.c_str());
        delete []p_Buffer;
        p_Buffer = newBuffer;
    }
    else
        std::strcat(p_Buffer, str.c_str());

    bufferLength = newLength;
    return bufferLength;
}

void DynamicBuffer::flush()
{
    bufferLength = 0;
    p_Buffer[0] = '\0';
}
