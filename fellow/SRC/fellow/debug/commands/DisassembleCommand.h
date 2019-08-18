#pragma once

#include "fellow/debug/commands/DebuggerCommand.h"
#include "fellow/api/VM.h"

struct DisassembleParameters
{
  ULO Address;
  unsigned int LineCount;
};

class DisassembleResult
{
public:
  ULO StartAddress;
  ULO EndAddress;
  std::vector<fellow::api::vm::M68KDisassemblyLine> Lines;
};

class DisassembleCommand : public DebuggerCommand<DisassembleParameters, DisassembleResult>
{
public:
  DisassembleResult Execute(const DisassembleParameters &parameters) override;

  virtual ~DisassembleCommand() = default;
};
