#pragma once

#include "fellow/api/defs.h"
#include "fellow/debug/commands/DebuggerCommand.h"
#include "fellow/api/vm/IM68K.h"

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
private:
  fellow::api::vm::IM68K *_m68k;

public:
  DisassembleResult Execute(const DisassembleParameters &parameters) override;

  DisassembleCommand(fellow::api::vm::IM68K *m68k);
  virtual ~DisassembleCommand() = default;
};
