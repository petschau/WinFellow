/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Convert Amiga graphics data to temporary format suitable for fast          */
/* drawing on a chunky display.                                               */
/*                                                                            */
/* Authors: Petter Schau                                                      */
/*          Worfje                                                            */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

#include "portable.h"
#include "renaming.h"
#include "defs.h"
#include "fellow.h"
#include "draw.h"
#include "cpu.h"
#include "fmem.h"
#include "graphm.h"            /* Macros used */
#include "kbd.h"
#include "copper.h"
#include "blit.h"
#include "graph.h"


/*===========================================================================*/
/* Lookup-tables for planar to chunky routines                               */
/*===========================================================================*/

planar2chunkyroutine graph_decode_line_ptr;
planar2chunkyroutine graph_decode_line_tab[16];
planar2chunkyroutine graph_decode_line_dual_tab[16];

/*===========================================================================*/
/* Planar to chunky lookuptables for data                                    */
/*===========================================================================*/

ULO graph_deco1[256][2];
ULO graph_deco2[256][2];
ULO graph_deco3[256][2];
ULO graph_deco4[256][2];
ULO graph_deco5[256][2];
ULO graph_deco6[256][2];

ULO graph_deco320hi1[256];
ULO graph_deco320hi2[256];
ULO graph_deco320hi3[256];
ULO graph_deco320hi4[256];

ULO graph_decode_tmp;

ULO serper_debug_register_ecx;
ULO serper_debug_register_edx;

UBY graph_line1_tmp[1024];
UBY graph_line2_tmp[1024];


/*===========================================================================*/
/* Line description registers and data                                       */
/*===========================================================================*/

ULO graph_DDF_start;
ULO graph_DDF_word_count;
ULO graph_DIW_first_visible;
ULO graph_DIW_last_visible;
ULO graph_allow_bpl_line_skip;


/*===========================================================================*/
/* Translation tables for colors                                             */
/*===========================================================================*/

ULO graph_color_shadow[64];  /* Colors corresponding to the different Amiga- */
                             /* registers. Initialized from draw_color_table */
		             /* whenever WCOLORXX is written.                */

UWO graph_color[64];         /* Copy of Amiga version of colors */
BOOLE graph_playfield_on;


/*===========================================================================*/
/* IO-registers                                                              */
/*===========================================================================*/

ULO bpl1pt, bpl2pt, bpl3pt, bpl4pt, bpl5pt, bpl6pt;
ULO lof, ddfstrt, ddfstop, bplcon0, bplcon1, bplcon2, bpl1mod, bpl2mod;
ULO evenscroll, evenhiscroll, oddscroll, oddhiscroll;
ULO diwstrt, diwstop; 
ULO diwxleft, diwxright, diwytop, diwybottom;
ULO graph_raster_x, graph_raster_y;
ULO dmaconr, dmacon;


/*===========================================================================*/
/* Framebuffer data about each line, max triple buffering                    */
/*===========================================================================*/

graph_line graph_frame[3][314];


/*===========================================================================*/
/* Initializes the Planar 2 Chunky translation tables                        */
/*===========================================================================*/

void graphP2CTablesInit(void) {
  ULO i, j, d[2];
 
  for (i = 0; i < 256; i++) {
    d[0] = d[1] = 0;
    for (j = 0; j < 4; j++) {
      d[0] |= ((i & (0x80>>j))>>(4 + 3 - j))<<(j*8);
      d[1] |= ((i & (0x8>>j))>>(3 - j))<<(j*8);
    }
    for (j = 0; j < 2; j++) {
      graph_deco1[i][j] = d[j]<<2;
      graph_deco2[i][j] = d[j]<<3;
      graph_deco3[i][j] = d[j]<<4;
      graph_deco4[i][j] = d[j]<<5;
      graph_deco5[i][j] = d[j]<<6;
      graph_deco6[i][j] = d[j]<<7;
    }
    graph_deco320hi1[i] = ((d[0] & 0xff) |
                           ((d[0] & 0xff0000)>>8) |
		           ((d[1] & 0xff)<<16) |
			   ((d[1] & 0xff0000)<<8))<<2;
    graph_deco320hi2[i] = graph_deco320hi1[i]<<1;
    graph_deco320hi3[i] = graph_deco320hi1[i]<<2;
    graph_deco320hi4[i] = graph_deco320hi1[i]<<3;
  }
  graph_decode_tmp = 0;
}


/*===========================================================================*/
/* Decode tables for lores size displays                                     */
/* Called from the draw module                                               */
/*===========================================================================*/

