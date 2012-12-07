/* @(#) $Id: BLIT.C,v 1.19 2012-12-07 14:05:43 carfesh Exp $ */
/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Blitter Initialization                                                     */
/*                                                                            */
/* Authors: Petter Schau                                                      */
/*          Worfje                                                            */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

#include "defs.h"
#include "fellow.h"
#include "blit.h"
#include "fmem.h"
#include "graph.h"
#include "draw.h"
#include "bus.h"
#include "fileops.h"
#include "CpuIntegration.h"

//#define BLIT_VERIFY_MINTERMS
//#define BLIT_OPERATION_LOG

/*============================================================================*/
/* Blitter registers                                                          */
/*============================================================================*/

typedef struct blitter_state_
{
  // Actual registers
  ULO bltcon;
  ULO bltafwm;
  ULO bltalwm;
  ULO bltapt;
  ULO bltbpt;
  ULO bltcpt;
  ULO bltdpt;
  ULO bltamod;
  ULO bltbmod;
  ULO bltcmod;
  ULO bltdmod;
  ULO bltadat;
  ULO bltbdat;
  ULO bltbdat_original;
  ULO bltcdat;
  ULO bltzero;

  // Calculated by the set-functions based on the state above
  ULO height;
  ULO width;

  // Line mode alters these and the state remains after the line is done.
  // ie. the next line continues where the last left off if the shifts are not re-initialised
  // by Amiga code.
  ULO a_shift_asc;
  ULO a_shift_desc;
  ULO b_shift_asc;
  ULO b_shift_desc;

  // Information about an ongoing blit
  // dma_pending is a flag showing that a blit has been activated (a write to BLTSIZE)
  // but at the time of activation the blit DMA was turned off
  BOOLE started;
  BOOLE dma_pending; 
  ULO cycle_length; // Estimate for how many cycles the started blit will take
  ULO cycle_free;   // How many of these cycles are free to use by the CPU

} blitter_state;

blitter_state blitter;

/*============================================================================*/
/* Blitter cycle usage table                                                  */
/* Unit is bus cycles (3.58MHz)						      */
/*============================================================================*/

ULO blit_cyclelength[16] = {2, 2, 2, 3, /* How long it takes for a blit to complete */
3, 3, 3, 4, 
2, 2, 2, 3, 
3, 3, 3, 4};
ULO blit_cyclefree[16] = {2, 1, 1, 1, /* Free cycles during blit */
2, 1, 1, 1, 
1, 0, 0, 0, 
1, 0, 0, 0};

/*============================================================================*/
/* Blitter fill-mode lookup tables                                            */
/*============================================================================*/

UBY blit_fill[2][2][256][2];/* [inc,exc][fc][data][0 = next fc, 1 = filled data] */

#ifdef BLIT_VERIFY_MINTERMS
BOOLE blit_minterm_seen[256];
#endif

/*===========================================================================*/
/* Blitter properties                                                        */
/*===========================================================================*/

BOOLE blitter_fast;   /* Blitter finishes in zero time */
BOOLE blitter_ECS;    /* Enable long blits */

void blitterSetFast(BOOLE fast)
{
  blitter_fast = fast;
}

BOOLE blitterGetFast(void)
{
  return blitter_fast;
}

void blitterSetECS(BOOLE ECS)
{
  blitter_ECS = ECS;
}

BOOLE blitterGetECS(void)
{
  return blitter_ECS;
}

BOOLE blitterGetDMAPending(void)
{
  return blitter.dma_pending;
}

BOOLE blitterGetZeroFlag(void)
{
  return (blitter.bltzero & 0xffff) == 0;
}

ULO blitterGetFreeCycles(void)
{
  return blitter.cycle_free;
}

BOOLE blitterIsStarted(void)
{
  return blitter.started;
}

BOOLE blitterIsDescending(void)
{
  return (blitter.bltcon & 2);
}

/*============================================================================*/
/* Blitter event setup                                                        */
/*============================================================================*/

void blitterRemoveEvent(void)
{
  if (blitterEvent.cycle != BUS_CYCLE_DISABLE)
  {
    busRemoveEvent(&blitterEvent);
    blitterEvent.cycle = BUS_CYCLE_DISABLE;
  }
}

void blitterInsertEvent(ULO cycle)
{
  if (cycle != BUS_CYCLE_DISABLE)
  {
    blitterEvent.cycle = cycle;
    busInsertEvent(&blitterEvent);
  }
}

/*============================================================================*/
/* Blitter operation log                                                      */
/*============================================================================*/

#ifdef BLIT_OPERATION_LOG

BOOLE blitter_operation_log;
BOOLE blitter_operation_log_first;

void blitterSetOperationLog(BOOLE operation_log)
{
  blitter_operation_log = operation_log;
}

BOOLE blitterGetOperationLog(void)
{
  return blitter_operation_log;
}

void blitterOperationLog(void) {
  if (blitter_operation_log) {
    FILE *F;
    char filename[MAX_PATH];

    fileopsGetGenericFileName(filename, "WinFellow", "blitterops.log");
    F = fopen(filename, (blitter_operation_log_first) ? "w" : "a");
    if (blitter_operation_log_first) {
      blitter_operation_log_first = FALSE;
      fprintf(F, "FRAME\tY\tX\tPC\tBLTCON0\tBLTCON1\tBLTAFWM\tBLTALWM\tBLTAPT\tBLTBPT\tBLTCPT\tBLTDPT\tBLTAMOD\tBLTBMOD\tBLTCMOD\tBLTDMOD\tBLTADAT\tBLTBDAT\tBLTCDAT\tHEIGHT\tWIDTH\n");
    }
    if (F)
    {
      fprintf(F, "%.7d\t%.3d\t%.3d\t%.4X\t%.4X\t%.4X\t%.4X\t%.6X\t%.6X\t%.6X\t%.6X\t%.4X\t%.4X\t%.4X\t%.4X\t%.4X\t%.4X\t%.4X\t%d\t%d\n",
	draw_frame_count, busGetRasterY(), busGetRasterX(), (blitter.bltcon >> 16) & 0xffff, blitter.bltcon & 0xffff, blitter.bltafwm & 0xffff, blitter.bltalwm & 0xffff, blitter.bltapt, blitter.bltbpt, blitter.bltcpt, blitter.bltdpt, blitter.bltamod & 0xffff, blitter.bltbmod & 0xffff, blitter.bltcmod & 0xffff, blitter.bltdmod & 0xffff, blitter.bltadat & 0xffff, blitter.bltbdat & 0xffff, blitter.bltcdat & 0xffff, blitter.height, blitter.width);
      fclose(F);
    }
  }
}

#endif

