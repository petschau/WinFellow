/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Initialization of 68000 core                                               */
/*                                                                            */
/* Authors: Petter Schau (peschau@online.no)                                  */
/*          Roman Dolejsi                                                     */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

/* This file define some attributes specific to CPUs above 68030
   but there are absolutely no plans to implement working code for these CPUs */



#include "portable.h"
#include "renaming.h"

#include "defs.h"
#include "fellow.h"
#include "fmem.h"
#include "cpu.h"
#include "cpudis.h"


ULO cpu_major, cpu_minor;
BOOLE cpu_opcode_table_is_invalid;

disFunc cpu_dis_tab[65536];

/* Opcode data table */

ULO t[65536][8];

/* M68k registers */

ULO da_regs[2][8]; /* 0 - data, 1 - address */
ULO pc, usp, ssp, sr;
ULO ccr0, ccr1;

#ifdef PREFETCH

ULO prefetch_word;

#endif

/* Cycles spent by chips (Blitter) as a result of an instruction */

ULO thiscycle;

/* BCD number translation tables */

UBY bcd2hex[256], hex2bcd[256];

/* Cycle count calculation for MULX, approximately. */

UBY mulutab[256], mulstab[256];

/* Irq raise table */

UBY irq_table[0x20000];

#ifdef PC_PTR

/* pc-pcbaseadr gives actual PC-value when PC is a pointer to memory */

UBY *pcbaseadr;

#endif


/*===========================================================================*/
/* CPU properties                                                            */
/*===========================================================================*/

cpu_types cpu_type;
ULO cpu_speed;
ULO cpu_speed_property; /* cpu_speed used RT    */


/* irq management */

ULO interruptlevel = 0;
ULO interruptaddress = 0;
ULO interruptflag = 0;
ULO irq_changed;

/* Event cycle time */

ULO cpu_next, irq_next;

/* intreq bit-number to irq-level translation table */

ULO int2int[16] = {1, 1, 1, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 6, 6, 7};

/* Flag set if CPU is stopped */

ULO cpu_stop;

/* Hacks to "simplify" exceptions */

ULO exceptionstack;
ULO exceptionbackdooraddress;
ULO stopreturnadr;

/* Truth table Variables GE True when 0X0X or 1X1X */

UBY ccctt[16] = {1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1};

/* Truth table Variables LT True when 0X1X or 1X0X */

UBY ccdtt[16] = {0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0};

/* Truth table Variables GT True when 000X or 101X */

UBY ccett[16] = {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0};

/* Truth table Variables LE True when X1XX 1X0X or 0X1X */

UBY ccftt[16] = {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1};

/* Tables of pointers to ea calculation routines        */
/* These are used when initializing the opcode tables   */
/* and must be initialized first with pointers to the   */
/* routines for the selected cpu                        */

eareadfunc arb[12], arw[12], arl[12], parb[12], parw[12], parl[12];
eawritefunc awb[12], aww[12], awl[12];
eacalcfunc eac[12];

/* Tables of pointers to ea calculation routines */
/* These are for 68000 emulation opcode tables   */
/* Higher cpu emulation must provide 680x0 versions */

eareadfunc arb68000[12] = {arb00, NULL, arb02, arb03, arb04, arb05, arb06,
                           arb70, arb71, arb72, arb73, arb74};
eareadfunc arw68000[12] = {arw00, arw01, arw02, arw03, arw04, arw05, arw06,
                           arw70, arw71, arw72, arw73, arw74};
eareadfunc arl68000[12] = {arl00, arl01, arl02, arl03, arl04, arl05, arl06,
                           arl70, arl71, arl72, arl73, arl74};
eareadfunc parb68000[12] = {arb00, NULL, arb02, parb03, parb04, parb05,
                            parb06, parb70, parb71, parb72, parb73, parb74};
eareadfunc parw68000[12] = {arw00, arw01, arw02, parw03, parw04, parw05,
                            parw06, parw70, parw71, parw72, parw73, parw74};
eareadfunc parl68000[12] = {arl00, arl01, arl02, parl03, parl04, parl05,
                            parl06, parl70, parl71, parl72, parl73, parl74};
eawritefunc awb68000[12] = {awb00, NULL, awb02, awb03, awb04, awb05, awb06,
                            awb70, awb71};
eawritefunc aww68000[12] = {aww00, aww01, aww02, aww03, aww04, aww05, aww06,
                            aww70, aww71};
eawritefunc awl68000[12] = {awl00, awl01, awl02, awl03, awl04, awl05, awl06,
                            awl70, awl71};
eacalcfunc eac68000[12] = {NULL, NULL, eac02, NULL, NULL, eac05, eac06, eac70,
                           eac71, eac72, eac73, NULL};


/*====================*/
/* Extended registers */
/*====================*/

/* ssp is used for isp */

ULO vbr, sfc, dfc;              /* X12346 */
ULO caar;                       /* XX23XX */
ULO cacr;                       /* XX2346 */
ULO msp;                        /* XX234X */
ULO iacr0, iacr1, dacr0, dacr1; /* 68EC40 */
ULO pcr, buscr;                 /* XXXXX6 */


/*===================================*/
/* ea calculation tables for 68020++ */
/*===================================*/

eareadfunc arb680X0[12] = {arb00, NULL, arb02, arb03, arb04, arb05, arb063,
                           arb70, arb71, arb72, arb0733, arb74};
eareadfunc arw680X0[12] = {arw00, arw01, arw02, arw03, arw04, arw05, arw063,
                           arw70, arw71, arw72, arw0733, arw74};
eareadfunc arl680X0[12] = {arl00, arl01, arl02, arl03, arl04, arl05, arl063,
                           arl70, arl71, arl72, arl0733, arl74};
eareadfunc parb680X0[12] = {arb00, NULL, arb02, parb03, parb04, parb05,
                            parb063, parb70, parb71, parb72, parb0733, parb74};
eareadfunc parw680X0[12] = {arw00, arw01, arw02, parw03, parw04, parw05,
                            parw063, parw70, parw71, parw72, parw0733, parw74};
eareadfunc parl680X0[12] = {arl00, arl01, arl02, parl03, parl04, parl05,
                            parl063, parl70, parl71, parl72, parl0733, parl74};
eawritefunc awb680X0[12] = {awb00, NULL, awb02, awb03, awb04, awb05, awb063,
                            awb70, awb71};
eawritefunc aww680X0[12] = {aww00, aww01, aww02, aww03, aww04, aww05, aww063,
                            aww70, aww71};
eawritefunc awl680X0[12] = {awl00, awl01, awl02, awl03, awl04, awl05, awl063,
                            awl70, awl71};
eacalcfunc eac680X0[12] = {NULL, NULL, eac02, NULL, NULL, eac05, eac063, eac70,
                           eac71, eac72, eac0733, NULL};
eac063isfunc eac063istab[8] = {eac063is0, eac063is1, eac063is2, eac063is3,
 			       eac063is4, eac063is5, eac063is6, eac063is7};


/* Tables for cycle count calculation for each addressmode */
/* Must be initialized first by values appropriate for the selected cpu */

UBY tarb[12], tarw[12], tarl[12];

/* Tables for address mode cycle counts, 68000 */
/* These values are moved to tarX arrays when 68000 is selected */

const UBY tarb68000[12] = {0, 0, 4, 4, 6, 8, 10, 8, 12, 8, 10, 4};
const UBY tarw68000[12] = {0, 0, 4, 4, 6, 8, 10, 8, 12, 8, 10, 4};
const UBY tarl68000[12] = {0, 0, 8, 8, 10, 12, 14, 12, 16, 12, 14, 8};

/* Address modes included in the different classes */

const UBY allmodes[16]  = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0};
const UBY data[16]      = {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0};
const UBY memory[16]    = {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0};
const UBY control[16]   = {0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};
const UBY alterable[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};

/* Exception stack frame jmptables */

cpuStackFrameFunc cpu_stack_frame_gen[64];

/* Intel EFLAGS to 68k SR translation */

ULO statustab[4096];


ULO get_pc(ULO thispc) {
#ifdef PC_PTR
  return thispc - (ULO) pcbaseadr; /*(ULO) memory_bank_pointer[thispc >> 16];*/
#else
  return thispc;
#endif
}

#ifdef PC_PTR

/* PC entered unmapped memory */

void cpu_pc_in_unmapped_mem(void) {
  fellowSetRuntimeErrorCode(FELLOW_RUNTIME_ERROR_CPU_PC_BAD_BANK);
  fellowNastyExit();
}

#endif

/*============================*/
/* Extract fields from opcode */
/*============================*/

ULO get_smod(ULO opc) {
  return (opc & SMOD)>>3;
}

ULO get_sreg(ULO opc) {
  return (opc & SREG);
}

ULO get_dmod(ULO opc) {
  return (opc & DMOD)>>6;
}

ULO get_dreg(ULO opc) {
  return (opc & DREG)>>9;
}

STR *btab[16] = {"RA", "SR", "HI", "LS", "CC", "CS", "NE", "EQ", "VC", "VS",
		 "PL", "MI", "GE", "LT", "GT", "LE"};

ULO get_branchtype(ULO opc) {
  return (opc & FBTYPE)>>8;
}

ULO get_bit3(ULO opc) {
  return !!(opc & BIT3);
}

ULO get_bit5(ULO opc) {
  return !!(opc & BIT5);
}

ULO get_bit6(ULO opc) {
  return !!(opc & BIT6);
}

ULO get_bit8(ULO opc) {
  return !!(opc & BIT8);
}

ULO get_bit10(ULO opc) {
  return !!(opc & BIT10);
}

ULO get_bit12(ULO opc) {
  return !!(opc & BIT12);
}

/* Size: bits 6 and 7, 00 - byte, 01 - word, 10 - long, returns 8,16,32 */
/* 64 used to determine memory shifts */

ULO get_size(ULO opc) {
  if ((opc & SIZE) == 0) return 8;
  else if ((opc & SIZE) == 0x40) return 16;
  else if ((opc & SIZE) == 0x80) return 32;
  else return 64;
}

/* Size: bit 8 - 0 - word, 1 - long */

ULO get_size2(ULO opc) {
  if ((opc & BIT8) == 0) return 16;
  else return 32;
}

/* Size: bits 12 and 13, 01 - byte, 11 - word, 10 - long, returns 8,16,32 */

ULO get_size3(ULO opc) {
  if ((opc & 0x3000) == 0x1000) return 8;
  else if ((opc & 0x3000) == 0x3000) return 16;
  else return 32;
}

/* Size: bits 10 and 9, 01 - byte, 10 - word, 11 - long, returns 8,16,32 */

ULO get_size4(ULO opc) {
  if ((opc & 0x600) == 0x200) return 8;
  else if ((opc & 0x600) == 0x400) return 16;
  else return 32;
}

char sizec(ULO size) {
  return (char) ((size == 8) ? 'B' : ((size == 16) ? 'W' : 'L'));
}

/* Add regs to mode if mode is 7 */

ULO modreg(ULO modes, ULO regs) {
  return (modes != 7) ? modes : (modes + regs);
}

/* Return register ptr for mode */

ULO greg(ULO mode, ULO reg) {return reg*4 + (ULO) ((mode == 0) ? &da_regs[0][0]:&da_regs[1][0]);}

/*=============================*/
/* Common disassembly routines */
/*=============================*/

/* Common disassembly for BCHG, BCLR, BSET, BTST */

ULO btXtrans[4] = {3, 0, 1, 2};

ULO btXdis(ULO prc, ULO opc, STR *st) {
  ULO pos = 18 - (5*get_bit8(opc)), reg = get_sreg(opc),
      bitreg = get_dreg(opc), mode = get_smod(opc), sizech,
      nr = btXtrans[(opc >> 6) & 3];
  STR *bnr[4] = {"CHG","CLR","SET","TST"};
  mode = modreg(mode, reg);
  sizech = (mode == 0) ? 'L' : 'B'; 
  if (pos == 18) {
    sprintf(st, "$%.6X %.4X %.4X              B%s.%c  #$%.4X,", prc, opc,
	    fetw(prc + 2), bnr[nr], sizech,
	    fetw(prc + 2) & ((mode == 0) ? 0x1f : 7));
    prc += 2;
    }
  else sprintf(st, "$%.6X %.4X                   B%s.%c  D%1X,", prc, opc,
               bnr[nr], sizech, bitreg);
  prc = disAdrMode(reg, prc + 2, st, 8, mode, &pos);  
  return prc;
}

/* Common disassembly for ADD, SUB, CMP, AND, EOR, OR */

STR *anr[6] = {"ADD","SUB","CMP","AND","EOR","OR"};

ULO arith1dis(ULO prc, ULO opc, STR *st, int nr) {
  ULO pos = 13, reg = get_sreg(opc), dreg = get_dreg(opc), o = get_bit8(opc),
      size = get_size(opc), mode = get_smod(opc);

  mode = modreg(mode, reg);
  sprintf(st, "$%.6X %.4X                   %s.%c   ", prc, opc, anr[nr],
	  sizec(size));
  if (nr == 5) strcat(st, " ");
  prc = (!o) ? disAdrMode(reg, prc + 2, st, size, mode, &pos) :
               disAdrMode(dreg, prc + 2, st, size, 0, &pos);
  strcat(st, ",");
  prc = (o) ? disAdrMode(reg, prc, st, size, mode, &pos) :
              disAdrMode(dreg, prc, st, size, 0, &pos);
  return prc;
}

/* Common disassembly for ADDA, SUBA, CMPA */

ULO arith2dis(ULO prc, ULO opc, STR *st, int nr) {
  ULO pos = 13, reg = get_sreg(opc),dreg = get_dreg(opc), mode = get_smod(opc),
      size = get_size2(opc);
  
  mode = modreg(mode, reg);
  sprintf(st, "$%.6X %.4X                   %sA.%c  ", prc, opc, anr[nr],
	  sizec(size));
  prc = disAdrMode(reg, prc + 2, st, size, mode, &pos);
  strcat(st, ",");
  disAdrMode(dreg, prc, st, size, 1, &pos);
  return prc;
}

/* Common disassembly for ADDI, SUBI, CMPI, ANDI, EORI, ORI */

ULO arith3dis(ULO prc, ULO opc, STR *st, int nr) {
  ULO pos = 13, reg = get_sreg(opc), mode = get_smod(opc),
      size = get_size(opc);
  mode = modreg(mode, reg);
  sprintf(st, "$%.6X %.4X                   %sI.%c  ", prc, opc, anr[nr],
	  sizec(size));
  if (nr == 5) strcat(st, " ");
  prc = disAdrMode(4, prc + 2, st, size, 11, &pos);
  strcat(st, ",");
  if ((nr > 2) && (mode == 11)) strcat(st, (size == 8) ? "CCR" : "SR");
  else prc = disAdrMode(reg, prc, st, size, mode, &pos);
  return prc;
}

/* Common disassembly for ADDQ, SUBQ */

ULO arith4dis(ULO prc, ULO opc, STR *st, int nr) {
  ULO pos = 13, reg = get_sreg(opc), mode = get_smod(opc),
      size = get_size(opc);
  mode = modreg(mode, reg);
  sprintf(st, "$%.6X %.4X                   %sQ.%c  #$%.1d,", prc, opc,
	  anr[nr], sizec(size), t[opc][6]);
  prc = disAdrMode(reg, prc + 2, st, size, mode, &pos);
  return prc;
}

/* Common disassembly for ADDX, SUBX, ABCD, SBCD, CMPM */

STR *a5nr[5] = {"ADDX","SUBX","ABCD","SBCD","CMPM"};

ULO arith5dis(ULO prc, ULO opc, STR *st, int nr) {
  STR *minus = (nr == 4) ? "" : "-";
  STR *plus = (nr == 4) ? "+" : "";

  if (!get_bit3(opc)) minus = plus = "";
  
  sprintf(st, ((!get_bit3(opc)) ?
                "$%.6X %.4X                   %s.%c  %sD%d%s,%sD%d%s":
                "$%.6X %.4X                   %s.%c  %s(A%d)%s,%s(A%d)%s"),
          prc, opc, a5nr[nr], sizec(get_size(opc)), minus, get_sreg(opc),
	  plus, minus, get_dreg(opc), plus);
  return prc + 2;
}

/* Common disassembly for ASX, LSX, ROX, ROXX */

STR *shnr[4] = {"AS", "LS", "RO", "ROX"};

ULO shiftdis(ULO prc, ULO opc, STR *st, int nr) {
  ULO pos = 13, reg = get_sreg(opc), mode = get_smod(opc),
      size = get_size(opc);
  char rl = (char) ((get_bit8(opc)) ? 'L' : 'R');
  STR *spaces = (nr < 3) ? "  " : " ";

  mode = modreg(mode, reg);
  if (size == 64) {
    sprintf(st, "$%.6X %.4X                   %s%c.W%s #$1,", prc, opc,
	    shnr[nr], rl, spaces);
    prc = disAdrMode(reg, prc + 2, st, 16, mode, &pos);
    }
  else {
    char sc = sizec(size);
    if (!get_bit5(opc))
      sprintf(st, "$%.6X %.4X                   %s%c.%c%s #$%1X,D%1d", prc,
	      opc, shnr[nr], rl, sc, spaces, t[opc][2], reg);
    else  
      sprintf(st, "$%.6X %.4X                   %s%c.%c%s D%1d,D%1d", prc, opc,
	      shnr[nr],rl, sc, spaces, get_dreg(opc), reg);
    prc += 2;
    }
  return prc;
}

/* Common disassembly for CLR, NEG, NOT, TST, JMP, JSR, PEA, NBCD, NEGX */

STR *unanr[10] = {"CLR","NEG","NOT","TST","JMP","JSR","PEA","TAS", "NCBD",
		   "NEGX"};

ULO unarydis(ULO prc, ULO opc, STR *st, int nr) {
  ULO pos = 13, reg = get_sreg(opc), mode = get_smod(opc),
      size = get_size(opc);
  char dot, sizech;
  STR *spaces = (nr > 7) ? " " : "  ";
  mode = modreg(mode, reg);
  if (nr < 4 || nr > 7) {
    dot = '.';
    sizech = sizec(size);
  }
  else dot = sizech = ' ';
  sprintf(st, "$%.6X %.4X                   %s%c%c%s ", prc, opc, unanr[nr],
	  dot, sizech, spaces);
  prc = disAdrMode(reg, prc + 2, st, size, mode, &pos);
  return prc;
}

/* Common disassembly for NOP, RESET, RTE, RTR, RTS, TRAPV */

STR *singnr[6] = {"NOP","RESET","RTE","RTR","RTS","TRAPV"};

ULO singledis(ULO prc, ULO opc, STR *st, int nr) {
  sprintf(st,"$%.6X %.4X                   %s", prc, opc, singnr[nr]);  
  return prc + 2;
}

/* Common disassembly for CHK, DIVS, DIVU, LEA, MULS, MULU */

STR *var1nr[6] = {"CHK","DIVS","DIVU","LEA","MULS","MULU"};

ULO various1dis(ULO prc, ULO opc, STR *st, int nr) {
  ULO pos = 13, reg = get_sreg(opc), dreg = get_dreg(opc),
      mode = get_smod(opc);
  mode = modreg(mode, reg);
  sprintf(st, "$%.6X %.4X                   %s.%s  ", prc, opc, var1nr[nr],
	  (nr == 3) ? "L ":"W");
  prc = disAdrMode(reg, prc + 2, st, 16, mode, &pos);
  strcat(st, ",");
  disAdrMode(dreg, prc, st, 16, (nr != 3) ? 0 : 1, &pos);  
  return prc;
}

/* Common disassembly for SWAP, UNLK */

STR *var2nr[2] = {"SWAP","UNLK"};

ULO various2dis(ULO prc, ULO opc, STR *st, int nr) {
  char regtype = (char) ((nr == 0) ? 'D' : 'A');
  sprintf(st, "$%.6X %.4X                   %s    %c%1X", prc, opc, var2nr[nr],
	  regtype, get_sreg(opc));  
  return prc+2;
}

/*  Illegal instruction */

ULO i00dis(ULO prc, ULO opc, STR *st) {
  sprintf(st, "$%.6X %.4X                   ILLEGAL INSTRUCTION", prc, opc);
  return prc + 2;
}

/* Instruction ABCD.B Ry,Rx / ABCD.B -(Ay),-(Ax)
   Table: 0 - Routinepointer 1 - Rx*4(dest) 2 - Ry*4 */

ULO i01dis(ULO prc, ULO opc, STR *st) {
  return arith5dis(prc, opc, st, 2);
}

void i01ini(void) {
  ULO op = 0xc100, ind, rm, rx, ry;

  for (rm = 0; rm < 2; rm++) 
    for (rx = 0; rx < 8; rx++) 
      for (ry = 0; ry < 8; ry++) {
        ind = op | ry | (rx<<9) | (rm<<3);
        t[ind][0] = (rm == 0) ? (ULO) i01_1 : (ULO) i01_2;
        cpu_dis_tab[ind] = i01dis;
        t[ind][1] = greg(rm, rx);
        t[ind][2] = greg(rm, ry);
        }
}

/* Instruction ADD.X <ea>,Dn / ADD.X Dn,<ea>
   Table:
   Type 1:  add <ea>,Dn
     0-Routinepointer 1-disasm routine  2-sourreg 3-sourceroutine
                      4-Dreg        5-cycle count                   
   Type 2:  add Dn,<ea>
     0-Routinepointer 1-disasm routine  2-<ea>reg 3-get <ea> routine
                      4-Dreg        5-cycle count   6-write<ea> routine */

ULO i02dis(ULO prc, ULO opc, STR *st) {
  return arith1dis(prc, opc, st, 0);
}

void i02ini(void) {
  ULO op = 0xd000, ind, regd, modes, regs, flats, size, o;
  for (o = 0; o < 2; o++)
    for (size = 0; size < 3; size++)
      for (regd = 0; regd < 8; regd++) 
        for (modes = 0; modes < 8; modes++) 
          for (regs = 0; regs < 8; regs++) {
            flats = modreg(modes, regs);
            if ((o == 0 && size == 0 && data[flats]) ||
                (o == 0 && size != 0 && allmodes[flats]) ||
                (o == 1 && alterable[flats] && memory[flats])) {
              ind = op | (regd<<9) | (o<<8) | (size<<6) | (modes<<3) | regs;
		t[ind][0] = (o == 0) ?
                             (size == 0) ?
                                (ULO) i02_B_1:
                                (size == 1) ?
                                   (ULO) i02_W_1:
                                   (ULO) i02_L_1:
                             (size == 0) ?
                                (ULO) i02_B_2:
                                (size == 1) ?
                                   (ULO) i02_W_2:
                                   (ULO) i02_L_2;
              t[ind][2] = greg(modes, regs);
              t[ind][3] = (ULO) ((o == 0) ?
                             (size == 0) ?
                                arb[flats]:
                                (size == 1) ?
                                   arw[flats]:
                                   arl[flats]:
                             (size == 0) ?
                                parb[flats]:
                                (size == 1) ?
                                   parw[flats]:
                                   parl[flats]);
              t[ind][4] = greg(0, regd);
              t[ind][5] = (o == 0) ?
                             (size == 0) ?
                                (4+tarb[flats]):
                                (size == 1) ?
                                   (4+tarw[flats]):
                                   (flats <= 1 || flats == 11) ?
                                      (8+tarl[flats]):
                                      (6+tarl[flats]):
                             (size == 0) ?
                                (8 + tarb[flats]):
                                (size == 1) ?
                                   (8 + tarw[flats]):
                                   (12 + tarl[flats]);
              if (o == 1) t[ind][6] = (ULO) ((size == 0) ?
                                         awb[flats]:
                                         (size == 1) ?
                                            aww[flats]:
                                            awl[flats]);
              cpu_dis_tab[ind] = i02dis;
	    }
            }
}

/* Instruction ADDA.W/L <ea>,An
   Table-usage:

   0-Routinepointer 1-disasm routine  2-sourreg 3-sourceroutine
                    4-Areg        5-cycle count                 */

ULO i03dis(ULO prc, ULO opc, STR *st) {
  return arith2dis(prc, opc, st, 0);
}

