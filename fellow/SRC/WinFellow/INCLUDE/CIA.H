#pragma once

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
extern void ciaRaiseIRQ(uint32_t i, uint32_t req); /* For kbd.c */
extern void ciaWritesp(uint32_t i, uint8_t data); /* For kbd.c */
extern void ciaUpdateEventCounter(uint32_t);
extern void ciaUpdateTimersEOF();
extern void ciaUpdateIRQ(uint32_t i);
extern void ciaRecheckIRQ();

extern BOOLE ciaIsSoundFilterEnabled(void);
