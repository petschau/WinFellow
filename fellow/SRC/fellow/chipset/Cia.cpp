/*=========================================================================*/
/* Fellow                                                                  */
/* Cia emulation                                                           */
/*                                                                         */
/* Authors: Petter Schau                                                   */
/*          Rainer Sinsch                                                  */
/*          Marco Nova (novamarco@hotmail.com)                             */
/*                                                                         */
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

#include "fellow/api/defs.h"
#include "fellow/scheduler/Scheduler.h"
#include "fellow/application/Gameport.h"
#include "fellow/memory/Memory.h"
#include "fellow/chipset/Floppy.h"
#include "fellow/chipset/Cia.h"

#include "fellow/cpu/CpuModule.h"
#include "fellow/application/Interrupt.h"

using namespace fellow::api;
using namespace fellow::api::vm;

#define CIA_NO_EVENT 0
#define CIAA_TA_TIMEOUT_EVENT 1
#define CIAA_TB_TIMEOUT_EVENT 2
#define CIAB_TA_TIMEOUT_EVENT 3
#define CIAB_TB_TIMEOUT_EVENT 4
#define CIA_RECHECK_IRQ_EVENT 5

#define CIA_TA_IRQ 1
#define CIA_TB_IRQ 2
#define CIA_ALARM_IRQ 4
#define CIA_KBD_IRQ 8
#define CIA_FLAG_IRQ 16

// #define CIA_LOGGING

#ifdef CIA_LOGGING
extern ULO cpuGetOriginalPC();
#endif

ULO Cia::GetBusCycleRatio()
{
  return _scheduler->GetCycleFromCycle280ns(5);
}

bool Cia::IsSoundFilterEnabled()
{
  return (_cia[0].pra & 2) == 2;
}

bool Cia::IsPowerLEDEnabled()
{
  return IsSoundFilterEnabled();
}

/* Translate timer -> cycles until timeout from current sof, start of frame */

ULO Cia::UnstabilizeValue(ULO value, ULO remainder)
{
  return (value * GetBusCycleRatio()) + _scheduler->GetFrameCycle() + remainder;
}

/* Translate cycles until timeout from current sof -> timer value */

ULO Cia::StabilizeValue(ULO value)
{
  return (value - _scheduler->GetFrameCycle()) / GetBusCycleRatio();
}

ULO Cia::StabilizeValueRemainder(ULO value)
{
  return (value - _scheduler->GetFrameCycle()) % GetBusCycleRatio();
}

ULO Cia::GetTimerValue(ULO value)
{
  if (value == 0)
  {
#ifdef CIA_LOGGING
    Service->Log.AddLogDebug("CIA Warning timer latch is zero. PC %X", cpuGetOriginalPC());
#endif
    return 1; // Avoid getting stuck on zero timeout.
  }
  return value;
}

void Cia::TAStabilize(ULO i)
{
  if (_cia[i].cra & 1)
  {
    _cia[i].ta = StabilizeValue(_cia[i].taleft);
    _cia[i].ta_rem = StabilizeValueRemainder(_cia[i].taleft);
  }
  _cia[i].taleft = SchedulerEvent::EventDisableCycle;
}

void Cia::TBStabilize(ULO i)
{
  if ((_cia[i].crb & 0x41) == 1)
  { // Timer B started and not attached to timer A
    _cia[i].tb = StabilizeValue(_cia[i].tbleft);
    _cia[i].tb_rem = StabilizeValueRemainder(_cia[i].tbleft);
  }
  _cia[i].tbleft = SchedulerEvent::EventDisableCycle;
}

void Cia::Stabilize(ULO i)
{
  TAStabilize(i);
  TBStabilize(i);
}

void Cia::TAUnstabilize(ULO i)
{
  if (_cia[i].cra & 1)
  {
    _cia[i].taleft = UnstabilizeValue(_cia[i].ta, _cia[i].ta_rem);
  }
}

