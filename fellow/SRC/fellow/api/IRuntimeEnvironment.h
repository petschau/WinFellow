#pragma once

#include <string>

#include "fellow/api/defs.h"
#include "fellow/api/VM.h"

namespace fellow::api
{
  class IRuntimeEnvironment
  {
  public:
    virtual void RequestResetBeforeStartingEmulation() = 0;
    virtual void RequestEmulationStop() = 0;
    virtual std::string GetVersionString() = 0;

    virtual bool StartEmulation() = 0;
    virtual void StopEmulation() = 0;

    virtual void StepOne() = 0;
    virtual void StepOver() = 0;
    virtual void RunDebug(ULO breakpoint) = 0;

    virtual fellow::api::VirtualMachine *GetVM() = 0;

    virtual ~IRuntimeEnvironment() = default;
  };
}
