#ifndef FELLOW_VM_MEMORYSYSTEM_H
#define FELLOW_VM_MEMORYSYSTEM_H

#include "fellow/api/vm/IMemorySystem.h"

namespace fellow::vm
{
  class MemorySystem : public fellow::api::vm::IMemorySystem
  {
  public:
    UBY ReadByte(ULO address) override;
    UWO ReadWord(ULO address) override;
    ULO ReadLong(ULO address) override;
    void WriteByte(UBY data, ULO address) override;
    void WriteWord(UWO data, ULO address) override;
    void WriteLong(ULO data, ULO address) override;

    void DmemSetByte(UBY data) override;
    void DmemSetWord(UWO data) override;
    void DmemSetLong(ULO data) override;
    void DmemSetLongNoCounter(ULO data, ULO offset) override;
    void DmemSetString(const STR *data) override;
    void DmemSetCounter(ULO val) override;
    ULO DmemGetCounter() override;
    ULO DmemGetCounterWithoutOffset() override;
    void DmemClear() override;

    void EmemClear() override;
    void EmemSet(ULO index, ULO data) override;
    void EmemCardAdd(fellow::api::vm::EmemCardInitFunc cardinit, fellow::api::vm::EmemCardMapFunc cardmap) override;
    void EmemMirror(ULO emem_offset, UBY *src, ULO size) override;

    void BankSet(
      fellow::api::vm::ReadByteFunc rb,
      fellow::api::vm::ReadWordFunc rw,
      fellow::api::vm::ReadLongFunc rl,
      fellow::api::vm::WriteByteFunc wb,
      fellow::api::vm::WriteWordFunc ww,
      fellow::api::vm::WriteLongFunc wl,
      UBY *basep,
      ULO bank,
      ULO basebank,
      BOOLE pointer_can_write) override;

    UBY *AddressToPtr(ULO address) override;
    ULO GetKickImageVersion() override;

    MemorySystem();
  };
}

#endif
