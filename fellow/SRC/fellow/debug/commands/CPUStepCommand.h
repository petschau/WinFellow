#pragma once

#include "fellow/api/vm/IM68K.h"
#include "fellow/debug/commands/DebuggerCommand.h"

enum class CPUStepType
{
  One,
  Over,
  UntilBreakpoint
};

struct CPUStepParameters
{
  CPUStepType CPUStepType;
  uint32_t BreakpointAddress;
};

class CPUStepResult
{
public:
  fellow::api::vm::M68KRegisters RegistersAfter;
  fellow::api::vm::M68KDisassemblyLine Disassembly;
  fellow::api::vm::M68KDisassemblyLine DisassemblyNext;
  uint32_t PC;
  uint32_t PCNext;
};

class CPUStepCommand : public DebuggerCommand<CPUStepParameters, CPUStepResult>
{
private:
  fellow::api::vm::IM68K *_cpu;

public:
  CPUStepResult Execute(const CPUStepParameters &parameters) override;

  CPUStepCommand(fellow::api::vm::IM68K *cpu);
  virtual ~CPUStepCommand() = default;
};
