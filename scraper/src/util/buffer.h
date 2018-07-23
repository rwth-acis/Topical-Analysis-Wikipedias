#ifndef BUFFER_H
#define BUFFER_H

#include <stdio.h>
#include <iostream>
#include <string.h>
#include <cstring>

class DynamicBuffer
{
    // raw pointer
    char* p_Buffer;
    // if maxSize == 0, no maximum
    size_t maxSize;
    // size of buffer including \0 chararcter
    size_t bufferSize;
    // length of buffer
    size_t bufferLength;

public:
  DynamicBuffer();
  DynamicBuffer(size_t size);
  DynamicBuffer(const std::string &str);
  ~DynamicBuffer();

  // returns size
  size_t size() const { return bufferSize-1; }

  // returns length
  size_t length() const { return bufferLength; }

  // returns raw pointer
  const char* raw() const { return p_Buffer; }

  // check if buffer is full
  bool full() const { return (bufferSize <= bufferLength+1); }

  // get maximum size
  size_t max() const { return maxSize; }

  // set maximum size
  size_t setMax(size_t);

  // enlarge buffer
  size_t enlarge(size_t);

  // flush buffer
  void flush();

  // functions for printing
  std::string print() const { return std::string(p_Buffer); }
  std::string print(size_t offset) const { return offset < bufferLength ? std::string(p_Buffer + offset) : ""; }
  std::string print(size_t from, size_t to);

  // read char at position
  char read(size_t id)
  {
      if (!p_Buffer)
      {
          std::cerr << "Invalid buffer" << std::endl;
          return -1;
      }

      if (id >= bufferLength)
      {
          std::cerr << "Index out of bounds" << std::endl;
          return -2;
      }

      return p_Buffer[id];
  }

  // index operator
  char operator [](size_t id)
  {
      if (!p_Buffer)
      {
          std::cerr << "Invalid buffer" << std::endl;
          return -1;
      }

      if (id >= bufferLength)
      {
          std::cerr << "Index out of bounds" << std::endl;
          return -2;
      }

      return p_Buffer[id];
  }

  // functions for writing to buffer
  int write(char c);
  int write(std::string);
};
#endif // BUFFER_H
