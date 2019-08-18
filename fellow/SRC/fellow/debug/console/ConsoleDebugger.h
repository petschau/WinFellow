#pragma once

#include "fellow/debug/console/ConsoleCommandHandler.h"

#include <string>
#include <vector>
#include <memory>

class ConsoleDebugger
{
private:
  std::vector<ConsoleCommandHandler *> _commandHandlers;

  void CreateCommandHandlers();
  void DeleteCommandHandlers();

  std::string UnknownCommand(const std::vector<std::string> &tokens);
  std::string GetErrors(const std::vector<std::string> &errors) const;
  std::string ExecuteCommand(ConsoleCommandHandler &command, const std::vector<std::string> &tokens);

public:
  std::string ExecuteCommand(const std::string &commandLine);

  ConsoleDebugger();
  ~ConsoleDebugger();
};