void graphP2C1XInit(void) {
  graph_decode_line_tab[0] = graphDecode0_C;
  graph_decode_line_tab[1] = graphDecode1_C;
  graph_decode_line_tab[2] = graphDecode2_C;
  graph_decode_line_tab[3] = graphDecode3_C;
  graph_decode_line_tab[4] = graphDecode4_C;
  graph_decode_line_tab[5] = graphDecode5_C;
  graph_decode_line_tab[6] = graphDecode6_C;
  graph_decode_line_tab[7] = graphDecode0_C;
  graph_decode_line_tab[8] = graphDecode0_C;
  graph_decode_line_tab[9] = graphDecode1Hi320;
  graph_decode_line_tab[10] = graphDecode2Hi320;
  graph_decode_line_tab[11] = graphDecode3Hi320;
  graph_decode_line_tab[12] = graphDecode4Hi320;
  graph_decode_line_tab[13] = graphDecode0_C;
  graph_decode_line_tab[14] = graphDecode0_C;
  graph_decode_line_tab[15] = graphDecode0_C;
  graph_decode_line_dual_tab[0] = graphDecode0_C;
  graph_decode_line_dual_tab[1] = graphDecode1_C;
  graph_decode_line_dual_tab[2] = graphDecode2Dual_C;
  graph_decode_line_dual_tab[3] = graphDecode3Dual_C;
  graph_decode_line_dual_tab[4] = graphDecode4Dual_C;
  graph_decode_line_dual_tab[5] = graphDecode5Dual_C;
  graph_decode_line_dual_tab[6] = graphDecode6Dual_C;
  graph_decode_line_dual_tab[7] = graphDecode0_C;
  graph_decode_line_dual_tab[8] = graphDecode0_C;
  graph_decode_line_dual_tab[9] = graphDecode1Hi320;
  graph_decode_line_dual_tab[10] = graphDecode2HiDual320;
  graph_decode_line_dual_tab[11] = graphDecode3HiDual320;
  graph_decode_line_dual_tab[12] = graphDecode4HiDual320;
  graph_decode_line_dual_tab[13] = graphDecode0_C;
  graph_decode_line_dual_tab[14] = graphDecode0_C;
  graph_decode_line_dual_tab[15] = graphDecode0_C;
  graph_decode_line_ptr = graphDecode0_C;
}


/*===========================================================================*/
/* Decode tables for hires size tables                                       */
/* Called from the draw module                                               */
/*===========================================================================*/

void graphP2C2XInit(void) {
  graph_decode_line_tab[0] = graphDecode0_C;
  graph_decode_line_tab[1] = graphDecode1_C;
  graph_decode_line_tab[2] = graphDecode2_C;
  graph_decode_line_tab[3] = graphDecode3_C;
  graph_decode_line_tab[4] = graphDecode4_C;
  graph_decode_line_tab[5] = graphDecode5_C;
  graph_decode_line_tab[6] = graphDecode6_C;
  graph_decode_line_tab[7] = graphDecode0_C;
  graph_decode_line_tab[8] = graphDecode0_C;
  graph_decode_line_tab[9] = graphDecode1_C;
  graph_decode_line_tab[10] = graphDecode2_C;
  graph_decode_line_tab[11] = graphDecode3_C;
  graph_decode_line_tab[12] = graphDecode4_C;
  graph_decode_line_tab[13] = graphDecode0_C;
  graph_decode_line_tab[14] = graphDecode0_C;
  graph_decode_line_tab[15] = graphDecode0_C;
  graph_decode_line_dual_tab[0] = graphDecode0_C;
  graph_decode_line_dual_tab[1] = graphDecode1_C;
  graph_decode_line_dual_tab[2] = graphDecode2Dual_C;
  graph_decode_line_dual_tab[3] = graphDecode3Dual_C;
  graph_decode_line_dual_tab[4] = graphDecode4Dual_C;
  graph_decode_line_dual_tab[5] = graphDecode5Dual_C;
  graph_decode_line_dual_tab[6] = graphDecode6Dual_C;
  graph_decode_line_dual_tab[7] = graphDecode0_C;
  graph_decode_line_dual_tab[8] = graphDecode0_C;
  graph_decode_line_dual_tab[9] = graphDecode1_C;
  graph_decode_line_dual_tab[10] = graphDecode2Dual_C;
  graph_decode_line_dual_tab[11] = graphDecode3Dual_C;
  graph_decode_line_dual_tab[12] = graphDecode4Dual_C;
  graph_decode_line_dual_tab[13] = graphDecode0_C;
  graph_decode_line_dual_tab[14] = graphDecode0_C;
  graph_decode_line_dual_tab[15] = graphDecode0_C;
  graph_decode_line_ptr = graphDecode0_C;
}


/*===========================================================================*/
/* Clear the line descriptions                                               */
/* This function is called on emulation start and from the debugger when     */
/* the emulator is stopped mid-line. It must create a valid dummy array.     */
/*===========================================================================*/

void graphLineDescClear(void) {
  int frame, line;

  memset(graph_frame, 0, sizeof(graph_line)*3*314);
  for (frame = 0; frame < 3; frame++)
    for (line = 0; line < 314; line++) {
      graph_frame[frame][line].linetype = GRAPH_LINE_BG;
      graph_frame[frame][line].draw_line_routine =
	                                        (void *) draw_line_BG_routine;
      graph_frame[frame][line].colors[0] = 0;
      graph_frame[frame][line].frames_left_until_BG_skip = 1; /* Ensures we draw once */
      graph_frame[frame][line].sprite_ham_slot = 0xffffffff;
    }
}


