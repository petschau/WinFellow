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

#include "fellow/api/defs.h"
#include "fellow/application/Fellow.h"
#include "fellow/cpu/CpuModule.h"
#include "fellow/cpu/CpuIntegration.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/chipset/Cia.h"
#include "fellow/chipset/CopperCommon.h"
#include "fellow/chipset/Blitter.h"
#include "fellow/scheduler/Scheduler.h"
#include "fellow/chipset/Graphics.h"
#include "fellow/chipset/Floppy.h"
#include "KBD.H"
#include "fellow/chipset/Sound.h"
#include "fellow/chipset/Sprite.h"
#include "fellow/application/HostRenderer.h"
#include "fellow/chipset/draw_interlace_control.h"
#include "interrupt.h"
#include "fellow/chipset/uart.h"
#include "../automation/Automator.h"

#ifdef RETRO_PLATFORM
#include "RetroPlatform.h"
#include "fellow/application/KbdDrv.h"
#endif

#include "fellow/chipset/DDFStateMachine.h"
#include "fellow/chipset/DIWXStateMachine.h"
#include "fellow/chipset/DIWYStateMachine.h"
#include "fellow/chipset/DMAController.h"
#include "fellow/chipset/BitplaneDMA.h"
#include "fellow/chipset/SpriteDMA.h"
#include "fellow/chipset/BitplaneShifter.h"
#include "fellow/debug/log/DebugLogHandler.h"

Scheduler scheduler;

void EndOfLineWrapper()
{
  scheduler.EndOfLine();
}

void EndOfFrameWrapper()
{
  scheduler.EndOfFrame();
}

FrameParameters pal_long_frame;
FrameParameters pal_short_frame;

SchedulerEvent cpuEvent;
SchedulerEvent copperEvent;
SchedulerEvent eolEvent;
SchedulerEvent eofEvent;
SchedulerEvent ciaEvent;
SchedulerEvent blitterEvent;
SchedulerEvent interruptEvent;
SchedulerEvent chipBusAccessEvent;
SchedulerEvent ddfEvent;
SchedulerEvent diwxEvent;
SchedulerEvent diwyEvent;
SchedulerEvent shifterEvent;
SchedulerEvent spriteDMAEvent;
SchedulerEvent bitplaneDMAEvent;

void Scheduler::InsertEvent(SchedulerEvent *ev)
{
  F_ASSERT(ev->cycle >= FrameCycleInsertGuard);
  F_ASSERT(((int)ev->cycle) >= 0);

  _queue.InsertEvent(ev);
}

void Scheduler::RemoveEvent(SchedulerEvent *ev)
{
  _queue.RemoveEvent(ev);
}

void Scheduler::RebaseForNewFrame(SchedulerEvent *ev)
{
  if (ev->IsEnabled())
  {
    _queue.RemoveEvent(ev);

    F_ASSERT(((int)(ev->cycle - GetCyclesInFrame())) >= 0);

    ev->cycle -= GetCyclesInFrame();

    F_ASSERT(((int)ev->cycle) >= 0);

    _queue.InsertEvent(ev);
  }
}

void Scheduler::RebaseForNewFrameWithNullCheck(SchedulerEvent *ev)
{
  if (ev->IsEnabled())
  {
    _queue.RemoveEvent(ev);

    F_ASSERT(((int)(ev->cycle - GetCyclesInFrame())) >= 0);

    ev->cycle -= GetCyclesInFrame();

    _queue.InsertEventWithNullCheck(ev);
  }
}

void Scheduler::LogPop(SchedulerEvent *ev)
{
  if (DebugLog.Enabled)
  {
    DebugLog.AddEventQueueLogEntry(DebugLogSource::EVENT_QUEUE, GetRasterFrameCount(), GetSHResTimestamp(), EventQueueLogReasons::Pop, ev->Name);
  }
}

/*==============================================================================*/
/* Global end of line handler                                                   */
/*==============================================================================*/

void Scheduler::EndOfLine()
{
  /*==============================================================*/
  /* Handles graphics planar to chunky conversion                 */
  /* and updates the graphics emulation for a new line            */
  /*==============================================================*/
  graphEndOfLine();
  if (chipsetIsCycleExact())
  {
    sprite_dma.EndOfLine();
  }
  else
  {
    spriteEndOfLine(GetRasterY());
  }

  /*==============================================================*/
  /* Update the CIA B event counter                               */
  /*==============================================================*/
  ciaUpdateEventCounter(1);

  /*==============================================================*/
  /* Handles disk DMA if it is running                            */
  /*==============================================================*/
  floppyEndOfLine();

  /*==============================================================*/
  /* Update the sound emulation                                   */
  /*==============================================================*/
  soundEndOfLine();

  /*==============================================================*/
  /* Handle keyboard events                                       */
  /*==============================================================*/
  kbdQueueHandler();
  kbdEventEOLHandler();

  uart.EndOfLine();
  automator.EndOfLine();

  /*==============================================================*/
  /* Set up the next end of line event                            */
  /*==============================================================*/

  eolEvent.cycle += GetCyclesInLine();
  InsertEvent(&eolEvent);
}

