/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Blitter Initialization                                                     */
/*                                                                            */
/* Author: Petter Schau (peschau@online.no)                                   */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

#include "portable.h"
#include "renaming.h"

#include "defs.h"
#include "fellow.h"
#include "blit.h"
#include "fmem.h"
#include "graph.h"
#include "draw.h"
#include "cpu.h"


/*============================================================================*/
/* Blitter registers                                                          */
/*============================================================================*/

ULO bltcon, bltafwm, bltalwm, bltapt, bltbpt, bltcpt, bltdpt, bltamod, bltbmod;
ULO bltcmod, bltdmod, bltadat, bltbdat, bltcdat, bltzero;


/*============================================================================*/
/* Blitter cycle usage table                                                  */
/*============================================================================*/

ULO blit_cycletab[16] = {4, 4, 4, 5, 5, 5, 5, 6, 4, 4, 4, 5, 5, 5, 5, 6};


/*============================================================================*/
/* Holds previous word                                                        */
/*============================================================================*/

ULO blit_leftoverA, blit_leftoverB;


/*============================================================================*/
/* Callback table for minterm-calculation functions                           */
/*============================================================================*/

blit_min_func blit_min_functable[256];
blit_min_func bltminterm;


/*============================================================================*/
/* Blitter fill-mode lookup tables                                            */
/*============================================================================*/

UWO fillincfc0[65536], fillincfc1[65536];
UBY fillincfc0after[65536], fillincfc1after[65536];
UWO fillexcfc0[65536], fillexcfc1[65536];
UBY fillexcfc0after[65536], fillexcfc1after[65536];


/*============================================================================*/
/* Various blitter variables                                                  */
/*============================================================================*/

ULO bltdesc, blitend, blitterstatus;
ULO bltadatoriginal, bltbdatoriginal;
ULO bltbdatline, bltlineheight, bltlinepointflag, bltfillbltconsave;
ULO bltlinedecision;
ULO linenum,linecount,linelength,blitterdmawaiting;
BOOLE blitter_operation_log, blitter_operation_log_first;


/*============================================================================*/
/* Function tables for different types of blitter emulation                   */
/*============================================================================*/

blitmodefunc blittermodes[16] = {blitterline, Dblitterline, blitterline,
				 blitterline, blitterline, blitterline,
				 blitterline, blitterline, blitterline,
				 ADblitterline, blitterline, blitterline,
				 blitterline, blitterline, blitterline,
				 blitterline};
				 
blitmodefunc blitterfillmodes[16] = {blitterfillline, blitterfillline,
				     blitterfillline, blitterfillline,
				     blitterfillline, blitterfillline,
				     blitterfillline, blitterfillline,
				     blitterfillline, ADblitterfillline,
				     blitterfillline, blitterfillline,
				     blitterfillline, blitterfillline,
				     blitterfillline, blitterfillline};

blitmodefunc bltlinesudsulaul[8] = {blitterlinemodeoctant0,
				    blitterlinemodeoctant1,
				    blitterlinemodeoctant2,
				    blitterlinemodeoctant3,
				    blitterlinemodeoctant4,
				    blitterlinemodeoctant5,
				    blitterlinemodeoctant6,
				    blitterlinemodeoctant7};


/*===========================================================================*/
/* Blitter properties                                                        */
/*===========================================================================*/

BOOLE blitter_fast;   /* Blitter finishes in zero time */
BOOLE blitter_ECS;    /* Enable long blits */

void blitterSetFast(BOOLE fast) {
  blitter_fast = fast;
}

BOOLE blitterGetFast(void) {
  return blitter_fast;
}

void blitterSetECS(BOOLE ECS) {
  blitter_ECS = ECS;
}

BOOLE blitterGetECS(void) {
  return blitter_ECS;
}

/*============================================================================*/
/* Blitter operation log                                                      */
/*============================================================================*/

void blitterSetOperationLog(BOOLE operation_log) {
  blitter_operation_log = operation_log;
}

BOOLE blitterGetOperationLog(void) {
  return blitter_operation_log;
}