/*===========================================================================*/
/* Clear the line descriptions partially                                     */
/* Removes all the SKIP tags.                                                */
/*===========================================================================*/

void graphLineDescClearSkips(void) {
  int frame, line;

  for (frame = 0; frame < 3; frame++)
    for (line = 0; line < 314; line++) 
      if (graph_frame[frame][line].linetype == GRAPH_LINE_SKIP) {
        graph_frame[frame][line].linetype = GRAPH_LINE_BG;
        graph_frame[frame][line].frames_left_until_BG_skip = 1;
      }
}


/*===========================================================================*/
/* The mapping from Amiga colors to host colors are dependent on the host    */
/* resolution. We need to recalculate the colors to reflect the possibly new */
/* color format.                                                             */
/* This function is called from the drawEmulationStart() functions since we  */
/* don't know the color format yet in graphEmulationStart()                  */
/*===========================================================================*/

void graphInitializeShadowColors(void) {
  ULO i;
  
  for (i = 0; i < 64; i++)
    graph_color_shadow[i] = draw_color_table[graph_color[i] & 0xfff];
}


/*===========================================================================*/
/* Clear all register data                                                   */
/*===========================================================================*/

static void graphIORegistersClear(void) {
  ULO i;
  
  for (i = 0; i < 64; i++) graph_color_shadow[i] = graph_color[i] = 0;
  graph_playfield_on = FALSE;
  lof = 0x8000;       /* Long frame */
  bpl1mod = 0;
  bpl2mod = 0;
  bpl1pt = 0;
  bpl2pt = 0;
  bpl3pt = 0;
  bpl4pt = 0;
  bpl5pt = 0;
  bpl6pt = 0;
  bplcon0 = 0;
  bplcon1 = 0;
  bplcon2 = 0;
  ddfstrt = 0;
  ddfstop = 0;
  graph_DDF_start = 0;
  graph_DDF_word_count = 0;
  diwstrt = 0;
  diwstop = 0;
  diwxleft = 0;
  diwxright = 0; 
  diwytop = 0;   
  diwybottom = 0;
  graph_DIW_first_visible = 256;
  graph_DIW_last_visible = 256;
  graph_raster_x = 0;
  graph_raster_y = 0;
  evenscroll = 0;
  evenhiscroll = 0;
  oddscroll = 0;
  oddhiscroll = 0;
  dmaconr = 0;
  dmacon = 0;
}


/*===========================================================================*/
/* Property set/get for smart drawing                                       */
/*===========================================================================*/

void graphSetAllowBplLineSkip(BOOLE allow_bpl_line_skip) {
  graph_allow_bpl_line_skip = allow_bpl_line_skip;
}

BOOLE graphGetAllowBplLineSkip(void) {
  return graph_allow_bpl_line_skip;
}

/*===========================================================================*/
/* Registers the graphics IO register handlers                               */
/*===========================================================================*/

void graphIOHandlersInstall(void) {
  ULO i;

  memorySetIOReadStub(0x002, rdmaconr);
  memorySetIOReadStub(0x004, rvposr);
  memorySetIOReadStub(0x006, rvhposr);
  memorySetIOReadStub(0x07c, rid);
  memorySetIOWriteStub(0x02a, wvpos);
  memorySetIOWriteStub(0x08e, wdiwstrt);
  memorySetIOWriteStub(0x090, wdiwstop);
  memorySetIOWriteStub(0x092, wddfstrt);
  memorySetIOWriteStub(0x094, wddfstop);
  memorySetIOWriteStub(0x096, wdmacon);
  memorySetIOWriteStub(0x0e0, wbpl1pth);
  memorySetIOWriteStub(0x0e2, wbpl1ptl);
  memorySetIOWriteStub(0x0e4, wbpl2pth);
  memorySetIOWriteStub(0x0e6, wbpl2ptl);
  memorySetIOWriteStub(0x0e8, wbpl3pth);
  memorySetIOWriteStub(0x0ea, wbpl3ptl);
  memorySetIOWriteStub(0x0ec, wbpl4pth);
  memorySetIOWriteStub(0x0ee, wbpl4ptl);
  memorySetIOWriteStub(0x0f0, wbpl5pth);
  memorySetIOWriteStub(0x0f2, wbpl5ptl);
  memorySetIOWriteStub(0x0f4, wbpl6pth);
  memorySetIOWriteStub(0x0f6, wbpl6ptl);
  memorySetIOWriteStub(0x100, wbplcon0);
  memorySetIOWriteStub(0x102, wbplcon1);
  memorySetIOWriteStub(0x104, wbplcon2);
  memorySetIOWriteStub(0x108, wbpl1mod);
  memorySetIOWriteStub(0x10a, wbpl2mod);
  for (i = 0x180; i < 0x1c0; i += 2) memorySetIOWriteStub(i, wcolor);
}


/*===========================================================================*/
/* Called on emulation end of frame                                          */
/*===========================================================================*/

