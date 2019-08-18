#pragma once

#include "fellow/api/vm/IChipset.h"
#include "fellow/debug/commands/DebuggerCommand.h"

struct ChipsetRegisterParameters
{
};

class ChipsetRegisterResult
{
public:
  fellow::api::vm::DisplayState DisplayState;
  fellow::api::vm::CopperState CopperState;
};

class ChipsetRegisterCommand : public DebuggerCommand<ChipsetRegisterParameters, ChipsetRegisterResult>
{
public:
  ChipsetRegisterResult Execute(const ChipsetRegisterParameters &parameters) override;

  virtual ~ChipsetRegisterCommand() = default;
};
