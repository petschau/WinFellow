/* @(#) $Id: BUS.C,v 1.1.1.1.2.6 2004-06-08 14:24:58 carfesh Exp $ */
/*=========================================================================*/
/* Fellow Amiga Emulator                                                   */
/*                                                                         */
/* Bus Event Scheduler Initialization                                      */
/*                                                                         */
/* Author: Petter Schau (peschau@online.no)                                */
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

extern ULO copper_ptr, copper_next;

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
  lvl3_ptr = endOfLineC;
  lvl2_next = CYCLESPERLINE - 1;
  lvl2_ptr = endOfLineC;
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

void busScanLevel6(ULO* lvlx_next, buseventfunc* lvlx_ptr)
{ 
  if ((*lvlx_next) > (ULO) cia_next_event_time)
  {
    *lvlx_next = cia_next_event_time;
    *lvlx_ptr = ciaHandleEventWrapper;
  }
  lvl6_next = *lvlx_next;
  lvl6_ptr = *lvlx_ptr; 
}

void busScanLevel5(ULO* lvlx_next, buseventfunc* lvlx_ptr)
{ 
  if ((*lvlx_next) > irq_next)
  {
    *lvlx_next = irq_next;
    *lvlx_ptr = cpuSetUpInterrupt;
  }
  lvl5_next = *lvlx_next;
  lvl5_ptr = *lvlx_ptr; 
}

void busScanLevel4(ULO* lvlx_next, buseventfunc* lvlx_ptr)
{ 
  if ((*lvlx_next) > blitend)
  {
    *lvlx_next = blitend;
    *lvlx_ptr = blitFinishBlit;
  }
  lvl4_next = *lvlx_next;
  lvl4_ptr = *lvlx_ptr; 
}

void busScanLevel3(ULO* lvlx_next, buseventfunc* lvlx_ptr)
{ 
  if ((*lvlx_next) > eol_next)
  {
    *lvlx_next = eol_next;
    *lvlx_ptr = endOfLineC;
  }
  lvl3_next = *lvlx_next;
  lvl3_ptr = *lvlx_ptr; 
}

void busScanLevel2(ULO* lvlx_next, buseventfunc* lvlx_ptr)
{ 
  if ((*lvlx_next) > copper_next)
  {
    *lvlx_next = copper_next;
    *lvlx_ptr = copperEmulate;
  }
  lvl2_next = *lvlx_next;
  lvl2_ptr = *lvlx_ptr; 
}

void busScanEventsLevel2(void)
{ 
  ULO lvlx_next;
  buseventfunc lvlx_ptr;

  lvlx_next = lvl3_next;
  lvlx_ptr = lvl3_ptr;

  busScanLevel2(&lvlx_next, &lvlx_ptr);
}

void busScanEventsLevel3(void)
{ 
  ULO lvlx_next;
  buseventfunc lvlx_ptr;

  lvlx_next = lvl4_next;
  lvlx_ptr = lvl4_ptr;

  busScanLevel3(&lvlx_next, &lvlx_ptr);
  busScanLevel2(&lvlx_next, &lvlx_ptr);
}

void busScanEventsLevel4(void)
{ 
  ULO lvlx_next;
  buseventfunc lvlx_ptr;

  lvlx_next = lvl5_next;
  lvlx_ptr = lvl5_ptr;

  busScanLevel4(&lvlx_next, &lvlx_ptr);
  busScanLevel3(&lvlx_next, &lvlx_ptr);
  busScanLevel2(&lvlx_next, &lvlx_ptr);
}

void busScanEventsLevel5(void)
{ 
  ULO lvlx_next;
  buseventfunc lvlx_ptr;

  lvlx_next = lvl6_next;
  lvlx_ptr = lvl6_ptr;

  busScanLevel5(&lvlx_next, &lvlx_ptr);
  busScanLevel4(&lvlx_next, &lvlx_ptr);
  busScanLevel3(&lvlx_next, &lvlx_ptr);
  busScanLevel2(&lvlx_next, &lvlx_ptr);
}