void graphEndOfFrame(void) {
  graph_playfield_on = FALSE;
}

/*===========================================================================*/
/* Called on emulation hard reset                                            */
/*===========================================================================*/

void graphHardReset(void) {
  graphIORegistersClear();
  graphLineDescClear();
}


/*===========================================================================*/
/* Called on emulation start                                                 */
/*===========================================================================*/

void graphEmulationStart(void) {
  graphLineDescClear();
  graphIOHandlersInstall();
}


/*===========================================================================*/
/* Called on emulation stop                                                  */
/*===========================================================================*/

void graphEmulationStop(void) {
}


/*===========================================================================*/
/* Initializes the graphics module                                           */
/*===========================================================================*/

void graphStartup(void) {
  graphP2CTablesInit();
  graphLineDescClear();
  graphIORegistersClear();
  graphSetAllowBplLineSkip(TRUE);
}


/*===========================================================================*/
/* Release resources taken by the graphics module                            */
/*===========================================================================*/

void graphShutdown(void) {
}

/*===========================================================================*/
/* Planar to chunky conversion, 1 to 4 (or 6) bitplanes hires (or lores)     */
/*===========================================================================*/

// Decode the odd part of the first 4 pixels
static __inline ULO graphDecodeOdd1(int bitplanes, ULO dat1, ULO dat3, ULO dat5)
{
  switch (bitplanes)
  {
    case 1:
    case 2: return graph_deco1[dat1][0]; 
    case 3:
    case 4: return graph_deco1[dat1][0] | graph_deco3[dat3][0]; 
	case 5:
    case 6: return graph_deco1[dat1][0] | graph_deco3[dat3][0] | graph_deco5[dat5][0]; 
  }
  return 0;
}

// Decode the odd part of the last 4 pixels
static __inline ULO graphDecodeOdd2(int bitplanes, ULO dat1, ULO dat3, ULO dat5)
{
  switch (bitplanes)
  {
    case 1:
    case 2: return graph_deco1[dat1][1]; 
    case 3:
    case 4: return graph_deco1[dat1][1] | graph_deco3[dat3][1]; 
	case 5:
    case 6: return graph_deco1[dat1][1] | graph_deco3[dat3][1] | graph_deco5[dat5][1]; 
  }
  return 0;
}

// Decode the even part of the first 4 pixels
static __inline ULO graphDecodeEven1(int bitplanes, ULO dat2, ULO dat4, ULO dat6)
{
  switch (bitplanes)
  {
    case 1:
    case 2:
    case 3: return graph_deco2[dat2][0]; 
    case 4: 
    case 5: return graph_deco2[dat2][0] | graph_deco4[dat4][0];  
    case 6: return graph_deco2[dat2][0] | graph_deco4[dat4][0] | graph_deco6[dat6][0]; 

  }
  return 0;
}

// Decode the even part of the last 4 pixels
static __inline ULO graphDecodeEven2(int bitplanes, ULO dat2, ULO dat4, ULO dat6)
{
  switch (bitplanes)
  {
    case 1:
    case 2:
    case 3: return graph_deco2[dat2][1]; 
    case 4:  
	case 5: return graph_deco2[dat2][1] | graph_deco4[dat4][1]; 
    case 6: return graph_deco2[dat2][1] | graph_deco4[dat4][1] | graph_deco6[dat6][1]; 
  }
  return 0;
}

// Decode the even part of the first 4 pixels
static __inline ULO graphDecodeDual1(int bitplanes, ULO datA, ULO datB, ULO datC)
{
  switch (bitplanes)
  {
    case 1:
    case 2:
    case 3: return graph_deco1[datA][0]; 
    case 4: 
    case 5: return graph_deco1[datA][0] | graph_deco2[datB][0];  
    case 6: return graph_deco1[datA][0] | graph_deco2[datB][0] | graph_deco3[datC][0]; 

  }
  return 0;
}

// Decode the even part of the last 4 pixels
static __inline ULO graphDecodeDual2(int bitplanes, ULO datA, ULO datB, ULO datC)
{
  switch (bitplanes)
  {
    case 1:
    case 2:
    case 3: return graph_deco1[datA][1]; 
    case 4:  
	case 5: return graph_deco1[datA][1] | graph_deco2[datB][1]; 
    case 6: return graph_deco1[datA][1] | graph_deco2[datB][1] | graph_deco3[datC][1]; 
  }
  return 0;
}

// Add modulo to the bitplane ptrs
static __inline void graphDecodeModulo(int bitplanes, ULO bpl_length_in_bytes)
{
  switch (bitplanes)
  {
    case 6: bpl6pt = (bpl6pt + bpl_length_in_bytes + bpl2mod) & 0x1FFFFF;
    case 5: bpl5pt = (bpl5pt + bpl_length_in_bytes + bpl1mod) & 0x1FFFFF;
    case 4: bpl4pt = (bpl4pt + bpl_length_in_bytes + bpl2mod) & 0x1FFFFF;
    case 3: bpl3pt = (bpl3pt + bpl_length_in_bytes + bpl1mod) & 0x1FFFFF;
    case 2: bpl2pt = (bpl2pt + bpl_length_in_bytes + bpl2mod) & 0x1FFFFF;
    case 1: bpl1pt = (bpl1pt + bpl_length_in_bytes + bpl1mod) & 0x1FFFFF;
  }
}

