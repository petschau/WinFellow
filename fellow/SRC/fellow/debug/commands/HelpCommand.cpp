#include "fellow/debug/commands/HelpCommand.h"

using namespace std;

DebuggerCommandHelpText HelpCommand::Help() const
{
  return DebuggerCommandHelpText{ .Command = _config.Command, .Description = "Summary of available commands" };
}

bool HelpCommand::IsCommand(const string& token) const
{
  return token == _config.Command;
}

ParametersResult HelpCommand::SetParameters(const vector<string>& tokens)
{
  return ParametersResult{ .HasErrors = false };
}

HelpResult HelpCommand::Execute(const HelpParameters& parameters)
{
  HelpResult result;

  for (const auto* command : _commands)
  {
    result.HelpTexts.emplace_back(command->Help());
  }

  return result;
}

HelpCommand::HelpCommand(const vector<DebuggerCommand*>& commands) noexcept
  : _config{.Command = "h"},
    _commands(commands)
{
}