void i03ini(void) {
  ULO op = 0xd0c0, ind, regd, modes, regs, flats, o;
  for (o = 0; o < 2; o++)
    for (regd = 0; regd < 8; regd++) 
      for (modes = 0; modes < 8; modes++) 
        for (regs = 0; regs < 8; regs++) 
          if (allmodes[flats = modreg(modes, regs)]) {
            ind = op | (regd<<9) | (o<<8) | (modes<<3) | regs;
	      t[ind][0] = (o == 0) ? ((ULO) i03_W) : ((ULO) i03_L);
	      t[ind][2] = greg(modes, regs);
	      t[ind][3] = (ULO) ((o == 0) ? arw[flats] : arl[flats]);
	      t[ind][4] = greg(1, regd);
	      t[ind][5] = (o==0) ?
		(8 + tarw[flats]):
		(flats <= 1 || flats == 11) ?
		(8 + tarl[flats]):
		(6 + tarl[flats]);
	    cpu_dis_tab[ind] = i03dis;
	  }
}

/* Instruction ADDI.X #i,<ea>
   Table-usage:

   0-Routinepointer 1-disasm routine  2-eareg 3-ea read routine
                    4-cycle count    5-ea write routine         */

ULO i04dis(ULO prc, ULO opc, STR *st) {
  return arith3dis(prc, opc, st, 0);
}

void i04ini(void) {
  ULO op=0x0600,ind,modes,regs,flats,size;
  for (size = 0; size < 3; size++)
    for (modes = 0; modes < 8; modes++) 
      for (regs = 0; regs < 8; regs++) {
        flats = modreg(modes, regs);
        if (data[flats] && alterable[flats]) {
          ind = op | (size<<6) | (modes<<3) | regs;
	    t[ind][0] = (size == 0) ?
	      (ULO) i04_B:
	      (size == 1) ?
	      (ULO) i04_W:
	      (ULO) i04_L; 
	    t[ind][2] = greg(modes, regs);
	    t[ind][3] = (ULO) ((size == 0) ?
			       parb[flats]:
			       (size == 1) ?
			       parw[flats]:
			       parl[flats]);
	    t[ind][4] = (size == 0) ?
	      (flats == 0) ?
	      8:
	    (12 + tarb[flats]):
	      (size == 1)  ?
	      (flats == 0) ?
	      8:
	    (12 + tarw[flats]):
	      (flats == 0) ?
	      16:
	    (20 + tarl[flats]);
	    t[ind][5] = (ULO) ((size == 0) ?
			       awb[flats]:
			       (size == 1) ? 
			       aww[flats]:
			       awl[flats]);
          cpu_dis_tab[ind] = i04dis;
	}
      }
}

/* Instruction ADDQ.X #i,<ea>
   Table-usage:

   0-Routinepointer 1-disasm routine  2-eareg 3-ea read routine
                4-cycle count    5-ea write routine 6-add immediate */

ULO i05dis(ULO prc, ULO opc, STR *st) {
  return arith4dis(prc, opc, st, 0);
}

void i05ini(void) {
  ULO op = 0x5000, ind, imm, modes, regs, flats, size;
    for (size = 0; size < 3; size++)
      for (modes = 0; modes < 8; modes++) 
        for (regs = 0; regs < 8; regs++) 
          for (imm = 0; imm < 8; imm++) {
            flats = modreg(modes, regs);
            if ((size == 0 && alterable[flats] && modes != 1) ||
                (size != 0 && alterable[flats])) {
              ind = op | (imm<<9) | (size<<6) | (modes<<3) | regs;
              if (modes == 1 && (size > 0)) t[ind][0] = (ULO) i05_ADR;
              else
                t[ind][0] = (ULO) ((size == 0) ? i05_B:
				   (size == 1) ? i05_W:i05_L);
              cpu_dis_tab[ind] = i05dis;
              t[ind][2] = greg(modes, regs);
              t[ind][3] = (ULO)((size == 0) ?
                             parb[flats]:
                             (size == 1) ?
                                parw[flats]:
                                parl[flats]);
              t[ind][4] = (size == 0) ?
                             (flats == 0) ?
                                (4+tarb[flats]):
                                (8+tarb[flats]):
                             (size == 1) ?
                                (flats == 0) ?
                                   (4+tarw[flats]):
                                   (8+tarw[flats]):
                                (flats < 2) ?
                                   (8+tarl[flats]):
                                   (12+tarl[flats]);
              t[ind][5] = (ULO)((size == 0) ?
                             awb[flats]:
                             (size == 1) ?
                                aww[flats]:
                                awl[flats]);
              t[ind][6] = (imm == 0) ?
                             8:
                             imm;
              }
            }
}

/* Instruction ADDX.X Ry,Rx / ADD.X -(Ay),-(Ax)
   Table-usage: 0 -Routinepointer 1 - disasm routine 2 - Rx*4(dest) 3 - Ry*4 */

ULO i06dis(ULO prc, ULO opc, STR *st) {
  return arith5dis(prc, opc, st, 0);
}

void i06ini(void) {
  ULO op=0xd100, ind, rm, rx, ry, sz;
  for (sz = 0; sz <= 2; sz++)
    for (rm = 0; rm <= 1; rm++)
      for (rx = 0; rx <= 7; rx++) 
        for (ry = 0; ry <= 7; ry++) {
          ind = op | ry | (rx<<9) | (rm<<3) | (sz<<6);
          t[ind][0] = (rm == 0) ?
                       ((sz == 0) ?
                          (ULO) i06_B_1:
                          (sz == 1) ?
                             (ULO) i06_W_1:
                             (ULO) i06_L_1):
                       ((sz == 0) ?
                          (ULO) i06_B_2:
                          (sz == 1) ?
                             (ULO) i06_W_2:
                             (ULO) i06_L_2);
        cpu_dis_tab[ind] = i06dis;
        t[ind][2] = greg(rm, rx);
        t[ind][3] = greg(rm, ry);
        }
}

/* Instruction AND.X <ea>,Dn / AND.X Dn,<ea>
   Table-usage:
   Type 1:  and <ea>,Dn

     0-Routinepointer 1-disasm routine  2-sourreg 3-sourceroutine
                      4-Dreg        5-cycle count                   
   Type 2:  and Dn,<ea>

     0-Routinepointer 1-disasm routine  2-<ea>reg 3-get <ea> routine
                      4-Dreg        5-cycle count   6-write<ea> routine */

ULO i07dis(ULO prc, ULO opc, STR *st) {
  return arith1dis(prc, opc, st, 3);
}

void i07ini(void) {
  ULO op = 0xc000, ind, regd, modes, regs, flats, size, o;
  for (o = 0; o < 2; o++)
    for (size = 0; size < 3; size++)
      for (regd = 0; regd < 8; regd++) 
        for (modes = 0; modes < 8; modes++) 
          for (regs = 0; regs < 8; regs++) {
            flats = modreg(modes, regs);
            if ((o==0 && data[flats]) ||
                (o==1 && alterable[flats] && memory[flats])) {
              ind = op | (regd<<9) | (o<<8) | (size<<6) | (modes<<3) | regs;
              t[ind][0] = (o == 0) ?
                             (size == 0) ?
                                (ULO) i07_B_1:
                                (size == 1) ?
                                   (ULO) i07_W_1:
                                   (ULO) i07_L_1:
                             (size == 0) ?
                                (ULO) i07_B_2:
                                (size == 1) ?
                                   (ULO) i07_W_2:
                                   (ULO) i07_L_2;
              t[ind][2] = greg(modes, regs);
              t[ind][3] = (ULO)((o == 0) ?
                             (size == 0) ?
                                arb[flats]:
                                (size == 1) ?
                                   arw[flats]:
                                   arl[flats]:
                             (size == 0) ?
                                parb[flats]:
                                (size == 1) ?
                                   parw[flats]:
                                   parl[flats]);
              t[ind][4] = greg(0, regd);
              t[ind][5] = (o == 0) ?
                             (size == 0) ?
                                (4+tarb[flats]):
                                (size == 1) ?
                                   (4+tarw[flats]):
                                   (flats <= 1 || flats == 11) ?
                                      (8+tarl[flats]):
                                      (6+tarl[flats]):
                             (size == 0) ?
                                (8 + tarb[flats]):
                                (size == 1) ?
                                   (8 + tarw[flats]):
                                   (12 + tarl[flats]);
              if (o == 1) t[ind][6] = (ULO)((size == 0) ?
                                         awb[flats]:
                                         (size == 1) ?
                                            aww[flats]:
                                            awl[flats]);
	    cpu_dis_tab[ind] = i07dis;
	    }
	  }
}



/* Instruction ANDI.X #i,<ea>
   Table-usage:

   0-Routinepointer 1-disasm routine  2-eareg 3-ea read routine
                    4-cycle count    5-ea write routine               */

ULO i08dis(ULO prc, ULO opc, STR *st) {
  return arith3dis(prc, opc, st, 3);
}

void i08ini(void) {
  ULO op = 0x0200, ind, modes, regs, flats, size;
  for (size = 0; size < 3; size++)
    for (modes = 0; modes < 8; modes++) 
      for (regs = 0; regs < 8; regs++) {
	flats = modreg(modes, regs);
        if ((data[flats] && alterable[flats]) || (flats == 11 && size < 2)) {
          ind = op | (size<<6) | (modes<<3) | regs;
          t[ind][0] = (size == 0) ?
                         ((flats == 11) ?
                           (ULO) i08_B_CCR:
                           (ULO) i08_B):
                         (size == 1) ?
                            ((flats == 11) ?
                              (ULO) i08_W_SR:
                              (ULO) i08_W):
                            (ULO) i08_L; 
          t[ind][2] = greg(modes, regs);
          t[ind][3] = (ULO)((size == 0) ?
                         parb[flats]:
                         (size == 1) ?
                            parw[flats]:
                            parl[flats]);
          t[ind][4] = (flats == 11) ?
                        20:
                        (size == 0) ?
                         (flats == 0) ?
                            8:
                            (12 + tarb[flats]):
                         (size == 1)  ?
                            (flats == 0) ?
                               8:
                               (12 + tarw[flats]):
                            (flats == 0) ?
                               16:
                               (20 + tarl[flats]);
          t[ind][5] = (ULO)((size == 0) ?
                         awb[flats]:
                         (size == 1) ? 
                            aww[flats]:
                            awl[flats]);
          cpu_dis_tab[ind] = i08dis;
        }
      }
}

/* Instruction ASL/ASR.X #i,<ea>
   3 versions: 
   Table-usage:

   ASX.X Dx,Dy
    0-Routinepointer 1-disasm routine  2-shcountreg 3-dreg
                     4-cycle count               

   ASX.X #,Dy
    0-Routinepointer 1-disasm routine  2-shiftcount 3-dreg
                     4-cycle count               

   ASX.X #1,<ea>
    0-Routinepointer 1-disasm routine  2-eareg 3-earead
                     4-eawrite        5-cycle count         */

ULO i09dis(ULO prc, ULO opc, STR *st) {
  return shiftdis(prc, opc, st, 0);
}

void i09ini(void) {
  ULO op = 0xe000, ind, dir, modes, regs, regc, flats, size, immreg;
  for (size = 0; size < 3; size++)
    for (regc = 0; regc < 8; regc++) 
      for (dir = 0; dir < 2; dir++) 
        for (regs = 0; regs < 8; regs++)
          for (immreg = 0; immreg < 2; immreg++) {
            ind = op | (regc<<9) | (dir<<8) | (size<<6) | (immreg<<5) | regs;
            t[ind][0] = (size == 0) ?
                           ((immreg==0) ?
                             ((dir==0) ?
                               (ULO) i09_RI_I_B:
                               (ULO) i09_LE_I_B):
                             ((dir==0) ?
                               (ULO) i09_RI_R_B:
                               (ULO) i09_LE_R_B)):
                           ((size == 1) ?
                             ((immreg==0) ?
                               ((dir==0) ?
                                 (ULO) i09_RI_I_W:
                                 (ULO) i09_LE_I_W):
                               ((dir==0) ?
                                 (ULO) i09_RI_R_W:
                                 (ULO) i09_LE_R_W)):
                             ((immreg==0) ?
                               ((dir==0) ?
                                 (ULO) i09_RI_I_L:
                                 (ULO) i09_LE_I_L):
                               ((dir==0) ?
                                 (ULO) i09_RI_R_L:
                                 (ULO) i09_LE_R_L)));
            cpu_dis_tab[ind] = i09dis;
            t[ind][2] = (immreg == 0) ?
                          ((regc == 0) ?
                            8:
                            regc):
                          greg(0, regc);
            t[ind][3] = greg(0, regs);
            t[ind][4] = (size == 3) ?
                          ((immreg == 0) ?
                            (8+(t[ind][2]*2)):
                            8):
                          ((immreg == 0) ?
                            (6+(t[ind][2]*2)):
                            6);
            }

  op = 0xe0c0;
  for (dir = 0; dir < 2; dir++) 
    for (modes = 0; modes < 8; modes++) 
      for (regs = 0; regs < 8; regs++) {
        flats = modreg(modes, regs);
        if (memory[flats] && alterable[flats]) {
          ind = op | (dir<<8) | (modes<<3) | regs;
	    t[ind][0] = (dir == 0) ? (ULO) i09_RI_M_W : (ULO) i09_LE_M_W;
	    t[ind][2] = greg(modes, regs);
	    t[ind][3] = (ULO) parw[flats];
	    t[ind][4] = (ULO) aww[flats];
	    t[ind][5] = 8+tarw[flats];
          cpu_dis_tab[ind] = i09dis;
	}
      }
}

/*===========================================================================
   Instruction Bcc
   Table-usage:

   0-Routinepointer 1-displacement 32 bit 2-handle routine 3-PC add (0/2/4)
  ===========================================================================*/

ULO i11dis(ULO prc, ULO opc, STR *st) {
  ULO j, adr;
  int disp = (int)(signed char) (opc & 0xff);

  sprintf(st, "$%.6X %.4X                   B%s.%c  ", prc, opc,
	  btab[get_branchtype(opc)],
	  (disp == -1) ? 'L':(disp == 0) ? 'W' : 'B');
  if (disp == 0) {
    prc += 2;
    j = fetw(prc);
    sprintf(&st[13], "%4.4X", j);
    st[17] = ' ';
    adr = (j > 32767) ? (prc + j - 65536) : (prc + j);
    }
  else if (disp == -1 && cpu_major >= 2) {
    prc += 2;
    j = fetl(prc);
    sprintf(&st[13], "%8.8X", j);
    st[21] = ' ';
    adr = prc + j;
    }
  else adr = prc + 2 + disp;
  sprintf(&st[36], "   $%6.6X", adr);
  return prc + 2;
}

void i11ini(void) {
  ULO op = 0x6000, c, ind;
  int di;
  ULO routines[16] = {(ULO) cc0,(ULO) cc1,(ULO) cc2,(ULO) cc3,
		     (ULO) cc4,(ULO) cc5,(ULO) cc6,(ULO) cc7,
		     (ULO) cc8,(ULO) cc9,(ULO) cca,(ULO) ccb,
		     (ULO) ccc,(ULO) ccd,(ULO) cce,(ULO) ccf};
  for (c = 0; c < 16; c++)
    for (di = -128; di < 128; di++) {
      ind = op | (di & 0xff) | (c<<8);
	if (cpu_major >= 2) {
	  t[ind][0] = (ULO) ((di == 0) ? i11_W:((di == -1) ? i11_L:i11_B));
	  t[ind][3] = (ULO) ((di == 0) ? 2:((di == -1) ? 4:0));
	}
	else {
	  t[ind][0] = (ULO) ((di == 0) ? i11_W:i11_B);
	  t[ind][3] = (ULO) ((di == 0) ? 2 : 0);
	}
	t[ind][1] = di;
	t[ind][2] = routines[c];
      cpu_dis_tab[ind] = i11dis;
    }
}

/* Instructions BCHG.B/L BCLR BSET BTST
   Table-usage dynamic:
     2 - eareg 3-earead 4-reg 5-cycle count 6-eawrite
   Table-usage static:
     2 - eareg 3-earead 4-cycle count 5-eawrite */

ULO i12dis(ULO prc, ULO opc, STR *st) {
  return btXdis(prc, opc, st);
}

int btXadrselect(int nr, int dynamic, int mode) {
  if (nr == 3) {
    if (dynamic) return data[mode];
    else if (mode == 11) return FALSE;
    else return data[mode];
  }
  return data[mode] && alterable[mode];
}

void btXini(int o, ULO dLfunc, ULO dBfunc, ULO sLfunc, ULO sBfunc,
	    int dc1, int dc2, int sc1, int sc2) {
  ULO op = o, ind, breg, mode, reg, flat, nr = btXtrans[(o >> 6) & 3];

  for (breg = 0; breg < 8; breg++) /* Dynamic */
    for (mode = 0; mode < 8; mode++) 
      for (reg = 0; reg < 8; reg++) {
        flat = modreg(mode, reg);
        if (btXadrselect(nr, TRUE, flat)) {
          ind = op | (breg<<9) | (mode<<3) | reg;
	    t[ind][0] = (mode == 0) ? dLfunc:dBfunc;
	    t[ind][2] = greg(mode, reg);
	    t[ind][3] = (ULO) ((nr != 3) ? parb[flat]:arb[flat]);
	    t[ind][4] = greg(0, breg);
	    t[ind][5] = (mode == 0) ? dc1:(dc2 + tarb[flat]);
	    t[ind][6] = (ULO) awb[flat];
	  cpu_dis_tab[ind] = i12dis;
	}
      }
  op = (o & 0x00ff) | 0x800;
  for (mode = 0; mode < 8; mode++) /* Static */
    for (reg = 0; reg < 8; reg++) {
      flat = modreg(mode, reg);
      if (btXadrselect(nr, FALSE, flat)) {
        ind = op | (mode<<3) | reg;
	  t[ind][0] = (mode == 0) ? sLfunc:sBfunc;
	  t[ind][2] = greg(mode, reg);
	  t[ind][3] = (ULO) ((nr != 3) ? parb[flat]:arb[flat]);
	  t[ind][4] = (mode == 0) ? sc1 : (sc2 + tarb[flat]);
	  t[ind][5] = (ULO) awb[flat];
	cpu_dis_tab[ind] = i12dis;
      }
    }
}

void i12ini(void) {
  btXini(0x0140, (ULO) i12_D_L, (ULO) i12_D_B, (ULO) i12_S_L, (ULO) i12_S_B,
	 8, 8, 12, 12); /* BCHG */
  btXini(0x0180, (ULO) i13_D_L, (ULO) i13_D_B, (ULO) i13_S_L, (ULO) i13_S_B,
	 10, 8, 14, 12); /* BCLR */
  btXini(0x01c0, (ULO) i15_D_L, (ULO) i15_D_B, (ULO) i15_S_L, (ULO) i15_S_B,
	 8, 8, 12, 12); /* BSET */
  btXini(0x0100, (ULO) i17_D_L, (ULO) i17_D_B, (ULO) i17_S_L, (ULO) i17_S_B,
	 6, 4, 10, 8); /* BTST */
}

/* Instruction CHK.W <ea>,Dn
   Table-usage:

    0-Routinepointer 1-disasm routine  2-sourreg 3-sourceroutine
                     4-Dreg        5 - cycle count             */

ULO i18dis(ULO prc, ULO opc, STR *st) {
  return various1dis(prc, opc, st, 0);
}

void i18ini(void) {
  ULO op = 0x4180, ind, regd, modes, regs, flats;
  for (regd = 0; regd < 8; regd++) 
    for (modes = 0; modes < 8; modes++) 
      for (regs = 0; regs < 8; regs++) 
        if (data[(flats = modreg(modes, regs))]) {
          ind = op | (regd<<9) | (modes<<3) | regs;
          t[ind][0] = (ULO) i18;
          cpu_dis_tab[ind] = i18dis;
          t[ind][2] = greg(modes, regs);
          t[ind][3] = (ULO) arw[flats];
          t[ind][4] = greg(0, regd);
          t[ind][5] = 10+tarw[flats];
          }
}

/* Instruction CLR.X <ea>
   Table-usage:
    0-Routinepointer 1-disasm routine  2-sourreg 3-sourceroutine
                     4-cycle count                   */

ULO i19dis(ULO prc, ULO opc, STR *st) {
  return unarydis(prc, opc, st, 0);
}

void i19ini(void) {
  ULO op=0x4200,ind,modes,regs,flats,size;
  for (size = 0; size < 3; size++)
    for (modes = 0; modes < 8; modes++) 
      for (regs = 0; regs < 8; regs++) {
        flats = modreg(modes, regs);
        if (data[flats] && alterable[flats]) {
          ind = op | (size<<6) | (modes<<3) | regs;
	  t[ind][0] = (ULO) i19;
	  t[ind][2] = greg(modes, regs);
	  t[ind][3] = (ULO)((size == 0) ?
			    awb[flats]:
			    (size == 1) ?
			    aww[flats]:
			    awl[flats]);
	  t[ind][4] = (size == 0) ?
	    4+tarb[flats]:
	    (size == 1) ?
	    (4+tarw[flats]):
	    (modes == 0) ?
	    6:
	  (4+tarl[flats]);
        cpu_dis_tab[ind] = i19dis;
	}
      }
}

/* Instruction CMP.X <ea>,Dn
   Table-usage:
   Type 1:  CMP <ea>,Dn
    0-Routinepointer 1-disasm routine  2-sourreg 3-sourceroutine
                   4-Dreg        5-cycle count                   */

ULO i20dis(ULO prc, ULO opc, STR *st) {
  return arith1dis(prc, opc, st, 2);
}  

void i20ini(void) {
  ULO op=0xb000,ind,regd,modes,regs,flats,size;
  for (size = 0; size < 3; size++)
    for (regd = 0; regd < 8; regd++) 
      for (modes = 0; modes < 8; modes++) 
        for (regs = 0; regs < 8; regs++) {
          flats = modreg(modes, regs);
          if ((size == 0 && data[flats]) ||
              (size != 0 && allmodes[flats])) {
            ind = op | (regd<<9) | (size<<6) | (modes<<3) | regs;
            t[ind][0] = (size == 0) ?
                           (ULO) i20_B:
                           (size == 1) ?
                              (ULO) i20_W:
                              (ULO) i20_L;
            t[ind][2] = greg(modes, regs);
            t[ind][3] = (ULO)((size == 0) ?
                           arb[flats]:
                           (size == 1) ?
                              arw[flats]:
                              arl[flats]);
            t[ind][4] = greg(0, regd);
            t[ind][5] = (size == 0) ?
                           (4+tarb[flats]):
                           (size == 1) ?
                              (4+tarw[flats]):
                              (6+tarl[flats]);
	  cpu_dis_tab[ind] = i20dis;
	  }
	}
}

/* Instruction CMPA.W/L <ea>,An
   Table-usage:

    0-Routinepointer 1-disasm routine  2-sourreg 3-sourceroutine
                     4-Areg        5-cycle count                   */

ULO i21dis(ULO prc,ULO opc,STR *st) {
  return arith2dis(prc, opc, st, 2);
}

void i21ini(void) {
  ULO op=0xb0c0,ind,regd,modes,regs,flats,o;
  for (o = 0; o < 2; o++)
    for (regd = 0; regd < 8; regd++) 
      for (modes = 0; modes < 8; modes++) 
        for (regs = 0; regs < 8; regs++) 
          if (allmodes[(flats = modreg(modes, regs))]) {
            ind = op | (regd<<9) | (o<<8) | (modes<<3) | regs;
	      t[ind][0] = (o == 0) ?
		(ULO) i21_W:
		(ULO) i21_L;
	      t[ind][2] = greg(modes, regs);
	      t[ind][3] = (ULO)((o == 0) ?
				arw[flats]:
				arl[flats]);
	      t[ind][4] = greg(1, regd);
	      t[ind][5] = (o==0) ?
		(6 + tarw[flats]):
		(6 + tarl[flats]);
	    cpu_dis_tab[ind] = i21dis;
	  }
}


/*===========================================================================
  Instruction CMPI.X #imm,<ea>

  Table-usage:

  0 - Routinepointer
  1 - EA register
  2 - EA read routine
  3 - Cycle count
  ===========================================================================*/

ULO i22dis(ULO prc, ULO opc, STR *st) {
  return arith3dis(prc, opc, st, 2);
}

