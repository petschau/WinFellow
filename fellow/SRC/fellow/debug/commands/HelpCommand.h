#ifndef HELP_COMMAND_H
#define HELP_COMMAND_H

#include "fellow/debug/commands/DebuggerCommand.h"

struct HelpConfig
{
  std::string Command;
};

struct HelpParameters
{
};

class HelpResult
{
public:
  std::vector<DebuggerCommandHelpText> HelpTexts;
};

class HelpCommand : public DebuggerCommand<HelpParameters, HelpResult>
{
private:
  HelpConfig _config;
  const std::vector<DebuggerCommand*>& _commands;

public:
  DebuggerCommandHelpText Help() const override;
  bool IsCommand(const std::string& token) const override;
  ParametersResult SetParameters(const std::vector<std::string>& tokens) override;
  HelpResult Execute(const HelpParameters& parameters) override;

  HelpCommand(const std::vector<DebuggerCommand*>& commands) noexcept;
};

#endif
