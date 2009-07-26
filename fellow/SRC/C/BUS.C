/* @(#) $Id: BUS.C,v 1.7 2009-07-26 22:56:07 peschau Exp $ */
/*=========================================================================*/
/* Fellow							           */
/*                                                                         */
/* Bus Event Scheduler                                                     */
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

#include "defs.h"
#include "fellow.h"
#include "CpuModule.h"
#include "CpuIntegration.h"
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
#include "fileops.h"

bus_state bus;

bus_event cpuEvent;
bus_event copperEvent;
bus_event eolEvent;
bus_event eofEvent;
bus_event ciaEvent;
bus_event blitterEvent;

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
  drawEndOfFrame();

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
    // The CPU is never in the queue
    cpuEvent.cycle -= BUS_CYCLE_PER_FRAME;
  }

  /*==============================================================*/
  /* Perform graphics end of frame                                */
  /*==============================================================*/
  graphEndOfFrame();
  timerEndOfFrame();

  busInsertEvent(&eofEvent);
  bus.frame_no++;
}

void busRemoveEvent(bus_event *ev)
{
  bus_event *tmp;
  BOOLE found = FALSE;
  for (tmp = bus.events; tmp != NULL; tmp = tmp->next)
  {
    if (tmp == ev)
    {
      found = TRUE;
      break;
    }
  }
  if (!found)
  {
    return;
  }

  if (ev->prev == NULL)
  {
    bus.events = ev->next;
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
  if (bus.events == NULL)
  {
    ev->prev = ev->next = NULL;
    bus.events = ev;
  }
  else
  {
    bus_event *tmp;
    bus_event *tmp_prev = NULL;
    for (tmp = bus.events; tmp != NULL; tmp = tmp->next)
    {
      if (ev->cycle < tmp->cycle)
      {
	ev->next = tmp;
	ev->prev = tmp_prev;
	tmp->prev = ev;
	if (tmp_prev == NULL) bus.events = ev; /* In front */
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
  if (bus.events->cycle >= cpuEvent.cycle)
  {
    return &cpuEvent;
  }
  else
  {
    bus_event *tmp = bus.events;
    bus.events = tmp->next;
    bus.events->prev = NULL;
    return tmp;
  }
}

FILE *BUSLOG = NULL;
BOOLE bus_log = FALSE;

FILE *busOpenLog(void)
{
  char filename[MAX_PATH];
  fileopsGetGenericFileName(filename, "bus.log");
  return fopen(filename, "w");
}

void busCloseLog(void)
{
  if (BUSLOG) fclose(BUSLOG);
}

void busEventLog(bus_event *e)
{
  if (!bus_log) return;
  if (BUSLOG == NULL) BUSLOG = busOpenLog();
  if (e == &copperEvent)
    fprintf(BUSLOG, "%d copper\n", e->cycle);
  else if (e == &ciaEvent)
    fprintf(BUSLOG, "%d cia\n", e->cycle);
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
  if (BUSLOG == NULL) BUSLOG = busOpenLog();
  if (BUSLOG) fprintf(BUSLOG, "%d %s\n", cpuEvent.cycle, s);
}

void busSetCycle(ULO cycle)
{
  bus.cycle = cycle;
}

ULO busGetCycle(void)
{
  return bus.cycle;
}

ULO busGetRasterY(void)
{
  return bus.cycle / BUS_CYCLE_PER_LINE;
}

ULO busGetRasterX(void)
{
  return bus.cycle % BUS_CYCLE_PER_LINE;
}

ULL busGetRasterFrameCount(void)
{
  return bus.frame_no;
}

void busRunNew(void)
{
  while (!fellow_request_emulation_stop && !fellow_request_emulation_stop_immediately)
  {
    if (cpuIntegrationMidInstructionExceptionGuard() == 0)
    {
      while (!fellow_request_emulation_stop && !fellow_request_emulation_stop_immediately)
      {
	bus_event *e = busPopEvent();
	busEventLog(e);
	busSetCycle(e->cycle);
	e->handler();
      }
    }
    else
    {
      // Came out of an CPU exception. Keep on working.
      cpuEvent.cycle = bus.cycle + cpuIntegrationGetChipCycles() + (cpuGetInstructionTime() >> cpuIntegrationGetSpeed());
      cpuIntegrationSetChipCycles(0);
    }
  }
}

void busDebugNew(void)
{
  while (!fellow_request_emulation_stop && !fellow_request_emulation_stop_immediately)
  {
    if (cpuIntegrationMidInstructionExceptionGuard() == 0)
    {
      while (!fellow_request_emulation_stop && !fellow_request_emulation_stop_immediately)
      {
	bus_event *e = busPopEvent();
	busEventLog(e);
	busSetCycle(e->cycle);
	e->handler();
	if (e == &cpuEvent) return;
      }
    }
    else
    {
      // Came out of an CPU exception. Keep on working.
      cpuEvent.cycle = bus.cycle + cpuIntegrationGetChipCycles() + (cpuGetInstructionTime() >> cpuIntegrationGetSpeed());
      cpuIntegrationSetChipCycles(0);
    }
  }
}

void busInitializeQueue(void)
{
  memset(&cpuEvent, 0, sizeof(bus_event));
  memset(&copperEvent, 0, sizeof(bus_event));
  memset(&eolEvent, 0, sizeof(bus_event));
  memset(&eofEvent, 0, sizeof(bus_event));
  memset(&ciaEvent, 0, sizeof(bus_event));
  memset(&blitterEvent, 0, sizeof(bus_event));
  bus.events = NULL;

  eolEvent.cycle = BUS_CYCLE_PER_LINE - 1;
  eolEvent.handler = busEndOfLine;
  eofEvent.cycle = BUS_CYCLE_PER_FRAME;
  eofEvent.handler = busEndOfFrame;
  ciaEvent.cycle = BUS_CYCLE_DISABLE;
  ciaEvent.handler = ciaHandleEvent;
  copperEvent.cycle = BUS_CYCLE_DISABLE;
  copperEvent.handler = copperEmulate;
  blitterEvent.cycle = BUS_CYCLE_DISABLE;
  blitterEvent.handler = blitFinishBlit;
  cpuEvent.cycle = 0;
  cpuEvent.handler = cpuIntegrationExecuteInstructionEventHandler;
  busInsertEvent(&eofEvent);
  busInsertEvent(&eolEvent);
}

/*===========================================================================*/
/* Called on emulation start / stop and reset                                */
/*===========================================================================*/

void busSaveState(FILE *F)
{
  fwrite(&bus.frame_no, sizeof(bus.frame_no), 1, F);
  fwrite(&bus.cycle, sizeof(bus.cycle), 1, F);
  fwrite(&cpuEvent.cycle, sizeof(cpuEvent.cycle), 1, F);
  fwrite(&copperEvent.cycle, sizeof(copperEvent.cycle), 1, F);
  fwrite(&eolEvent.cycle, sizeof(eolEvent.cycle), 1, F);
  fwrite(&eofEvent.cycle, sizeof(eofEvent.cycle), 1, F);
  fwrite(&ciaEvent.cycle, sizeof(ciaEvent.cycle), 1, F);
  fwrite(&blitterEvent.cycle, sizeof(blitterEvent.cycle), 1, F);
}

void busLoadState(FILE *F)
{
  fread(&bus.frame_no, sizeof(bus.frame_no), 1, F);
  fread(&bus.cycle, sizeof(bus.cycle), 1, F);
  fread(&cpuEvent.cycle, sizeof(cpuEvent.cycle), 1, F);
  fread(&copperEvent.cycle, sizeof(copperEvent.cycle), 1, F);
  fread(&eolEvent.cycle, sizeof(eolEvent.cycle), 1, F);
  fread(&eofEvent.cycle, sizeof(eofEvent.cycle), 1, F);
  fread(&ciaEvent.cycle, sizeof(ciaEvent.cycle), 1, F);
  fread(&blitterEvent.cycle, sizeof(blitterEvent.cycle), 1, F);

  bus.events = NULL;
  if (cpuEvent.cycle != BUS_CYCLE_DISABLE) busInsertEvent(&cpuEvent);
  if (copperEvent.cycle != BUS_CYCLE_DISABLE) busInsertEvent(&copperEvent);
  if (eolEvent.cycle != BUS_CYCLE_DISABLE) busInsertEvent(&eolEvent);
  if (eofEvent.cycle != BUS_CYCLE_DISABLE) busInsertEvent(&eofEvent);
  if (ciaEvent.cycle != BUS_CYCLE_DISABLE) busInsertEvent(&ciaEvent);
  if (blitterEvent.cycle != BUS_CYCLE_DISABLE) busInsertEvent(&blitterEvent);
}

void busEmulationStart(void)
{
}

void busEmulationStop(void)
{
}

void busSoftReset(void)
{
  busInitializeQueue();
}

void busHardReset(void)
{
  busInitializeQueue();
}

/*===========================================================================*/
/* Called on emulator startup / shutdown                                     */
/*===========================================================================*/

void busStartup(void)
{
  bus.frame_no = 0;
  bus.cycle = 0;
}

void busShutdown(void)
{
  busCloseLog();
}
