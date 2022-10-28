#pragma once

#include <string>
#include <vector>

class DebuggerCommandHelpText
{
public:
  std::string Command;
  std::string Description;

  const std::string &GetCommand() const
  {
    return Command;
  }
  const std::string &GetDescription() const
  {
    return Description;
  }
};

class ParametersResult
{
public:
  bool HasErrors;
  std::vector<std::string> Errors;
};

class DebuggerCommandResult
{
};

template <class PARAMETERS, class RESULT> class DebuggerCommand
{
public:
  virtual RESULT Execute(const PARAMETERS &config) = 0;
};
