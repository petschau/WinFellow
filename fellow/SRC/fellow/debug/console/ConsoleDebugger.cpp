#include "fellow/debug/console/ConsoleDebugger.h"
#include "fellow/debug/console/ParameterUtils.h"

#include "fellow/debug/console/DisassembleHandler.h"
#include "fellow/debug/console/CPURegisterHandler.h"
#include "fellow/debug/console/CPUStepHandler.h"
#include "fellow/debug/console/MemoryHandler.h"
#include "fellow/debug/console/MemoryMapHandler.h"
#include "fellow/debug/console/ChipsetRegisterHandler.h"
#include "fellow/debug/console/HelpHandler.h"
#include "fellow/api/VM.h"

#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;
using namespace fellow::api;

string ConsoleDebugger::UnknownCommand(const vector<string> &tokens) const
{
  ostringstream os;
  os << "Unknown command " << tokens[0] << " - type h for command summary" << endl;
  return os.str();
}

string ConsoleDebugger::GetErrors(const vector<string> &errors) const
{
  ostringstream os;

  for (const auto &error : errors)
  {
    os << error << endl;
  }

  return os.str();
}

string ConsoleDebugger::ExecuteCommand(ConsoleCommandHandler &handler, const vector<string> &tokens)
{
  ConsoleCommandHandlerResult result = handler.Handle(tokens);

  if (!result.ParameterValidationResult.Success())
  {
    return GetErrors(result.ParameterValidationResult.Errors);
  }

  return result.ExecuteResult;
}

string ConsoleDebugger::ExecuteCommand(const string &commandLine)
{
  vector<string> tokens = ParameterUtils::SplitArguments(commandLine);

  if (tokens.empty())
  {
    return "";
  }

  for (auto *handler : _commandHandlers)
  {
    if (handler->IsCommand(tokens[0]))
    {
      return ExecuteCommand(*handler, tokens);
    }
  }

  return UnknownCommand(tokens);
}

void ConsoleDebugger::CreateCommandHandlers()
{
  _commandHandlers.push_back(new CPURegisterHandler());
  _commandHandlers.push_back(new CPUStepHandler());
  _commandHandlers.push_back(new DisassembleHandler(_vm->CPU));
  _commandHandlers.push_back(new MemoryHandler(_vm->Memory));
  _commandHandlers.push_back(new MemoryMapHandler(_vm->Memory));
  _commandHandlers.push_back(new ChipsetRegisterHandler());
  _commandHandlers.push_back(new HelpHandler(_commandHandlers));
}

void ConsoleDebugger::DeleteCommandHandlers()
{
  for (auto command : _commandHandlers)
  {
    delete command;
  }
  _commandHandlers.clear();
}

ConsoleDebugger::ConsoleDebugger(VirtualMachine *vm) : _vm(vm)
{
  CreateCommandHandlers();
}

ConsoleDebugger::~ConsoleDebugger()
{
  DeleteCommandHandlers();
}
