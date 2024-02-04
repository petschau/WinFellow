#pragma once

#include <cstdio>
#include <cstdint>
#include "Defs.h"

#include "CustomChipset/IBlitter.h"
#include "Scheduler/SchedulerEvent.h"

/*===========================================================================*/
/* Blitter-properties                                                        */
/*===========================================================================*/

extern void blitterSetFast(BOOLE fast);
extern BOOLE blitterGetFast();

extern void blitterSetOperationLog(BOOLE operation_log);
extern BOOLE blitterGetOperationLog();

extern BOOLE blitterGetDMAPending();
extern BOOLE blitterGetZeroFlag();
extern uint32_t blitterGetFreeCycles();
extern BOOLE blitterIsStarted();

/*===========================================================================*/
/* Declare C blitter functions                                               */
/*===========================================================================*/

void blitterEndOfFrame();
void blitterEmulationStart();
void blitterEmulationStop();
void blitterHardReset();
void blitterStartup();
void blitterShutdown();
void blitterLineMode();
void blitFinishBlit();
void blitForceFinish();
void blitterCopy();

class Blitter : public IBlitter
{
private:
  SchedulerEvent &_blitterEvent;

public:
  void InitializeBlitterEvent() override;

  Blitter(SchedulerEvent &blitterEvent);
  virtual ~Blitter();
};
