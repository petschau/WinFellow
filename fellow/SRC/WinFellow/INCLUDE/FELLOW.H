#ifndef FELLOW_H
#define FELLOW_H

#include <string>
#include <list>

typedef enum {
  FELLOW_RUNTIME_ERROR_NO_ERROR = 0,
  FELLOW_RUNTIME_ERROR_CPU_PC_BAD_BANK = 1
} fellow_runtime_error_codes;

typedef enum {
  FELLOW_REQUESTER_TYPE_NONE  = 0,
  FELLOW_REQUESTER_TYPE_INFO  = 1,
  FELLOW_REQUESTER_TYPE_WARN  = 2,
  FELLOW_REQUESTER_TYPE_ERROR = 3
} FELLOW_REQUESTER_TYPE;

extern BOOLE fellow_request_emulation_stop;

extern void fellowRun(void);
extern void fellowStepOne(void);
extern void fellowStepOver(void);
extern void fellowRunDebug(uint32_t breakpoint);
extern void fellowSetRuntimeErrorCode(fellow_runtime_error_codes error_code);
extern void fellowNastyExit(void);
extern char *fellowGetVersionString(void);
extern void fellowSetPreStartReset(BOOLE reset);
extern BOOLE fellowGetPreStartReset(void);
extern BOOLE fellowSaveState(STR *filename);
extern BOOLE fellowLoadState(STR *filename);
extern void fellowSoftReset(void);
extern void fellowHardReset(void);
extern BOOLE fellowEmulationStart(void);
extern void fellowEmulationStop(void);
extern void fellowRequestEmulationStop(void);

extern void fellowAddLog(const char *, ...);
extern void fellowAddLogRequester(FELLOW_REQUESTER_TYPE, const char *, ...);
extern void fellowAddTimelessLog(const char *, ...);
extern void fellowAddLog2(STR *msg);

#endif