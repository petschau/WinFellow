#pragma once
#include "fellow/api/defs.h"
#include "fellow/chipset/Cia.h"

class CiaBankHandler
{
private:
  Cia *_cia{};

public:
  UBY ReadByte(ULO address);
  UWO ReadWord(ULO address);
  ULO ReadLong(ULO address);
  void WriteByte(UBY data, ULO address);
  void WriteWord(UWO data, ULO address);
  void WriteLong(ULO data, ULO address);

  void Map();

  CiaBankHandler(Cia *cia);
};
