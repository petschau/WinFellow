#pragma once
#include <functional>

#include "fellow/api/defs.h"
#include "fellow/api/vm/MemorySystemTypes.h"

class ChipsetBankHandler
{
private:
  std::function<uint16_t(uint32_t)> _iobank_read[257]{};
  std::function<void(uint16_t, uint32_t)> _iobank_write[257]{};

  static UWO ReadDefault(ULO address);
  static void WriteDefault(UWO data, ULO address);

public:
  void SetIoReadFunction(ULO index, std::function<uint16_t(uint32_t)> ioreadfunction);
  void SetIoWriteFunction(ULO index, std::function<void(uint16_t, uint32_t)> iowritefunction);
  void SetIoReadStub(ULO index, fellow::api::vm::IoReadFunc ioreadfunction);
  void SetIoWriteStub(ULO index, fellow::api::vm::IoWriteFunc iowritefunction);

  UBY ReadByte(ULO address);
  UWO ReadWord(ULO address);
  ULO ReadLong(ULO address);
  void WriteByte(UBY data, ULO address);
  void WriteWord(UWO data, ULO address);
  void WriteLong(ULO data, ULO address);

  void Clear();
};
