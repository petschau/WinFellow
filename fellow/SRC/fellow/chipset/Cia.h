#pragma once

extern void ciaHardReset();
extern void ciaEmulationStart();
extern void ciaEmulationStop();
extern void ciaStartup();
extern void ciaShutdown();
extern void ciaMemoryMap();
extern void ciaHandleEvent();
extern void ciaRaiseIndexIRQ();          /* For the floppy loader */
extern void ciaRaiseIRQ(ULO i, ULO req); /* For kbd.c */
extern void ciaWritesp(ULO i, UBY data); /* For kbd.c */
extern void ciaUpdateEventCounter(ULO);
extern void ciaUpdateTimersEOF();
extern void ciaUpdateIRQ(ULO i);
extern void ciaRecheckIRQ();

extern bool ciaIsSoundFilterEnabled();
extern bool ciaIsPowerLEDEnabled();
