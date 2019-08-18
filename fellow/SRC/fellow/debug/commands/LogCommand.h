#pragma once

#include "fellow/api/defs.h"
#include "fellow/debug/commands/DebuggerCommand.h"

struct LogParameters
{
};

class LogResult
{
};

class LogCommand : public DebuggerCommand<LogParameters, LogResult>
{
public:
  LogResult Execute(const LogParameters &parameters) override;

  virtual ~LogCommand() = default;
};
