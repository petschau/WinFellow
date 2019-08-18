#pragma once

#include "fellow/api/vm/IM68K.h"
#include "fellow/debug/commands/DebuggerCommand.h"

struct CPURegisterParameters
{
};

class CPURegisterResult
{
public:
  fellow::api::vm::M68KRegisters Registers;
};

class CPURegisterCommand : public DebuggerCommand<CPURegisterParameters, CPURegisterResult>
{
public:
  CPURegisterResult Execute(const CPURegisterParameters &parameters) override;

  virtual ~CPURegisterCommand() = default;
};