static __inline void graphDecodeGeneric(int bitplanes)
{
  ULO bpl_length_in_bytes = graph_DDF_word_count * 2;

  if (bitplanes == 0) return;
  if (bpl_length_in_bytes != 0) 
  {
    ULO *dest_odd;
    ULO *dest_even;
    ULO *dest_tmp;
    ULO *end_odd;
    ULO *end_even;
    UBY *pt1_tmp, *pt2_tmp, *pt3_tmp, *pt4_tmp, *pt5_tmp, *pt6_tmp;
    ULO dat1, dat2, dat3, dat4, dat5, dat6; 
	int maxscroll;
	ULO temp = 0;

    dat1 = dat2 = dat3= dat4= dat5 = dat6 = 0;
    
	  if ((bplcon0 & 0x8000) == 0x8000) // check if hires bit is set (bit 15 of register BPLCON0)
	  {
		  // high resolution
		  dest_odd = (ULO*) (graph_line1_tmp + graph_DDF_start + oddhiscroll);		

		  // Find out how many pixels the bitplane is scrolled
		  // the first pixels must then be zeroed to avoid garbage.
		  maxscroll = (evenhiscroll > oddhiscroll) ? evenhiscroll : oddhiscroll; 
	  } 
	  else 
	  {
		  dest_odd = (ULO*) (graph_line1_tmp + graph_DDF_start + oddscroll);			

		  // Find out how many pixels the bitplane is scrolled
		  // the first pixels must then be zeroed to avoid garbage.
		  maxscroll = (evenscroll > oddscroll) ? evenscroll : oddscroll; 
	  }

	while (maxscroll > 0)
	{
	  graph_line1_tmp[graph_DDF_start + temp] = 0;
	  graph_line1_tmp[graph_DDF_start + (graph_DDF_word_count << 4) + temp] = 0;
	  maxscroll -= 4;
	  temp += 4;
	}
    end_odd = dest_odd + bpl_length_in_bytes * 2; 
    
	if (bitplanes > 1)
    {
		if ((bplcon0 & 0x8000) == 0x8000) // check if hires bit is set (bit 15 of register BPLCON0)
		{
			// high resolution
			dest_even = (ULO*) (graph_line1_tmp + graph_DDF_start + evenhiscroll);
		}
		else
		{
			// low resolution
			dest_even = (ULO*) (graph_line1_tmp + graph_DDF_start + evenscroll);
		}
		end_even = dest_even + bpl_length_in_bytes * 2; 
    }

    switch (bitplanes)
    {
      case 6: pt6_tmp = memory_chip + bpl6pt;
      case 5: pt5_tmp = memory_chip + bpl5pt;
      case 4: pt4_tmp = memory_chip + bpl4pt;
      case 3: pt3_tmp = memory_chip + bpl3pt;
      case 2: pt2_tmp = memory_chip + bpl2pt;
      case 1: pt1_tmp = memory_chip + bpl1pt;
    }

    for (dest_tmp = dest_odd; dest_tmp != end_odd; dest_tmp += 2) 
    {
      if (bitplanes >= 1) dat1 = *pt1_tmp++;
      if (bitplanes >= 3) dat3 = *pt3_tmp++;
	  if (bitplanes >= 5) dat5 = *pt5_tmp++;
      dest_tmp[0] = graphDecodeOdd1(bitplanes, dat1, dat3, dat5);
      dest_tmp[1] = graphDecodeOdd2(bitplanes, dat1, dat3, dat5);
    }

    if (bitplanes >= 2) 
    {
      for (dest_tmp = dest_even; dest_tmp != end_even; dest_tmp += 2)
      {
        if (bitplanes >= 2) dat2 = *pt2_tmp++;
        if (bitplanes >= 4) dat4 = *pt4_tmp++;
		if (bitplanes >= 6) dat6 = *pt6_tmp++;
			dest_tmp[0] |= graphDecodeEven1(bitplanes, dat2, dat4, dat6);
			dest_tmp[1] |= graphDecodeEven2(bitplanes, dat2, dat4, dat6);
      }
    }
  }
  graphDecodeModulo(bitplanes, bpl_length_in_bytes);
}

