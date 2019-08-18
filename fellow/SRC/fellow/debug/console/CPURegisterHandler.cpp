#include "fellow/debug/console/CPURegisterHandler.h"

using namespace std;

void CPURegisterHandler::WriteCPURegisters(DebugWriter &writer, const CPURegisterResult &result) const
{
  writer.String("D0:").Hex32Vector(result.Registers.DataRegisters).String(":D7").Endl();
  writer.String("A0:").Hex32Vector(result.Registers.AddressRegisters).String(":A7").Endl();

  writer.String("PC:").Hex32(result.Registers.PC);
  writer.String(" USP:").Hex32(result.Registers.USP);
  writer.String(" SSP:").Hex32(result.Registers.SSP);
  writer.String(" MSP:").Hex32(result.Registers.MSP);
  writer.String(" ISP:").Hex32(result.Registers.ISP);
  writer.String(" VBR:").Hex32(result.Registers.VBR);
  writer.Endl();

  writer.String("SR:").Hex16(result.Registers.SR);
  writer.String(" T:").NumberBits(result.Registers.SR, 14, 2);
  writer.String(" S:").NumberBit(result.Registers.SR, 13);
  writer.String(" M:").NumberBit(result.Registers.SR, 12);
  writer.String(" I:").NumberBits(result.Registers.SR, 8, 3);
  writer.String(" X:").NumberBit(result.Registers.SR, 4);
  writer.String(" N:").NumberBit(result.Registers.SR, 3);
  writer.String(" Z:").NumberBit(result.Registers.SR, 2);
  writer.String(" V:").NumberBit(result.Registers.SR, 1);
  writer.String(" C:").NumberBit(result.Registers.SR, 0);
  writer.Endl();
}

string CPURegisterHandler::ToString(const CPURegisterResult &result) const
{
  DebugWriter writer;
  WriteCPURegisters(writer, result);
  return writer.GetStreamString();
}

HelpText CPURegisterHandler::Help() const
{
  return HelpText{.Command = GetCommandToken(), .Description = "List CPU registers"};
}

ParseParametersResult<CPURegisterParameters> CPURegisterHandler::ParseParameters(const vector<string> &tokens) const
{
  ParseParametersResult<CPURegisterParameters> result{};
  ValidateParameterMaxCount(result.ValidationResult, tokens, 1);
  return result;
}

ConsoleCommandHandlerResult CPURegisterHandler::Handle(const std::vector<std::string> &tokens)
{
  auto parametersResult = ParseParameters(tokens);
  ConsoleCommandHandlerResult result{.ParameterValidationResult = parametersResult.ValidationResult};

  if (!result.ParameterValidationResult.Success())
  {
    result.Success = false;
    return result;
  }

  auto commandResult = CPURegisterCommand().Execute(CPURegisterParameters());

  result.ExecuteResult = ToString(commandResult);
  result.Success = true;

  return result;
}

CPURegisterHandler::CPURegisterHandler() : ConsoleCommandHandler("r")
{
}