void busScanEventsLevel6(void)
{ 
  ULO lvlx_next;
  buseventfunc lvlx_ptr;

  lvlx_next = CYCLESPERFRAME;
  lvlx_ptr = endOfFrame;

  busScanLevel6(&lvlx_next, &lvlx_ptr);
  busScanLevel5(&lvlx_next, &lvlx_ptr);
  busScanLevel4(&lvlx_next, &lvlx_ptr);
  busScanLevel3(&lvlx_next, &lvlx_ptr);
  busScanLevel2(&lvlx_next, &lvlx_ptr);
}


/*==============================================================================*/
/* Global end of line handler                                                   */
/*==============================================================================*/

void endOfLineC(void)
{

  /*==============================================================*/
	/* Handles graphics planar to chunky conversion                 */
	/* and updates the graphics emulation for a new line            */
	/*==============================================================*/
  
  graphEndOfLine(); // assembly function

	/*==============================================================*/
	/* Update the CIA B event counter                               */
	/*==============================================================*/

  ciaUpdateEventCounterC(1);

	/*==============================================================*/
	/* Handles disk DMA if it is running                            */
	/*==============================================================*/

	floppyEndOfLineC();

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

	eol_next += CYCLESPERLINE;
	busScanEventsLevel3();
}

/*==============================================================================*/
/* Global end of frame handler                                                  */
/*==============================================================================*/


/*
		FALIGN32

global _endOfFrame_
_endOfFrame_:	push	edx
		push	ebx


		;==============================================================
		; Draw the frame in the host buffer
		;==============================================================

		call	_drawEndOfFrame_


		;==============================================================
		; Handle keyboard events
		;==============================================================

		KBDEVENTEOFHANDLER_CWRAP


		;==============================================================
		; Reset some aspects of the graphics emulation
		;==============================================================

		xor	dword [lof], 08000h		; Short/long frame 
		xor	edx, edx
		mov	ecx, 0102h			; Zero scroll

		pushad
		call	dword [memory_iobank_write + 2*ecx]
		popad


		;==============================================================
		; Restart copper
		;==============================================================

		call	_copperEndOfFrame_		; Restart copper


		;==============================================================
		; Update frame counters
		;==============================================================

		inc	dword [draw_frame_count]	; Count frames
		dec	dword [draw_frame_skip]		; Frame skipping
		jns	.l6
		mov	edx, dword [draw_frame_skip_factor]
		mov	dword [draw_frame_skip], edx
.l6:

		;==============================================================
		; Update CIA timer counters
		;==============================================================

		CIA_UPDATE_TIMERS_EOF_CWRAP


		;==============================================================
		; Sprite end of frame updates
		;==============================================================

		SPRITE_EOF_CWRAP


		;==============================================================
		; Recalculate blitter finished time
		;==============================================================

		mov	dword [graph_playfield_on], ecx
		test	dword [blitend], 0ffffffffh
		js	.l14
		sub	dword [blitend], CYCLESPERFRAME
		setns	byte [blitend]
.l14:

		;==============================================================
		; Flag vertical refresh IRQ
		;==============================================================

		mov	edx, 08020h

		pushad
		call	dword [memory_iobank_write + 0138h]
		popad


		;==============================================================
		; Set up next end of line event
		;==============================================================

		mov	dword [eol_next], CYCLESPERLINE
		dec	dword [eol_next]


		;==============================================================
		; Update next CPU instruction time
		;==============================================================

		mov	ebx, dword [cpu_next]
		test	ebx, ebx
		js	.l15
		sub	ebx, CYCLESPERFRAME
		mov	dword [cpu_next], ebx
.l15:

		;==============================================================
		; Update next IRQ time
		;==============================================================

		mov	ebx, dword [irq_next]
		test	ebx, ebx
		js	.l16
		sub	ebx, CYCLESPERFRAME
		mov	dword [irq_next], ebx
.l16:

		;==============================================================
		; Perform graphics end of frame
		;==============================================================

		call	graphEndOfFrame
		call	timerEndOfFrame

		;==============================================================
		; Recalculate the entire event queue
		;==============================================================

		call busScanEventsLevel6
		;SCAN_EVENTS_LVL6


		;==============================================================
		; Bail out of emulation if we're asked to
		;==============================================================

		test	dword [fellow_request_emulation_stop], 1
		jnz	.l18
		test	dword [fellow_request_emulation_stop_immediately], 1
		jz	.l17
.l18:		mov	esp, dword [exceptionstack]
		jmp	bus_exit

.l17:		pop	ebx
		pop	edx
		ret
*/