#pragma once

#include "fellow/api/defs.h"
#include "fellow/debug/commands/DebuggerCommand.h"

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
  ULO ReadLongAsBytes(ULO address) const;

public:
  MemoryResult Execute(const MemoryParameters &parameters) override;

  virtual ~MemoryCommand() = default;
};