/*==============================================================================*/
/* Global end of frame handler                                                  */
/*==============================================================================*/

static bool busIsInEndOfFrame = false;

void Scheduler::EndOfFrame()
{
  busIsInEndOfFrame = true;
  DebugLog.Clear();
  FrameCycleInsertGuard = 0;

  //==============================================================
  // Draw the frame in the host buffer
  //==============================================================
  if (!chipsetIsCycleExact())
  {
    Draw.EndOfFrame();
  }

  //===============================================================
  // Handle keyboard events like insert/eject disk, BMP dump, exit
  //===============================================================
#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode()) kbdDrvEOFHandler();
#endif
  kbdEventEOFHandler();

  //===============================================================
  // End of frame handlers for cycle exact
  //===============================================================
  if (chipsetIsCycleExact())
  {
    sprite_dma.EndOfFrame(); // Removes event
    BitplaneDMA::EndOfFrame();
    dma_controller.EndOfFrame();     // Rebases event
    diwy_state_machine.EndOfFrame(); // Schedules event
    diwx_state_machine.EndOfFrame(); // Schedules event
    ddf_state_machine.EndOfFrame();  // Schedules event at vertical blank end line
    bitplane_shifter.EndOfFrame();   // Schedules event at horisontal sync
  }

  //==============================================================
  // Restart copper, event not carried over to next frame
  //==============================================================
  copperEndOfFrame(); // Requests DMA that could schedule event

  //==============================================================
  // Update CIA timer counters, rebase cycle offset and event
  //==============================================================
  ciaUpdateTimersEOF();

  //==============================================================
  // Sprite end of frame updates, no event involved
  //==============================================================
  spriteEndOfFrame();

  //==============================================================
  // Blitter, rebase event for new frame
  //==============================================================
  blitterEndOfFrame();

  //==============================================================
  // UART, rebase receive and transmit times for new frame
  //==============================================================
  uart.EndOfFrame();

  //==============================================================
  // Flag vertical refresh IRQ, can set IRQ event
  //==============================================================
  wintreq_direct(0x8020, 0xdff09c, true);

  //==============================================================
  // IRQ, rebase event for new frame
  //==============================================================
  interruptEndOfFrame();

  //==============================================================
  // Rebase next CPU instruction time for new frame
  //==============================================================
  if (cpuEvent.IsEnabled())
  {
    // Rebase CPU event, the CPU is never in the queue
    cpuEvent.cycle -= GetCyclesInFrame();
  }

  //==============================================================
  // Perform graphics end of frame, mostly for line exact
  // TODO: Handle graph_buffer_lost in cycle exact mode
  //==============================================================
  graphEndOfFrame();

  //==============================================================
  // Automatic testing, save screen buffer snapshot to file
  // TODO: For cycle exact this is currently one frame behind, as rendering happens some cycles later
  // TODO: Finish up the line here, should be ok as the last line never contains content anyway
  //==============================================================
  automator.EndOfFrame();

  //==============================================================
  // Decide interlace rendering status for next frame and switch
  // screen geometry based on long/short frame
  //
  // Warning this can change the frame layout
  // Number of cycles in a frame will be different
  // Must be called after all events have been rebased
  //==============================================================
  drawInterlaceEndOfFrame();

  //==============================================================
  // Set end of line event
  // At this time it is possibly the only event on the queue
  //==============================================================
  _queue.RemoveEvent(&eolEvent);
  eolEvent.cycle = GetCyclesInLine() - 1;
  _queue.InsertEventWithNullCheck(&eolEvent);

  //==============================================================
  // Set end of frame event
  //==============================================================
  eofEvent.cycle = GetCyclesInFrame();
  _queue.InsertEvent(&eofEvent);

  //==============================================================
  // Stats
  //==============================================================
  FrameNo++;
  FrameCycle = 0;
  busIsInEndOfFrame = false;
}

ULO Scheduler::GetCycleFromCycle280ns(ULO cycle280ns)
{
  return cycle280ns << BUS_CYCLE_FROM_280NS_SHIFT;
}

