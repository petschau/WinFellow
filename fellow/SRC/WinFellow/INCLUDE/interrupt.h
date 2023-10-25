#pragma once

extern uint16_t intena;

void interruptHandleEvent();
void interruptRaisePending();
const char *interruptGetInterruptName(uint32_t interrupt_number);
BOOLE interruptIsRequested(uint16_t bitmask);

void wintreq_direct(uint16_t data, uint32_t address, bool delayIRQ);

// Fellow standard module events

void interruptSoftReset();
void interruptHardReset();
void interruptEmulationStart();
void interruptEmulationStop();
void interruptStartup();
void interruptShutdown();
