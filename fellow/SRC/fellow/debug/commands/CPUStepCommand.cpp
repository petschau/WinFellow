#include "fellow/debug/commands/CPUStepCommand.h"
#include "fellow/api/VM.h"

using namespace fellow::api;
using namespace fellow::api::vm;

CPUStepResult CPUStepCommand::Execute(const CPUStepParameters &parameters)
{
  uint32_t pc = _cpu->GetPC();
  M68KDisassemblyLine disassembly = _cpu->GetDisassembly(pc);

  if (parameters.CPUStepType == CPUStepType::One)
  {
    _cpu->StepOne();
  }
  else if (parameters.CPUStepType == CPUStepType::Over)
  {
    _cpu->StepOver();
  }
  else if (parameters.CPUStepType == CPUStepType::UntilBreakpoint)
  {
    _cpu->StepUntilBreakpoint(parameters.BreakpointAddress);
  }

  uint32_t pcNext = _cpu->GetPC();
  M68KDisassemblyLine disassemblyNext = _cpu->GetDisassembly(pcNext);

  return CPUStepResult{.RegistersAfter = _cpu->GetRegisters(), .Disassembly = disassembly, .DisassemblyNext = disassemblyNext, .PC = pc, .PCNext = pcNext};
}

CPUStepCommand::CPUStepCommand(fellow::api::vm::IM68K *cpu) : _cpu(cpu)
{
}
