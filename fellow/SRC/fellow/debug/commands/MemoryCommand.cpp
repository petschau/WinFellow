#include "fellow/debug/commands/MemoryCommand.h"
#include "fellow/api/VM.h"

using namespace std;
using namespace fellow::api;

MemoryResult MemoryCommand::Execute(const MemoryParameters &parameters)
{
  ULO address = parameters.Address;
  ULO longCount = parameters.LineCount * parameters.LongsPerLine;

  MemoryResult result{.StartAddress = address, .EndAddress = address + longCount * 4};

  for (unsigned int i = 0; i < longCount; i++)
  {
    result.Values.push_back(ReadLongAsBytes(address + i * 4));
  }

  return result;
}

ULO MemoryCommand::ReadLongAsBytes(ULO address) const
{
  return VM->Memory.ReadByte(address) << 24 | VM->Memory.ReadByte(address + 1) << 16 | VM->Memory.ReadByte(address + 2) << 8 | VM->Memory.ReadByte(address + 3);
}
