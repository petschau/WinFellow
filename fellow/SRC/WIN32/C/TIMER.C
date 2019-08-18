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
#include <list>
#include "fellow/api/defs.h"
#include "sound.h"
#include <windows.h>
#include "fellow/api/Services.h"
#include "TIMER.H"

using namespace fellow::api;

bool timer_running;
ULO timer_mmresolution;
ULO timer_mmtimer;
ULO timer_ticks;

std::list<timerCallbackFunction> timerCallbacks;

/*===========================================================================*/
/* Returns current time in ms                                                */
/*===========================================================================*/

ULO timerGetTimeMs()
{
  return timeGetTime();
}

/*==========================================================================*/
/* Multimedia Callback fnc.                                                 */
/*==========================================================================*/

void CALLBACK timerCallback(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
  timer_ticks++;

  for (timerCallbackFunction callback : timerCallbacks)
  {
    callback(timer_ticks);
  }
}

void timerAddCallback(timerCallbackFunction callback)
{
  timerCallbacks.push_back(callback);
}

/*===========================================================================*/
/* Fellow module functions                                                   */
/*===========================================================================*/

void timerEmulationStart()
{
  TIMECAPS timecaps;
  MMRESULT mmres;

  timer_ticks = 0;
  mmres = timeGetDevCaps(&timecaps, sizeof(TIMECAPS));
  if (mmres != TIMERR_NOERROR)
  {
    Service->Log.AddLog("timer: timerEmulationStart() timeGetDevCaps() failed\n");
    timer_running = false;
    return;
  }

  Service->Log.AddLog("timer: timerEmulationStart() timeGetDevCaps: min: %u, max %u\n", timecaps.wPeriodMin, timecaps.wPeriodMax);

  timer_mmresolution = timecaps.wPeriodMin;

  mmres = timeBeginPeriod(timer_mmresolution);
  if (mmres != TIMERR_NOERROR)
  {
    Service->Log.AddLog("timer: timerEmulationStart() timeBeginPeriod() failed\n");
    timer_running = false;
    return;
  }

  mmres = timeSetEvent(1, 0, timerCallback, (DWORD_PTR)0, (UINT)TIME_PERIODIC);
  if (mmres == 0)
  {
    Service->Log.AddLog("timer: timerEmulationStart() timeSetEvent() failed\n");
    timer_running = false;
    return;
  }
  timer_mmtimer = mmres;
  timer_running = true;
}

void timerEmulationStop()
{
  if (timer_running)
  {
    MMRESULT mmres = timeKillEvent(timer_mmtimer);
    mmres = timeEndPeriod(timer_mmresolution);
    if (mmres != TIMERR_NOERROR)
    {
      Service->Log.AddLog("timer: timerEmulationStop() timeEndPeriod() failed, unable to restore previous timer resolution.");
    }
    timer_running = false;
  }
  timerCallbacks.clear();
}

void timerStartup()
{
  timerCallbacks.clear();
  timer_running = false;
}

void timerShutdown()
{
}