void blitterOperationLog(void) {
  if (blitter_operation_log) {
    FILE *F = fopen("blitterops.log", (blitter_operation_log_first) ? "w" : "a");
    if (blitter_operation_log_first) {
      blitter_operation_log_first = FALSE;
      fprintf(F, "FRAME\tY\tX\tPC\tBLTCON0\tBLTCON1\tBLTAFWM\tBLTALWM\tBLTAPT\tBLTBPT\tBLTCPT\tBLTDPT\tBLTAMOD\tBLTBMOD\tBLTCMOD\tBLTDMOD\tBLTADAT\tBLTBDAT\tBLTCDAT\tHEIGHT\tWIDTH\n");
    }
    if (F) {
      fprintf(F, "%.7d\t%.3d\t%.3d\t%.6X\t%.4X\t%.4X\t%.4X\t%.4X\t%.6X\t%.6X\t%.6X\t%.6X\t%.4X\t%.4X\t%.4X\t%.4X\t%.4X\t%.4X\t%.4X\t%d\t%d\n",
	      draw_frame_count, graph_raster_y, graph_raster_x, cpuGetPC(pc), (bltcon >> 16) & 0xffff, bltcon & 0xffff, bltafwm & 0xffff, bltalwm & 0xffff, bltapt, bltbpt, bltcpt, bltdpt, bltamod & 0xffff, bltbmod & 0xffff, bltcmod & 0xffff, bltdmod & 0xffff, bltadat & 0xffff, bltbdat & 0xffff, bltcdat & 0xffff, linenum, linelength);
      fclose(F);
    }
  }
}


/*============================================================================*/
/* Fill-table init                                                            */
/*============================================================================*/

static void blitterFillTableInit(void) {
  int i,j,k,l;
/* inclusive fill, fc = 0 at start of word */
  for (i = 0; i < 65536; i++) {
    l = 0;
    j = i;
    for (k = 0; k < 16; k++) {
      j |= l<<k;                 /* Set fill for this bit */
/* Check if fill status has changed */
      if ((i & (0x1<<k))) {
        if (l == 1) l = 0; else l = 1;
        }
      }
    fillincfc0[i] = j;
    fillincfc0after[i] = l;
    }
/* inclusive fill, fc = 1 at start of word */
  for (i = 0; i < 65536; i++) {
    l = 1;
    j = i;
    for (k = 0; k < 16; k++) {
      j |= l<<k;                 /* Set fill for this bit */
/* Check if fill status has changed */
      if ((i & (0x1<<k))) {
        if (l == 1) l = 0; else l = 1;
        }
      }
    fillincfc1[i] = j;
    fillincfc1after[i] = l;
    }
/* exclusive fill, fc = 0 at start of word */
  for (i = 0; i < 65536; i++) {
    l = 0;
    j = i;
    for (k = 0; k < 16; k++) {
      j ^= l<<k;                 /* Set fill for this bit */
/* Check if fill status has changed */
      if ((i & (0x1<<k))) {
        if (l == 1) l = 0; else l = 1;
        }
      }
    fillexcfc0[i] = j;
    fillexcfc0after[i] = l;
    }

/* exclusive fill, fc = 1 at start of word */
  for (i = 0; i < 65536; i++) {
    l = 1;
    j = i;
    for (k = 0; k < 16; k++) {
      j ^= l<<k;                 /* Set fill for this bit */
/* Check if fill status has changed */
      if ((i & (0x1<<k))) {
        if (l == 1) l = 0; else l = 1;
        }
      }
    fillexcfc1[i] = j;
    fillexcfc1after[i] = l;
    }
}


/*============================================================================*/
/* Minterm calculation callback table initialization                          */
/*============================================================================*/

static void blitterMinTableInit(void) {
  ULO i;

  for (i = 0; i < 256; i++)
    blit_min_functable[i] = blit_min_generic;
  blit_min_functable[0x00] = blit_min_00;
  blit_min_functable[0x01] = blit_min_01;
  blit_min_functable[0x02] = blit_min_02;
  blit_min_functable[0x03] = blit_min_03;
  blit_min_functable[0x04] = blit_min_04;
  blit_min_functable[0x05] = blit_min_05;
  blit_min_functable[0x06] = blit_min_06;
  blit_min_functable[0x07] = blit_min_07;
  blit_min_functable[0x08] = blit_min_08;
  blit_min_functable[0x09] = blit_min_09;
  blit_min_functable[0x0a] = blit_min_0a;
  blit_min_functable[0x0b] = blit_min_0b;
  blit_min_functable[0x0c] = blit_min_0c;
  blit_min_functable[0x0d] = blit_min_0d;
  blit_min_functable[0x0e] = blit_min_0e;
  blit_min_functable[0x0f] = blit_min_0f;
  blit_min_functable[0x2a] = blit_min_2a;
  blit_min_functable[0x4a] = blit_min_4a;
  blit_min_functable[0xca] = blit_min_ca;
  blit_min_functable[0xd8] = blit_min_d8;
  blit_min_functable[0xea] = blit_min_ea;
  blit_min_functable[0xf0] = blit_min_f0;
  blit_min_functable[0xfa] = blit_min_fa;
  blit_min_functable[0xfc] = blit_min_fc;
  blit_min_functable[0xff] = blit_min_ff;
}  


