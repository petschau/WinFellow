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
#include <windows.h>
#include "fellow/api/Services.h"
#include "fellow/os/windows/timer/TimerDriver.h"

using namespace std;
using namespace fellow::api;

//===========================
// Returns current time in ms
//===========================

ULO TimerDriver::GetTimeMs()
{
  return timeGetTime();
}

//=========================
// Multimedia Callback fnc.
//=========================

void CALLBACK TimerDriver::Callback(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
  ((TimerDriver *)dwUser)->HandleCallback();
}

void TimerDriver::HandleCallback()
{
  _ticks++;

  for (timerCallbackFunction callback : _callbacks)
  {
    callback(_ticks);
  }
}

void TimerDriver::AddCallback(timerCallbackFunction callback)
{
  _callbacks.push_back(callback);
}

void TimerDriver::EmulationStart()
{
  TIMECAPS timecaps{};

  _ticks = 0;
  MMRESULT mmres = timeGetDevCaps(&timecaps, sizeof(TIMECAPS));
  if (mmres != TIMERR_NOERROR)
  {
    Service->Log.AddLog("timer: timerEmulationStart() timeGetDevCaps() failed\n");
    _running = false;
    return;
  }

  Service->Log.AddLog("timer: timerEmulationStart() timeGetDevCaps: min: %u, max %u\n", timecaps.wPeriodMin, timecaps.wPeriodMax);

  _mmresolution = timecaps.wPeriodMin;

  mmres = timeBeginPeriod(_mmresolution);
  if (mmres != TIMERR_NOERROR)
  {
    Service->Log.AddLog("timer: timerEmulationStart() timeBeginPeriod() failed\n");
    _running = false;
    return;
  }

  mmres = timeSetEvent(1, 0, TimerDriver::Callback, (DWORD_PTR)this, (UINT)TIME_PERIODIC);
  if (mmres == 0)
  {
    Service->Log.AddLog("timer: timerEmulationStart() timeSetEvent() failed\n");
    _running = false;
    return;
  }
  _mmtimer = mmres;
  _running = true;
}

void TimerDriver::EmulationStop()
{
  if (_running)
  {
    MMRESULT mmres = timeKillEvent(_mmtimer);
    mmres = timeEndPeriod(_mmresolution);
    if (mmres != TIMERR_NOERROR)
    {
      Service->Log.AddLog("timer: timerEmulationStop() timeEndPeriod() failed, unable to restore previous timer resolution.");
    }
    _running = false;
  }

  _callbacks.clear();
}

void TimerDriver::Startup()
{
  _callbacks.clear();
  _running = false;
}

void TimerDriver::Shutdown()
{
}
