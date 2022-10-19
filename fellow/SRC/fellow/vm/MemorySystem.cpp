#include "fellow/vm/MemorySystem.h"
#include "fellow/memory/Memory.h"

using namespace fellow::api::vm;
using namespace std;

namespace fellow::vm
{
  uint8_t MemorySystem::ReadByte(uint32_t address)
  {
    return memoryReadByte(address);
  }

  uint16_t MemorySystem::ReadWord(uint32_t address)
  {
    return memoryReadWord(address);
  }

  uint32_t MemorySystem::ReadLong(uint32_t address)
  {
    return memoryReadLong(address);
  }

  void MemorySystem::WriteByte(uint8_t data, uint32_t address)
  {
    memoryWriteByte(data, address);
  }

  void MemorySystem::WriteWord(uint16_t data, uint32_t address)
  {
    memoryWriteWord(data, address);
  }

  void MemorySystem::WriteLong(uint32_t data, uint32_t address)
  {
    memoryWriteLong(data, address);
  }

  void MemorySystem::DmemSetByte(uint8_t data)
  {
    memoryDmemSetByte(data);
  }

  void MemorySystem::DmemSetWord(uint16_t data)
  {
    memoryDmemSetWord(data);
  }

  void MemorySystem::DmemSetLong(uint32_t data)
  {
    memoryDmemSetLong(data);
  }

  void MemorySystem::DmemSetLongNoCounter(uint32_t data, uint32_t offset)
  {
    memoryDmemSetLongNoCounter(data, offset);
  }

  void MemorySystem::DmemSetString(const char *data)
  {
    memoryDmemSetString(data);
  }

  void MemorySystem::DmemSetCounter(uint32_t val)
  {
    memoryDmemSetCounter(val);
  }

  uint32_t MemorySystem::DmemGetCounter()
  {
    return memoryDmemGetCounter();
  }

  uint32_t MemorySystem::DmemGetCounterWithoutOffset()
  {
    return memoryDmemGetCounterWithoutOffset();
  }

  void MemorySystem::DmemClear()
  {
    memoryDmemClear();
  }

  void MemorySystem::EmemClear()
  {
    memoryEmemClear();
  }

  void MemorySystem::EmemSet(uint32_t index, uint32_t data)
  {
    memoryEmemSet(index, data);
  }

  void MemorySystem::EmemCardAdd(EmemCardInitFunc cardinit, EmemCardMapFunc cardmap)
  {
    memoryEmemCardAdd(cardinit, cardmap);
  }

  void MemorySystem::EmemMirror(uint32_t emem_offset, uint8_t *src, uint32_t size)
  {
    memoryEmemMirror(emem_offset, src, size);
  }

  void MemorySystem::BankSet(const MemoryBankDescriptor memoryBankDescriptor)
  {
    memoryBankSet(memoryBankDescriptor);
  }

  uint8_t *MemorySystem::AddressToPtr(uint32_t address)
  {
    return memoryAddressToPtr(address);
  }

  uint32_t MemorySystem::GetKickImageVersion()
  {
    return memoryGetKickImageVersion();
  }

  bool MemorySystem::GetAddressSpace32Bits()
  {
    return memoryGetAddressSpace32Bit();
  }

  vector<MemoryMapDescriptor> MemorySystem::GetMemoryMapDescriptors()
  {
    auto descriptors = memoryGetMemoryMapDescriptors();

    vector<MemoryMapDescriptor> result;

    for (const auto &descriptor : descriptors)
    {
      result.emplace_back(MemoryMapDescriptor{.Kind = descriptor.Kind, .StartAddress = descriptor.StartAddress, .EndAddress = descriptor.EndAddress});
    }

    return result;
  }
}
