/*===========================================================================*/
/* Fellow Amiga Emulator                                                     */
/* Provides timer routines                                                   */
/* Author:  Petter Schau (peschau@online.no)                                 */
/*                                                                           */
/* This file is under the GNU Public License (GPL)                           */
/*===========================================================================*/

#include "defs.h"
#include "sound.h"
#include <windows.h>

BOOLE timer_use_50hz_timer;
BOOLE timer_running;
ULO timer_mmresolution;
ULO timer_mmtimer;
ULO timer_ticks;
HANDLE timer_event;


/*===========================================================================*/
/* Returns current time in ms                                                */
/*===========================================================================*/

ULO timerGetTimeMs(void) {
  return timeGetTime();
}

/*==========================================================================*/
/* Config settings                                                          */
/*==========================================================================*/

void timerSetUse50HzTimer(BOOLE use_50hz_timer)
{
  timer_use_50hz_timer = use_50hz_timer;
}

BOOLE timerGetUse50HzTimer(void)
{
  return timer_use_50hz_timer;
}

/*==========================================================================*/
/* Multimedia Callback fnc.                                                 */
/*==========================================================================*/

void CALLBACK timerCallback(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
  if (timer_ticks < 20)
  {
    timer_ticks++;
    if (timer_ticks == 20) SetEvent(timer_event);
  }
}


/*===========================================================================*/
/* Fellow module functions                                                   */
/*===========================================================================*/

void timerEndOfFrame(void)
{
  if (timer_running && timer_use_50hz_timer)
  {
    WaitForSingleObject(timer_event, INFINITE);
    timer_ticks = 0;
  }
}

void timerEmulationStart(void)
{
  if (timer_use_50hz_timer && (soundGetEmulation() != SOUND_PLAY))
  {
    TIMECAPS timecaps;
    MMRESULT mmres;
    char s[100];

    timer_ticks = 0;
    mmres = timeGetDevCaps(&timecaps, sizeof(TIMECAPS));
    if(mmres != TIMERR_NOERROR)
    {
      fellowAddLog("timer: timerEmulationStart() timeGetDevCaps() failed\n");
      timer_running = FALSE;
      return;
    }

    sprintf(s, "timer: timerEmulationStart() timeGetDevCaps: min: %d, max %d\n", timecaps.wPeriodMin, timecaps.wPeriodMax);
    fellowAddLog(s);

    timer_mmresolution = timecaps.wPeriodMin;

    mmres = timeBeginPeriod(timer_mmresolution);
    if(mmres != TIMERR_NOERROR)
    {
      fellowAddLog("timer: timerEmulationStart() timeBeginPeriod() failed\n");
      timer_running = FALSE;
      return;
    }

    mmres = timeSetEvent(1, 0, timerCallback, 0, TIME_PERIODIC);
    if(mmres == 0)
    {
      fellowAddLog("timer: timerEmulationStart() timeSetEvent() failed\n");
      timer_running = FALSE;
      return;
    }
    timer_mmtimer = mmres;
    timer_running = TRUE;
  }
}

void timerEmulationStop(void)
{
  if (timer_running)
  {
    MMRESULT mmres;
    mmres = timeKillEvent(timer_mmtimer);
  
    mmres = timeEndPeriod(timer_mmresolution);
    if(mmres != TIMERR_NOERROR)
    {
      fellowAddLog("timer: timerEmulationStop() timeEndPeriod() failed");
    }
    timer_running = FALSE;
  }
}

void timerStartup(void)
{
  timer_running = FALSE;
  timer_event = CreateEvent(0, FALSE, FALSE, 0);
  timerSetUse50HzTimer(TRUE);
}

void timerShutdown(void)
{
  if (timer_event) CloseHandle(timer_event);
}