void Cia::TBUnstabilize(ULO i)
{
  if ((_cia[i].crb & 0x41) == 1) // Timer B started and not attached to timer A
  {
    _cia[i].tbleft = UnstabilizeValue(_cia[i].tb, _cia[i].tb_rem);
  }
}

void Cia::Unstabilize(ULO i)
{
  TAUnstabilize(i);
  TBUnstabilize(i);
}

void Cia::UpdateIRQ(ULO i)
{
  if (_cia[i].icrreq & _cia[i].icrmsk)
  {
#ifdef CIA_LOGGING
    Service->Log.AddLogDebug("CIA %c IRQ, req is %X, icrmsk is %X\n", (i == 0) ? 'A' : 'B', _cia[i].icrreq, _cia[i].icrmsk);
#endif

    _cia[i].icrreq |= 0x80;
    UWO mask = (i == 0) ? 0x0008 : 0x2000;
    if (!interruptIsRequested(mask))
    {
      wintreq_direct(mask | 0x8000, 0xdff09c, true);
    }
  }
}

void Cia::RaiseIRQ(ULO i, ULO req)
{
  _cia[i].icrreq |= (req & 0x1f);
  UpdateIRQ(i);
}

/* Helps the floppy loader, Cia B Flag IRQ */

void Cia::RaiseIndexIRQ()
{
  RaiseIRQ(1, CIA_FLAG_IRQ);
}

void Cia::CheckAlarmMatch(ULO i)
{
  if (_cia[i].evwritelatching)
  {
    return;
  }

  bool hasRealMatch = _cia[i].ev == _cia[i].evalarm;
  bool hasBuggyMatch = (!(_cia[i].ev & 0x000fff)) && ((_cia[i].ev - 1) & 0xfff000) == _cia[i].evalarm;

  if (hasRealMatch || hasBuggyMatch)
  {
#ifdef CIA_LOGGING
    Service->Log.AddLogDebug("CIA %c Alarm IRQ\n", (i == 0) ? 'A' : 'B');
#endif

    RaiseIRQ(i, CIA_ALARM_IRQ);
  }
}

/* Timeout handlers */

void Cia::HandleTBTimeout(ULO i)
{
#ifdef CIA_LOGGING
  Service->Log.AddLogDebug(
      "CIA %c Timer B expired in %s mode, reloading %X\n", (i == 0) ? 'A' : 'B', (_cia[i].crb & 8) ? "one-shot" : "continuous", _cia[i].tblatch);
#endif

  _cia[i].tb = GetTimerValue(_cia[i].tblatch); /* Reload from latch */

  if (_cia[i].crb & 8) /* One Shot Mode */
  {
    _cia[i].crb &= 0xfe; /* Stop timer */
    _cia[i].tbleft = SchedulerEvent::EventDisableCycle;
  }
  else if (!(_cia[i].crb & 0x40)) /* Continuous mode, no attach */
  {
    _cia[i].tbleft = UnstabilizeValue(_cia[i].tb, 0);
  }

  RaiseIRQ(i, CIA_TB_IRQ); /* Raise irq */
}

void Cia::HandleTATimeout(ULO i)
{
#ifdef CIA_LOGGING
  Service->Log.AddLogDebug(
      "CIA %c Timer A expired in %s mode, reloading %X\n", (i == 0) ? 'A' : 'B', (_cia[i].cra & 8) ? "one-shot" : "continuous", _cia[i].talatch);
#endif

  _cia[i].ta = GetTimerValue(_cia[i].talatch); /* Reload from latch */
  if ((_cia[i].crb & 0x41) == 0x41)
  { /* Timer B attached and started */
    _cia[i].tb = (_cia[i].tb - 1) & 0xffff;
    if (_cia[i].tb == 0)
    {
      HandleTBTimeout(i);
    }
  }
  if (_cia[i].cra & 8) /* One Shot Mode */
  {
    _cia[i].cra &= 0xfe; /* Stop timer */
    _cia[i].taleft = SchedulerEvent::EventDisableCycle;
  }
  else /* Continuous mode */
  {
    _cia[i].taleft = UnstabilizeValue(_cia[i].ta, 0);
  }

#ifdef CIA_LOGGING
  Service->Log.AddLogDebug("CIA %c Timer A attempt to raise irq, TA icr mask is %d\n", (i == 0) ? 'A' : 'B', _cia[i].icrmsk & 1);
#endif
  RaiseIRQ(i, CIA_TA_IRQ); /* Raise irq */
}

