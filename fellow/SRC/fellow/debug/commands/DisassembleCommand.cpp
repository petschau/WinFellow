#include "fellow/debug/commands/DisassembleCommand.h"
#include "fellow/api/VM.h"

using namespace fellow::api;

DisassembleResult DisassembleCommand::Execute(const DisassembleParameters &parameters)
{
  ULO address = parameters.Address;
  DisassembleResult result{.StartAddress = address};

  for (unsigned i = 0; i < parameters.LineCount; i++)
  {
    auto line = VM->CPU.GetDisassembly(address);
    result.Lines.push_back(line);
    address += line.Length;
  }

  result.EndAddress = address;

  return result;
}
