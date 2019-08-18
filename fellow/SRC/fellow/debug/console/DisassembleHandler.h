#pragma once

#include <string>
#include "fellow/api/defs.h"
#include "fellow/debug/console/ConsoleCommandHandler.h"
#include "fellow/debug/commands/DisassembleCommand.h"
#include "fellow/debug/console/DebugWriter.h"

class DisassembleHandler : public ConsoleCommandHandler
{
private:
  const unsigned int DefaultLineCount = 16;
  ULO _defaultAddress;
  std::string ToString(const DisassembleResult &result) const;

  ParseParametersResult<DisassembleParameters> ParseParameters(const std::vector<std::string> &tokens) const;

public:
  HelpText Help() const override;
  ConsoleCommandHandlerResult Handle(const std::vector<std::string> &tokens) override;

  DisassembleHandler();
  virtual ~DisassembleHandler() = default;
};