/* Called from eol-handler (B) or eof handler (A) */

void Cia::UpdateEventCounter(ULO i)
{
  if (!_cia[i].evwritelatching)
  {
    _cia[i].ev = (_cia[i].ev + 1) & 0xffffff;
    CheckAlarmMatch(i);
  }
}

/* Called from the eof-handler to update timers */

void Cia::UpdateTimersEOF()
{
  ULO cyclesInFrame = _scheduler->GetCyclesInFrame();
  for (auto &state : _cia)
  {
    if (state.taleft >= 0)
    {
      if ((state.taleft -= cyclesInFrame) < 0)
      {
        state.taleft = 0;
      }
    }
    if (state.tbleft >= 0)
    {
      if ((state.tbleft -= cyclesInFrame) < 0)
      {
        state.tbleft = 0;
      }
    }
  }

  if (_recheck_irq)
  {
    _recheck_irq_time -= cyclesInFrame;
  }

  _scheduler->RebaseForNewFrame(&ciaEvent);

  UpdateEventCounter(0);
}

/* Record next timer timeout */

void Cia::EventSetup()
{
  if (ciaEvent.IsEnabled())
  {
    _scheduler->InsertEvent(&ciaEvent);
  }
}

void Cia::SetupNextEvent()
{
  ULO nextevtime = SchedulerEvent::EventDisableCycle, nextevtype = CIA_NO_EVENT;

  if (_recheck_irq)
  {
    nextevtime = _recheck_irq_time;
    nextevtype = CIA_RECHECK_IRQ_EVENT;
  }

  for (ULO i = 0; i < 2; i++)
  {
    if (((ULO)_cia[i].taleft) < nextevtime)
    {
      nextevtime = _cia[i].taleft;
      nextevtype = (i * 2) + 1;
    }
    if (((ULO)_cia[i].tbleft) < nextevtime)
    {
      nextevtime = _cia[i].tbleft;
      nextevtype = (i * 2) + 2;
    }
  }

  if (ciaEvent.IsEnabled())
  {
    _scheduler->RemoveEvent(&ciaEvent);
    ciaEvent.Disable();
  }

  ciaEvent.cycle = nextevtime;
  _next_event_type = nextevtype;
  EventSetup();
}

void Cia::HandleEvent()
{
  ciaEvent.Disable();
  switch (_next_event_type)
  {
    case CIAA_TA_TIMEOUT_EVENT: HandleTATimeout(0); break;
    case CIAA_TB_TIMEOUT_EVENT: HandleTBTimeout(0); break;
    case CIAB_TA_TIMEOUT_EVENT: HandleTATimeout(1); break;
    case CIAB_TB_TIMEOUT_EVENT: HandleTBTimeout(1); break;
    case CIA_RECHECK_IRQ_EVENT:
      _recheck_irq = false;
      _recheck_irq_time = SchedulerEvent::EventDisableCycle;
      UpdateIRQ(0);
      UpdateIRQ(1);
      break;
    default: break;
  }
  SetupNextEvent();
}

void Cia::RecheckIRQ()
{
  _recheck_irq = true;
  _recheck_irq_time = _scheduler->GetFrameCycle() + _scheduler->GetCycleFromCycle280ns(10);
  SetupNextEvent();
}

/* PRA */

