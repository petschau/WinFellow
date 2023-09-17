#include "fellow/vm/MemorySystem.h"
#include "FMEM.H"

using namespace fellow::api::vm;

namespace fellow::vm
{
  uint8_t MemorySystem::ReadByte(uint32_t address)
  {
    return memoryReadByte(address);
  }

  UWO MemorySystem::ReadWord(uint32_t address)
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

  void MemorySystem::WriteWord(UWO data, uint32_t address)
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

  void MemorySystem::DmemSetWord(UWO data)
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

  void MemorySystem::DmemSetString(const STR *data)
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

  void MemorySystem::BankSet(
    ReadByteFunc rb,
    ReadWordFunc rw,
    ReadLongFunc rl,
    WriteByteFunc wb,
    WriteWordFunc ww,
    WriteLongFunc wl,
    uint8_t *basep,
    uint32_t bank,
    uint32_t basebank,
    BOOLE pointer_can_write)
  {
    memoryBankSet(rb, rw, rl, wb, ww, wl, basep, bank, basebank, pointer_can_write);
  }

  uint8_t *MemorySystem::AddressToPtr(uint32_t address)
  {
    return memoryAddressToPtr(address);
  }

  uint32_t MemorySystem::GetKickImageVersion()
  {
    return memoryGetKickImageVersion();
  }

  MemorySystem::MemorySystem()
  {
  }
}