ULO Scheduler::GetCycle280nsFromCycle(ULO cycle)
{
  return cycle >> BUS_CYCLE_FROM_280NS_SHIFT;
}

ULO Scheduler::GetCycleFromCycle140ns(ULO cycle140ns)
{
  return cycle140ns << BUS_CYCLE_FROM_140NS_SHIFT;
}

ULO Scheduler::GetLineCycle280ns()
{
  return CurrentCycleChipBusTimestamp.GetCycle();
}

ULO Scheduler::GetBaseClockTimestamp(ULO line, ULO cycle)
{
  return GetCyclesInLine() * line + cycle;
}

const ChipBusTimestamp &Scheduler::GetChipBusTimestamp()
{
  return CurrentCycleChipBusTimestamp;
}

const SHResTimestamp &Scheduler::GetSHResTimestamp()
{
  return CurrentCycleSHResTimestamp;
}

void Scheduler::SetFrameCycle(ULO frameCycle)
{
  F_ASSERT(frameCycle >= FrameCycle);

  ULO cyclesInLine = FrameParameters->ShortLineCycles;
  FrameCycle = frameCycle;
  FrameCycleInsertGuard = frameCycle;
  CurrentCycleSHResTimestamp.Set(frameCycle / cyclesInLine, frameCycle % cyclesInLine);
  CurrentCycleChipBusTimestamp.Set(CurrentCycleSHResTimestamp.Line, CurrentCycleSHResTimestamp.ChipCycle());
}

ULO Scheduler::GetFrameCycle(bool doassert)
{
  if (doassert)
  {
    F_ASSERT(!busIsInEndOfFrame);
  }
  return FrameCycle;
}

unsigned int Scheduler::GetRasterY() const
{
  return CurrentCycleSHResTimestamp.Line;
}

unsigned int Scheduler::GetRasterX() const
{
  return CurrentCycleSHResTimestamp.Pixel;
}

ULO Scheduler::GetLastCycleInCurrentCylinder()
{
  return (CurrentCycleSHResTimestamp.Pixel & BUS_CYCLE_140NS_MASK) + BUS_CYCLE_FROM_140NS_MULTIPLIER - 1;
}

ULL Scheduler::GetRasterFrameCount()
{
  return FrameNo;
}

ULO Scheduler::GetHorisontalBlankStart()
{
  return FrameParameters->HorisontalBlankStart;
}

ULO Scheduler::GetHorisontalBlankEnd()
{
  return FrameParameters->HorisontalBlankEnd;
}

ULO Scheduler::GetVerticalBlankEnd()
{
  return FrameParameters->VerticalBlankEnd;
}

ULO Scheduler::GetCyclesInLine()
{
  return FrameParameters->ShortLineCycles;
}

ULO Scheduler::GetCylindersInLine()
{
  return FrameParameters->ShortLineCycles / BUS_CYCLE_FROM_140NS_MULTIPLIER;
}

ULO Scheduler::Get280nsCyclesInLine()
{
  return GetCycle280nsFromCycle(FrameParameters->ShortLineCycles);
}

ULO Scheduler::GetLinesInFrame()
{
  return FrameParameters->LinesInFrame;
}

ULO Scheduler::GetCyclesInFrame()
{
  return FrameParameters->CyclesInFrame;
}

void Scheduler::HandleNextEvent()
{
  SchedulerEvent *e = _queue.PopEvent();
  SetFrameCycle(e->cycle);

  F_ASSERT(GetRasterY() < GetLinesInFrame() || e == &eofEvent);
  LogPop(e);

  e->handler();
}

SchedulerEvent *Scheduler::PeekNextEvent()
{
  return _queue.PeekNextEvent();
}

void Scheduler::Run68000Fast()
{
  while (!fellow_request_emulation_stop)
  {
    if (setjmp(cpu_integration_exception_buffer) == 0)
    {
      while (!fellow_request_emulation_stop)
      {
        while (_queue.GetNextEventCycle() >= cpuEvent.cycle)
        {
          SetFrameCycle(cpuEvent.cycle);
          cpuIntegrationExecuteInstructionEventHandler68000Fast();
        }
        do
        {
          HandleNextEvent();
        } while (_queue.GetNextEventCycle() < cpuEvent.cycle && !fellow_request_emulation_stop);
      }
    }
    else
    {
      // Came out of an CPU exception. Keep on working.
      // TODO: This looks wrong, shifting down with cpuIntegrationGetSpeed?
      cpuEvent.cycle = FrameCycle + cpuIntegrationGetChipCycles() + (cpuGetInstructionTime() >> cpuIntegrationGetSpeed());
      cpuIntegrationSetChipCycles(0);
    }
  }
}