UBY Cia::ReadApra()
{
  UBY result = 0;

  if (gameport_autofire0[0])
  {
    gameport_fire0[0] = !gameport_fire0[0];
  }
  if (gameport_autofire0[1])
  {
    gameport_fire0[1] = !gameport_fire0[1];
  }

  if (!gameport_fire0[0])
  {
    result |= 0x40; /* Two firebuttons on port 1 */
  }
  if (!gameport_fire0[1])
  {
    result |= 0x80;
  }
  ULO drivesel = floppySelectedGet(); /* Floppy bits */

  if (!floppyIsReady(drivesel))
  {
    result |= 0x20;
  }
  if (!floppyIsTrack0(drivesel))
  {
    result |= 0x10;
  }
  if (!floppyIsWriteProtected(drivesel))
  {
    result |= 8;
  }
  if (!floppyIsChanged(drivesel))
  {
    result |= 4;
  }
  return result | (UBY)(_cia[0].pra & 2);
}

UBY Cia::ReadBpra()
{
  return (UBY)_cia[1].pra;
}

UBY Cia::Readpra(ULO i)
{
  if (i == 0)
  {
    return ReadApra();
  }

  return ReadBpra();
}

void Cia::WriteApra(UBY data)
{
  if ((data & 0x1) && !(_cia[0].pra & 0x1))
  {
    memoryChipMap(true);
  }
  else if ((_cia[0].pra & 0x1) && !(data & 0x1))
  {
    memoryChipMap(false);
  }
  _cia[0].pra = data;
}

void Cia::WriteBpra(UBY data)
{
  _cia[1].pra = data;
}

void Cia::Writepra(ULO i, UBY data)
{
  if (i == 0)
  {
    WriteApra(data);
  }
  else
  {
    WriteBpra(data);
  }
}

/* PRB */

UBY Cia::Readprb(ULO i)
{
  return _cia[i].prb;
}

void Cia::WriteAprb(UBY data)
{
  _cia[0].prb = data;
}

// Motor, drive latches this value when SEL goes from high to low

void Cia::WriteBprb(UBY data)
{
  int j = 0;
  BOOLE motor_was_high = (_cia[1].prb & 0x80) == 0x80;
  BOOLE motor_is_high = (data & 0x80) == 0x80;

  for (int i = 8; i < 0x80; i <<= 1, j++)
  {
    BOOLE sel_was_high = _cia[1].prb & i;
    BOOLE sel_is_high = data & i;
    if (sel_was_high && !sel_is_high)
    {
      // Motor is latched when sel goes from high to low
      // According to HRM motor bit must be set up in advance by software
      if (!motor_was_high || !motor_is_high)
      {
        floppyMotorSet(j, false); // Active low, motor is on
      }
      else if (motor_was_high)
      {
        floppyMotorSet(j, true); // Active low, motor is off
      }
    }
  }
  _cia[1].prb = data;
  floppySelectedSet((data & 0x78) >> 3);
  floppySideSet((data & 4) >> 2);
  floppyDirSet((data & 2) >> 1);
  floppyStepSet(data & 1);
}

void Cia::Writeprb(ULO i, UBY data)
{
  if (i == 0)
  {
    WriteAprb(data);
  }
  else
  {
    WriteBprb(data);
  }
}

/* DDRA */

UBY Cia::Readddra(ULO i)
{
  if (i == 0)
  {
    return 3;
  }

  return 0xff;
}

void Cia::Writeddra(ULO i, UBY data)
{
}

/* DDRB */

UBY Cia::Readddrb(ULO i)
{
  if (i == 0)
  {
    return _cia[0].ddrb;
  }

  return 0xff;
}

void Cia::Writeddrb(ULO i, UBY data)
{
  if (i == 0)
  {
    _cia[0].ddrb = data;
  }
}

/* SP (Keyboard serial data on Cia A) */

UBY Cia::Readsp(ULO i)
{
  return _cia[i].sp;
}

void Cia::Writesp(ULO i, UBY data)
{
  _cia[i].sp = data;
}

/* Timer A */

UBY Cia::Readtalo(ULO i)
{
  if (_cia[i].cra & 1)
  {
    return (UBY)StabilizeValue(_cia[i].taleft);
  }
  return (UBY)_cia[i].ta;
}

