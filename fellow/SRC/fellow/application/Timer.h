#pragma once

#include "fellow/api/defs.h"

typedef void (*timerCallbackFunction)(ULO millisecondTicks);

extern void timerAddCallback(timerCallbackFunction callback);
extern ULO timerGetTimeMs();
void timerEmulationStart();
void timerEmulationStop();
void timerStartup();
void timerShutdown();