#define blitterMinterm00(a_dat, b_dat, c_dat, d_dat) d_dat = (0);                                   /* 0 */
#define blitterMinterm01(a_dat, b_dat, c_dat, d_dat) d_dat = (~(a_dat | b_dat | c_dat));            /* !(A+B+C) */
#define blitterMinterm02(a_dat, b_dat, c_dat, d_dat) d_dat = (~a_dat & ~b_dat & c_dat);             /* abC */
#define blitterMinterm03(a_dat, b_dat, c_dat, d_dat) d_dat = (~(a_dat | b_dat));                    /* !(A+B) */
#define blitterMinterm04(a_dat, b_dat, c_dat, d_dat) d_dat = (~a_dat & b_dat & ~c_dat);             /* aBc */
#define blitterMinterm05(a_dat, b_dat, c_dat, d_dat) d_dat = (~(a_dat | c_dat));                    /* !(A+C) */
#define blitterMinterm06(a_dat, b_dat, c_dat, d_dat) d_dat = (~a_dat & (b_dat ^ c_dat));            /* a(B xor C) */
#define blitterMinterm07(a_dat, b_dat, c_dat, d_dat) d_dat = (~(a_dat | (b_dat & c_dat)));          /* !(A+BC) */
#define blitterMinterm08(a_dat, b_dat, c_dat, d_dat) d_dat = (~a_dat & b_dat & c_dat);              /* aBC */
#define blitterMinterm09(a_dat, b_dat, c_dat, d_dat) d_dat = (~(a_dat | (b_dat ^ c_dat)));          /* !(A+(B xor C)) */
#define blitterMinterm0a(a_dat, b_dat, c_dat, d_dat) d_dat = (~a_dat & c_dat);                      /* aC */
#define blitterMinterm0b(a_dat, b_dat, c_dat, d_dat) d_dat = (~a_dat & (~b_dat | c_dat));           /* a(b+C) */
#define blitterMinterm0c(a_dat, b_dat, c_dat, d_dat) d_dat = (~a_dat & b_dat);                      /* aB */
#define blitterMinterm0d(a_dat, b_dat, c_dat, d_dat) d_dat = (~(a_dat | (~b_dat & c_dat)));         /* !(A+(bC)) */
#define blitterMinterm0e(a_dat, b_dat, c_dat, d_dat) d_dat = (~a_dat & (b_dat | c_dat));            /* a(B+C) */
#define blitterMinterm0f(a_dat, b_dat, c_dat, d_dat) d_dat = (~a_dat);                              /* a */
#define blitterMinterm10(a_dat, b_dat, c_dat, d_dat) d_dat = (~b_dat & a_dat & ~c_dat);             /* Abc */
#define blitterMinterm11(a_dat, b_dat, c_dat, d_dat) d_dat = (~(b_dat | c_dat));                    /* !(B+C) */
#define blitterMinterm12(a_dat, b_dat, c_dat, d_dat) d_dat = (~b_dat & (a_dat ^ c_dat));            /* b(A xor C) */
#define blitterMinterm13(a_dat, b_dat, c_dat, d_dat) d_dat = (~(b_dat | (a_dat & c_dat)));          /* !(B+AC) */
#define blitterMinterm16(a_dat, b_dat, c_dat, d_dat) d_dat = ((~a_dat & ~b_dat & c_dat) | ((a_dat ^ b_dat) & ~c_dat));          /* c(A xor B) + abC */
#define blitterMinterm1a(a_dat, b_dat, c_dat, d_dat) d_dat = ((a_dat & ~b_dat & ~c_dat) | (~a_dat & c_dat)); /* Abc + aC */
#define blitterMinterm1f(a_dat, b_dat, c_dat, d_dat) d_dat = (~a_dat | (a_dat & ~b_dat & ~c_dat));  /* Abc + a */
#define blitterMinterm20(a_dat, b_dat, c_dat, d_dat) d_dat = (~b_dat & a_dat & c_dat);              /* bAC */
#define blitterMinterm21(a_dat, b_dat, c_dat, d_dat) d_dat = (~(b_dat | (a_dat ^ c_dat)));          /* !(B+(A xor C)) */
#define blitterMinterm22(a_dat, b_dat, c_dat, d_dat) d_dat = (~b_dat & c_dat);                      /* bC */
#define blitterMinterm23(a_dat, b_dat, c_dat, d_dat) d_dat = (~b_dat & (~a_dat | c_dat));           /* b(a+C) */
#define blitterMinterm2a(a_dat, b_dat, c_dat, d_dat) d_dat = (~(a_dat & b_dat) & c_dat);            /* (!AB)C */
#define blitterMinterm30(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat & ~b_dat);                      /* Ab */
#define blitterMinterm31(a_dat, b_dat, c_dat, d_dat) d_dat = (~(b_dat | (~a_dat & c_dat)));         /* !(B+(aC)) */
#define blitterMinterm32(a_dat, b_dat, c_dat, d_dat) d_dat = (~b_dat & (a_dat | c_dat));            /* b(A+C) */
#define blitterMinterm33(a_dat, b_dat, c_dat, d_dat) d_dat = (~b_dat);                              /* b */
#define blitterMinterm3a(a_dat, b_dat, c_dat, d_dat) d_dat = ((a_dat & ~b_dat) | (~a_dat & c_dat)); /* Ab + aC */
#define blitterMinterm3c(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat ^ b_dat);                       /* A xor B */
#define blitterMinterm40(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat & b_dat & ~c_dat);              /* ABc */
#define blitterMinterm42(a_dat, b_dat, c_dat, d_dat) d_dat = ((a_dat & b_dat & ~c_dat) | (~a_dat & ~b_dat & c_dat)); /* ABc + abC */
#define blitterMinterm4a(a_dat, b_dat, c_dat, d_dat) d_dat = ((~a_dat & c_dat) | (a_dat & b_dat & ~c_dat)); /* aC + ABc */
#define blitterMinterm54(a_dat, b_dat, c_dat, d_dat) d_dat = ((a_dat | (b_dat & ~a_dat)) & ~c_dat);	/* c(A + aB) */
#define blitterMinterm5a(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat ^ c_dat);						/* A xor C */
#define blitterMinterm6a(a_dat, b_dat, c_dat, d_dat) d_dat = ((c_dat & ~b_dat) | (b_dat & (a_dat ^ c_dat))); /* bC + B(A xor C) */
#define blitterMinterm80(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat & b_dat & c_dat);				/* ABC */
#define blitterMinterm88(a_dat, b_dat, c_dat, d_dat) d_dat = (b_dat & c_dat);			    /* BC */
#define blitterMinterm8a(a_dat, b_dat, c_dat, d_dat) d_dat = (c_dat & (b_dat | ~a_dat));	    /* C(B+!A)) */
#define blitterMinterm96(a_dat, b_dat, c_dat, d_dat) d_dat = ((a_dat & ~(b_dat ^ c_dat)) | ((b_dat ^ c_dat) & ~a_dat));	/* A(~(B xor C)) + a(B xor C) */
#define blitterMinterm9a(a_dat, b_dat, c_dat, d_dat) d_dat = ((b_dat & c_dat) | ((a_dat ^ c_dat) & ~b_dat));	    /* BC + b(A xor C)) */
#define blitterMinterma0(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat & c_dat);			    /* AC */
#define blitterMinterma8(a_dat, b_dat, c_dat, d_dat) d_dat = (((a_dat & ~b_dat) | b_dat) & c_dat);  /* (Ab + B)C */
#define blitterMintermaa(a_dat, b_dat, c_dat, d_dat) d_dat = (c_dat);                               /* C */
#define blitterMintermac(a_dat, b_dat, c_dat, d_dat) d_dat = ((a_dat & c_dat) | (~a_dat & b_dat));  /* AC + aB */
#define blitterMintermb8(a_dat, b_dat, c_dat, d_dat) d_dat = ((b_dat & c_dat) | (a_dat & ~b_dat));  /* BC + Ab */
#define blitterMintermc0(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat & b_dat);                       /* AB */
#define blitterMintermca(a_dat, b_dat, c_dat, d_dat) d_dat = ((a_dat & b_dat) | (~a_dat & c_dat));  /* AB + aC */
#define blitterMintermcc(a_dat, b_dat, c_dat, d_dat) d_dat = (b_dat);                               /* B */
#define blitterMintermcd(a_dat, b_dat, c_dat, d_dat) d_dat = (b_dat | (~a_dat & ~c_dat));           /* B + ac */
#define blitterMintermce(a_dat, b_dat, c_dat, d_dat) d_dat = (b_dat | (~a_dat & c_dat));            /* B + aC */
#define blitterMintermcf(a_dat, b_dat, c_dat, d_dat) d_dat = (b_dat | ~a_dat);                      /* B + a */
#define blitterMintermd8(a_dat, b_dat, c_dat, d_dat) d_dat = ((a_dat & ~c_dat) | (b_dat & c_dat));  /* Ac + BC */
#define blitterMintermdc(a_dat, b_dat, c_dat, d_dat) d_dat = (b_dat | (a_dat & ~c_dat));            /* B + Ac */
#define blitterMintermdd(a_dat, b_dat, c_dat, d_dat) d_dat = (b_dat | ~c_dat);                      /* B + c */
#define blitterMintermde(a_dat, b_dat, c_dat, d_dat) d_dat = (b_dat | (a_dat ^ c_dat));             /* B + (A xor C) */
#define blitterMintermdf(a_dat, b_dat, c_dat, d_dat) d_dat = (b_dat | ~a_dat | ~c_dat);             /* B + a + c */
#define blitterMinterme2(a_dat, b_dat, c_dat, d_dat) d_dat = ((a_dat & (b_dat | c_dat)) | (~a_dat & ~b_dat & c_dat));  /* A(B+C) + abC */
#define blitterMinterme8(a_dat, b_dat, c_dat, d_dat) d_dat = ((a_dat & b_dat) | (c_dat & (a_dat ^ b_dat))); /* AB + C(A xor B) */
#define blitterMintermea(a_dat, b_dat, c_dat, d_dat) d_dat = ((~a_dat & c_dat) | a_dat & (b_dat | c_dat));  /* aC + A(B+C) */
#define blitterMintermec(a_dat, b_dat, c_dat, d_dat) d_dat = (b_dat | (a_dat & c_dat));             /* B + AC */
#define blitterMintermed(a_dat, b_dat, c_dat, d_dat) d_dat = (b_dat | ~(a_dat ^ c_dat));            /* B + ~(A xor C) */
#define blitterMintermee(a_dat, b_dat, c_dat, d_dat) d_dat = (b_dat | c_dat);                       /* B + C */
#define blitterMintermef(a_dat, b_dat, c_dat, d_dat) d_dat = (b_dat | ~a_dat | c_dat);              /* B + a + C */
#define blitterMintermf0(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat);                               /* A */
#define blitterMintermf1(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat | (~b_dat & ~c_dat));           /* A + bc */
#define blitterMintermf2(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat | (~b_dat & c_dat));            /* A + bC */
#define blitterMintermf3(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat | ~b_dat);                      /* A + b */
#define blitterMintermf4(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat | (b_dat & ~c_dat));            /* A + Bc */
#define blitterMintermf5(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat | ~c_dat);                      /* A + c */
#define blitterMintermf6(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat | (b_dat ^ c_dat));             /* A + (B xor C) */
#define blitterMintermf7(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat | ~b_dat | ~c_dat);             /* A + b + c */
#define blitterMintermf8(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat | (b_dat & c_dat));             /* A + BC */
#define blitterMintermf9(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat | ~(b_dat ^ c_dat));            /* A + ~(B xor C) */
#define blitterMintermfa(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat | c_dat);                       /* A + C */
#define blitterMintermfb(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat | ~b_dat | c_dat);              /* A + b + C */
#define blitterMintermfc(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat | b_dat);                       /* A + B */
#define blitterMintermfd(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat | b_dat | ~c_dat);              /* A + B + c */
#define blitterMintermfe(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat | b_dat | c_dat);               /* A + B + C */
#define blitterMintermff(a_dat, b_dat, c_dat, d_dat) d_dat = (0xffffffff);                          /* All 1 */

