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

/*#define BLIT_TSC_PROFILE*/

#ifdef BLIT_TSC_PROFILE
LLO blit_tsc_tmp = 0;
LLO blit_tsc = 0;
LON blit_tsc_times = 0;
#endif

/*ULO blit_tsc_words = 0;*/

/*============================================================================*/
/* Blitter registers                                                          */
/*============================================================================*/

ULO bltcon, bltafwm, bltalwm, bltapt, bltbpt, bltcpt, bltdpt, bltamod, bltbmod;
ULO bltcmod, bltdmod, bltadat, bltbdat, bltbdat_original, bltcdat, bltzero;

/*============================================================================*/
/* Blitter cycle usage table                                                  */
/*============================================================================*/

ULO blit_cycletab[16] = {4, 4, 4, 5, 5, 5, 5, 6, 4, 4, 4, 5, 5, 5, 5, 6};

/*============================================================================*/
/* Callback table for minterm-calculation functions                           */
/*============================================================================*/

blit_min_func blit_min_functable[256];
blit_min_func blit_asm_minterm;

/*============================================================================*/
/* Blitter fill-mode lookup tables                                            */
/*============================================================================*/

UBY blit_fill[2][2][256][2];/* [inc,exc][fc][data][0 = next fc, 1 = filled data] */

/*============================================================================*/
/* Various blitter variables                                                  */
/*============================================================================*/

ULO blitend, blitterstatus;
ULO blit_height, blit_width, blitterdmawaiting;
BOOLE blitter_operation_log, blitter_operation_log_first;
ULO blit_a_shift_asc, blit_a_shift_desc;
ULO blit_b_shift_asc, blit_b_shift_desc;
ULO blit_minterm;
BOOLE blit_desc;

/*============================================================================*/
/* Function tables for different types of blitter emulation                   */
/*============================================================================*/

blitmodefunc bltlinesulsudaul[8] = {blitterlinemodeoctant0,
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
#define blitterMinterm4a(a_dat, b_dat, c_dat, d_dat) d_dat = ((~a_dat & c_dat) | (a_dat & b_dat & ~c_dat)); /* aC + ABc */
#define blitterMinterm6a(a_dat, b_dat, c_dat, d_dat) d_dat = ((c_dat & ~b_dat) | (b_dat & (a_dat ^ c_dat))); /* bC + B(A xor C) */
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
    case 0x4a: blitterMinterm4a(a_dat, b_dat, c_dat, d_dat); break; \
    case 0x6a: blitterMinterm6a(a_dat, b_dat, c_dat, d_dat); break; \
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
    memory_chip[pt_tmp] = dat >> 8; \
    memory_chip[pt_tmp + 1] = dat; \
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
    UBY dat1 = dat; \
    UBY dat2 = (dat >> 8); \
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
  ULO fwm[2] = {0xffff, bltafwm}; \
  ULO lwm[2] = {0xffff, bltalwm}; \
  UBY minterms = blit_minterm; \
  ULO fill_exclusive = (bltcon & 0x8) ? 0 : 1; \
  ULO zero_flag = 0; \
  ULO height = blit_height; \
  ULO width = blit_width; \
  BOOLE fc_original = !!(bltcon & 0x4); \
  BOOLE fill_carry; \
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
  blit_asm_minterm = blit_min_generic;
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
  for (minterm = 0xc0; minterm <= 0xc0; minterm++)
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
  blitterMinTableInit();
  blitterFillTableInit();
  blitterSetFast(FALSE);
  blitterSetECS(FALSE);
  blitterIORegistersClear();
  blitterSetOperationLog(FALSE);
  blitter_operation_log_first = TRUE;
//  verifyMinterms();
}

void blitterShutdown(void) {
#ifdef BLIT_TSC_PROFILE
  {
  FILE *F = fopen("blitprofile.txt", "w");
  fprintf(F, "FUNCTION\tTOTALCYCLES\tCALLEDCOUNT\tAVGCYCLESPERCALL\tWORDS\tWORDSPERCALL\tCYCLESPERWORD\n");
  fprintf(F, "blitter copy\t%I64d\t%d\t%I64d\t%d\t%d\t%d\n", blit_tsc, blit_tsc_times, (blit_tsc_times == 0) ? 0 : (blit_tsc / blit_tsc_times), blit_tsc_words, (blit_tsc_times == 0) ? 0 : (blit_tsc_words / blit_tsc_times), (blit_tsc_words == 0) ? 0 : (blit_tsc / blit_tsc_words));  
  fclose(F);
  }
#endif
}
