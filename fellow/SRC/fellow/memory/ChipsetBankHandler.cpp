#include "ChipsetBankHandler.h"

UBY ChipsetBankHandler::ReadByte(ULO address)
{
  ULO adr = address & 0x1fe;
  if (address & 0x1)
  { // Odd address
    return (UBY)_iobank_read[adr >> 1](adr);
  }
  else
  { // Even address
    return (UBY)(_iobank_read[adr >> 1](adr) >> 8);
  }
}

UWO ChipsetBankHandler::ReadWord(ULO address)
{
  return _iobank_read[(address & 0x1fe) >> 1](address & 0x1fe);
}

ULO ChipsetBankHandler::ReadLong(ULO address)
{
  ULO adr = address & 0x1fe;
  ULO r1 = (ULO)_iobank_read[adr >> 1](adr);
  ULO r2 = (ULO)_iobank_read[(adr + 2) >> 1](adr + 2);
  return (r1 << 16) | r2;
}

void ChipsetBankHandler::WriteByte(UBY data, ULO address)
{
  ULO adr = address & 0x1fe;
  if (address & 0x1)
  { // Odd address
    _iobank_write[adr >> 1]((UWO)data, adr);
  }
  else
  { // Even address
    _iobank_write[adr >> 1](((UWO)data) << 8, adr);
  }
}

void ChipsetBankHandler::WriteWord(UWO data, ULO address)
{
  ULO adr = address & 0x1fe;
  _iobank_write[adr >> 1](data, adr);
}

void ChipsetBankHandler::WriteLong(ULO data, ULO address)
{
  ULO adr = address & 0x1fe;
  _iobank_write[adr >> 1]((UWO)(data >> 16), adr);
  _iobank_write[(adr + 2) >> 1]((UWO)data, adr + 2);
}

void ChipsetBankHandler::SetIoReadStub(ULO index, fellow::api::vm::IoReadFunc ioreadfunction)
{
  _iobank_read[index >> 1] = ioreadfunction;
}

void ChipsetBankHandler::SetIoWriteStub(ULO index, fellow::api::vm::IoWriteFunc iowritefunction)
{
 _iobank_write[index >> 1] = iowritefunction;
}
 
void ChipsetBankHandler::SetIoReadFunction(ULO index, std::function<uint16_t(uint32_t)> ioreadfunction)
{
  _iobank_read[index >> 1] = std::move(ioreadfunction);
}

void ChipsetBankHandler::SetIoWriteFunction(ULO index, std::function<void(uint16_t, uint32_t)> iowritefunction)
{
  _iobank_write[index >> 1] = std::move(iowritefunction);
}

void ChipsetBankHandler::Clear()
{
  // Array has 257 elements to account for long writes to the last address.
  for (unsigned int i = 0; i <= 512; i += 2)
  {
    SetIoReadStub(i, ReadDefault);
    SetIoWriteStub(i, WriteDefault);
  }
}

// ----------------------------
// Chip read register functions
// ----------------------------

// To simulate noise, return 0 and -1 every second time.
// Why? Bugged demos test write-only registers for various bit-values
// and to break out of loops, both 0 and 1 values must be returned.

UWO ChipsetBankHandler::ReadDefault(ULO address)
{
  return (UWO)(((rand() % 256) << 8) | (rand() % 256));
}

void ChipsetBankHandler::WriteDefault(UWO data, ULO address)
{
}