UBY Cia::Readtahi(ULO i)
{
  if (_cia[i].cra & 1)
  {
    return (UBY)(StabilizeValue(_cia[i].taleft) >> 8);
  }
  return (UBY)(_cia[i].ta >> 8);
}

void Cia::Writetalo(ULO i, UBY data)
{
  _cia[i].talatch = (_cia[i].talatch & 0xff00) | (ULO)data;

#ifdef CIA_LOGGING
  Service->Log.AddLogDebug("CIA %c Timer A written (low-part): %X PC %X\n", (i == 0) ? 'A' : 'B', _cia[i].talatch, cpuGetOriginalPC());
#endif
}

bool Cia::MustReloadOnTHiWrite(UBY cr)
{
  // Reload when not started, or one-shot mode
  return !(cr & 1) || (cr & 8);
}

void Cia::Writetahi(ULO i, UBY data)
{
  _cia[i].talatch = (_cia[i].talatch & 0xff) | (((ULO)data) << 8);

  if (MustReloadOnTHiWrite(_cia[i].cra)) // Reload when not started, or one-shot mode
  {
    _cia[i].ta = GetTimerValue(_cia[i].talatch);
    _cia[i].ta_rem = 0;
    _cia[i].taleft = SchedulerEvent::EventDisableCycle;
  }

#ifdef CIA_LOGGING
  Service->Log.AddLogDebug("CIA %c Timer A written (hi-part): %X PC %X\n", (i == 0) ? 'A' : 'B', _cia[i].talatch, cpuGetOriginalPC());
#endif

  if (_cia[i].cra & 8) // Timer A is one-shot, write starts it
  {
    _cia[i].cra |= 1;
    Unstabilize(i);
    SetupNextEvent();

#ifdef CIA_LOGGING
    Service->Log.AddLogDebug("CIA %c Timer A one-shot mode automatically started PC %X\n", (i == 0) ? 'A' : 'B', cpuGetOriginalPC());
#endif
  }
}

/* Timer B */

UBY Cia::Readtblo(ULO i)
{
  if ((_cia[i].crb & 1) && !(_cia[i].crb & 0x40))
  {
    return (UBY)StabilizeValue(_cia[i].tbleft);
  }

  return (UBY)_cia[i].tb;
}

UBY Cia::Readtbhi(ULO i)
{
  if ((_cia[i].crb & 1) && !(_cia[i].crb & 0x40))
  {
    return (UBY)(StabilizeValue(_cia[i].tbleft) >> 8);
  }

  return (UBY)(_cia[i].tb >> 8);
}

void Cia::Writetblo(ULO i, UBY data)
{
  _cia[i].tblatch = (_cia[i].tblatch & 0xff00) | ((ULO)data);
#ifdef CIA_LOGGING
  Service->Log.AddLogDebug("CIA %c Timer B written (low-part): %X PC %X\n", (i == 0) ? 'A' : 'B', _cia[i].tblatch, cpuGetOriginalPC());
#endif
}

void Cia::Writetbhi(ULO i, UBY data)
{
  _cia[i].tblatch = (_cia[i].tblatch & 0xff) | (((ULO)data) << 8);

  if (MustReloadOnTHiWrite(_cia[i].crb)) // Reload when not started, or one-shot mode
  {
    _cia[i].tb = GetTimerValue(_cia[i].tblatch);
    _cia[i].tb_rem = 0;
    _cia[i].tbleft = SchedulerEvent::EventDisableCycle;
  }

#ifdef CIA_LOGGING
  Service->Log.AddLogDebug("CIA %c Timer B (hi-part) written: %X PC %X\n", (i == 0) ? 'A' : 'B', _cia[i].tblatch, cpuGetOriginalPC());
#endif
  if (_cia[i].crb & 8) // Timer B is one-shot, write starts it
  {
    _cia[i].crb |= 1;
    Unstabilize(i);
    SetupNextEvent();
#ifdef CIA_LOGGING
    Service->Log.AddLogDebug("CIA %c Timer B one-shot mode automatically started. PC %X\n", (i == 0) ? 'A' : 'B', cpuGetOriginalPC());
#endif
  }
}

