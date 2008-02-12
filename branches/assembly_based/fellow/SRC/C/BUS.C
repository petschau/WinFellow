/* @(#) $Id: BUS.C,v 1.2 2004-06-08 14:28:57 carfesh Exp $ */
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

#include "portable.h"
#include "renaming.h"

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


ULO debugging;
ULO curcycle;
ULO eol_next, eof_next;
ULO lvl2_next;
ULO lvl3_next;
ULO lvl4_next;
ULO lvl5_next;
ULO lvl6_next;
ULO lvl7_next;
buseventfunc lvl2_ptr, lvl3_ptr, lvl4_ptr, lvl5_ptr, lvl6_ptr, lvl7_ptr;

UWO bus_cycle_to_ypos[0x20000];
UBY bus_cycle_to_xpos[0x20000];


/*===========================================================================*/
/* Cycle counter to raster beam position table                               */
/*===========================================================================*/

void busCycleToRasterPosTableInit(void) {
  int i;

  for (i = 0; i < 0x20000; i++) {
    bus_cycle_to_ypos[i>>2] = (UWO) (i / 228);
    bus_cycle_to_xpos[i] = (UBY) (i % 228);
  }
}


/*===========================================================================*/
/* Set up events for initial execution                                       */
/*===========================================================================*/

void busEventlistClear(void) {
  lvl7_next = CYCLESPERFRAME;
  lvl7_ptr = endOfFrame;
  lvl6_next = CYCLESPERFRAME;
  lvl6_ptr = endOfFrame;
  lvl5_next = CYCLESPERFRAME;
  lvl5_ptr = endOfFrame;
  lvl4_next = CYCLESPERFRAME;
  lvl4_ptr = endOfFrame;
  lvl3_next = CYCLESPERLINE - 1;
  lvl3_ptr = endOfLine;
  lvl2_next = CYCLESPERLINE - 1;
  lvl2_ptr = endOfLine;
}


/*===========================================================================*/
/* Called on emulation start / stop and reset                                */
/*===========================================================================*/

void busEmulationStart(void) {
}

void busEmulationStop(void) {
}

void busHardReset(void) {
  busEventlistClear();
  irq_next = 0xffffffff;
  curcycle = 0;
  cpu_next = 0;
  eol_next = CYCLESPERLINE - 1;
  eof_next = CYCLESPERFRAME;
}


/*===========================================================================*/
/* Called on emulator startup / shutdown                                     */
/*===========================================================================*/

void busStartup(void) {
  busCycleToRasterPosTableInit();
  curcycle = 0;
}

void busShutdown(void) {
}
