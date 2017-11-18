#ifndef TIMER_H
#define TIMER_H

typedef void(*timerCallbackFunction)(ULO millisecondTicks);

extern void timerAddCallback(timerCallbackFunction callback);
extern ULO timerGetTimeMs();
void timerEmulationStart();
void timerEmulationStop();
void timerStartup();
void timerShutdown();

#endif
