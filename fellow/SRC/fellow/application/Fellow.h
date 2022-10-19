#pragma once

#include "fellow/api/defs.h"

enum class fellow_runtime_error_codes
{
  FELLOW_RUNTIME_ERROR_NO_ERROR = 0,
  FELLOW_RUNTIME_ERROR_CPU_PC_BAD_BANK = 1
};

extern BOOLE fellow_request_emulation_stop;

extern void fellowRun();
extern void fellowStepOne();
extern void fellowStepLine();
extern void fellowStepFrame();
extern void fellowStepOver();
extern void fellowRunDebug(ULO breakpoint);
extern void fellowSetRuntimeErrorCode(fellow_runtime_error_codes error_code);
extern void fellowNastyExit();
extern char *fellowGetVersionString();
extern void fellowSetPreStartReset(bool reset);
extern bool fellowGetPreStartReset();
extern void fellowSoftReset();
extern void fellowHardReset();
extern bool fellowEmulationStart();
extern void fellowEmulationStop();
extern void fellowRequestEmulationStop();
