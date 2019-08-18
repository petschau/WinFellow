#pragma once

#include "fellow/debug/console/ConsoleCommandHandler.h"
#include "fellow/debug/console/DebugWriter.h"
#include "fellow/debug/commands/MemoryCommand.h"

class MemoryHandler : public ConsoleCommandHandler
{
private:
  const unsigned int DefaultLineCount = 8;
  const unsigned int DefaultLongsPerLine = 8;
  uint32_t _defaultAddress;

  ULO GetStartAddressForLine(const MemoryResult &result, size_t lineIndex) const;
  std::string ToString(const MemoryResult &result, const MemoryParameters &parameters) const;
  ParseParametersResult<MemoryParameters> ParseParameters(const std::vector<std::string> &tokens) const;

public:
  HelpText Help() const override;
  ConsoleCommandHandlerResult Handle(const std::vector<std::string> &tokens) override;

  MemoryHandler();
  virtual ~MemoryHandler() = default;
};
