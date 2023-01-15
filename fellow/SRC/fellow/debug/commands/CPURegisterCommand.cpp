#include "fellow/debug/commands/CPURegisterCommand.h"
#include "fellow/api/VM.h"

using namespace fellow::api;

CPURegisterResult CPURegisterCommand::Execute(const CPURegisterParameters &parameters)
{
  return CPURegisterResult{.Registers = _cpu->GetRegisters()};
}

CPURegisterCommand::CPURegisterCommand(fellow::api::vm::IM68K *cpu) : _cpu(cpu)
{
}
