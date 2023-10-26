#pragma once

extern void ciaSaveState(FILE *F);
extern void ciaLoadState(FILE *F);
extern void ciaHardReset();
extern void ciaEmulationStart();
extern void ciaEmulationStop();
extern void ciaStartup();
extern void ciaShutdown();
extern void ciaMemoryMap();
extern void ciaHandleEvent();
extern void ciaRaiseIndexIRQ();                    /* For the floppy loader */
extern void ciaRaiseIRQ(uint32_t i, uint32_t req); /* For kbd.c */
extern void ciaWritesp(uint32_t i, uint8_t data);  /* For kbd.c */
extern void ciaUpdateEventCounter(uint32_t);
extern void ciaUpdateTimersEOF();
extern void ciaUpdateIRQ(uint32_t i);
extern void ciaRecheckIRQ();

extern BOOLE ciaIsSoundFilterEnabled();
