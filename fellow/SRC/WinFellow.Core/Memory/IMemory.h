#pragma once

#include <cstdint>

typedef uint16_t (*memoryIoReadFunc)(uint32_t address);
typedef void (*memoryIoWriteFunc)(uint16_t data, uint32_t address);

class IMemory
{
public:
  virtual void SetIoWriteStub(uint32_t index, memoryIoWriteFunc iowritefunction) = 0;
  virtual void SetIoReadStub(uint32_t index, memoryIoReadFunc ioreadfunction) = 0;
};
