#ifndef BLIT_H
#define BLIT_H

/*===========================================================================*/
/* Blitter-properties                                                        */
/*===========================================================================*/

extern void blitterSetFast(BOOLE fast);
extern BOOLE blitterGetFast(void);

extern void blitterSetOperationLog(BOOLE operation_log);
extern BOOLE blitterGetOperationLog(void);

extern BOOLE blitterGetDMAPending(void);
extern BOOLE blitterGetZeroFlag(void);
extern ULO blitterGetFreeCycles(void);
extern BOOLE blitterIsStarted(void);

/*===========================================================================*/
/* Declare C blitter functions                                               */
/*===========================================================================*/

extern void blitterSaveState(FILE *F);
extern void blitterLoadState(FILE *F);
void blitterEndOfFrame(void);
void blitterEmulationStart(void);
void blitterEmulationStop(void);
void blitterHardReset(void);
void blitterStartup(void);
void blitterShutdown(void);
void blitterLineMode(void);
void blitFinishBlit(void);
void blitForceFinish(void);
void blitterCopy(void);

#endif