/* Event counter */

UBY Cia::Readevlo(ULO i)
{
  if (_cia[i].evlatching)
  {
    _cia[i].evlatching = false;
    return (UBY)_cia[i].evlatch;
  }
  return (UBY)_cia[i].ev;
}

UBY Cia::Readevmi(ULO i)
{
  if (_cia[i].evlatching)
  {
    return (UBY)(_cia[i].evlatch >> 8);
  }
  return (UBY)(_cia[i].ev >> 8);
}

UBY Cia::Readevhi(ULO i)
{
  _cia[i].evlatching = true;
  _cia[i].evlatch = _cia[i].ev;
  return (UBY)(_cia[i].ev >> 16);
}

void Cia::Writeevlo(ULO i, UBY data)
{
  if (_cia[i].crb & 0x80) // Alarm
  {
    _cia[i].evalarm = (_cia[i].evalarm & 0xffff00) | (ULO)data;
  }
  else // Time of day
  {
    _cia[i].evwritelatching = false;
    _cia[i].evwritelatch = (_cia[i].evwritelatch & 0xffff00) | (ULO)data;
    _cia[i].ev = _cia[i].evwritelatch;
  }

  if ((cpuGetPC() & 0xf80000) == 0xf80000 && _cia[i].ev == 0 && _cia[i].evalarm == 0) return;

  CheckAlarmMatch(i);
}

void Cia::Writeevmi(ULO i, UBY data)
{
  if (_cia[i].crb & 0x80) // Alarm
  {
    _cia[i].evalarm = (_cia[i].evalarm & 0xff00ff) | ((ULO)data << 8);
    CheckAlarmMatch(i);
  }
  else // Time of day
  {
    _cia[i].evwritelatching = true;
    _cia[i].evwritelatch = (_cia[i].evwritelatch & 0xff00ff) | (((ULO)data) << 8);
  }
}

void Cia::Writeevhi(ULO i, UBY data)
{
  if (_cia[i].crb & 0x80) // Alarm
  {
    _cia[i].evalarm = (_cia[i].evalarm & 0xffff) | ((ULO)data << 16);
    CheckAlarmMatch(i);
  }
  else // Time of day
  {
    _cia[i].evwritelatching = true;
    _cia[i].evwritelatch = (_cia[i].evwritelatch & 0xffff) | (((ULO)data) << 16);
  }
}

/* ICR */

UBY Cia::Readicr(ULO i)
{
  UBY tmp = _cia[i].icrreq;
  _cia[i].icrreq = 0;

#ifdef CIA_LOGGING
  Service->Log.AddLogDebug("CIA %c ICR read, req is %X\n", (i == 0) ? 'A' : 'B', _cia[i].icrreq);
#endif

  return tmp;
}

void Cia::Writeicr(ULO i, UBY data)
{
#ifdef CIA_LOGGING
  ULO old = _cia[i].icrmsk;
#endif

  if (data & 0x80)
  {
    _cia[i].icrmsk |= (data & 0x1f);
  }
  else
  {
    _cia[i].icrmsk &= ~(data & 0x1f);
  }

  UpdateIRQ(i);

#ifdef CIA_LOGGING
  Service->Log.AddLogDebug("CIA %c IRQ mask data %X, mask was %X is %X. PC %X\n", (i == 0) ? 'A' : 'B', data, old, _cia[i].icrmsk, cpuGetOriginalPC());
#endif
}

/* CRA */

UBY Cia::Readcra(ULO i)
{
  return _cia[i].cra;
}

