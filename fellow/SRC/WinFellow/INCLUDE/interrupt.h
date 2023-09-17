#ifndef INTERRUPT_H
#define INTERRUPT_H

extern uint16_t intena;

void interruptHandleEvent(void);
void interruptRaisePending();
STR *interruptGetInterruptName(uint32_t interrupt_number);
BOOLE interruptIsRequested(uint16_t bitmask);

void wintreq_direct(uint16_t data, uint32_t address, bool delayIRQ);

// Fellow standard module events

void interruptSoftReset(void);
void interruptHardReset(void);
void interruptEmulationStart(void);
void interruptEmulationStop(void);
void interruptStartup(void);
void interruptShutdown(void);

#endif
