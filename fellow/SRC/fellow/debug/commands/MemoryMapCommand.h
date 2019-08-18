#pragma once

#include "fellow/api/defs.h"
#include "fellow/debug/commands/DebuggerCommand.h"
#include "fellow/api/vm/MemorySystemTypes.h"

struct MemoryMapParameters
{
};

struct MemoryMapEntry
{
  ULO StartAddress;
  ULO EndAddress;
  std::string Description;
};

class MemoryMapResult
{
public:
  unsigned int Bits;
  std::vector<MemoryMapEntry> Entries;
};

class MemoryMapCommand : public DebuggerCommand<MemoryMapParameters, MemoryMapResult>
{
private:
  std::vector<MemoryMapEntry> GetMemoryMapEntries();
  std::string GetMemoryKindDescription(fellow::api::vm::MemoryKind kind);

public:
  MemoryMapResult Execute(const MemoryMapParameters &parameters) override;

  virtual ~MemoryMapCommand() = default;
};