void Scheduler::Run68000FastExperimental()
{
  while (!fellow_request_emulation_stop)
  {
    if (setjmp(cpu_integration_exception_buffer) == 0)
    {
      while (!fellow_request_emulation_stop)
      {
        while (chipBusAccessEvent.IsEnabled() && _queue.GetNextEventCycle() >= chipBusAccessEvent.cycle && cpuEvent.cycle >= chipBusAccessEvent.cycle)
        {
          SetFrameCycle(chipBusAccessEvent.cycle);
          dma_controller.Handler();
        }

        if (_queue.GetNextEventCycle() >= cpuEvent.cycle)
        {
          SetFrameCycle(cpuEvent.cycle);
          cpuIntegrationExecuteInstructionEventHandler68000Fast();
        }
        else
        {
          HandleNextEvent();
        }
      }
    }
    else
    {
      // Came out of an CPU exception. Keep on working.
      // TODO: This looks wrong, shifting down with cpuIntegrationGetSpeed?
      cpuEvent.cycle = FrameCycle + cpuIntegrationGetChipCycles() + (cpuGetInstructionTime() >> cpuIntegrationGetSpeed());
      cpuIntegrationSetChipCycles(0);
    }
  }
}

void Scheduler::RunGeneric()
{
  while (!fellow_request_emulation_stop)
  {
    if (setjmp(cpu_integration_exception_buffer) == 0)
    {
      while (!fellow_request_emulation_stop)
      {
        while (_queue.GetNextEventCycle() >= cpuEvent.cycle)
        {
          SetFrameCycle(cpuEvent.cycle);
          cpuEvent.handler();
        }
        do
        {
          HandleNextEvent();
        } while (_queue.GetNextEventCycle() < cpuEvent.cycle && !fellow_request_emulation_stop);
      }
    }
    else
    {
      // Came out of an CPU exception. Keep on working.
      // TODO: This looks wrong, shifting down with cpuIntegrationGetSpeed?
      cpuEvent.cycle = FrameCycle + cpuIntegrationGetChipCycles() + (cpuGetInstructionTime() >> cpuIntegrationGetSpeed());
      cpuIntegrationSetChipCycles(0);
    }
  }
}

Scheduler::ChipBusRunHandlerFunc Scheduler::GetRunHandler() const
{
  if (cpuGetModelMajor() <= 1)
  {
    if (cpuIntegrationGetSpeed() == 4)
    {
      if (chipset_info.IsCycleExact)
      {
        if (!dma_controller.UseEventQueue)
        {
          return &Scheduler::Run68000FastExperimental;
        }
        else
        {
          return &Scheduler::Run68000Fast;
        }
      }

      return &Scheduler::Run68000Fast;
    }
  }
  return &Scheduler::RunGeneric;
}

void Scheduler::Run()
{
  (this->*GetRunHandler())();
}

/* Steps one instruction forward */
void Scheduler::DebugStepOneInstruction()
{
  while (!fellow_request_emulation_stop)
  {
    if (setjmp(cpu_integration_exception_buffer) == 0)
    {
      while (!fellow_request_emulation_stop)
      {
        if (_queue.GetNextEventCycle() >= cpuEvent.cycle)
        {
          SetFrameCycle(cpuEvent.cycle);
          cpuEvent.handler();
          return;
        }
        do
        {
          HandleNextEvent();
        } while (_queue.GetNextEventCycle() < cpuEvent.cycle && !fellow_request_emulation_stop);
      }
    }
    else
    {
      // Came out of an CPU exception. Return to debugger after setting the cycle count
      cpuEvent.cycle = FrameCycle + cpuIntegrationGetChipCycles() + (cpuGetInstructionTime() >> cpuIntegrationGetSpeed());
      cpuIntegrationSetChipCycles(0);
      return;
    }
  }
}

SchedulerEventHandler Scheduler::GetCpuInstructionEventHandler()
{
  if (cpuGetModelMajor() <= 1)
  {
    if (cpuIntegrationGetSpeed() == 4)
    {
      return cpuIntegrationExecuteInstructionEventHandler68000Fast;
    }
    else
    {
      return cpuIntegrationExecuteInstructionEventHandler68000General;
    }
  }
  else
  {
    return cpuIntegrationExecuteInstructionEventHandler68020;
  }
}

