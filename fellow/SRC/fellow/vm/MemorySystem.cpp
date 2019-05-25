#include "fellow/vm/MemorySystem.h"
#include "FMEM.H"

using namespace fellow::api::vm;

namespace fellow::vm
{
  UBY MemorySystem::ReadByte(ULO address)
  {
    return memoryReadByte(address);
  }

  UWO MemorySystem::ReadWord(ULO address)
  {
    return memoryReadWord(address);
  }

  ULO MemorySystem::ReadLong(ULO address)
  {
    return memoryReadLong(address);
  }

  void MemorySystem::WriteByte(UBY data, ULO address)
  {
    memoryWriteByte(data, address);
  }

  void MemorySystem::WriteWord(UWO data, ULO address)
  {
    memoryWriteWord(data, address);
  }

  void MemorySystem::WriteLong(ULO data, ULO address)
  {
    memoryWriteLong(data, address);
  }

  void MemorySystem::DmemSetByte(UBY data)
  {
    memoryDmemSetByte(data);
  }

  void MemorySystem::DmemSetWord(UWO data)
  {
    memoryDmemSetWord(data);
  }

  void MemorySystem::DmemSetLong(ULO data)
  {
    memoryDmemSetLong(data);
  }

  void MemorySystem::DmemSetLongNoCounter(ULO data, ULO offset)
  {
    memoryDmemSetLongNoCounter(data, offset);
  }

  void MemorySystem::DmemSetString(const STR *data)
  {
    memoryDmemSetString(data);
  }

  void MemorySystem::DmemSetCounter(ULO val)
  {
    memoryDmemSetCounter(val);
  }

  ULO MemorySystem::DmemGetCounter()
  {
    return memoryDmemGetCounter();
  }

  ULO MemorySystem::DmemGetCounterWithoutOffset()
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

  void MemorySystem::EmemSet(ULO index, ULO data)
  {
    memoryEmemSet(index, data);
  }

  void MemorySystem::EmemCardAdd(EmemCardInitFunc cardinit, EmemCardMapFunc cardmap)
  {
    memoryEmemCardAdd(cardinit, cardmap);
  }

  void MemorySystem::EmemMirror(ULO emem_offset, UBY *src, ULO size)
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
    UBY *basep,
    ULO bank,
    ULO basebank,
    BOOLE pointer_can_write)
  {
    memoryBankSet(rb, rw, rl, wb, ww, wl, basep, bank, basebank, pointer_can_write);
  }

  UBY *MemorySystem::AddressToPtr(ULO address)
  {
    return memoryAddressToPtr(address);
  }

  ULO MemorySystem::GetKickImageVersion()
  {
    return memoryGetKickImageVersion();
  }

  MemorySystem::MemorySystem()
  {
  }
}