void i22ini(void) {
  ULO op = 0x0c00, ind, modes, regs, flats, size;

  for (size = 0; size < 3; size++)
    for (modes = 0; modes < 8; modes++) 
      for (regs = 0; regs < 8; regs++) {
        flats = modreg(modes, regs);
        if ((data[flats] && alterable[flats]) ||
	    ((cpu_major >= 2) && (flats == 9 || flats == 10))) {
          ind = op | (size<<6) | (modes<<3) | regs;
          t[ind][0] = (size == 0) ?
                         (ULO) i22_B:
                         (size == 1) ?
                            (ULO) i22_W:
                            (ULO) i22_L; 
          t[ind][1] = greg(modes, regs);
          t[ind][2] = (ULO)((size == 0) ?
                         arb[flats]:
                         (size == 1) ?
                            arw[flats]:
                            arl[flats]);
          t[ind][3] = (size == 0) ?
                         (8 + tarb[flats]):
                         (size == 1)  ?
                            (8 + tarw[flats]):
                            (flats == 0) ?
                               14:
                               (12 + tarl[flats]);
          cpu_dis_tab[ind] = i22dis;
        }
      }
}

/* Instruction CMPM.X (Ay)+,(Ax)+
   Table-usage: 0 - Routinepointer 1 - disasm routine 2 - sou 3 - dest */

ULO i23dis(ULO prc, ULO opc, STR *st) {
  return arith5dis(prc, opc, st, 4);
}

void i23ini(void) {
  ULO op=0xb108,ind,rx,ry,sz;
  for (sz = 0; sz <= 2; sz++)
    for (rx = 0; rx <= 7; rx++) 
      for (ry = 0; ry <= 7; ry++) {
        ind = op | ry | (rx<<9) | (sz<<6);
        t[ind][0] = (sz == 0) ?
                      (ULO) i23_B:
                      (sz == 1) ?
                        (ULO) i23_W:
                        (ULO) i23_L;
        cpu_dis_tab[ind] = i23dis;
        t[ind][2] = greg(1, ry);
        t[ind][3] = greg(1, rx);
        }
}
/* Instruction DBcc Dn,offset
   Table-usage:

   0-Routinepointer 1-disasm routine  2-displacement 32 bit 3-handle routine */

ULO i24dis(ULO prc, ULO opc, STR *st) {
  ULO j, adr, bratype = get_branchtype(opc);
  prc += 2;
  j = fetw(prc);
  adr = (j > 32767) ?
           (prc+j-65536):
           (prc+j);
  sprintf(st,"$%.6X %.4X %.4X              DB",prc-2,opc,j);

  if (bratype == 0) strcat(st, "T    ");
  else if (bratype == 1) strcat(st, "F    ");
  else {
    strcat(st, btab[bratype]);
    strcat(st, "   ");
  }
  sprintf(&st[36],"   D%1d,$%6.6X", get_sreg(opc),adr);
  return prc+2;
}


void i24ini(void) {
  ULO op=0x50c8,c,ind,reg;
  ULO routines[16] = {(ULO) cc0,(ULO) cc1false,(ULO) cc2,(ULO) cc3,(ULO) cc4,
		      (ULO) cc5,(ULO) cc6,(ULO) cc7,(ULO) cc8,(ULO) cc9,
		      (ULO) cca,(ULO) ccb,(ULO) ccc,(ULO) ccd,(ULO) cce,
		      (ULO) ccf};
  for (c = 0; c < 16; c++)
    for (reg = 0; reg < 8; reg++) {
      ind = op | reg | (c<<8);
      t[ind][0] = (ULO) i24;
      cpu_dis_tab[ind] = i24dis;
      t[ind][2] = greg(0, reg);
      t[ind][3] = routines[c];
      }
}

/* Instruction DIVS.W <ea>,Dn
   Table-usage:

    0-Routinepointer 1-disasm routine  2-sourreg 3-sourceroutine
                     4-Dreg                          */

ULO i25dis(ULO prc, ULO opc, STR *st) {
  return various1dis(prc, opc, st, 1);
}

void i25ini(void) {
  ULO op = 0x81c0, ind, regd, modes, regs, flats;
  for (regd = 0; regd < 8; regd++) 
    for (modes = 0; modes < 8; modes++) 
      for (regs = 0; regs < 8; regs++) 
        if (data[(flats = modreg(modes, regs))]) {
          ind = op | (regd<<9) | (modes<<3) | regs;
          t[ind][0] = (ULO) i25;
          cpu_dis_tab[ind] = i25dis;
          t[ind][2] = greg(modes, regs);
          t[ind][3] = (ULO) arw[flats];
          t[ind][4] = greg(0, regd);
          }
}

/* Instruction DIVU.W <ea>,Dn
   Table-usage:

    0-Routinepointer 1-disasm routine  2-sourreg 3-sourceroutine
                     4-Dreg                          */

ULO i26dis(ULO prc, ULO opc, STR *st) {
  return various1dis(prc, opc, st, 2);
}

void i26ini(void) {
  ULO op=0x80c0,ind,regd,modes,regs,flats;
  for (regd = 0; regd < 8; regd++) 
    for (modes = 0; modes < 8; modes++) 
      for (regs = 0; regs < 8; regs++) 
        if (data[(flats = modreg(modes, regs))]) {
          ind = op|regd<<9|modes<<3|regs;
          t[ind][0] = (ULO) i26;
          cpu_dis_tab[ind] = i26dis;
          t[ind][2] = greg(modes, regs);
          t[ind][3] = (ULO) arw[flats];
          t[ind][4] = (ULO) &da_regs[0][regd];
          }
}

/* Instruction EOR.X <ea>,Dn / EOR.X Dn,<ea>
   Table-usage:
     EOR Dn,<ea>
       0-Routinepointer 1-disasm routine  2-<ea>reg 3-get <ea> routine
                        4-Dreg        5-cycle count   6-write<ea> routine */

ULO i27dis(ULO prc, ULO opc, STR *st) {
  return arith1dis(prc, opc, st, 4);
}

void i27ini(void) {
  ULO op=0xb100,ind,regd,modes,regs,flats,size;
  for (size = 0; size < 3; size++)
    for (regd = 0; regd < 8; regd++) 
      for (modes = 0; modes < 8; modes++) 
        for (regs = 0; regs < 8; regs++) {
          flats = modreg(modes, regs);
          if (data[flats] && alterable[flats]) {
            ind = op|regd<<9|size<<6|modes<<3|regs;
            t[ind][0] = (size == 0) ?
                           (ULO) i27_B:
                           (size == 1) ?
                              (ULO) i27_W:
                              (ULO) i27_L;
            cpu_dis_tab[ind] = i27dis;
            t[ind][2] = greg(modes, regs);
            t[ind][3] = (ULO)((size == 0) ?
                           parb[flats]:
                           (size == 1) ?
                              parw[flats]:
                              parl[flats]);
            t[ind][4] = greg(0, regd);
            t[ind][5] = (size == 0) ?
                           (8 + tarb[flats]):
                           (size == 1) ?
                              (8 + tarw[flats]):
                              (12 + tarl[flats]);
            t[ind][6] = (ULO)((size == 0) ?
                           awb[flats]:
                           (size == 1) ?
                              aww[flats]:
                              awl[flats]);
            cpu_dis_tab[ind] = i27dis;
            }
          }
}

/* Instruction EORI.X #i,<ea>
   Table-usage:

     0-Routinepointer 1-disasm routine  2-eareg 3-ea read routine
                      4-cycle count    5-ea write routine         */

ULO i28dis(ULO prc, ULO opc, STR *st) {
  return arith3dis(prc, opc, st, 4);
}

void i28ini(void) {
  ULO op=0x0a00,ind,modes,regs,flats,size;
  for (size = 0; size < 3; size++)
    for (modes = 0; modes < 8; modes++) 
      for (regs = 0; regs < 8; regs++) {
        flats = modreg(modes, regs);
        if ((data[flats] && alterable[flats]) || (flats == 11 && size < 2)) {
          ind = op|size<<6|modes<<3|regs;
          t[ind][0] = (size == 0) ?
                         ((flats == 11) ?
                           (ULO) i28_B_CCR:
                           (ULO) i28_B):
                         (size == 1) ?
                            ((flats == 11) ?
                              (ULO) i28_W_SR:
                              (ULO) i28_W):
                            (ULO) i28_L; 
          t[ind][2] = greg(modes, regs);
          t[ind][3] = (ULO)((size == 0) ?
                         parb[flats]:
                         (size == 1) ?
                            parw[flats]:
                            parl[flats]);
          t[ind][4] = (flats == 11) ?
                        20:
                        (size == 0) ?
                         (flats == 0) ?
                            8:
                            (12 + tarb[flats]):
                         (size == 1)  ?
                            (flats == 0) ?
                               8:
                               (12 + tarw[flats]):
                            (flats == 0) ?
                               16:
                               (20 + tarl[flats]);
          t[ind][5] = (ULO)((size == 0) ?
                         awb[flats]:
                         (size == 1) ? 
                            aww[flats]:
                            awl[flats]);
          cpu_dis_tab[ind] = i28dis;
        }
      }
}

/* Instruction exg Rx,Ry
   Table-usage:

    0-Routinepointer 1-disasm routine  2-reg1 3-reg2 */

ULO i29dis(ULO prc,ULO opc,STR *st) {
  ULO o=(opc&0x00f8)>>3, reg1 = get_dreg(opc), reg2 = get_sreg(opc);
  if (o == 8) 
    sprintf(st,"$%.6X %.4X                   EXG.L   D%d,D%d",prc,opc,reg2,reg1);
  else if (o == 9)
    sprintf(st,"$%.6X %.4X                   EXG.L   A%d,A%d",prc,opc,reg2,reg1);
  else
    sprintf(st,"$%.6X %.4X                   EXG.L   A%d,D%d",prc,opc,reg2,reg1);
  return prc + 2;
}

void i29ini(void) {
  ULO op=0xc100,ind,reg1,reg2;

  for (reg2 = 0; reg2 < 8; reg2++)
    for (reg1 = 0; reg1 < 8; reg1++) {
      ind = op | (8<<3) | (reg2<<9) | reg1;
      t[ind][0] = (ULO) i29;
      cpu_dis_tab[ind] = i29dis;
      t[ind][2] = greg(0, reg1);
      t[ind][3] = greg(0, reg2);
      }

  for (reg2 = 0; reg2 < 8; reg2++)
    for (reg1 = 0; reg1 < 8; reg1++) {
      ind = op | (9<<3) | (reg2<<9) | reg1;
      t[ind][0] = (ULO) i29;
      cpu_dis_tab[ind] = i29dis;
      t[ind][2] = greg(1, reg1);
      t[ind][3] = greg(1, reg2);
      }

  for (reg2 = 0; reg2 < 8; reg2++)
    for (reg1 = 0; reg1 < 8; reg1++) {
      ind = op | (0x11<<3) | (reg2<<9) | reg1;
      t[ind][0] = (ULO) i29;
      cpu_dis_tab[ind] = i29dis;
      t[ind][2] = greg(1, reg1);
      t[ind][3] = greg(0, reg2);
      }
}

/* Instruction ext Dx
   Table-usage:

    0-Routinepointer 1-disasm routine  2-reg 3-cycle count  */

ULO i30dis(ULO prc,ULO opc,STR *st) {
  ULO o = get_bit6(opc), reg = get_sreg(opc);
  sprintf(st,"$%.6X %.4X                   EXT.%s   D%d",prc,opc,(o==0) ? "W":"L",reg);
  return prc+2;
}

void i30ini(void) {
  ULO op=0x4880,ind,o,regs;
  for (o = 0; o < 2; o++) 
    for (regs = 0; regs < 8; regs++) {
      ind = op|o<<6|regs;
      t[ind][0] = (o == 0) ? ((ULO) i30_W):((ULO) i30_L);
      t[ind][1] = greg(0, regs);
      cpu_dis_tab[ind] = i30dis;
      }
}

/* Instruction jmp <ea>
   Table-usage:

    0-Routinepointer 1-disasm routine  2-sourreg 3-sourceread
                     4-cycle count                   */

ULO i31dis(ULO prc, ULO opc, STR *st) {
  return unarydis(prc, opc, st, 4);
}

void i31ini(void) {
  ULO op=0x4ec0,ind,modes,regs,flats;
  for (modes = 0; modes < 8; modes++) 
    for (regs = 0; regs < 8; regs++) {
      flats = modreg(modes, regs);
      if (control[flats]) {
        ind = op|modes<<3|regs;
        t[ind][0] = (ULO) i31;
        t[ind][2] = greg(1, regs);
        t[ind][3] = (ULO) eac[flats];
        t[ind][4] = (flats == 2) ? 8:
                      (flats == 5) ? 10:
                        (flats == 6) ? 14:
                          (flats == 7) ? 10:
                            (flats == 9) ? 12:
                              (flats == 10) ? 10:14;
        cpu_dis_tab[ind] = i31dis;
      }
      }
}

/* Instruction jsr <ea>
   Table-usage:

    0-Routinepointer 1-disasm routine  2-sourreg 3-sourceread
                     4-cycle count                   */

ULO i32dis(ULO prc, ULO opc, STR *st) {
  return unarydis(prc, opc, st, 5);
}

void i32ini(void) {
  ULO op=0x4e80,ind,modes,regs,flats;
  for (modes = 0; modes < 8; modes++) 
    for (regs = 0; regs < 8; regs++) {
      flats = modreg(modes, regs);
      if (control[flats]) {
        ind = op|modes<<3|regs;
        t[ind][0] = (ULO) i32;
        t[ind][2] = greg(1, regs);
        t[ind][3] = (ULO) eac[flats];
        t[ind][4] = (flats == 2) ? 8:
                      (flats == 5) ? 10:
                        (flats == 6) ? 14:
                          (flats == 7) ? 10:
                            (flats == 9) ? 12:
                              (flats == 10) ? 10:14;
        cpu_dis_tab[ind] = i32dis;
      }
      }
}

/* Instruction lea <ea>
   Table-usage:

    0-Routinepointer 1-disasm routine  2-sourreg 3-earead
                     4-Areg 5-cycle count                   */

ULO i33dis(ULO prc, ULO opc, STR *st) {
  return various1dis(prc, opc, st, 3);
}

void i33ini(void) {
  ULO op=0x41c0,ind,areg,modes,regs,flats;
  for (areg = 0; areg < 8; areg++)
  for (modes = 0; modes < 8; modes++)
    for (regs = 0; regs < 8; regs++) {
      flats = modreg(modes, regs);
      if (control[flats]) {
        ind = op|areg<<9|modes<<3|regs;
        t[ind][0] = (ULO) i33;
        cpu_dis_tab[ind] = i33dis;
        t[ind][2] = greg(1, regs);
        t[ind][3] = (ULO) eac[flats];
        t[ind][4] = greg(1, areg);
        t[ind][5] = (flats == 2) ? 4:
                      (flats == 5) ? 8:
                        (flats == 6) ? 12:
                          (flats == 7) ? 8:
                            (flats == 8) ? 12:
                              (flats == 9) ? 8:12;
        }
      }
}

/* Instruction LINK An,#
   Table-usage:
     0-Routinepointer 1-disasm routine  2-reg */

ULO i34dis(ULO prc, ULO opc, STR *st) {
  ULO j, reg = get_sreg(opc);
  j = fetw(prc+2);
  sprintf(st,"$%.6X %.4X %.4X              LINK    A%1d,#$%.4X",prc,opc,j,reg,j);  
  return prc+4;
}

void i34ini(void) {
  ULO op=0x4e50,ind,reg;
  for (reg = 0; reg < 8; reg++) {
    ind = op|reg;
    t[ind][0] = (ULO) i34;
    cpu_dis_tab[ind] = i34dis;
    t[ind][2] = greg(1, reg);
    }    
}

/* Instruction LSL/LSR.X #i,<ea>
   3 versions: 
   Table-usage:

   LSX.X Dx,Dy
    0-Routinepointer 1-disasm routine  2-shcountreg 3-dreg
                4-cycle count               

   LSX.X #,Dy
    0-Routinepointer 1-disasm routine  2-shiftcount 3-dreg
                4-cycle count               

   LSX.X #1,<ea>
    0-Routinepointer 1-disasm routine  2-eareg 3-earead
                4-eawrite        5-cycle count           */

ULO i35dis(ULO prc, ULO opc, STR *st) {
  return shiftdis(prc, opc, st, 1);
}

void i35ini(void) {
  ULO op=0xe008,ind,dir,modes,regs,regc,flats,size,immreg;
  for (size = 0; size < 3; size++)
    for (regc = 0; regc < 8; regc++) 
      for (dir = 0; dir < 2; dir++) 
        for (regs = 0; regs < 8; regs++)
          for (immreg = 0; immreg < 2; immreg++) {
            ind = op|regc<<9|dir<<8|size<<6|immreg<<5|regs;
            t[ind][0] = (size == 0) ?
                           ((immreg==0) ?
                             ((dir==0) ?
                               (ULO) i35_RI_I_B:
                               (ULO) i35_LE_I_B):
                             ((dir==0) ?
                               (ULO) i35_RI_R_B:
                               (ULO) i35_LE_R_B)):
                           ((size == 1) ?
                             ((immreg==0) ?
                               ((dir==0) ?
                                 (ULO) i35_RI_I_W:
                                 (ULO) i35_LE_I_W):
                               ((dir==0) ?
                                 (ULO) i35_RI_R_W:
                                 (ULO) i35_LE_R_W)):
                             ((immreg==0) ?
                               ((dir==0) ?
                                 (ULO) i35_RI_I_L:
                                 (ULO) i35_LE_I_L):
                               ((dir==0) ?
                                 (ULO) i35_RI_R_L:
                                 (ULO) i35_LE_R_L)));
            cpu_dis_tab[ind] = i35dis;
            t[ind][2] = (immreg == 0) ?
                          ((regc == 0) ?
                            8:
                            regc):
                          greg(0, regc);
            t[ind][3] = greg(0, regs);
            t[ind][4] = (size == 3) ?
                          ((immreg == 0) ?
                            (8+(t[ind][2]*2)):
                            8):
                          ((immreg == 0) ?
                            (6+(t[ind][2]*2)):
                            6);
            }


  op = 0xe2c0;
  for (dir = 0; dir < 2; dir++) 
    for (modes = 0; modes < 8; modes++) 
      for (regs = 0; regs < 8; regs++) {
        flats = modreg(modes, regs);
        if (memory[flats] && alterable[flats]) {
          ind = op|dir<<8|modes<<3|regs;
	    t[ind][0] = (dir == 0) ?
	      (ULO) i35_RI_M_W:
	      (ULO) i35_LE_M_W;
	    t[ind][2] = greg(modes, regs);
	    t[ind][3] = (ULO) parw[flats];
	    t[ind][4] = (ULO) aww[flats];
	    t[ind][5] = 8+tarw[flats];
          cpu_dis_tab[ind] = i35dis;
	}
      }
}

/* Instruction MOVE.X <ea>,<ea>
   Table usage: 2 - get src ea  3 - src reg  4 - set dst ea  5 - cycles
                6 - dst reg*/

ULO i37dis(ULO prc, ULO opc, STR *st) {
  ULO mode = get_smod(opc), pos = 13, reg = get_sreg(opc),
      size = get_size3(opc);
  sprintf(st,"$%.6X %.4X                   MOVE.%c  ", prc, opc, sizec(size));
  mode = modreg(mode, reg);
  prc = disAdrMode(reg,prc+2,st,size,mode,&pos);
  strcat(st,",");
  reg = get_dreg(opc);
  mode = modreg(get_dmod(opc), reg);
  prc = disAdrMode(reg,prc,st,size,mode,&pos);  
  return prc;
}

void i37ini(void) {
  ULO op=0x0000,ind,moded,regd,modes,regs,size,flatd,flats;
  
  size = 0x1000; /* Byte */
  for (moded = 0; moded < 8; moded++) {
    for (regd = 0; regd < 8; regd++) {
      flatd = modreg(moded, regd);
      if (data[flatd] && alterable[flatd]) {
        for (modes = 0; modes < 8; modes++) {
          for (regs = 0; regs < 8; regs++) {
	    flats = modreg(modes, regs);
            if (data[flats]) {
              ind = op|size|regd<<9|moded<<6|modes<<3|regs;
		t[ind][0] = (ULO) i37_B;
		t[ind][2] = (ULO) arb[flats];
		t[ind][3] = greg(modes, regs);
		t[ind][4] = (ULO) awb[flatd];
		t[ind][6] = greg(moded, regd);
		t[ind][5] = 4+tarb[flats]+tarb[flatd];
	      cpu_dis_tab[ind] = i37dis;
	    }
	  }
	}
      }
    }
  }
  size = 0x3000; /* Word */
  for (moded = 0; moded < 8; moded++) {
    for (regd = 0; regd < 8; regd++) {
      flatd = modreg(moded, regd);
      if (data[flatd] && alterable[flatd]) {
        for (modes = 0; modes < 8; modes++) {
          for (regs = 0; regs < 8; regs++) {
	    flats = modreg(modes, regs);
            if (allmodes[flats]) {
              ind = op|size|regd<<9|moded<<6|modes<<3|regs;
		t[ind][0] = (ULO) i37_W;
		t[ind][2] = (ULO) arw[flats];
		t[ind][3] = greg(modes, regs);
		t[ind][4] = (ULO) aww[flatd];
		t[ind][6] = greg(moded, regd);
		t[ind][5] = 4+tarw[flats]+tarw[flatd];
	      cpu_dis_tab[ind] = i37dis;
	    }
	  }
	}
      }
    }
  }
  size = 0x2000; /* Long */
  for (moded = 0; moded < 8; moded++) {
    for (regd = 0; regd < 8; regd++) {
      flatd = modreg(moded, regd);
      if (data[flatd] && alterable[flatd]) {
        for (modes = 0; modes < 8; modes++) {
          for (regs = 0; regs < 8; regs++) {
	    flats = modreg(modes, regs);
            if (allmodes[flats]) {
              ind = op|size|regd<<9|moded<<6|modes<<3|regs;
		t[ind][0] = (ULO) i37_L;
		t[ind][3] = greg(modes, regs);
		t[ind][2] = (ULO) arl[flats];
		t[ind][6] = greg(moded, regd);
		t[ind][4] = (ULO) awl[flatd];
		t[ind][5] = 4+tarl[flats]+tarl[flatd];
	      cpu_dis_tab[ind] = i37dis;
	    }
	  }
	}
      }
    }
  }
}


/* Instruction MOVE.B to CCR
   Table-usage:

    0-Routinepointer 1-disasm routine  2-sourcereg 3-sourceroutine
                     4-cycle count                   */

ULO i38dis(ULO prc, ULO opc, STR *st) {
  ULO pos = 13, reg = get_sreg(opc), mode = get_smod(opc);
  sprintf(st,"$%.6X %.4X                   MOVE.B  ",prc,opc);
  mode = modreg(mode, reg);
  prc = disAdrMode(reg,prc+2,st,8,mode,&pos);
  strcat(st,",CCR");  
  return prc;
}

void i38ini(void) {
  ULO op=0x44c0,ind,reg,mode,flat;
    for (reg = 0; reg < 8; reg++) 
      for (mode = 0; mode < 8; mode++) {
        flat = modreg(mode, reg);
        if (data[flat]) {
	  ind = op|mode<<3|reg;
            t[ind][0] = (ULO) i38;
            t[ind][2] = greg(mode, reg);
            t[ind][3] = (ULO) arw[flat];
            t[ind][4] = 12 + tarw[flat];
            cpu_dis_tab[ind] = i38dis;
	}
      }   
}

/* Instruction MOVE.W to SR
   Table-usage:

    0-Routinepointer 1-disasm routine  2-sourcereg 3-sourceroutine
                     4-cycle count                   */

ULO i39dis(ULO prc, ULO opc, STR *st) {
  ULO pos = 13, reg = get_sreg(opc), mode = get_smod(opc);
  sprintf(st,"$%.6X %.4X            (PRIV) MOVE.W  ",prc,opc);
  mode = modreg(mode, reg);
  prc = disAdrMode(reg,prc+2,st,16,mode,&pos);
  strcat(st,",SR");
  return prc;
}

void i39ini(void) {
  ULO op=0x46c0,ind,reg,mode,flat;
    for (reg = 0; reg < 8; reg++) 
      for (mode = 0; mode < 8; mode++) {
        flat = modreg(mode, reg);
        if (data[flat]) {
	  ind = op|mode<<3|reg;
            t[ind][0] = (ULO) i39;
            t[ind][2] = greg(mode, reg);
            t[ind][3] = (ULO) arw[flat];
            t[ind][4] = 12 + tarw[flat];
	  cpu_dis_tab[ind] = i39dis;
	}
      }    
}

/* Instruction MOVE.W from SR
   Table-usage:

    0-Routinepointer 1-disasm routine  2-destreg 3-destroutine
                     4-cycle count                   */

