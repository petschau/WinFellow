/*=========================================================================*/
/* Fellow							           */
/*                                                                         */
/* Chip Bus Event Scheduler                                                */
/*                                                                         */
/* Author: Petter Schau                                                    */
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

#include <cassert>
#include <csetjmp>
#include "Scheduler.h"
#include "VirtualHost/Core.h"

#ifdef RETRO_PLATFORM
#include "fellow/os/windows/retroplatform/RetroPlatform.h"
#include "fellow/api/Drivers.h"
#endif

void Scheduler::InsertEvent(SchedulerEvent *ev)
{
  assert(ev->cycle.IsEnabledAndValid());
  assert(ev->cycle.IsNowOrLater(_clocks.GetMasterTime()));

  _queue.InsertEvent(ev);
}

void Scheduler::RemoveEvent(SchedulerEvent *ev)
{
  _queue.RemoveEvent(ev);
}

void Scheduler::NewFrame(const FrameParameters &newFrameParameters)
{
  const auto cyclesInEndedFrame = _currentFrameParameters.CyclesInFrame;
  _clocks.NewFrame(newFrameParameters);
  _queue.RebaseEvents(cyclesInEndedFrame);
}

void Scheduler::RequestEmulationStop()
{
  _stopEmulationRequested = true;
}

void Scheduler::ClearRequestEmulationStop()
{
  _stopEmulationRequested = false;
}

void Scheduler::HandleNextEvent()
{
  SchedulerEvent *e = _queue.PopEvent();
  _clocks.SetMasterTime(e->cycle);

  assert(_clocks.GetChipTime().Line < _currentFrameParameters.LinesInFrame || e == &_events.eofEvent);

  e->handler();
}

SchedulerEvent *Scheduler::PeekNextEvent()
{
  return _queue.PeekNextEvent();
}

void Scheduler::DisableEvent(SchedulerEvent &ev)
{
  if (ev.IsEnabled())
  {
    _queue.RemoveEvent(&ev);
    ev.Disable();
  }
}

void Scheduler::RunGeneric()
{
  ClearRequestEmulationStop();

  auto cpuMasterEventLoopWrapper = _core.Cpu->GetMasterEventLoopWrapper();
  cpuMasterEventLoopWrapper([this]() { this->MasterEventLoop_InstructionsUntilChipAndThenChipUntilNextInstruction(); });
}

// Steps one instruction forward, returns true if stepping was ended by request to stop emulation
bool Scheduler::DebugStepOneInstruction()
{
  ClearRequestEmulationStop();

  auto cpuMasterEventLoopWrapper = _core.Cpu->GetMasterEventLoopWrapper();
  cpuMasterEventLoopWrapper([this]() { this->MasterEventLoop_DebugStepOneInstruction(); });

  return _stopEmulationRequested;
}

// Steps one instruction forward, returns true if stepping was ended by request to stop emulation
bool Scheduler::DebugStepUntilPc(uint32_t untilPc)
{
  ClearRequestEmulationStop();

  auto cpuMasterEventLoopWrapper = _core.Cpu->GetMasterEventLoopWrapper();
  cpuMasterEventLoopWrapper([this, untilPc]() { this->MasterEventLoop_DebugStepUntilPc(untilPc); });

  return _stopEmulationRequested;
}

void Scheduler::MasterEventLoop_InstructionsUntilChipAndThenChipUntilNextInstruction()
{
  while (!_stopEmulationRequested)
  {
    while (_queue.GetNextEventCycle() >= _events.cpuEvent.cycle)
    {
      _clocks.SetMasterTime(_events.cpuEvent.cycle);
      _events.cpuEvent.handler();
    }
    do
    {
      HandleNextEvent();
    } while (_queue.GetNextEventCycle() < _events.cpuEvent.cycle && !_stopEmulationRequested);
  }
}

// Executes chip events until cpu is scheduled to run, then executes one cpu instruction and returns
void Scheduler::MasterEventLoop_DebugStepOneInstruction()
{
  while (!_stopEmulationRequested)
  {
    if (_queue.GetNextEventCycle() >= _events.cpuEvent.cycle)
    {
      _clocks.SetMasterTime(_events.cpuEvent.cycle);
      _events.cpuEvent.handler();
      return;
    }
    do
    {
      HandleNextEvent();
    } while (_queue.GetNextEventCycle() < _events.cpuEvent.cycle && !_stopEmulationRequested);
  }
}

// Executes chip events and cpu instructions until pc points to the specified pc.
// Useful for stepping over backward branches, jsr, bsr and similar, but can also run forever if flow does not return to the specifiec pc
void Scheduler::MasterEventLoop_DebugStepUntilPc(uint32_t overPc)
{
  while (!_stopEmulationRequested)
  {
    if (_queue.GetNextEventCycle() >= _events.cpuEvent.cycle)
    {
      _clocks.SetMasterTime(_events.cpuEvent.cycle);
      _events.cpuEvent.handler();

      if (_core.Cpu->GetPc() == overPc) return;
    }
    do
    {
      HandleNextEvent();
    } while (_queue.GetNextEventCycle() < _events.cpuEvent.cycle && !_stopEmulationRequested);
  }
}

void Scheduler::Run()
{
  RunGeneric();
}

void Scheduler::ClearQueue()
{
  _queue.Clear();
}

void Scheduler::InitializeQueue()
{
  _queue.Clear();
  _clocks.Clear(_currentFrameParameters);

  _core.Cpu->InitializeCpuEvent();
  _core.Agnus->InitializeEndOfLineEvent();
  _core.Agnus->InitializeEndOfFrameEvent();
  _core.Cia->InitializeCiaEvent();
  _core.CurrentCopper->InitializeCopperEvent();
  _core.Blitter->InitializeBlitterEvent();
  _core.Paula->InitializeInterruptEvent();

  // ddfEvent.Initialize(DDFStateMachine::HandleEvent, "DDF");
  // diwxEvent.Initialize(DIWXStateMachine::HandleEvent, "DIWX");
  // diwyEvent.Initialize(DIWYStateMachine::HandleEvent, "DIWY");
  // chipBusAccessEvent.Initialize(DMAController::HandleEvent, "Chip bus access");
  // shifterEvent.Initialize(BitplaneShifter::HandleEvent, "Shifter");
  // bitplaneDMAEvent.Initialize(BitplaneDMA::HandleEvent, "Bitplane DMA");
  // spriteDMAEvent.Initialize(SpriteDMA::HandleEvent, "Sprite DMA");

  _events.eofEvent.cycle = MasterTimestamp{.Cycle = _currentFrameParameters.CyclesInFrame.Offset};
  _queue.InsertEventWithNullCheck(&_events.eofEvent);

  _events.eolEvent.cycle = MasterTimestamp{.Cycle = _currentFrameParameters.LongLineMasterCycles.Offset - 1};
  _queue.InsertEvent(&_events.eolEvent);
}

void Scheduler::EmulationStart()
{
  _stopEmulationRequested = false;
}

void Scheduler::EmulationStop()
{
}

void Scheduler::SoftReset()
{
  // Soft reset does not change the scheduling state as such.
  // Each module must assess how the soft reset affects their currently scheduled events
}

void Scheduler::HardReset()
{
  InitializeQueue();
}

void Scheduler::Startup()
{
  InitializeQueue();
}

void Scheduler::Shutdown()
{
}

Scheduler::Scheduler(Clocks &clocks, SchedulerEvents &events, FrameParameters &currentFrameParameters)
  : _clocks(clocks), _events(events), _currentFrameParameters(currentFrameParameters)
{
}
