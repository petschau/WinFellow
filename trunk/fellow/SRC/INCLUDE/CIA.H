#ifndef CIA_H
#define CIA_H

extern void ciaSaveState(FILE *F);
extern void ciaLoadState(FILE *F);
extern void ciaHardReset(void);
extern void ciaEmulationStart(void);
extern void ciaEmulationStop(void);
extern void ciaStartup(void);
extern void ciaShutdown(void);
extern void ciaMemoryMap(void);
extern void ciaHandleEvent(void);
extern void ciaRaiseIndexIRQ(void); /* For the floppy loader */
extern void ciaRaiseIRQ(ULO i, ULO req); /* For kbd.c */
extern void ciaWritesp(ULO i, UBY data); /* For kbd.c */
extern void ciaUpdateEventCounter(ULO);
extern void ciaUpdateTimersEOF(void);
extern void ciaUpdateIRQ(ULO i);

extern BOOLE ciaIsSoundFilterEnabled(void);

#endif
