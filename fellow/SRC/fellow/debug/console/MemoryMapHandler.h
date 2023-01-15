#pragma once

#include "fellow/debug/console/ConsoleCommandHandler.h"
#include "fellow/debug/commands/MemoryMapCommand.h"

class MemoryMapHandler : public ConsoleCommandHandler
{
private:
  fellow::api::vm::IMemorySystem *_vmMemory;
  std::string ToString(const MemoryMapResult &result) const;
  ParseParametersResult<MemoryMapParameters> ParseParameters(const std::vector<std::string> &tokens) const;

public:
  HelpText Help() const override;
  ConsoleCommandHandlerResult Handle(const std::vector<std::string> &tokens) override;

  MemoryMapHandler(fellow::api::vm::IMemorySystem *vmMemory);
  virtual ~MemoryMapHandler() = default;
};
