#ifndef FELLOW_H
#define FELLOW_H

typedef enum {
  FELLOW_RUNTIME_ERROR_NO_ERROR = 0,
  FELLOW_RUNTIME_ERROR_CPU_PC_BAD_BANK = 1
} fellow_runtime_error_codes;

extern void fellowRun(void);
extern void fellowStepOne(void);
extern void fellowStepOver(void);
extern void fellowRunDebug(void);
extern void fellowSetRuntimeErrorCode(fellow_runtime_error_codes error_code);
extern void fellowNastyExit(void);
extern void fellowAddLog(STR *msg);
extern BOOLE fellowGetMMXDetected(void);
extern void fellowSetUseGUI(BOOLE use_gui);
extern BOOLE fellowGetUseGUI(void);
extern void fellowPreStartReset(BOOLE reset);
extern BOOLE fellowGetPreStartReset(void);
extern void fellowHardReset(void);
extern BOOLE fellowEmulationStart(void);
extern void fellowEmulationStop(void);
extern void fellowRequestEmulationStop(void);
void fellowRequestEmulationStopImmediately(void);

#endif

