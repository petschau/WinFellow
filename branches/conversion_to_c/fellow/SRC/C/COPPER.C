/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Copper Emulation Initialization                                            */
/*                                                                            */
/* Author: Petter Schau (peschau@online.no)                                   */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/


#include "portable.h"
#include "renaming.h"

#include "defs.h"
#include "copper.h"
#include "sprite.h"
#include "fmem.h"
#include "bus.h"

/*============================================================================*/
/* Not exactly correct, but a good average...?                                */
/*============================================================================*/

ULO copper_cycletable[16] = {4, 4, 4, 4, 4, 5, 6, 4,  4, 4, 4, 8, 16, 4, 4, 4};


/*============================================================================*/
/* Translation table for raster ypos to cycle translation                     */
/*============================================================================*/

ULO copper_ytable[512];
ULO copper_ptr;                                     /* Copper program counter */
ULO copper_next;                                /* Cycle for next instruction */
BOOLE copper_dma;                                           /* Mirrors DMACON */
ULO copper_suspended_wait;/* Position the copper should have been waiting for */
                                          /* if copper DMA had been turned on */

/*============================================================================*/
/* Copper registers                                                           */
/*============================================================================*/

ULO copcon, cop1lc, cop2lc;


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
  copper_suspended_wait = copper_next = 0xffffffff;
  copper_ptr = cop1lc;
  copper_dma = FALSE;
}

void copperIOHandlersInstall(void) {
  memorySetIOWriteStub(0x02e, wcopcon);
  memorySetIOWriteStub(0x080, wcop1lch);
  memorySetIOWriteStub(0x082, wcop1lcl);
  memorySetIOWriteStub(0x084, wcop2lch);
  memorySetIOWriteStub(0x086, wcop2lcl);
  memorySetIOWriteStub(0x088, wcopjmp1);
  memorySetIOWriteStub(0x08a, wcopjmp2);
  memorySetIOReadStub(0x088, rcopjmp1_C);
  memorySetIOReadStub(0x08a, rcopjmp2_C);
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

/*----------------------------
/* Chip read register functions	
/*---------------------------- */

/*========
; COPJMP1
;========
; $dff088 - word*/

ULO rcopjmp1_C(ULO address)
{
	copperLoad1C();
	return 0;
}


/*========
; COPJMP2
;========
; $dff08A - word*/

ULO rcopjmp2_C(ULO address)
{
	copperLoad2C();
	return 0;
}

/*-------------------------------------------------------------------------------
; Initializes the copper from ptr 1
;-------------------------------------------------------------------------------*/

void copperLoad1C(void)
{
	copper_ptr = cop1lc;

	if (copper_dma == TRUE)
	{
		copper_next = curcycle + 4;
		busScanEventsLevel2();
	}
}
	
void copperLoad2C(void)
{
	copper_ptr = cop2lc;

	if (copper_dma == TRUE)
	{
		copper_next = curcycle + 4;
		busScanEventsLevel2();
	}
}