static __inline void graphDecodeDualGeneric(int bitplanes)
{
  ULO bpl_length_in_bytes = graph_DDF_word_count * 2;
  if (bitplanes == 0) return;
  if (bpl_length_in_bytes != 0) 
  {
    ULO *dest_odd;
    ULO *dest_even;
    ULO *dest_tmp;
    ULO *end_odd;
    ULO *end_even;
    UBY *pt1_tmp, *pt2_tmp, *pt3_tmp, *pt4_tmp, *pt5_tmp, *pt6_tmp;
    ULO dat1, dat2, dat3, dat4, dat5, dat6; 
  
	  int maxscroll;
	  ULO temp;

    dat1 = dat2 = dat3= dat4= dat5 = dat6 = 0;

	// clear edges
    maxscroll = (evenscroll > oddscroll) ? evenscroll : oddscroll; 

	  temp = 0;
	  while (maxscroll > 0) {
		  graph_line1_tmp[graph_DDF_start + temp] = 0;
		  graph_line2_tmp[graph_DDF_start + temp] = 0;
		  graph_line1_tmp[graph_DDF_start + (graph_DDF_word_count << 4) + temp] = 0;
		  graph_line2_tmp[graph_DDF_start + (graph_DDF_word_count << 4) + temp] = 0;
		  maxscroll -= 4;
		  temp += 4;
	  }
	  
	  // setup loop
	  dest_odd = (ULO*) (graph_line1_tmp + graph_DDF_start + oddscroll);			
    end_odd = dest_odd + bpl_length_in_bytes * 2; 
    
	if (bitplanes > 1)
    {
		// low resolution
		dest_even = (ULO*) (graph_line2_tmp + graph_DDF_start + evenscroll);
		end_even = dest_even + bpl_length_in_bytes * 2; 
    }

    switch (bitplanes)
    {
      case 6: pt6_tmp = memory_chip + bpl6pt;
      case 5: pt5_tmp = memory_chip + bpl5pt;
      case 4: pt4_tmp = memory_chip + bpl4pt;
      case 3: pt3_tmp = memory_chip + bpl3pt;
      case 2: pt2_tmp = memory_chip + bpl2pt;
      case 1: pt1_tmp = memory_chip + bpl1pt;
    }

    for (dest_tmp = dest_odd; dest_tmp != end_odd; dest_tmp += 2) 
    {
      if (bitplanes >= 1) dat1 = *pt1_tmp++;
      if (bitplanes >= 3) dat3 = *pt3_tmp++;
	    if (bitplanes >= 5) dat5 = *pt5_tmp++;
      dest_tmp[0] = graphDecodeDual1(bitplanes, dat1, dat3, dat5);
      dest_tmp[1] = graphDecodeDual2(bitplanes, dat1, dat3, dat5);
    }

    if (bitplanes >= 2) 
    {
      for (dest_tmp = dest_even; dest_tmp != end_even; dest_tmp += 2)
      {
        if (bitplanes >= 2) dat2 = *pt2_tmp++;
        if (bitplanes >= 4) dat4 = *pt4_tmp++;
		    if (bitplanes >= 6) dat6 = *pt6_tmp++;
			  dest_tmp[0] = graphDecodeDual1(bitplanes, dat2, dat4, dat6);
			  dest_tmp[1] = graphDecodeDual2(bitplanes, dat2, dat4, dat6);
      }
    }
  }
  graphDecodeModulo(bitplanes, bpl_length_in_bytes);
}

/*===========================================================================*/
/* Planar to chunky conversion, 1 bitplane hires or lores                    */
/*===========================================================================*/

void graphDecode0_C(void)
{
  graphDecodeGeneric(0);
}

/*===========================================================================*/
/* Planar to chunky conversion, 1 bitplane hires or lores                    */
/*===========================================================================*/

void graphDecode1_C(void)
{
  graphDecodeGeneric(1);
}

/*===========================================================================*/
/* Planar to chunky conversion, 2 bitplanes hires or lores                   */
/*===========================================================================*/

void graphDecode2_C(void)
{
  graphDecodeGeneric(2);
}

/*===========================================================================*/
/* Planar to chunky conversion, 3 bitplanes hires or lores                   */
/*===========================================================================*/

void graphDecode3_C(void)
{
  graphDecodeGeneric(3);
}

/*===========================================================================*/
/* Planar to chunky conversion, 4 bitplanes hires or lores                   */
/*===========================================================================*/

void graphDecode4_C(void)
{
  graphDecodeGeneric(4);
}

/*===========================================================================*/
/* Planar to chunky conversion, 5 bitplanes hires or lores                   */
/*===========================================================================*/

void graphDecode5_C(void)
{
  graphDecodeGeneric(5);
}

/*===========================================================================*/
/* Planar to chunky conversion, 6 bitplanes hires or lores                   */
/*===========================================================================*/

void graphDecode6_C(void)
{
  graphDecodeGeneric(6);
}

/*===========================================================================*/
/* Planar to chunky conversion, 2 bitplanes lores, dual playfield            */
/*===========================================================================*/
void graphDecode2Dual_C(void)
{
	graphDecodeDualGeneric(2);
}

/*===========================================================================*/
/* Planar to chunky conversion, 3 bitplanes lores, dual playfield            */
/*===========================================================================*/
void graphDecode3Dual_C(void)
{
	graphDecodeDualGeneric(3);
}

/*===========================================================================*/
/* Planar to chunky conversion, 3 bitplanes lores, dual playfield            */
/*===========================================================================*/
void graphDecode4Dual_C(void)
{
	graphDecodeDualGeneric(4);
}

