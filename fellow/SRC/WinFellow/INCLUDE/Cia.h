#pragma once

#include <cstdio>
#include <cstdint>

#include "Defs.h"
#include "IO/ICia.h"
#include "Scheduler/SchedulerEvent.h"

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
extern void ciaUpdateTimersEOF(uint32_t cyclesInEndedFrame);
extern void ciaUpdateIRQ(uint32_t i);
extern void ciaRecheckIRQ();
extern BOOLE ciaIsSoundFilterEnabled();

class Cia : public ICia
{
private:
  SchedulerEvent &_ciaEvent;

public:
  void InitializeCiaEvent() override;

  Cia(SchedulerEvent &ciaEvent);
  virtual ~Cia();
};