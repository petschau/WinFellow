#pragma once

#include <string>
#include <vector>

struct HelpText
{
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

class ValidationResult
{
public:
  bool Success() const
  {
    return Errors.empty();
  }

  std::vector<std::string> Errors;
};

template <class T> class ParseParametersResult
{
public:
  T Parameters;
  ValidationResult ValidationResult;
};

class ConsoleCommandHandlerResult
{
public:
  bool Success;
  ValidationResult ParameterValidationResult;
  std::string ExecuteResult;
};

class ConsoleCommandHandler
{
private:
  std::string _commandToken;

public:
  const std::string &GetCommandToken() const
  {
    return _commandToken;
  }

  void ValidateParameterMaxCount(ValidationResult &result, const std::vector<std::string> &tokens, size_t maxParameterCount) const
  {
    if (tokens.size() > maxParameterCount)
    {
      result.Errors.emplace_back("Too many parameters");
    }
  }

  bool IsCommand(const std::string &token) const
  {
    return token == _commandToken;
  }

  virtual HelpText Help() const = 0;
  virtual ConsoleCommandHandlerResult Handle(const std::vector<std::string> &tokens) = 0;

  ConsoleCommandHandler(const std::string &commandToken) : _commandToken(commandToken)
  {
  }
  virtual ~ConsoleCommandHandler() = default;
};
