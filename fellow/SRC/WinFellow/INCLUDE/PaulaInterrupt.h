#pragma once

#include <cstdint>
#include "Defs.h"
#include "CustomChipset/IPaula.h"
#include "Scheduler/SchedulerEvent.h"

extern uint16_t intena;

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

class Paula : public IPaula
{
private:
  SchedulerEvent &_interruptEvent;

  void HandleInterruptEvent();

public:
  void InitializeInterruptEvent() override;

  Paula(SchedulerEvent &interruptEvent);
  virtual ~Paula();
};