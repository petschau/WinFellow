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
  memorySetIOWriteStub(0x02e, wcopcon_C);
  memorySetIOWriteStub(0x080, wcop1lch_C);
  memorySetIOWriteStub(0x082, wcop1lcl_C);
  memorySetIOWriteStub(0x084, wcop2lch_C);
  memorySetIOWriteStub(0x086, wcop2lcl_C);
  memorySetIOWriteStub(0x088, wcopjmp1_C);
  memorySetIOWriteStub(0x08a, wcopjmp2_C);
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
; COPCON
;========
; $dff02e*/

void wcopcon_C(ULO data, ULO address)
{
	copcon = data;
}

/*=======
; COP1LC
;=======
; $dff080 - WORD*/

void wcop1lch_C(ULO data, ULO address)
{
	// store 21 bit (2 MB)
	*(((UWO*) &cop1lc) + 1) = (UWO) (data & 0x1f);
}

/*; $dff082 - WORD*/

void wcop1lcl_C(ULO data, ULO address)
{
	// no odd addresses
	*((UWO*) &cop1lc)= (UWO) (data & 0x0000fffe);
}

/*=======
; COP2LC
;=======

; $dff084 - WORD*/

void wcop2lch_C(ULO data, ULO address)
{
	// store 21 bit (2 MB)
	*(((UWO*) &cop2lc) + 1) = (UWO) (data & 0x1f);
}

/*; $dff082 - WORD*/

void wcop2lcl_C(ULO data, ULO address)
{
	// no odd addresses
	*((UWO*) &cop2lc)= (UWO) (data & 0x0000fffe);
}

/*;========
; COPJMP1
;========
; $dff088 - word*/

void wcopjmp1_C(ULO data, ULO address)
{
	copperLoad1C();
}

ULO rcopjmp1_C(ULO address)
{
	copperLoad1C();
	return 0;
}

/*========
; COPJMP2
;========
; $dff08A - word*/


void wcopjmp2_C(ULO data, ULO address)
{
	copperLoad2C();
}

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
			copper_suspended_wait = copper_next;
			copper_next = -1;
			copper_dma = FALSE;

			busScanEventsLevel2();
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
			if (copper_suspended_wait != -1)
			{
				// dma not hanging
				if (copper_suspended_wait <= curcycle)
				{
					copper_next = curcycle + 4;
				}
				else
				{
					copper_next = copper_suspended_wait;
				}
			}
			else
			{
				copper_next = copper_suspended_wait;
			}
			busScanEventsLevel2();
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
		copper_next = 40;
		busScanEventsLevel2();
	}
}

/*-------------------------------------------------------------------------------
; Emulates one copper instruction
;-------------------------------------------------------------------------------*/


