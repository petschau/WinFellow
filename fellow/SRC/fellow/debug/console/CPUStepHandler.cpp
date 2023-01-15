#include "fellow/debug/console/CPUStepHandler.h"
#include "fellow/debug/console/CPUWriterUtils.h"
#include "fellow/debug/console/ParameterUtils.h"

using namespace std;

string CPUStepHandler::ToString(const CPUStepResult &result) const
{
  DebugWriter writer;
  CPUWriterUtils::WriteCPURegisters(writer, result.RegistersAfter);
  writer.String("Next: ");
  CPUWriterUtils::WriteCPUDisassemblyLine(writer, result.DisassemblyNext);
  return writer.GetStreamString();
}

HelpText CPUStepHandler::Help() const
{
  return HelpText{.Command = GetCommandToken() + " < o | hex address>",
                  .Description = "Step instructions, one instruction if no parameter, step over if 'o' parameter, step until breakpoint if address is specified"};
}

ParseParametersResult<CPUStepParameters> CPUStepHandler::ParseParameters(const vector<string> &tokens) const
{
  ParseParametersResult<CPUStepParameters> result{};
  ValidateParameterMaxCount(result.ValidationResult, tokens, 2);

  if (tokens.size() == 1)
  {
    result.Parameters.CPUStepType = CPUStepType::One;
  }
  else if (tokens.size() == 2)
  {
    if (tokens[1] == "o")
    {
      result.Parameters.CPUStepType = CPUStepType::Over;
    }
    else
    {
      result.Parameters.CPUStepType = CPUStepType::UntilBreakpoint;
      ParameterUtils::ParseHex(tokens[1], result.Parameters.BreakpointAddress, result.ValidationResult);
    }
  }

  return result;
}

ConsoleCommandHandlerResult CPUStepHandler::Handle(const std::vector<std::string> &tokens)
{
  auto parametersResult = ParseParameters(tokens);
  ConsoleCommandHandlerResult result{.ParameterValidationResult = parametersResult.ValidationResult};

  if (!result.ParameterValidationResult.Success())
  {
    result.Success = false;
    return result;
  }

  auto commandResult = CPUStepCommand(_cpu).Execute(parametersResult.Parameters);

  result.ExecuteResult = ToString(commandResult);
  result.Success = true;

  return result;
}

CPUStepHandler::CPUStepHandler(fellow::api::vm::IM68K *cpu) : ConsoleCommandHandler("s"), _cpu(cpu)
{
}
