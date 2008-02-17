/* @(#) $Id: BUS.C,v 1.3 2008-02-17 12:56:47 peschau Exp $ */
/*=========================================================================*/
/* Fellow Amiga Emulator                                                      */
/*                                                                         */
/* Bus Event Scheduler Initialization                                         */
/*                                                                            */
/* Author: Petter Schau (peschau@online.no)                                   */
/*                                                                            */
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
#include "cpu.h"
#include "cia.h"
#include "copper.h"
#include "fmem.h"
#include "blit.h"
#include "bus.h"
#include "graph.h"
#include "floppy.h"
#include "kbd.h"
#include "sound.h"
#include "kbd.h"
#include "sprite.h"
#include "timer.h"
#include "draw.h"

ULO debugging;
ULO bus_cycle;

UWO bus_cycle_to_ypos[0x20000];
UBY bus_cycle_to_xpos[0x20000];

extern ULO copper_ptr, copper_next, draw_frame_count, draw_frame_skip_factor;
extern BOOLE fellow_request_emulation_stop_immediately, fellow_request_emulation_stop;
extern LON draw_frame_skip;

/* Draft of a new and cleaner bus-event handling */

bus_event cpuEvent;
bus_event copperEvent;
bus_event eolEvent;
bus_event eofEvent;
bus_event ciaEvent;
bus_event blitterEvent;
bus_event irqEvent;
bus_event *bus_events;

/*==============================================================================*/
/* Global end of line handler                                                   */
/*==============================================================================*/

void busEndOfLine(void)
{

  /*==============================================================*/
  /* Handles graphics planar to chunky conversion                 */
  /* and updates the graphics emulation for a new line            */
  /*==============================================================*/
  graphEndOfLine(); 
  spriteEndOfLine();

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

  /*==============================================================*/
  /* Set up the next end of line event                            */
  /*==============================================================*/
  eolEvent.cycle += BUS_CYCLE_PER_LINE;
  busInsertEvent(&eolEvent);
}

/*==============================================================================*/
/* Global end of frame handler                                                  */
/*==============================================================================*/

void busEndOfFrame(void)
{
  /*==============================================================*/
  /* Draw the frame in the host buffer                            */
  /*==============================================================*/
  drawEndOfFramePre();

  /*==============================================================*/
  /* Handle keyboard events                                       */
  /*==============================================================*/
  kbdEventEOFHandler();

  /*==============================================================*/
  /* Reset some aspects of the graphics emulation                 */
  /*==============================================================*/
  lof = lof ^ 0x8000;	// short/long frame
  
  /*==============================================================*/
  /* Restart copper                                               */
  /*==============================================================*/
  copperEndOfFrame();

  /*==============================================================*/
  /* Update frame counters                                        */
  /*==============================================================*/
  draw_frame_count++; // count frames
  draw_frame_skip--;  // frame skipping

  if (draw_frame_skip < 0) 
  {
    draw_frame_skip = draw_frame_skip_factor;
  }

  /*==============================================================*/
  /* Update CIA timer counters                                    */
  /*==============================================================*/
  ciaUpdateTimersEOF();

  /*==============================================================*/
  /* Sprite end of frame updates                                  */
  /*==============================================================*/
  spriteEndOfFrame();

  /*==============================================================*/
  /* Recalculate blitter finished time                            */
  /*==============================================================*/
  blitterEndOfFrame();

  /*==============================================================*/
  /* Flag vertical refresh IRQ                                    */
  /*==============================================================*/

  memoryWriteWord(0x8020, 0xdff09c);

  /*==============================================================*/
  /* Set up next end of line event                                */
  /*==============================================================*/

  busRemoveEvent(&eolEvent);
  eolEvent.cycle = BUS_CYCLE_PER_LINE - 1;
  busInsertEvent(&eolEvent);

  /*==============================================================*/
  /* Update next CPU instruction time                             */
  /*==============================================================*/

  if (cpuEvent.cycle != BUS_CYCLE_DISABLE)
  {
    //busRemoveEvent(&cpuEvent);
    cpuEvent.cycle -= BUS_CYCLE_PER_FRAME;
    //busInsertEvent(&cpuEvent);
  }

  /*==============================================================*/
  /* Update next IRQ time                                         */
  /*==============================================================*/
  if (irqEvent.cycle != BUS_CYCLE_DISABLE)
  {
    busRemoveEvent(&irqEvent);
    irqEvent.cycle -= BUS_CYCLE_PER_FRAME;
    busInsertEvent(&irqEvent);
  }

  /*==============================================================*/
  /* Perform graphics end of frame                                */
  /*==============================================================*/
  graphEndOfFrame();
  timerEndOfFrame();

  busInsertEvent(&eofEvent);	  
}

