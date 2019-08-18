#include "fellow/debug/console/ChipsetRegisterHandler.h"

using namespace std;
using namespace fellow::api::vm;

void ChipsetRegisterHandler::WriteRegister12(DebugWriter &writer, ULO address, const string &name, UWO value) const
{
  WriteRegister(writer, address, name, value, 3);
}

void ChipsetRegisterHandler::WriteRegister16(DebugWriter &writer, ULO address, const string &name, UWO value) const
{
  WriteRegister(writer, address, name, value, 4);
}

void ChipsetRegisterHandler::WriteRegister20(DebugWriter &writer, ULO address, const string &name, ULO value) const
{
  WriteRegister(writer, address, name, value, 5);
}

void ChipsetRegisterHandler::WriteDisplayState(DebugWriter &writer, const DisplayState &displayState) const
{
  writer.String("Display register values:").Endl();
  writer.String("------------------------").Endl();

  WriteRegister16(writer, 0xdff004, "VPOS", displayState.VPOS);
  WriteRegister16(writer, 0xdff006, "VHPOS", displayState.VHPOS);
  WriteRegister16(writer, 0xdff07c, "DENISEID", displayState.DENISEID);
  WriteRegister16(writer, 0xdff08e, "DIWSTRT", displayState.DIWSTRT);
  WriteRegister16(writer, 0xdff090, "DIWSTOP", displayState.DIWSTOP);
  WriteRegister16(writer, 0xdff092, "DDFSTRT", displayState.DDFSTRT);
  WriteRegister16(writer, 0xdff094, "DDFSTOP", displayState.DDFSTOP);

  for (unsigned int registerNumber = 0; registerNumber < 6; registerNumber++)
  {
    ostringstream oss;
    oss << "BPL" << setw(1) << registerNumber + 1 << "PT";
    WriteRegister20(writer, 0xdff0e0 + registerNumber * 4, oss.str(), displayState.BPLPT[registerNumber]);
  }

  WriteRegister16(writer, 0xdff100, "BPLCON0", displayState.BPLCON0);
  WriteRegister16(writer, 0xdff102, "BPLCON1", displayState.BPLCON1);
  WriteRegister16(writer, 0xdff104, "BPLCON2", displayState.BPLCON2);
  WriteRegister16(writer, 0xdff108, "BPL1MOD", displayState.BPL1MOD);
  WriteRegister16(writer, 0xdff10a, "BPL2MOD", displayState.BPL2MOD);

  for (unsigned int registerNumber = 0; registerNumber < 32; registerNumber++)
  {
    ostringstream oss;
    oss << "COLOR" << setw(2) << setfill('0') << registerNumber;
    WriteRegister12(writer, 0xdff180 + registerNumber * 2, oss.str(), displayState.COLOR[registerNumber]);
  }

  for (unsigned int registerNumber = 0; registerNumber < 6; registerNumber++)
  {
    ostringstream oss;
    oss << "BPL" << setw(1) << registerNumber + 1 << "DAT";
    WriteRegister16(writer, 0xdff110 + registerNumber * 2, oss.str(), displayState.BPLDAT[registerNumber]);
  }

  WriteRegister16(writer, 0xdff1e4, "DIWHIGH", displayState.DIWHIGH);
  writer.Endl();

  writer.String("Decomposed values:").Endl();
  writer.String("------------------").Endl();
  writer.String("Vertical position:   ").NumberHex12(displayState.VerticalPosition).Endl();
  writer.String("Horisontal position: ")
      .String("SHres: ")
      .NumberHex12(displayState.HorisontalPosition)
      .String(" CCK: ")
      .NumberHex12(displayState.HorisontalPosition / 4)
      .String(" Bus: ")
      .NumberHex12(displayState.HorisontalPosition / 8)
      .Endl();
  writer.String("Long frame:          ").String(displayState.Lof ? "yes" : "no").Endl();
  writer.String("DIW vertical start:  ").NumberHex12(displayState.DiwYStart).Endl();
  writer.String("DIW vertical stop:   ").NumberHex12(displayState.DiwYStop).Endl();
  writer.String("DIW horisontal start:").NumberHex12(displayState.DiwXStart).Endl();
  writer.String("DIW horisontal stop: ").NumberHex12(displayState.DiwXStop).Endl();
  writer.String("Resolution:          ").String(displayState.IsLores ? "Lores" : "Hires").Endl();
  writer.String("Dual playfield:      ").String(displayState.IsDualPlayfield ? "yes" : "no").Endl();
  writer.String("HAM:                 ").String(displayState.IsHam ? "yes" : "no").Endl();
  writer.String("Bitplane count:      ").Number(displayState.EnabledBitplaneCount).Endl();
  writer.String("Agnus Id:            ").Hex8(displayState.AgnusId).Endl();
}

