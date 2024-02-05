///*=========================================================================*/
///* Fellow							           */
///*                                                                         */
///* Bus Event Scheduler                                                     */
///*                                                                         */
///* Author: Petter Schau                                                    */
///*                                                                         */
///* Copyright (C) 1991, 1992, 1996 Free Software Foundation, Inc.           */
///*                                                                         */
///* This program is free software; you can redistribute it and/or modify    */
///* it under the terms of the GNU General Public License as published by    */
///* the Free Software Foundation; either version 2, or (at your option)     */
///* any later version.                                                      */
///*                                                                         */
///* This program is distributed in the hope that it will be useful,         */
///* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
///* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
///* GNU General Public License for more details.                            */
///*                                                                         */
///* You should have received a copy of the GNU General Public License       */
///* along with this program; if not, write to the Free Software Foundation, */
///* Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.          */
///*=========================================================================*/
//
// #include "Defs.h"
// #include "FellowMain.h"
// #include "CpuModule.h"
// #include "CpuIntegration.h"
// #include "Cia.h"
// #include "LegacyCopper.h"
// #include "MemoryInterface.h"
// #include "Blitter.h"
// #include "BusScheduler.h"
// #include "GraphicsPipeline.h"
// #include "FloppyDisk.h"
// #include "Keyboard.h"
// #include "Sprites.h"
// #include "Timers.h"
// #include "Renderer.h"
// #include "draw_interlace_control.h"
// #include "interrupt.h"
// #include "IO/Uart.h"
// #include "../automation/Automator.h"
// #include "GfxDrvCommon.h"
//
// #ifdef RETRO_PLATFORM
// #include "RetroPlatform.h"
// #include "KeyboardDriver.h"
// #endif
//
// #include "graphics/Graphics.h"
//
// bus_state bus;
//
// bus_screen_limits pal_long_frame;
// bus_screen_limits pal_short_frame;
//
// bus_event cpuEvent;
// bus_event copperEvent;
// bus_event eolEvent;
// bus_event eofEvent;
// bus_event ciaEvent;
// bus_event blitterEvent;
// bus_event interruptEvent;
//
///*==============================================================================*/
///* Global end of frame handler                                                  */
///*==============================================================================*/
//
// void busRemoveEvent(bus_event *ev)
//{
//  BOOLE found = FALSE;
//  for (bus_event *tmp = bus.events; tmp != nullptr; tmp = tmp->next)
//  {
//    if (tmp == ev)
//    {
//      found = TRUE;
//      break;
//    }
//  }
//  if (!found)
//  {
//    return;
//  }
//
//  if (ev->prev == nullptr)
//  {
//    bus.events = ev->next;
//  }
//  else
//  {
//    ev->prev->next = ev->next;
//  }
//  if (ev->next != nullptr) ev->next->prev = ev->prev;
//  ev->prev = ev->next = nullptr;
//}
//
// void busInsertEventWithNullCheck(bus_event *ev)
//{
//  if (bus.events == nullptr)
//  {
//    ev->prev = ev->next = nullptr;
//    bus.events = ev;
//  }
//  else
//  {
//    busInsertEvent(ev);
//  }
//}
//
// void busInsertEvent(bus_event *ev)
//{
//  bus_event *tmp_prev = nullptr;
//  for (bus_event *tmp = bus.events; tmp != nullptr; tmp = tmp->next)
//  {
//    if (ev->cycle < tmp->cycle)
//    {
//      ev->next = tmp;
//      ev->prev = tmp_prev;
//      tmp->prev = ev;
//      if (tmp_prev == nullptr)
//        bus.events = ev; /* In front */
//      else
//        tmp_prev->next = ev;
//      return;
//    }
//    tmp_prev = tmp;
//  }
//  tmp_prev->next = ev; /* At end */
//  ev->prev = tmp_prev;
//  ev->next = nullptr;
//}
//
// bus_event *busPopEvent()
//{
//  bus_event *tmp = bus.events;
//  bus.events = tmp->next;
//  bus.events->prev = nullptr;
//  return tmp;
//}
//
// void busSetCycle(uint32_t cycle)
//{
//  bus.cycle = cycle;
//}
//
// uint32_t busGetCycle()
//{
//  return bus.cycle;
//}
//
// uint32_t busGetRasterY()
//{
//  return bus.cycle / busGetCyclesInThisLine();
//}
//
// uint32_t busGetRasterX()
//{
//  return bus.cycle % busGetCyclesInThisLine();
//}
//
// uint64_t busGetRasterFrameCount()
//{
//  return bus.frame_no;
//}
//
// uint32_t busGetCyclesInThisLine()
//{
//  return bus.screen_limits->cycles_in_this_line;
//}
//
// uint32_t busGetLinesInThisFrame()
//{
//  return bus.screen_limits->lines_in_this_frame;
//}
//
// uint32_t busGetMaxLinesInFrame()
//{
//  return bus.screen_limits->max_lines_in_frame;
//}
//
// uint32_t busGetCyclesInThisFrame()
//{
//  return bus.screen_limits->cycles_in_this_frame;
//}
//
// void busRun68000Fast()
//{
//  while (!fellow_request_emulation_stop)
//  {
//    if (setjmp(cpu_integration_exception_buffer) == 0)
//    {
//      while (!fellow_request_emulation_stop)
//      {
//        while (bus.events->cycle >= cpuEvent.cycle)
//        {
// #ifdef ENABLE_BUS_EVENT_LOGGING
//          busEventLog(&cpuEvent);
// #endif
//          busSetCycle(cpuEvent.cycle);
//          cpuIntegrationExecuteInstructionEventHandler68000Fast();
//        }
//        do
//        {
//          bus_event *e = busPopEvent();
//
// #ifdef ENABLE_BUS_EVENT_LOGGING
//          busEventLog(e);
// #endif
//          busSetCycle(e->cycle);
//          e->handler();
//        } while (bus.events->cycle < cpuEvent.cycle && !fellow_request_emulation_stop);
//      }
//    }
//    else
//    {
//      // Came out of an CPU exception. Keep on working.
//      // TODO: This looks wrong, shifting down with cpuIntegrationGetSpeed?
//      cpuEvent.cycle = bus.cycle + cpuIntegrationGetChipCycles() + (cpuGetInstructionTime() >> cpuIntegrationGetSpeed());
//      cpuIntegrationSetChipCycles(0);
//    }
//  }
//}
//
// void busRunGeneric()
//{
//  while (!fellow_request_emulation_stop)
//  {
//    if (setjmp(cpu_integration_exception_buffer) == 0)
//    {
//      while (!fellow_request_emulation_stop)
//      {
//        while (bus.events->cycle >= cpuEvent.cycle)
//        {
// #ifdef ENABLE_BUS_EVENT_LOGGING
//          busEventLog(&cpuEvent);
// #endif
//          busSetCycle(cpuEvent.cycle);
//          cpuEvent.handler();
//        }
//        do
//        {
//          bus_event *e = busPopEvent();
//
// #ifdef ENABLE_BUS_EVENT_LOGGING
//          busEventLog(e);
// #endif
//          busSetCycle(e->cycle);
//          e->handler();
//        } while (bus.events->cycle < cpuEvent.cycle && !fellow_request_emulation_stop);
//      }
//    }
//    else
//    {
//      // Came out of an CPU exception. Keep on working.
//      // TODO: This looks wrong, shifting down with cpuIntegrationGetSpeed?
//      cpuEvent.cycle = bus.cycle + cpuIntegrationGetChipCycles() + (cpuGetInstructionTime() >> cpuIntegrationGetSpeed());
//      cpuIntegrationSetChipCycles(0);
//    }
//  }
//}
//
// typedef void (*busRunHandlerFunc)();
// busRunHandlerFunc busGetRunHandler()
//{
//  if (cpuGetModelMajor() <= 1)
//  {
//    if (cpuIntegrationGetSpeed() == 4) return busRun68000Fast;
//  }
//  return busRunGeneric;
//}
//
// void busRun()
//{
//  busGetRunHandler()();
//}
//
///* Steps one instruction forward */
// void busDebugStepOneInstruction()
//{
//   while (!fellow_request_emulation_stop)
//   {
//     if (setjmp(cpu_integration_exception_buffer) == 0)
//     {
//       while (!fellow_request_emulation_stop)
//       {
//         if (bus.events->cycle >= cpuEvent.cycle)
//         {
// #ifdef ENABLE_BUS_EVENT_LOGGING
//           busEventLog(&cpuEvent);
// #endif
//           busSetCycle(cpuEvent.cycle);
//           cpuEvent.handler();
//           return;
//         }
//         do
//         {
//           bus_event *e = busPopEvent();
//
// #ifdef ENABLE_BUS_EVENT_LOGGING
//           busEventLog(e);
// #endif
//           busSetCycle(e->cycle);
//           e->handler();
//         } while (bus.events->cycle < cpuEvent.cycle && !fellow_request_emulation_stop);
//       }
//     }
//     else
//     {
//       // Came out of an CPU exception. Return to debugger after setting the cycle count
//       cpuEvent.cycle = bus.cycle + cpuIntegrationGetChipCycles() + (cpuGetInstructionTime() >> cpuIntegrationGetSpeed());
//       cpuIntegrationSetChipCycles(0);
//       return;
//     }
//   }
// }
//
// void busClearEvent(bus_event *ev, busEventHandler handlerFunc)
//{
//   memset(ev, 0, sizeof(bus_event));
//   ev->cycle = BUS_CYCLE_DISABLE;
//   ev->handler = handlerFunc;
// }
//
// void busDetermineCpuInstructionEventHandler()
//{
//   if (cpuGetModelMajor() <= 1)
//   {
//     if (cpuIntegrationGetSpeed() == 4)
//       cpuEvent.handler = cpuIntegrationExecuteInstructionEventHandler68000Fast;
//     else
//       cpuEvent.handler = cpuIntegrationExecuteInstructionEventHandler68000General;
//   }
//   else
//     cpuEvent.handler = cpuIntegrationExecuteInstructionEventHandler68020;
// }
//
// void busClearCpuEvent()
//{
//   memset(&cpuEvent, 0, sizeof(bus_event));
//   cpuEvent.cycle = 0;
//   busDetermineCpuInstructionEventHandler();
// }
//
// void busInitializeQueue()
//{
//   bus.events = nullptr;
//   busClearCpuEvent();
//   busClearEvent(&eolEvent, busEndOfLine);
//   busClearEvent(&eofEvent, busEndOfFrame);
//   busClearEvent(&ciaEvent, ciaHandleEvent);
//   busClearEvent(&copperEvent, copperEventHandler);
//   busClearEvent(&blitterEvent, blitFinishBlit);
//   busClearEvent(&interruptEvent, interruptHandleEvent);
//
//   eofEvent.cycle = busGetCyclesInThisFrame();
//   busInsertEventWithNullCheck(&eofEvent);
//   eolEvent.cycle = busGetCyclesInThisLine() - 1;
//   busInsertEvent(&eolEvent);
// }
//
// void busInitializePalLongFrame()
//{
//   pal_long_frame.cycles_in_this_line = 227;
//   pal_long_frame.max_cycles_in_line = 227;
//   pal_long_frame.lines_in_this_frame = 313;
//   pal_long_frame.max_lines_in_frame = 314;
//   pal_long_frame.cycles_in_this_frame = 313 * 227;
// }
// void busInitializePalShortFrame()
//{
//   pal_short_frame.cycles_in_this_line = 227;
//   pal_short_frame.max_cycles_in_line = 227;
//   pal_short_frame.lines_in_this_frame = 312;
//   pal_short_frame.max_lines_in_frame = 314;
//   pal_short_frame.cycles_in_this_frame = 312 * 227;
// }
//
// void busInitializeScreenLimits()
//{
//   busInitializePalLongFrame();
//   busInitializePalShortFrame();
// }
//
// void busSetScreenLimits(bool is_long_frame)
//{
//   if (is_long_frame)
//   {
//     bus.screen_limits = &pal_long_frame;
//   }
//   else
//   {
//     bus.screen_limits = &pal_short_frame;
//   }
// }
//
///*===========================================================================*/
///* Called on emulation start / stop and reset                                */
///*===========================================================================*/
//
// void busEmulationStart()
//{
//}
//
// void busEmulationStop()
//{
//}
//
// void busSoftReset()
//{
//  busInitializeQueue();
//}
//
// void busHardReset()
//{
//  busInitializeQueue();
//
//  // Continue to use the selected cycle layout, interlace control will switch it when necessary
//  // it must only be changed in the end of frame handler to maintain event time consistency
//  busSetScreenLimits(drawGetFrameIsLong());
//}
//
///*===========================================================================*/
///* Called on emulator startup / shutdown                                     */
///*===========================================================================*/
//
// void busStartup()
//{
//  bus.frame_no = 0;
//  bus.cycle = 0;
//  bus.screen_limits = &pal_long_frame;
//
//  busInitializeScreenLimits();
//}
//
// void busShutdown()
//{
// #ifdef ENABLE_BUS_EVENT_LOGGING
//  busCloseLog();
// #endif
//}
