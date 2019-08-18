#include "fellow/debug/console/MemoryHandler.h"
#include "fellow/debug/console/ParameterUtils.h"

using namespace std;

string MemoryHandler::ToString(const MemoryResult &result, const MemoryParameters &parameters) const
{
  DebugWriter writer;
  size_t lineCount = (result.Values.size() + parameters.LongsPerLine - 1) / parameters.LongsPerLine;
  size_t lineIndex = 0;

  for (size_t line = 0; line < lineCount; line++)
  {
    writer.Hex32(GetStartAddressForLine(result, lineIndex)).Char(':');

    for (size_t i = 0; i < parameters.LongsPerLine; i++)
    {
      writer.Char(' ');
      writer.Hex32(result.Values[lineIndex + i]);
    }

    writer.Char(' ');
    for (size_t i = 0; i < parameters.LongsPerLine; i++)
    {
      writer.Chars(result.Values[lineIndex + i]);
    }
    writer.Endl();
    lineIndex += parameters.LongsPerLine;
  }

  return writer.GetStreamString();
}

HelpText MemoryHandler::Help() const
{
  return HelpText{.Command = GetCommandToken() + " <hex address>", .Description = "List memory contents, omit address to continue from last address"};
}

ULO MemoryHandler::GetStartAddressForLine(const MemoryResult &result, size_t lineIndex) const
{
  return result.StartAddress + ((ULO)lineIndex) * 4;
}

ParseParametersResult<MemoryParameters> MemoryHandler::ParseParameters(const vector<string> &tokens) const
{
  ParseParametersResult<MemoryParameters> result{.Parameters = {.Address = _defaultAddress, .LineCount = DefaultLineCount, .LongsPerLine = DefaultLongsPerLine}};

  ValidateParameterMaxCount(result.ValidationResult, tokens, 2);

  if (result.ValidationResult.Success() && tokens.size() == 2)
  {
    ParameterUtils::ParseHex(tokens[1], result.Parameters.Address, result.ValidationResult);
  }

  return result;
}

ConsoleCommandHandlerResult MemoryHandler::Handle(const vector<string> &tokens)
{
  auto parametersResult = ParseParameters(tokens);

  ConsoleCommandHandlerResult result{.ParameterValidationResult = parametersResult.ValidationResult};

  if (!result.ParameterValidationResult.Success())
  {
    result.Success = false;
    return result;
  }

  auto commandResult = MemoryCommand().Execute(parametersResult.Parameters);

  result.ExecuteResult = ToString(commandResult, parametersResult.Parameters);
  result.Success = true;

  _defaultAddress = commandResult.EndAddress;

  return result;
}

MemoryHandler::MemoryHandler() : ConsoleCommandHandler("m"), _defaultAddress(0)
{
}