#include "fellow/debug/commands/CPUStepCommand.h"
#include "fellow/api/VM.h"

using namespace fellow::api;
using namespace fellow::api::vm;

CPUStepResult CPUStepCommand::Execute(const CPUStepParameters &parameters)
{
  uint32_t pc = VM->CPU.GetPC();
  M68KDisassemblyLine disassembly = VM->CPU.GetDisassembly(pc);

  if (parameters.CPUStepType == CPUStepType::One)
  {
    VM->CPU.StepOne();
  }
  else if (parameters.CPUStepType == CPUStepType::Over)
  {
    VM->CPU.StepOver();
  }
  else if (parameters.CPUStepType == CPUStepType::UntilBreakpoint)
  {
    VM->CPU.StepUntilBreakpoint(parameters.BreakpointAddress);
  }

  uint32_t pcNext = VM->CPU.GetPC();
  M68KDisassemblyLine disassemblyNext = VM->CPU.GetDisassembly(pcNext);

  return CPUStepResult{.RegistersAfter = VM->CPU.GetRegisters(), .Disassembly = disassembly, .DisassemblyNext = disassemblyNext, .PC = pc, .PCNext = pcNext};
}