#include "fellow/debug/commands/CPURegisterCommand.h"
#include "fellow/api/VM.h"

using namespace fellow::api;

CPURegisterResult CPURegisterCommand::Execute(const CPURegisterParameters &parameters)
{
  return CPURegisterResult{.Registers = VM->CPU.GetRegisters()};
}