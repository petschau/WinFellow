#ifndef TIMER_H
#define TIMER_H

extern ULO timerGetTimeMs(void);
void timerEmulationStart(void);
void timerEmulationStop(void);
void timerStartup(void);
void timerShutdown(void);

#endif