#ifdef BLIT_VERIFY_MINTERMS
#define blitterSetMintermSeen(minterm) blit_minterm_seen[mins] = TRUE
#else
#define blitterSetMintermSeen(minterm)
#endif

#define blitterMintermGeneric(a_dat, b_dat, c_dat, d_dat, mins) \
  blitterSetMintermSeen(mins); \
  d_dat = 0; \
  if (mins & 0x80) d_dat |= (a_dat & b_dat & c_dat); \
  if (mins & 0x40) d_dat |= (a_dat & b_dat & ~c_dat); \
  if (mins & 0x20) d_dat |= (a_dat & ~b_dat & c_dat); \
  if (mins & 0x10) d_dat |= (a_dat & ~b_dat & ~c_dat); \
  if (mins & 0x08) d_dat |= (~a_dat & b_dat & c_dat); \
  if (mins & 0x04) d_dat |= (~a_dat & b_dat & ~c_dat); \
  if (mins & 0x02) d_dat |= (~a_dat & ~b_dat & c_dat); \
  if (mins & 0x01) d_dat |= (~a_dat & ~b_dat & ~c_dat);

#define blitterMinterms(a_dat, b_dat, c_dat, d_dat, mins) \
  switch (mins) \
{ \
  case 0x00: blitterMinterm00(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x01: blitterMinterm01(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x02: blitterMinterm02(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x03: blitterMinterm03(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x04: blitterMinterm04(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x05: blitterMinterm05(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x06: blitterMinterm06(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x07: blitterMinterm07(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x08: blitterMinterm08(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x09: blitterMinterm09(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x0a: blitterMinterm0a(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x0b: blitterMinterm0b(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x0c: blitterMinterm0c(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x0d: blitterMinterm0d(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x0e: blitterMinterm0e(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x0f: blitterMinterm0f(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x10: blitterMinterm10(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x11: blitterMinterm11(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x12: blitterMinterm12(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x13: blitterMinterm13(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x16: blitterMinterm16(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x1a: blitterMinterm1a(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x1f: blitterMinterm1f(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x20: blitterMinterm20(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x21: blitterMinterm21(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x22: blitterMinterm22(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x23: blitterMinterm23(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x2a: blitterMinterm2a(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x30: blitterMinterm30(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x31: blitterMinterm31(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x32: blitterMinterm32(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x33: blitterMinterm33(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x3a: blitterMinterm3a(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x3c: blitterMinterm3c(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x40: blitterMinterm40(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x42: blitterMinterm42(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x4a: blitterMinterm4a(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x54: blitterMinterm54(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x5a: blitterMinterm5a(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x6a: blitterMinterm6a(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x80: blitterMinterm80(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x88: blitterMinterm88(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x8a: blitterMinterm8a(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x96: blitterMinterm96(a_dat, b_dat, c_dat, d_dat); break; \
  case 0x9a: blitterMinterm9a(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xa0: blitterMinterma0(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xa8: blitterMinterma8(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xaa: blitterMintermaa(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xac: blitterMintermac(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xb8: blitterMintermb8(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xc0: blitterMintermc0(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xca: blitterMintermca(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xcc: blitterMintermcc(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xcd: blitterMintermcd(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xce: blitterMintermce(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xcf: blitterMintermcf(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xd8: blitterMintermd8(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xdc: blitterMintermdc(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xdd: blitterMintermdd(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xde: blitterMintermde(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xdf: blitterMintermdf(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xe2: blitterMinterme2(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xe8: blitterMinterme8(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xea: blitterMintermea(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xec: blitterMintermec(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xed: blitterMintermed(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xee: blitterMintermee(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xef: blitterMintermef(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xf0: blitterMintermf0(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xf1: blitterMintermf1(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xf2: blitterMintermf2(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xf3: blitterMintermf3(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xf4: blitterMintermf4(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xf5: blitterMintermf5(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xf6: blitterMintermf6(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xf7: blitterMintermf7(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xf8: blitterMintermf8(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xf9: blitterMintermf9(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xfa: blitterMintermfa(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xfb: blitterMintermfb(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xfc: blitterMintermfc(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xfd: blitterMintermfd(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xfe: blitterMintermfe(a_dat, b_dat, c_dat, d_dat); break; \
  case 0xff: blitterMintermff(a_dat, b_dat, c_dat, d_dat); break; \
  default: blitterMintermGeneric(a_dat, b_dat, c_dat, d_dat, mins); break; \
}

#define blitterReadWordEnabled(pt, dat, ascending) \
  dat = (((ULO) memory_chip[pt]) << 8) | (ULO) memory_chip[pt + 1]; \
  if (!ascending) pt = (pt - 2) & 0x1ffffe; \
  if (ascending) pt = (pt + 2) & 0x1ffffe;

#define blitterWriteWordEnabled(pt, pt_tmp, dat, ascending) \
  if (pt_tmp != 0xffffffff) \
  { \
    memory_chip[pt_tmp] = (UBY) (dat >> 8); \
    memory_chip[pt_tmp + 1] = (UBY) dat; \
  } \
  pt_tmp = pt; \
  if (!ascending) pt = (pt - 2) & 0x1ffffe; \
  if (ascending) pt = (pt + 2) & 0x1ffffe;

#define blitterReadWordDisabled(dat, dat_preload) \
  dat = dat_preload;

#define blitterReadWord(pt, dat, dat_preload, enabled, ascending) \
  if (enabled) {blitterReadWordEnabled(pt, dat, ascending);} \
  else {blitterReadWordDisabled(dat, dat_preload);}

#define blitterWriteWord(pt, pt_tmp, dat, enabled, ascending) \
  if (enabled) {blitterWriteWordEnabled(pt, pt_tmp, dat, ascending);}

#define blitterShiftWord(dat, dat_tmp, ascending, shift, prev) \
  if (ascending) dat = ((prev << 16) | dat_tmp) >> shift; \
  else dat = ((dat_tmp << 16) | prev) >> shift; \
  prev = dat_tmp;

#define blitterFLWM(dat, is_flwm, flwm) \
  dat &= flwm[is_flwm];

#define blitterReadA(pt, dat, dat_preload, enabled, ascending, shift, prev, is_fwm, is_lwm, fwm, lwm) \
{ \
  ULO a_tmp; \
  blitterReadWord(pt, a_tmp, dat_preload, enabled, ascending); \
  dat_preload = a_tmp; /* Need to remember unWMed value */ \
  blitterFLWM(a_tmp, is_fwm, fwm); \
  blitterFLWM(a_tmp, is_lwm, lwm); \
  blitterShiftWord(dat, a_tmp, ascending, shift, prev); \
}

#define blitterReadB(pt, dat, dat_preload, enabled, ascending, shift, prev) \
  if (enabled) { \
    blitterReadWord(pt, dat_preload, dat_preload, enabled, ascending); \
    blitterShiftWord(dat, dat_preload, ascending, shift, prev); \
  }

#define blitterReadC(pt, dat, enabled, ascending) \
  if (enabled) blitterReadWord(pt, dat, dat, enabled, ascending);

#define blitterWriteD(pt, pt_tmp, dat, enabled, ascending) \
  blitterWriteWord(pt, pt_tmp, dat, enabled, ascending);

#define blitterMakeZeroFlag(dat, flag) \
  flag |= dat;

#define blitterModulo(pt, modul, enabled) \
  if (enabled) pt = (pt + modul) & 0x1ffffe;

#define blitterFillCarryReload(fill, dst, fc_original) \
  if (fill) dst = fc_original;

#define blitterFill(dat, fill, exclusive, fc) \
{ \
  if (fill) \
  { \
    UBY dat1 = (UBY) dat; \
    UBY dat2 = (UBY) (dat >> 8); \
    ULO fc2 = blit_fill[exclusive][fc][dat1][0]; \
    dat = ((ULO) blit_fill[exclusive][fc][dat1][1]) | (((ULO) blit_fill[exclusive][fc2][dat2][1]) << 8); \
    fc = blit_fill[exclusive][fc2][dat2][0]; \
  } \
}

#define blitterBlit(a_enabled, b_enabled, c_enabled, d_enabled, ascending, fill) \
{ \
  LON x, y; \
  ULO a_pt = blitter.bltapt; \
  ULO b_pt = blitter.bltbpt; \
  ULO c_pt = blitter.bltcpt; \
  ULO d_pt = blitter.bltdpt; \
  ULO d_pt_tmp = 0xffffffff; \
  ULO a_shift = (ascending) ? blitter.a_shift_asc : blitter.a_shift_desc; \
  ULO b_shift = (ascending) ? blitter.b_shift_asc : blitter.b_shift_desc; \
  ULO a_dat, b_dat = (b_enabled) ? 0 : blitter.bltbdat, c_dat = blitter.bltcdat, d_dat; \
  ULO a_dat_preload = blitter.bltadat; \
  ULO b_dat_preload = 0; \
  ULO a_prev = 0; \
  ULO b_prev = 0; \
  ULO a_mod = (ascending) ? blitter.bltamod : ((ULO) - (LON) blitter.bltamod); \
  ULO b_mod = (ascending) ? blitter.bltbmod : ((ULO) - (LON) blitter.bltbmod); \
  ULO c_mod = (ascending) ? blitter.bltcmod : ((ULO) - (LON) blitter.bltcmod); \
  ULO d_mod = (ascending) ? blitter.bltdmod : ((ULO) - (LON) blitter.bltdmod); \
  ULO fwm[2]; \
  ULO lwm[2]; \
  UBY minterms = (UBY) (blitter.bltcon >> 16); \
  ULO fill_exclusive = (blitter.bltcon & 0x8) ? 0 : 1; \
  ULO zero_flag = 0; \
  LON height = blitter.height; \
  LON width = blitter.width; \
  BOOLE fc_original = !!(blitter.bltcon & 0x4); \
  BOOLE fill_carry; \
  fwm[0] = lwm[0] = 0xffff; \
  fwm[1] = blitter.bltafwm; \
  lwm[1] = blitter.bltalwm; \
  for (y = height; y > 0; y--) \
  { \
    blitterFillCarryReload(fill, fill_carry, fc_original); \
    for (x = width; x > 0; x--) \
    { \
      blitterReadA(a_pt, a_dat, a_dat_preload, a_enabled, ascending, a_shift, a_prev, (x == width), (x == 1), fwm, lwm); \
      blitterReadB(b_pt, b_dat, b_dat_preload, b_enabled, ascending, b_shift, b_prev); \
      blitterReadC(c_pt, c_dat, c_enabled, ascending); \
      blitterWriteD(d_pt, d_pt_tmp, d_dat, d_enabled, ascending); \
      blitterMinterms(a_dat, b_dat, c_dat, d_dat, minterms); \
      blitterFill(d_dat, fill, fill_exclusive, fill_carry); \
      blitterMakeZeroFlag(d_dat, zero_flag); \
    } \
    blitterModulo(a_pt, a_mod, a_enabled); \
    blitterModulo(b_pt, b_mod, b_enabled); \
    blitterModulo(c_pt, c_mod, c_enabled); \
    blitterModulo(d_pt, d_mod, d_enabled); \
  } \
  blitterWriteD(d_pt, d_pt_tmp, d_dat, d_enabled, ascending); \
  if (a_enabled) { \
    blitter.bltadat = a_dat_preload; \
    blitter.bltapt = a_pt; \
  } \
  if (b_enabled) { \
    ULO x_tmp = 0; \
    blitterShiftWord(blitter.bltbdat, b_dat_preload, ascending, b_shift, x_tmp); \
    blitter.bltbdat_original = b_dat_preload; \
    blitter.bltbpt = b_pt; \
  } \
  if (c_enabled) { \
    blitter.bltcdat = c_dat; \
    blitter.bltcpt = c_pt; \
  } \
  if (d_enabled) blitter.bltdpt = d_pt_tmp; \
  blitter.bltzero = zero_flag; \
  memoryWriteWord(0x8040, 0xdff09c); \
}

void blitterCopyABCD(void)
{
  if (blitter.bltcon & 0x18)
  { /* Fill */
    if (blitterIsDescending())
    { /* Decending mode */
      switch ((blitter.bltcon >> 24) & 0xf)
      {
      case 0: blitterBlit(FALSE, FALSE, FALSE, FALSE, FALSE, TRUE); break;
      case 1: blitterBlit(FALSE, FALSE, FALSE, TRUE, FALSE, TRUE); break;
      case 2: blitterBlit(FALSE, FALSE, TRUE, FALSE, FALSE, TRUE); break;
      case 3: blitterBlit(FALSE, FALSE, TRUE, TRUE, FALSE, TRUE); break;
      case 4: blitterBlit(FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); break;
      case 5: blitterBlit(FALSE, TRUE, FALSE, TRUE, FALSE, TRUE); break;
      case 6: blitterBlit(FALSE, TRUE, TRUE, FALSE, FALSE, TRUE); break;
      case 7: blitterBlit(FALSE, TRUE, TRUE, TRUE, FALSE, TRUE); break;
      case 8: blitterBlit(TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); break;
      case 9: blitterBlit(TRUE, FALSE, FALSE, TRUE, FALSE, TRUE); break;
      case 10: blitterBlit(TRUE, FALSE, TRUE, FALSE, FALSE, TRUE); break;
      case 11: blitterBlit(TRUE, FALSE, TRUE, TRUE, FALSE, TRUE); break;
      case 12: blitterBlit(TRUE, TRUE, FALSE, FALSE, FALSE, TRUE); break;
      case 13: blitterBlit(TRUE, TRUE, FALSE, TRUE, FALSE, TRUE); break;
      case 14: blitterBlit(TRUE, TRUE, TRUE, FALSE, FALSE, TRUE); break;
      case 15: blitterBlit(TRUE, TRUE, TRUE, TRUE, FALSE, TRUE); break;
      }
    }
    else
    { /* Ascending mode */
      switch ((blitter.bltcon >> 24) & 0xf)
      {
      case 0: blitterBlit(FALSE, FALSE, FALSE, FALSE, TRUE, TRUE); break;
      case 1: blitterBlit(FALSE, FALSE, FALSE, TRUE, TRUE, TRUE); break;
      case 2: blitterBlit(FALSE, FALSE, TRUE, FALSE, TRUE, TRUE); break;
      case 3: blitterBlit(FALSE, FALSE, TRUE, TRUE, TRUE, TRUE); break;
      case 4: blitterBlit(FALSE, TRUE, FALSE, FALSE, TRUE, TRUE); break;
      case 5: blitterBlit(FALSE, TRUE, FALSE, TRUE, TRUE, TRUE); break;
      case 6: blitterBlit(FALSE, TRUE, TRUE, FALSE, TRUE, TRUE); break;
      case 7: blitterBlit(FALSE, TRUE, TRUE, TRUE, TRUE, TRUE); break;
      case 8: blitterBlit(TRUE, FALSE, FALSE, FALSE, TRUE, TRUE); break;
      case 9: blitterBlit(TRUE, FALSE, FALSE, TRUE, TRUE, TRUE); break;
      case 10: blitterBlit(TRUE, FALSE, TRUE, FALSE, TRUE, TRUE); break;
      case 11: blitterBlit(TRUE, FALSE, TRUE, TRUE, TRUE, TRUE); break;
      case 12: blitterBlit(TRUE, TRUE, FALSE, FALSE, TRUE, TRUE); break;
      case 13: blitterBlit(TRUE, TRUE, FALSE, TRUE, TRUE, TRUE); break;
      case 14: blitterBlit(TRUE, TRUE, TRUE, FALSE, TRUE, TRUE); break;
      case 15: blitterBlit(TRUE, TRUE, TRUE, TRUE, TRUE, TRUE); break;
      }
    }
  }
  else
  { /* Copy */
    if (blitterIsDescending())
    { /* Decending mode */
      switch ((blitter.bltcon >> 24) & 0xf)
      {
      case 0: blitterBlit(FALSE, FALSE, FALSE, FALSE, FALSE, FALSE); break;
      case 1: blitterBlit(FALSE, FALSE, FALSE, TRUE, FALSE, FALSE); break;
      case 2: blitterBlit(FALSE, FALSE, TRUE, FALSE, FALSE, FALSE); break;
      case 3: blitterBlit(FALSE, FALSE, TRUE, TRUE, FALSE, FALSE); break;
      case 4: blitterBlit(FALSE, TRUE, FALSE, FALSE, FALSE, FALSE); break;
      case 5: blitterBlit(FALSE, TRUE, FALSE, TRUE, FALSE, FALSE); break;
      case 6: blitterBlit(FALSE, TRUE, TRUE, FALSE, FALSE, FALSE); break;
      case 7: blitterBlit(FALSE, TRUE, TRUE, TRUE, FALSE, FALSE); break;
      case 8: blitterBlit(TRUE, FALSE, FALSE, FALSE, FALSE, FALSE); break;
      case 9: blitterBlit(TRUE, FALSE, FALSE, TRUE, FALSE, FALSE); break;
      case 10: blitterBlit(TRUE, FALSE, TRUE, FALSE, FALSE, FALSE); break;
      case 11: blitterBlit(TRUE, FALSE, TRUE, TRUE, FALSE, FALSE); break;
      case 12: blitterBlit(TRUE, TRUE, FALSE, FALSE, FALSE, FALSE); break;
      case 13: blitterBlit(TRUE, TRUE, FALSE, TRUE, FALSE, FALSE); break;
      case 14: blitterBlit(TRUE, TRUE, TRUE, FALSE, FALSE, FALSE); break;
      case 15: blitterBlit(TRUE, TRUE, TRUE, TRUE, FALSE, FALSE); break;
      }
    }
    else
    { /* Ascending mode */
      switch ((blitter.bltcon >> 24) & 0xf)
      {
      case 0: blitterBlit(FALSE, FALSE, FALSE, FALSE, TRUE, FALSE); break;
      case 1: blitterBlit(FALSE, FALSE, FALSE, TRUE, TRUE, FALSE); break;
      case 2: blitterBlit(FALSE, FALSE, TRUE, FALSE, TRUE, FALSE); break;
      case 3: blitterBlit(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE); break;
      case 4: blitterBlit(FALSE, TRUE, FALSE, FALSE, TRUE, FALSE); break;
      case 5: blitterBlit(FALSE, TRUE, FALSE, TRUE, TRUE, FALSE); break;
      case 6: blitterBlit(FALSE, TRUE, TRUE, FALSE, TRUE, FALSE); break;
      case 7: blitterBlit(FALSE, TRUE, TRUE, TRUE, TRUE, FALSE); break;
      case 8: blitterBlit(TRUE, FALSE, FALSE, FALSE, TRUE, FALSE); break;
      case 9: blitterBlit(TRUE, FALSE, FALSE, TRUE, TRUE, FALSE); break;
      case 10: blitterBlit(TRUE, FALSE, TRUE, FALSE, TRUE, FALSE); break;
      case 11: blitterBlit(TRUE, FALSE, TRUE, TRUE, TRUE, FALSE); break;
      case 12: blitterBlit(TRUE, TRUE, FALSE, FALSE, TRUE, FALSE); break;
      case 13: blitterBlit(TRUE, TRUE, FALSE, TRUE, TRUE, FALSE); break;
      case 14: blitterBlit(TRUE, TRUE, TRUE, FALSE, TRUE, FALSE); break;
      case 15: blitterBlit(TRUE, TRUE, TRUE, TRUE, TRUE, FALSE); break;
      }
    }
  }
}

#define blitterLineIncreaseX(a_shift, cpt) \
  if (a_shift < 15) a_shift++; \
  else \
{ \
  a_shift = 0; \
  cpt = (cpt + 2) & 0x1ffffe; \
}

#define blitterLineDecreaseX(a_shift, cpt) \
{ \
  if (a_shift == 0) \
{ \
  a_shift = 16; \
  cpt = (cpt - 2) & 0x1ffffe; \
} \
  a_shift--; \
}

#define blitterLineIncreaseY(cpt, cmod) \
  cpt = (cpt + cmod) & 0x1ffffe;

#define blitterLineDecreaseY(cpt, cmod) \
  cpt = (cpt - cmod) & 0x1ffffe;

/*================================================*/
/* blitterLineMode                                */
/* responsible for drawing lines with the blitter */
/*                                                */
/*================================================*/

void blitterLineMode(void)
{
  ULO bltadat_local;
  ULO bltbdat_local;
  ULO bltcdat_local = blitter.bltcdat;
  ULO bltddat_local;
  UWO mask = (UWO) ((blitter.bltbdat_original >> blitter.b_shift_asc) | (blitter.bltbdat_original << (16 - blitter.b_shift_asc)));
  BOOLE a_enabled = blitter.bltcon & 0x08000000;
  BOOLE c_enabled = blitter.bltcon & 0x02000000;

  BOOLE decision_is_signed = (((blitter.bltcon >> 6) & 1) == 1);
  LON decision_variable = (LON) blitter.bltapt;

  // Quirk: Set decision increases to 0 if a is disabled, ensures bltapt remains unchanged
  WOR decision_inc_signed = (a_enabled) ? ((WOR) blitter.bltbmod) : 0;
  WOR decision_inc_unsigned = (a_enabled) ? ((WOR) blitter.bltamod) : 0;

  ULO bltcpt_local = blitter.bltcpt;
  ULO bltdpt_local = blitter.bltdpt;
  ULO blit_a_shift_local = blitter.a_shift_asc;
  ULO bltzero_local = 0;
  ULO i;

  ULO sulsudaul = (blitter.bltcon >> 2) & 0x7;
  BOOLE x_independent = (sulsudaul & 4);
  BOOLE x_inc = ((!x_independent) && !(sulsudaul & 2)) || (x_independent && !(sulsudaul & 1));
  BOOLE y_inc = ((!x_independent) && !(sulsudaul & 1)) || (x_independent && !(sulsudaul & 2));
  BOOLE single_dot = FALSE;
  UBY minterm = (UBY) (blitter.bltcon >> 16);

  for (i = 0; i < blitter.height; ++i)
  {
    // Read C-data from memory if the C-channel is enabled
    if (c_enabled)
    {
      bltcdat_local = (memory_chip[bltcpt_local] << 8) | memory_chip[bltcpt_local + 1];
    }

    // Calculate data for the A-channel
    bltadat_local = (blitter.bltadat & blitter.bltafwm) >> blit_a_shift_local;

    // Check for single dot
    if (x_independent) 
    {
      if (blitter.bltcon & 0x00000002)
      {
	if (single_dot) 
	{
	  bltadat_local = 0;
	}
	else
	{
	  single_dot = TRUE;
	}
      }
    }

    // Calculate data for the B-channel
    bltbdat_local = (mask & 1) ? 0xffff : 0;

    // Calculate result
    blitterMinterms(bltadat_local, bltbdat_local, bltcdat_local, bltddat_local, minterm);

    // Save result to D-channel, same as the C ptr after first pixel. 
    if (c_enabled) // C-channel must be enabled
    {
      memory_chip[bltdpt_local] = (UBY) (bltddat_local >> 8);
      memory_chip[bltdpt_local + 1] = (UBY) (bltddat_local);
    }

    // Remember zero result status
    bltzero_local = bltzero_local | bltddat_local;

    // Rotate mask
    mask = (mask << 1) | (mask >> 15);

    // Test movement in the X direction
    // When the decision variable gets positive,
    // the line moves one pixel to the right

    // decrease/increase x
    if (decision_is_signed)
    {
      // Do not yet increase, D has sign
      // D = D + (2*sdelta = bltbmod)
      decision_variable += decision_inc_signed;
    }
    else
    {
      // increase, D reached a positive value
      // D = D + (2*sdelta - 2*ldelta = bltamod)
      decision_variable += decision_inc_unsigned;

      if (!x_independent)
      {
	if (x_inc)
	{
	  blitterLineIncreaseX(blit_a_shift_local, bltcpt_local);
	}
	else
	{
	  blitterLineDecreaseX(blit_a_shift_local, bltcpt_local);
	}
      }
      else
      {
	if (y_inc)
	{
	  blitterLineIncreaseY(bltcpt_local, blitter.bltcmod);
	}
	else
	{
	  blitterLineDecreaseY(bltcpt_local, blitter.bltcmod);
	}
	single_dot = FALSE;
      }
    }
    decision_is_signed = ((WOR)decision_variable < 0);

    if (!x_independent)
    {
      // decrease/increase y
      if (y_inc) 
      {
	blitterLineIncreaseY(bltcpt_local, blitter.bltcmod);
      }
      else
      {
	blitterLineDecreaseY(bltcpt_local, blitter.bltcmod);
      }
    }
    else
    {
      if (x_inc) 
      {
	blitterLineIncreaseX(blit_a_shift_local, bltcpt_local);
      }
      else
      {
	blitterLineDecreaseX(blit_a_shift_local, bltcpt_local);
      }
    }
    bltdpt_local = bltcpt_local;
  }
  blitter.bltcon = blitter.bltcon & 0x0FFFFFFBF;
  if (decision_is_signed) blitter.bltcon |= 0x00000040;

  blitter.a_shift_asc = blit_a_shift_local;
  blitter.a_shift_desc = 16 - blitter.a_shift_asc;
  blitter.bltbdat = bltbdat_local;
  blitter.bltapt = decision_variable;
  blitter.bltcpt = bltcpt_local;
  blitter.bltdpt = bltdpt_local;
  blitter.bltzero = bltzero_local;
  memoryWriteWord(0x8040, 0x00DFF09C);
}

void blitInitiate(void)
{
  ULO channels = (blitter.bltcon >> 24) & 0xf;
  ULO cycle_length, cycle_free;

#ifdef BLIT_OPERATION_LOG
  if (blitter_operation_log) blitterOperationLog();
#endif

  blitter.bltzero = 0;
  if (blitter_fast)
  {
    cycle_length = 3;
    cycle_free = 0;
  }
  else
  {
    if (blitter.bltcon & 1)
    {
      cycle_free = 2;
      if (!(channels & 2)) cycle_free++;
      cycle_length = 4*blitter.height;
      cycle_free *= blitter.height;
    }
    else
    {
      cycle_length = blit_cyclelength[channels]*blitter.width*blitter.height;
      cycle_free = blit_cyclefree[channels]*blitter.width*blitter.height;
    }
  }

  /* If BLTHOG is set, and there are no free cycles, */
  /* CPU is stopped completely, since that is what programs expect. */
  /* If the blit leaves free cycles for the CPU, the CPU is left alone. */
  /* If BLTHOG is not set, and there are no free cycles, */
  /* the CPU will have to wait until the 4th cycle every time it wants the bus. */
  /* The CPU does not use the bus all the time, so the added blit time or CPU slowdown is not */
  /* a full cycle_length/4. Give some impression of */
  /* added time, but the way the core operates, we can't compensate in detail for bpl and actual */
  /* CPU cycles. */

  if (cycle_free == 0)
  {
    if ((dmaconr & 0x400))
    {
      cpuIntegrationSetChipCycles(cycle_length); // Delay CPU for the entire time during the blit.
    }
    else
    {
      cycle_free = cycle_length / 5;
      cycle_length += cycle_free;
    }
  }
  blitter.cycle_length = cycle_length;
  blitter.cycle_free = cycle_free;
  if (blitter.cycle_free != 0)
  {
    ULO chip_slowdown = (blitter.cycle_length / blitter.cycle_free);
    if (chip_slowdown > 1) chip_slowdown--;
    cpuIntegrationSetChipSlowdown(chip_slowdown);
  }
  else
  {
    cpuIntegrationSetChipSlowdown(1);
  }
  blitter.started = TRUE;
  dmaconr |= 0x4000; /* Blitter busy bit */
  intreq &= 0xffbf;
  blitterInsertEvent(cycle_length + bus.cycle);
}

// Handles a blitter event.
// Can also be called by writes to certain registers. (Via. blitterForceFinish())
// Event has already been popped.
void blitFinishBlit(void) 
{
  blitterEvent.cycle = BUS_CYCLE_DISABLE;
  blitter.dma_pending = FALSE;
  blitter.started = FALSE;
  cpuIntegrationSetChipSlowdown(1);
  dmaconr = dmaconr & 0x0000bfff;
  if ((blitter.bltcon & 0x00000001) == 0x00000001)
  {
    blitterLineMode();
  }
  else
  {
    blitterCopyABCD();
  }
}

void blitForceFinish(void)
{
  if (blitterIsStarted()) 
  {
    blitterRemoveEvent();
    blitFinishBlit();
  }
}

void blitterCopy(void) 
{
  blitInitiate();
}

/*======================================================*/
/* BLTCON0                                              */
/*                                                      */
/* register address is $DFF040                          */
/* blitter control register 0                           */
/* write only                                           */
/* located in Agnus                                     */
/* only used by CPU or blitter (not by DMA controller)  */
/*======================================================*/

void wbltcon0(UWO data, ULO address)
{
  blitForceFinish();
  blitter.bltcon = (blitter.bltcon & 0x0000FFFF) | (((ULO)data) << 16);
  blitter.a_shift_asc = data >> 12;
  blitter.a_shift_desc = 16 - blitter.a_shift_asc;
}

/*======================================================*/
/* BLTCON1                                              */
/*                                                      */
/* register address is $DFF042                          */
/* blitter control register 1                           */
/* write only                                           */
/* located in Agnus                                     */
/* only used by CPU or blitter (not by DMA controller)  */
/*======================================================*/

void wbltcon1(UWO data, ULO address)
{
  blitForceFinish();
  blitter.bltcon = (blitter.bltcon & 0xFFFF0000) | ((ULO)data);
  blitter.b_shift_asc = data >> 12;
  blitter.b_shift_desc = 16 - blitter.b_shift_asc;
}

/*======================================================*/
/* BLTAFWM                                              */
/*                                                      */
/* register address is $DFF044                          */
/* blitter mask for first word of area A                */
/* write only                                           */
/* located in Agnus                                     */
/* only used by CPU or blitter (not by DMA controller)  */
/*======================================================*/

void wbltafwm(UWO data, ULO address)
{
  blitForceFinish();
  blitter.bltafwm = data;  
}

/*======================================================*/
/* BLTALWM                                              */
/*                                                      */
/* register address is $DFF046                          */
/* blitter mask for last word of area A                 */
/* write only                                           */
/* located in Agnus                                     */
/* only used by CPU or blitter (not by DMA controller)  */
/*======================================================*/

void wbltalwm(UWO data, ULO address)
{
  blitForceFinish();
  blitter.bltalwm = data;  
}

/*======================================================*/
/* BLTCPTH                                              */
/*                                                      */
/* register address is $DFF048                          */
/* adres of source C (high 5 bits, found at bit 4 to 0) */
/* write only                                           */
/* located in Agnus                                     */
/* only used by CPU or blitter (not by DMA controller)  */
/*======================================================*/

void wbltcpth(UWO data, ULO address)
{
  blitForceFinish();
  blitter.bltcpt = (blitter.bltcpt & 0x0000FFFF) | (((ULO) (data & 0x0000001F)) << 16);
}

/*=========================================================*/
/* BLTCPTL                                                 */
/*                                                         */
/* register address is $DFF04A                             */
/* adres of source C (lower 15 bits, found at bit 15 to 1) */
/* write only                                              */
/* located in Agnus                                        */
/* only used by CPU or blitter (not by DMA controller)     */
/*=========================================================*/

void wbltcptl(UWO data, ULO address)
{
  blitForceFinish();
  blitter.bltcpt = (blitter.bltcpt & 0xFFFF0000) | ((ULO) (data & 0x0000FFFE));
}

/*======================================================*/
/* BLTBPTH                                              */
/*                                                      */
/* register address is $DFF04C                          */
/* adres of source B (high 5 bits, found at bit 4 to 0) */
/* write only                                           */
/* located in Agnus                                     */
/* only used by CPU or blitter (not by DMA controller)  */
/*======================================================*/

void wbltbpth(UWO data, ULO address)
{
  blitForceFinish();
  blitter.bltbpt = (blitter.bltbpt & 0x0000FFFF) | (((ULO) (data & 0x0000001F)) << 16);
}

/*=========================================================*/
/* BLTBPTL                                                 */
/*                                                         */
/* register address is $DFF04E                             */
/* adres of source B (lower 15 bits, found at bit 15 to 1) */
/* write only                                              */
/* located in Agnus                                        */
/* only used by CPU or blitter (not by DMA controller)     */
/*=========================================================*/

void wbltbptl(UWO data, ULO address)
{
  blitForceFinish();
  blitter.bltbpt = (blitter.bltbpt & 0xFFFF0000) | ((ULO) (data & 0x0000FFFE));
}

/*======================================================*/
/* BLTAPTH                                              */
/*                                                      */
/* register address is $DFF050                          */
/* adres of source A (high 5 bits, found at bit 4 to 0) */
/* write only                                           */
/* located in Agnus                                     */
/* only used by CPU or blitter (not by DMA controller)  */
/*======================================================*/

void wbltapth(UWO data, ULO address)
{
  blitForceFinish();
  blitter.bltapt = (blitter.bltapt & 0x0000FFFF) | (((ULO) (data & 0x0000001F)) << 16);
}

/*=========================================================*/
/* BLTAPTL                                                 */
/*                                                         */
/* register address is $DFF052                             */
/* adres of source A (lower 15 bits, found at bit 15 to 1) */
/* write only                                              */
/* located in Agnus                                        */
/* only used by CPU or blitter (not by DMA controller)     */
/*=========================================================*/

void wbltaptl(UWO data, ULO address)
{
  blitForceFinish();
  blitter.bltapt = (blitter.bltapt & 0xFFFF0000) | ((ULO) (data & 0x0000FFFE));
}

/*===========================================================*/
/* BLTDPTH                                                   */
/*                                                           */
/* register address is $DFF054                               */
/* adres of destination D (high 5 bits, found at bit 4 to 0) */
/* write only                                                */
/* located in Agnus                                          */
/* only used by CPU or blitter (not by DMA controller)       */
/*===========================================================*/

void wbltdpth(UWO data, ULO address)
{
  blitForceFinish();
  blitter.bltdpt = (blitter.bltdpt & 0x0000FFFF) | (((ULO) (data & 0x0000001F)) << 16);
}

/*==============================================================*/
/* BLTDPTL                                                      */
/*                                                              */
/* register address is $DFF056                                  */
/* adres of destination D (lower 15 bits, found at bit 15 to 1) */
/* write only                                                   */
/* located in Agnus                                             */
/* only used by CPU or blitter (not by DMA controller)          */
/*==============================================================*/

void wbltdptl(UWO data, ULO address)
{
  blitForceFinish();
  blitter.bltdpt = (blitter.bltdpt & 0xFFFF0000) | ((ULO) (data & 0x0000FFFE));
}

/*==============================================================*/
/* BLTSIZE                                                      */
/*                                                              */
/* register address is $DFF058                                  */
/* Blitter start and size (win/width, height)                   */
/* write only                                                   */
/* located in Agnus                                             */
/* only used by CPU or blitter (not by DMA controller)          */
/*==============================================================*/

void wbltsize(UWO data, ULO address)
{
  blitForceFinish();
  if ((data & 0x003F) != 0)
  {
    blitter.width = data & 0x0000003F;
  }
  else
  {
    blitter.width = 64;
  }
  if (((data >> 6) & 0x000003FF) != 0)
  {
    blitter.height = (data >> 6) & 0x000003FF;
  }
  else
  {
    blitter.height = 1024;
  }
  // check if blitter DMA is on
  if ((dmacon & 0x00000040) != 0) 
  {
    blitterCopy();
  }
  else
  {
    blitter.dma_pending = TRUE;
  }
}

/*==============================================================*/
/* BLTCON0L - ECS register                                      */
/*                                                              */
/* register address is $DFF05A                                  */
/* Blitter control 0 lower 8 bits (minterms)                    */
/* write only                                                   */
/* located in Agnus                                             */
/* only used by CPU or blitter (not by DMA controller)          */
/*==============================================================*/

void wbltcon0l(UWO data, ULO address)
{
  blitForceFinish();
  blitter.bltcon = (blitter.bltcon & 0xFF00FFFF) | ((((ULO)data) << 16) & 0x00FF0000);
}

/*==============================================================*/
/* BLTSIZV - ECS register                                       */
/*                                                              */
/* register address is $DFF05C                                  */
/* Blitter V size (for 15 bit vert size)                        */
/* write only                                                   */
/* located in Agnus                                             */
/* only used by CPU or blitter (not by DMA controller)          */
/*==============================================================*/

void wbltsizv(UWO data, ULO address)
{
  blitForceFinish();
  if ((data & 0x00007FFF) != 0)
  {
    blitter.height = data & 0x00007FFF;
  }
  else
  {
    blitter.height = 0x00008000; 
    // ECS increased possible blit height to 32768 lines
    // OCS is limited to a blit height of 1024 lines
  }
}

/*==============================================================*/
/* BLTSIZH - ECS register                                       */
/*                                                              */
/* register address is $DFF05E                                  */
/* Blitter H size & start (for 11 bit H size)                   */
/* write only                                                   */
/* located in Agnus                                             */
/* only used by CPU or blitter (not by DMA controller)          */
/*==============================================================*/

void wbltsizh(UWO data, ULO address)
{
  blitForceFinish();
  if ((data & 0x000007FF) != 0)
  {
    blitter.width = data & 0x000007FF;
  }
  else
  {
    blitter.width = 0x00000800; 
    // ECS increased possible blit width to 2048
    // OCS is limited to a blit height of 1024
  }

  if ((dmacon & 0x00000040) != 0) 
  {
    blitterCopy();
  }
  else
  {
    blitter.dma_pending = TRUE;
  }
}

/*==============================================================*/
/* BLTCMOD                                                      */
/*                                                              */
/* register address is $DFF060                                  */
/* Blitter modulo for source C                                  */
/* write only                                                   */
/* located in Agnus                                             */
/* only used by CPU or blitter (not by DMA controller)          */
/*==============================================================*/

void wbltcmod(UWO data, ULO address)
{
  blitForceFinish();
  blitter.bltcmod = (ULO)(LON)(WOR)(data & 0x0000FFFE);
}

/*==============================================================*/
/* BLTBMOD                                                      */
/*                                                              */
/* register address is $DFF062                                  */
/* Blitter modulo for source B                                  */
/* write only                                                   */
/* located in Agnus                                             */
/* only used by CPU or blitter (not by DMA controller)          */
/*==============================================================*/

void wbltbmod(UWO data, ULO address)
{
  blitForceFinish();
  blitter.bltbmod = (ULO)(LON)(WOR)(data & 0x0000FFFE);
}

/*==============================================================*/
/* BLTAMOD                                                      */
/*                                                              */
/* register address is $DFF064                                  */
/* Blitter modulo for source A                                  */
/* write only                                                   */
/* located in Agnus                                             */
/* only used by CPU or blitter (not by DMA controller)          */
/*==============================================================*/

void wbltamod(UWO data, ULO address)
{
  blitForceFinish();
  blitter.bltamod = (ULO)(LON)(WOR)(data & 0x0000FFFE);
}

/*==============================================================*/
/* BLTDMOD                                                      */
/*                                                              */
/* register address is $DFF066                                  */
/* Blitter modulo for source D                                  */
/* write only                                                   */
/* located in Agnus                                             */
/* only used by CPU or blitter (not by DMA controller)          */
/*==============================================================*/

void wbltdmod(UWO data, ULO address)
{
  blitForceFinish();
  blitter.bltdmod = (ULO)(LON)(WOR)(data & 0x0000FFFE);
}

/*==============================================================*/
/* BLTCDAT                                                      */
/*                                                              */
/* register address is $DFF070                                  */
/* Blitter source C data reg                                    */
/* write only                                                   */
/* located in Agnus                                             */
/* only used by DMA controller (not by CPU or blitter)          */
/*==============================================================*/

void wbltcdat(UWO data, ULO address)
{
  blitForceFinish();
  blitter.bltcdat = data;
}

/*==============================================================*/
/* BLTBDAT                                                      */
/*                                                              */
/* register address is $DFF072                                  */
/* Blitter source B data reg                                    */
/* write only                                                   */
/* located in Agnus                                             */
/* only used by DMA controller (not by CPU or blitter)          */
/*==============================================================*/

void wbltbdat(UWO data, ULO address)
{
  blitForceFinish();
  blitter.bltbdat_original = (ULO) (data & 0x0000FFFF);
  if (blitterIsDescending())
  {
    blitter.bltbdat = (blitter.bltbdat_original << blitter.b_shift_asc);
  }
  else
  {
    blitter.bltbdat = (blitter.bltbdat_original >> blitter.b_shift_asc);
  }
}

/*==============================================================*/
/* BLTADAT                                                      */
/*                                                              */
/* register address is $DFF074                                  */
/* Blitter source A data reg                                    */
/* write only                                                   */
/* located in Agnus                                             */
/* only used by DMA controller (not by CPU or blitter)          */
/*==============================================================*/

void wbltadat(UWO data, ULO address)
{
  blitForceFinish();
  blitter.bltadat = data;
}

/*============================================================================*/
/* Fill-table init                                                            */
/*============================================================================*/

static void blitterFillTableInit(void)
{
  ULO i, data, bit, fc_tmp, fc, mode;

  for (mode = 0; mode < 2; mode++)
    for (fc = 0; fc < 2; fc++)
      for (i = 0; i < 256; i++)
      {
	fc_tmp = fc;
	data = i;
	for (bit = 0; bit < 16; bit++)
	{
	  if (mode == 0) data |= fc_tmp<<bit;
	  else data ^= fc_tmp<<bit;
	  if ((i & (0x1<<bit))) fc_tmp = (fc_tmp == 1) ? 0 : 1;
	}
	blit_fill[mode][fc][i][0] = (UBY) fc_tmp;
	blit_fill[mode][fc][i][1] = (UBY) data;
      }
}

/*============================================================================*/
/* Set blitter IO stubs in IO read/write table                                */
/*============================================================================*/

static void blitterIOHandlersInstall(void)
{
  memorySetIoWriteStub(0x40, wbltcon0);
  memorySetIoWriteStub(0x42, wbltcon1);
  memorySetIoWriteStub(0x44, wbltafwm);
  memorySetIoWriteStub(0x46, wbltalwm);
  memorySetIoWriteStub(0x48, wbltcpth);
  memorySetIoWriteStub(0x4a, wbltcptl);
  memorySetIoWriteStub(0x4c, wbltbpth);
  memorySetIoWriteStub(0x4e, wbltbptl);
  memorySetIoWriteStub(0x50, wbltapth);
  memorySetIoWriteStub(0x52, wbltaptl);
  memorySetIoWriteStub(0x54, wbltdpth);
  memorySetIoWriteStub(0x56, wbltdptl);
  memorySetIoWriteStub(0x58, wbltsize);
  memorySetIoWriteStub(0x60, wbltcmod);
  memorySetIoWriteStub(0x62, wbltbmod);
  memorySetIoWriteStub(0x64, wbltamod);
  memorySetIoWriteStub(0x66, wbltdmod);
  memorySetIoWriteStub(0x70, wbltcdat);
  memorySetIoWriteStub(0x72, wbltbdat);
  memorySetIoWriteStub(0x74, wbltadat);
  if (blitterGetECS())
  {
    memorySetIoWriteStub(0x5a, wbltcon0l);
    memorySetIoWriteStub(0x5c, wbltsizv);
    memorySetIoWriteStub(0x5e, wbltsizh);
  }
}

/*============================================================================*/
/* Set blitter to default values                                              */
/*============================================================================*/

static void blitterIORegistersClear(void)
{
  blitter.bltapt = 0;
  blitter.bltbpt = 0;
  blitter.bltcpt = 0;
  blitter.bltdpt = 0;
  blitter.bltcon = 0;
  blitter.bltafwm = 0;
  blitter.bltalwm = 0;
  blitter.bltamod = 0;
  blitter.bltbmod = 0;
  blitter.bltcmod = 0;
  blitter.bltdmod = 0;
  blitter.bltadat = 0;
  blitter.bltbdat = 0;
  blitter.bltbdat_original = 0;
  blitter.bltcdat = 0;
  blitter.bltzero = 0;

  blitter.width = 0;
  blitter.height = 0;
  blitter.a_shift_asc = 0;
  blitter.a_shift_desc = 0;
  blitter.b_shift_asc = 0;
  blitter.b_shift_desc = 0;
  blitter.started = FALSE;
  blitter.dma_pending = FALSE;
}

/*============================================================================*/
/* Reset blitter to default values                                            */
/*============================================================================*/

void blitterHardReset(void)
{
  blitterIORegistersClear();
}

void blitterEndOfFrame(void)
{
  if (blitterEvent.cycle != BUS_CYCLE_DISABLE)
  {
    LON cycle = blitterEvent.cycle -= BUS_CYCLE_PER_FRAME;
    blitterRemoveEvent();    
    blitterInsertEvent(cycle);
  }
}

/*===========================================================================*/
/* Called on emulator start / stop                                           */
/*===========================================================================*/

void blitterSaveState(FILE *F)
{
  fwrite(&blitter.bltcon, sizeof(blitter.bltcon), 1, F);
  fwrite(&blitter.bltafwm, sizeof(blitter.bltafwm), 1, F);
  fwrite(&blitter.bltalwm, sizeof(blitter.bltalwm), 1, F);
  fwrite(&blitter.bltapt, sizeof(blitter.bltapt), 1, F);
  fwrite(&blitter.bltbpt, sizeof(blitter.bltbpt), 1, F);
  fwrite(&blitter.bltcpt, sizeof(blitter.bltcpt), 1, F);
  fwrite(&blitter.bltdpt, sizeof(blitter.bltdpt), 1, F);
  fwrite(&blitter.bltamod, sizeof(blitter.bltamod), 1, F);
  fwrite(&blitter.bltbmod, sizeof(blitter.bltbmod), 1, F);
  fwrite(&blitter.bltcmod, sizeof(blitter.bltcmod), 1, F);
  fwrite(&blitter.bltdmod, sizeof(blitter.bltdmod), 1, F);
  fwrite(&blitter.bltadat, sizeof(blitter.bltadat), 1, F);
  fwrite(&blitter.bltbdat, sizeof(blitter.bltbdat), 1, F);
  fwrite(&blitter.bltbdat_original, sizeof(blitter.bltbdat_original), 1, F);
  fwrite(&blitter.bltcdat, sizeof(blitter.bltcdat), 1, F);
  fwrite(&blitter.bltzero, sizeof(blitter.bltzero), 1, F);

  fwrite(&blitter.height, sizeof(blitter.height), 1, F);
  fwrite(&blitter.width, sizeof(blitter.width), 1, F);

  fwrite(&blitter.a_shift_asc, sizeof(blitter.a_shift_asc), 1, F);
  fwrite(&blitter.a_shift_desc, sizeof(blitter.a_shift_desc), 1, F);
  fwrite(&blitter.b_shift_asc, sizeof(blitter.b_shift_asc), 1, F);
  fwrite(&blitter.b_shift_desc, sizeof(blitter.b_shift_desc), 1, F);

  fwrite(&blitter.started, sizeof(blitter.started), 1, F);
  fwrite(&blitter.dma_pending, sizeof(blitter.dma_pending), 1, F);
  fwrite(&blitter.cycle_length, sizeof(blitter.cycle_length), 1, F);
  fwrite(&blitter.cycle_free, sizeof(blitter.cycle_free), 1, F);
}

void blitterLoadState(FILE *F)
{
  fread(&blitter.bltcon, sizeof(blitter.bltcon), 1, F);
  fread(&blitter.bltafwm, sizeof(blitter.bltafwm), 1, F);
  fread(&blitter.bltalwm, sizeof(blitter.bltalwm), 1, F);
  fread(&blitter.bltapt, sizeof(blitter.bltapt), 1, F);
  fread(&blitter.bltbpt, sizeof(blitter.bltbpt), 1, F);
  fread(&blitter.bltcpt, sizeof(blitter.bltcpt), 1, F);
  fread(&blitter.bltdpt, sizeof(blitter.bltdpt), 1, F);
  fread(&blitter.bltamod, sizeof(blitter.bltamod), 1, F);
  fread(&blitter.bltbmod, sizeof(blitter.bltbmod), 1, F);
  fread(&blitter.bltcmod, sizeof(blitter.bltcmod), 1, F);
  fread(&blitter.bltdmod, sizeof(blitter.bltdmod), 1, F);
  fread(&blitter.bltadat, sizeof(blitter.bltadat), 1, F);
  fread(&blitter.bltbdat, sizeof(blitter.bltbdat), 1, F);
  fread(&blitter.bltbdat_original, sizeof(blitter.bltbdat_original), 1, F);
  fread(&blitter.bltcdat, sizeof(blitter.bltcdat), 1, F);
  fread(&blitter.bltzero, sizeof(blitter.bltzero), 1, F);

  fread(&blitter.height, sizeof(blitter.height), 1, F);
  fread(&blitter.width, sizeof(blitter.width), 1, F);

  fread(&blitter.a_shift_asc, sizeof(blitter.a_shift_asc), 1, F);
  fread(&blitter.a_shift_desc, sizeof(blitter.a_shift_desc), 1, F);
  fread(&blitter.b_shift_asc, sizeof(blitter.b_shift_asc), 1, F);
  fread(&blitter.b_shift_desc, sizeof(blitter.b_shift_desc), 1, F);

  fread(&blitter.started, sizeof(blitter.started), 1, F);
  fread(&blitter.dma_pending, sizeof(blitter.dma_pending), 1, F);
  fread(&blitter.cycle_length, sizeof(blitter.cycle_length), 1, F);
  fread(&blitter.cycle_free, sizeof(blitter.cycle_free), 1, F);
}

void blitterEmulationStart(void)
{
  blitterIOHandlersInstall();
}

void blitterEmulationStop(void)
{
}

#ifdef BLIT_VERIFY_MINTERMS

ULO optimizedMinterms(UBY minterm, ULO a_dat, ULO b_dat, ULO c_dat)
{
  ULO d_dat = 0;
  blitterMinterms(a_dat, b_dat, c_dat, d_dat, minterm);
  return d_dat;
}

ULO correctMinterms(UBY minterm, ULO a_dat, ULO b_dat, ULO c_dat)
{
  ULO d_dat = 0;
  blitterMintermGeneric(a_dat, b_dat, c_dat, d_dat, minterm);
  return d_dat;
}

void verifyMinterms()
{
  UBY minterm;
  ULO a_dat, b_dat, c_dat;
  for (minterm = 0xe8; minterm <= 0xe8; minterm++)
  {
    BOOLE minterm_had_error = FALSE;
    char s[40];
    for (a_dat = 0; a_dat < 256; a_dat++)
      for (b_dat = 0; b_dat < 256; b_dat++)
	for (c_dat = 0; c_dat < 256; c_dat++)
	  minterm_had_error |= (correctMinterms(minterm, a_dat, b_dat, c_dat) != optimizedMinterms(minterm, a_dat, b_dat, c_dat));
    sprintf(s, "Minterm %X was %s", minterm, (minterm_had_error) ? "incorrect" : "correct");
    MessageBox(0, s, "Minterm check", 0);
  }
}

#endif

/*===========================================================================*/
/* Called on emulator startup / shutdown                                     */
/*===========================================================================*/

void blitterStartup(void)
{
  blitterFillTableInit();
  blitterSetFast(FALSE);
  blitterSetECS(FALSE);
  blitterIORegistersClear();

#ifdef BLIT_OPERATION_LOG
  blitterSetOperationLog(FALSE);
  blitter_operation_log_first = TRUE;
#endif

#ifdef BLIT_VERIFY_MINTERMS
  {
    ULO i;
    for (i = 0; i < 256; i++) blit_minterm_seen[i] = FALSE;
    verifyMinterms();
  }
#endif
}

void blitterShutdown(void)
{
}