ULO i40dis(ULO prc,ULO opc,STR *st) {
  ULO pos = 13, reg = get_sreg(opc), mode = get_smod(opc);
  sprintf(st,"$%.6X %.4X                   MOVE.W  SR,",prc,opc);
  mode = modreg(mode, reg);
  prc = disAdrMode(reg,prc+2,st,16,mode,&pos);
  return prc;
}

void i40ini(void) {
  ULO op=0x40c0,ind,reg,mode,flat;
    for (reg = 0; reg < 8; reg++) 
      for (mode = 0; mode < 8; mode++) { 
        flat = modreg(mode, reg);
        if (data[flat] & alterable[flat]) {
            ind = op|mode<<3|reg;
	      t[ind][0] = (ULO) i40;
	      cpu_dis_tab[ind] = i40dis;
	      t[ind][2] = greg(mode, reg);
	      t[ind][3] = (ULO) aww[flat];
	      t[ind][4] = (flat < 2) ? 6:(8 + tarw[flat]);
	}
      }     
}

/* Instruction MOVE.L USP to/from Ax
   Table-usage:
   0-Routinepointer 1-disasm routine  2-reg */

ULO i41dis(ULO prc,ULO opc,STR *st) {
  ULO reg = get_sreg(opc), dir = get_bit3(opc);
  if (dir == 0)
    sprintf(st,"$%.6X %.4X            (PRIV) MOVE.L  A%1d,USP",prc,opc,reg);
  else
    sprintf(st,"$%.6X %.4X            (PRIV) MOVE.L  USP,A%1d",prc,opc,reg);
  return prc+2;
}

void i41ini(void) {
  ULO op=0x4e60,ind,reg,dir;
  for (dir = 0; dir < 2; dir++)
    for (reg = 0; reg < 8; reg++) {
      ind = op | reg | dir<<3;
      t[ind][0] = (dir == 0) ? (ULO) i41_2 : (ULO) i41_1;
      cpu_dis_tab[ind] = i41dis;
      t[ind][2] = greg(1, reg);
      }
}

/* Instruction MOVEA.W/L <ea>,An
   Table-usage:

    0-Routinepointer 1-disasm routine  2-sourreg 3-sourceroutine
                     4-Areg        5-cycle count                   */

ULO i42dis(ULO prc, ULO opc, STR *st) {
  ULO pos = 13, reg = get_sreg(opc), dreg = get_dreg(opc),
      mode = get_smod(opc), size = get_size3(opc);
  mode = modreg(mode, reg);
  sprintf(st,"$%.6X %.4X                   MOVEA.%c ", prc, opc, sizec(size));
  prc = disAdrMode(reg,prc+2,st,size,mode,&pos);
  strcat(st,",");
  disAdrMode(dreg,prc,st,size,1,&pos);
  return prc;
}

void i42ini(void) {
  ULO op=0x2040,ind,regd,modes,regs,flats,o;
  for (o = 0; o < 2; o++)
    for (regd = 0; regd < 8; regd++) 
      for (modes = 0; modes < 8; modes++) 
        for (regs = 0; regs < 8; regs++) {
          flats = modreg(modes, regs);
          if (allmodes[flats]) {
            ind = op|regd<<9|o<<12|modes<<3|regs;
            t[ind][0] = (o == 0) ?
                           (ULO) i42_L:
                           (ULO) i42_W;
            cpu_dis_tab[ind] = i42dis;
            t[ind][2] = greg(modes, regs);
            t[ind][3] = (ULO)((o == 0) ?
                           arl[flats]:
                           arw[flats]);
            t[ind][4] = greg(1, regd);
            t[ind][5] = (o == 0) ?
                           (4 + tarl[flats]):
                           (4 + tarw[flats]);
            }
          }
}

/*===========================================================================
   Instruction MOVEM.W/L 
   Table-usage:

   0-Routinepointer 1-disasm routine 2-eareg 3-earead/write 4-base cycle time
  ===========================================================================*/

void cpuMovemRegmaskStrCat(ULO regmask, STR *s, BOOLE predec) {
  ULO i, j;
  STR tmp[2][16];
  STR *tmpp;
  for (j = 0; j < 2; j++) {
    tmpp = tmp[j];
    for (i = (8*j); i < (8 + (8*j)); i++)
      if (regmask & (1<<i)) *tmpp++ = (STR) (0x30 + ((predec) ? ((7 + 8*j) - i) : (i - j*8)));
    *tmpp = '\0';
  }
  if (tmp[0][0] != '\0') {strcat(s, ((predec) ? "A" : "D")); strcat(s, tmp[0]);}
  if (tmp[1][0] != '\0') {strcat(s, ((predec) ? "D" : "A")); strcat(s, tmp[1]);}
}

ULO cpuMovemDis(ULO prc, ULO opc, STR *st) {
  ULO pos = 18, reg = get_sreg(opc), mode = get_smod(opc), size,
      dir = get_bit10(opc), regmask;
  size = (!get_bit6(opc)) ? 16:32;
  regmask = fetw(prc+2);
  mode = modreg(mode, reg);

  if (dir == 0 && mode == 4) {  /* Register to memory, predecrement */
    sprintf(st,"$%.6X %.4X %.4X              MOVEM.%s ",prc,opc,regmask,((size==16)?"W":"L"));
    cpuMovemRegmaskStrCat(regmask, st, TRUE);
    strcat(st,",");
    prc = disAdrMode(reg,prc+4,st,size,mode,&pos);
    }     
  else if (dir) { /* Memory to register, any legal adressmode */
    sprintf(st,"$%.6X %.4X %.4X              MOVEM.%s ",prc,opc,regmask,((size==16)?"W":"L"));

    prc = disAdrMode(reg,prc+4,st,size,mode,&pos);
    strcat(st,",");
    cpuMovemRegmaskStrCat(regmask, st, FALSE);
  }     
  else {  /* Register to memory, the rest of the adr.modes */
    sprintf(st,"$%.6X %.4X %.4X              MOVEM.%s ",prc,opc,regmask,((size==16)?"W":"L"));
    cpuMovemRegmaskStrCat(regmask, st, FALSE);
    strcat(st,",");
    prc = disAdrMode(reg,prc+4,st,size,mode,&pos);
  }
  return prc;
}

void cpuMovemIni(void) {
  ULO op=0x4880,ind,modes,regs,flats,dir,sz;
  for (dir = 0; dir < 2; dir++)
    for (sz = 0; sz < 2; sz++) 
      for (modes = 0; modes < 8; modes++) 
        for (regs = 0; regs < 8; regs++) {
          flats = modreg(modes, regs);
          if (control[flats] || ((flats == 3) && (dir == 1)) || ((flats == 4) && (dir == 0))) {
            ind = op|dir<<10|sz<<6|modes<<3|regs;
            t[ind][0] = (sz == 0) ?
                           ((dir == 0) ?
                             ((flats == 4) ?
                               (ULO) MOVEM_W_PREM:
                               (ULO) MOVEM_W_CONM):
                             ((flats == 3) ?
                               (ULO) MOVEM_W_POSTR:
                               (ULO) MOVEM_W_CONR)):
                           ((dir == 0) ?
                             ((flats == 4) ?
                               (ULO) MOVEM_L_PREM:
                               (ULO) MOVEM_L_CONM):
                             ((flats == 3) ?
                               (ULO) MOVEM_L_POSTR:
                               (ULO) MOVEM_L_CONR));
            cpu_dis_tab[ind] = cpuMovemDis;
            t[ind][2] = greg(1, regs);
            t[ind][3] = (ULO) eac[flats];
            t[ind][4] = (sz == 0) ?
                           (8 + tarw[flats]):
                           (8 + tarl[flats]);
            }
          }
}


/*===================*/
/* Instruction MOVEP */
/*===================*/

ULO i44dis(ULO prc,ULO opc,STR *st) {
  sprintf(st,"$%.6X %.4X                   MOVEP",prc,opc);  
  return prc + 4;
}

void i44ini(void) {
  ULO op = 0x0008, ind, dreg, areg, mode;

  for (areg = 0; areg < 8; areg++) 
   for (dreg = 0; dreg < 8; dreg++) 
    for (mode = 4; mode < 8; mode++) {
      ind = op | areg | mode<<6 | dreg<<9;
      if (mode == 4) t[ind][0] = (ULO) i44_W_1;
      else if (mode == 5) t[ind][0] = (ULO) i44_L_1;
      else if (mode == 6) t[ind][0] = (ULO) i44_W_2;
      else if (mode == 7) t[ind][0] = (ULO) i44_L_2;
      cpu_dis_tab[ind] = i44dis;
      t[ind][2] = greg(1, areg);
      t[ind][3] = greg(0, dreg);
      }
}

/* Instruction MOVEQ.L #8 bit signed imm,Dn
   Table-usage:

    0-Routinepointer 1-disasm routine  2-reg 3-32 bit immediate
                     4-flag byte                          */

ULO i45dis(ULO prc, ULO opc, STR *st) {
  ULO reg = get_dreg(opc);

  if (get_bit8(opc) && ((prc & 0xf80000) != 0xf80000))
    sprintf(st,"$%.6X %.4X                   ILLEGAL INSTRUCTION",prc,opc);
  else 
    sprintf(st,"$%.6X %.4X                   MOVEQ.L #$%8.8X,D%d",prc,opc,t[opc][3],reg);
  return prc + 2;
}

void i45ini(void) {
  ULO op=0x7000,ind,reg;
  int imm;
  for (reg = 0; reg < 8; reg++) 
    for (imm = -128; imm < 128; imm++) {
      ind = op|reg<<9|imm&0xff;
      t[ind][0] = (ULO) i45;
      cpu_dis_tab[ind] = i45dis;
      t[ind][2] = greg(0, reg);
      t[ind][3] = imm;
      t[ind][4] = 0 | ((imm < 0) ? 0x8:0) | ((imm == 0) ? 0x4:0);
      }
  op = 0x7100;
  for (reg = 0; reg < 8; reg++)
    for (imm = -128; imm < 128; imm++) {
      ind = op|reg<<9|imm&0xff;
      t[ind][0] = (ULO) i45;
      cpu_dis_tab[ind] = i45dis;
      t[ind][2] = greg(0, reg);
      t[ind][3] = imm;
      t[ind][4] = 0 | ((imm < 0) ? 0x8:0) | ((imm == 0) ? 0x4:0);
      }
}

/* Instruction MULS.W <ea>,Dn
   Table-usage:

    0-Routinepointer 1-disasm routine  2-sourreg 3-sourceroutine
                     4-Dreg                          */

ULO i46dis(ULO prc, ULO opc, STR *st) {
  return various1dis(prc, opc, st, 4);
}

void i46ini(void) {
  ULO op=0xc1c0,ind,regd,modes,regs,flats;
  for (regd = 0; regd < 8; regd++) 
    for (modes = 0; modes < 8; modes++) 
      for (regs = 0; regs < 8; regs++) {
        flats = modreg(modes, regs);
        if (data[flats]) {
          ind = op|regd<<9|modes<<3|regs;
          t[ind][0] = (ULO) i46;
          cpu_dis_tab[ind] = i46dis;
          t[ind][2] = greg(modes, regs);
          t[ind][3] = (ULO) arw[flats];
          t[ind][4] = greg(0, regd);
          }
       }
}

/* Instruction MULU.W <ea>,Dn
   Table-usage:

    0-Routinepointer 1-disasm routine  2-sourreg 3-sourceroutine
                     4-Dreg                          */

ULO i47dis(ULO prc, ULO opc, STR *st) {
  return various1dis(prc, opc, st, 5);
}

void i47ini(void) {
  ULO op=0xc0c0,ind,regd,modes,regs,flats;
  for (regd = 0; regd < 8; regd++) 
    for (modes = 0; modes < 8; modes++) 
      for (regs = 0; regs < 8; regs++) 
        if (data[(flats = modreg(modes, regs))]) {
          ind = op|regd<<9|modes<<3|regs;
          t[ind][0] = (ULO) i47;
          cpu_dis_tab[ind] = i47dis;
          t[ind][2] = greg(modes, regs);
          t[ind][3] = (ULO) arw[flats];
          t[ind][4] = greg(0, regd);
          }
}

/* Instruction NBCD.B <ea>
   Table-usage:
    0-Routinepointer 1-disasm routine  2-sourreg 3-sourceroutine 4-eawrite
                     5-cycle count                   */

ULO i48dis(ULO prc, ULO opc, STR *st) {
  return unarydis(prc, opc, st, 8);
}

void i48ini(void) {
  ULO op = 0x4800, ind, modes, regs, flats;

  for (modes = 0; modes < 8; modes++) 
      for (regs = 0; regs < 8; regs++) {
        flats = modreg(modes, regs);
        if (data[flats] && alterable[flats]) {
          ind = op|modes<<3|regs;
	    t[ind][0] = (ULO) i48_B;
	    t[ind][2] = greg(modes, regs);
	    t[ind][3] = (ULO) parb[flats];
	    t[ind][4] = (ULO) awb[flats];
	    t[ind][5] = (modes == 0) ?
	      6:
	    (8+tarb[flats]);
          cpu_dis_tab[ind] = i48dis;
	}
      }
}

/* Instruction NEG.X <ea>
   Table-usage:
    0-Routinepointer 1-disasm routine  2-sourreg 3-sourceroutine 4-eawrite
                     5-cycle count                   */

ULO i49dis(ULO prc,ULO opc,STR *st) {
  return unarydis(prc, opc, st, 1);
}

void i49ini(void) {
  ULO op = 0x4400, ind, modes, regs, flats, size;
  for (size = 0; size < 3; size++)
    for (modes = 0; modes < 8; modes++) 
      for (regs = 0; regs < 8; regs++) {
        flats = modreg(modes, regs);
        if (data[flats] && alterable[flats]) {
          ind = op | size<<6 | modes<<3 | regs;
        t[ind][0] = (size == 0) ? (ULO) i49_B:
                      (size == 1) ? (ULO) i49_W:(ULO) i49_L;
        t[ind][2] = greg(modes, regs);
        t[ind][3] = (ULO)((size == 0) ?
                       parb[flats]:
                       ((size == 1) ?
                          parw[flats]:
                          parl[flats]));
        t[ind][4] = (ULO)((size == 0) ?
                       awb[flats]:
                       ((size == 1) ?
                          aww[flats]:
                          awl[flats]));
        t[ind][5] = (size == 0) ?
                       (4+tarb[flats]):
                       ((size == 1) ?
                          (4+tarw[flats]):
                          ((modes == 0) ?
                             6:
                             (4+tarl[flats])));
        cpu_dis_tab[ind] = i49dis;
	}
      }
}

/* Instruction NEGX.X <ea>
   Table-usage:
    0-Routinepointer 1-disasm routine  2-sourreg 3-sourceroutine 4-eawrite
                     5-cycle count                   */

ULO i50dis(ULO prc, ULO opc, STR *st) {
  return unarydis(prc, opc, st, 9);
}

void i50ini(void) {
  ULO op = 0x4000, ind, modes, regs, flats, size;
  for (size = 0; size < 3; size++)
    for (modes = 0; modes < 8; modes++) 
      for (regs = 0; regs < 8; regs++) {
        flats = modreg(modes, regs);
        if (data[flats] && alterable[flats]) {
          ind = op | size<<6 | modes<<3 | regs;
        t[ind][0] = (size == 0) ? (ULO) i50_B:
                      (size == 1) ? (ULO) i50_W:(ULO) i50_L;
        t[ind][2] = greg(modes, regs);
        t[ind][3] = (ULO)((size == 0) ?
                       parb[flats]:
                       (size == 1) ?
                          parw[flats]:
                          parl[flats]);
        t[ind][4] = (ULO)((size == 0) ?
                       awb[flats]:
                       (size == 1) ?
                          aww[flats]:
                          awl[flats]);
        t[ind][5] = (size == 0) ?
                       (4+tarb[flats]):
                       (size == 1) ?
                          (4+tarw[flats]):
                          (modes == 0) ?
                             6:
                             (4+tarl[flats]);
        cpu_dis_tab[ind] = i50dis;
	}
      }
}

/* Instruction NOP
   Table-usage:
     0-Routinepointer 1-disasm routine */

ULO i51dis(ULO prc, ULO opc, STR *st) {
  return singledis(prc, opc, st, 0);
}

void i51ini(void) {
  ULO op = 0x4e71;
  t[op][0] = (ULO) i51;
  cpu_dis_tab[op] = i51dis;
}

/* Instruction NOT.X <ea>
   Table-usage:
    0-Routinepointer 1-disasm routine  2-sourreg 3-sourceroutine 4-eawrite
                     5-cycle count                   */

ULO i52dis(ULO prc, ULO opc, STR *st) {
  return unarydis(prc, opc, st, 2);
}

void i52ini(void) {
  ULO op = 0x4600, ind, modes, regs, flats, size;
  for (size = 0; size < 3; size++)
    for (modes = 0; modes < 8; modes++) 
      for (regs = 0; regs < 8; regs++) {
        flats = modreg(modes, regs);
        if (data[flats] && alterable[flats]) {
          ind = op | size<<6 | modes<<3 | regs;
        t[ind][0] = (size == 0) ? (ULO) i52_B:
                      (size == 1) ? (ULO) i52_W:(ULO) i52_L;
        t[ind][2] = greg(modes, regs);
        t[ind][3] = (ULO)((size == 0) ?
                       parb[flats]:
                       (size == 1) ?
                          parw[flats]:
                          parl[flats]);
        t[ind][4] = (ULO)((size == 0) ?
                       awb[flats]:
                       (size == 1) ?
                          aww[flats]:
                          awl[flats]);
        t[ind][5] = (size == 0) ?
                       (4+tarb[flats]):
                       (size == 1) ?
                          (4+tarw[flats]):
                          (modes == 0) ?
                             6:
                             (4+tarl[flats]);
        cpu_dis_tab[ind] = i52dis;
	}
      }
}

/* Instruction OR.X <ea>,Dn / OR.X Dn,<ea>
   Table-usage:
   Type 1:  OR <ea>,Dn

    0-Routinepointer 1-disasm routine  2-sourreg 3-sourceroutine
                     4-Dreg        5-cycle count                   
   Type 2:  OR Dn,<ea>

    0-Routinepointer 1-disasm routine  2-<ea>reg 3-get <ea> routine
                     4-Dreg        5-cycle count   6-write<ea> routine  */

ULO i53dis(ULO prc, ULO opc, STR *st) {
  return arith1dis(prc, opc, st, 5);
}

void i53ini(void) {
  ULO op = 0x8000, ind, regd, modes, regs, flats, size, o;
  for (o = 0; o < 2; o++)
    for (size = 0; size < 3; size++)
      for (regd = 0; regd < 8; regd++) 
        for (modes = 0; modes < 8; modes++) 
          for (regs = 0; regs < 8; regs++) {
            flats = modreg(modes, regs);
            if ((o==0 && data[flats]) ||
                (o==1 && alterable[flats] && memory[flats])) {
              ind = op | regd<<9 | o<<8 | size<<6 | modes<<3 | regs;
              t[ind][0] = (o == 0) ?
                             (size == 0) ?
                                (ULO) i53_B_1:
                                (size == 1) ?
                                   (ULO) i53_W_1:
                                   (ULO) i53_L_1:
                             (size == 0) ?
                                (ULO) i53_B_2:
                                (size == 1) ?
                                   (ULO) i53_W_2:
                                   (ULO) i53_L_2;
              t[ind][2] = greg(modes, regs);
              t[ind][3] = (ULO)((o == 0) ?
                             (size == 0) ?
                                arb[flats]:
                                (size == 1) ?
                                   arw[flats]:
                                   arl[flats]:
                             (size == 0) ?
                                parb[flats]:
                                (size == 1) ?
                                   parw[flats]:
                                   parl[flats]);
              t[ind][4] = greg(0, regd);
              t[ind][5] = (o == 0) ?
                             (size == 0) ?
                                (4+tarb[flats]):
                                (size == 1) ?
                                   (4+tarw[flats]):
                                   (flats <= 1 || flats == 11) ?
                                      (8+tarl[flats]):
                                      (6+tarl[flats]):
                             (size == 0) ?
                                (8 + tarb[flats]):
                                (size == 1) ?
                                   (8 + tarw[flats]):
                                   (12 + tarl[flats]);
              if (o == 1) t[ind][6] = (ULO)((size == 0) ?
                                         awb[flats]:
                                         (size == 1) ?
                                            aww[flats]:
                                            awl[flats]);
              cpu_dis_tab[ind] = i53dis;
	    }
	  }
}

/* Instruction ORI.X #i,<ea>
   Table-usage:

    0-Routinepointer 1-disasm routine  2-eareg 3-ea read routine
                     4-cycle count    5-ea write routine               */

ULO i54dis(ULO prc, ULO opc, STR *st) {
  return arith3dis(prc, opc, st, 5);
}

void i54ini(void) {
  ULO op = 0, ind, modes, regs, flats, size;
  for (size = 0; size < 3; size++)
    for (modes = 0; modes < 8; modes++) 
      for (regs = 0; regs < 8; regs++) {
        flats = modreg(modes, regs);
        if ((data[flats] && alterable[flats]) || (flats == 11 && size < 2)) {
          ind = op | size<<6 | modes<<3 | regs;
          t[ind][0] = (size == 0) ?
                         ((flats == 11) ?
                           (ULO) i54_B_CCR:
                           (ULO) i54_B):
                         (size == 1) ?
                            ((flats == 11) ?
                              (ULO) i54_W_SR:
                              (ULO) i54_W):
                            (ULO) i54_L; 
          t[ind][2] = greg(modes, regs);
          t[ind][3] = (ULO)((size == 0) ?
                         parb[flats]:
                         (size == 1) ?
                            parw[flats]:
                            parl[flats]);
          t[ind][4] = (flats == 11) ?
                        20:
                        (size == 0) ?
                         (flats == 0) ?
                            8:
                            (12 + tarb[flats]):
                         (size == 1)  ?
                            (flats == 0) ?
                               8:
                               (12 + tarw[flats]):
                            (flats == 0) ?
                               16:
                               (20 + tarl[flats]);
          t[ind][5] = (ULO)((size == 0) ?
                         awb[flats]:
                         (size == 1) ? 
                            aww[flats]:
                            awl[flats]);
          cpu_dis_tab[ind] = i54dis;
	}
      }
}

/* Instruction pea <ea>
   Table-usage:
    0-Routinepointer 1-disasm routine  2-sourreg 3-earead
                     4-cycle count                   */

ULO i55dis(ULO prc, ULO opc, STR *st) {
  return unarydis(prc, opc, st, 6);
}

void i55ini(void) {
  ULO op = 0x4840, ind, modes, regs, flats;
  UBY cyc[12] = {0,0,12,0,0,16,20,16,20,20,16,20};
  for (modes = 0; modes < 8; modes++)
    for (regs = 0; regs < 8; regs++) {
      flats = modreg(modes, regs);
      if (control[flats]) {
        ind = op | modes<<3 | regs;
        t[ind][0] = (ULO) i55;
        t[ind][2] = greg(1, regs);
        t[ind][3] = (ULO) eac[flats];
        t[ind][4] = cyc[flats];
        cpu_dis_tab[ind] = i55dis;
      }
      }
}

/* Instruction RESET
   Table-usage:
       0-Routinepointer 1-disasm routine */

ULO i56dis(ULO prc, ULO opc, STR *st) {
  return singledis(prc, opc, st, 1);
}

void i56ini(void) {
  ULO op = 0x4e70;
  t[op][0] = (ULO) i56;
  cpu_dis_tab[op] = i56dis;
}

/* Instruction ROL/ROR.X #i,<ea>
   3 versions: 
   Table-usage:

   ROX.X Dx,Dy
    0-Routinepointer 1-disasm routine  2-shcountreg 3-dreg
                4-cycle count               

   ROX.X #,Dy
    0-Routinepointer 1-disasm routine  2-shiftcount 3-dreg
                4-cycle count               

   ROX.X #1,<ea>
    0-Routinepointer 1-disasm routine  2-eareg 3-earead
                4-eawrite        5-cycle count               */

ULO i57dis(ULO prc, ULO opc, STR *st) {
  return shiftdis(prc, opc, st, 2);
}

