#pragma once

#include <vector>
#include "fellow/api/vm/MemorySystemTypes.h"

namespace fellow::api::vm
{
  class IMemorySystem
  {
  public:
    virtual uint8_t ReadByte(uint32_t address) = 0;
    virtual uint16_t ReadWord(uint32_t address) = 0;
    virtual uint32_t ReadLong(uint32_t address) = 0;
    virtual void WriteByte(uint8_t data, uint32_t address) = 0;
    virtual void WriteWord(uint16_t data, uint32_t address) = 0;
    virtual void WriteLong(uint32_t data, uint32_t address) = 0;

    virtual void DmemSetByte(uint8_t data) = 0;
    virtual void DmemSetWord(uint16_t data) = 0;
    virtual void DmemSetLong(uint32_t data) = 0;
    virtual void DmemSetLongNoCounter(uint32_t data, uint32_t offset) = 0;
    virtual void DmemSetString(const char *data) = 0;
    virtual void DmemSetCounter(uint32_t val) = 0;
    virtual uint32_t DmemGetCounter() = 0;
    virtual uint32_t DmemGetCounterWithoutOffset() = 0;
    virtual void DmemClear() = 0;

    virtual void EmemClear() = 0;
    virtual void EmemSet(uint32_t index, uint32_t data) = 0;
    virtual void EmemCardAdd(EmemCardInitFunc cardinit, EmemCardMapFunc cardmap) = 0;
    virtual void EmemMirror(uint32_t emem_offset, uint8_t *src, uint32_t size) = 0;

    virtual void BankSet(const MemoryBankDescriptor memoryBankDescriptor) = 0;

    virtual uint8_t *AddressToPtr(uint32_t address) = 0;
    virtual uint32_t GetKickImageVersion() = 0;

    virtual bool GetAddressSpace32Bits() = 0;
    virtual std::vector<MemoryMapDescriptor> GetMemoryMapDescriptors() = 0;
  };
}
