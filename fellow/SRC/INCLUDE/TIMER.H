#ifndef TIMER_H
#define TIMER_H

extern ULO timerGetTimeMs(void);
void timerSetUse50HzTimer(BOOLE use_50hz_timer);
BOOLE timerGetUse50HzTimer(void);
void timerEndOfFrame(void);
void timerEmulationStart(void);
void timerEmulationStop(void);
void timerStartup(void);
void timerShutdown(void);

#endif