void i57ini(void) {
  ULO op = 0xe018, ind, dir, modes, regs, regc, flats, size, immreg;
  for (size = 0; size < 3; size++)
    for (regc = 0; regc < 8; regc++) 
      for (dir = 0; dir < 2; dir++) 
        for (regs = 0; regs < 8; regs++)
          for (immreg = 0; immreg < 2; immreg++) {
            ind = op | regc<<9 | dir<<8 | size<<6 | immreg<<5 | regs;
            t[ind][0] = (size == 0) ?
                           ((immreg==0) ?
                             ((dir==0) ?
                               (ULO) i57_RI_I_B:
                               (ULO) i57_LE_I_B):
                             ((dir==0) ?
                               (ULO) i57_RI_R_B:
                               (ULO) i57_LE_R_B)):
                           ((size == 1) ?
                             ((immreg==0) ?
                               ((dir==0) ?
                                 (ULO) i57_RI_I_W:
                                 (ULO) i57_LE_I_W):
                               ((dir==0) ?
                                 (ULO) i57_RI_R_W:
                                 (ULO) i57_LE_R_W)):
                             ((immreg==0) ?
                               ((dir==0) ?
                                 (ULO) i57_RI_I_L:
                                 (ULO) i57_LE_I_L):
                               ((dir==0) ?
                                 (ULO) i57_RI_R_L:
                                 (ULO) i57_LE_R_L)));
            cpu_dis_tab[ind] = i57dis;
            t[ind][2] = (immreg == 0) ?
                          ((regc == 0) ?
                            8:
                            regc):
                          greg(0, regc);
            t[ind][3] = greg(0, regs);
            t[ind][4] = (size == 3) ?
                          ((immreg == 0) ?
                            (8+(t[ind][2]*2)):
                            8):
                          ((immreg == 0) ?
                            (6+(t[ind][2]*2)):
                            6);
            }

  op = 0xe6c0;
  for (dir = 0; dir < 2; dir++) 
    for (modes = 0; modes < 8; modes++) 
      for (regs = 0; regs < 8; regs++) {
        flats = modreg(modes, regs);
        if (memory[flats] && alterable[flats]) {
          ind = op | dir<<8 | modes<<3 | regs;
	    t[ind][0] = (dir == 0) ? (ULO) i57_RI_M_W : (ULO) i57_LE_M_W;
	    t[ind][2] = greg(modes, regs);
	    t[ind][3] = (ULO) parw[flats];
	    t[ind][4] = (ULO) aww[flats];
	    t[ind][5] = 8 + tarw[flats];
          cpu_dis_tab[ind] = i57dis;
	}
      }
}

/* Instruction ROXL/ROXR.X #i,<ea>
   3 versions: 
   Table-usage:

   ROXX.X Dx,Dy
    0-Routinepointer 1-disasm routine  2-shcountreg 3-dreg
                4-cycle count               

   ROXX.X #,Dy
    0-Routinepointer 1-disasm routine  2-shiftcount 3-dreg
                4-cycle count               

   ROXX.X #1,<ea>
    0-Routinepointer 1-disasm routine  2-eareg 3-earead
                4-eawrite        5-cycle count             */

ULO i59dis(ULO prc, ULO opc, STR *st) {
  return shiftdis(prc, opc, st, 3);
}

void i59ini(void) {
  ULO op = 0xe010, ind, dir, modes, regs, regc, flats, size, immreg;
  for (size = 0; size < 3; size++)
    for (regc = 0; regc < 8; regc++) 
      for (dir = 0; dir < 2; dir++) 
        for (regs = 0; regs < 8; regs++)
          for (immreg = 0; immreg < 2; immreg++) {
            ind = op | regc<<9 | dir<<8 | size<<6 | immreg<<5 | regs;
            t[ind][0] = (size == 0) ?
                           ((immreg==0) ?
                             ((dir==0) ?
                               (ULO) i59_RI_I_B:
                               (ULO) i59_LE_I_B):
                             ((dir==0) ?
                               (ULO) i59_RI_R_B:
                               (ULO) i59_LE_R_B)):
                           ((size == 1) ?
                             ((immreg==0) ?
                               ((dir==0) ?
                                 (ULO) i59_RI_I_W:
                                 (ULO) i59_LE_I_W):
                               ((dir==0) ?
                                 (ULO) i59_RI_R_W:
                                 (ULO) i59_LE_R_W)):
                             ((immreg==0) ?
                               ((dir==0) ?
                                 (ULO) i59_RI_I_L:
                                 (ULO) i59_LE_I_L):
                               ((dir==0) ?
                                 (ULO) i59_RI_R_L:
                                 (ULO) i59_LE_R_L)));
            cpu_dis_tab[ind] = i59dis;
            t[ind][2] = (immreg == 0) ?
                          ((regc == 0) ?
                            8:
                            regc):
                          greg(0, regc);
            t[ind][3] = greg(0, regs);
            t[ind][4] = (size == 3) ?
                          ((immreg == 0) ?
                            (8+(t[ind][2]*2)):
                            8):
                          ((immreg == 0) ?
                            (6+(t[ind][2]*2)):
                            6);
            }

  op = 0xe4c0;
  for (dir = 0; dir < 2; dir++) 
    for (modes = 0; modes < 8; modes++) 
      for (regs = 0; regs < 8; regs++) {
        flats = modreg(modes, regs);
        if (memory[flats] && alterable[flats]) {
          ind = op | dir<<8 | modes<<3 | regs;
	    t[ind][0] = (dir == 0) ? (ULO) i59_RI_M_W : (ULO) i59_LE_M_W;
	    t[ind][2] = greg(modes, regs);
	    t[ind][3] = (ULO) parw[flats];
	    t[ind][4] = (ULO) aww[flats];
	    t[ind][5] = 8 + tarw[flats];
          cpu_dis_tab[ind] = i59dis;
	}
      }
}

/* Instruction RTE
   Table-usage:
     0-Routinepointer 1-disasm routine */

ULO i61dis(ULO prc, ULO opc, STR *st) {
  return singledis(prc, opc, st, 2);
}

void i61ini(void) {
  ULO op = 0x4e73;
  t[op][0] = (ULO) i61;
  cpu_dis_tab[op] = i61dis;
}

/* Instruction RTR
   Table-usage:
     0-Routinepointer 1-disasm routine */

ULO i62dis(ULO prc, ULO opc, STR *st) {
  return singledis(prc, opc, st, 3);
}

void i62ini(void) {
  ULO op = 0x4e77;
  t[op][0] = (ULO) i62;
  cpu_dis_tab[op] = i62dis;
}    

/* Instruction RTS
   Table-usage:
     0-Routinepointer 1-disasm routine */

ULO i63dis(ULO prc, ULO opc, STR *st) {
  return singledis(prc, opc, st, 4);
}

void i63ini(void) {
  ULO op = 0x4e75;
  t[op][0] = (ULO) i63;
  cpu_dis_tab[op] = i63dis;
}    


/*========================================================
   Instruction SBCD.B Ry,Rx / SBCD.B -(Ay),-(Ax)
   Table-usage: 0 -Routinepointer 1 - Rx*4(dest) 2 - Ry*4
  ========================================================*/

ULO i64dis(ULO prc, ULO opc, STR *st) {
  return arith5dis(prc, opc, st, 3);
}

void i64ini(void) {
  ULO op = 0x8100, ind, rm, rx, ry;

  for (rm = 0; rm < 2; rm++) 
    for (rx = 0; rx < 8; rx++) 
      for (ry = 0; ry < 8; ry++) {
        ind = op | ry | rx<<9 | rm<<3;
        t[ind][0] = (rm == 0) ? (ULO) i64_1 : (ULO) i64_2;
        cpu_dis_tab[ind] = i64dis;
        t[ind][1] = greg(rm, rx);
        t[ind][2] = greg(rm, ry);
        }
}


/*===========================================================================
   Instruction Scc.B <ea>
   Table-usage:

     0 - Emulation routine
     1 - ea register pointer
     2 - ea write routine
     3 - Condition handler
     4 - cycle count false
     5 - cycle count true
  ===========================================================================*/

ULO i65dis(ULO prc, ULO opc, STR *st) {
  ULO pos = 13, mode = get_smod(opc), reg = get_sreg(opc),
      bratype = get_branchtype(opc);
  sprintf(st,"$%.6X %.4X                   S",prc,opc);
  if (bratype == 0) strcat(st, "T.B    ");
  else if (bratype == 1) strcat(st, "F.B    ");
  else {
    strcat(st, btab[bratype]);
    strcat(st, ".B   ");
  }
  mode = modreg(mode, reg);
  prc = disAdrMode(reg, prc + 2, st, 8, mode, &pos);
  return prc;
}

void i65ini(void) {
  ULO op = 0x50c0, c, ind, flat, mode, reg;
  ULO routines[16] = {(ULO) cc0,(ULO) cc1false,(ULO) cc2,(ULO) cc3,(ULO) cc4,
		      (ULO) cc5,(ULO) cc6,(ULO) cc7,(ULO) cc8,(ULO) cc9,
		      (ULO) cca,(ULO) ccb,(ULO) ccc,(ULO) ccd,(ULO) cce,
		      (ULO) ccf};

  for (c = 0; c < 16; c++)
    for (mode = 0; mode < 8; mode++) 
      for (reg = 0; reg < 8; reg++) {
        flat = modreg(mode, reg);
        if (data[flat] && alterable[flat]) {
          ind = op | c<<8 | mode<<3 | reg;
          t[ind][0] = (ULO) i65;
          cpu_dis_tab[ind] = i65dis;
          t[ind][1] = greg(mode, reg);
          t[ind][2] = (ULO) awb[flat];
          t[ind][3] = routines[c];
          t[ind][4] = 4 + tarb[flat];
          t[ind][5] = (flat == 0) ? 6 : (4 + tarb[flat]);
          }
        }
}

/* Instruction STOP #
   Table-usage:
     0-Routinepointer 1-disasm routine */

ULO i66dis(ULO prc, ULO opc, STR *st) {
  sprintf(st, "$%.6X %.4X                   STOP #$%.4X", prc, opc,
	  fetw(prc + 2));
  return prc + 4;
}

void i66ini(void) {
  ULO op = 0x4e72;
  t[op][0] = (ULO) i66;
  cpu_dis_tab[op] = i66dis;
}

/* Instruction SUB.X <ea>,Dn / SUB.X Dn,<ea>
   Table-usage:
   Type 1:  SUB <ea>,Dn

    0-Routinepointer 1-disasm routine  2-sourreg 3-sourceroutine
                     4-Dreg        5-cycle count                   
   Type 2:  SUB Dn,<ea>

    0-Routinepointer 1-disasm routine  2-<ea>reg 3-get <ea> routine
                     4-Dreg        5-cycle count   6-write<ea> routine   */

ULO i67dis(ULO prc, ULO opc, STR *st) {
  return arith1dis(prc, opc, st, 1);
}

void i67ini(void) {
  ULO op = 0x9000, ind, regd, modes, regs, flats, size, o;
  for (o = 0; o < 2; o++)
    for (size = 0; size < 3; size++)
      for (regd = 0; regd < 8; regd++) 
        for (modes = 0; modes < 8; modes++) 
          for (regs = 0; regs < 8; regs++) {
            flats = modreg(modes, regs);
            if ((o==0 && size == 0 && data[flats]) ||
                (o==0 && size != 0 && allmodes[flats]) ||
                (o==1 && alterable[flats] && memory[flats])) {
              ind = op | regd<<9 | o<<8 | size<<6 | modes<<3 | regs;
		t[ind][0] = (o == 0) ?
                             (size == 0) ?
                                (ULO) i67_B_1:
                                (size == 1) ?
                                   (ULO) i67_W_1:
                                   (ULO) i67_L_1:
                             (size == 0) ?
                                (ULO) i67_B_2:
                                (size == 1) ?
                                   (ULO) i67_W_2:
                                   (ULO) i67_L_2;
              t[ind][2] = greg(modes, regs);
              t[ind][3] = (ULO)((o == 0) ?
                             (size == 0) ?
                                arb[flats]:
                                (size == 1) ?
                                   arw[flats]:
                                   arl[flats]:
                             (size == 0) ?
                                parb[flats]:
                                (size == 1) ?
                                   parw[flats]:
                                   parl[flats]);
              t[ind][4] = greg(0, regd);
              t[ind][5] = (o == 0) ?
                             (size == 0) ?
                                (4+tarb[flats]):
                                (size == 1) ?
                                   (4+tarw[flats]):
                                   (flats <= 1 || flats == 11) ?
                                      (8+tarl[flats]):
                                      (6+tarl[flats]):
                             (size == 0) ?
                                (8 + tarb[flats]):
                                (size == 1) ?
                                   (8 + tarw[flats]):
                                   (12 + tarl[flats]);
              if (o == 1) t[ind][6] = (ULO) ((size == 0) ?
                                         awb[flats]:
                                         (size == 1) ?
                                            aww[flats]:
                                            awl[flats]);
              cpu_dis_tab[ind] = i67dis;
	    }
	  }
}


/*===============================================================
   Instruction SUBA.W/L <ea>,An
   Table-usage:

    0-Routinepointer 1-sourreg 2-sourceroutine
                     3-Areg        4-cycle count
  ===============================================================*/

ULO i68dis(ULO prc, ULO opc, STR *st) {
  return arith2dis(prc, opc, st, 1);
}

void i68ini(void) {
  ULO op = 0x90c0, ind, regd, modes, regs, flats, o;

  for (o = 0; o < 2; o++)
    for (regd = 0; regd < 8; regd++) 
      for (modes = 0; modes < 8; modes++) 
        for (regs = 0; regs < 8; regs++) 
          if (allmodes[(flats = modreg(modes, regs))]) {
            ind = op | regd<<9 | o<<8 | modes<<3 | regs;
	    t[ind][0] = (o == 0) ?
	      (ULO) i68_W:
	      (ULO) i68_L;
	    t[ind][1] = greg(modes, regs);
	    t[ind][2] = (ULO)((o == 0) ?
			      arw[flats]:
			      arl[flats]);
	    t[ind][3] = greg(1, regd);
	    t[ind][4] = (o==0) ?
	      (8 + tarw[flats]):
	      (flats <= 1 || flats == 11) ?
	      (8 + tarl[flats]):
	      (6 + tarl[flats]);
	    cpu_dis_tab[ind] = i68dis;
	  }
}

/* Instruction SUBI.X #i,<ea>
   Table-usage:

    0-Routinepointer 1-disasm routine  2-eareg 3-ea read routine
                     4-cycle count    5-ea write routine               */

ULO i69dis(ULO prc, ULO opc, STR *st) {
  return arith3dis(prc, opc, st, 1);
}

void i69ini(void) {
  ULO op = 0x0400, ind, modes, regs, flats, size;
  for (size = 0; size < 3; size++)
    for (modes = 0; modes < 8; modes++) 
      for (regs = 0; regs < 8; regs++) {
        flats = modreg(modes, regs);
        if (data[flats] && alterable[flats]) {
          ind = op | size<<6 | modes<<3 | regs;
          t[ind][0] = (size == 0) ?
                         (ULO) i69_B:
                         (size == 1) ?
                            (ULO) i69_W:
                            (ULO) i69_L; 
          t[ind][2] = greg(modes, regs);
          t[ind][3] = (ULO)((size == 0) ?
                         parb[flats]:
                         (size == 1) ?
                            parw[flats]:
                            parl[flats]);
          t[ind][4] = (size == 0) ?
                         (flats == 0) ?
                            8:
                            (12 + tarb[flats]):
                         (size == 1)  ?
                            (flats == 0) ?
                               8:
                               (12 + tarw[flats]):
                            (flats == 0) ?
                               16:
                               (20 + tarl[flats]);
          t[ind][5] = (ULO)((size == 0) ?
                         awb[flats]:
                         (size == 1) ? 
                            aww[flats]:
                            awl[flats]);
          cpu_dis_tab[ind] = i69dis;
	}
      }
}

/* Instruction SUBQ.X #i,<ea>
   Table-usage:

     0-Routinepointer 1-disasm routine  2-eareg 3-ea read routine
                      4-cycle count    5-ea write routine 6-SUB immediate */

ULO i70dis(ULO prc, ULO opc, STR *st) {
  return arith4dis(prc, opc, st, 1);
}

void i70ini(void) {
  ULO op = 0x5100, ind, imm, modes, regs, flats, size;
    for (size = 0; size < 3; size++)
      for (modes = 0; modes < 8; modes++) 
        for (regs = 0; regs < 8; regs++) 
          for (imm = 0; imm < 8; imm++) {
            flats = modreg(modes, regs);
            if ((size == 0 && alterable[flats] && modes != 1) ||
                (size != 0 && alterable[flats])) {
              ind = op | imm<<9 | size<<6 | modes<<3 | regs;
              t[ind][0] = (size == 0) ?
                             (ULO) i70_B:
                             (size == 1) ?
                                ((modes == 1) ? (ULO) i70_ADR:(ULO) i70_W):
                                ((modes == 1) ? (ULO) i70_ADR:(ULO) i70_L);
              cpu_dis_tab[ind] = i70dis;
              t[ind][2] = greg(modes, regs);
              t[ind][3] = (ULO)((size == 0) ?
                             parb[flats]:
                             (size == 1) ?
                                parw[flats]:
                                parl[flats]);
              t[ind][4] = (size == 0) ?
                             (flats == 0) ?
                                (4+tarb[flats]):
                                (8+tarb[flats]):
                             (size == 1) ?
                                (flats == 0) ?
                                   (4+tarw[flats]):
                                   (8+tarw[flats]):
                                (flats < 2) ?
                                   (8+tarl[flats]):
                                   (12+tarl[flats]);
              t[ind][5] = (ULO)((size == 0) ?
                             awb[flats]:
                             (size == 1) ?
                                aww[flats]:
                                awl[flats]);
              t[ind][6] = (imm == 0) ?
                             8:
                             imm;
              }
            }
}


/*===========================================================================
  Instruction SUBX.X Ry,Rx / SUBX.X -(Ay),-(Ax)
 
  Table-usage:
    0 - Emulation routine
    1 - Rx pointer (Destination)
    2 - Ry pointer (Source)
=============================================================================*/

ULO cpuSubXDis(ULO prc, ULO opc, STR *st) {
  return arith5dis(prc, opc, st, 1);
}

void cpuSubXIni(void) {
  ULO op = 0x9100, ind, rm, rx, ry, sz;

  for (sz = 0; sz <= 2; sz++)
    for (rm = 0; rm <= 1; rm++)
      for (rx = 0; rx <= 7; rx++) 
        for (ry = 0; ry <= 7; ry++) {
          ind = op | ry | (rx<<9) | (rm<<3) | (sz<<6);
          t[ind][0] = (rm == 0) ?
                         (sz == 0) ?
                            (ULO) cpuSubXB1:
                            (sz == 1) ?
                               (ULO) cpuSubXW1:
                               (ULO) cpuSubXL1:
                         (sz == 0) ?
                            (ULO) cpuSubXB2:
                            (sz == 1) ?
                               (ULO) cpuSubXW2:
                               (ULO) cpuSubXL2;
          t[ind][1] = greg(rm, rx);
          t[ind][2] = greg(rm, ry);
          cpu_dis_tab[ind] = cpuSubXDis;
          }
}


/*===========================================================================
  Instruction SWAP.W Dn
  
  Table-usage:
    0 - Emulation routine
    1 - Register pointer
=============================================================================*/

ULO cpuSwapWDis(ULO prc, ULO opc, STR *st) {
  return various2dis(prc, opc, st, 0);
}

void cpuSwapWIni(void) {
  ULO op = 0x4840, ind, reg;

  for (reg = 0; reg < 8; reg++) {
    ind = op | reg;
    t[ind][0] = (ULO) cpuSwapW;
    t[ind][1] = greg(0, reg);
    cpu_dis_tab[ind] = cpuSwapWDis;
  }
}


/*===========================================================================
  Instruction TAS.B <ea>

  Table-usage:
    0 - Emulation routine
    1 - EA register pointer
    2 - EA read routine
    3 - EA write routine
    4 - Cycle count
=============================================================================*/

ULO cpuTasBDis(ULO prc, ULO opc, STR *st) {
  return unarydis(prc, opc, st, 7);
}

void cpuTasBIni(void) {
  ULO op = 0x4ac0, ind, mode, reg, flat;

  for (mode = 0; mode < 8; mode++) 
    for (reg = 0; reg < 8; reg++) {
      flat = modreg(mode, reg);
      if (data[flat] && alterable[flat]) {
        ind = op | mode<<3 | reg;
	t[ind][0] = (ULO) cpuTasB;
	t[ind][1] = greg(mode, reg);
	t[ind][2] = (ULO) parb[flat];
	t[ind][3] = (ULO) awb[flat];
	t[ind][4] = (flat == 0) ? 4 : (10 + tarb[flat]);
        cpu_dis_tab[ind] = cpuTasBDis;
      }
    }
}


/*===========================================================================
  Instruction TRAP #n
  
  Table-usage:
    0 - Emulation routine
    1 - n*4 + $80
=============================================================================*/

ULO cpuTrapDis(ULO prc, ULO opc, STR *st) {
  sprintf(st, "$%.6X %.4X                   TRAP    #$%1X", prc, opc, opc & 0xf);
  return prc + 2;
}

void cpuTrapIni(void) {
  ULO op = 0x4e40, ind, vector;
  
  for (vector = 0; vector < 16; vector++) {
    ind = op | vector;
    t[ind][0] = (ULO) cpuTrap;
    t[ind][1] = vector*4 + 0x80;
    cpu_dis_tab[ind] = cpuTrapDis;
  }
}


/*===========================================================================
  Instruction TRAPV
   
  Table-usage:
    0 - Emulation routine
=============================================================================*/

ULO cpuTrapVDis(ULO prc, ULO opc, STR *st) {
  return singledis(prc, opc, st, 5);
}

void cpuTrapVIni(void) {
  ULO op = 0x4e76;

  t[op][0] = (ULO) cpuTrapV;
  cpu_dis_tab[op] = cpuTrapVDis;
}


/*===========================================================================
   Instruction TST.B/W/L <ea>

   Table-usage:
     0 - Emulation routine
     1 - EA register pointer
     2 - EA read routine
     3 - Cycle count

  On 020++ testing an address register of size word or long is permitted.
  On 020++ testing on modes beyond absolute.L is allowed.
=============================================================================*/

ULO cpuTstDis(ULO prc, ULO opc, STR *st) {
  return unarydis(prc, opc, st, 3);
}

void cpuTstIni(void) {
  ULO op = 0x4a00, ind, mode, reg, flat, size;

  for (size = 0; size < 3; size++)
    for (mode = 0; mode < 8; mode++) 
      for (reg = 0; reg < 8; reg++) {
        flat = modreg(mode, reg);
        if ((data[flat] && alterable[flat]) ||
	    ((cpu_major >= 2) && (size >= 1) && (flat == 1)) ||
	    ((cpu_major >= 2) && (flat >= 9))) {
          ind = op | (size<<6) | (mode<<3) | reg;
          t[ind][0] = (size == 0) ?
                       (ULO) cpuTstB:
                       (size == 1) ?
                          (ULO) cpuTstW:
                          (ULO) cpuTstL;
          t[ind][1] = greg(mode, reg);
          t[ind][2] = (ULO)((size == 0) ?
                       arb[flat]:
                       (size == 1) ?
                          arw[flat]:
                          arl[flat]);
          t[ind][3] = (size == 0) ?
                       (4 + tarb[flat]):
                       (size == 1) ?
                          (4 + tarw[flat]):
                          (4 + tarl[flat]);
          cpu_dis_tab[ind] = cpuTstDis;
	}
      }
}


/*===========================================================================
   Instruction UNLK An

  Table-usage:
    0 - Emulation routine
    1 - Address register pointer
=============================================================================*/

ULO cpuUnlkDis(ULO prc, ULO opc, STR *st) {
  return various2dis(prc, opc, st, 1);
}

void cpuUnlkIni(void) {
  ULO op = 0x4e58, ind, reg;
  
  for (reg = 0; reg < 8; reg++) {
    ind = op | reg;
    t[ind][0] = (ULO) cpuUnlk;
    t[ind][1] = greg(1, reg);
    cpu_dis_tab[ind] = cpuUnlkDis;
  }
}


/*===========================================================================
  Instruction BKPT #  (ILLEGAL on all processors, included for disassembly)

  Table-usage:
    0 - Emulation routine
=============================================================================*/

ULO cpuBkptDis(ULO prc, ULO opc, STR *st) {
  sprintf(st, "$%.6X %.4X                   BKPT    #$%1X", prc, opc, opc&0x7);
  return prc + 2;
}

