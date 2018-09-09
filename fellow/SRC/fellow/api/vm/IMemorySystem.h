#ifndef FELLOW_API_VM_IMEMORYSYSTEM_H
#define FELLOW_API_VM_IMEMORYSYSTEM_H

#include "fellow/api/defs.h"

namespace fellow::api::vm
{
  typedef void(*EmemCardInitFunc)();
  typedef void(*EmemCardMapFunc)(ULO);

  typedef UBY(*ReadByteFunc)(ULO);
  typedef UWO(*ReadWordFunc)(ULO);
  typedef ULO(*ReadLongFunc)(ULO);
  typedef void(*WriteByteFunc)(UBY, ULO);
  typedef void(*WriteWordFunc)(UWO, ULO);
  typedef void(*WriteLongFunc)(ULO, ULO);

  class IMemorySystem
  {
  public:
    virtual UBY ReadByte(ULO address) = 0;
    virtual UWO ReadWord(ULO address) = 0;
    virtual ULO ReadLong(ULO address) = 0;
    virtual void WriteByte(UBY data, ULO address) = 0;
    virtual void WriteWord(UWO data, ULO address) = 0;
    virtual void WriteLong(ULO data, ULO address) = 0;

    virtual void DmemSetByte(UBY data) = 0;
    virtual void DmemSetWord(UWO data) = 0;
    virtual void DmemSetLong(ULO data) = 0;
    virtual void DmemSetLongNoCounter(ULO data, ULO offset) = 0;
    virtual void DmemSetString(const STR *data) = 0;
    virtual void DmemSetCounter(ULO val) = 0;
    virtual ULO DmemGetCounter() = 0;
    virtual void DmemClear() = 0;

    virtual void EmemClear() = 0;
    virtual void EmemSet(ULO index, ULO data) = 0;
    virtual void EmemCardAdd(EmemCardInitFunc cardinit, EmemCardMapFunc cardmap) = 0;
    virtual void EmemMirror(ULO emem_offset, UBY *src, ULO size) = 0;

    virtual void BankSet(
      ReadByteFunc rb,
      ReadWordFunc rw,
      ReadLongFunc rl,
      WriteByteFunc wb,
      WriteWordFunc ww,
      WriteLongFunc wl,
      UBY *basep,
      ULO bank,
      ULO basebank,
      BOOLE pointer_can_write) = 0;

    virtual UBY *AddressToPtr(ULO address) = 0;
    virtual ULO GetKickImageVersion() = 0;
  };
}

#endif
