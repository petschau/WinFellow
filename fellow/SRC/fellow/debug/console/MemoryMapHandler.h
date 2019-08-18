#pragma once

#include "fellow/debug/console/ConsoleCommandHandler.h"
#include "fellow/debug/commands/MemoryMapCommand.h"

class MemoryMapHandler : public ConsoleCommandHandler
{
private:
  std::string ToString(const MemoryMapResult &result) const;
  ParseParametersResult<MemoryMapParameters> ParseParameters(const std::vector<std::string> &tokens) const;

public:
  HelpText Help() const override;
  ConsoleCommandHandlerResult Handle(const std::vector<std::string> &tokens) override;

  MemoryMapHandler();
  virtual ~MemoryMapHandler() = default;
};