void Cia::Writecra(ULO i, UBY data)
{
  Stabilize(i);
  if (data & 0x10) // Force load
  {
    _cia[i].ta = GetTimerValue(_cia[i].talatch);
    _cia[i].ta_rem = 0;
    data &= 0xef; // Clear force load bit
#ifdef CIA_LOGGING
    Service->Log.AddLogDebug("CIA %c Timer A force load %X. PC %X\n", (i == 0) ? 'A' : 'B', _cia[i].ta, cpuGetOriginalPC());
#endif
  }
#ifdef CIA_LOGGING
  if ((data & 1) != (_cia[i].cra & 1))
  {
    Service->Log.AddLogDebug(
        "CIA %c Timer A is %s, was %s. PC %X\n",
        (i == 0) ? 'A' : 'B',
        (data & 1) ? "started" : "stopped",
        (_cia[i].cra & 1) ? "started" : "stopped",
        cpuGetOriginalPC());
  }
#endif
  _cia[i].cra = data;
  Unstabilize(i);
  SetupNextEvent();
}

/* CRB */

UBY Cia::Readcrb(ULO i)
{
  return _cia[i].crb;
}

void Cia::Writecrb(ULO i, UBY data)
{
  Stabilize(i);
  if (data & 0x10) // Force load
  {
    _cia[i].tb = GetTimerValue(_cia[i].tblatch);
    _cia[i].tb_rem = 0;
    data &= 0xef; // Clear force load bit
#ifdef CIA_LOGGING
    Service->Log.AddLogDebug("CIA %c Timer B force load %X. PC %X\n", (i == 0) ? 'A' : 'B', _cia[i].tb, cpuGetOriginalPC());
#endif
  }
#ifdef CIA_LOGGING
  if ((data & 1) != (_cia[i].cra & 1))
  {
    Service->Log.AddLogDebug(
        "CIA %c Timer B is %s, was %s. PC %X\n",
        (i == 0) ? 'A' : 'B',
        (data & 1) ? "started" : "stopped",
        (_cia[i].crb & 1) ? "started" : "stopped",
        cpuGetOriginalPC());
  }
#endif
  _cia[i].crb = data;
  Unstabilize(i);
  SetupNextEvent();
}

/* Dummy read and write */

UBY Cia::ReadNothing(ULO i)
{
  return 0xff;
}

void Cia::WriteNothing(ULO i, UBY data)
{
}

UBY Cia::ReadByte(ULO address)
{
  if ((address & 0xa01001) == 0xa00001)
  {
    return _read[(address & 0xf00) >> 8](0);
  }

  if ((address & 0xa02001) == 0xa00000)
  {
    return _read[(address & 0xf00) >> 8](1);
  }

  return 0xff;
}

void Cia::WriteByte(UBY data, ULO address)
{
  if ((address & 0xa01001) == 0xa00001)
  {
    _write[(address & 0xf00) >> 8](0, data);
  }
  else if ((address & 0xa02001) == 0xa00000)
  {
    _write[(address & 0xf00) >> 8](1, data);
  }
}

void Cia::StateClear()
{
  for (auto &state : _cia)
  {
    state.ev = 0;
    state.evlatch = 0;
    state.evlatching = false;
    state.evalarm = 0;
    state.evwritelatch = 0;
    state.evwritelatching = true; // Turns off tod counting until a value is set by a program
    state.taleft = SchedulerEvent::EventDisableCycle;
    state.tbleft = SchedulerEvent::EventDisableCycle;
    state.ta = 0xffff;
    state.tb = 0xffff;
    state.talatch = 0xffff;
    state.tblatch = 0xffff;
    state.pra = 0xff;
    state.prb = 0;
    state.ddra = 0;
    state.ddrb = 0;
    state.icrreq = 0;
    state.icrmsk = 0;
    state.cra = 0;
    state.crb = 0;
  }
  _recheck_irq = false;
  _recheck_irq_time = SchedulerEvent::EventDisableCycle;
  _next_event_type = 0;
}

