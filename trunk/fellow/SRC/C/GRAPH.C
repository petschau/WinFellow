/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Convert Amiga graphics data to temporary format suitable for fast          */
/* drawing on a chunky display.                                               */
/*                                                                            */
/* Author: Petter Schau (peschau@online.no)                                   */
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
planar2chunkyroutine graph_decode_nop_table[8] = {graphDecodeNOP0,
						  graphDecodeNOP1,
						  graphDecodeNOP2,
						  graphDecodeNOP3,
						  graphDecodeNOP4,
						  graphDecodeNOP5,
						  graphDecodeNOP6,
						  graphDecodeNOP6};


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


/*===========================================================================*/
/* Line description registers and data                                       */
/*===========================================================================*/

ULO graph_DDF_start;
ULO graph_DDF_word_count;
ULO graph_DIW_first_visible;
ULO graph_DIW_last_visible;


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
  graph_decode_line_tab[0] = graphDecode0;
  graph_decode_line_tab[1] = graphDecode1;
  graph_decode_line_tab[2] = graphDecode2;
  graph_decode_line_tab[3] = graphDecode3;
  graph_decode_line_tab[4] = graphDecode4;
  graph_decode_line_tab[5] = graphDecode5;
  graph_decode_line_tab[6] = graphDecode6;
  graph_decode_line_tab[7] = graphDecode0;
  graph_decode_line_tab[8] = graphDecode0;
  graph_decode_line_tab[9] = graphDecode1Hi320;
  graph_decode_line_tab[10] = graphDecode2Hi320;
  graph_decode_line_tab[11] = graphDecode3Hi320;
  graph_decode_line_tab[12] = graphDecode4Hi320;
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
  graph_decode_line_dual_tab[9] = graphDecode1Hi320;
  graph_decode_line_dual_tab[10] = graphDecode2HiDual320;
  graph_decode_line_dual_tab[11] = graphDecode3HiDual320;
  graph_decode_line_dual_tab[12] = graphDecode4HiDual320;
  graph_decode_line_dual_tab[13] = graphDecode0;
  graph_decode_line_dual_tab[14] = graphDecode0;
  graph_decode_line_dual_tab[15] = graphDecode0;
  graph_decode_line_ptr = graphDecode0;
}


/*===========================================================================*/
/* Decode tables for hires size tables                                       */
/* Called from the draw module                                               */
/*===========================================================================*/

void graphP2C2XInit(void) {
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
      graph_frame[frame][line].frames_left_until_BG_skip = 1;
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
}


/*===========================================================================*/
/* Release resources taken by the graphics module                            */
/*===========================================================================*/

void graphShutdown(void) {
}
