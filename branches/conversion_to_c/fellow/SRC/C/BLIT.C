/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Blitter Initialization                                                     */
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
#include "blit.h"
#include "fmem.h"
#include "graph.h"
#include "draw.h"
#include "cpu.h"
#include "bus.h"

/*#define BLIT_TSC_PROFILE*/

#ifdef BLIT_TSC_PROFILE
LLO blit_tsc_tmp = 0;
LLO blit_tsc = 0;
LON blit_tsc_times = 0;
ULO blit_tsc_words = 0;
#endif


/*============================================================================*/
/* Blitter registers                                                          */
/*============================================================================*/

ULO bltcon, bltafwm, bltalwm, bltapt, bltbpt, bltcpt, bltdpt, bltamod, bltbmod;
ULO bltcmod, bltdmod, bltadat, bltbdat, bltbdat_original, bltcdat, bltzero;

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

/*============================================================================*/
/* Various blitter variables                                                  */
/*============================================================================*/

ULO blitend;
ULO blit_height, blit_width;

// flag showing that a blit has been activated (a write to BLTSIZE)
// but at the time of activation the blit DMA was turned of
ULO blitterdmawaiting; 

BOOLE blitter_operation_log, blitter_operation_log_first;
ULO blit_a_shift_asc, blit_a_shift_desc;
ULO blit_b_shift_asc, blit_b_shift_desc;
ULO blit_minterm;
BOOLE blit_desc;
BOOLE blit_started;
ULO blit_cycle_length, blit_cycle_free;


#ifdef BLIT_TSC_PROFILE
BOOLE blit_minterm_seen[256];
#endif



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
	      draw_frame_count, graph_raster_y, graph_raster_x, cpuGetPC(pc), (bltcon >> 16) & 0xffff, bltcon & 0xffff, bltafwm & 0xffff, bltalwm & 0xffff, bltapt, bltbpt, bltcpt, bltdpt, bltamod & 0xffff, bltbmod & 0xffff, bltcmod & 0xffff, bltdmod & 0xffff, bltadat & 0xffff, bltbdat & 0xffff, bltcdat & 0xffff, blit_height, blit_width);
      fclose(F);
    }
  }
}

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
#define blitterMinterm1a(a_dat, b_dat, c_dat, d_dat) d_dat = ((a_dat & ~b_dat & ~c_dat) | (~a_dat & c_dat)); /* Abc + aC */
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
#define blitterMinterm4a(a_dat, b_dat, c_dat, d_dat) d_dat = ((~a_dat & c_dat) | (a_dat & b_dat & ~c_dat)); /* aC + ABc */
#define blitterMinterm5a(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat ^ c_dat);						/* A xor C */
#define blitterMinterm6a(a_dat, b_dat, c_dat, d_dat) d_dat = ((c_dat & ~b_dat) | (b_dat & (a_dat ^ c_dat))); /* bC + B(A xor C) */
#define blitterMinterm80(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat & b_dat & c_dat);				/* ABC */
#define blitterMinterma0(a_dat, b_dat, c_dat, d_dat) d_dat = (a_dat & c_dat);						/* AC */
#define blitterMinterma8(a_dat, b_dat, c_dat, d_dat) d_dat = (((a_dat & ~b_dat) | b_dat) & c_dat);  /* (Ab + B)C */
#define blitterMintermaa(a_dat, b_dat, c_dat, d_dat) d_dat = (c_dat);                               /* C */
#define blitterMintermac(a_dat, b_dat, c_dat, d_dat) d_dat = ((a_dat & c_dat) | (~a_dat & b_dat));  /* AC + aB */
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
#define blitterMintermff(a_dat, b_dat, c_dat, d_dat) d_dat = (-1);                                  /* All 1 */