/*===========================================================================*/
/* Planar to chunky conversion, 3 bitplanes lores, dual playfield            */
/*===========================================================================*/
void graphDecode5Dual_C(void)
{
	graphDecodeDualGeneric(5);
}

/*===========================================================================*/
/* Planar to chunky conversion, 3 bitplanes lores, dual playfield            */
/*===========================================================================*/
void graphDecode6Dual_C(void)
{
	graphDecodeDualGeneric(6);
}

/*===========================================================================*/
/* Calculate all variables connected to the screen emulation                 */
/* First calculate the ddf variables graph_DDF_start and graph_DDF_word_count*/
/*   Check if stop is bigger than start                                      */
/*   Check if strt and stop is mod 8 aligned                                 */
/*   If NOK then set numberofwords and startpos to 0 and                     */
/*   DIWvisible vars to 128                                                  */
/*                                                                           */
/* NOTE: The following is (was? worfje) (sadly) a prime example of spaghetti code.... (PS) */
/*       Though it somehow works, so just leave it and hope it does not break*/
/*===========================================================================*/


void graphCalculateWindow_C(void) 
{
	ULO ddfstop_aligned, ddfstrt_aligned, last_position_in_line;

	if ((bplcon0 & 0x8000) == 0x8000) // check if Hires bit is set (bit 15 of BPLCON0)
	{
		graphCalculateWindowHires_C();
	} 
	else 
	{
		// set DDF (Display Data Fetch Start) variables
		if (ddfstop < ddfstrt) 
		{
			graph_DDF_start = graph_DDF_word_count = 0;
			graph_DIW_first_visible = graph_DIW_last_visible = 256;
		}
		
		ddfstop_aligned = ddfstop & 0x07;
		ddfstrt_aligned = ddfstrt & 0x07;

		graph_DDF_word_count = ((ddfstop - ddfstrt) >> 3) + 1;
		if (ddfstop_aligned != ddfstrt_aligned) 
		{
			graph_DDF_word_count++;
		} 
		
		graph_DDF_start = ((ddfstrt << 1) + 0x11);	// 0x11 = 17 pixels = 8.5 bus cycles

		// set DIW (Display Window Start) variables (only used with drawing routines)
		
		if (diwxleft >= diwxright) 
		{
			graph_DDF_start = graph_DDF_word_count = 0;
			graph_DIW_first_visible = graph_DIW_last_visible = 256;
		}

		graph_DIW_first_visible = draw_left;
		if (diwxleft < graph_DDF_start) // cmp dword[diwxleft], dword[graph_DDF_start]
		{  
			// jb  .cwdiwless
			if (graph_DDF_start > draw_left) // cmp dword[graph_DDF_start], dword[draw_left]
			{  
				// ja  .cwdiwnoclip2
				graph_DIW_first_visible = graph_DDF_start;
			} 
		} 
		else 
		{
			if (diwxleft > draw_left) // cmp word[diwxleft], word[draw_left]
			{  
				// ja  .cwdiwnoclip
				graph_DIW_first_visible = diwxleft;
			} 
		}

		// .cwdiwlastpos:
		last_position_in_line = (graph_DDF_word_count << 4) + graph_DDF_start;
		if (oddscroll > evenscroll) // cmp word[oddscroll], word[evenscroll]
		{
			// ja  .cwaddodd
			last_position_in_line += oddscroll;
		} 
		else 
		{
			last_position_in_line += evenscroll;
		}

		// .cwdiwlastpos2:
		graph_DIW_last_visible = draw_right;
		if (last_position_in_line < diwxright) // cmp ecx, word[diwxright]
		{
			// jb  .cwdiwxx
			if (last_position_in_line < draw_right) // cmp ecx, word[draw_right]
			{
				// jb  .cwdiwnoclip4
				graph_DIW_last_visible = last_position_in_line;
			}
		} 
		else 
		{
			if (diwxright < draw_right) // cmp word[diwxright], word[draw_right]
			{
				// jb  .cwdiwnoclip3
				graph_DIW_last_visible = diwxright;
			}
		}
	}
}

