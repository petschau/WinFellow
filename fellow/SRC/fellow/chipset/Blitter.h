#pragma once

/*===========================================================================*/
/* Blitter-properties                                                        */
/*===========================================================================*/

extern void blitterSetFast(BOOLE fast);
extern BOOLE blitterGetFast();

extern void blitterSetOperationLog(BOOLE operation_log);
extern BOOLE blitterGetOperationLog();

extern BOOLE blitterGetDMAPending();
extern BOOLE blitterGetZeroFlag();
extern ULO blitterGetFreeCycles();
extern BOOLE blitterIsStarted();

/*===========================================================================*/
/* Declare C blitter functions                                               */
/*===========================================================================*/

void blitterEndOfFrame();
void blitterEmulationStart();
void blitterEmulationStop();
void blitterHardReset();
void blitterStartup();
void blitterShutdown();
void blitterLineMode();
void blitFinishBlit();
void blitForceFinish();
void blitterCopy();
