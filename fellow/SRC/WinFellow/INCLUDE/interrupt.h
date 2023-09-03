#ifndef INTERRUPT_H
#define INTERRUPT_H

extern UWO intena;

void interruptHandleEvent(void);
void interruptRaisePending();
STR *interruptGetInterruptName(ULO interrupt_number);
BOOLE interruptIsRequested(UWO bitmask);

void wintreq_direct(UWO data, ULO address, bool delayIRQ);

// Fellow standard module events

void interruptSoftReset(void);
void interruptHardReset(void);
void interruptEmulationStart(void);
void interruptEmulationStop(void);
void interruptStartup(void);
void interruptShutdown(void);

#endif
