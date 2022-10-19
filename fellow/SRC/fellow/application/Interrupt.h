#pragma once

extern UWO intena;

void interruptHandleEvent();
void interruptRaisePending();
const STR *interruptGetInterruptName(ULO interrupt_number);
BOOLE interruptIsRequested(UWO bitmask);

void wintreq_direct(UWO data, ULO address, bool delayIRQ);

// Fellow standard module events

void interruptEndOfFrame();
void interruptSoftReset();
void interruptHardReset();
void interruptEmulationStart();
void interruptEmulationStop();
void interruptStartup();
void interruptShutdown();
