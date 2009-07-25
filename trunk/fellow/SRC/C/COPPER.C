/* @(#) $Id: COPPER.C,v 1.5 2009-07-25 03:09:00 peschau Exp $ */
/*=========================================================================*/
/* Fellow                                                                  */
/* Copper Emulation Initialization                                         */
/*                                                                         */
/* Author: Petter Schau                                                    */
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

#include "defs.h"
#include "copper.h"
#include "sprite.h"
#include "fmem.h"
#include "bus.h"
#include "blit.h"

/*=======================================================*/
/* external references not exported by the include files */
/*=======================================================*/



/*============================================================================*/
/* Not exactly correct, but a good average...?                                */
/*============================================================================*/

ULO copper_cycletable[16] = {4, 4, 4, 4, 4, 5, 6, 4,  4, 4, 4, 8, 16, 4, 4, 4};


/*============================================================================*/
/* Translation table for raster ypos to cycle translation                     */
/*============================================================================*/

ULO copper_ytable[512];
ULO copper_ptr;                                     /* Copper program counter */
BOOLE copper_dma;                                           /* Mirrors DMACON */
ULO copper_suspended_wait;/* Position the copper should have been waiting for */
/* if copper DMA had been turned on */

/*============================================================================*/
/* Copper registers                                                           */
/*============================================================================*/

ULO copcon, cop1lc, cop2lc;

/*----------------------------
/* Chip read register functions	
/*---------------------------- */

/*
========
COPCON
========
$dff02e
*/

void wcopcon(UWO data, ULO address)
{
  copcon = data;
}

/*
=======
COP1LC
=======
$dff080
*/

void wcop1lch(UWO data, ULO address)
{
  // store 21 bit (2 MB)
  *(((UWO*) &cop1lc) + 1) = data & 0x1f;
}

/* $dff082 */

void wcop1lcl(UWO data, ULO address)
{
  // no odd addresses
  *((UWO*) &cop1lc) = data & 0xfffe;
}

/*
=======
COP2LC
=======

$dff084
*/

void wcop2lch(UWO data, ULO address)
{
  // store 21 bit (2 MB)
  *(((UWO*) &cop2lc) + 1) = data & 0x1f;
}

/* $dff082 */

void wcop2lcl(UWO data, ULO address)
{
  // no odd addresses
  *((UWO*) &cop2lc) = data & 0xfffe;
}

/*
========
COPJMP1
========
$dff088
*/

void wcopjmp1(UWO data, ULO address)
{
  copperLoad1();
}

UWO rcopjmp1(ULO address)
{
  copperLoad1();
  return 0;
}

/*
========
COPJMP2
========
$dff08A
*/


void wcopjmp2(UWO data, ULO address)
{
  copperLoad2();
}

UWO rcopjmp2(ULO address)
{
  copperLoad2();
  return 0;
}

/*============================================================================*/
/* Initialize translation table                                               */
/*============================================================================*/

void copperYTableInit(void) {
  int i, ex = 16;
  spriteSetDelay(40);
  for (i = 0; i < 512; i++) {
    copper_ytable[i] = i*228 + ex;
  }
}

/*============================================================================*/
/* Module control                                                             */
/*============================================================================*/

static void copperStateClear(void) {
  cop1lc = 0;
  cop2lc = 0;
  copper_suspended_wait = 0xffffffff;
  copper_ptr = cop1lc;
  copper_dma = FALSE;
}

void copperIOHandlersInstall(void) {
  memorySetIoWriteStub(0x02e, wcopcon);
  memorySetIoWriteStub(0x080, wcop1lch);
  memorySetIoWriteStub(0x082, wcop1lcl);
  memorySetIoWriteStub(0x084, wcop2lch);
  memorySetIoWriteStub(0x086, wcop2lcl);
  memorySetIoWriteStub(0x088, wcopjmp1);
  memorySetIoWriteStub(0x08a, wcopjmp2);
  memorySetIoReadStub(0x088, rcopjmp1);
  memorySetIoReadStub(0x08a, rcopjmp2);
}

void copperSaveState(FILE *F)
{
  fwrite(&copcon, sizeof(copcon), 1, F);
  fwrite(&cop1lc, sizeof(cop1lc), 1, F);
  fwrite(&cop2lc, sizeof(cop2lc), 1, F);
  fwrite(&copper_ptr, sizeof(copper_ptr), 1, F);
  fwrite(&copper_dma, sizeof(copper_dma), 1, F);
  fwrite(&copper_suspended_wait, sizeof(copper_suspended_wait), 1, F);
}

void copperLoadState(FILE *F)
{
  fread(&copcon, sizeof(copcon), 1, F);
  fread(&cop1lc, sizeof(cop1lc), 1, F);
  fread(&cop2lc, sizeof(cop2lc), 1, F);
  fread(&copper_ptr, sizeof(copper_ptr), 1, F);
  fread(&copper_dma, sizeof(copper_dma), 1, F);
  fread(&copper_suspended_wait, sizeof(copper_suspended_wait), 1, F);
}

