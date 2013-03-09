/* @(#) $Id: TIMER.C,v 1.7 2009-07-25 10:24:00 peschau Exp $ */
/*=========================================================================*/
/* Fellow                                                                  */
/* Provides timer routines                                                 */
/* Author:  Petter Schau (peschau@online.no)                               */
/*                                                                         */
/* Copyright (C) 1991, 1992, 1996 Free Software Foundation, Inc.           */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2, or (at your option)     */
/* any later version.                                                      */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software Foundation, */
/* Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.          */
/*=========================================================================*/
#include "defs.h"
#include "sound.h"
#include <windows.h>
#include "fellow.h"

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

void CALLBACK timerCallback(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
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

    sprintf(s, "timer: timerEmulationStart() timeGetDevCaps: min: %u, max %u\n", timecaps.wPeriodMin, timecaps.wPeriodMax);
    fellowAddLog(s);

    timer_mmresolution = timecaps.wPeriodMin;

    mmres = timeBeginPeriod(timer_mmresolution);
    if(mmres != TIMERR_NOERROR)
    {
      fellowAddLog("timer: timerEmulationStart() timeBeginPeriod() failed\n");
      timer_running = FALSE;
      return;
    }

    mmres = timeSetEvent(1, 0, timerCallback, (DWORD_PTR)0, (UINT)TIME_PERIODIC);
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
  timerSetUse50HzTimer(FALSE);
}

void timerShutdown(void)
{
  if (timer_event) CloseHandle(timer_event);
}
