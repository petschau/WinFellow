#pragma once

#include "fellow/api/vm/IMemorySystem.h"

namespace fellow::vm
{
  class MemorySystem : public fellow::api::vm::IMemorySystem
  {
  public:
    uint8_t ReadByte(uint32_t address) override;
    uint16_t ReadWord(uint32_t address) override;
    uint32_t ReadLong(uint32_t address) override;
    void WriteByte(uint8_t data, uint32_t address) override;
    void WriteWord(uint16_t data, uint32_t address) override;
    void WriteLong(uint32_t data, uint32_t address) override;

    void DmemSetByte(uint8_t data) override;
    void DmemSetWord(uint16_t data) override;
    void DmemSetLong(uint32_t data) override;
    void DmemSetLongNoCounter(uint32_t data, uint32_t offset) override;
    void DmemSetString(const char *data) override;
    void DmemSetCounter(uint32_t val) override;
    uint32_t DmemGetCounter() override;
    uint32_t DmemGetCounterWithoutOffset() override;
    void DmemClear() override;

    void EmemClear() override;
    void EmemSet(uint32_t index, uint32_t data) override;
    void EmemCardAdd(fellow::api::vm::EmemCardInitFunc cardinit, fellow::api::vm::EmemCardMapFunc cardmap) override;
    void EmemMirror(uint32_t emem_offset, uint8_t *src, uint32_t size) override;

    void BankSet(
      fellow::api::vm::ReadByteFunc rb,
      fellow::api::vm::ReadWordFunc rw,
      fellow::api::vm::ReadLongFunc rl,
      fellow::api::vm::WriteByteFunc wb,
      fellow::api::vm::WriteWordFunc ww,
      fellow::api::vm::WriteLongFunc wl,
      uint8_t *basep,
      uint32_t bank,
      uint32_t basebank,
      BOOLE pointer_can_write) override;

    uint8_t *AddressToPtr(uint32_t address) override;
    uint32_t GetKickImageVersion() override;

    MemorySystem();
  };
}