void copperEmulationStart(void) {
  copperIOHandlersInstall();
}


void copperEmulationStop(void) {
}


void copperHardReset(void) {
  copperStateClear();
}


void copperStartup(void) {
  copperYTableInit();
  copperStateClear();
}


void copperShutdown(void) {
}

void copperRemoveEvent(void)
{
  if (copperEvent.cycle != BUS_CYCLE_DISABLE)
  {
    busRemoveEvent(&copperEvent);
    copperEvent.cycle = BUS_CYCLE_DISABLE;
  }
}

void copperInsertEvent(ULO cycle)
{
  if (cycle != BUS_CYCLE_DISABLE)
  {
    copperEvent.cycle = cycle;
    busInsertEvent(&copperEvent);
  }
}
/*-------------------------------------------------------------------------------
; Initializes the copper from ptr 1
;-------------------------------------------------------------------------------*/

void copperLoad1(void)
{
  copper_ptr = cop1lc;

  if (copper_dma == TRUE)
  {
    copperRemoveEvent();
    copperInsertEvent(bus.cycle + 4);
  }
}

void copperLoad2(void)
{
  copper_ptr = cop2lc;

  if (copper_dma == TRUE)
  {
    copperRemoveEvent();
    copperInsertEvent(bus.cycle + 4);
  }
}

/*-------------------------------------------------------------------------------
; void copperUpdateDMA(void);
; Called by wdmacon every time that register is written
; This routine takes action when the copper DMA state is changed
;-------------------------------------------------------------------------------*/

void copperUpdateDMA(void)
{
  if (copper_dma == TRUE)
  {
    // here copper was on, test if it is still on
    if ((dmacon & 0x80) == 0x0)
    {
      // here copper DMA is being turned off
      // record which cycle the copper was waiting for
      // remove copper from the event list
      copper_suspended_wait = copperEvent.cycle;
      copper_dma = FALSE;
      copperRemoveEvent();
    }
  }
  else
  {
    // here copper was off, test if it is still off
    if ((dmacon & 0x80) == 0x80)
    {
      // here copper is being turned on
      // reactivate the cycle the copper was waiting for the last time it was on.
      // if we have passed it in the mean-time, execute immediately.
      copperRemoveEvent();
      if (copper_suspended_wait != -1)
      {
	// dma not hanging
	if (copper_suspended_wait <= bus.cycle)
	{
	  copperInsertEvent(bus.cycle + 4);
	}
	else
	{
	  copperInsertEvent(copper_suspended_wait);
	}
      }
      else
      {
	copperInsertEvent(copper_suspended_wait);
      }
      copper_dma = TRUE;
    }
  }
}

/*-------------------------------------------------------------------------------
; Called every end of frame, it restarts the copper
;-------------------------------------------------------------------------------*/

void copperEndOfFrame(void)
{
  copper_ptr = cop1lc;
  copper_suspended_wait = 40;
  if (copper_dma == TRUE)
  {
    copperRemoveEvent();
    copperInsertEvent(40);
  }
}

/*-------------------------------------------------------------------------------
; Emulates one copper instruction
;-------------------------------------------------------------------------------*/


