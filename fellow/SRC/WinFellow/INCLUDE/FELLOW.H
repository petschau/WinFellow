#pragma once

#include <string>
#include <list>

typedef enum
{
  FELLOW_RUNTIME_ERROR_NO_ERROR = 0,
  FELLOW_RUNTIME_ERROR_CPU_PC_BAD_BANK = 1
} fellow_runtime_error_codes;

typedef enum
{
  FELLOW_REQUESTER_TYPE_NONE = 0,
  FELLOW_REQUESTER_TYPE_INFO = 1,
  FELLOW_REQUESTER_TYPE_WARN = 2,
  FELLOW_REQUESTER_TYPE_ERROR = 3
} FELLOW_REQUESTER_TYPE;

extern BOOLE fellow_request_emulation_stop;

extern void fellowRun();
extern void fellowStepOne();
extern void fellowStepOver();
extern void fellowRunDebug(uint32_t breakpoint);
extern void fellowSetRuntimeErrorCode(fellow_runtime_error_codes error_code);
extern void fellowNastyExit();
extern char *fellowGetVersionString();
extern void fellowSetPreStartReset(bool reset);
extern bool fellowGetPreStartReset();
extern BOOLE fellowSaveState(char *filename);
extern BOOLE fellowLoadState(char *filename);
extern void fellowSoftReset();
extern void fellowHardReset();
extern bool fellowEmulationStart();
extern void fellowEmulationStop();
extern void fellowRequestEmulationStop();

extern void fellowShowRequester(FELLOW_REQUESTER_TYPE, const char *, ...);
