#include "fellow/debug/console/DisassembleHandler.h"
#include "fellow/debug/console/CPUWriterUtils.h"
#include "fellow/debug/console/ParameterUtils.h"

using namespace std;
using namespace fellow::api;
using namespace fellow::api::vm;

string DisassembleHandler::ToString(const DisassembleResult &result) const
{
  DebugWriter writer;

  size_t maxDataLength = writer.GetMaxLength<M68KDisassemblyLine>(result.Lines, [](const M68KDisassemblyLine &line) -> const std::string & { return line.GetData(); });
  size_t maxInstructionLength = writer.GetMaxLength<M68KDisassemblyLine>(result.Lines, [](const M68KDisassemblyLine &line) -> const std::string & { return line.GetInstruction(); });

  for (const auto &line : result.Lines)
  {
    CPUWriterUtils::WriteCPUDisassemblyLine(writer, line, maxDataLength, maxInstructionLength);
  }

  return writer.GetStreamString();
}

HelpText DisassembleHandler::Help() const
{
  return HelpText{.Command = GetCommandToken() + " <address | pc>", .Description = "Disassemble, omit address to continue from last address"};
}

ParseParametersResult<DisassembleParameters> DisassembleHandler::ParseParameters(const vector<string> &tokens) const
{
  ParseParametersResult<DisassembleParameters> result{.Parameters{.Address = _defaultAddress, .LineCount = DefaultLineCount}};

  ValidateParameterMaxCount(result.ValidationResult, tokens, 2);

  if (result.ValidationResult.Success() && tokens.size() == 2)
  {
    if (tokens[1] == "pc")
    {
      result.Parameters.Address = _m68k->GetPC();
    }
    else
    {
      ParameterUtils::ParseHex(tokens[1], result.Parameters.Address, result.ValidationResult);
    }
  }

  if (!result.ValidationResult.Success()) return result;

  ParameterUtils::ValidateIsEvenAddress(result.Parameters.Address, result.ValidationResult);

  return result;
}

ConsoleCommandHandlerResult DisassembleHandler::Handle(const vector<string> &tokens)
{
  ParseParametersResult<DisassembleParameters> parseResult = ParseParameters(tokens);
  ConsoleCommandHandlerResult result{.ParameterValidationResult = parseResult.ValidationResult};

  if (!result.ParameterValidationResult.Success())
  {
    result.Success = false;
    return result;
  }

  DisassembleCommand command(_m68k);
  DisassembleResult commandResult = command.Execute(parseResult.Parameters);

  result.ExecuteResult = ToString(commandResult);
  result.Success = true;

  _defaultAddress = commandResult.EndAddress;

  return result;
}

DisassembleHandler::DisassembleHandler(IM68K* m68k) : ConsoleCommandHandler("d"), _m68k(m68k), _defaultAddress(0)
{
}