void Cia::InitializeInternalReadWriteFunctions()
{
  _read[0] = [this](ULO ciaIndex) -> UBY { return this->Readpra(ciaIndex); };
  _read[1] = [this](ULO ciaIndex) -> UBY { return this->Readprb(ciaIndex); };
  _read[2] = [this](ULO ciaIndex) -> UBY { return this->Readddra(ciaIndex); };
  _read[3] = [this](ULO ciaIndex) -> UBY { return this->Readddrb(ciaIndex); };
  _read[4] = [this](ULO ciaIndex) -> UBY { return this->Readtalo(ciaIndex); };
  _read[5] = [this](ULO ciaIndex) -> UBY { return this->Readtahi(ciaIndex); };
  _read[6] = [this](ULO ciaIndex) -> UBY { return this->Readtblo(ciaIndex); };
  _read[7] = [this](ULO ciaIndex) -> UBY { return this->Readtbhi(ciaIndex); };
  _read[8] = [this](ULO ciaIndex) -> UBY { return this->Readevlo(ciaIndex); };
  _read[9] = [this](ULO ciaIndex) -> UBY { return this->Readevmi(ciaIndex); };
  _read[10] = [this](ULO ciaIndex) -> UBY { return this->Readevhi(ciaIndex); };
  _read[11] = [this](ULO ciaIndex) -> UBY { return this->ReadNothing(ciaIndex); };
  _read[12] = [this](ULO ciaIndex) -> UBY { return this->Readsp(ciaIndex); };
  _read[13] = [this](ULO ciaIndex) -> UBY { return this->Readicr(ciaIndex); };
  _read[14] = [this](ULO ciaIndex) -> UBY { return this->Readcra(ciaIndex); };
  _read[15] = [this](ULO ciaIndex) -> UBY { return this->Readcrb(ciaIndex); };

  _write[0] = [this](ULO ciaIndex, UBY data) -> void { this->Writepra(ciaIndex, data); };
  _write[1] = [this](ULO ciaIndex, UBY data) -> void { this->Writeprb(ciaIndex, data); };
  _write[2] = [this](ULO ciaIndex, UBY data) -> void { this->Writeddra(ciaIndex, data); };
  _write[3] = [this](ULO ciaIndex, UBY data) -> void { this->Writeddrb(ciaIndex, data); };
  _write[4] = [this](ULO ciaIndex, UBY data) -> void { this->Writetalo(ciaIndex, data); };
  _write[5] = [this](ULO ciaIndex, UBY data) -> void { this->Writetahi(ciaIndex, data); };
  _write[6] = [this](ULO ciaIndex, UBY data) -> void { this->Writetblo(ciaIndex, data); };
  _write[7] = [this](ULO ciaIndex, UBY data) -> void { this->Writetbhi(ciaIndex, data); };
  _write[8] = [this](ULO ciaIndex, UBY data) -> void { this->Writeevlo(ciaIndex, data); };
  _write[9] = [this](ULO ciaIndex, UBY data) -> void { this->Writeevmi(ciaIndex, data); };
  _write[10] = [this](ULO ciaIndex, UBY data) -> void { this->Writeevhi(ciaIndex, data); };
  _write[11] = [this](ULO ciaIndex, UBY data) -> void { this->WriteNothing(ciaIndex, data); };
  _write[12] = [this](ULO ciaIndex, UBY data) -> void { this->Writesp(ciaIndex, data); };
  _write[13] = [this](ULO ciaIndex, UBY data) -> void { this->Writeicr(ciaIndex, data); };
  _write[14] = [this](ULO ciaIndex, UBY data) -> void { this->Writecra(ciaIndex, data); };
  _write[15] = [this](ULO ciaIndex, UBY data) -> void { this->Writecrb(ciaIndex, data); };
}

void Cia::StartEmulation()
{
}

void Cia::StopEmulation()
{
}

void Cia::HardReset()
{
  StateClear();
}

void Cia::Startup()
{
  StateClear();
  InitializeInternalReadWriteFunctions();
};

void Cia::Shutdown()
{
}

Cia::Cia(Scheduler *scheduler) : _scheduler(scheduler)
{
}