/*============================================================================*/
/* Set blitter IO stubs in IO read/write table                                */
/*============================================================================*/

static void blitterIOHandlersInstall(void) {
  memorySetIOWriteStub(0x40, wbltcon0);
  memorySetIOWriteStub(0x42, wbltcon1);
  memorySetIOWriteStub(0x44, wbltafwm);
  memorySetIOWriteStub(0x46, wbltalwm);
  memorySetIOWriteStub(0x48, wbltcpth);
  memorySetIOWriteStub(0x4a, wbltcptl);
  memorySetIOWriteStub(0x4c, wbltbpth);
  memorySetIOWriteStub(0x4e, wbltbptl);
  memorySetIOWriteStub(0x50, wbltapth);
  memorySetIOWriteStub(0x52, wbltaptl);
  memorySetIOWriteStub(0x54, wbltdpth);
  memorySetIOWriteStub(0x56, wbltdptl);
  memorySetIOWriteStub(0x58, wbltsize);
  memorySetIOWriteStub(0x60, wbltcmod);
  memorySetIOWriteStub(0x62, wbltbmod);
  memorySetIOWriteStub(0x64, wbltamod);
  memorySetIOWriteStub(0x66, wbltdmod);
  memorySetIOWriteStub(0x70, wbltcdat);
  memorySetIOWriteStub(0x72, wbltbdat);
  memorySetIOWriteStub(0x74, wbltadat);
  if (blitterGetECS()) {
    memorySetIOWriteStub(0x5a, wbltcon0l);
    memorySetIOWriteStub(0x5c, wbltsizv);
    memorySetIOWriteStub(0x5e, wbltsizh);
  }    
}


/*============================================================================*/
/* Set blitter to default values                                              */
/*============================================================================*/

static void blitterIORegistersClear(void) {
  bltminterm = blit_min_generic;
  blitend = 0xffffffff;             /* Must keep blitend -1 when not blitting */
  blitterstatus = 0;
  bltapt = 0;
  bltbpt = 0;
  bltcpt = 0;
  bltdpt = 0;
  bltcon = 0;
  bltafwm = bltalwm = 0;
  bltamod = 0;
  bltbmod = 0;
  bltcmod = 0;
  bltdmod = 0;
  bltadat = 0;
  bltbdat = 0;
  bltcdat = 0;
  bltzero = 0;
  blit_leftoverA = blit_leftoverB = 0;
  bltadatoriginal = 0;
  bltbdatoriginal = 0;
  bltdesc = 0;
  bltbdatline = 0;
  bltlineheight = 0;
  bltlinepointflag = 0;
  bltfillbltconsave = 0;
  linenum = 0;
  linecount = 0;
  linelength = 0;
  blitterdmawaiting = 0;
  bltlinedecision = 0;
}


/*============================================================================*/
/* Reset blitter to default values                                            */
/*============================================================================*/

void blitterHardReset(void) {
  blitterIORegistersClear();
}


/*===========================================================================*/
/* Called on emulator start / stop                                           */
/*===========================================================================*/

void blitterEmulationStart(void) {
  blitterIOHandlersInstall();
}

void blitterEmulationStop(void) {
}


/*===========================================================================*/
/* Called on emulator startup / shutdown                                     */
/*===========================================================================*/

void blitterStartup(void) {
  blitterMinTableInit();
  blitterFillTableInit();
  blitterSetFast(FALSE);
  blitterSetECS(FALSE);
  blitterIORegistersClear();
  blitterSetOperationLog(FALSE);
  blitter_operation_log_first = TRUE;
}

void blitterShutdown(void) {
}
