#pragma once

#include "fellow/api/defs.h"

namespace fellow::api::vm
{
  typedef void(*EmemCardInitFunc)();
  typedef void(*EmemCardMapFunc)(uint32_t);

  typedef uint8_t(*ReadByteFunc)(uint32_t);
  typedef uint16_t(*ReadWordFunc)(uint32_t);
  typedef uint32_t(*ReadLongFunc)(uint32_t);
  typedef void(*WriteByteFunc)(uint8_t, uint32_t);
  typedef void(*WriteWordFunc)(uint16_t, uint32_t);
  typedef void(*WriteLongFunc)(uint32_t, uint32_t);

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

    virtual void BankSet(
      ReadByteFunc rb,
      ReadWordFunc rw,
      ReadLongFunc rl,
      WriteByteFunc wb,
      WriteWordFunc ww,
      WriteLongFunc wl,
      uint8_t *basep,
      uint32_t bank,
      uint32_t basebank,
      BOOLE pointer_can_write) = 0;

    virtual uint8_t *AddressToPtr(uint32_t address) = 0;
    virtual uint32_t GetKickImageVersion() = 0;
  };
}