string ChipsetRegisterHandler::GetCopdangString(CopperChipType type, UWO copdang) const
{
  if (type == CopperChipType::OCS)
  {
    if (copdang)
    {
      return "Access to $40 and above (OCS)";
    }

    return "Access to $80 and above (OCS)";
  }

  if (type == CopperChipType::ECS)
  {
    if (copdang)
    {
      return "Access to $10 and above (ECS)";
    }

    return "Access to $40 and above (ECS)";
  }

  return "AGA chip type returned - not implemented";
}

string ChipsetRegisterHandler::GetRunStateString(CopperRunState runstate) const
{
  switch (runstate)
  {
    case CopperRunState::Active: return "Active";
    case CopperRunState::Halted: return "Halted";
    case CopperRunState::Suspended: return "Suspended";
  }
  return "Unknown (bug)";
}

void ChipsetRegisterHandler::WriteCopperState(DebugWriter &writer, const CopperState &copperState) const
{
  writer.String("Copper register values:").Endl();
  writer.String("-----------------------").Endl();

  WriteRegister16(writer, 0xdff02e, "COPCON", copperState.COPCON);
  WriteRegister20(writer, 0xdff080, "COP1LC", copperState.COP1LC);
  WriteRegister20(writer, 0xdff084, "COP2LC", copperState.COP2LC);

  writer.Endl();

  writer.String("Decomposed values:").Endl();
  writer.String("------------------").Endl();
  writer.String("Current LC:     ").Hex20(copperState.COPLC).Endl();
  writer.String("Copper state:   ").String(GetRunStateString(copperState.RunState)).Endl();
  writer.String("Copdang status: ").String(GetCopdangString(copperState.ChipType, copperState.COPCON)).Endl();
}

string ChipsetRegisterHandler::ToString(const ChipsetRegisterResult &result) const
{
  DebugWriter writer;
  WriteDisplayState(writer, result.DisplayState);
  writer.Endl();
  WriteCopperState(writer, result.CopperState);
  return writer.GetStreamString();
}

ParseParametersResult<ChipsetRegisterParameters> ChipsetRegisterHandler::ParseParameters(const vector<string> &tokens) const
{
  ParseParametersResult<ChipsetRegisterParameters> result{};
  ValidateParameterMaxCount(result.ValidationResult, tokens, 1);
  return result;
}

HelpText ChipsetRegisterHandler::Help() const
{
  return HelpText{.Command = GetCommandToken(), .Description = "List custom chipset registers"};
}

ConsoleCommandHandlerResult ChipsetRegisterHandler::Handle(const vector<string> &tokens)
{
  auto parseParametersResult = ParseParameters(tokens);
  ConsoleCommandHandlerResult result{.ParameterValidationResult = parseParametersResult.ValidationResult};

  if (!result.ParameterValidationResult.Success())
  {
    result.Success = false;
    return result;
  }

  auto commandResult = ChipsetRegisterCommand().Execute(parseParametersResult.Parameters);

  result.ExecuteResult = ToString(commandResult);
  result.Success = true;

  return result;
}

ChipsetRegisterHandler::ChipsetRegisterHandler() : ConsoleCommandHandler("c")
{
}