void Scheduler::DetermineCpuInstructionEventHandler()
{
  cpuEvent.handler = GetCpuInstructionEventHandler();
}

void Scheduler::InitializeCpuEvent()
{
  cpuEvent.Initialize(GetCpuInstructionEventHandler(), "CPU");
  cpuEvent.cycle = 0;
}

void Scheduler::ClearQueue()
{
  _queue.Clear();
}

void Scheduler::InitializeQueue()
{
  _queue.Clear();

  FrameCycle = 0;
  FrameCycleInsertGuard = 0;

  InitializeCpuEvent();
  eolEvent.Initialize(EndOfLineWrapper, "End of line");
  eofEvent.Initialize(EndOfFrameWrapper, "End of frame");
  ciaEvent.Initialize(ciaHandleEvent, "Cia");
  copperEvent.Initialize(copperEventHandler, "Copper");
  blitterEvent.Initialize(blitFinishBlit, "Blitter");
  interruptEvent.Initialize(interruptHandleEvent, "Interrupt");
  ddfEvent.Initialize(DDFStateMachine::HandleEvent, "DDF");
  diwxEvent.Initialize(DIWXStateMachine::HandleEvent, "DIWX");
  diwyEvent.Initialize(DIWYStateMachine::HandleEvent, "DIWY");
  chipBusAccessEvent.Initialize(DMAController::HandleEvent, "Chip bus access");
  shifterEvent.Initialize(BitplaneShifter::HandleEvent, "Shifter");
  bitplaneDMAEvent.Initialize(BitplaneDMA::HandleEvent, "Bitplane DMA");
  spriteDMAEvent.Initialize(SpriteDMA::HandleEvent, "Sprite DMA");

  eofEvent.cycle = GetCyclesInFrame();
  _queue.InsertEventWithNullCheck(&eofEvent);

  eolEvent.cycle = GetCyclesInLine() - 1;
  _queue.InsertEvent(&eolEvent);
}

void Scheduler::InitializePalLongFrame()
{
  pal_long_frame.HorisontalBlankStart = GetCycleFromCycle280ns(9); // First blanked pixel
  pal_long_frame.HorisontalBlankEnd = GetCycleFromCycle280ns(44);  // First visible pixel of line
  pal_long_frame.VerticalBlankEnd = 26;                            // First line with active display
  pal_long_frame.ShortLineCycles = GetCycleFromCycle280ns(227);
  pal_long_frame.LongLineCycles = GetCycleFromCycle280ns(227);
  pal_long_frame.LinesInFrame = 313;
  pal_long_frame.CyclesInFrame = 313 * GetCycleFromCycle280ns(227);
}

void Scheduler::InitializePalShortFrame()
{
  pal_short_frame.HorisontalBlankStart = GetCycleFromCycle280ns(9); // First blanked pixel
  pal_short_frame.HorisontalBlankEnd = GetCycleFromCycle280ns(44);  // First visible pixel of line
  pal_short_frame.VerticalBlankEnd = 26;                            // First line with active display
  pal_short_frame.ShortLineCycles = GetCycleFromCycle280ns(227);
  pal_short_frame.LongLineCycles = GetCycleFromCycle280ns(227);
  pal_short_frame.LinesInFrame = 312;
  pal_short_frame.CyclesInFrame = 312 * GetCycleFromCycle280ns(227);
}

void Scheduler::InitializeScreenLimits()
{
  InitializePalLongFrame();
  InitializePalShortFrame();
}

void Scheduler::SetScreenLimits(bool is_long_frame)
{
  if (is_long_frame)
  {
    FrameParameters = &pal_long_frame;
  }
  else
  {
    FrameParameters = &pal_short_frame;
  }
}

/*===========================================================================*/
/* Called on emulation start / stop and reset                                */
/*===========================================================================*/

void Scheduler::EmulationStart()
{
}

void Scheduler::EmulationStop()
{
}

void Scheduler::SoftReset()
{
  InitializeQueue();
}

void Scheduler::HardReset()
{
  InitializeQueue();

  // Continue to use the selected cycle layout, interlace control will switch it when necessary
  // it must only be changed in the end of frame handler to maintain event time consistency
  SetScreenLimits(drawGetFrameIsLong());
}

/*===========================================================================*/
/* Called on emulator startup / shutdown                                     */
/*===========================================================================*/

void Scheduler::Startup()
{
  FrameNo = 0;
  FrameCycle = 0;
  FrameParameters = &pal_long_frame;

  InitializeScreenLimits();
  InitializeQueue();
  SetFrameCycle(0);
}

void Scheduler::Shutdown()
{
}
