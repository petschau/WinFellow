/* @(#) $Id: GRAPH.C,v 1.7 2009-07-25 03:09:00 peschau Exp $ */
/*=========================================================================*/
/* Fellow                                                                  */
/* Convert graphics data to temporary format suitable for fast             */
/* drawing on a chunky display.                                            */
/*                                                                         */
/* Authors: Petter Schau                                                   */
/*          Worfje                                                         */
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
#include "draw.h"
#include "fmem.h"
#include "kbd.h"
#include "copper.h"
#include "blit.h"
#include "graph.h"
#include "bus.h"
#include "sound.h"
#include "draw.h"
#include "graph.h"
#include "sprite.h"
#include "CpuIntegration.h"

/*======================================================================*/
/* flag that handles loss of surface content due to DirectX malfunction */
/*======================================================================*/

BOOLE graph_buffer_lost;

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
}


/*===========================================================================*/
/* Clear the line descriptions                                               */
/* This function is called on emulation start and from the debugger when     */
/* the emulator is stopped mid-line. It must create a valid dummy array.     */
/*===========================================================================*/

void graphLineDescClear(void)
{
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

void graphLineDescClearSkips(void)
{
  int frame, line;

  for (frame = 0; frame < 3; frame++)
    for (line = 0; line < 314; line++) 
      if (graph_frame[frame][line].linetype == GRAPH_LINE_SKIP) {
	graph_frame[frame][line].linetype = GRAPH_LINE_BG;
	graph_frame[frame][line].frames_left_until_BG_skip = 1;
      }
      else if (graph_frame[frame][line].linetype == GRAPH_LINE_BPL_SKIP) {
	graph_frame[frame][line].linetype = GRAPH_LINE_BPL;
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
/* Graphics Register Access routines                                         */
/*===========================================================================*/

/*===========================================================================*/
/* DMACONR - $dff002 Read                                                    */
/*                                                                           */
/* return dmaconr | ((!((WOR)bltzero))<<13);                                 */
/*===========================================================================*/

UWO rdmaconr(ULO address)
{
  if (blitterGetZeroFlag())
  {
    return (UWO) (dmaconr | 0x00002000);
  }
  return (UWO) dmaconr;
}

/*===========================================================================*/
/* VPOSR - $dff004 Read vpos and chipset ID bits                             */
/*                                                                           */
/* return lof | (graph_raster_y>>8);                                         */
/*===========================================================================*/

UWO rvposr(ULO address)
{
  if (blitterGetECS())
  {
    return (UWO) ((lof | (busGetRasterY() >> 8)) | 0x2000);
  }
  return (UWO) (lof | (busGetRasterY() >> 8));  
}

/*===========================================================================*/
/* VHPOSR - $dff006 Read                                                     */
/*                                                                           */
/* return (graph_raster_x>>1) | ((graph_raster_y & 0xff)<<8);                */
/*===========================================================================*/

UWO rvhposr(ULO address)
{
  return (UWO) ((busGetRasterX() >> 1) | ((busGetRasterY() & 0x000000FF) << 8));
}

/*===========================================================================*/
/* ID - $dff07c Read                                                         */
/*                                                                           */
/* return 0xffff                                                             */
/*===========================================================================*/

UWO rid(ULO address)
{
  return 0xFFFF;
}

/*===========================================================================*/
/* VPOS - $dff02a Write                                                      */
/*                                                                           */
/* lof = data & 0x8000;                                                      */
/*===========================================================================*/

void wvpos(UWO data, ULO address)
{
  lof = (ULO) (data & 0x8000);
}

/*===========================================================================*/
/* SERDAT - $dff030 Write                                                    */
/*                                                                           */
/*                                                                           */
/*===========================================================================*/

void wserdat(UWO data, ULO address)
{
}

/*===========================================================================*/
/* SERPER - $dff032 Write                                                    */
/*                                                                           */
/*                                                                           */
/*===========================================================================*/

void wserper(UWO data, ULO address)
{
}

/*===========================================================================*/
/* DIWSTRT - $dff08e Write                                                   */
/*                                                                           */
/*                                                                           */
/*===========================================================================*/

void wdiwstrt(UWO data, ULO address)
{
  diwstrt = data;
  diwytop = (data >> 8) & 0x000000FF;
  if (diwxright == 472)
  {
    diwxleft = 88;
  }
  else
  {
    diwxleft = data & 0x000000FF;
  }
  graphCalculateWindow();
  graphPlayfieldOnOff();
}

/*===========================================================================*/
/* DIWSTOP - $dff090 Write                                                   */
/*===========================================================================*/

void wdiwstop(UWO data, ULO address)
{
  diwstop = data;
  if ((((data >> 8) & 0xff) & 0x80) == 0x0)
  {
    diwybottom = ((data >> 8) & 0xff) + 256;  
  }
  else
  {
    diwybottom = (data >> 8) & 0xff;
  }

  if (((data & 0xff) ^ 0x100) < 457)
  {
    diwxleft = diwstrt & 0xff;
    diwxright = (data & 0xff) ^ 0x100;
  }
  else
  {
    diwxleft = 88;
    diwxright = 472;
  }
  graphCalculateWindow();
  graphPlayfieldOnOff();
}

/*==============================================================================*/
/* DDFSTRT - $dff092 Write                                                      */
/*                                                                              */
/* When this value is written, the scroll (BPLCON1) also need to be reevaluated */
/* for _reversal_ of the scroll values.                                         */
/* _wbplcon1_ calls _graphCalculateWindow_ so we don't need to here             */                                                           
/*==============================================================================*/

void wddfstrt(UWO data, ULO address)
{
  if ((data & 0xfc) < 0x18)
  {
    ddfstrt = 0x18;
  }
  else
  {
    ddfstrt = data & 0xfc;
  }
  sprite_ddf_kill = (data & 0xfc) - 0x14;
  wbplcon1((UWO)bplcon1, address);
}

/*==============================================================================*/
/* DDFSTOP - $dff094 Write                                                      */
/*                                                                              */
/* These registers control the horizontal timing of the                         */
/* beginning and end of the bitplane DMA display data fetch.                    */
/* (quote from 'Amiga Hardware Reference Manual')                               */
/*==============================================================================*/

void wddfstop(UWO data, ULO address)
{
  if ((data & 0xfc) > 0xd8)
  {
    ddfstop = 0xd8;
  }
  else
  {
    ddfstop = data & 0xfc;
  }
  graphCalculateWindow();
}

/*===========================================================================*/
/* DMACON - $dff096 Write                                                    */
/*                                                                           */
/* $dff096  - Read from $dff002                                              */
/* dmacon - zero if master dma bit is off                                    */
/* dmaconr - is always correct.                                              */
/*===========================================================================*/

void wdmacon(UWO data, ULO address)
{
  ULO local_data;
  ULO prev_dmacon;
  ULO i;

  // check SET/CLR bit is 1 or 0
  if ((data & 0x8000) != 0x0)
  {
    // SET/CLR bit is 1 (bits set to 1 will set bits)
    local_data = data & 0x7ff; // zero bits 15 to 11 (readonly bits and set/clr bit)
    // test if BLTPRI got turned on (blitter DMA priority)
    if ((local_data & 0x0400) != 0x0) 
    {
      // BLTPRI bit is on now
      if ((dmaconr & 0x0400) == 0x0)
      {
	// BLTPRI was turned off before and therefor
	// BLTPRI got turned on, stop CPU until a blit is 
	// finished if this is a blit that uses all cycles
	if (blitterEvent.cycle != BUS_CYCLE_DISABLE)
	{
	  if (blitterGetFreeCycles() == 0)
	  {
	    // below delays CPU additionally cycles
	    cpuIntegrationSetChipCycles(cpuIntegrationGetChipCycles() + (blitterEvent.cycle - bus.cycle));
	  }
	}
      }
    }

    dmaconr |= local_data;
    prev_dmacon = dmacon; // stored in edx
    if ((dmaconr & 0x0200) == 0x0)
    {
      dmacon = 0;
    }
    else
    {
      dmacon = dmaconr;
    }

    // enable audio channel X ?
    for (i = 0; i < 4; i++) 
    {
      if (((dmacon & (1 << i))) != 0x0)
      {
	// audio channel 0 DMA enable bit is set now
	if ((prev_dmacon & (1 << i)) == 0x0)
	{
	  // audio channel 0 DMA enable bit was clear previously
	  soundChannelEnable(i);
	}
      }
    }    

    // check if already a call to wbltsize was executed 
    // before the blitter DMA channel was activated
    if (blitterGetDMAPending())
    {
      if ((dmacon & 0x0040) != 0x0)
      {
	blitterCopy();
      }
    }

    // update Copper DMA
    copperUpdateDMA(); 
  }
  else
  {
    // SET/CLR bit is 0 (bits set to 1 will clear bits)
    // in Norwegian this translates to 'slett' bits
    dmaconr = (~(data & 0x07ff)) & dmaconr;
    prev_dmacon = dmacon;
    if ((dmaconr & 0x0200) == 0x0)
    {
      dmacon = 0;
    }
    else
    {
      dmacon = dmaconr;
    }

    // if a blitter DMA is turned off in the middle of the blit action
    // we finish the blit
    if ((dmacon & 0x0040) == 0x0)
    {
      if (blitterIsStarted())
      {
	blitForceFinish();
      }
    }

    // disable audio channel X ?
    for (i = 0; i < 4; i++) 
    {
      if ((dmacon & (1 << i)) == 0x0)
      {
	if ((prev_dmacon & (1 << i)) != 0x0)
	{
	  soundChannelKill(i);
	}
      }
    }

    // check if already a call to wbltsize was executed 
    // before the blitter DMA channel was activated
    if (blitterGetDMAPending())
    {
      if ((dmacon & 0x0040) != 0x0)
      {
	blitterCopy();
      }
    }

    // update Copper DMA
    copperUpdateDMA(); 
  }
}

/*===========================================================================*/
/* BPL1PTH - $dff0e0 Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl1pth(UWO data, ULO address)
{
  bpl1pt = (bpl1pt & 0x0000ffff) | ((ULO)(data & 0x01f)) << 16;
}

/*===========================================================================*/
/* BPL1PTL - $dff0e2 Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl1ptl(UWO data, ULO address)
{
  bpl1pt = (bpl1pt & 0xffff0000) | (ULO)(data & 0x0fffe);
}

/*===========================================================================*/
/* BPL2PTH - $dff0e4 Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl2pth(UWO data, ULO address)
{
  bpl2pt = (bpl2pt & 0x0000ffff) | ((UWO)(data & 0x01f)) << 16;
}

/*===========================================================================*/
/* BPL2PTL - $dff0e6 Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl2ptl(UWO data, ULO address)
{
  bpl2pt = (bpl2pt & 0xffff0000) | (ULO)(data & 0x0fffe);
}

/*===========================================================================*/
/* BPL3PTH - $dff0e8 Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl3pth(UWO data, ULO address)
{
  bpl3pt = (bpl3pt & 0x0000ffff) | ((ULO)(data & 0x01f)) << 16;
}

/*===========================================================================*/
/* BPL3PTL - $dff0eA Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl3ptl(UWO data, ULO address)
{
  bpl3pt = (bpl3pt & 0xffff0000) | (ULO)(data & 0x0fffe);
}

/*===========================================================================*/
/* BPL4PTH - $dff0e8 Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl4pth(UWO data, ULO address)
{
  bpl4pt = (bpl4pt & 0x0000ffff) | ((ULO)(data & 0x01f)) << 16;
}

/*===========================================================================*/
/* BPL4PTL - $dff0eA Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl4ptl(UWO data, ULO address)
{
  bpl4pt = (bpl4pt & 0xffff0000) | (ULO)(data & 0x0fffe);
}

/*===========================================================================*/
/* BPL5PTH - $dff0e8 Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl5pth(UWO data, ULO address)
{
  bpl5pt = (bpl5pt & 0x0000ffff) | ((ULO)(data & 0x01f)) << 16;
}

/*===========================================================================*/
/* BPL5PTL - $dff0eA Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl5ptl(UWO data, ULO address)
{
  bpl5pt = (bpl5pt & 0xffff0000) | (ULO)(data & 0x0fffe);
}

/*===========================================================================*/
/* BPL3PTH - $dff0e8 Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl6pth(UWO data, ULO address)
{
  bpl6pt = (bpl6pt & 0x0000ffff) | ((ULO)(data & 0x01f)) << 16;
}

/*===========================================================================*/
/* BPL6PTL - $dff0eA Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl6ptl(UWO data, ULO address)
{
  bpl6pt = (bpl6pt & 0xffff0000) | (ULO)(data & 0x0fffe);
}

/*===========================================================================*/
/* BPLCON0 - $dff100 Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbplcon0(UWO data, ULO address)
{
  ULO local_data;

  bplcon0 = data;
  local_data = (data >> 12) & 0x0f;

  // check if DBLPF bit is set 
  if ((bplcon0 & 0x0400) != 0)
  {
    // double playfield, select a decoding function 
    // depending on hires and playfield priority
    graph_decode_line_ptr = graph_decode_line_dual_tab[local_data];
  }
  else
  {
    graph_decode_line_ptr = graph_decode_line_tab[local_data];
  }

  // check if HOMOD bit is set 
  if ((bplcon0 & 0x0800) != 0)
  {
    // hold-and-modify mode
    draw_line_BPL_res_routine = draw_line_HAM_lores_routine;
  }
  else
  {
    // check if DBLPF bit is set
    if ((bplcon0 & 0x0400) != 0)
    {
      // check if HIRES is set
      if ((bplcon0 & 0x8000) != 0)
      {
	draw_line_BPL_res_routine = draw_line_dual_hires_routine;
      }
      else
      {
	draw_line_BPL_res_routine = draw_line_dual_lores_routine;
      }
    }
    else
    {
      // check if HIRES is set
      if ((bplcon0 & 0x8000) != 0)
      {
	draw_line_BPL_res_routine = draw_line_hires_routine;
      }
      else
      {
	draw_line_BPL_res_routine = draw_line_lores_routine;
      }
    }
  }
  graphCalculateWindow();
}

// when ddfstrt is (mod 8)+4, shift order is 8-f,0-7 (lores) (Example: New Zealand Story)
// when ddfstrt is (mod 8)+2, shift order is 4-7,0-3 (hires)

/*===========================================================================*/
/* BPLCON1 - $dff102 Write                                                   */
/*                                                                           */
/* extra variables                                                           */
/* oddscroll - dword with the odd lores scrollvalue                          */
/* evenscroll - dword with the even lores scrollvalue                         */
/*                                                                           */
/* oddhiscroll - dword with the odd hires scrollvalue                        */
/* evenhiscroll - dword with the even hires scrollvalue                      */
/*===========================================================================*/

void wbplcon1(UWO data, ULO address)
{
  bplcon1 = data & 0xff;

  // check for reverse shift order
  if ((ddfstrt & 0x04) != 0)
  {
    oddscroll = ((bplcon1 & 0x0f) + 8) & 0x0f;
  }
  else
  {
    oddscroll = bplcon1 & 0x0f;
  }

  // check for ?
  if ((ddfstrt & 0x02) != 0)
  {
    oddhiscroll = (((oddscroll & 0x07) + 4) & 0x07) << 1;
  }
  else
  {
    oddhiscroll = (oddscroll & 0x07) << 1;
  }

  // check for reverse shift order
  if ((ddfstrt & 0x04) != 0)
  {
    evenscroll = (((bplcon1 & 0xf0) >> 4) + 8) & 0x0f;
  }
  else
  {
    evenscroll = (bplcon1 & 0xf0) >> 4;
  }

  // check for ?
  if ((ddfstrt & 0x02) != 0)
  {
    evenhiscroll = (((evenscroll & 0x07) + 4) & 0x07) << 1;
  }
  else
  {
    evenhiscroll = (evenscroll & 0x07) << 1;
  }

  graphCalculateWindow();
}

/*===========================================================================*/
/* BPLCON2 - $dff104 Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbplcon2(UWO data, ULO address)
{
  bplcon2 = data;
}

/*===========================================================================*/
/* BPL1MOD - $dff108 Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl1mod(UWO data, ULO address)
{
  bpl1mod = (ULO)(LON)(WOR)(data & 0xfffe);
}

/*===========================================================================*/
/* BPL2MOD - $dff10a Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl2mod(UWO data, ULO address)
{
  bpl2mod = (ULO)(LON)(WOR)(data & 0xfffe);
}

/*===========================================================================*/
/* COLOR - $dff180 to $dff1be                                                */
/*                                                                           */
/*===========================================================================*/

void wcolor(UWO data, ULO address)
{
  // normal mode
  graph_color[((address & 0x1ff) - 0x180) >> 1] = (UWO) (data & 0x0fff);
  graph_color_shadow[((address & 0x1ff) - 0x180) >> 1] = draw_color_table[data & 0xfff];
  // half bright mode
  graph_color[(((address & 0x1ff) - 0x180) >> 1) + 32] = (UWO) (((data & 0xfff) & 0xeee) >> 1);
  graph_color_shadow[(((address & 0x1ff) - 0x180) >> 1) + 32] = draw_color_table[(((data & 0xfff) & 0xeee) >> 1)];
}

/*===========================================================================*/
/* Registers the graphics IO register handlers                               */
/*===========================================================================*/

void graphIOHandlersInstall(void)
{
  ULO i;

  memorySetIoReadStub(0x002, rdmaconr);
  memorySetIoReadStub(0x004, rvposr);
  memorySetIoReadStub(0x006, rvhposr);
  memorySetIoReadStub(0x07c, rid);
  memorySetIoWriteStub(0x02a, wvpos);
  memorySetIoWriteStub(0x08e, wdiwstrt);
  memorySetIoWriteStub(0x090, wdiwstop);
  memorySetIoWriteStub(0x092, wddfstrt);
  memorySetIoWriteStub(0x094, wddfstop);
  memorySetIoWriteStub(0x096, wdmacon);
  memorySetIoWriteStub(0x0e0, wbpl1pth);
  memorySetIoWriteStub(0x0e2, wbpl1ptl);
  memorySetIoWriteStub(0x0e4, wbpl2pth);
  memorySetIoWriteStub(0x0e6, wbpl2ptl);
  memorySetIoWriteStub(0x0e8, wbpl3pth);
  memorySetIoWriteStub(0x0ea, wbpl3ptl);
  memorySetIoWriteStub(0x0ec, wbpl4pth);
  memorySetIoWriteStub(0x0ee, wbpl4ptl);
  memorySetIoWriteStub(0x0f0, wbpl5pth);
  memorySetIoWriteStub(0x0f2, wbpl5ptl);
  memorySetIoWriteStub(0x0f4, wbpl6pth);
  memorySetIoWriteStub(0x0f6, wbpl6ptl);
  memorySetIoWriteStub(0x100, wbplcon0);
  memorySetIoWriteStub(0x102, wbplcon1);
  memorySetIoWriteStub(0x104, wbplcon2);
  memorySetIoWriteStub(0x108, wbpl1mod);
  memorySetIoWriteStub(0x10a, wbpl2mod);
  for (i = 0x180; i < 0x1c0; i += 2) memorySetIoWriteStub(i, wcolor);
}

/*===========================================================================*/
/* Called on emulation end of frame                                          */
/*===========================================================================*/

void graphEndOfFrame(void)
{
  graph_playfield_on = FALSE;
  if (graph_buffer_lost == TRUE)
  {
    graphLineDescClear();
    graph_buffer_lost = FALSE;
  }
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
  graph_buffer_lost = FALSE;
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
static __inline ULO graphDecodeDualOdd1(int bitplanes, ULO datA, ULO datB, ULO datC)
{
  switch (bitplanes)
  {
  case 1:
  case 2: return graph_deco1[datA][0]; 
  case 3: 
  case 4: return graph_deco1[datA][0] | graph_deco2[datB][0];  
  case 5: 
  case 6: return graph_deco1[datA][0] | graph_deco2[datB][0] | graph_deco3[datC][0];
  }
  return 0;
}

// Decode the even part of the last 4 pixels
static __inline ULO graphDecodeDualOdd2(int bitplanes, ULO datA, ULO datB, ULO datC)
{
  switch (bitplanes)
  {
  case 1:
  case 2: return graph_deco1[datA][1]; 
  case 3: 
  case 4: return graph_deco1[datA][1] | graph_deco2[datB][1];  
  case 5: 
  case 6: return graph_deco1[datA][1] | graph_deco2[datB][1] | graph_deco3[datC][1]; 
  }
  return 0;
}

// Decode the even part of the first 4 pixels
static __inline ULO graphDecodeDualEven1(int bitplanes, ULO datA, ULO datB, ULO datC)
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
static __inline ULO graphDecodeDualEven2(int bitplanes, ULO datA, ULO datB, ULO datC)
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

// Decode the odd part of the first 4 pixels
static __inline ULO graphDecodeHiOdd(int bitplanes, ULO dat1, ULO dat3, ULO dat5)
{
  switch (bitplanes)
  {
  case 1:
  case 2: return graph_deco320hi1[dat1]; 
  case 3:
  case 4: return graph_deco320hi1[dat1] | graph_deco320hi3[dat3]; 
  case 5:
  case 6: return graph_deco320hi1[dat1] | graph_deco320hi3[dat3]; 
  }
  return 0;
}

// Decode the even part of the first 4 pixels
static __inline ULO graphDecodeHiEven(int bitplanes, ULO dat2, ULO dat4, ULO dat6)
{
  switch (bitplanes)
  {
  case 1:
  case 2:
  case 3: return graph_deco320hi2[dat2]; 
  case 4: 
  case 5: return graph_deco320hi2[dat2] | graph_deco320hi4[dat4];  
  case 6: return graph_deco320hi2[dat2] | graph_deco320hi4[dat4];
  }
  return 0;
}

// Decode the even part of the last 4 pixels
static __inline ULO graphDecodeHiDualOdd(int bitplanes, ULO datA, ULO datB, ULO datC)
{
  switch (bitplanes)
  {
  case 1:
  case 2: return graph_deco320hi1[datA]; 
  case 3: 
  case 4: return graph_deco320hi1[datA] | graph_deco320hi2[datB];  
  case 5: 
  case 6: return graph_deco320hi1[datA] | graph_deco320hi2[datB] | graph_deco320hi3[datC]; 
  }
  return 0;
}

// Decode the even part of the first 4 pixels
static __inline ULO graphDecodeHiDualEven(int bitplanes, ULO datA, ULO datB, ULO datC)
{
  switch (bitplanes)
  {
  case 1:
  case 2:
  case 3: return graph_deco320hi1[datA]; 
  case 4: 
  case 5: return graph_deco320hi1[datA] | graph_deco320hi2[datB];  
  case 6: return graph_deco320hi1[datA] | graph_deco320hi2[datB] | graph_deco320hi3[datC];
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

static void graphSetLinePointers(UBY **line1, UBY **line2)
{
  // make function compatible for no line skipping
  if (graph_allow_bpl_line_skip)
  {
    // Dummy lines
    *line1 = graph_line1_tmp;
    *line2 = graph_line2_tmp;
  }
  else
  {
    ULO currentY = busGetRasterY();
    *line1 = graph_frame[draw_buffer_draw][currentY].line1;
    *line2 = graph_frame[draw_buffer_draw][currentY].line2;
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
    UBY *line1;
    UBY *line2;

    dat1 = dat2 = dat3= dat4= dat5 = dat6 = 0;

    graphSetLinePointers(&line1, &line2);

    if ((bplcon0 & 0x8000) == 0x8000) // check if hires bit is set (bit 15 of register BPLCON0)
    {
      // high resolution
      dest_odd = (ULO*) (line1 + graph_DDF_start + oddhiscroll);		

      // Find out how many pixels the bitplane is scrolled
      // the first pixels must then be zeroed to avoid garbage.
      maxscroll = (evenhiscroll > oddhiscroll) ? evenhiscroll : oddhiscroll; 
    } 
    else 
    {
      dest_odd = (ULO*) (line1 + graph_DDF_start + oddscroll);			

      // Find out how many pixels the bitplane is scrolled
      // the first pixels must then be zeroed to avoid garbage.
      maxscroll = (evenscroll > oddscroll) ? evenscroll : oddscroll; 
    }

    // clear edges
    while (maxscroll > 0)
    {
      line1[graph_DDF_start + temp] = 0;
      line1[graph_DDF_start + (graph_DDF_word_count << 4) + temp] = 0;
      maxscroll--;
      temp++;
    }

    // setup loop
    end_odd = dest_odd + bpl_length_in_bytes * 2; 

    if (bitplanes > 1)
    {
      if ((bplcon0 & 0x8000) == 0x8000) // check if hires bit is set (bit 15 of register BPLCON0)
      {
	// high resolution
	dest_even = (ULO*) (line1 + graph_DDF_start + evenhiscroll);
      }
      else
      {
	// low resolution
	dest_even = (ULO*) (line1 + graph_DDF_start + evenscroll);
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
      switch (bitplanes)
      {
      case 6:
      case 5:	dat5 = *pt5_tmp++;
      case 4:
      case 3:	dat3 = *pt3_tmp++;
      case 2:
      case 1:	dat1 = *pt1_tmp++;
      }
      dest_tmp[0] = graphDecodeOdd1(bitplanes, dat1, dat3, dat5);
      dest_tmp[1] = graphDecodeOdd2(bitplanes, dat1, dat3, dat5);
    }

    if (bitplanes >= 2) 
    {
      for (dest_tmp = dest_even; dest_tmp != end_even; dest_tmp += 2)
      {
	switch (bitplanes)
	{
	case 6:	dat6 = *pt6_tmp++;
	case 5:
	case 4:	dat4 = *pt4_tmp++;
	case 3:
	case 2:	dat2 = *pt2_tmp++;
	}
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
    UBY *line1;
    UBY *line2;

    dat1 = dat2 = dat3= dat4= dat5 = dat6 = 0;

    graphSetLinePointers(&line1, &line2);

    // clear edges
    maxscroll = (evenscroll > oddscroll) ? evenscroll : oddscroll; 

    temp = 0;
    while (maxscroll > 0)
    {
      line1[graph_DDF_start + temp] = 0;
      line2[graph_DDF_start + temp] = 0;
      line1[graph_DDF_start + (graph_DDF_word_count << 4) + temp] = 0;
      line2[graph_DDF_start + (graph_DDF_word_count << 4) + temp] = 0;
      maxscroll--;
      temp++;
    }

    // setup loop
    dest_odd = (ULO*) (line1 + graph_DDF_start + oddscroll);			
    end_odd = dest_odd + bpl_length_in_bytes * 2; 

    if (bitplanes > 1)
    {
      // low resolution
      dest_even = (ULO*) (line2 + graph_DDF_start + evenscroll);
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
      switch (bitplanes)
      {
      case 6:
      case 5:	dat5 = *pt5_tmp++;
      case 4:
      case 3:	dat3 = *pt3_tmp++;
      case 2:
      case 1:	dat1 = *pt1_tmp++;
      }
      dest_tmp[0] = graphDecodeDualOdd1(bitplanes, dat1, dat3, dat5);
      dest_tmp[1] = graphDecodeDualOdd2(bitplanes, dat1, dat3, dat5);
    }

    if (bitplanes >= 2) 
    {
      for (dest_tmp = dest_even; dest_tmp != end_even; dest_tmp += 2)
      {
	switch (bitplanes)
	{
	case 6:	dat6 = *pt6_tmp++;
	case 5:
	case 4:	dat4 = *pt4_tmp++;
	case 3:
	case 2:	dat2 = *pt2_tmp++;
	}
	dest_tmp[0] = graphDecodeDualEven1(bitplanes, dat2, dat4, dat6);
	dest_tmp[1] = graphDecodeDualEven2(bitplanes, dat2, dat4, dat6);
      }
    }
  }
  graphDecodeModulo(bitplanes, bpl_length_in_bytes);
}

static __inline void graphDecodeHi320Generic(int bitplanes)
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
    UBY *line1;
    UBY *line2;

    dat1 = dat2 = dat3= dat4= dat5 = dat6 = 0;

    graphSetLinePointers(&line1, &line2);

    if ((bplcon0 & 0x8000) == 0x8000) // check if hires bit is set (bit 15 of register BPLCON0)
    {
      // high resolution
      dest_odd = (ULO*) (line1 + (graph_DDF_start >> 1) + (oddhiscroll >> 1));		

      // setup loop
      end_odd = dest_odd + bpl_length_in_bytes; 

      if (bitplanes > 1)
      {
	// high resolution
	dest_even = (ULO*) (line1 + (graph_DDF_start >> 1) + (evenhiscroll >> 1));
	end_even = dest_even + bpl_length_in_bytes; 
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

      for (dest_tmp = dest_odd; dest_tmp != end_odd; dest_tmp += 1) 
      {
	switch (bitplanes)
	{
	case 6:
	case 5:	dat5 = *pt5_tmp++;
	case 4:
	case 3:	dat3 = *pt3_tmp++;
	case 2:
	case 1:	dat1 = *pt1_tmp++;
	}
	dest_tmp[0] = graphDecodeHiOdd(bitplanes, dat1, dat3, dat5);
      }

      if (bitplanes >= 2) 
      {
	for (dest_tmp = dest_even; dest_tmp != end_even; dest_tmp += 1)
	{
	  switch (bitplanes)
	  {
	  case 6:	dat6 = *pt6_tmp++;
	  case 5:
	  case 4:	dat4 = *pt4_tmp++;
	  case 3:
	  case 2:	dat2 = *pt2_tmp++;
	  }
	  dest_tmp[0] |= graphDecodeHiEven(bitplanes, dat2, dat4, dat6);
	}
      }
    }
    graphDecodeModulo(bitplanes, bpl_length_in_bytes);
  }
}


static __inline void graphDecodeDualHi320Generic(int bitplanes)
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
    UBY *line1;
    UBY *line2;

    dat1 = dat2 = dat3= dat4= dat5 = dat6 = 0;

    graphSetLinePointers(&line1, &line2);

    // setup loop
    dest_odd = (ULO*) (line1 + (graph_DDF_start >> 1) + (oddscroll >> 1));			
    end_odd = dest_odd + bpl_length_in_bytes; 

    if (bitplanes > 1)
    {
      // low resolution
      dest_even = (ULO*) (line2 + (graph_DDF_start >> 1)+ (evenscroll >> 1));
      end_even = dest_even + bpl_length_in_bytes; 
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

    for (dest_tmp = dest_odd; dest_tmp != end_odd; dest_tmp += 1) 
    {
      switch (bitplanes)
      {
      case 6:
      case 5:	dat5 = *pt5_tmp++;
      case 4:
      case 3:	dat3 = *pt3_tmp++;
      case 2:
      case 1:	dat1 = *pt1_tmp++;
      }
      dest_tmp[0] = graphDecodeHiDualOdd(bitplanes, dat1, dat3, dat5);
    }

    if (bitplanes >= 2) 
    {
      for (dest_tmp = dest_even; dest_tmp != end_even; dest_tmp += 1)
      {
	switch (bitplanes)
	{
	case 6:	dat6 = *pt6_tmp++;
	case 5:
	case 4:	dat4 = *pt4_tmp++;
	case 3:
	case 2:	dat2 = *pt2_tmp++;
	}
	dest_tmp[0] = graphDecodeHiDualEven(bitplanes, dat2, dat4, dat6);
      }
    }
  }
  graphDecodeModulo(bitplanes, bpl_length_in_bytes);
}

/*===========================================================================*/
/* Planar to chunky conversion, 1 bitplane hires or lores                    */
/*===========================================================================*/

void graphDecode0(void)
{
  graphDecodeGeneric(0);
}

/*===========================================================================*/
/* Planar to chunky conversion, 1 bitplane hires or lores                    */
/*===========================================================================*/

void graphDecode1(void)
{
  graphDecodeGeneric(1);
}

/*===========================================================================*/
/* Planar to chunky conversion, 2 bitplanes hires or lores                   */
/*===========================================================================*/

void graphDecode2(void)
{
  graphDecodeGeneric(2);
}

/*===========================================================================*/
/* Planar to chunky conversion, 3 bitplanes hires or lores                   */
/*===========================================================================*/

void graphDecode3(void)
{
  graphDecodeGeneric(3);
}

/*===========================================================================*/
/* Planar to chunky conversion, 4 bitplanes hires or lores                   */
/*===========================================================================*/

void graphDecode4(void)
{
  graphDecodeGeneric(4);
}

/*===========================================================================*/
/* Planar to chunky conversion, 5 bitplanes hires or lores                   */
/*===========================================================================*/

void graphDecode5(void)
{
  graphDecodeGeneric(5);
}

/*===========================================================================*/
/* Planar to chunky conversion, 6 bitplanes hires or lores                   */
/*===========================================================================*/

void graphDecode6(void)
{
  graphDecodeGeneric(6);
}

/*===========================================================================*/
/* Planar to chunky conversion, 1 bitplane hires to 320                      */
/*===========================================================================*/

void graphDecodeHi1(void)
{
  graphDecodeHi320Generic(1);
}

/*===========================================================================*/
/* Planar to chunky conversion, 2 bitplanes hires to 320                     */
/*===========================================================================*/

void graphDecodeHi2(void)
{
  graphDecodeHi320Generic(2);
}

/*===========================================================================*/
/* Planar to chunky conversion, 3 bitplanes hires to 320                     */
/*===========================================================================*/

void graphDecodeHi3(void)
{
  graphDecodeHi320Generic(3);
}

/*===========================================================================*/
/* Planar to chunky conversion, 4 bitplanes hires to 320                     */
/*===========================================================================*/

void graphDecodeHi4(void)
{
  graphDecodeHi320Generic(4);
}

/*===========================================================================*/
/* Planar to chunky conversion, 5 bitplanes hires to 320                     */
/*===========================================================================*/

void graphDecodeHi5(void)
{
  graphDecodeHi320Generic(5);
}

/*===========================================================================*/
/* Planar to chunky conversion, 6 bitplanes hires to 320                     */
/*===========================================================================*/

void graphDecodeHi6(void)
{
  graphDecodeHi320Generic(6);
}

/*===========================================================================*/
/* Planar to chunky conversion, 2 bitplanes lores, dual playfield            */
/*===========================================================================*/
void graphDecode2Dual(void)
{
  graphDecodeDualGeneric(2);
}

/*===========================================================================*/
/* Planar to chunky conversion, 3 bitplanes lores, dual playfield            */
/*===========================================================================*/
void graphDecode3Dual(void)
{
  graphDecodeDualGeneric(3);
}

/*===========================================================================*/
/* Planar to chunky conversion, 4 bitplanes lores, dual playfield            */
/*===========================================================================*/
void graphDecode4Dual(void)
{
  graphDecodeDualGeneric(4);
}

/*===========================================================================*/
/* Planar to chunky conversion, 5 bitplanes lores, dual playfield            */
/*===========================================================================*/
void graphDecode5Dual(void)
{
  graphDecodeDualGeneric(5);
}

/*===========================================================================*/
/* Planar to chunky conversion, 6 bitplanes lores, dual playfield            */
/*===========================================================================*/
void graphDecode6Dual(void)
{
  graphDecodeDualGeneric(6);
}

/*===========================================================================*/
/* Planar to chunky conversion, 2 bitplanes hires, dual playfield            */
/*===========================================================================*/
void graphDecodeHi2Dual(void)
{
  graphDecodeDualHi320Generic(2);
}

/*===========================================================================*/
/* Planar to chunky conversion, 3 bitplanes hires, dual playfield            */
/*===========================================================================*/
void graphDecodeHi3Dual(void)
{
  graphDecodeDualHi320Generic(3);
}

/*===========================================================================*/
/* Planar to chunky conversion, 4 bitplanes hires, dual playfield            */
/*===========================================================================*/
void graphDecodeHi4Dual(void)
{
  graphDecodeDualHi320Generic(4);
}

/*===========================================================================*/
/* Planar to chunky conversion, 5 bitplanes hires, dual playfield            */
/*===========================================================================*/
void graphDecodeHi5Dual(void)
{
  graphDecodeDualHi320Generic(5);
}

/*===========================================================================*/
/* Planar to chunky conversion, 6 bitplanes hires, dual playfield            */
/*===========================================================================*/
void graphDecodeHi6Dual(void)
{
  graphDecodeDualHi320Generic(6);
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

void graphCalculateWindow(void) 
{
  ULO ddfstop_aligned, ddfstrt_aligned, last_position_in_line;

  if ((bplcon0 & 0x8000) == 0x8000) // check if Hires bit is set (bit 15 of BPLCON0)
  {
    graphCalculateWindowHires();
  } 
  else 
  {
    // set DDF (Display Data Fetch Start) variables
    if (ddfstop < ddfstrt) 
    {
      graph_DDF_start = graph_DDF_word_count = 0;
      graph_DIW_first_visible = graph_DIW_last_visible = 256;
      return;
    }

    ddfstop_aligned = ddfstop & 0x07;
    ddfstrt_aligned = ddfstrt & 0x07;

    if (ddfstop >= ddfstrt) 
    {
      graph_DDF_word_count = ((ddfstop - ddfstrt) >> 3) + 1;
    }
    else
    {
      graph_DDF_word_count = ((0xd8 - ddfstrt) >> 3) + 1;
      ddfstop_aligned = 0;
    }

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

void graphCalculateWindowHires(void)
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

  if (ddfstop >= ddfstrt)
  {
    graph_DDF_word_count = (((ddfstop - ddfstrt) + 15) >> 2) & 0x0FFFFFFFE;
  }
  else
  {
    graph_DDF_word_count = (((0xd8 - ddfstrt) + 15) >> 2) & 0x0FFFFFFFE;
    ddfstop_aligned = 0;
  }

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
  ULO currentY = busGetRasterY();
  if (graph_playfield_on != 0) 
  {
    // Playfield on, check if top has moved below graph_raster_y
    // Playfield on, check if playfield is turned off at this line
    if (!(currentY != diwybottom))
    {
      graph_playfield_on = 0;
    }
  } 
  else 
  {
    // Playfield off, Check if playfield is turned on at this line
    if (!(currentY != diwytop))
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

void graphDecodeNOP(void)
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

/*-------------------------------------------------------------------------------
/* Copy color block to line description
/* [4 + esp] - linedesc struct
/*-------------------------------------------------------------------------------*/


void graphLinedescColors(graph_line* current_graph_line)
{
  ULO color;

  color = 0;
  while (color < 64)
  {
    current_graph_line->colors[color] = graph_color_shadow[color];
    color++;
  }
}


/*-------------------------------------------------------------------------------
/* Sets line routines for this line
/* [4 + esp] - linedesc struct
/*-------------------------------------------------------------------------------*/

void graphLinedescRoutines(graph_line* current_graph_line)
{
  /*==============================================================
  /* Set drawing routines
  /*==============================================================*/
  current_graph_line->draw_line_routine = draw_line_routine;
  current_graph_line->draw_line_BPL_res_routine = draw_line_BPL_res_routine;
}

/*-------------------------------------------------------------------------------
/* Sets line geometry data in line description
/* [4 + esp] - linedesc struct
/*-------------------------------------------------------------------------------*/

void graphLinedescGeometry(graph_line* current_graph_line)
{
  ULO local_graph_DIW_first_visible;
  LON local_graph_DIW_last_visible;
  ULO local_graph_DDF_start;
  ULO local_draw_left;
  ULO local_draw_right;
  ULO shift;

  local_graph_DIW_first_visible = graph_DIW_first_visible;
  local_graph_DIW_last_visible  = (LON) graph_DIW_last_visible;
  local_graph_DDF_start         = graph_DDF_start;
  local_draw_left               = draw_left;
  local_draw_right              = draw_right;
  shift                         = 0;

  /*===========================================================*/
  /* Calculate first and last visible DIW and DIW pixel count  */
  /*===========================================================*/

  if ((bplcon0 & 0x8000) != 0)
  {
    // bit 15, HIRES is set
    local_graph_DIW_first_visible >>= 1;
    local_graph_DIW_last_visible >>= 1;
    local_graph_DDF_start >>= 1;
    if (draw_hscale == 2)
    {
      shift = 1;
    }
  }
  if (local_graph_DIW_first_visible < local_draw_left)
  {
    local_graph_DIW_first_visible = local_draw_left;
  }
  if (local_graph_DIW_last_visible > local_draw_right)
  {
    local_graph_DIW_last_visible = (LON) local_draw_right;
  }
  local_graph_DIW_last_visible -= local_graph_DIW_first_visible;
  if (local_graph_DIW_last_visible < 0)
  {
    local_graph_DIW_last_visible = 0;
  }
  local_graph_DIW_first_visible <<= shift;
  local_graph_DIW_last_visible <<= shift;
  current_graph_line->DIW_first_draw = local_graph_DIW_first_visible;
  current_graph_line->DIW_pixel_count = local_graph_DIW_last_visible;
  current_graph_line->DDF_start = local_graph_DDF_start;
  local_graph_DIW_first_visible >>= shift;
  local_graph_DIW_last_visible >>= shift;

  /*=========================================*/
  /* Calculate BG front and back pad count   */
  /*=========================================*/

  local_draw_right -= local_graph_DIW_first_visible;
  local_graph_DIW_first_visible -= local_draw_left;
  local_draw_right -= local_graph_DIW_last_visible;
  current_graph_line->BG_pad_front = local_graph_DIW_first_visible;
  current_graph_line->BG_pad_back  = local_draw_right;

  /*==========================================================*/
  /* Need to remember playfield priorities to sort dual pf    */
  /*==========================================================*/

  current_graph_line->bplcon2 = bplcon2;
}

/*-------------------------------------------------------------------------------
/* Makes a description of this line
/* [4 + esp] - linedesc struct
/*-------------------------------------------------------------------------------*/

void graphLinedescMake(graph_line* current_graph_line)
{
  /*==========================================================*/
  /* Is this a bitplane or background line?                   */
  /*==========================================================*/
  if (draw_line_BG_routine != draw_line_routine)
  {
    /*===========================================*/
    /* This is a bitplane line                   */
    /*===========================================*/
    current_graph_line->linetype = GRAPH_LINE_BPL;
    graphLinedescColors(current_graph_line);
    graphLinedescGeometry(current_graph_line);
    graphLinedescRoutines(current_graph_line);
  }
  else
  {
    /*===========================================*/
    /* This is a background line                 */
    /*===========================================*/
    if (graph_color_shadow[0] == current_graph_line->colors[0])
    {
      if (current_graph_line->linetype == GRAPH_LINE_SKIP)
      {
	return;
      }
      if (current_graph_line->linetype == GRAPH_LINE_BG)
      {
	/*==================================================================*/
	/* This line was a "background" line during the last drawing of     */
	/* this frame buffer,                                               */
	/* and it has the same color as last time.                          */
	/* We might be able to skip drawing it, we need to check the        */
	/* flag that says it has been drawn in all of our buffers.          */
	/* The flag is only of importance when "deinterlacing"              */
	/*==================================================================*/
	if (current_graph_line->frames_left_until_BG_skip == 0)
	{
	  current_graph_line->linetype = GRAPH_LINE_SKIP;
	  return;
	}
	else
	{
	  current_graph_line->frames_left_until_BG_skip--;
	  return;
	}
      }
    }

    /*==================================================================*/
    /* This background line is different from the one in the buffer     */
    /*==================================================================*/
    current_graph_line->linetype = GRAPH_LINE_BG;
    current_graph_line->colors[0] = graph_color_shadow[0];
    if (draw_deinterlace == 0)
    {
      // dword [draw_buffer_count]
      current_graph_line->frames_left_until_BG_skip = 0;
    }
    else
    {
      current_graph_line->frames_left_until_BG_skip = 1;
    }
    graphLinedescRoutines(current_graph_line);
  }
}

/*-------------------------------------------------------------------------------
/* Compose the visible layout of the line
/* [4 + esp] - linedesc struct
/*-------------------------------------------------------------------------------*/

void graphComposeLineOutput(graph_line* current_graph_line)
{
  /*==================================================================*/
  /* Check if we need to decode bitplane data                         */
  /*==================================================================*/

  if (draw_line_BG_routine != draw_line_routine) 
  {
    /*================================================================*/
    /* Do the planar to chunky conversion                             */
    /*================================================================*/
    graph_decode_line_ptr();

    /*================================================================*/
    /* Add sprites to the line image                                  */
    /*================================================================*/
    if (sprites_online == 1)
    {
      spritesMerge(current_graph_line);
      sprites_online = 0;
    }

    /*================================================================*/
    /* Remember line geometry for later drawing                       */
    /*================================================================*/
  }
  graphLinedescMake(current_graph_line);
}

/*-------------------------------------------------------------------------------
/* Smart sets line routines for this line
/* [4 + esp] - linedesc struct
/* Return 1 if routines have changed (eax)
/*-------------------------------------------------------------------------------*/
BOOLE graphLinedescRoutinesSmart(graph_line* current_graph_line)
{
  BOOLE result;

  /*==============================================================*/
  /* Set drawing routines                                         */
  /*==============================================================*/

  result = FALSE;
  if (current_graph_line->draw_line_routine != draw_line_routine)
  {
    result = TRUE;
  }
  current_graph_line->draw_line_routine = draw_line_routine;

  if (current_graph_line->draw_line_BPL_res_routine != draw_line_BPL_res_routine)
  {
    result = TRUE;
  }
  current_graph_line->draw_line_BPL_res_routine = draw_line_BPL_res_routine;
  return result;
}

/*-------------------------------------------------------------------------------
/* Sets line geometry data in line description
/* [4 + esp] - linedesc struct
/* Return 1 if geometry has changed (eax)
/*-------------------------------------------------------------------------------*/
BOOLE graphLinedescGeometrySmart(graph_line* current_graph_line)
{
  ULO local_graph_DIW_first_visible;
  LON local_graph_DIW_last_visible;
  ULO local_graph_DDF_start;
  ULO local_draw_left;
  ULO local_draw_right;
  ULO shift;
  BOOLE line_desc_changed;

  local_graph_DIW_first_visible = graph_DIW_first_visible;
  local_graph_DIW_last_visible  = (LON) graph_DIW_last_visible;
  local_graph_DDF_start         = graph_DDF_start;
  local_draw_left               = draw_left;
  local_draw_right              = draw_right;
  shift                         = 0;
  line_desc_changed             = FALSE;

  /*===========================================================*/
  /* Calculate first and last visible DIW and DIW pixel count  */
  /*===========================================================*/

  if ((bplcon0 & 0x8000) != 0)
  {
    // bit 15, HIRES is set
    local_graph_DIW_first_visible >>= 1;
    local_graph_DIW_last_visible >>= 1;
    local_graph_DDF_start >>= 1;
    if (draw_hscale == 2)
    {
      shift = 1;
    }
  }
  if (local_graph_DIW_first_visible < local_draw_left)
  {
    local_graph_DIW_first_visible = local_draw_left;
  }
  if (local_graph_DIW_last_visible > local_draw_right)
  {
    local_graph_DIW_last_visible = (LON) local_draw_right;
  }
  local_graph_DIW_last_visible -= local_graph_DIW_first_visible;
  if (local_graph_DIW_last_visible < 0)
  {
    local_graph_DIW_last_visible = 0;
  }

  local_graph_DIW_first_visible <<= shift;
  local_graph_DIW_last_visible <<= shift;
  if (current_graph_line->DIW_first_draw != local_graph_DIW_first_visible)
  {
    line_desc_changed = TRUE;
  }
  current_graph_line->DIW_first_draw = local_graph_DIW_first_visible;
  if (current_graph_line->DIW_pixel_count != local_graph_DIW_last_visible)
  {
    line_desc_changed = TRUE;
  }
  current_graph_line->DIW_pixel_count = local_graph_DIW_last_visible;
  if (current_graph_line->DDF_start != local_graph_DDF_start)
  {
    line_desc_changed = TRUE;
  }
  current_graph_line->DDF_start = local_graph_DDF_start;
  local_graph_DIW_first_visible >>= shift;
  local_graph_DIW_last_visible >>= shift;

  /*=========================================*/
  /* Calculate BG front and back pad count   */
  /*=========================================*/

  local_draw_right -= local_graph_DIW_first_visible;
  local_graph_DIW_first_visible -= local_draw_left;
  local_draw_right -= local_graph_DIW_last_visible;
  if (current_graph_line->BG_pad_front != local_graph_DIW_first_visible)
  {
    line_desc_changed = TRUE;
  }
  current_graph_line->BG_pad_front = local_graph_DIW_first_visible;
  if (current_graph_line->BG_pad_back != local_draw_right)
  {
    line_desc_changed = TRUE;
  }
  current_graph_line->BG_pad_back = local_draw_right;

  /*==========================================================*/
  /* Need to remember playfield priorities to sort dual pf    */
  /*==========================================================*/

  if (current_graph_line->bplcon2 != bplcon2)
  {
    line_desc_changed = TRUE;
  }
  current_graph_line->bplcon2 = bplcon2;
  return line_desc_changed;
}

/*-------------------------------------------------------------------------------
/* Smart copy color block to line description
/* [4 + esp] - linedesc struct
/* Return 1 if colors have changed (eax)
/*-------------------------------------------------------------------------------*/

BOOLE graphLinedescColorsSmart(graph_line* current_graph_line)
{
  BOOLE result = FALSE;
  ULO i;

  // check full brightness colors
  for (i = 0; i < 32; i++)
  {
    if (graph_color_shadow[i] != current_graph_line->colors[i])
    {
      current_graph_line->colors[i] = graph_color_shadow[i];
      current_graph_line->colors[i + 32] = graph_color_shadow[i + 32];
      result = TRUE;
    }
  }
  return result;
}

/*-------------------------------------------------------------------------------
/* Copy remainder of the data, data changed, no need to compare anymore.
/* Return TRUE = not equal
/*-------------------------------------------------------------------------------*/

static BOOLE graphCompareCopyRest(ULO first_pixel, LON pixel_count, UBY* dest_line, UBY* source_line)
{
  // line has changed, copy the rest
  while ((first_pixel & 0x3) != 0)
  {
    dest_line[first_pixel] = source_line[first_pixel];
    first_pixel++;
    pixel_count--;
    if (pixel_count == 0)
    {
      return TRUE;
    }
  }

  while (pixel_count >= 4)
  {
    *((ULO *) (dest_line + first_pixel)) = *((ULO *) (source_line + first_pixel));
    first_pixel += 4;
    pixel_count -= 4;
  }

  while (pixel_count > 0)
  {
    dest_line[first_pixel] = source_line[first_pixel];
    first_pixel++;
    pixel_count--;
  }
  return TRUE;
}

/*-------------------------------------------------------------------------------
/* Copy data and compare
/* [4 + esp] - source playfield
/* [8 + esp] - destination playfield
/* [12 + esp] - pixel count
/* [16 + esp] - first pixel
/* Return 1 = not equal (eax), 0 = equal
/*-------------------------------------------------------------------------------*/

static BOOLE graphCompareCopy(ULO first_pixel, LON pixel_count, UBY* dest_line, UBY* source_line)
{
  BOOLE result = FALSE;

  if (pixel_count > 0)
  {
    // align to 4-byte boundary
    while ((first_pixel & 0x3) != 0)
    {
      if (dest_line[first_pixel] == source_line[first_pixel])
      {
	first_pixel++;
	pixel_count--;
	if (pixel_count == 0)
	{
	  return FALSE;
	}
      }
      else
      {
	// line has changed, copy the rest
	return graphCompareCopyRest(first_pixel, pixel_count, dest_line, source_line);
      }
    }

    // compare dword aligned values
    while (pixel_count >= 4)
    {
      if (*((ULO *) (source_line + first_pixel)) == *((ULO *) (dest_line + first_pixel)))
      {
	first_pixel += 4;
	pixel_count -= 4;
      }
      else
      {
	// line has changed, copy the rest
	return graphCompareCopyRest(first_pixel, pixel_count, dest_line, source_line);
      }
    }

    // compare last couple of bytes
    while (pixel_count > 0)
    {
      if (source_line[first_pixel] == dest_line[first_pixel])
      {
	first_pixel++;
	pixel_count--;
      }
      else
      {
	result = TRUE;
	dest_line[first_pixel] = source_line[first_pixel];
	first_pixel++;
	pixel_count--;
      }
    }
  }
  return result;
}

/*-------------------------------------------------------------------------------
/* Smart makes a description of this line
/* [4 + esp] - linedesc struct
/* Return 1 if linedesc has changed (eax)
/*-------------------------------------------------------------------------------*/

BOOLE graphLinedescMakeSmart(graph_line* current_graph_line)
{
  BOOLE line_desc_changed;

  line_desc_changed = FALSE;

  /*==========================================================*/
  /* Is this a bitplane or background line?                   */
  /*==========================================================*/
  if (draw_line_BG_routine != draw_line_routine)
  {
    /*===========================================*/
    /* This is a bitplane line                   */
    /*===========================================*/
    if (current_graph_line->linetype != GRAPH_LINE_BPL)
    {
      if (current_graph_line->linetype != GRAPH_LINE_BPL_SKIP)
      {
	current_graph_line->linetype = GRAPH_LINE_BPL;
	graphLinedescColors(current_graph_line);
	graphLinedescGeometry(current_graph_line);
	graphLinedescRoutines(current_graph_line);
	return TRUE;
      }
    }
    line_desc_changed = graphLinedescColorsSmart(current_graph_line);
    line_desc_changed |= graphLinedescGeometrySmart(current_graph_line);
    line_desc_changed |= graphLinedescRoutinesSmart(current_graph_line);
    return line_desc_changed;
  }
  else
  {
    /*===========================================*/
    /* This is a background line                 */
    /*===========================================*/
    if (graph_color_shadow[0] == current_graph_line->colors[0])
    {
      if (current_graph_line->linetype == GRAPH_LINE_SKIP)
      {
	line_desc_changed = FALSE;
	return line_desc_changed;
      }
      if (current_graph_line->linetype == GRAPH_LINE_BG)
      {
	/*==================================================================*/
	/* This line was a "background" line during the last drawing of     */
	/* this frame buffer,                                               */
	/* and it has the same color as last time.                          */
	/* We might be able to skip drawing it, we need to check the        */
	/* flag that says it has been drawn in all of our buffers.          */
	/* The flag is only of importance when "deinterlacing"              */
	/*==================================================================*/
	if (current_graph_line->frames_left_until_BG_skip == 0)
	{
	  current_graph_line->linetype = GRAPH_LINE_SKIP;
	  line_desc_changed = FALSE;
	  return line_desc_changed;
	}
	else
	{
	  current_graph_line->frames_left_until_BG_skip--;
	  line_desc_changed = TRUE;
	  return line_desc_changed;
	}
      }
    }

    /*==================================================================*/
    /* This background line is different from the one in the buffer     */
    /*==================================================================*/
    current_graph_line->linetype = GRAPH_LINE_BG;
    current_graph_line->colors[0] = graph_color_shadow[0];
    if (draw_deinterlace == 0)
    {
      // dword [draw_buffer_count]
      current_graph_line->frames_left_until_BG_skip = 0;
    }
    else
    {
      current_graph_line->frames_left_until_BG_skip = 1;
    }
    graphLinedescRoutines(current_graph_line);
    line_desc_changed = TRUE;
  }
  return line_desc_changed;
}

/*===========================================================================*/
/* Smart compose the visible layout of the line                              */
/* [4 + esp] - linedesc struct                                               */
/*===========================================================================*/

void graphComposeLineOutputSmart(graph_line* current_graph_line)
{
  // remember the basic properties of the line
  BOOLE line_desc_changed = graphLinedescMakeSmart(current_graph_line);

  // check if we need to decode bitplane data
  if (draw_line_BG_routine != draw_line_routine)
  {
    // do the planar to chunky conversion
    // stuff the data into a temporary array
    // then copy it and compare
    graph_decode_line_ptr();

    // compare line data to old data  
    line_desc_changed |= graphCompareCopy(current_graph_line->DIW_first_draw, (LON) (current_graph_line->DIW_pixel_count), current_graph_line->line1, graph_line1_tmp);

    // if the line is dual playfield, compare second playfield too
    if ((bplcon0 & 0x0400) != 0x0)
    {
      line_desc_changed |= graphCompareCopy(current_graph_line->DIW_first_draw, (LON) (current_graph_line->DIW_pixel_count), current_graph_line->line2, graph_line2_tmp);
    }

    // add sprites to the line image
    if (sprites_online)
    {
      line_desc_changed = TRUE;
      spritesMerge(current_graph_line);
      sprites_online = FALSE;
    }

    // final test for line skip
    if (line_desc_changed)
    {
      current_graph_line->linetype = GRAPH_LINE_BPL;
    }
    else
    {
      current_graph_line->linetype = GRAPH_LINE_BPL_SKIP;
    }
  }
}

/*===========================================================================*/
/* Decode tables for horizontal scale                                        */
/* 1x scaling Lores                                                          */
/* 0.5x scaling Hires                                                        */
/* Called from the draw module                                               */
/*===========================================================================*/

void graphP2C1XInit(void)
{
  graph_decode_line_tab[0] = graphDecode0;
  graph_decode_line_tab[1] = graphDecode1;
  graph_decode_line_tab[2] = graphDecode2;
  graph_decode_line_tab[3] = graphDecode3;
  graph_decode_line_tab[4] = graphDecode4;
  graph_decode_line_tab[5] = graphDecode5;
  graph_decode_line_tab[6] = graphDecode6;
  graph_decode_line_tab[7] = graphDecode0;
  graph_decode_line_tab[8] = graphDecode0;
  graph_decode_line_tab[9] = graphDecodeHi1;
  graph_decode_line_tab[10] = graphDecodeHi2;
  graph_decode_line_tab[11] = graphDecodeHi3;
  graph_decode_line_tab[12] = graphDecodeHi4;
  graph_decode_line_tab[13] = graphDecode0;
  graph_decode_line_tab[14] = graphDecode0;
  graph_decode_line_tab[15] = graphDecode0;
  graph_decode_line_dual_tab[0] = graphDecode0;
  graph_decode_line_dual_tab[1] = graphDecode1;
  graph_decode_line_dual_tab[2] = graphDecode2Dual;
  graph_decode_line_dual_tab[3] = graphDecode3Dual;
  graph_decode_line_dual_tab[4] = graphDecode4Dual;
  graph_decode_line_dual_tab[5] = graphDecode5Dual;
  graph_decode_line_dual_tab[6] = graphDecode6Dual;
  graph_decode_line_dual_tab[7] = graphDecode0;
  graph_decode_line_dual_tab[8] = graphDecode0;
  graph_decode_line_dual_tab[9] = graphDecodeHi1;
  graph_decode_line_dual_tab[10] = graphDecodeHi2Dual;
  graph_decode_line_dual_tab[11] = graphDecodeHi3Dual;
  graph_decode_line_dual_tab[12] = graphDecodeHi4Dual;
  graph_decode_line_dual_tab[13] = graphDecode0;
  graph_decode_line_dual_tab[14] = graphDecode0;
  graph_decode_line_dual_tab[15] = graphDecode0;
  graph_decode_line_ptr = graphDecode0;
}


/*===========================================================================*/
/* Decode tables for horizontal scale                                        */
/* 1x scaling Lores                                                          */
/* 1x scaling Hires                                                          */
/* Called from the draw module                                               */
/*===========================================================================*/

void graphP2C2XInit(void)
{
  graph_decode_line_tab[0] = graphDecode0;
  graph_decode_line_tab[1] = graphDecode1;
  graph_decode_line_tab[2] = graphDecode2;
  graph_decode_line_tab[3] = graphDecode3;
  graph_decode_line_tab[4] = graphDecode4;
  graph_decode_line_tab[5] = graphDecode5;
  graph_decode_line_tab[6] = graphDecode6;
  graph_decode_line_tab[7] = graphDecode0;
  graph_decode_line_tab[8] = graphDecode0;
  graph_decode_line_tab[9] = graphDecode1;
  graph_decode_line_tab[10] = graphDecode2;
  graph_decode_line_tab[11] = graphDecode3;
  graph_decode_line_tab[12] = graphDecode4;
  graph_decode_line_tab[13] = graphDecode0;
  graph_decode_line_tab[14] = graphDecode0;
  graph_decode_line_tab[15] = graphDecode0;
  graph_decode_line_dual_tab[0] = graphDecode0;
  graph_decode_line_dual_tab[1] = graphDecode1;
  graph_decode_line_dual_tab[2] = graphDecode2Dual;
  graph_decode_line_dual_tab[3] = graphDecode3Dual;
  graph_decode_line_dual_tab[4] = graphDecode4Dual;
  graph_decode_line_dual_tab[5] = graphDecode5Dual;
  graph_decode_line_dual_tab[6] = graphDecode6Dual;
  graph_decode_line_dual_tab[7] = graphDecode0;
  graph_decode_line_dual_tab[8] = graphDecode0;
  graph_decode_line_dual_tab[9] = graphDecode1;
  graph_decode_line_dual_tab[10] = graphDecode2Dual;
  graph_decode_line_dual_tab[11] = graphDecode3Dual;
  graph_decode_line_dual_tab[12] = graphDecode4Dual;
  graph_decode_line_dual_tab[13] = graphDecode0;
  graph_decode_line_dual_tab[14] = graphDecode0;
  graph_decode_line_dual_tab[15] = graphDecode0;
  graph_decode_line_ptr = graphDecode0;
}

/*===========================================================================*/
/* End of line handler for graphics                                          */
/*===========================================================================*/

void graphEndOfLine(void)
{
  graph_line* current_graph_line;

  // skip this frame?
  if (draw_frame_skip == 0) 
  {
    ULO currentY = busGetRasterY();
    // update diw state
    graphPlayfieldOnOff();

    // skip lines before line $12
    if (currentY >= 0x12)
    {
      // make pointer to linedesc for this line
      current_graph_line = &graph_frame[draw_buffer_draw][currentY];

      // decode sprites if DMA is enabled and raster is after line $18
      if ((dmacon & 0x20) == 0x20)
      {
	if (currentY >= 0x18) 
	{
	  spritesDMASpriteHandler();
	  spriteProcessActionList();
	}
	else
	{
	  //spriteProcessActionList();
	}
      }

      // sprites decoded, sprites_onlineflag is set if there are any
      // update pointer to drawing routine
      drawUpdateDrawmode();

      // check if we are clipped
      if ((currentY >= draw_top) || (currentY < draw_bottom))
      {

	// visible line, either background or bitplanes
	if (graph_allow_bpl_line_skip == 0) 
	{
	  graphComposeLineOutput(current_graph_line);
	}
	else
	{
	  graphComposeLineOutputSmart(current_graph_line);
	}
      }
      else
      {
	// do nop decoding, no screen output needed
	if (draw_line_BG_routine != draw_line_routine)
	{
	  graphDecodeNOP();
	}
      }

      // if diw state changing from background to bitplane output,
      // set new drawing routine pointer

      if (draw_switch_bg_to_bpl != 0) 
      {
	draw_line_BPL_manage_routine = draw_line_routine;
	draw_switch_bg_to_bpl = 0;
      }
    }
  }
}
