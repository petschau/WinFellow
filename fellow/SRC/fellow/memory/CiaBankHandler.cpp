#include "CiaBankHandler.h"

#include "fellow/memory/Memory.h"
#include "fellow/api/vm/MemorySystemTypes.h"

UBY CiaBankHandler::ReadByte(ULO address)
{
  return _cia->ReadByte(address);
}

UWO CiaBankHandler::ReadWord(ULO address)
{
  return (((UWO)ReadByte(address)) << 8) | ((UWO)ReadByte(address + 1));
}

ULO CiaBankHandler::ReadLong(ULO address)
{
  return (((ULO)ReadByte(address)) << 24) | (((ULO)ReadByte(address + 1)) << 16) | (((ULO)ReadByte(address + 2)) << 8) | ((ULO)ReadByte(address + 3));
}

void CiaBankHandler::WriteByte(UBY data, ULO address)
{
  _cia->WriteByte(data, address);
}

void CiaBankHandler::WriteWord(UWO data, ULO address)
{
  WriteByte((UBY)(data >> 8), address);
  WriteByte((UBY)data, address + 1);
}

void CiaBankHandler::WriteLong(ULO data, ULO address)
{
  WriteByte((UBY)(data >> 24), address);
  WriteByte((UBY)(data >> 16), address + 1);
  WriteByte((UBY)(data >> 8), address + 2);
  WriteByte((UBY)data, address + 3);
}
