#ifndef BLIT_H
#define BLIT_H

/*==========================*/
/* Global blitter variables */
/*==========================*/

extern LON blitend;
extern ULO blitterstatus;
extern ULO bltzero;
extern BOOLE blitter_ECS;
extern ULO blit_cycle_length;
extern ULO blit_cycle_free;
extern ULO blitterdmawaiting;
extern BOOLE blit_started;

/*===========================================================================*/
/* Blitter-properties                                                        */
/*===========================================================================*/

extern void blitterSetFast(BOOLE fast);
extern BOOLE blitterGetFast(void);
extern void blitterSetECS(BOOLE ECS);
extern BOOLE blitterGetECS(void);

extern void blitterSetOperationLog(BOOLE operation_log);
extern BOOLE blitterGetOperationLog(void);


/*===========================================================================*/
/* Declare C blitter functions                                               */
/*===========================================================================*/

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