#define blitterMintermGeneric(a_dat, b_dat, c_dat, d_dat, mins) \
  /*blit_minterm_seen[mins] = TRUE;*/ \
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
    case 0x1a: blitterMinterm1a(a_dat, b_dat, c_dat, d_dat); break; \
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
    case 0x4a: blitterMinterm4a(a_dat, b_dat, c_dat, d_dat); break; \
    case 0x5a: blitterMinterm5a(a_dat, b_dat, c_dat, d_dat); break; \
    case 0x6a: blitterMinterm6a(a_dat, b_dat, c_dat, d_dat); break; \
    case 0x80: blitterMinterm80(a_dat, b_dat, c_dat, d_dat); break; \
    case 0xa0: blitterMinterma0(a_dat, b_dat, c_dat, d_dat); break; \
    case 0xa8: blitterMinterma8(a_dat, b_dat, c_dat, d_dat); break; \
    case 0xaa: blitterMintermaa(a_dat, b_dat, c_dat, d_dat); break; \
    case 0xac: blitterMintermac(a_dat, b_dat, c_dat, d_dat); break; \
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
  ULO a_pt = bltapt; \
  ULO b_pt = bltbpt; \
  ULO c_pt = bltcpt; \
  ULO d_pt = bltdpt; \
  ULO d_pt_tmp = 0xffffffff; \
  ULO a_shift = (ascending) ? blit_a_shift_asc : blit_a_shift_desc; \
  ULO b_shift = (ascending) ? blit_b_shift_asc : blit_b_shift_desc; \
  ULO a_dat, b_dat = (b_enabled) ? 0 : bltbdat, c_dat = bltcdat, d_dat; \
  ULO a_dat_preload = bltadat; \
  ULO b_dat_preload = 0; \
  ULO a_prev = 0; \
  ULO b_prev = 0; \
  ULO a_mod = (ascending) ? bltamod : ((ULO) - (LON) bltamod); \
  ULO b_mod = (ascending) ? bltbmod : ((ULO) - (LON) bltbmod); \
  ULO c_mod = (ascending) ? bltcmod : ((ULO) - (LON) bltcmod); \
  ULO d_mod = (ascending) ? bltdmod : ((ULO) - (LON) bltdmod); \
  ULO fwm[2]; \
  ULO lwm[2]; \
  UBY minterms = (UBY) blit_minterm; \
  ULO fill_exclusive = (bltcon & 0x8) ? 0 : 1; \
  ULO zero_flag = 0; \
  LON height = blit_height; \
  LON width = blit_width; \
  BOOLE fc_original = !!(bltcon & 0x4); \
  BOOLE fill_carry; \
  fwm[0] = lwm[0] = 0xffff; \
  fwm[1] = bltafwm; \
  lwm[1] = bltalwm; \
  /*if (!fill) blit_tsc_words += height*width;*/ \
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
    bltadat = a_dat_preload; \
    bltapt = a_pt; \
  } \
  if (b_enabled) { \
    ULO x_tmp = 0; \
    blitterShiftWord(bltbdat, b_dat_preload, ascending, b_shift, x_tmp); \
    bltbdat_original = b_dat_preload; \
    bltbpt = b_pt; \
  } \
  if (c_enabled) { \
    bltcdat = c_dat; \
    bltcpt = c_pt; \
  } \
  if (d_enabled) bltdpt = d_pt_tmp; \
  bltzero = zero_flag; \
  wriw(0x8040, 0xdff09c); \
}

