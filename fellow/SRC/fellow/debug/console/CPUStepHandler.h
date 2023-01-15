#pragma once

#include "fellow/debug/console/ConsoleCommandHandler.h"
#include "fellow/debug/commands/CPUStepCommand.h"
#include "fellow/debug/console/DebugWriter.h"

class CPUStepHandler : public ConsoleCommandHandler
{
private:
  fellow::api::vm::IM68K *_cpu;
  std::string ToString(const CPUStepResult &result) const;

  ParseParametersResult<CPUStepParameters> ParseParameters(const std::vector<std::string> &tokens) const;

public:
  HelpText Help() const override;
  ConsoleCommandHandlerResult Handle(const std::vector<std::string> &tokens) override;

  CPUStepHandler(fellow::api::vm::IM68K *cpu);
  virtual ~CPUStepHandler() = default;
};