void busMsg(STR *msg)
{
  MessageBox(0, msg, msg, 0);
}

void busRemoveEvent(bus_event *ev)
{
  bus_event *tmp;
  BOOLE found = FALSE;
  for (tmp = bus_events; tmp != NULL; tmp = tmp->next)
  {
    if (tmp == ev)
    {
      found = TRUE;
      break;
    }
  }
  if (!found)
  {
    busMsg("Removing event that is not in the queue!");
    return;
  }

  if (ev->prev == NULL)
  {
    bus_events = ev->next;
  }
  else
  {
    ev->prev->next = ev->next;
  }
  if (ev->next != NULL) ev->next->prev = ev->prev;
  ev->prev = ev->next = NULL;
}

void busInsertEvent(bus_event *ev)
{
  if (bus_events == NULL)
  {
    ev->prev = ev->next = NULL;
    bus_events = ev;
  }
  else
  {
    bus_event *tmp;
    bus_event *tmp_prev = NULL;
    for (tmp = bus_events; tmp != NULL; tmp = tmp->next)
    {
      if (ev->cycle < tmp->cycle)
      {
	ev->next = tmp;
	ev->prev = tmp_prev;
	tmp->prev = ev;
	if (tmp_prev == NULL) bus_events = ev; /* In front */
	else tmp_prev->next = ev;
	return;
      }
      tmp_prev = tmp;
    }
    tmp_prev->next = ev; /* At end */
    ev->prev = tmp_prev;
    ev->next = NULL;
  }
}

bus_event *busPopEvent(void)
{
  if (bus_events->cycle >= cpuEvent.cycle)
  {
    return &cpuEvent;
  }
  else
  {
    bus_event *tmp = bus_events;
    bus_events = tmp->next;
    bus_events->prev = NULL;
    return tmp;
  }
}

FILE *BUSLOG = NULL;
BOOLE bus_log = FALSE;

void busEventLog(bus_event *e)
{
  if (!bus_log) return;
  if (BUSLOG == NULL) BUSLOG = fopen("buslog.txt", "w");
  if (e == &copperEvent)
    fprintf(BUSLOG, "%d copper\n", e->cycle);
  else if (e == &ciaEvent)
    fprintf(BUSLOG, "%d cia\n", e->cycle);
  else if (e == &irqEvent)
    fprintf(BUSLOG, "%d irq\n", e->cycle);
  else if (e == &blitterEvent)
    fprintf(BUSLOG, "%d blitter\n", e->cycle);
  else if (e == &eolEvent)
    fprintf(BUSLOG, "%d eol\n", e->cycle);
  else if (e == &eofEvent)
    fprintf(BUSLOG, "%d eof\n", e->cycle);
}

void busLogCpu(STR *s)
{
  if (!bus_log) return;
  if (BUSLOG == NULL) BUSLOG = fopen("c:\\buslog.txt", "a");
  fprintf(BUSLOG, "%d %s\n", cpuEvent.cycle, s);
}

void busLogCpuException(STR *s)
{
  /*
  if (!bus_log) return;
  if (BUSLOG == NULL) BUSLOG = fopen("c:\\buslog.txt", "a");
  fprintf(BUSLOG, "%d: %s for opcode %.4X at PC %.8X from PC %.8X\n", cpuEvent.cycle, s, cpu_current_opcode, cpu_original_pc, pc);
  */
}

void busUpdateCycleClock(ULO cycle)
{
  bus_cycle = cycle;
  graph_raster_y = bus_cycle_to_ypos[cycle>>2] & 0x1ff;
  graph_raster_x = bus_cycle_to_xpos[cycle] & 0xff;
}

