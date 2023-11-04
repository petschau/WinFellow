#pragma once

typedef void (*timerCallbackFunction)(uint32_t millisecondTicks);

extern void timerAddCallback(timerCallbackFunction callback);
extern uint32_t timerGetTimeMs();
void timerEmulationStart();
void timerEmulationStop();
void timerStartup();
void timerShutdown();
