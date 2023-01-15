#pragma once

#include "fellow/api/defs.h"
#include "fellow/debug/commands/DebuggerCommand.h"
#include "fellow/api/vm/IMemorySystem.h"

struct MemoryParameters
{
  ULO Address;
  unsigned int LineCount;
  unsigned int LongsPerLine;
};

class MemoryResult
{
public:
  ULO StartAddress;
  ULO EndAddress;
  std::vector<ULO> Values;
};

class MemoryCommand : public DebuggerCommand<MemoryParameters, MemoryResult>
{
private:
  fellow::api::vm::IMemorySystem *_vmMemory;

  ULO ReadLongAsBytes(ULO address) const;

public:
  MemoryResult Execute(const MemoryParameters &parameters) override;

  MemoryCommand(fellow::api::vm::IMemorySystem *vmMemory);
  virtual ~MemoryCommand() = default;
};