void cpuBkptIni(void) {
  ULO op = 0x4848, ind, vector;

  for (vector = 0; vector < 8; vector++) {
    ind = op | vector;
    t[ind][0] = (ULO) i00;
    cpu_dis_tab[ind] = cpuBkptDis;
    }
}


/*====================================*/
/* 680X0 specific instructions follow */
/*====================================*/

/*==========================================================================
   Instruction BFCHG <ea>{offset:size}
   Instruction BFCLR <ea>{offset:size}
   Instruction BFEXTS <ea>{offset:size},Dn
   Instruction BFEXTU <ea>{offset:size},Dn
   Instruction BFFFO <ea>{offset:size},Dn
   Instruction BFINS Dn,<ea>{offset:size}
   Instruction BFSET <ea>{offset:size}
   Instruction BFTST <ea>{offset:size}

   Table-usage:
   0 - Routineptr, 1 - srcreg, 2-srcroutine, 3 - cycle count
  ==========================================================================*/

STR *bftxt[8] = {"TST ","EXTU","CHG ","EXTS","CLR ","FFO ","SET ","INS "};

ULO bfdis(ULO prc, ULO opc, STR *st) {
  ULO pos = 18, reg = get_sreg(opc), ext = fetw(prc + 2), n = (opc>>8) & 7;
  ULO offset = (ext & 0x7c0)>>6, width = ext & 0x1f;
  STR stmp[16];

  sprintf(st, "$%.6X %.4X %.4X              BF%s   ", prc, opc, ext, bftxt[n]);
  if (n == 7) {
    sprintf(stmp, "D%d,", (ext>>12) & 7);
    strcat(st, stmp);
  }
  prc = disAdrMode(reg, prc + 4, st, 16, modreg(get_smod(opc), reg), &pos);
  if (ext & 0x800) sprintf(stmp, "{D%d:", offset & 7);
  else sprintf(stmp, "{%d:", offset);
  strcat(st, stmp);
  if (ext & 0x20) sprintf(stmp, "D%d}", width & 7);
  else sprintf(stmp, "%d}", width);
  strcat(st, stmp);
  if ((n == 1) || (n == 3) || (n == 7)) {
    sprintf(stmp, ",D%d", (ext>>12) & 7);
    strcat(st, stmp);
  }
  return prc;
}

int bfadrselect(ULO n, ULO flats) {
  if ((n == 1) || (n == 3) || (n == 5)) return control[flats] || !flats;
  return (control[flats] && alterable[flats]) || !flats;
}

void bfini(ULO o, ULO Rfunc, ULO Mfunc) {
  ULO op = o, ind, modes, regs, flats, n = (o>>8) & 7;

  for (modes = 0; modes < 8; modes++)
    for (regs = 0; regs < 8; regs++) {
      flats = modreg(modes, regs);
      if (bfadrselect(n, flats)) {
        ind = op | modes<<3 | regs;
        t[ind][0] = (ULO) ((modes == 0) ? Rfunc : Mfunc);
        t[ind][1] = greg(modes, regs);
        t[ind][2] = (ULO) eac[flats];
	t[ind][3] = 4;
        cpu_dis_tab[ind] = bfdis;
      }
    }
}

void i301ini(void) {
  bfini(0xeac0, (ULO) i301_R, (ULO) i301_M);
  bfini(0xecc0, (ULO) i302_R, (ULO) i302_M);
  bfini(0xebc0, (ULO) i303_R, (ULO) i303_M);
  bfini(0xe9c0, (ULO) i304_R, (ULO) i304_M);
  bfini(0xedc0, (ULO) i305_R, (ULO) i305_M);
  bfini(0xefc0, (ULO) i306_R, (ULO) i306_M);
  bfini(0xeec0, (ULO) i307_R, (ULO) i307_M);
  bfini(0xe8c0, (ULO) i308_R, (ULO) i308_M);
}


/*==========================================================================
  Instruction CAS Dc,Du,<ea> CAS2 Dc1:Dc2,Du1:Du2,(Rn1):(Rn2)
   Table-usage:
   0 - Routinepointer, 1 - srcreg, 2 - srcroutine, 3 - cycle count
   0 - Routinepointer, 1 - cycle count (CAS2)
  ==========================================================================*/

ULO i309dis(ULO prc, ULO opc, STR *st) {
  ULO pos = 18, reg = get_sreg(opc), mode = get_smod(opc), ext = fetw(prc + 2);

  if ((opc & 0x3f) == 0x3c) { /* CAS2 */
    ULO ext2 = fetw(prc + 4);
    
    sprintf(st, "$%.6X %.4X %.4X %.4X  CAS2.%c  D%d:D%d,D%d:D%d,(%s%d):(%s%d)",
	    prc, opc, ext, ext2, sizec(get_size4(opc)), ext & 7, ext2 & 7,
	    (ext>>6) & 7, (ext2>>6) & 7, (ext & 0x8000) ? "A":"D",
            (ext >> 12) & 7, (ext2 & 0x8000) ? "A":"D", (ext2 >> 12) & 7);
    prc += 6;
  }
  else {
    mode = modreg(mode, reg);
    sprintf(st,"$%.6X %.4X %.4X              CAS.%c     D%d,D%d,", prc, opc,
	    ext, sizec(get_size4(opc)), ext & 7, (ext>>6) & 7);
    prc = disAdrMode(reg, prc + 4, st, 16, mode, &pos);
  }
  return prc;
}

void i309ini(void) {
  ULO op = 0x08c0, ind, siz, modes, regs, flats;

  for (siz = 1; siz < 4; siz++) /* CAS */
    for (modes = 0; modes < 8; modes++)
      for (regs = 0; regs < 8; regs++) {
        flats = modreg(modes, regs);
        if (memory[flats] && alterable[flats]) {
          ind = op | siz<<9 | modes<<3 | regs;
          t[ind][0] = (ULO) ((siz == 0) ? i309_B:((siz == 1) ? i309_W:i309_L));
          t[ind][1] = greg(modes, regs);
          t[ind][2] = (ULO) eac[flats];
	  t[ind][3] = 4;
          cpu_dis_tab[ind] = i309dis;
        }
      }
  op = 0x08fc;
  for (siz = 2; siz < 4; siz++) {
    ind = op | siz<<9;
    t[ind][0] = (ULO) ((siz == 1) ? i310_W:i310_L);
    t[ind][1] = 4;
    cpu_dis_tab[ind] = i309dis;
  }
}


/*==============================================================
   Instruction CHK.L <ea>,Dn
   Table-usage:
   0 - Routinepointer, 1 - sourreg, 2 - sourceroutine
                       3 - Dreg
  ==============================================================*/

ULO i311dis(ULO prc, ULO opc, STR *st) {  
  ULO pos = 13, reg = get_sreg(opc);

  sprintf(st, "$%.6X %.4X                   CHK.L   ", prc, opc);
  prc = disAdrMode(reg, prc + 2, st, 16, modreg(get_smod(opc), reg), &pos);
  strcat(st, ",");
  return disAdrMode(get_dreg(opc), prc + 2, st, 16, 0, &pos);
}

void i311ini(void) {
  ULO op = 0x4100, ind, regd, modes, regs, flats;

  for (regd = 0; regd < 8; regd++)
    for (modes = 0; modes < 8; modes++)
      for (regs = 0; regs < 8; regs++)
        if (data[flats = modreg(modes, regs)]) {
          ind = op | regd<<9 | modes<<3 | regs;
          t[ind][0] = (ULO) i311;
          t[ind][1] = greg(modes, regs);
          t[ind][2] = (ULO) arw[flats];
          t[ind][3] = greg(0, regd);
          cpu_dis_tab[ind] = i311dis;
          }
}


/*======================================================
   Instruction CHK2.X <ea>,Rn / CMP2.X <ea>,Rn
    Table-usage:
     0 - Routinepointer, 1 - source register
     2 - source routine, 3 - cycle count
  ======================================================*/

ULO i312dis(ULO prc,ULO opc,STR *st) {  
  ULO pos = 18, reg = get_sreg(opc),
      mode = get_smod(opc), ext = fetw(prc + 2);
  STR stmp[16];
  
  mode = modreg(mode, reg);
  sprintf(st, "$%.6X %.4X %.4X              %s2.%c  ", prc, opc, ext,
	  (ext & 0x800) ? "CHK" : "CMP", sizec(get_size4(opc)));
  prc = disAdrMode(reg, prc + 4, st, 16, mode, &pos);
  strcat(st, ",");
  sprintf(stmp, "%s%d", (ext & 0x8000) ? "A" : "D", (ext>>12) & 7);
  strcat(st, stmp);
  return prc;
}

void i312ini(void) {
  ULO op = 0x00c0, ind, siz, modes, regs, flats;

  for (siz = 0; siz < 3; siz++)
    for (modes = 0; modes < 8; modes++)
      for (regs = 0; regs < 8; regs++)
        if (control[flats = modreg(modes, regs)]) {
          ind = op | siz<<9 | modes<<3 | regs;
          t[ind][0] = (ULO) ((siz == 0) ? i312_B:((siz == 1) ? i312_W:i312_L));
          t[ind][1] = greg(modes, regs);
          t[ind][2] = (ULO) ((siz == 0) ? arb[flats] :
                             ((siz == 1) ? arw[flats] : arl[flats]));
          t[ind][3] = 4;
          cpu_dis_tab[ind] = i312dis;
        }
}


/*========================================================================
   Instruction cpBcc <label>
   Table-usage:
   0 - Routinepointer, 1 - disasm routine, 2 - coprocessor, 3 - condition
  ========================================================================*/

ULO i313dis(ULO prc, ULO opc, STR *st) {  
  ULO pos = 13, reg = get_sreg(opc), dreg = get_dreg(opc), mode =get_smod(opc);

  mode = greg(mode, reg);
  sprintf(st, "$%.6X %.4X      !!!!!!       cpBcc    ", prc, opc);
  prc = disAdrMode(reg, prc + 2, st, 16, mode, &pos);
  strcat(st, ",");
  disAdrMode(dreg, prc, st, 16, 0, &pos);  
  return prc;
}

void i313ini(void) {
  ULO op = 0xf080, ind, cpnum, cpnumstart, cond, siz;

  cpnumstart = (cpu_major == 3) ? 1 : 0;
  for (siz = 0; siz < 2; siz++)
    for (cpnum = cpnumstart; cpnum < 8; cpnum++)
      for (cond = 0; cond < 64; cond++) {
        ind = op | cpnum<<9 | siz<<6 | cond;
	if (t[ind][0] != (ULO)i00) printf("13\n");
        t[ind][0] = (ULO) ( (siz == 0) ? i313_W : i313_L );
        cpu_dis_tab[ind] = i313dis;
        t[ind][2] = cpnum;
        t[ind][3] = cond;
      }
}


/*==================================================================
   Instruction cpDBcc <label>
   Table-usage:
   0 - Routinepointer, 1 - disasm routine, 2 - coprocessor, 3 - *Dn
  ==================================================================*/

ULO i314dis(ULO prc,ULO opc,STR *st) {  
  ULO pos = 13, reg = get_sreg(opc), dreg = get_dreg(opc), mode =get_smod(opc);

  mode = greg(mode, reg);
  sprintf(st, "$%.6X %.4X      !!!!!!       cpDBcc   ", prc, opc);
  prc = disAdrMode(reg, prc + 2, st, 16, mode, &pos);
  strcat(st, ",");
  disAdrMode(dreg, prc, st, 16, 0, &pos);  
  return prc;
}

void i314ini(void) {
  ULO op = 0xf048, ind, cpnum, cpnumstart, reg;

  cpnumstart = (cpu_major == 3) ? 1 : 0;
  for (cpnum = cpnumstart; cpnum < 8; cpnum++)
    for (reg = 0; reg < 8; reg++) {
      ind = op | cpnum<<9 | reg;
      if (t[ind][0] != (ULO)i00) printf("14\n");
      t[ind][0] = (ULO) i314;
      cpu_dis_tab[ind] = i314dis;
      t[ind][2] = cpnum;
      t[ind][3] = greg(0, reg);
    }
}


/*========================================
   Instruction cpGEN
   Table-usage:
   0 - Routinepointer, 1 - disasm routine
   2 - coprocessor, 3 - reg, 4 - srcproc
  ========================================*/

ULO i315dis(ULO prc,ULO opc,STR *st) {  
  ULO pos = 13, reg = get_sreg(opc), dreg = get_dreg(opc),mode=(opc&0x38)>>3;

  if (mode == 7) mode += reg;
  sprintf(st,"$%.6X %.4X      !!!!!!       cpGEN    ",prc,opc);
  prc = disAdrMode(reg,prc+2,st,16,mode,&pos);
  strcat(st,",");
  disAdrMode(dreg,prc,st,16,0,&pos);  
  return prc;
}

void i315ini(void) {
  ULO op = 0xf000, ind, cpnum, cpnumstart, modes, regs, flats;

  cpnumstart = (cpu_major == 3) ? 1 : 0;
  for (cpnum = cpnumstart; cpnum < 8; cpnum++)
    for (modes = 0; modes < 8; modes++)
      for (regs = 0; regs < 8; regs++)
        if(allmodes[flats = modreg(modes, regs)]) {
          ind = op | cpnum<<9 | modes<<3 | regs;
	  if (t[ind][0] != (ULO)i00) printf("15\n");
          t[ind][0] = (ULO) i315;
          cpu_dis_tab[ind] = i315dis;
          t[ind][2] = cpnum;
          t[ind][3] = greg(modes, regs);
          t[ind][4] = (ULO) arw[flats];
        }
}


/*========================================
   Instruction cpRESTORE <ea>
   Table-usage:
   0 - Routinepointer, 1 - disasm routine
   2 - coprocessor, 3 - reg, 4 - srcproc
  ========================================*/

ULO i316dis(ULO prc,ULO opc,STR *st) {  
  ULO pos=13,reg=opc&0x7,dreg=(opc&0xe00)>>9,mode=(opc&0x38)>>3;

  if (mode == 7) mode += reg;
  sprintf(st,"$%.6X %.4X      !!!!!!       cpRESTORE",prc,opc);
  prc = disAdrMode(reg,prc+2,st,16,mode,&pos);
  strcat(st,",");
  disAdrMode(dreg,prc,st,16,0,&pos);
  return prc;
}

void i316ini(void) {
  ULO op = 0xf140, ind, cpnum, cpnumstart, modes, regs, flats;

  cpnumstart = (cpu_major == 3) ? 1 : 0;
  for (cpnum = cpnumstart; cpnum < 8; cpnum++)
    for (modes = 0; modes < 8; modes++)
      for (regs = 0; regs < 8; regs++) {
        flats = modreg(modes, regs);
        if( control[flats] || flats == 2 ) {
          ind = op|cpnum<<9|modes<<3|regs;
  	  if (t[ind][0] != (ULO)i00) printf("16\n");
          t[ind][0] = (ULO) i316;
          cpu_dis_tab[ind] = i316dis;
          t[ind][2] = cpnum;
          t[ind][3] = greg(1, regs);
          t[ind][4] = (ULO) arw[flats];
        }
      }
}


/*========================================
   Instruction cpSAVE <ea>
   Table-usage:
   0 - Routinepointer, 1 - disasm routine
   2 - coprocessor, 3 - reg, 4 - dstproc
  ========================================*/

ULO i317dis(ULO prc,ULO opc,STR *st) {  
  ULO pos=13,reg=opc&0x7,dreg=(opc&0xe00)>>9,mode=(opc&0x38)>>3;
  if (mode == 7) mode += reg;
  sprintf(st,"$%.6X %.4X      !!!!!!       cpSAVE   ",prc,opc);
  prc = disAdrMode(reg,prc+2,st,16,mode,&pos);
  strcat(st,",");
  disAdrMode(dreg,prc,st,16,0,&pos);
  return prc;
}

void i317ini(void) {
  ULO op = 0xf100, ind, cpnum, cpnumstart, modes, regs, flats;

  cpnumstart = (cpu_major == 3) ? 1 : 0;
  for (cpnum = cpnumstart; cpnum < 8; cpnum++)
    for (modes = 0; modes < 8; modes++)
      for (regs = 0; regs < 8; regs++) {
        flats = modreg(modes, regs);
        if( control[flats] || flats == 3 ) {
          ind = op|cpnum<<9|modes<<3|regs;
	  if (t[ind][0] != (ULO)i00) printf("17\n");
          t[ind][0] = (ULO) i317;
          cpu_dis_tab[ind] = i317dis;
          t[ind][2] = cpnum;
          t[ind][3] = greg(1, regs);
          t[ind][4] = (ULO) aww[flats];
        }
      }
}


/*========================================
   Instruction cpScc <ea>
   Table-usage:
   0 - Routinepointer, 1 - disasm routine
   2 - coprocessor, 3 - reg, 4 - dstproc
  ========================================*/

ULO i318dis(ULO prc,ULO opc,STR *st) {  
  ULO pos=13,reg=opc&0x7,dreg=(opc&0xe00)>>9,mode=(opc&0x38)>>3;

  if (mode == 7) mode += reg;
  sprintf(st,"$%.6X %.4X      !!!!!!       cpScc    ",prc,opc);
  prc = disAdrMode(reg,prc+2,st,16,mode,&pos);
  strcat(st,",");
  disAdrMode(dreg,prc,st,16,0,&pos);
  return prc;
}

void i318ini(void) {
  ULO op = 0xf040, ind, cpnum, cpnumstart, modes, regs, flats;

  cpnumstart = (cpu_major == 3) ? 1 : 0;
  for (cpnum = cpnumstart; cpnum < 8; cpnum++)
    for (modes = 0; modes < 8; modes++)
      for (regs = 0; regs < 8; regs++) {
        flats = modreg(modes, regs);
        if( data[flats] && alterable[flats] ) {
          ind = op|cpnum<<9|modes<<3|regs;
	if (t[ind][0] != (ULO)i00) printf("18\n");
          t[ind][0] = (ULO) i318;
          cpu_dis_tab[ind] = i318dis;
          t[ind][2] = cpnum;
          t[ind][3] = greg(modes, regs);
          t[ind][4] = (ULO) awb[flats];
        }
      }
}


/*========================================
   Instruction cpTRAPcc #
   Table-usage:
   0 - Routinepointer, 1 - disasm routine
   2 - coprocessor, 3 - operand type
  ========================================*/

ULO i319dis(ULO prc,ULO opc,STR *st) {
  ULO pos=13,reg=opc&0x7,dreg=(opc&0xe00)>>9,mode=(opc&0x38)>>3;

  if (mode == 7) mode += reg;
  sprintf(st,"$%.6X %.4X      !!!!!!       cpTRAPcc ",prc,opc);
  prc = disAdrMode(reg,prc+2,st,16,mode,&pos);
  strcat(st,",");
  disAdrMode(dreg,prc,st,16,0,&pos);
  return prc;
}

void i319ini(void) {
  ULO op = 0xf078, ind, cpnum, cpnumstart, optype;

  cpnumstart = (cpu_major == 3) ? 1 : 0;
  for (cpnum = cpnumstart; cpnum < 8; cpnum++)
    for( optype = 2; optype < 5; optype++) {
      ind = op|cpnum<<9|optype;
      if (t[ind][0] != (ULO)i00) printf("19\n");
      t[ind][0] = (ULO) i319;
      cpu_dis_tab[ind] = i319dis;
      t[ind][2] = cpnum;
      t[ind][3] = (ULO) ( (optype!=4) ? optype-1 : 0 );
    }
}


/*=============================================================================
   Instruction DIV(S/U).L <ea>,Dq / DIV(S/U).L <ea>,Dr:Dq /DIV(S/U)L <ea>,Dr:Dq
   Table-usage:
   0 - Routinepointer, 1 - source register, 2 - source routine
  ===========================================================================*/

ULO i320dis(ULO prc, ULO opc, STR *st) {
  ULO pos = 18, reg = get_sreg(opc), ext = fetw(prc + 2), dq, dr;
  STR stmp[16];
  
  dq = (ext>>12) & 7;
  dr = ext & 7;
  if (ext & 0x400)
    sprintf(st, "$%.6X %.4X %.4X              DIV%cL.L ", prc, opc, ext,
	    (ext & 0x800) ? 'S':'U');
  else
    sprintf(st, "$%.6X %.4X %.4X              DIV%c.L  ", prc, opc, ext,
	    (ext & 0x800) ? 'S':'U');
  prc = disAdrMode(reg, prc + 4, st, 32, modreg(get_smod(opc), reg), &pos);
  if (ext & 0x400 || dq != dr)
    sprintf(stmp, ",D%d:D%d", dr, dq);
  else
    sprintf(stmp, ",D%d", dq);
  strcat(st, stmp);
  return prc;
}

void i320ini(void) {
  ULO op = 0x4c40, ind, modes, regs, flats;

  for (modes = 0; modes < 8; modes++)
    for (regs = 0; regs < 8; regs++) {
      flats = modreg(modes, regs);
      if (data[flats]) {
        ind = op | modes<<3 | regs;
        t[ind][0] = (ULO) i320;
        t[ind][1] = greg(modes, regs);
        t[ind][2] = (ULO) arl[flats];
        cpu_dis_tab[ind] = i320dis;
      }
    }
}


/*====================================================*/
/* The complexity of this routine is not asm material */
/* Trade some cycles in for the ease of doing it in C */
/* Returns: 0 - div by zero  1 - OK, -1 - 060 unimpl  */
/*====================================================*/

#define DIV_OK 1
#define DIV_BY_ZERO 0
#define DIV_UNIMPLEMENTED_060 -1


LON i320_C(ULO ext, ULO divisor) {
  ULL q, dvsor, result, rest;
  LON dq, dr, size64, sign;
  BOOLE resultsigned = FALSE, restsigned = FALSE;
  
  if (divisor != 0) {
    dq = (ext>>12) & 7; /* Get operand registers, size and sign */
    dr = ext & 7;
    size64 = (ext>>10) & 1;
    if (size64 && (cpu_major == 6))
      return DIV_UNIMPLEMENTED_060;
    sign = (ext>>11) & 1;
    if (sign) {         /* Convert operands to 64 bit */
      if (size64) q = da_regs[0][dq] || (((ULL) da_regs[0][dr])<<32);
      else q = (LLO) (LON) da_regs[0][dq];
      dvsor = (LLO) (LON) divisor;
      if (((LLO) dvsor) < 0) {
	dvsor = -((LLO) dvsor);
	resultsigned = !resultsigned;
      }
      if (((LLO) q) < 0) {
	q = -((LLO) q);
	resultsigned = !resultsigned;
	restsigned = TRUE;
      }	
    }
    else {
      if (size64) q = da_regs[0][dq] || (((ULL) da_regs[0][dr])<<32);
      else q = (ULL) da_regs[0][dq];
      dvsor = (ULL) divisor;
    }
    if ((dvsor*0xffffffff + dvsor - 1) >= q) {
      result = q / dvsor;
      rest = q % dvsor;
      if (sign) {
	if (result > 0x80000000 && resultsigned) { /* Overflow */
	  sr &= 0xfffffffc;
	  sr |= 2;
	  return DIV_OK;
	}
	else if (result > 0x7fffffff && !resultsigned) { /* Overflow */
	  sr &= 0xfffffffc;
	  sr |= 2;
	  return DIV_OK;
	}
	if (resultsigned && (result != 0)) result = -((LLO) result);
	if (restsigned) rest = -((LLO) rest);
      }
      da_regs[0][dr] = (ULO) rest;
      da_regs[0][dq] = (ULO) result;   
      sr &= 0xfffffff0;
      if (((LLO) result) < 0) sr |= 8;
      else if (result == 0) sr |= 4;
    }
    else { /* Numerical overflow, leave operands untouched, set V */
      sr &= 0xfffffffc;
      sr |= 2;
      return DIV_OK;
    }
  }
  else return DIV_BY_ZERO; /* Division by zero, generate exception */
  return DIV_OK;
}


/*=========================================================
   Instruction EXTB Dx
   Table-usage:
     0 - Routine pointer 1 - Register Pointer
  =========================================================*/

ULO i321dis(ULO prc, ULO opc, STR *st) {  
  sprintf(st, "$%.6X %.4X                   EXTB.L  D%d", prc, opc,
	  get_sreg(opc));
  return prc + 2;
}

