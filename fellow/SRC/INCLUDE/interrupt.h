#ifndef INTERRUPT_H
#define INTERRUPT_H

extern UWO intena;

void interruptHandleEvent(void);
void interruptRaisePending(void);
STR *interruptGetInterruptName(ULO interrupt_number);
BOOLE interruptIsRequested(UWO bitmask);

// Fellow standard module events

void interruptSoftReset(void);
void interruptHardReset(void);
void interruptEmulationStart(void);
void interruptEmulationStop(void);
void interruptStartup(void);
void interruptShutdown(void);

#endif