void copperEmulateC(void)
{
	UBY bswapRegA[4];
	UBY bswapRegB[4];
	ULO bswapRegC;
	ULO bswapRegD;
	BOOLE correctLine;
	ULO maskedY;
	ULO maskedX;
	ULO waitX;

	if (cpu_next != -1)
	{
		cpu_next += 2;
	}
	
	// retrieve Copper command (two words)
	*((ULO *) &bswapRegA) = *((ULO *) (memory_chip + copper_ptr));
	bswapRegB[0] = bswapRegA[3];
	bswapRegB[1] = bswapRegA[2];
	bswapRegB[2] = bswapRegA[1];
	bswapRegB[3] = bswapRegA[0];
	bswapRegC = *((ULO*) &bswapRegB);
	bswapRegD = bswapRegC;

	copper_ptr = ((copper_ptr + 4) & 0x1ffffe);
	if (bswapRegC != 0xfffffffe)
	{
		// ecx
		bswapRegC = bswapRegC >> 16;
		// edx 
		bswapRegD &= 0xffff;

		// check bit 0 of first instruction word, zero is move
		if ((bswapRegC & 0x1) == 0x0)
		{
			// move instruction
			bswapRegC &= 0x1fe;
			copper_next = copper_cycletable[(bplcon0 >> 12) & 0xf] + curcycle;

			busScanEventsLevel2();

			// check if access to $40 - $7f (if so, Copper is using Blitter)
			if ((bswapRegC >= 0x80) || ((bswapRegC >= 0x40) && ((copcon & 0xffff) != 0x0)))
			{
				// move data to Blitter register
				memory_iobank_write[bswapRegC >> 1](bswapRegD, bswapRegC);
				busScanEventsLevel2();
			}
			else
			{
				// not okay
				copper_next = -1;
				busScanEventsLevel2();
			}
		}
		else
		{
			// wait instruction or skip instruction
			bswapRegC &= 0xfffe;
			// check bit BFD (Bit Finish Disable)
			if (((bswapRegD & 0x8000) == 0x0) && ((blit_started & 0xff) != 0x0))
			{
				// Copper waits until Blitter is finished
				copper_ptr -= 4;
				if ((blitend + 4) <= curcycle)
				{
					copper_next = curcycle + 4;
				}
				else
				{
					copper_next = blitend + 4;
				}
				busScanEventsLevel2();
			}
			else
			{
				// Copper will not wait for Blitter to finish for executing wait or skip

				// zero is wait (works??)
				// (not really!)
				if ((bswapRegD & 0x1) == 0x0)
				{
					// wait instruction 

					// calculate our line, masked with vmask
					// bl - masked graph_raster_y 

					// use mask on graph_raster_y
					bswapRegD |= 0x8000;
					// used to indicate correct line or not
					correctLine = FALSE;

					// compare masked y with wait line
					maskedY = graph_raster_y;
					*((UBY*) &maskedY) &= ((bswapRegD >> 8) & 0xff);
					if ((maskedY & 0xff) > ((bswapRegC >> 8) & 0xff))
					{
						// we have passed the line, set up next instruction immediately
						// cexit

						// to help the copper to wait line 256, do some tests here
						// Here we have detected a wait position that we are past.
						// Check if current line is 255, then if line we are supposed
						// to wait for are less than 60, do the wait afterall.

						// Here we have detected a wait that has been passed.
						
						// do some tests if line is 255
						if (graph_raster_y != 255)
						{
							// ctrueexit
							//Here we must do a new copper access immediately (in 4 cycles)
							copper_next = curcycle + 4;
							busScanEventsLevel2();
						}
						else
						{
							// test line to be waited for
							if (((bswapRegC >> 8) & 0xff) > 0x40)
							{
								// line is 256-313, wrong to not wait
								// ctrueexit
								//Here we must do a new copper access immediately (in 4 cycles)
								copper_next = curcycle + 4;
								busScanEventsLevel2();
							}
							else
							{
								// better to do the wait afterall
								// here we recalculate the wait stuff
								
								// invert masks
								bswapRegD = ~bswapRegD;
								//bswapRegD &= 0xffff;

								// get missing bits
								maskedY = (graph_raster_y | 0x100);
								*((UBY*) &maskedY) &= ((bswapRegD >> 8) & 0xff);
								// mask them into vertical position
								*((UBY*) &maskedY) |= ((bswapRegC >> 8) & 0xff);
								maskedY *= CYCLESPERLINE;
								bswapRegC &= 0xfe;
								bswapRegD &= 0xfe;
								
								maskedX = (graph_raster_x & bswapRegD);
								// mask in horizontal
								bswapRegC |= maskedX;
								bswapRegC = bswapRegC + maskedY + 4;
								if (bswapRegC <= curcycle)
								{
									// fix
									bswapRegC = curcycle + 4;
								}
								copper_next = bswapRegC;
								busScanEventsLevel2();
							} 
						}
					}
					else if ((maskedY & 0xff) < ((bswapRegC >> 8) & 0xff))
					{
						// we are above the line, calculate the cycle when wait is true
						// notnever

						// invert masks
						bswapRegD = ~bswapRegD;
						//bswapRegD &= 0xffff;

						// get bits that is not masked out
						maskedX = graph_raster_y;
						*((UBY*) &maskedX) &= ((bswapRegD >> 8) & 0xff);
						// mask in bits from waitgraph_raster_y
						*((UBY*) &maskedX) |= ((bswapRegC >> 8) & 0xff);
						waitX = copper_ytable[maskedX];
						
						// when wait is on same line, use masking stuff on graph_raster_x
						// else on graph_raster_x = 0

						// prepare waitxpos
						bswapRegC &= 0xfe;
						bswapRegD &= 0xfe;
						if (correctLine == TRUE)
						{
							if (bswapRegD < graph_raster_x)
							{
								// get unmasked bits from current x
								copper_next = waitX + ((bswapRegD & graph_raster_x) | bswapRegC) + 4;
								busScanEventsLevel2();
							}
							else
							{
								// copwaitnotsameline
								copper_next = waitX + ((bswapRegD & 0) | bswapRegC) + 4;
								busScanEventsLevel2();
							}
						}
						else
						{
							// copwaitnotsameline
							copper_next = waitX + ((bswapRegD & 0) | bswapRegC) + 4;
							busScanEventsLevel2();
						}
					}
					else
					{
						// here we are on the correct line
						// calculate our xposition, masked with hmask
						// al - masked graph_raster_x

						correctLine = TRUE;
						// use mask on graph_raster_x
						maskedX = graph_raster_x;
						*((UBY*) &maskedX) &= (bswapRegD & 0xff);
						// compare masked x with wait x
						if ((maskedX & 0xff) < (bswapRegC & 0xff))
						{
							// here the wait position is not reached yet, calculate cycle when wait is true
							// previous position checks should assure that a calculated position is not less than the current cycle
							// notnever

							// invert masks
							bswapRegD = ~bswapRegD;
							//bswapRegD &= 0xffff;

							// get bits that is not masked out
							maskedX = graph_raster_x;
							*((UBY*) &maskedX) &= ((bswapRegD >> 8) & 0xff);
							// mask in bits from waitgraph_raster_y
							*((UBY*) &maskedX) |= ((bswapRegC >> 8) & 0xff);
							waitX = copper_ytable[maskedX];
							
							// when wait is on same line, use masking stuff on graph_raster_x
							// else on graph_raster_x = 0

							// prepare waitxpos
							bswapRegC &= 0xfe;
							bswapRegD &= 0xfe;
							if (correctLine == TRUE)
							{
								if (bswapRegD < graph_raster_x)
								{
									// get unmasked bits from current x
									copper_next = waitX + ((bswapRegD & graph_raster_x) | bswapRegC) + 4;
									busScanEventsLevel2();
								}
								else
								{
									// copwaitnotsameline
									copper_next = waitX + ((bswapRegD & 0) | bswapRegC) + 4;
									busScanEventsLevel2();
								}
							}
							else
							{
								// copwaitnotsameline
								copper_next = waitX + ((bswapRegD & 0) | bswapRegC) + 4;
								busScanEventsLevel2();
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
							if (graph_raster_y != 255)
							{
								// ctrueexit
								//Here we must do a new copper access immediately (in 4 cycles)
								copper_next = curcycle + 4;
								busScanEventsLevel2();
							}
							else
							{
								// test line to be waited for
								if (((bswapRegC >> 8) & 0xff) > 0x40)
								{
									// line is 256-313, wrong to not wait
									// ctrueexit
									//Here we must do a new copper access immediately (in 4 cycles)
									copper_next = curcycle + 4;
									busScanEventsLevel2();
								}
								else
								{
									// better to do the wait afterall
									// here we recalculate the wait stuff
									
									// invert masks
									bswapRegD = ~bswapRegD;
									//bswapRegD &= 0xffff;

									// get missing bits
									maskedY = (graph_raster_y | 0x100);
									*((UBY*) &maskedY) &= ((bswapRegD >> 8) & 0xff);
									// mask them into vertical position
									*((UBY*) &maskedY) |= ((bswapRegC >> 8) & 0xff);
									maskedY *= CYCLESPERLINE;
									bswapRegC &= 0xfe;
									bswapRegD &= 0xfe;
									
									maskedX = (graph_raster_x & bswapRegD);
									// mask in horizontal
									bswapRegC |= maskedX;
									bswapRegC = bswapRegC + maskedY + 4;
									if (bswapRegC <= curcycle)
									{
										// fix
										bswapRegC = curcycle + 4;
									}
									copper_next = bswapRegC;
									busScanEventsLevel2();
								} 
							}
						}
					}
				}
				else
				{
					// skip instruction

					// new skip, copied from cwait just to try something
					// cskip
				
					// calculate our line, masked with vmask
					// bl - masked graph_raster_y 

					// use mask on graph_raster_y
					bswapRegD |= 0x8000;
					// used to indicate correct line or not
					correctLine = FALSE;
					maskedY = graph_raster_y;
					*((UBY*) &maskedY) &= ((bswapRegD >> 8) & 0xff);
					if ((maskedY & 0xff) > ((bswapRegC >> 8) & 0xff))
					{
						// do skip
						// we have passed the line, set up next instruction immediately
						copper_ptr += 4;
						copper_next = curcycle + 4;
						busScanEventsLevel2();
					}
					else if (maskedY < ((bswapRegC >> 8) & 0xff))
					{
						// above line, don't skip
						copper_next = curcycle + 4;
						busScanEventsLevel2();
					}
					else
					{
						// here we are on the correct line

						// calculate our xposition, masked with hmask
						// al - masked graph_raster_x
						correctLine = TRUE;		
						
						// use mask on graph_raster_x
						// Compare masked x with wait x
						maskedX = graph_raster_x;
						*((UBY*) &maskedX) &= (bswapRegD & 0xff);
						if ((maskedX & 0xff) >= (bswapRegC & 0xff))
						{
							// position reached, set up next instruction immediately
							copper_ptr += 4;
						}
						copper_next = curcycle + 4;
						busScanEventsLevel2();
					}
				}
			}
		}
	}
	else
	{
		// not okay
		copper_next = -1;
		busScanEventsLevel2();
	}
}