void i321ini(void) {
  ULO op = 0x49c0, ind, regs;

  for (regs = 0; regs < 8; regs++) {
    ind = op | regs;
    t[ind][0] = (ULO) i321;
    t[ind][1] = greg(0, regs);
    cpu_dis_tab[ind] = i321dis;
  }
}


/*==================================================
   Instruction LINK.L An,#
   Table-usage:
    0 - Routinepointer, 1 - register pointer
  ==================================================*/

ULO i322dis(ULO prc, ULO opc, STR *st) {
  sprintf(st, "$%.6X %.4X                   LINK.L  A%d", prc, opc,
	  get_sreg(opc));
  return prc + 2;
}

void i322ini(void) {
  ULO op = 0x4808, ind, regs;

  for (regs = 0; regs < 8; regs++) {
    ind = op | regs;
    t[ind][0] = (ULO) i322;
    t[ind][1] = greg(0, regs);
    cpu_dis_tab[ind] = i322dis;
  }
}


/*==================================================================
   Instruction MOVE.W CCR,<ea>
   Table-usage:
    0 - Routinepointer, 1 - dstreg, 2 - dstroutine, 3 - cycle count
  ==================================================================*/

ULO i323dis(ULO prc, ULO opc, STR *st) {
  ULO pos = 13, reg = get_sreg(opc);

  sprintf(st, "$%.6X %.4X                   MOVE.W  CCR,", prc, opc);
  return disAdrMode(reg, prc + 2, st, 16, modreg(get_smod(opc), reg), &pos);
}

void i323ini(void) {
  ULO op = 0x42c0, ind, reg, mode, flat;

  for (reg = 0; reg < 8; reg++)
    for (mode = 0; mode < 8; mode++) {
      flat = modreg(mode, reg);
      if (data[flat] && alterable[flat]) {
        ind = op | mode<<3 | reg;
        t[ind][0] = (ULO) i323;
        t[ind][1] = greg(mode, reg);
        t[ind][2] = (ULO) aww[flat];
        t[ind][3] = 12 + tarw[flat];
        cpu_dis_tab[ind] = i323dis;
      }
    }
}


/*=================================
   Instruction MOVEC Rc,Rn / Rn,Rc
   Table-usage:
    0 - Routinepointer
  =================================*/

ULO i324dis(ULO prc, ULO opc, STR *st) {
  ULO creg, extw = fetw(prc + 2);
  STR stmp[16];

  sprintf(st, "$%.6X %.4X %.4X              MOVEC.L ", prc, opc, extw);
  if (opc & 1) { /* To control register */
    sprintf(stmp, "%s%d,", (extw & 0x8000) ? "A" : "D", (extw>>12) & 7); 
    strcat(st, stmp);
  }
  creg = extw & 0xfff;
  if (cpu_major == 1 && ((creg != 0) && (creg != 1) && (creg != 0x800) &&
			       (creg != 0x801))) creg = 0xfff;
  switch (creg) {
    case 0x000:    /* SFC, Source function code X12346*/
      strcat(st, "SFC");
      break;
    case 0x001:    /* DFC, Destination function code X12346 */
      strcat(st, "DFC");
      break;
    case 0x800:    /* USP, User stack pointer X12346 */
      strcat(st, "USP");
      break;
    case 0x801:    /* VBR, Vector base register X12346 */
      strcat(st, "VBR");
      break;
    case 0x002:    /* CACR, Cache control register XX2346 */
      strcat(st, "CACR");
      break;
    case 0x802:    /* CAAR, Cache adress register XX2346 */
      strcat(st, "CAAR");
      break;
    case 0x803:    /* MSP, Master stack pointer XX234X */
      strcat(st, "MSP");
      break;
    case 0x804:    /* ISP, Interrupt stack pointer XX234X */
      strcat(st, "ISP");
      break;
    default:
      strcat(st, "ILLEGAL!");
      break;
  }
  if (!(opc & 1)) { /* From control register */
    sprintf(stmp, ",%s%d", (extw & 0x8000) ? "A":"D", (extw>>12) & 7); 
    strcat(st, stmp);
  }
  return prc + 4;
}

void i324ini(void) {
  ULO op = 0x4e7a, ind, mode;

  for (mode = 0; mode < 2; mode++) {
    ind = op | mode;
    t[ind][0] = (ULO) ((mode == 0) ? i324_FROM : i324_TO);
    cpu_dis_tab[ind] = i324dis;
  }
}


/*===========================================
   Instruction MOVES Rn,<ea> / <ea>,Rn
   Table-usage:

   0 - Routinepointer, 1 - register pointer
   2 - srcroutine,     3 - dstroutine
  ===========================================*/

ULO i325dis(ULO prc, ULO opc, STR *st) {
  ULO pos = 18, reg = get_sreg(opc), ext = fetw(prc + 2);
  STR stmp[16];
 
  sprintf(st, "$%.6X %.4X %.4X              MOVES.%c ", prc, opc, ext,
	  sizec(get_size(opc)));
  if (ext & 0x800) {
    sprintf(stmp, "%s%d,", (ext & 0x8000) ? "A":"D", (ext>>12) & 7);
    strcat(st, stmp);
  }
  prc = disAdrMode(reg, prc + 4, st, 8, modreg(get_smod(opc), reg), &pos);
  if (!(ext & 0x800)) {
    sprintf(stmp, ",%s%d", (ext & 0x8000) ? "A":"D", (ext>>12) & 7);
    strcat(st, stmp);
  }
  return prc;
}

void i325ini(void) {
  ULO op = 0x0e00, ind, siz, reg, mode, flat;

  for (siz = 0; siz < 3; siz++)
    for (reg = 0; reg < 8; reg++)
      for (mode = 0; mode < 8; mode++) {
        flat = modreg(mode, reg);
        if (memory[flat] && alterable[flat]) {
          ind = op | siz<<6 | mode<<3 | reg;
          t[ind][0] = (ULO) ( (siz == 0) ? i325_B :
                              (siz == 1) ? i325_W : i325_L );
          t[ind][1] = greg(1, reg);
          t[ind][2] = (ULO) ( (siz == 0) ? arb[flat] :
                              (siz == 1) ? arw[flat] : arl[flat] );
          t[ind][3] = (ULO) ( (siz == 0) ? awb[flat] :
                              (siz == 1) ? aww[flat] : awl[flat] );
          /*t[ind][5] = 12 + tarw[flat];*/
          cpu_dis_tab[ind] = i325dis;
        }
      }
}


/*========================================================
   Instruction MUL(S/U).L <ea>,Dn / MUL(S/U).L <ea>,Dh:Dl
   Table-usage:
   0 - Routinepointer, 1 - sourcereg, 2 - sourceroutine
  ========================================================*/

ULO i326dis(ULO prc, ULO opc, STR *st) {
  ULO pos = 18, reg = get_sreg(opc), ext = fetw(prc + 2), dl, dh;
  STR stmp[16];
  
  dl = (ext>>12) & 7;
  dh = ext & 7;
  sprintf(st, "$%.6X %.4X %.4X              MUL%c.L  ", prc, opc, ext,
	  (ext & 0x800) ? 'S':'U');
  prc = disAdrMode(reg, prc + 4, st, 32, modreg(get_smod(opc), reg), &pos);
  if (ext & 0x400)
    sprintf(stmp, ",D%d:D%d", dh, dl);
  else
    sprintf(stmp, ",D%d", dl);
  strcat(st, stmp);
  return prc;
}

void i326ini(void) {
  ULO op = 0x4c00, ind, modes, regs, flats;

  for (modes = 0; modes < 8; modes++)
    for (regs = 0; regs < 8; regs++)
      if (data[flats = modreg(modes, regs)]) {
        ind = op | modes<<3 | regs;
        t[ind][0] = (ULO) i326;
        t[ind][1] = greg(modes, regs);
        t[ind][2] = (ULO) arl[flats];
        cpu_dis_tab[ind] = i326dis;
      }
}


/*==================================================
   Instruction PACK -(Ax),-(Ay),# / Dx,Dy,#
   Table-usage:
   0 - Routinepointer, 1 - srcreg
   2 - srcroutine, 3 - dstreg, 4 - dstroutine
  ==================================================*/

ULO i327dis(ULO prc, ULO opc, STR *st) {
  ULO pos = 13, reg = get_sreg(opc), dreg = get_dreg(opc), mode = (opc & 8)>>1,
      adjw = fetw(prc + 2);

  sprintf(st,"$%.6X %.4X %.4X !!!!!!!      PACK    ", prc, opc, adjw);
  prc = disAdrMode(reg, prc + 4, st, 16, mode, &pos);
  strcat(st,",");
  return disAdrMode(dreg, prc, st, 16, mode, &pos);
}

void i327ini(void) {
  ULO op = 0x8140, ind, rm, rx, ry;

  for (rm = 0; rm < 2; rm++)
    for (rx = 0; rx < 8; rx++)
      for (ry = 0; ry < 8; ry++) {
        ind = op | ry<<9 | rm<<3 | rx;
        t[ind][0] = (ULO) i327;
        t[ind][1] = greg(rm, rx);
        t[ind][2] = (ULO) arw[rm<<2];
        t[ind][3] = greg(rm, ry);
        t[ind][4] = (ULO) awb[rm<<2];
        cpu_dis_tab[ind] = i327dis;
      }
}

/*===================================================================
   Instruction PFLUSH A / <fc>,# / <fc>,#,<ea>
   Instruction PLOADR <fc>,<ea> / PLOADW <fc>,<ea>
   Instruction PMOVE reg,<ea> / <ea>,reg / PMOVEFD <ea>,reg
   Instruction PTESTR/W <fc>,<ea>,# / <fc>,<ea>,#,An
   Table-usage:
   0-Routinepointer 1-disasm routine  2-srcreg 3-srcroutine

   PFLUSH, On 68030 and 68851 same opcode, 851 slightly more options
   PFLUSH, On 68040, different from 68030 
   In any case, test S bit and then NOOP
  ===================================================================*/


/* Disassemble for 030 */

void i328dis_030_print_fc(STR *st, ULO fcode) {
  STR stmp[16];

  if (fcode == 0) strcat(st, "   SFC,");
  else if (fcode == 1) strcat(st, "   DFC,");
  else if ((fcode & 0x18) == 8) {
    sprintf(stmp, "    D%d,", fcode & 7);
    strcat(st, stmp);
  }
  else if ((fcode & 0x18) == 0x10) {
    sprintf(stmp, "    #%d,", fcode & 7);
    strcat(st, stmp);
  }
}

ULO i328dis_030(ULO prc, ULO opc, STR *st) {
  ULO pos = 13, reg = get_sreg(opc), ext = fetw(prc + 2), mode, fcode, mask,op;
  STR stmp[16];
  
  mode = (ext >> 10) & 7;
  fcode = ext & 0x1f;
  mask = (ext >> 5) & 7;
  op = (ext >> 13) & 7;
  
  if (op == 0x1) {
    if (mode != 0) {  /* PFLUSH */
      sprintf(st, "$%.6X %.4X %.4X              PFLUSH", prc, opc, ext);
      prc += 4;
      if (mode == 1) strcat(st, "A");
      else {
        i328dis_030_print_fc(st, fcode);
        sprintf(stmp, "%.3X", mask);
        strcat(st, stmp);
        if (mode == 6) {
	  strcat(st, ",");
          prc = disAdrMode(reg, prc, st, 16, modreg(get_smod(opc), reg), &pos);
        }
      }
    }
    else { /* PLOAD */
      sprintf(st, "$%.6X %.4X %.4X              PLOAD%c", prc, opc, ext,
	      (ext & 0x200) ? 'R':'W');
      prc += 4;
      i328dis_030_print_fc(st, fcode);
      strcat(st, ",");
      prc = disAdrMode(reg, prc, st, 16, modreg(get_smod(opc), reg), &pos);
    }
  }
  else if (op == 4) { /* PTEST */
      sprintf(st, "$%.6X %.4X %.4X              PTEST ", prc, opc, ext);
      prc += 4;
      prc = disAdrMode(reg, prc, st, 16, modreg(get_smod(opc), reg), &pos);
  }
  else if (op == 0 || op == 2 || op == 3) { /* PMOVE */
      sprintf(st, "$%.6X %.4X %.4X              PMOVE ", prc, opc, ext);
      prc += 4;
      prc = disAdrMode(reg, prc, st, 16, modreg(get_smod(opc), reg), &pos);
  }
  return prc;
}

/* PFLUSH disassemble on EC040 */

ULO PFLUSHDisEC040(ULO prc, ULO opc, STR *st) {
  ULO reg = get_sreg(opc), opmode;
  STR namesuff[16];
  
  opmode = (opc & 0x18)>>3;
  switch (opmode) {
    case 0:
      sprintf(namesuff, "N  (A%d)", reg);
      break;
    case 1:
      sprintf(namesuff, "   (A%d)", reg);
      break;
    case 2:
      strcpy(namesuff, "AN");
      break;
    case 3:
      strcpy(namesuff, "A");
      break;
  }      
  sprintf(st, "$%.6X %.4X                   PFLUSH%s", prc, opc, namesuff);
  return prc + 2;
}

/* PTEST disassemble on EC040 */

ULO PTESTDisEC040(ULO prc, ULO opc, STR *st) {
  ULO reg = get_sreg(opc), rw;
  
  rw = (opc & 0x20)>>5;
  sprintf(st, "$%.6X %.4X                   PTEST%c (A%d)", prc, opc,
	  (rw) ? "R" : "W", reg);
  return prc + 2;
}

/*
   030: Coprocessor ID 0 is MMU, EC030 has a couple of NOP MMU instructions
        Unimplemented ID 0 instructions take priv. violation in User mode
	F-line exception in Supervisor
*/

void MMUini030(void) {
  ULO op, ind, modes, regs, flats;

  op = 0xf000;
  for (ind = 0xf000; ind < 0xf200; ind++) { /* Clear ID 0 instructions */
    t[ind][0] = (ULO) MMUIllegal030;
    cpu_dis_tab[ind] = i00dis;
  }
  for (modes = 0; modes < 8; modes++)
    for (regs = 0; regs < 8; regs++) {
      flats = modreg(modes, regs);
      if (control[flats] && alterable[flats]) {
	ind = op | modes<<3 | regs;
	t[ind][0] = (ULO) MMU030;
	t[ind][1] = (ULO) greg(modes, regs);
	t[ind][2] = (ULO) arl[flats];
	cpu_dis_tab[ind] = i328dis_030;
      }
    }
}


/*
   040: All unimplemented Coprocessor opcodes generate F-lines
*/

void MMUIni040(void) {
  ULO op, ind, modes, regs;

  op = 0xf500;                          /* PFLUSH on EC040, = NOP */
  for (modes = 0; modes < 4; modes++)
    for (regs = 0; regs < 8; regs++) {
      ind = op | modes<<3 | regs;
      t[ind][0] = (ULO) PFLUSHEC040;
      cpu_dis_tab[ind] = PFLUSHDisEC040;
    }
  op = 0xf548;                          /* PTEST on EC040, = NOP */ 
  for (modes = 0; modes < 2; modes++)
    for (regs = 0; regs < 8; regs++) {
      ind = op | modes<<5 | regs;
      t[ind][0] = (ULO) PTESTEC040;
      cpu_dis_tab[ind] = PTESTDisEC040;
    }
}


/*====================================
  Instruction RTD
   Table-usage:
   0-Routinepointer, 1-disasm routine
  ====================================*/

ULO i329dis(ULO prc, ULO opc, STR *st) {
  ULO extw = fetw(prc + 2);

  sprintf(st, "$%.6X %.4X %.4X              RTD    #%.4X", prc, opc,extw,extw);
  return prc + 4;
}

void i329ini(void) {
  if (t[0x4e74][0] != (ULO)i00) printf("29\n");
  t[0x4e74][0] = (ULO) i329;
  cpu_dis_tab[0x4e74] = i329dis;
}


/*===========================================================================
   Instruction TRAPcc
   Table-usage:
   0 - Routinepointer, 1 - handle routine, 2 - paramsize
  ===========================================================================*/

ULO i330dis(ULO prc, ULO opc, STR *st) {
  ULO bratype = get_branchtype(opc), ext = 0, op;
  char stmp[16];
  
  op = (opc & 7) - 2;
  if (op == 2)
    sprintf(st,"$%.6X %.4X                   TRAP",prc,opc);
  else if (op == 0) {
    ext = fetw(prc += 2);
    sprintf(st,"$%.6X %.4X %.4X              TRAP",prc,opc, ext);
  }
  else if (op == 1) {
    ext = fetl(prc += 4);
    sprintf(st,"$%.6X %.4X %.8X          TRAP",prc,opc, ext);
  }
  if (bratype == 0) strcat(st, "T    ");
  else if (bratype == 1) strcat(st, "F    ");
  else {
    strcat(st, btab[bratype]);
  }
  if (op == 0) {
    sprintf(stmp, ".W    #%.4X", ext);
    strcat(st, stmp);
  }
  else if (op == 1) {
    sprintf(stmp, ".L    #%.8X", ext);
    strcat(st, stmp);
  }
  return prc;
}


void i330ini(void) {
  ULO op = 0x50f8, c, mode, ind;
  ULO routines[16] = {(ULO) cc0,(ULO) cc1,(ULO) cc2,(ULO) cc3,(ULO) cc4,
		      (ULO) cc5,(ULO) cc6,(ULO) cc7,(ULO) cc8,(ULO) cc9,
		      (ULO) cca,(ULO) ccb,(ULO) ccc,(ULO) ccd,(ULO) cce,
		      (ULO) ccf};
  for (c = 0; c < 16; c++)
    for (mode = 2; mode < 5; mode++) {
      ind = op | c<<8 | mode;
	if (t[ind][0] != (ULO)i00) printf("30\n");
      t[ind][0] = (ULO) i330;
      t[ind][1] = routines[c];
      t[ind][2] = (mode != 4) ? mode-1 : 0;
      cpu_dis_tab[ind] = i330dis;
      }
}


/*==================================================
   Instruction UNPK -(Ax),-(Ay),# / Dx,Dy,#
   Table-usage:
   0 - Routinepointer, 1 - srcreg
   2 - srcroutine, 3 - dstreg, 4 - dstroutine
  ==================================================*/

ULO i331dis(ULO prc, ULO opc, STR *st) {
  ULO pos = 18, reg = get_sreg(opc), dreg = get_dreg(opc), mode = (opc & 8)>>1,
      adjw = fetw(prc + 2);

  sprintf(st, "$%.6X %.4X %.4X              UNPK    ", prc, opc, adjw);
  prc = disAdrMode(reg, prc + 4, st, 16, mode, &pos);
  strcat(st, ",");
  return disAdrMode(dreg, prc, st, 16, mode, &pos);
}

void i331ini(void) {
  ULO op = 0x8180, ind, rm, rx, ry;

  for (rm = 0; rm < 2; rm++)
    for (rx = 0; rx < 8; rx++)
      for (ry = 0; ry < 8; ry++) {
        ind = op | ry<<9 | rm<<3 | rx;
	if (t[ind][0] != (ULO)i00) printf("31\n");
        t[ind][0] = (ULO) i327;
        t[ind][1] = greg(rm, rx);
        t[ind][2] = (ULO) arw[rm<<2];
        t[ind][3] = greg(rm, ry);
        t[ind][4] = (ULO) awb[rm<<2];
        cpu_dis_tab[ind] = i327dis;
      }
}


/*===============================================
  Instruction CALLM #x,<ea>
   Table-usage:
   0 - Routinepointer, 1 - srcreg, 2 - earoutine
  ===============================================*/

ULO i332dis(ULO prc, ULO opc, STR *st) {
  ULO pos = 18, reg = get_sreg(opc), ext = fetw(prc + 2);
  
  sprintf(st, "$%.6X %.4X %.4X              CALLM   #%d,", prc, opc, ext,
	  ext & 0xff);
  return disAdrMode(reg, prc + 4, st, 16, modreg(get_smod(opc), reg), &pos);
}

void i332ini(void) {
  ULO op = 0x06c0, ind, mode, reg, flat;

  for (mode = 0; mode < 8; mode++)
    for (reg = 0; reg < 8; reg++)
      if (control[flat = modreg(mode, reg)]) {
        ind = op | mode<<3 | reg;
	if (t[ind][0] != (ULO)i00) printf("32\n");
        t[ind][0] = (ULO) i332;
        t[ind][1] = greg(mode, reg);
        t[ind][2] = (ULO) eac[flat];
        cpu_dis_tab[ind] = i332dis;
      }
}


/*=================================================
   Instruction RTM
   Table-usage:
   0 - Routinepointer, 1 - reg
  =================================================*/

ULO i333dis(ULO prc, ULO opc, STR *st) {  
  sprintf(st, "$%.6X %.4X                   RTM      %c%d", prc, opc,
	  (opc & 8) ? 'A':'D', get_sreg(opc));
  return prc + 2;
}

void i333ini(void) {
  ULO op = 0x06c0, ind, mode, reg;

  for (mode = 0; mode < 2; mode++)
    for (reg = 0; reg < 8; reg++) {
      ind = op | mode<<3 | reg;
	if (t[ind][0] != (ULO)i00) printf("33\n");
      t[ind][0] = (ULO) i333;
      t[ind][1] = greg(mode, reg);
      cpu_dis_tab[ind] = i333dis;
    }
}


/*======================================================================
   Instruction MOVE16 (040 060)

   Type 1: move16 (Ax)+, (Ay)+   (post-increment format)

   Type 2: move16 $L, (An)   11  (absolute long address or destination)
           move16 $L, (An)+  01
	   move16 (An), $L   10
	   move16 (An)+, $L  00    

   Table-usage:
     0 - Routine-pointer  1 - Pointer to address register
  ======================================================================*/

ULO i334dis(ULO prc, ULO opc, STR *st) {  
  sprintf(st, "$%.6X %.4X                   MOVE16", prc, opc);
  if (opc & 0x0020) return prc + 2;
  return prc + 6;
}

void i334ini(void) {
  ULO op, reg, mode, ind;
  ULO instructions[4] = {(ULO) i334_2_00, (ULO) i334_2_01,
			 (ULO) i334_2_10, (ULO) i334_2_11};
  
  op = 0xf620;           /* Type 1, second register in extension word */
  for (reg = 0; reg < 8; reg++) {
    ind = op | reg;
    t[ind][0] = (ULO) i334_1;
    t[ind][1] = greg(1, reg);
    cpu_dis_tab[ind] = i334dis;
  }

  op = 0xf600;           /* Type 2 */
  for (mode = 0; mode < 4; mode++)
    for (reg = 0; reg < 8; reg++) {
      ind = op | mode << 3 | reg;
      t[ind][0] = instructions[mode];
      t[ind][1] = greg(1, reg);
      cpu_dis_tab[ind] = i334dis;
    }
}


/*======================================================================
   Instruction CINV (040 040LC)

   CINVL  <cache>, (An)    Line invalidate
   CINVP  <cache>, (An)    Page invalidate
   CINVA  <cache>          All invalidate

   Table-usage:
     0 - Routine-pointer,  1 - register ptr,  2 - cache spec, 3 - scope
  ======================================================================*/


void CINVGetCacheDesc(ULO cache, STR *cachedesc) {
  switch (cache) {
    case 0:
      strcpy(cachedesc, "NOP");
      break;
    case 1:
      strcpy(cachedesc, "INST");
      break;
    case 2:
      strcpy(cachedesc, "DATA");
      break;
    case 3:
      strcpy(cachedesc, "BOTH");
      break;
  }  
}

void CINVGetScopeDesc(ULO scope, STR *scopedesc, STR *cachedesc, ULO reg) {
  switch (scope) {
    case 1:
      sprintf(scopedesc, "L  %s, (A%d)", cachedesc, reg);
      break;
    case 2:
      sprintf(scopedesc, "P  %s, (A%d)", cachedesc, reg);
      break;
    case 3:
      sprintf(scopedesc, "A  %s", cachedesc);
      break;
  }      
}

ULO CINVDis040(ULO prc, ULO opc, STR *st) {
  ULO reg = get_sreg(opc), cache = (opc>>6) & 3, scope =(opc>>3) & 3;
  STR scopedesc[32];
  STR cachedesc[8];

  CINVGetCacheDesc(cache, cachedesc);
  CINVGetScopeDesc(scope, scopedesc, cachedesc, reg);
  sprintf(st, "$%.6X %.4X                   CINV%s", prc, opc, scopedesc);
  return prc + 2;
}