void blitterCopyABCD(void)
{
  if (bltcon & 0x18)
  { /* Fill */
    if (blit_desc)
    { /* Decending mode */
      switch ((bltcon >> 24) & 0xf)
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
      switch ((bltcon >> 24) & 0xf)
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
    if (blit_desc)
    { /* Decending mode */
      switch ((bltcon >> 24) & 0xf)
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
      switch ((bltcon >> 24) & 0xf)
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

void blitInitiate(void) {
  ULO channels = (bltcon >> 24) & 0xf;
  ULO cycle_length, cycle_free;
  if (blitter_operation_log) blitterOperationLog();
  bltzero = 0;
  if (blitter_fast) {
    cycle_length = 3;
    cycle_free = 0;
  }
  else {
    if (bltcon & 1) {
      cycle_free = 2;
      if (!(channels & 1)) cycle_free++;
      if (!(channels & 2)) cycle_free++;
      cycle_length = 4*blit_height;
      cycle_free *= blit_height;
    }
    else {
      cycle_length = blit_cyclelength[channels]*blit_width*blit_height;
      cycle_free = blit_cyclefree[channels]*blit_width*blit_height;
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

  if (cycle_free == 0) {
    if ((dmaconr & 0x400)) thiscycle = cycle_length;
    else cycle_length += (cycle_length/5);
  }
  blit_cycle_length = cycle_length;
  blit_cycle_free = cycle_free;
  blitend = cycle_length + curcycle;
  blit_started = TRUE;
  dmaconr |= 0x4000; /* Blitter busy bit */
}

/*============================================================================*/
/* Fill-table init                                                            */
/*============================================================================*/

static void blitterFillTableInit(void) {
  int i, data, bit, fc_tmp, fc, mode;
  for (mode = 0; mode < 2; mode++)
    for (fc = 0; fc < 2; fc++)
      for (i = 0; i < 256; i++) {
	fc_tmp = fc;
	data = i;
	for (bit = 0; bit < 16; bit++) {
	  if (mode == 0) data |= fc_tmp<<bit;
	  else data ^= fc_tmp<<bit;
	  if ((i & (0x1<<bit))) fc_tmp = (fc_tmp == 1) ? 0 : 1;
	}
	blit_fill[mode][fc][i][0] = fc_tmp;
	blit_fill[mode][fc][i][1] = data;
      }
}

/*============================================================================*/
/* Set blitter IO stubs in IO read/write table                                */
/*============================================================================*/

static void blitterIOHandlersInstall(void) {
  memorySetIOWriteStub(0x40, wbltcon0_C);
  memorySetIOWriteStub(0x42, wbltcon1_C);
  memorySetIOWriteStub(0x44, wbltafwm_C);
  memorySetIOWriteStub(0x46, wbltalwm_C);
  memorySetIOWriteStub(0x48, wbltcpth_C);
  memorySetIOWriteStub(0x4a, wbltcptl_C);
  memorySetIOWriteStub(0x4c, wbltbpth_C);
  memorySetIOWriteStub(0x4e, wbltbptl_C);
  memorySetIOWriteStub(0x50, wbltapth_C);
  memorySetIOWriteStub(0x52, wbltaptl_C);
  memorySetIOWriteStub(0x54, wbltdpth_C);
  memorySetIOWriteStub(0x56, wbltdptl_C);
  memorySetIOWriteStub(0x58, wbltsize_C);
  memorySetIOWriteStub(0x60, wbltcmod_C);
  memorySetIOWriteStub(0x62, wbltbmod_C);
  memorySetIOWriteStub(0x64, wbltamod_C);
  memorySetIOWriteStub(0x66, wbltdmod_C);
  memorySetIOWriteStub(0x70, wbltcdat_C);
  memorySetIOWriteStub(0x72, wbltbdat_C);
  memorySetIOWriteStub(0x74, wbltadat_C);
  if (blitterGetECS()) {
    memorySetIOWriteStub(0x5a, wbltcon0l_C);
    memorySetIOWriteStub(0x5c, wbltsizv_C);
    memorySetIOWriteStub(0x5e, wbltsizh_C);
  }    
}


/*============================================================================*/
/* Set blitter to default values                                              */
/*============================================================================*/

static void blitterIORegistersClear(void) {
  //blit_asm_minterm = blit_min_generic;
  blitend = 0xffffffff;             /* Must keep blitend -1 when not blitting */
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
  bltbdat_original = 0;
  bltcdat = 0;
  bltzero = 0;
  blitterdmawaiting = 0;
  blit_width = 0;
  blit_height = 0;
  blit_minterm = 0;
  blit_a_shift_asc = 0;
  blit_a_shift_desc = 0;
  blit_b_shift_asc = 0;
  blit_b_shift_desc = 0;
  blit_desc = FALSE;
  blit_started = FALSE;
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

/*
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
  for (minterm = 0x80; minterm <= 0x80; minterm++)
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
*/

/*===========================================================================*/
/* Called on emulator startup / shutdown                                     */
/*===========================================================================*/

void blitterStartup(void) {
#ifdef BLIT_TSC_PROFILE
  ULO i;
  for (i = 0; i < 256; i++) blit_minterm_seen[i] = FALSE;
#endif
  
  blitterFillTableInit();
  blitterSetFast(FALSE);
  blitterSetECS(FALSE);
  blitterIORegistersClear();
  blitterSetOperationLog(FALSE);
  blitter_operation_log_first = TRUE;
  //verifyMinterms();
}

void blitterShutdown(void) {
#ifdef BLIT_TSC_PROFILE
  {
  FILE *F = fopen("blitprofile.txt", "w");
  fprintf(F, "FUNCTION\tTOTALCYCLES\tCALLEDCOUNT\tAVGCYCLESPERCALL\tWORDS\tWORDSPERCALL\tCYCLESPERWORD\n");
  fprintf(F, "blitter copy\t%I64d\t%d\t%I64d\t%d\t%d\t%d\n", blit_tsc, blit_tsc_times, (blit_tsc_times == 0) ? 0 : (blit_tsc / blit_tsc_times), blit_tsc_words, (blit_tsc_times == 0) ? 0 : (blit_tsc_words / blit_tsc_times), (blit_tsc_words == 0) ? 0 : (blit_tsc / blit_tsc_words));  
  {ULO i; for (i = 0; i < 256; i++) if (blit_minterm_seen[i]) fprintf(F, "%.2X\n", i);}
  fclose(F);
  }
#endif
}


void blitFinishBlit(void) 
{
  blitend = -1;
  busScanEventsLevel4();
  blitterdmawaiting = 0;
  blit_started = 0;
  dmaconr = dmaconr & 0x0000bfff;
  if ((bltcon & 0x00000001) == 0x00000001)
  {
    blitterLineMode();
  }
  else
  {
    blitterCopyABCD();
  }
}

void blitMinitermsSet(ULO data)
{
  blit_minterm = data & 0x000000FF;
  //blit_asm_minterm = blit_min_functable[blit_minterm];
}

void blitForceFinish(void)
{
  if (blit_started == TRUE) 
  {
    blitFinishBlit();
  }
}

void blitterCopy(void) 
{
  blitInitiate();
	busScanEventsLevel4();
}


/*=============================*/
/* Blitter IO register stubs   */
/*=============================*/

/*======================================================*/
/* BLTCON0                                              */
/*                                                      */
/* register address is $DFF040                          */
/* blitter control register 0                           */
/* write only                                           */
/* located in Agnus                                     */
/* only used by CPU or blitter (not by DMA controller)  */
/*======================================================*/

void wbltcon0_C(ULO data, ULO address)
{
  blitForceFinish();
  bltcon = (bltcon & 0x0000FFFF) | (data << 16);
  blitMinitermsSet(data);
  blit_a_shift_asc = (data & 0x0000FFFF) >> 12;
  blit_a_shift_desc = 16 - blit_a_shift_asc;
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

void wbltcon1_C(ULO data, ULO address)
{
  blitForceFinish();
  bltcon = (bltcon & 0xFFFF0000) | (data & 0x0000FFFF);
  if ((data & 0x00000002) == 0x00000000)
  {
    // ascending mode 
    blit_desc = 0;
  }
  else
  {
    // descending mode 
    blit_desc = 1;
  }
  blit_b_shift_asc = (data & 0x0000FFFF) >> 12;
  blit_b_shift_desc = 16 - blit_b_shift_asc;
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

void wbltafwm_C(ULO data, ULO address)
{
  blitForceFinish();
  bltafwm = data;  
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

void wbltalwm_C(ULO data, ULO address)
{
  blitForceFinish();
  bltalwm = data;  
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

void wbltcpth_C(ULO data, ULO address)
{
  blitForceFinish();
  // CAUTION, BELOW IS VERY SLOW IN DEBUG MODE
  bltcpt = (bltcpt & 0x0000FFFF) | ((data & 0x0000001F) << 16);

  // THIS IS AN ALTERNATIVE FOR SPEED IN DEBUG MODE
  /*
  __asm 
  {
    push edx
    mov edx, DWORD PTR [data]
    and edx, 0x01F
    mov WORD PTR [bltcpt+2], dx
    pop edx
  }
  */
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

void wbltcptl_C(ULO data, ULO address)
{
  blitForceFinish();
  bltcpt = (bltcpt & 0xFFFF0000) | (data & 0x0000FFFE);
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

void wbltbpth_C(ULO data, ULO address)
{
  blitForceFinish();
  bltbpt = (bltbpt & 0x0000FFFF) | ((data & 0x0000001F) << 16);
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

void wbltbptl_C(ULO data, ULO address)
{
  blitForceFinish();
  bltbpt = (bltbpt & 0xFFFF0000) | (data & 0x0000FFFE);
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

void wbltapth_C(ULO data, ULO address)
{
  blitForceFinish();
  bltapt = (bltapt & 0x0000FFFF) | ((data & 0x0000001F) << 16);
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

void wbltaptl_C(ULO data, ULO address)
{
  blitForceFinish();
  bltapt = (bltapt & 0xFFFF0000) | (data & 0x0000FFFE);
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

void wbltdpth_C(ULO data, ULO address)
{
  blitForceFinish();
  bltdpt = (bltdpt & 0x0000FFFF) | ((data & 0x0000001F) << 16);
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

void wbltdptl_C(ULO data, ULO address)
{
  blitForceFinish();
  bltdpt = (bltdpt & 0xFFFF0000) | (data & 0x0000FFFE);
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

void wbltsize_C(ULO data, ULO address)
{
  blitForceFinish();
  if ((data & 0x0000003F) != 0)
  {
    blit_width = data & 0x0000003F;
  }
  else
  {
    blit_width = 64;
  }
  if (((data >> 6) & 0x000003FF) != 0)
  {
    blit_height = (data >> 6) & 0x000003FF;
  }
  else
  {
    blit_height = 1024;
  }
  if ((dmacon & 0x00000040) != 0) 
  {
    blitterCopy();
  }
  else
  {
    blitterdmawaiting = 1;
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

void wbltcon0l_C(ULO data, ULO address)
{
  blitForceFinish();
  bltcon = (bltcon & 0xFF00FFFF) | ((data << 16) & 0x00FF0000);
  blitMinitermsSet(data);
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

void wbltsizv_C(ULO data, ULO address)
{
  blitForceFinish();
  if ((data & 0x00007FFF) != 0)
  {
    blit_height = data & 0x00007FFF;
  }
  else
  {
    blit_height = 0x00008000; 
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

void wbltsizh_C(ULO data, ULO address)
{
  blitForceFinish();
  if ((data & 0x000007FF) != 0)
  {
    blit_width = data & 0x000007FF;
  }
  else
  {
    blit_width = 0x00000800; 
    // ECS increased possible blit width to 2048
    // OCS is limited to a blit height of 1024
  }
  if ((dmacon & 0x00000040) != 0) 
  {
    blitterCopy();
  }
  else
  {
    blitterdmawaiting = 1;
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

void wbltcmod_C(ULO data, ULO address)
{
  blitForceFinish();
  bltcmod = (ULO)(LON)(WOR)(data & 0x0000FFFE);
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

void wbltbmod_C(ULO data, ULO address)
{
  blitForceFinish();
  bltbmod = (ULO)(LON)(WOR)(data & 0x0000FFFE);
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

void wbltamod_C(ULO data, ULO address)
{
  blitForceFinish();
  bltamod = (ULO)(LON)(WOR)(data & 0x0000FFFE);
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

void wbltdmod_C(ULO data, ULO address)
{
  blitForceFinish();
  bltdmod = (ULO)(LON)(WOR)(data & 0x0000FFFE);
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

void wbltcdat_C(ULO data, ULO address)
{
  blitForceFinish();
  bltcdat = data;
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

void wbltbdat_C(ULO data, ULO address)
{
  blitForceFinish();
  bltbdat_original = (data & 0x0000FFFF);
  if ((blit_desc == 1) != 0)
  {
    bltbdat = (bltbdat_original << blit_b_shift_asc);
  }
  else
  {
    bltbdat = (bltbdat_original >> blit_b_shift_asc);
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

void wbltadat_C(ULO data, ULO address)
{
  blitForceFinish();
  bltadat = data;
}

#define blitterLineIncreaseX(a_shift, cpt, dpt) \
  if (a_shift < 15) a_shift++; \
  else \
  { \
    a_shift = 0; \
    cpt = (cpt + 2) & 0x1ffffe; \
    dpt = (dpt + 2) & 0x1ffffe; \
  }

#define blitterLineDecreaseX(a_shift, cpt, dpt) \
{ \
  if (a_shift == 0) \
  { \
    a_shift = 16; \
    cpt = (cpt - 2) & 0x1ffffe; \
    dpt = (dpt - 2) & 0x1ffffe; \
  } \
  a_shift--; \
}

#define blitterLineIncreaseY(cpt, dpt, cmod) \
    cpt = (cpt + cmod) & 0x1ffffe; \
    dpt = (dpt + cmod) & 0x1ffffe;

#define blitterLineDecreaseY(cpt, dpt, cmod) \
    cpt = (cpt - cmod) & 0x1ffffe; \
    dpt = (dpt - cmod) & 0x1ffffe;

/*================================================*/
/* blitterLineMode                                */
/* responsible for drawing lines with the blitter */
/*                                                */
/*================================================*/

void blitterLineMode(void)
{
  ULO bltadat_local;
  ULO bltbdat_local;
  ULO bltcdat_local = bltcdat;
  ULO bltddat_local;
  UWO mask = (UWO) ((bltbdat_original >> blit_b_shift_asc) | (bltbdat_original << (16 - blit_b_shift_asc)));

  BOOL decision_is_signed = (((bltcon >> 6) & 1) == 1);
  WOR decision_variable = (WOR) bltapt;
  WOR decision_inc_signed = (WOR) bltbmod;
  WOR decision_inc_unsigned = (WOR) bltamod;
  
  ULO bltcpt_local = bltcpt;
  ULO bltdpt_local = bltdpt;
  ULO blit_a_shift_local = blit_a_shift_asc;
  ULO bltzero_local = 0;
  ULO i;

  ULO sulsudaul = (bltcon >> 2) & 0x7;
  BOOL x_independent = (sulsudaul & 4);
  BOOL x_inc = ((!x_independent) && !(sulsudaul & 2)) || (x_independent && !(sulsudaul & 1));
  BOOL y_inc = ((!x_independent) && !(sulsudaul & 1)) || (x_independent && !(sulsudaul & 2));
  BOOL single_dot = FALSE;


  for (i = 0; i < blit_height; ++i)
  {
    // Read C-data from memory if the C-channel is enabled
    if (bltcon & 0x02000000) bltcdat_local = (memory_chip[bltcpt_local] << 8) | memory_chip[bltcpt_local + 1];

    // Calculate data for the A-channel
    bltadat_local = (bltadat & 0xffff) >> blit_a_shift_local;

    // Check for single dot
    if (x_independent) 
    {
      if (bltcon & 0x00000002)
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
    blitterMinterms(bltadat_local, bltbdat_local, bltcdat_local, bltddat_local, blit_minterm);

    // Save result to D-channel
    memory_chip[bltdpt_local] = (UBY) (bltddat_local >> 8);
    memory_chip[bltdpt_local + 1] = (UBY) (bltddat_local);

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
          blitterLineIncreaseX(blit_a_shift_local, bltcpt_local, bltdpt_local);
        }
        else
        {
          blitterLineDecreaseX(blit_a_shift_local, bltcpt_local, bltdpt_local);
        }
      }
      else
      {
        if (y_inc)
        {
          blitterLineIncreaseY(bltcpt_local, bltdpt_local, bltcmod);
        }
        else
        {
          blitterLineDecreaseY(bltcpt_local, bltdpt_local, bltcmod);
        }
        single_dot = FALSE;
      }
    }
    decision_is_signed = (decision_variable < 0);

    if (!x_independent)
    {
      // decrease/increase y
      if (y_inc) 
      {
        blitterLineIncreaseY(bltcpt_local, bltdpt_local, bltcmod);
      }
      else
      {
        blitterLineDecreaseY(bltcpt_local, bltdpt_local, bltcmod);
      }
    }
    else
    {
      if (x_inc) 
      {
        blitterLineIncreaseX(blit_a_shift_local, bltcpt_local, bltdpt_local);
      }
      else
      {
        blitterLineDecreaseX(blit_a_shift_local, bltcpt_local, bltdpt_local);
      }
    }
  }
  bltcon = bltcon & 0x0FFFFFFBF;
  if (decision_is_signed) bltcon |= 0x00000040;

  bltapt = decision_variable;
  bltcpt = bltcpt_local;
  bltdpt = bltdpt_local;
  bltzero = bltzero_local;
  wriw(0x00008040, 0x00DFF09C);
}