#pragma once

#include "fellow/debug/console/ConsoleCommandHandler.h"
#include "fellow/debug/console/DebugWriter.h"
#include "fellow/debug/commands/DelayedRendererCommand.h"

class LogHandler : public ConsoleCommandHandler
{
private:
  std::string ToString(const LogResult &result, const LogParameters &parameters) const;
  ParseParametersResult<LogParameters> ParseParameters(const std::vector<std::string> &tokens) const;

public:
  HelpText Help() const override;
  ConsoleCommandHandlerResult Handle(const std::vector<std::string> &tokens) override;

  LogHandler();
  virtual ~LogHandler() = default;
};
