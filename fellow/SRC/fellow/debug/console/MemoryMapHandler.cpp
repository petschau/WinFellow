#include "fellow/debug/console/MemoryMapHandler.h"
#include "fellow/debug/console/DebugWriter.h"

using namespace std;
using namespace fellow::api::vm;

string MemoryMapHandler::ToString(const MemoryMapResult &result) const
{
  DebugWriter writer;

  writer.String("Memory space is ").Number(result.Bits).String(" bits").Endl();

  for (auto &entry : result.Entries)
  {
    writer.Hex32(entry.StartAddress).Char('-').Hex32(entry.EndAddress).Char('-').String(entry.Description).Endl();
  }

  return writer.GetStreamString();
}

HelpText MemoryMapHandler::Help() const
{
  return HelpText{.Command = GetCommandToken(), .Description = "List memory map"};
}

ParseParametersResult<MemoryMapParameters> MemoryMapHandler::ParseParameters(const vector<string> &tokens) const
{
  ParseParametersResult<MemoryMapParameters> result{};
  ValidateParameterMaxCount(result.ValidationResult, tokens, 1);
  return result;
}

ConsoleCommandHandlerResult MemoryMapHandler::Handle(const std::vector<std::string> &tokens)
{
  auto parametersResult = ParseParameters(tokens);
  ConsoleCommandHandlerResult result{.ParameterValidationResult = parametersResult.ValidationResult};

  if (!result.ParameterValidationResult.Success())
  {
    result.Success = false;
    return result;
  }

  auto commandResult = MemoryMapCommand(_vmMemory).Execute(parametersResult.Parameters);

  result.ExecuteResult = ToString(commandResult);
  result.Success = true;

  return result;
}

MemoryMapHandler::MemoryMapHandler(IMemorySystem *vmMemory) : ConsoleCommandHandler("mm"), _vmMemory(vmMemory)
{
}
