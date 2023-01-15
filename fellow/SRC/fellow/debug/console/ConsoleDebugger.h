#pragma once

#include "fellow/debug/console/ConsoleCommandHandler.h"
#include "fellow/api/VM.h"

#include <string>
#include <vector>

class ConsoleDebugger
{
private:
  fellow::api::VirtualMachine *_vm;
  std::vector<ConsoleCommandHandler *> _commandHandlers;

  void CreateCommandHandlers();
  void DeleteCommandHandlers();

  std::string UnknownCommand(const std::vector<std::string> &tokens) const;
  std::string GetErrors(const std::vector<std::string> &errors) const;
  std::string ExecuteCommand(ConsoleCommandHandler &command, const std::vector<std::string> &tokens);

public:
  std::string ExecuteCommand(const std::string &commandLine);

  ConsoleDebugger(fellow::api::VirtualMachine *vm);
  ~ConsoleDebugger();
};
