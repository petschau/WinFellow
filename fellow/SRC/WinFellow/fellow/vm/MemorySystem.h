#ifndef FELLOW_VM_MEMORYSYSTEM_H
#define FELLOW_VM_MEMORYSYSTEM_H

#include "fellow/api/vm/IMemorySystem.h"

namespace fellow::vm
{
  class MemorySystem : public fellow::api::vm::IMemorySystem
  {
  public:
    UBY ReadByte(uint32_t address) override;
    UWO ReadWord(uint32_t address) override;
    uint32_t ReadLong(uint32_t address) override;
    void WriteByte(UBY data, uint32_t address) override;
    void WriteWord(UWO data, uint32_t address) override;
    void WriteLong(uint32_t data, uint32_t address) override;

    void DmemSetByte(UBY data) override;
    void DmemSetWord(UWO data) override;
    void DmemSetLong(uint32_t data) override;
    void DmemSetLongNoCounter(uint32_t data, uint32_t offset) override;
    void DmemSetString(const STR *data) override;
    void DmemSetCounter(uint32_t val) override;
    uint32_t DmemGetCounter() override;
    uint32_t DmemGetCounterWithoutOffset() override;
    void DmemClear() override;

    void EmemClear() override;
    void EmemSet(uint32_t index, uint32_t data) override;
    void EmemCardAdd(fellow::api::vm::EmemCardInitFunc cardinit, fellow::api::vm::EmemCardMapFunc cardmap) override;
    void EmemMirror(uint32_t emem_offset, UBY *src, uint32_t size) override;

    void BankSet(
      fellow::api::vm::ReadByteFunc rb,
      fellow::api::vm::ReadWordFunc rw,
      fellow::api::vm::ReadLongFunc rl,
      fellow::api::vm::WriteByteFunc wb,
      fellow::api::vm::WriteWordFunc ww,
      fellow::api::vm::WriteLongFunc wl,
      UBY *basep,
      uint32_t bank,
      uint32_t basebank,
      BOOLE pointer_can_write) override;

    UBY *AddressToPtr(uint32_t address) override;
    uint32_t GetKickImageVersion() override;

    MemorySystem();
  };
}

#endif
