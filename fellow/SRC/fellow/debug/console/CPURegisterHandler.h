#pragma once

#include "fellow/debug/console/ConsoleCommandHandler.h"
#include "fellow/debug/commands/CPURegisterCommand.h"
#include "fellow/debug/console/DebugWriter.h"

class CPURegisterHandler : public ConsoleCommandHandler
{
private:
  std::string _command = "r";

  void WriteCPURegisters(DebugWriter &writer, const CPURegisterResult &result) const;
  std::string ToString(const CPURegisterResult &result) const;

  ParseParametersResult<CPURegisterParameters> ParseParameters(const std::vector<std::string> &tokens) const;

public:
  HelpText Help() const override;
  ConsoleCommandHandlerResult Handle(const std::vector<std::string> &tokens) override;

  CPURegisterHandler();
  virtual ~CPURegisterHandler() = default;
};