void CINVIni(void) {
  ULO op, ind, cache, regs, scope;

  if (cpu_major == 4) {
    op = 0xf400;
    for (cache = 0; cache < 4; cache++)
      for (scope = 1; scope < 4; scope++)
	for (regs = 0; regs < 8; regs++) {
	  ind = op | cache<<6 | scope<<3 | regs;
	  t[ind][0] = (ULO) CINV040;
	  t[ind][1] = greg(1, regs);
	  t[ind][2] = cache;
	  t[ind][3] = scope;
	  cpu_dis_tab[ind] = CINVDis040;
    }
  }
}


/*======================================================================
   Instruction CPUSH (040 040LC)

   CPUSHL  <cache>, (An)    Line
   CPUSHP  <cache>, (An)    Page
   CPUSHA  <cache>          All

   Table-usage:
     0 - Routine-pointer,  1 - register ptr,  2 - cache spec, 3 - scope
  ======================================================================*/


ULO CPUSHDis040(ULO prc, ULO opc, STR *st) {
  ULO reg = get_sreg(opc), cache = (opc>>6) & 3, scope =(opc>>3) & 3;
  STR scopedesc[32];
  STR cachedesc[8];

  CINVGetCacheDesc(cache, cachedesc);
  CINVGetScopeDesc(scope, scopedesc, cachedesc, reg);
  sprintf(st, "$%.6X %.4X                   CPUSH%s", prc, opc, scopedesc);
  return prc + 2;
}

void CPUSHIni(void) {
  ULO op, ind, cache, regs, scope;

  if (cpu_major == 4) {
    op = 0xf420;
    for (cache = 0; cache < 4; cache++)
      for (scope = 1; scope < 4; scope++)
	for (regs = 0; regs < 8; regs++) {
	  ind = op | cache<<6 | scope<<3 | regs;
	  t[ind][0] = (ULO) CPUSH040;
	  t[ind][1] = greg(1, regs);
	  t[ind][2] = cache;
	  t[ind][3] = scope;
	  cpu_dis_tab[ind] = CPUSHDis040;
    }
  }
}


/*===========================================================================*/
/* createstatustab                                                           */
/* Will translate the flags C X V N Z som the intel flag register            */
/* X is set to the same as C since Intel doesn't have X                      */
/*===========================================================================*/

void cpuCreateStatusTable(void) {
  ULO i;

  for (i = 0; i < 4096; i++)
    statustab[i]=(i&0x1)|((i&0x1)<<4)|((i&0xc0)>>4)|((i&0x800)>>10);
}


/* Make muls and mulu tables for clock calculations */

void cpuMakeMulTable(void) {
  ULO i, j, k;

  for (i = 0; i < 256; i++) {
    j = 0;
    for (k = 0; k < 8; k++)
      if (((i>>k) & 1) == 1)
	j++;
    mulutab[i] = (UBY) j;
    j = 0;
    for (k = 0; k < 8; k++)
      if ((((i>>k) & 3) == 1) || (((i>>k) & 3) == 2))
	j++; 
    mulstab[i] = (UBY) j;
  }
}

/* Sets the opcode table to illegal (i00) and similar disasm routine */

void cpuClearOpcodeTable(void) {
  ULO i, j;

  for (i = 0; i <= 65535; i++) {
    t[i][0] = (ULO) &i00;
    cpu_dis_tab[i] = i00dis;
    for (j = 2; j <= 7; j++)
      t[i][j] = 0;
  }
}

/* BCD number translation tables. */
/* NBCD can generate result 255, which must translate to 0x99 */
/* If the input is not a bcd number, the result is unpredictable.... */

void cpuMakeBCDTables(void) {
  ULO x;

  for (x = 0; x < 100; x++) hex2bcd[x] = (UBY) (x % 10 | (x / 10) << 4);
  for (x = 100; x < 255; x++) hex2bcd[x] = 0;
  hex2bcd[255] = 0x99;
  for (x = 0; x < 256; x++)
    bcd2hex[x] = (UBY) ((((x & 0xf) < 10) && ((x & 0xf0) < 0xa0)) ? ((x & 0xf) + ((x & 0xf0)>>4)*10) : 0);
}

/* Irq table initialization */

static UBY cpuIRQTableDecide(ULO runlevel, ULO req) {
  LON actualrunlevel = -1;
  ULO i;

  for (i = 0; i < 14; i++) {
    if ((req & 1) && (int2int[i] > runlevel) && (actualrunlevel == -1))
      actualrunlevel = int2int[i];
    req >>= 1;
  }
  return (UBY) actualrunlevel;
}	
  
void cpuIRQTableInit(void) {
  ULO runlevel, req;
  
  for (runlevel = 0; runlevel < 8; runlevel++)
    for (req = 0; req < 0x4000; req++)
      irq_table[req | (runlevel<<14)] = cpuIRQTableDecide(runlevel, req);
}      


/*===================*/
/* Get/Set PC from C */
/*===================*/

void cpuSetPC(ULO address) {
#ifdef PC_PTR
  UBY *pcptr;

  pcptr = memory_bank_pointer[address>>16];
  if (pcptr == NULL)
    cpu_pc_in_unmapped_mem();
  pcbaseadr = pcptr;
  pc = (ULO) (pcptr + address);
#else
  pc = (ULO) address;
#endif
}

ULO cpuGetPC(ULO address) {
#ifdef PC_PTR
  return address - (ULO) pcbaseadr;
#else
  return address;
#endif
}


/*=====================================*/
/* Fill prefetch word (for use from C) */
/*=====================================*/

#ifdef PREFETCH
void cpuPrefetchFill(ULO address) {
  prefetch_word = fetw(address);
}
#endif

/*============================*/
/* Init stack frame jmptables */
/*============================*/

void cpuStackFrameInit000(void) {
  ULO i;

  for (i = 0; i < 64; i++)
    cpu_stack_frame_gen[i] = cpuGroup1;/* Avoid NULL ptrs */
  cpu_stack_frame_gen[2] = cpuGroup2;  /* 2 - Bus error */
  cpu_stack_frame_gen[3] = cpuGroup2;  /* 3 - Address error */
}

void cpuStackFrameInit010(void) {
  ULO i;

  for (i = 0; i < 64; i++)
    cpu_stack_frame_gen[i] = cpuFrame0;/* Avoid NULL ptrs */
  cpu_stack_frame_gen[2] = cpuFrame8;  /* 2 - Bus error */
  cpu_stack_frame_gen[3] = cpuFrame8;  /* 3 - Address error */
}

void cpuStackFrameInit020(void) {
  ULO i;

  for (i = 0; i < 64; i++)
    cpu_stack_frame_gen[i] = cpuFrame0;/* Avoid NULL ptrs */
  cpu_stack_frame_gen[2] = cpuFrameA;  /* 2  - Bus Error */
  cpu_stack_frame_gen[3] = cpuFrameA;  /* 3  - Addrss Error */
  cpu_stack_frame_gen[5] = cpuFrame2;  /* 5  - Zero Divide */
  cpu_stack_frame_gen[6] = cpuFrame2;  /* 6  - CHK, CHK2 */
  cpu_stack_frame_gen[7] = cpuFrame2;  /* 7  - TRAPV, TRAPcc, cpTRAPcc */
  cpu_stack_frame_gen[9] = cpuFrame2;  /* 9  - Trace */
}

void cpuStackFrameInit030(void) {
  ULO i;

  for (i = 0; i < 64; i++)
    cpu_stack_frame_gen[i] = cpuFrame0;/* Avoid NULL ptrs */
  cpu_stack_frame_gen[2] = cpuFrameA;  /* 2  - Bus Error */
  cpu_stack_frame_gen[3] = cpuFrameA;  /* 3  - Addrss Error */
  cpu_stack_frame_gen[5] = cpuFrame2;  /* 5  - Zero Divide */
  cpu_stack_frame_gen[6] = cpuFrame2;  /* 6  - CHK, CHK2 */
  cpu_stack_frame_gen[7] = cpuFrame2;  /* 7  - TRAPV, TRAPcc, cpTRAPcc */
  cpu_stack_frame_gen[9] = cpuFrame2;  /* 9  - Trace */
}

void cpuStackFrameInit040(void) {
  ULO i;

  for (i = 0; i < 64; i++)
    cpu_stack_frame_gen[i] = cpuFrame0;/* Avoid NULL ptrs */
  cpu_stack_frame_gen[2] = cpuFrameA;  /* 2  - Bus Error */
  cpu_stack_frame_gen[3] = cpuFrameA;  /* 3  - Addrss Error */
  cpu_stack_frame_gen[5] = cpuFrame2;  /* 5  - Zero Divide */
  cpu_stack_frame_gen[6] = cpuFrame2;  /* 6  - CHK, CHK2 */
  cpu_stack_frame_gen[7] = cpuFrame2;  /* 7  - TRAPV, TRAPcc, cpTRAPcc */
  cpu_stack_frame_gen[9] = cpuFrame2;  /* 9  - Trace */
}

void cpuStackFrameInit060(void) {
  ULO i;

  for (i = 0; i < 64; i++)
    cpu_stack_frame_gen[i] = cpuFrame0;/* Avoid NULL ptrs */
  cpu_stack_frame_gen[2] = cpuFrame4;  /* 2  - Access Fault */
  cpu_stack_frame_gen[3] = cpuFrame2;  /* 3  - Addrss Error */
  cpu_stack_frame_gen[5] = cpuFrame2;  /* 5  - Zero Divide */
  cpu_stack_frame_gen[6] = cpuFrame2;  /* 6  - CHK, CHK2 */
  cpu_stack_frame_gen[7] = cpuFrame2;  /* 7  - TRAPV, TRAPcc, cpTRAPcc */
  cpu_stack_frame_gen[9] = cpuFrame2;  /* 9  - Trace */
  /* Unfinished */
}

void cpuStackFrameInit(void) {
  switch (cpu_major) {
    case 0:
      cpuStackFrameInit000();
      break;
    case 1:
      cpuStackFrameInit010();
      break;
    case 2:
      cpuStackFrameInit020();
      break;
    case 3:
      cpuStackFrameInit030();
      break;
    case 4:
      cpuStackFrameInit040();
      break;
    case 6:
      cpuStackFrameInit060();
      break;
  }
}


/*====================================*/
/* Set adressmode data to 68000 stubs */
/*====================================*/

void cpuAdrModeTablesInit000(void) {
  ULO i;

  for (i = 0; i < 12; i++) {
    arb[i] = arb68000[i];
    arw[i] = arw68000[i];
    arl[i] = arl68000[i];
    parb[i] = parb68000[i];
    parw[i] = parw68000[i];
    parl[i] = parl68000[i];
    awb[i] = awb68000[i];
    aww[i] = aww68000[i];
    awl[i] = awl68000[i];
    eac[i] = eac68000[i];
    tarb[i] = tarb68000[i];
    tarw[i] = tarw68000[i];
    tarl[i] = tarl68000[i];
  }
}


/*=============================================================*/
/* Set address-mode tables to 020 and above compilant routines */
/*=============================================================*/

void cpuAdrModeTablesInit020(void) {
  ULO i;

  for (i = 0; i < 12; i++) {    /* Set adressmode data to 680x0 stubs */
    arb[i] = arb680X0[i];
    arw[i] = arw680X0[i];
    arl[i] = arl680X0[i];
    parb[i] = parb680X0[i];
    parw[i] = parw680X0[i];
    parl[i] = parl680X0[i];
    awb[i] = awb680X0[i];
    aww[i] = aww680X0[i];
    awl[i] = awl680X0[i];
    eac[i] = eac680X0[i];
    tarb[i] = tarb68000[i];    /* let even 68030 instrs to have same timing */
    tarw[i] = tarw68000[i];    /* not right, though... */
    tarl[i] = tarl68000[i];
    }
}


/*============================*/
/* Actually a Reset exception */
/*============================*/

void cpuReset000(void) {
  sr &= 0x271f;         /* T = 0 */
  sr |= 0x2700;         /* S = 1, ilvl = 7 */
  vbr = 0;  
  ssp = memoryInitialSP();        /* ssp = fake vector 0 */
  cpuSetPC(memoryInitialPC()); /* pc = fake vector 1 */
#ifdef PREFETCH
  cpuPrefetchFill(cpuGetPC(pc));
#endif
}

void cpuReset010(void) {
  sr &= 0x271f;         /* T1T0 = 0 */
  sr |= 0x2700;         /* S = 1, ilvl = 7 */
  vbr = 0;              /* vbr = 0 */
  ssp = memoryInitialSP();        /* ssp = fake vector 0 */
  cpuSetPC(memoryInitialPC()); /* pc = fake vector 1 */
#ifdef PREFETCH
  cpuPrefetchFill(cpuGetPC(pc));
#endif
}

void cpuReset020(void) {
  sr &= 0x271f;         /* T1T0 = 0, M = 0 */
  sr |= 0x2700;         /* S = 1, ilvl = 7 */
  vbr = 0;              /* vbr = 0 */
  cacr = 0;             /* E = 0, F = 0 */
                        /* Invalidate cache, we don't have one */
  ssp = memoryInitialSP();  /* ssp = fake vector 0 */
  cpuSetPC(memoryInitialPC()); /* pc = fake vector 1 */
#ifdef PREFETCH
  cpuPrefetchFill(cpuGetPC(pc));
#endif
}

void cpuReset030(void) {
  sr &= 0x271f;         /* T1T0 = 0, M = 0 */
  sr |= 0x2700;         /* S = 1, ilvl = 7 */
  vbr = 0;              /* vbr = 0 */
  cacr = 0;             /* E = 0, F = 0 */
                        /* Invalidate cache, we don't have one */
  ssp = memoryInitialSP();        /* ssp = fake vector 0 */
  cpuSetPC(memoryInitialPC()); /* pc = fake vector 1 */
#ifdef PREFETCH
  cpuPrefetchFill(cpuGetPC(pc));
#endif
}

/* 040 and 060, useless right now, will remain so if it is up to me (PS) */

void cpuReset040(void) {
  sr &= 0x271f;         /* T1T0 = 0, M = 0 */
  sr |= 0x2700;         /* S = 1, ilvl = 7 */
  vbr = 0;              /* vbr = 0 */
  cacr = 0;             /* E = 0, F = 0 */
                        /* Invalidate cache, we don't have one */
  ssp = memoryInitialSP();        /* ssp = fake vector 0 */
  cpuSetPC(memoryInitialPC()); /* pc = fake vector 1 */
#ifdef PREFETCH
  cpuPrefetchFill(cpuGetPC(pc));
#endif
}

void cpuReset060(void) {
  sr &= 0x271f;         /* T = 0 */
  sr |= 0x2700;         /* S = 1, ilvl = 7 */
  vbr = 0;              /* vbr = 0 */
  cacr = 0;             /* cacr = 0 */
                        /* DTTn[E-bit] = 0 */
                        /* ITTn[E-bit] = 0 */
                        /* TCR = 0 */
                        /* BUSCR = 0 */
                        /* PCR = 0 */
                        /* FPU Data regs = NAN */
                        /* FPU Control regs = 0 */
  ssp = memoryInitialSP();        /* ssp = fake vector 0 */
  cpuSetPC(memoryInitialPC()); /* pc = fake vector 1 */
#ifdef PREFETCH
  cpuPrefetchFill(cpuGetPC(pc));
#endif
}


/*==================================================*/
/* Add 000 specific instruction set to opcode table */
/*==================================================*/

void cpuInit000(void) {
  i44ini();                           /* MOVEP */
}


/*==================================================*/
/* Add 010 specific instruction set to opcode table */
/*==================================================*/

void cpuInit010(void) {
  i44ini();                           /* MOVEP */
  i323ini();                          /* MOVE.W CCR */
  i324ini();                          /* MOVEC */
  i325ini();                          /* MOVES */
  i329ini();                          /* RTD */
}


/*==================================================*/
/* Add 020 specific instruction set to opcode table */
/*==================================================*/

void cpuInit020(void) {
  i44ini();                           /* MOVEP */
  i301ini();                          /* Bit fields */
  i309ini();                          /* CAS CAS2 */
  i311ini();                          /* CHK.D */
  i312ini();                          /* CHK2/CMP2 */
  i313ini();                          /* cpBcc */
  i314ini();                          /* cpDBcc */
  i315ini();                          /* cpGEN */
  i316ini();                          /* cpRESTORE */
  i317ini();                          /* cpSAVE */
  i318ini();                          /* cpScc */
  i319ini();                          /* cpTRAPcc */
  i320ini();                          /* DIVSL/DIVUL */
  i321ini();                          /* EXTB */
  i322ini();                          /* LINK.L */
  i323ini();                          /* MOVE.W CCR */
  i324ini();                          /* MOVEC */
  i325ini();                          /* MOVES */
  i326ini();                          /* MULSL/MULUL */
  i327ini();                          /* PACK */
  i329ini();                          /* RTD */
  i330ini();                          /* TRAPcc */
  i331ini();                          /* UNPK */
  i332ini();                          /* CALLM */
  i333ini();                          /* RTM */
}


/*==================================================*/
/* Add 030 specific instruction set to opcode table */
/*==================================================*/

void cpuInit030(void) {
  i44ini();                           /* MOVEP */
  i301ini();                          /* Bit fields */
  i309ini();                          /* CAS CAS2 */
  i311ini();                          /* CHK.D */
  i312ini();                          /* CHK2/CMP2 */
  i313ini();                          /* cpBcc */
  i314ini();                          /* cpDBcc */
  i315ini();                          /* cpGEN */
  i316ini();                          /* cpRESTORE */
  i317ini();                          /* cpSAVE */
  i318ini();                          /* cpScc */
  i319ini();                          /* cpTRAPcc */
  i320ini();                          /* DIVSL/DIVUL */
  i321ini();                          /* EXTB */
  i322ini();                          /* LINK.L */
  i323ini();                          /* MOVE.W CCR */
  i324ini();                          /* MOVEC */
  i325ini();                          /* MOVES */
  i326ini();                          /* MULSL/MULUL */
  i327ini();                          /* PACK */
  MMUini030();                        /* PFLUSH/PLOAD/PMOVE/PTEST */
  i329ini();                          /* RTD */
  i330ini();                          /* TRAPcc */
  i331ini();                          /* UNPK */
}


/*==================================================*/
/* Add 040 specific instruction set to opcode table */
/*==================================================*/

void cpuInit040(void) {
  i44ini();                           /* MOVEP */
  i301ini();                          /* Bit fields */
  i309ini();                          /* CAS CAS2 */
  i311ini();                          /* CHK.D */
  i312ini();                          /* CHK2/CMP2 */
  i320ini();                          /* DIVSL/DIVUL */
  i321ini();                          /* EXTB */
  i322ini();                          /* LINK.L */
  i323ini();                          /* MOVE.W CCR */
  i324ini();                          /* MOVEC */
  i325ini();                          /* MOVES */
  i326ini();                          /* MULSL/MULUL */
  i327ini();                          /* PACK */
  MMUIni040();                        /* PFLUSH/PTEST */
  i329ini();                          /* RTD */
  i330ini();                          /* TRAPcc */
  i331ini();                          /* UNPK */
  i334ini();                          /* MOVE16 */
/*  CINVIni();*/                          /* CINV (Full 040 and LC040 only) */
/*  CPUSHIni();*/                         /* CPUSH (Full 040 and LC040 only) */
} 


/*==================================================*/
/* Add 060 specific instruction set to opcode table */
/*==================================================*/

void cpuInit060(void) {
  i301ini();                          /* Bit fields */
  i311ini();                          /* CHK.D */
  i320ini();                          /* DIVSL/DIVUL */
  i321ini();                          /* EXTB */
  i322ini();                          /* LINK.L */
  i323ini();                          /* MOVE.W CCR */
  i324ini();                          /* MOVEC */
  i325ini();                          /* MOVES */
  i326ini();                          /* MULSL/MULUL */
  i327ini();                          /* PACK */
  i329ini();                          /* RTD */
  i330ini();                          /* TRAPcc */
  i331ini();                          /* UNPK */
  i334ini();                          /* MOVE16 */
}


/*=====================================*/
/* cpuReset performs a Reset exception */
/*=====================================*/

void cpuHardReset(void) {
  interruptflag = cpu_stop = FALSE;
  switch (cpu_major) {
    case 0:
      cpuReset000();
      break;
    case 1:
      cpuReset010();
      break;
    case 2:
      cpuReset020();
      break;
    case 3:
      cpuReset030();
      break;
    case 4:
      cpuReset040();
      break;
    case 6:
      cpuReset060();
      break;
  }
}


/*===========================================================================*/
/* CPU properties                                                            */
/*===========================================================================*/

BOOLE cpuSetType(cpu_types type) {
   BOOLE needreset = (cpu_type != type);
  cpu_type = type;
  switch (type) {
    case M68000: cpu_major = 0; cpu_minor = 0; break;
    case M68010: cpu_major = 1; cpu_minor = 0; break;
    case M68020: cpu_major = 2; cpu_minor = 0; break;
    case M68030: cpu_major = 3; cpu_minor = 0; break;
    case M68040: cpu_major = 4; cpu_minor = 0; break;
    case M68060: cpu_major = 6; cpu_minor = 0; break;
    case M68EC20: cpu_major = 2; cpu_minor = 1; break;
    case M68EC30: cpu_major = 3; cpu_minor = 1; break;  
  }
  cpu_opcode_table_is_invalid |= needreset;
  return needreset;
}

cpu_types cpuGetType(void) {
  return cpu_type;
}

void cpuSetSpeed(ULO speed) {
  cpu_speed_property = speed;
}

ULO cpuGetSpeed(void) {
  return cpu_speed_property;
}


/*==================================================================*/
/* cpuInit is called whenever the opcode tables needs to be changed */
/* 1. At emulator startup or                                        */
/* 2. Cpu type has changed                                          */
/*==================================================================*/

void cpuInit(void) {
  ULO i, j;
  
  cpuClearOpcodeTable();
  for (j = 0; j < 2; j++)
    for (i = 0; i < 8; i++)
      da_regs[j][i] = 0;

  if (cpu_major < 2)
    cpuAdrModeTablesInit000();
  else
    cpuAdrModeTablesInit020();
  cpuMakeBCDTables();
  cpuCreateStatusTable();
  cpuIRQTableInit();  

  /* Common opcodes for all CPUs */
  
  i01ini();  /* ABCD */
  i02ini();
  i03ini();
  i04ini();
  i05ini();
  i06ini();
  i07ini();
  i08ini();
  i09ini();
  i11ini();
  i12ini();
  i18ini();
  i19ini();
  i20ini();
  i21ini();
  i22ini();
  i23ini();
  i24ini();
  i25ini();
  i26ini();
  i27ini();
  i28ini();
  i29ini();
  i30ini();
  i31ini();
  i32ini();
  i33ini();
  i34ini();
  i35ini();
  i37ini();
  i38ini();
  i39ini();
  i40ini();
  i41ini();
  i42ini();
  cpuMovemIni();
  i45ini();  /* MOVEQ */
  i46ini();
  i47ini();
  i48ini();
  i49ini();
  i50ini();
  i51ini();
  i52ini();
  i53ini();
  i54ini();
  i55ini();
  i56ini();
  i57ini();
  i59ini();
  i61ini();
  i62ini();
  i63ini();
  i64ini();
  i65ini();
  i66ini();
  i67ini();
  i68ini();
  i69ini();
  i70ini();
  cpuSubXIni();
  cpuSwapWIni();
  cpuTasBIni();
  cpuTrapIni();
  cpuTrapVIni();
  cpuTstIni();
  cpuUnlkIni();
  cpuBkptIni();

  if (cpu_major == 0) cpuInit000();
  else if (cpu_major == 1) cpuInit010();
  else if (cpu_major == 2) cpuInit020();
  else if (cpu_major == 3) cpuInit030();
  else if (cpu_major == 4) cpuInit040();
  else if (cpu_major == 6) cpuInit060();
  cpuMakeMulTable();  
  cpuStackFrameInit();
  cpu_opcode_table_is_invalid = FALSE;
}

void cpuEmulationStart(void) {
  if (cpu_opcode_table_is_invalid) cpuInit();
  if (cpuGetSpeed() >= 8) cpu_speed = 0;
  else if (cpuGetSpeed() >= 4) cpu_speed = 1;
  else cpu_speed = 2;
}

void cpuEmulationStop(void) {
}

void cpuStartup(void) {
  cpuSetType(M68000);
  cpu_opcode_table_is_invalid = TRUE;
  cpu_stop = FALSE;
}

void cpuShutdown(void) {
}