void copperEmulate(void)
{
  ULO bswapRegC;
  ULO bswapRegD;
  BOOLE correctLine;
  ULO maskedY;
  ULO maskedX;
  ULO waitY;
  ULO currentY = busGetRasterY();
  ULO currentX = busGetRasterX();

  copperEvent.cycle = BUS_CYCLE_DISABLE;
  if (cpuEvent.cycle != BUS_CYCLE_DISABLE)
  {
    cpuEvent.cycle += 2;
  }

  // retrieve Copper command (two words)
  bswapRegC = memoryChipReadWord(copper_ptr);
  bswapRegD = memoryChipReadWord(copper_ptr + 2);
  copper_ptr = ((copper_ptr + 4) & 0x1ffffe);
  if (bswapRegC != 0xffff || bswapRegD != 0xfffe)
  {
    // check bit 0 of first instruction word, zero is move
    if ((bswapRegC & 0x1) == 0x0)
    {
      // MOVE instruction
      bswapRegC &= 0x1fe;

      // check if access to $40 - $7f (if so, Copper is using Blitter)
      if ((bswapRegC >= 0x80) || ((bswapRegC >= 0x40) && ((copcon & 0xffff) != 0x0)))
      {
	// move data to Blitter register
	copperInsertEvent(copper_cycletable[(bplcon0 >> 12) & 0xf] + bus.cycle);
	memory_iobank_write[bswapRegC >> 1]((UWO)bswapRegD, bswapRegC);
      }
    }
    else
    {
      // wait instruction or skip instruction
      bswapRegC &= 0xfffe;
      // check bit BFD (Bit Finish Disable)
      if (((bswapRegD & 0x8000) == 0x0) && blitterIsStarted())
      {
	// Copper waits until Blitter is finished
	copper_ptr -= 4;
	if ((blitterEvent.cycle + 4) <= bus.cycle)
	{
	  copperInsertEvent(bus.cycle + 4);
	}
	else
	{
	  copperInsertEvent(blitterEvent.cycle + 4);
	}
      }
      else
      {
	// Copper will not wait for Blitter to finish for executing wait or skip

	// zero is wait (works??)
	// (not really!)
	if ((bswapRegD & 0x1) == 0x0)
	{
	  // WAIT instruction 

	  // calculate our line, masked with vmask
	  // bl - masked graph_raster_y 

	  // use mask on graph_raster_y
	  bswapRegD |= 0x8000;
	  // used to indicate correct line or not
	  correctLine = FALSE;

	  // compare masked y with wait line
	  //maskedY = graph_raster_y;
	  maskedY = currentY;

	  *((UBY*) &maskedY) &= (UBY) (bswapRegD >> 8);
	  if (*((UBY*) &maskedY) > ((UBY) (bswapRegC >> 8)))
	  {
	    // we have passed the line, set up next instruction immediately
	    // cexit

	    // to help the copper to wait line 256, do some tests here
	    // Here we have detected a wait position that we are past.
	    // Check if current line is 255, then if line we are supposed
	    // to wait for are less than 60, do the wait afterall.
	    // Here we have detected a wait that has been passed.

	    // do some tests if line is 255
	    if (currentY != 255)
	    {
	      // ctrueexit
	      //Here we must do a new copper access immediately (in 4 cycles)
	      copperInsertEvent(bus.cycle + 4);
	    }
	    else
	    {
	      // test line to be waited for
	      if ((bswapRegC >> 8) > 0x40)
	      {
		// line is 256-313, wrong to not wait
		// ctrueexit
		//Here we must do a new copper access immediately (in 4 cycles)
		copperInsertEvent(bus.cycle + 4);
	      }
	      else
	      {
		// better to do the wait afterall
		// here we recalculate the wait stuff

		// invert masks
		bswapRegD = ~bswapRegD;

		// get missing bits
		maskedY = (currentY | 0x100);
		*((UBY*) &maskedY) &= (bswapRegD >> 8);
		// mask them into vertical position
		*((UBY*) &maskedY) |= (bswapRegC >> 8);
		maskedY *= BUS_CYCLE_PER_LINE;
		bswapRegC &= 0xfe;
		bswapRegD &= 0xfe;

		maskedX = (currentX & bswapRegD);
		// mask in horizontal
		bswapRegC |= maskedX;
		bswapRegC = bswapRegC + maskedY + 4;
		if (bswapRegC <= bus.cycle)
		{
		  // fix
		  bswapRegC = bus.cycle + 4;
		}
		copperInsertEvent(bswapRegC);
	      } 
	    }
	  }
	  else if (*((UBY*) &maskedY) < (UBY) (bswapRegC >> 8))
	  {
	    // we are above the line, calculate the cycle when wait is true
	    // notnever

	    // invert masks
	    bswapRegD = ~bswapRegD;

	    // get bits that is not masked out
	    maskedY = currentY;
	    *((UBY*) &maskedY) &= (bswapRegD >> 8);
	    // mask in bits from waitgraph_raster_y
	    *((UBY*) &maskedY) |= (bswapRegC >> 8);
	    waitY = copper_ytable[maskedY];

	    // when wait is on same line, use masking stuff on graph_raster_x
	    // else on graph_raster_x = 0
	    // prepare waitxpos
	    bswapRegC &= 0xfe;
	    bswapRegD &= 0xfe;
	    if (correctLine == TRUE)
	    {
	      if (bswapRegD < currentX)
	      {
		// get unmasked bits from current x
		copperInsertEvent(waitY + ((bswapRegD & currentX) | bswapRegC) + 4);
	      }
	      else
	      {
		// copwaitnotsameline
		copperInsertEvent(waitY + ((bswapRegD & 0) | bswapRegC) + 4);
	      }
	    }
	    else
	    {
	      // copwaitnotsameline
	      copperInsertEvent(waitY + ((bswapRegD & 0) | bswapRegC) + 4);
	    }
	  }
	  else
	  {
	    // here we are on the correct line
	    // calculate our xposition, masked with hmask
	    // al - masked graph_raster_x

	    correctLine = TRUE;
	    // use mask on graph_raster_x
	    maskedX = currentX;
	    *((UBY*) &maskedX) &= (bswapRegD & 0xff);
	    // compare masked x with wait x
	    if (*((UBY*) &maskedX) < (UBY) (bswapRegC & 0xff))
	    {
	      // here the wait position is not reached yet, calculate cycle when wait is true
	      // previous position checks should assure that a calculated position is not less than the current cycle
	      // notnever

	      // invert masks
	      bswapRegD = ~bswapRegD;
	      //bswapRegD &= 0xffff;

	      // get bits that is not masked out
	      maskedY = currentY;
	      *((UBY*) &maskedY) &= (UBY) (bswapRegD >> 8);
	      // mask in bits from waitgraph_raster_y
	      *((UBY*) &maskedY) |= (UBY) (bswapRegC >> 8);
	      waitY = copper_ytable[maskedY];

	      // when wait is on same line, use masking stuff on graph_raster_x
	      // else on graph_raster_x = 0

	      // prepare waitxpos
	      bswapRegC &= 0xfe;
	      bswapRegD &= 0xfe;
	      if (correctLine == TRUE)
	      {
		if (bswapRegD < currentX)
		{
		  // get unmasked bits from current x
		  copperInsertEvent(waitY + ((bswapRegD & currentX) | bswapRegC) + 4);
		}
		else
		{
		  // copwaitnotsameline
		  copperInsertEvent(waitY + ((bswapRegD & 0) | bswapRegC) + 4);
		}
	      }
	      else
	      {
		// copwaitnotsameline
		copperInsertEvent(waitY + ((bswapRegD & 0) | bswapRegC) + 4);
	      }
	    }
	    else
	    {
	      // position reached, set up next instruction immediately
	      // cexit

	      // to help the copper to wait line 256, do some tests here
	      // Here we have detected a wait position that we are past.
	      // Check if current line is 255, then if line we are supposed
	      // to wait for are less than 60, do the wait afterall.

	      // Here we have detected a wait that has been passed.

	      // do some tests if line is 255
	      if (currentY != 255)
	      {
		// ctrueexit
		//Here we must do a new copper access immediately (in 4 cycles)
		copperInsertEvent(bus.cycle + 4);
	      }
	      else
	      {
		// test line to be waited for
		if ((bswapRegC >> 8) > 0x40)
		{
		  // line is 256-313, wrong to not wait
		  // ctrueexit
		  //Here we must do a new copper access immediately (in 4 cycles)
		  copperInsertEvent(bus.cycle + 4);
		}
		else
		{
		  // better to do the wait afterall
		  // here we recalculate the wait stuff

		  // invert masks
		  bswapRegD = ~bswapRegD;

		  // get missing bits
		  maskedY = (currentY | 0x100);
		  *((UBY*) &maskedY) &= (bswapRegD >> 8);
		  // mask them into vertical position
		  *((UBY*) &maskedY) |= (bswapRegC >> 8);
		  maskedY *= BUS_CYCLE_PER_LINE;
		  bswapRegC &= 0xfe;
		  bswapRegD &= 0xfe;

		  maskedX = (currentX & bswapRegD);
		  // mask in horizontal
		  bswapRegC |= maskedX;
		  bswapRegC = bswapRegC + maskedY + 4;
		  if (bswapRegC <= bus.cycle)
		  {
		    // fix
		    bswapRegC = bus.cycle + 4;
		  }
		  copperInsertEvent(bswapRegC);
		} 
	      }
	    }
	  }
	}
	else
	{
	  // SKIP instruction

	  // new skip, copied from cwait just to try something
	  // cskip

	  // calculate our line, masked with vmask
	  // bl - masked graph_raster_y 

	  // use mask on graph_raster_y
	  bswapRegD |= 0x8000;
	  // used to indicate correct line or not
	  correctLine = FALSE;
	  maskedY = currentY;
	  *((UBY*) &maskedY) &= ((bswapRegD >> 8));
	  if (*((UBY*) &maskedY) > (UBY) (bswapRegC >> 8))
	  {
	    // do skip
	    // we have passed the line, set up next instruction immediately
	    copper_ptr += 4;
	    copperInsertEvent(bus.cycle + 4);
	  }
	  else if (*((UBY *) &maskedY) < (UBY) (bswapRegC >> 8))
	  {
	    // above line, don't skip
	    copperInsertEvent(bus.cycle + 4);
	  }
	  else
	  {
	    // here we are on the correct line

	    // calculate our xposition, masked with hmask
	    // al - masked graph_raster_x
	    correctLine = TRUE;		

	    // use mask on graph_raster_x
	    // Compare masked x with wait x
	    maskedX = currentX;
	    *((UBY*) &maskedX) &= bswapRegD;
	    if (*((UBY*) &maskedX) >= (UBY) (bswapRegC & 0xff))
	    {
	      // position reached, set up next instruction immediately
	      copper_ptr += 4;
	    }
	    copperInsertEvent(bus.cycle + 4);
	  }
	}
      }
    }
  }
}
