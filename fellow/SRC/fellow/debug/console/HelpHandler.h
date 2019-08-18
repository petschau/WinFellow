#pragma once

#include <string>
#include <vector>
#include "fellow/debug/console/ConsoleCommandHandler.h"

class HelpHandler : public ConsoleCommandHandler
{
private:
  std::vector<ConsoleCommandHandler *> _commandHandlers;

  std::string ToString(const std::vector<HelpText> &handlers) const;
  std::vector<HelpText> GetHelpTexts() const;

public:
  HelpText Help() const override;
  ConsoleCommandHandlerResult Handle(const std::vector<std::string> &tokens) override;

  HelpHandler(const std::vector<ConsoleCommandHandler *> &commandHandlers);
};