void graphCalculateWindowHires_C(void)
{
	ULO ddfstop_aligned, ddfstrt_aligned, last_position_in_line;

	if (ddfstrt > ddfstop) // cmp dword[ddfstrt], dword[ddfstop]
	{
		// ja near cwdiwerrh
		graph_DDF_start = graph_DDF_word_count = 0;
		graph_DIW_first_visible = graph_DIW_last_visible = 256;
	}

	ddfstop_aligned = ddfstop & 0x07;
	ddfstrt_aligned = ddfstrt & 0x07;

	graph_DDF_word_count = (((ddfstop - ddfstrt) + 15) >> 2) & 0x0FFFFFFFE;
	graph_DDF_start = (ddfstrt << 2) + 18;

	// DDF variables done, now check DIW variables

	if ((diwxleft << 1) >= (diwxright << 1)) 
	{
		graph_DDF_start = graph_DDF_word_count = 0;
		graph_DIW_first_visible = graph_DIW_last_visible = 256;
	}

	if ((diwxleft << 1) < graph_DDF_start) 
	{
		if (graph_DDF_start > (draw_left << 1)) 
		{
			graph_DIW_first_visible = graph_DDF_start;
		} 
		else
		{
			graph_DIW_first_visible = draw_left << 1;
		}
	}
	else
	{
		if ((diwxleft << 1) > (draw_left << 1))
		{
			graph_DIW_first_visible = diwxleft << 1;
		}
		else
		{
			graph_DIW_first_visible = draw_left << 1;
		}
	}
	
	last_position_in_line = graph_DDF_start + (graph_DDF_word_count << 4);
	if (oddhiscroll > evenhiscroll) // cmp word[oddhiscroll], word[evenhiscroll]
	{
		// ja  .cwaddoddh
		last_position_in_line += oddhiscroll;
	} 
	else 
	{
		last_position_in_line += evenhiscroll;
	}

	if (last_position_in_line < (diwxright << 1)) 
	{
		if (last_position_in_line < (draw_right << 1))
		{
			graph_DIW_last_visible = last_position_in_line;
		}
		else
		{
			graph_DIW_last_visible = draw_right << 1;
		}
	}
	else
	{
		if ((diwxright << 1) < (draw_right << 1)) 
		{
			graph_DIW_last_visible = diwxright << 1;
		}
		else
		{
			graph_DIW_last_visible = draw_right << 1;
		}
	}
}

void graphPlayfieldOnOff(void)
{
	if (graph_playfield_on != 0) 
	{
		// Playfield on, check if top has moved below graph_raster_y
		// Playfield on, check if playfield is turned off at this line
		if (!(graph_raster_y != diwybottom))
		{
			// %%nopfspecial:
			graph_playfield_on = 0;
		}
	} 
	else 
	{
		// Playfield off, Check if playfield is turned on at this line
		if (!(graph_raster_y != diwytop))
		{
			// Don't turn on when top > bottom
			if (diwytop < diwybottom)
			{
				graph_playfield_on = 1;
			}
		}
	}
	// OK, here the state of the playfield is taken care of
}

void graphDecodeNOP_C(void)
{
    switch ((bplcon0 >> 12) & 0x07) {
    case 0:
      break;
    case 6:
      bpl6pt += (graph_DDF_word_count << 1) + bpl2mod;
    case 5:
      bpl5pt += (graph_DDF_word_count << 1) + bpl1mod;
    case 4:
      bpl4pt += (graph_DDF_word_count << 1) + bpl2mod;
    case 3:
      bpl3pt += (graph_DDF_word_count << 1) + bpl1mod;
    case 2:
      bpl2pt += (graph_DDF_word_count << 1) + bpl2mod;
    case 1:
      bpl1pt += (graph_DDF_word_count << 1) + bpl1mod;
      break;
    }
}

/*===========================================================================*/
/* End of line handler for graphics                                          */
/*===========================================================================*/

void graphEndOfLine_C(void)
{
  
}


/*
void graphEndOfLine(void)
{

	graph_line* current_graph_line;

	// skip this frame?
	if (draw_frame_skip != 0) 
	{
		// update diw state
		graphPlayfieldOnOff();

		// skip lines before line $12
		if (graph_raster_y >= 0x12) {

			// make pointer to linedesc for this line
			current_graph_line = &graph_frame[draw_buffer_draw][graph_raster_y];
			// (graph_raster_y * graph_line_end) + (graph_line_end * 314 * draw_buffer_draw));

			// decode sprites if DMA is enabled and raster is after line $18
			if ((dmacon & 0x20) == 0x20)
			{
				if (graph_raster_y >= 0x18) 
				{
					spritesDecode();
				}
			}
			
			// sprites decoded, sprites_onlineflag is set if there are any
			
			// update pointer to drawing routine
			update_drawmode();

			// check if we are clipped

			if ((graph_raster_y >= draw_top) || (graph_raster_y < draw_bottom))
			{
				// .decode
				// visible line, either background or bitplanes
				if (graph_allow_bpl_line_skip == 0) 
				{
					_graphComposeLineOutput_
				}
				else
				{
					_graphComposeLineOutputSmart_
				}
				// add	esp, 4 ????????????????

			}
			else
			{
				// .nop_line
				// do nop decoding, no screen output needed
		
				// pop ebp ?????????????????
				if (draw_line_BG_routine != draw_line_routine)
				{
					// push dword .drawend
					graphDecodeNOP;
				}
			}

			// if diw state changing from background to bitplane output,
		    // set new drawing routine pointer

			// .draw_end
			if (draw_switch_bg_to_bpl != 0) {

				draw_line_BPL_manage_routine = draw_line_routine;
				draw_switch_bg_to_bpl = 0

			}

		}
	}
	

	// .skip_frame
	_graphSpriteHack_

		// no return??? or did something get pushed on the stack?
}
*/