extern jmp_buf cpu_exception_buffer;

void busRunNew(void)
{
  while (!fellow_request_emulation_stop && !fellow_request_emulation_stop_immediately)
  {
    if (setjmp(cpu_exception_buffer) == 0)
    {
      while (!fellow_request_emulation_stop && !fellow_request_emulation_stop_immediately)
      {
	bus_event *e = busPopEvent();
	//busEventLog(e);
	busUpdateCycleClock(e->cycle);
	e->handler();
      }
    }
    else
    {
      // Came out of an CPU exception. Keep on working.
      cpuEvent.cycle = bus_cycle + cpu_chip_cycles + (cpu_instruction_time >> cpu_speed);
      //busInsertEvent(&cpuEvent);
      cpu_chip_cycles = 0;
    }
  }
}

void busDebugNew(void)
{
  while (!fellow_request_emulation_stop && !fellow_request_emulation_stop_immediately)
  {
    if (setjmp(cpu_exception_buffer) == 0)
    {
      while (!fellow_request_emulation_stop && !fellow_request_emulation_stop_immediately)
      {
	bus_event *e = busPopEvent();
	busEventLog(e);
	busUpdateCycleClock(e->cycle);
	e->handler();
	if (e == &cpuEvent) return;
      }
    }
    else
    {
      // Came out of an CPU exception. Keep on working.
      cpuEvent.cycle = bus_cycle + cpu_chip_cycles + (cpu_instruction_time >> cpu_speed);
      //busInsertEvent(&cpuEvent);
      cpu_chip_cycles = 0;
    }
  }
}

void busInitializeQueue(BOOLE insert_cpu_event)
{
  memset(&cpuEvent, 0, sizeof(bus_event));
  memset(&copperEvent, 0, sizeof(bus_event));
  memset(&eolEvent, 0, sizeof(bus_event));
  memset(&eofEvent, 0, sizeof(bus_event));
  memset(&ciaEvent, 0, sizeof(bus_event));
  memset(&blitterEvent, 0, sizeof(bus_event));
  memset(&irqEvent, 0, sizeof(bus_event));
  bus_events = NULL;

  eolEvent.cycle = BUS_CYCLE_PER_LINE - 1;
  eolEvent.handler = busEndOfLine;
  eofEvent.cycle = BUS_CYCLE_PER_FRAME;
  eofEvent.handler = busEndOfFrame;
  ciaEvent.cycle = BUS_CYCLE_DISABLE;
  ciaEvent.handler = ciaHandleEvent;
  irqEvent.cycle = BUS_CYCLE_DISABLE;
  irqEvent.handler = cpuSetUpInterrupt;
  copperEvent.cycle = BUS_CYCLE_DISABLE;
  copperEvent.handler = copperEmulate;
  blitterEvent.cycle = BUS_CYCLE_DISABLE;
  blitterEvent.handler = blitFinishBlit;
  cpuEvent.cycle = 0;
  cpuEvent.handler = cpuEventHandler;
  busInsertEvent(&eofEvent);
  busInsertEvent(&eolEvent);
//  if (insert_cpu_event) busInsertEvent(&cpuEvent);
}

/*===========================================================================*/
/* Cycle counter to raster beam position table                               */
/*===========================================================================*/

void busCycleToRasterPosTableInit(void)
{
  ULO i;

  for (i = 0; i < 0x20000; i++)
  {
    bus_cycle_to_ypos[i>>2] = (UWO) (i / 228);
    bus_cycle_to_xpos[i] = (UBY) (i % 228);
  }
}

/*===========================================================================*/
/* Called on emulation start / stop and reset                                */
/*===========================================================================*/

void busEmulationStart(void)
{
}

void busEmulationStop(void)
{
}

void busSoftReset(void)
{
  busInitializeQueue(FALSE);
}

void busHardReset(void)
{
  busInitializeQueue(TRUE);
}


/*===========================================================================*/
/* Called on emulator startup / shutdown                                     */
/*===========================================================================*/

void busStartup(void)
{
  busCycleToRasterPosTableInit();
}

void busShutdown(void)
{
  if (BUSLOG != NULL) fclose(BUSLOG);
}
