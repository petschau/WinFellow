#pragma once
#include "fellow/api/defs.h"
#include "fellow/debug/console/ConsoleCommandHandler.h"
#include "fellow/debug/commands/ChipsetRegisterCommand.h"
#include "fellow/debug/console/DebugWriter.h"

class ChipsetRegisterHandler : public ConsoleCommandHandler
{
private:
  template <typename T> void WriteRegister(DebugWriter &writer, ULO address, const std::string &name, T value, unsigned int valueDigits) const
  {
    writer.Hex24(address).Char(' ').String(name).Char(' ').Hex(value, valueDigits).Endl();
  }

  void WriteRegister12(DebugWriter &writer, ULO address, const std::string &name, UWO value) const;
  void WriteRegister16(DebugWriter &writer, ULO address, const std::string &name, UWO value) const;
  void WriteRegister20(DebugWriter &writer, ULO address, const std::string &name, ULO value) const;
  void WriteDisplayState(DebugWriter &writer, const fellow::api::vm::DisplayState &displayState) const;

  std::string GetRunStateString(fellow::api::vm::CopperRunState runstate) const;
  std::string GetCopdangString(fellow::api::vm::CopperChipType type, UWO copdang) const;
  void WriteCopperState(DebugWriter &writer, const fellow::api::vm::CopperState &copperState) const;

  std::string ToString(const ChipsetRegisterResult &result) const;
  ParseParametersResult<ChipsetRegisterParameters> ParseParameters(const std::vector<std::string> &tokens) const;

public:
  HelpText Help() const override;
  ConsoleCommandHandlerResult Handle(const std::vector<std::string> &tokens) override;

  ChipsetRegisterHandler();
  virtual ~ChipsetRegisterHandler() = default;
};
