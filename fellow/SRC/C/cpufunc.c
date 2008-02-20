/* @(#) $Id: cpufunc.c,v 1.3 2008-02-20 23:57:50 peschau Exp $ */
/*=========================================================================*/
/* Fellow                                                                  */
/* CPU 68k functions                                                       */
/*                                                                         */
/* Author: Petter Schau                                                    */
/*                                                                         */
/*                                                                         */
/* Copyright (C) 1991, 1992, 1996 Free Software Foundation, Inc.           */
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
#include "fmem.h"
#include "cpu.h"
#include "bus.h"


#define CPU_EXCEPTION_ILLEGAL 0x10

/* M68k registers */
static ULO cpu_regs[2][8]; /* 0 - data, 1 - address */
static ULO cpu_pc;
ULO cpu_usp;
ULO cpu_ssp;
ULO cpu_msp;
static ULO cpu_sfc;
static ULO cpu_dfc;
static UWO cpu_sr;
static ULO cpu_vbr;
static ULO cpu_ccr0;
static ULO cpu_ccr1;
static UWO cpu_prefetch_word;
static ULO cpu_cacr;
static ULO cpu_caar;

/* Exception stack frame jmptables */
typedef void(*cpuStackFrameFunc)(UWO, ULO);
static cpuStackFrameFunc cpu_stack_frame_gen[64];

/* Irq management */
static ULO cpu_irq_level = 0;
static ULO cpu_irq_address = 0;

jmp_buf cpu_exception_buffer;
ULO cpu_instruction_time;

/* Flag set if CPU is stopped */
static ULO cpu_stop;

/* The current CPU model */
static UBY cpu_model_mask;

/*============================================================================*/
/* profiling help functions                                                   */
/*============================================================================*/

#ifndef X64
static __inline cpuTscBefore(LLO* a)
{
  LLO local_a = *a;
  __asm 
  {
    push    eax
    push    edx
    push    ecx
    mov     ecx,10h
    rdtsc
    pop     ecx
    mov     dword ptr [local_a], eax
    mov     dword ptr [local_a + 4], edx
    pop     edx
    pop     eax
  }
  *a = local_a;
}

static __inline cpuTscAfter(LLO* a, LLO* b, ULO* c)
{
  LLO local_a = *a;
  LLO local_b = *b;
  ULO local_c = *c;

  __asm 
  {
    push    eax
    push    edx
    push    ecx
    mov     ecx, 10h
    rdtsc
    pop     ecx
    sub     eax, dword ptr [local_a]
    sbb     edx, dword ptr [local_a + 4]
    add     dword ptr [local_b], eax
    adc     dword ptr [local_b + 4], edx
    inc     dword ptr [local_c]
    pop     edx
    pop     eax
  }
  *a = local_a;
  *b = local_b;
  *c = local_c;
}
#endif

/* Get and set data and address registers */
void cpuSetDReg(ULO i, ULO value) {cpu_regs[0][i] = value;}
ULO cpuGetDReg(ULO i) {return cpu_regs[0][i];}
void cpuSetAReg(ULO i, ULO value) {cpu_regs[1][i] = value;}
ULO cpuGetAReg(ULO i) {return cpu_regs[1][i];}
void cpuSetReg(ULO da, ULO i, ULO value) {cpu_regs[da][i] = value;}
ULO cpuGetReg(ULO da, ULO i) {return cpu_regs[da][i];}

void cpuSetPC(ULO address) {cpu_pc = address;}
ULO cpuGetPC(void) {return cpu_pc;}
void cpuSetStop(BOOLE stop) {cpu_stop = stop;}
BOOLE cpuGetStop(void) {return cpu_stop;}
void cpuSetVbr(ULO vbr) {cpu_vbr = vbr;}
ULO cpuGetVbr(void) {return cpu_vbr;}
void cpuSetSfc(ULO sfc) {cpu_sfc = sfc;}
ULO cpuGetSfc(void) {return cpu_sfc;}
void cpuSetDfc(ULO dfc) {cpu_dfc = dfc;}
ULO cpuGetDfc(void) {return cpu_dfc;}
void cpuSetCacr(ULO cacr) {cpu_cacr = cacr;}
ULO cpuGetCacr(void) {return cpu_cacr;}
void cpuSetCaar(ULO caar) {cpu_caar = caar;}
ULO cpuGetCaar(void) {return cpu_caar;}
void cpuSetSR(UWO sr) {cpu_sr = sr;}
UWO cpuGetSR(void) {return cpu_sr;}
void cpuSetIrqLevel(ULO irq_level) {cpu_irq_level = irq_level;}
ULO cpuGetIrqLevel(void) {return cpu_irq_level;}
void cpuSetIrqAddress(ULO irq_address) {cpu_irq_address = irq_address;}
ULO cpuGetIrqAddress(void) {return cpu_irq_address;}
void cpuSetModelMask(UBY model_mask) {cpu_model_mask = model_mask;}

/* Private help functions */
static ULO cpuSignExtByteToLong(UBY v) {return (ULO)(LON)(BYT) v;}
static UWO cpuSignExtByteToWord(UBY v) {return (UWO)(WOR)(BYT) v;}
static ULO cpuSignExtWordToLong(UWO v) {return (ULO)(LON)(WOR) v;}

static ULO cpuJoinWordToLong(UWO upper, UWO lower) {return (((ULO)upper) << 16) | ((ULO)lower);}
static ULO cpuJoinByteToLong(UBY upper, UBY midh, UBY midl, UBY lower) {return (((ULO)upper) << 24) | (((ULO)midh) << 16) | (((ULO)midl) << 8) | ((ULO)lower);}
static UWO cpuJoinByteToWord(UBY upper, UBY lower) {return (((UWO)upper) << 8) | ((UWO)lower);}

static void cpuSetDRegWord(ULO regno, UWO val) {*((WOR*)&cpu_regs[0][regno]) = val;}
static void cpuSetDRegByte(ULO regno, UBY val) {*((UBY*)&cpu_regs[0][regno]) = val;}
static UWO cpuGetRegWord(ULO i, ULO regno) {return (UWO)cpu_regs[i][regno];}

static UWO cpuGetDRegWord(ULO regno) {return (UWO)cpu_regs[0][regno];}
static UBY cpuGetDRegByte(ULO regno) {return (UBY)cpu_regs[0][regno];}

static ULO cpuGetDRegWordSignExtLong(ULO regno) {return cpuSignExtWordToLong(cpuGetDRegWord(regno));}
static UWO cpuGetDRegByteSignExtWord(ULO regno) {return cpuSignExtByteToWord(cpuGetDRegByte(regno));}
static ULO cpuGetDRegByteSignExtLong(ULO regno) {return cpuSignExtByteToLong(cpuGetDRegByte(regno));}

static UWO cpuGetARegWord(ULO regno) {return (UWO)cpu_regs[1][regno];}

	/// <summary>
        /// Returns the most significant bit.
        /// </summary>
	static UBY cpuMsbB(UBY v)
	{
	  return v & 0x80;
	}
    
        /// <summary>
        /// Returns the most significant bit.
        /// </summary>
	static UWO cpuMsbW(UWO v)
	{
	  return v & 0x8000;
	}
    
        /// <summary>
        /// Returns the most significant bit.
        /// </summary>
	static ULO cpuMsbL(ULO v)
	{
	  return v & 0x80000000;
	}
    
        /// <summary>
        /// Returns TRUE when v is zero.
        /// </summary>
	static BOOLE cpuIsZeroB(UBY v)
	{
	  return v == 0;
	}
    
        /// <summary>
        /// Returns TRUE when v is zero.
        /// </summary>
	static BOOLE cpuIsZeroW(UWO v)
	{
	  return v == 0;
	}
    
        /// <summary>
        /// Returns TRUE when v is zero.
        /// </summary>
	static BOOLE cpuIsZeroL(ULO v)
	{
	  return v == 0;
	}
    
        /// <summary>
        /// Store the number of cycles spent by the current instruction.
        /// </summary>
        /// <param name="cycles">The number of cycles.</param>        
	static void cpuSetInstructionTime(ULO cycles)
	{
	  cpu_instruction_time = cycles;
	}

        /// <summary>
        /// Calculate C flag of an add operation.
        /// </summary>
        /// <param name="rm">The MSB of the result.</param>        
        /// <param name="dm">The MSB of the destination source.</param>        
        /// <param name="sm">The MSB of the source.</param>        
        static BOOLE cpuMakeFlagCAdd(BOOLE rm, BOOLE dm, BOOLE sm)
	{
          return (sm && dm) || (dm && (!rm)) || (sm && (!rm));
        }

        /// <summary>
        /// Calculate C flag of a sub operation.
        /// </summary>
        /// <param name="rm">The MSB of the result.</param>        
        /// <param name="dm">The MSB of the destination source.</param>        
        /// <param name="sm">The MSB of the source.</param>        
        static BOOLE cpuMakeFlagCSub(BOOLE rm, BOOLE dm, BOOLE sm)
        {
          return (sm && (!dm)) || (rm && (!dm)) || (sm && rm);
        }

        /// <summary>
        /// Calculate the overflow, V flag of an add operation.
        /// </summary>
        /// <param name="rm">The MSB of the result.</param>        
        /// <param name="dm">The MSB of the destination source.</param>        
        /// <param name="sm">The MSB of the source.</param>        
        static BOOLE cpuMakeFlagVAdd(BOOLE rm, BOOLE dm, BOOLE sm)
        {
	  return (sm && dm && (!rm)) || ((!sm) && (!dm) && rm);
        }
		
        /// <summary>
        /// Calculate the overflow, V flag of a sub operation.
        /// </summary>
        /// <param name="rm">The MSB of the result.</param>        
        /// <param name="dm">The MSB of the destination source.</param>        
        /// <param name="sm">The MSB of the source.</param>        
        static BOOLE cpuMakeFlagVSub(BOOLE rm, BOOLE dm, BOOLE sm)
        {
	  return ((!sm) && dm && (!rm)) || (sm && (!dm) && rm);
        }
		
        /// <summary>
        /// Get the supervisor bit from sr.
        /// </summary>
	static BOOLE cpuGetFlagSupervisor()
	{
	  return cpu_sr & 0x2000;
	}

        /// <summary>
        /// Get the master/irq state bit from sr.
        /// </summary>
	static BOOLE cpuGetFlagMaster()
	{
	  return cpu_sr & 0x1000;
	}

	/// <summary>
        /// Returns the master stack pointer.
        /// </summary>
	static ULO cpuGetMsp()
	{
	  if (cpuGetFlagSupervisor() && cpuGetFlagMaster())
	  {
	    return cpuGetAReg(7);
	  }
	  return cpu_msp;
	}
    
	/// <summary>
        /// Sets the master stack pointer.
        /// </summary>
	static void cpuSetMsp(ULO new_msp)
	{
	  if (cpuGetFlagSupervisor() && cpuGetFlagMaster())
	  {
	    cpuSetAReg(7, new_msp);
	  }
	  else
	  {
	    cpu_msp = new_msp;
	  }
	}
    
	/// <summary>
        /// Returns the interrupt stack pointer. ssp is used as isp.
        /// </summary>
	static ULO cpuGetIsp()
	{
	  if (cpuGetFlagSupervisor() && !cpuGetFlagMaster())
	  {
	    return cpuGetAReg(7);
	  }
	  return cpu_ssp;
	}
    
	/// <summary>
        /// Sets the interrupt stack pointer. ssp is used as isp.
        /// </summary>
	static void cpuSetIsp(ULO new_isp)
	{
	  if (cpuGetFlagSupervisor() && !cpuGetFlagMaster())
	  {
	    cpuSetAReg(7, new_isp);
	  }
	  else
	  {
	    cpu_ssp = new_isp;
	  }
	}
    
        /// <summary>
        /// Set the X and C flags to the value f.
        /// </summary>
        /// <param name="f">The new state of the flags.</param>        
	static void cpuSetFlagXC(BOOLE f)
	{
	  cpu_sr = (cpu_sr & 0xffee) | ((f) ? 0x11 : 0);
	}

        /// <summary>
        /// Set the C flag to the value f.
        /// </summary>
        /// <param name="f">The new state of the flag.</param>        
	static void cpuSetFlagC(BOOLE f)
	{
	  cpu_sr = (cpu_sr & 0xfffe) | ((f) ? 1 : 0);
	}

        /// <summary>
        /// Set the V flag.
        /// </summary>
        /// <param name="f">The new state of the flag.</param>        
	static void cpuSetFlagV(BOOLE f)
	{
	  cpu_sr = (cpu_sr & 0xfffd) | ((f) ? 2 : 0);
	}

        /// <summary>
        /// Get the V flag.
        /// </summary>
	static BOOLE cpuGetFlagV()
	{
	  return cpu_sr & 0x2;
	}

        /// <summary>
        /// Set the N flag.
        /// </summary>
        /// <param name="f">The new state of the flag.</param>        
	static void cpuSetFlagN(BOOLE f)
	{
	  cpu_sr = (cpu_sr & 0xfff7) | ((f) ? 8 : 0);
	}

        /// <summary>
        /// Set the Z flag.
        /// </summary>
        /// <param name="f">The new state of the flag.</param>        
	static void cpuSetFlagZ(BOOLE f)
	{
	  cpu_sr = (cpu_sr & 0xfffb) | ((f) ? 4 : 0);
	}

        /// <summary>
        /// Get the Z flag.
        /// </summary>
	static BOOLE cpuGetFlagZ()
	{
	  return cpu_sr & 0x4;
	}

        /// <summary>
        /// Get the X flag.
        /// </summary>
	static BOOLE cpuGetFlagX()
	{
	  return cpu_sr & 0x10;
	}

        /// <summary>
        /// Set the flags.
        /// </summary>
	static void cpuSetFlags0100()
	{
	  cpu_sr = (cpu_sr & 0xfff0) | 4;
	}

        /// <summary>
        /// Set the flags NZVC.
        /// </summary>
        /// <param name="z">The Z flag.</param>        
        /// <param name="n">The N flag.</param>        
        /// <param name="v">The V flag.</param>        
        /// <param name="c">The C flag.</param>        
	static void cpuSetFlagsNZVC(BOOLE z, BOOLE n, BOOLE v, BOOLE c)
	{
	  cpuSetFlagZ(z);
	  cpuSetFlagN(n);
	  cpuSetFlagV(v);
	  cpuSetFlagC(c);
	}

        /// <summary>
        /// Set the flags VC.
        /// </summary>
        /// <param name="v">The V flag.</param>        
        /// <param name="c">The C flag.</param>        
	static void cpuSetFlagsVC(BOOLE v, BOOLE c)
	{
	  cpuSetFlagV(v);
	  cpuSetFlagC(c);
	}

        /// <summary>
        /// Set the flags (all) of an add operation.
        /// </summary>
        /// <param name="z">The Z flag.</param>        
        /// <param name="rm">The MSB of the result.</param>        
        /// <param name="dm">The MSB of the destination source.</param>        
        /// <param name="sm">The MSB of the source.</param>        
	static void cpuSetFlagsAdd(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm)
	{
	  cpuSetFlagZ(z);
	  cpuSetFlagN(rm);
	  cpuSetFlagXC(cpuMakeFlagCAdd(rm, dm, sm));
	  cpuSetFlagV(cpuMakeFlagVAdd(rm, dm, sm));
	}

        /// <summary>
        /// Set the flags (all) of a sub operation.
        /// </summary>
        /// <param name="z">The Z flag.</param>        
        /// <param name="rm">The MSB of the result.</param>        
        /// <param name="dm">The MSB of the destination source.</param>        
        /// <param name="sm">The MSB of the source.</param>        
	static void cpuSetFlagsSub(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm)
	{
	  cpuSetFlagZ(z);
	  cpuSetFlagN(rm);
	  cpuSetFlagXC(cpuMakeFlagCSub(rm, dm, sm));
	  cpuSetFlagV(cpuMakeFlagVSub(rm, dm, sm));
	}

        /// <summary>
        /// Set the flags (all) of an addx operation.
        /// </summary>
        /// <param name="z">The Z flag.</param>        
        /// <param name="rm">The MSB of the result.</param>        
        /// <param name="dm">The MSB of the destination source.</param>        
        /// <param name="sm">The MSB of the source.</param>        
	static void cpuSetFlagsAddX(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm)
	{
	  if (!z) cpuSetFlagZ(0);
	  cpuSetFlagN(rm);
	  cpuSetFlagXC(cpuMakeFlagCAdd(rm, dm, sm));
	  cpuSetFlagV(cpuMakeFlagVAdd(rm, dm, sm));
	}

        /// <summary>
        /// Set the flags (all) of a subx operation.
        /// </summary>
        /// <param name="z">The Z flag.</param>        
        /// <param name="rm">The MSB of the result.</param>        
        /// <param name="dm">The MSB of the destination source.</param>        
        /// <param name="sm">The MSB of the source.</param>        
	static void cpuSetFlagsSubX(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm)
	{
	  if (!z) cpuSetFlagZ(0);
	  cpuSetFlagN(rm);
	  cpuSetFlagXC(cpuMakeFlagCSub(rm, dm, sm));
	  cpuSetFlagV(cpuMakeFlagVSub(rm, dm, sm));
	}

        /// <summary>
        /// Set the flags (all) of a neg operation.
        /// </summary>
        /// <param name="z">The Z flag.</param>        
        /// <param name="rm">The MSB of the result.</param>        
        /// <param name="dm">The MSB of the destination source.</param>        
	static void cpuSetFlagsNeg(BOOLE z, BOOLE rm, BOOLE dm)
	{
	  cpuSetFlagZ(z);
	  cpuSetFlagN(rm);
	  cpuSetFlagXC(dm || rm);
	  cpuSetFlagV(dm && rm);
	}

        /// <summary>
        /// Set the flags (all) of a negx operation.
        /// </summary>
        /// <param name="z">The Z flag.</param>        
        /// <param name="rm">The MSB of the result.</param>        
        /// <param name="dm">The MSB of the destination source.</param>        
	static void cpuSetFlagsNegx(BOOLE z, BOOLE rm, BOOLE dm)
	{
	  if (!z) cpuSetFlagZ(0);
	  cpuSetFlagN(rm);
	  cpuSetFlagXC(dm || rm);
	  cpuSetFlagV(dm && rm);
	}

        /// <summary>
        /// Set the flags (all) of a cmp operation. (Same as sub, but no X.)
        /// </summary>
        /// <param name="z">The Z flag.</param>        
        /// <param name="rm">The MSB of the result.</param>        
        /// <param name="dm">The MSB of the destination source.</param>        
        /// <param name="sm">The MSB of the source.</param>        
	static void cpuSetFlagsCmp(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm)
	{
	  cpuSetFlagZ(z);
	  cpuSetFlagN(rm);
	  cpuSetFlagC(cpuMakeFlagCSub(rm, dm, sm));
	  cpuSetFlagV(cpuMakeFlagVSub(rm, dm, sm));
	}

        /// <summary>
        /// Set the flags of a 0 shift count operation.
        /// </summary>
        /// <param name="z">The Z flag.</param>        
        /// <param name="rm">The MSB of the result.</param>        
	static void cpuSetFlagsShiftZero(BOOLE z, BOOLE rm)
	{
	  cpuSetFlagZ(z);
	  cpuSetFlagN(rm);
	  cpuSetFlagC(0);
	  cpuSetFlagV(0);
	}

        /// <summary>
        /// Set the flags of a shift operation.
        /// </summary>
        /// <param name="z">The Z flag.</param>        
        /// <param name="rm">The MSB of the result.</param>        
        /// <param name="c">The carry of the result.</param>        
        /// <param name="c">The overflow of the result.</param>        
	static void cpuSetFlagsShift(BOOLE z, BOOLE rm, BOOLE c, BOOLE v)
	{
	  cpuSetFlagZ(z);
	  cpuSetFlagN(rm);
	  cpuSetFlagXC(c);
	  cpuSetFlagV(v);
	}

        /// <summary>
        /// Set the flags of a rotate operation.
        /// </summary>
        /// <param name="z">The Z flag.</param>        
        /// <param name="rm">The MSB of the result.</param>        
        /// <param name="c">The carry of the result.</param>        
	static void cpuSetFlagsRotate(BOOLE z, BOOLE rm, BOOLE c)
	{
	  cpuSetFlagZ(z);
	  cpuSetFlagN(rm);
	  cpuSetFlagC(c);
	  cpuSetFlagV(0);
	}

	/// <summary>
        /// Set the flags of a rotate with x operation.
        /// </summary>
        /// <param name="z">The Z flag.</param>        
        /// <param name="rm">The MSB of the result.</param>        
        /// <param name="c">The extend bit of the result.</param>        
	static void cpuSetFlagsRotateX(BOOLE z, BOOLE rm, BOOLE x)
	{
	  cpuSetFlagZ(z);
	  cpuSetFlagN(rm);
	  cpuSetFlagXC(x);
	  cpuSetFlagV(0);
	}

	/// <summary>
        /// Set the flags (ZN00).
        /// </summary>
        /// <param name="z">The Z flag.</param>        
        /// <param name="rm">The MSB of the result.</param>        
	static void cpuSetFlagsNZ00(BOOLE z, BOOLE rm)
	{
	  cpuSetFlagZ(z);
	  cpuSetFlagN(rm);
	  cpuSetFlagC(FALSE);
	  cpuSetFlagV(FALSE);
	}

        /// <summary>
        /// Set the 4 flags absolute.
        /// </summary>
        /// <param name="f">flags</param>        
	static void cpuSetFlagsAbs(UWO f)
	{
	  cpu_sr = (cpu_sr & 0xfff0) | f;
	}

/* Reads the prefetch word from memory. */
void cpuReadPrefetch()
{
  cpu_prefetch_word = memoryReadWord(cpuGetPC());
}

/* Returns the prefetch word. */
UWO cpuGetPrefetch()
{
  return cpu_prefetch_word;
}

/* Returns the next 16-bit data in the instruction stream. */
static UWO cpuGetNextOpcode16(void)
{
  UWO tmp = cpuGetPrefetch();
  cpuSetPC(cpuGetPC() + 2);
  cpuReadPrefetch();
  return tmp;
}

static ULO cpuGetNextOpcode16SignExt(void)
{
  return cpuSignExtWordToLong(cpuGetNextOpcode16());
}

/* Returns the next 32-bit data in the instruction stream. */
static ULO cpuGetNextOpcode32()
{
  ULO tmp = cpuJoinWordToLong(cpuGetPrefetch(), memoryReadWord(cpuGetPC() + 2));
  cpuSetPC(cpuGetPC() + 4);
  cpuReadPrefetch();
  return tmp;
}

/* Calculates EA for (Ax). */
static ULO cpuEA02(ULO regno)
{
  return cpuGetAReg(regno);
}

/* Calculates EA for (Ax)+ */
static ULO cpuEA03(ULO regno, ULO size)
{
  ULO tmp = cpuGetAReg(regno);
  if (regno == 7 && size == 1) size++;
  cpuSetAReg(regno, tmp + size);
  return tmp;
}

/* Calculates EA for -(Ax) */
static ULO cpuEA04(ULO regno, ULO size)
{
  if (regno == 7 && size == 1) size++;
  cpuSetAReg(regno, cpuGetAReg(regno) - size);
  return cpuGetAReg(regno);
}

/* Calculates EA for disp16(Ax) */
static ULO cpuEA05(ULO regno)
{
  return cpuGetAReg(regno) + cpuGetNextOpcode16SignExt();
}

/* Calculates EA for disp8(Ax,Ri.size) with 68020 extended modes. */
static ULO cpuEA06Ext(UWO ext, ULO reg_value, ULO index_value)
{
  ULO base_displacement;
  ULO outer_displacement;
  BOOLE index_register_suppressed = (ext & 0x0040);

  if (index_register_suppressed)
  {
    index_value = 0;
  }
  if (ext & 0x0080)
  {
    reg_value = 0;		  /* Base register suppressed */
  }
  switch ((ext >> 4) & 3)
  {
    case 0:			  /* Reserved */
      cpuPrepareException(CPU_EXCEPTION_ILLEGAL, cpuGetPC() - 2, TRUE);	  /* Illegal instruction */
      break;
    case 1:			  /* Null displacement */
      base_displacement = 0;
      break;
    case 2:			  /* Word displacement */
      base_displacement = cpuGetNextOpcode16SignExt();
      break;
    case 3:			  /* Long displacement */
      base_displacement = cpuGetNextOpcode32();
      break;
  }
  switch (ext & 7)
  {
    case 0: /* No memory indirect action */
      return reg_value + base_displacement + index_value;
    case 1: /* Indirect preindexed with null outer displacement */
      return memoryReadLong(reg_value + base_displacement + index_value);
    case 2: /* Indirect preindexed with word outer displacement */
      outer_displacement = cpuGetNextOpcode16SignExt();
      return memoryReadLong(reg_value + base_displacement + index_value) + outer_displacement;
    case 3: /* Indirect preindexed with long outer displacement */
      outer_displacement = cpuGetNextOpcode32();
      return memoryReadLong(reg_value + base_displacement + index_value) + outer_displacement;
    case 4: /* Reserved */
      cpuPrepareException(CPU_EXCEPTION_ILLEGAL, cpuGetPC() - 2, TRUE);	  /* Illegal instruction */
      break;
    case 5: /* Indirect postindexed with null outer displacement, reserved for index register suppressed */
      if (index_register_suppressed)
      {
	cpuPrepareException(CPU_EXCEPTION_ILLEGAL, cpuGetPC() - 2, TRUE);  /* Illegal instruction */
      }
      return memoryReadLong(reg_value + base_displacement) + index_value;
    case 6: /* Indirect postindexed with word outer displacement, reserved for index register suppressed */
      if (index_register_suppressed)
      {
	cpuPrepareException(CPU_EXCEPTION_ILLEGAL, cpuGetPC() - 2, TRUE);  /* Illegal instruction */
      }
      outer_displacement = cpuGetNextOpcode16SignExt();
      return memoryReadLong(reg_value + base_displacement) + index_value + outer_displacement;
    case 7: /* Indirect postindexed with long outer displacement, reserved for index register suppressed */
      if (index_register_suppressed)
      {
	cpuPrepareException(CPU_EXCEPTION_ILLEGAL, cpuGetPC() - 2, TRUE);  /* Illegal instruction */
      }
      outer_displacement = cpuGetNextOpcode32();
      return memoryReadLong(reg_value + base_displacement) + index_value + outer_displacement;
  }
  return 0; /* Should never come here. */
}

/* Calculates EA for disp8(Ax,Ri.size), calls cpuEA06Ext() for 68020 extended modes. */
static ULO cpuEA06(ULO regno)
{
  ULO reg_value = cpuGetAReg(regno);
  UWO ext = cpuGetNextOpcode16();
  ULO index_value = cpuGetReg(ext >> 15, (ext >> 12) & 0x7);
  if (!(ext & 0x0800))
  {
    index_value = cpuSignExtWordToLong((UWO)index_value);
  }
  if (cpu_major >= 2)
  {
    index_value = index_value << ((ext >> 9) & 0x3);	/* Scaling index value */
    if (ext & 0x0100)					/* Full extension word */
    {
      return cpuEA06Ext(ext, reg_value, index_value);
    }
  }
  return reg_value + index_value + cpuSignExtByteToLong((UBY)ext);
}

/* Calculates EA for xxxx.W */
static ULO cpuEA70()
{
  return cpuGetNextOpcode16SignExt();
}

/* Calculates EA for xxxxxxxx.L */
static ULO cpuEA71()
{
  return cpuGetNextOpcode32();
}

        /// <summary>
        /// Calculates EA for disp16(PC)
        /// </summary>
        /// <returns>Address</returns>
        static ULO cpuEA72()
        {
	  ULO pc_tmp = cpuGetPC();
          return pc_tmp + cpuGetNextOpcode16SignExt();
        }

        /// <summary>
	/// Calculates EA for disp8(PC,Ri.size). Calls cpuEA06Ext() to calculate extended 68020 modes.
        /// </summary>
        /// <returns>Address</returns>
        static ULO cpuEA73()
        {
	  ULO reg_value = cpuGetPC();
          UWO ext = cpuGetNextOpcode16();
	  ULO index_value = cpuGetReg(ext >> 15, (ext >> 12) & 0x7);
          if (!(ext & 0x0800))
	  {
	    index_value = cpuSignExtWordToLong((UWO)index_value);
	  }
	  if (cpu_major >= 2)
	  {
	    index_value = index_value << ((ext >> 9) & 0x3);	// Scaling index value
	    if (ext & 0x0100)					// Full extension word
	    {
	      return cpuEA06Ext(ext, reg_value, index_value);
	    }
	  }
	  return reg_value + index_value + cpuSignExtByteToLong((UBY)ext);
        }

        /// <summary>
        /// Calculates the values for the condition codes.
        /// </summary>
        /// <returns>TRUE or FALSE</returns>
	static BOOLE cpuCalculateConditionCode0(void)
	{
	  return TRUE;
	}

	static BOOLE cpuCalculateConditionCode1(void)
	{
	  return FALSE;
	}

	static BOOLE cpuCalculateConditionCode2(void)
	{
	  return !(cpu_sr & 5);     // HI - !C && !Z
	}

	static BOOLE cpuCalculateConditionCode3(void)
	{
	  return cpu_sr & 5;	      // LS - C || Z
	}

	static BOOLE cpuCalculateConditionCode4(void)
	{
	  return (~cpu_sr) & 1;	      // CC - !C
	}

	static BOOLE cpuCalculateConditionCode5(void)
	{
	  return cpu_sr & 1;	      // CS - C
	}

	static BOOLE cpuCalculateConditionCode6(void)
	{
	  return (~cpu_sr) & 4;	      // NE - !Z
	}

	static BOOLE cpuCalculateConditionCode7(void)
	{
	  return cpu_sr & 4;	      // EQ - Z
	}

	static BOOLE cpuCalculateConditionCode8(void)
	{
	  return (~cpu_sr) & 2;	      // VC - !V
	}

	static BOOLE cpuCalculateConditionCode9(void)
	{
	  return cpu_sr & 2;	      // VS - V
	}

	static BOOLE cpuCalculateConditionCode10(void)
	{
	  return (~cpu_sr) & 8;      // PL - !N
	}

	static BOOLE cpuCalculateConditionCode11(void)
	{
	  return cpu_sr & 8;	      // MI - N
	}

	static BOOLE cpuCalculateConditionCode12(void)
	{
	  UWO tmp = cpu_sr & 0xa;
	  return (tmp == 0xa) || (tmp == 0);  // GE - (N && V) || (!N && !V)
	}

	static BOOLE cpuCalculateConditionCode13(void)
	{
	  UWO tmp = cpu_sr & 0xa;
	  return (tmp == 0x8) || (tmp == 0x2);	// LT - (N && !V) || (!N && V)
	}

	static BOOLE cpuCalculateConditionCode14(void)
	{
	  UWO tmp = cpu_sr & 0xa;
	  return (!(cpu_sr & 0x4)) && ((tmp == 0xa) || (tmp == 0)); // GT - (N && V && !Z) || (!N && !V && !Z) 
	}

	static BOOLE cpuCalculateConditionCode15(void)
	{
	  UWO tmp = cpu_sr & 0xa;
	  return (cpu_sr & 0x4) || (tmp == 0x8) || (tmp == 2);// LE - Z || (N && !V) || (!N && V)
	}

	static _inline BOOLE cpuCalculateConditionCode(ULO cc)
	{
	  switch (cc&0xf)
	  {
	    case 0: return cpuCalculateConditionCode0();
	    case 1: return cpuCalculateConditionCode1();
	    case 2: return cpuCalculateConditionCode2();
	    case 3: return cpuCalculateConditionCode3();
	    case 4: return cpuCalculateConditionCode4();
	    case 5: return cpuCalculateConditionCode5();
	    case 6: return cpuCalculateConditionCode6();
	    case 7: return cpuCalculateConditionCode7();
	    case 8: return cpuCalculateConditionCode8();
	    case 9: return cpuCalculateConditionCode9();
	    case 10: return cpuCalculateConditionCode10();
	    case 11: return cpuCalculateConditionCode11();
	    case 12: return cpuCalculateConditionCode12();
	    case 13: return cpuCalculateConditionCode13();
	    case 14: return cpuCalculateConditionCode14();
	    case 15: return cpuCalculateConditionCode15();
	  }
	}

	static void cpuIllegal(void)
	{
	  UWO opcode = memoryReadWord(cpuGetPC() - 2);
	  if ((opcode & 0xf000) == 0xf000)
	  {
	    cpuPrepareException(0x2c, cpuGetPC() - 2, FALSE);
	  }
	  else if ((opcode & 0xa000) == 0xa000)
	  {
#ifdef UAE_FILESYS
	    if ((cpuGetPC() & 0xff0000) == 0xf00000)
	    {
		call_calltrap(opcode & 0xfff);
		cpuReadPrefetch();
		cpuSetInstructionTime(512);
	    }
	    else
#endif
	    {
	      cpuPrepareException(0x28, cpuGetPC() - 2, FALSE);
	    }
	  }
	  else
	  {
	    cpuPrepareException(0x10, cpuGetPC() - 2, FALSE);
	  }
	}

        /// <summary>
        /// Illegal instruction handler.
        /// </summary>
        static void cpuIllegalInstruction(ULO *opcode_data)
        {
	  cpuIllegal(); 
	}

        /// <summary>
        /// BKPT
        /// </summary>
        static void cpuBkpt(ULO vector)
        {
	  cpuIllegal();
	}

        /// <summary>
        /// Adds bytes src1 to src2. Sets all flags.
        /// </summary>
        /// <returns>The result</returns>
        static UBY cpuAddB(UBY src2, UBY src1)
        {
          UBY res = src2 + src1;
	  cpuSetFlagsAdd(cpuIsZeroB(res), cpuMsbB(res), cpuMsbB(src2), cpuMsbB(src1));
          return res;
        }

        /// <summary>
        /// Adds words src1 to src2. Sets all flags.
        /// </summary>
        /// <returns>The result</returns>
        static UWO cpuAddW(UWO src2, UWO src1)
        {
          UWO res = src2 + src1;
	  cpuSetFlagsAdd(cpuIsZeroW(res), cpuMsbW(res), cpuMsbW(src2), cpuMsbW(src1));
          return res;
        }

        /// <summary>
        /// Adds dwords src1 to src2. Sets all flags.
        /// </summary>
        /// <returns>The result</returns>
        static ULO cpuAddL(ULO src2, ULO src1)
        {
          ULO res = src2 + src1;
	  cpuSetFlagsAdd(cpuIsZeroL(res), cpuMsbL(res), cpuMsbL(src2), cpuMsbL(src1));
          return res;
        }

        /// <summary>
        /// Adds src1 to src2 (For address registers). No flags.
        /// </summary>
        /// <returns>The result</returns>
        static ULO cpuAddaW(ULO src2, ULO src1)
        {
          return src2 + src1;
        }

        /// <summary>
        /// Adds src1 to src2 (For address registers). No flags.
        /// </summary>
        /// <returns>The result</returns>
        static ULO cpuAddaL(ULO src2, ULO src1)
        {
          return src2 + src1;
        }

        /// <summary>
        /// Subtracts src1 from src2. Sets all flags.
        /// </summary>
        /// <returns>The result</returns>
        static UBY cpuSubB(UBY src2, UBY src1)
        {
          UBY res = src2 - src1;
	  cpuSetFlagsSub(cpuIsZeroB(res), cpuMsbB(res), cpuMsbB(src2), cpuMsbB(src1));
          return res;
        }

        /// <summary>
        /// Subtracts src1 from src2. Sets all flags.
        /// </summary>
        /// <returns>The result</returns>
        static UWO cpuSubW(UWO src2, UWO src1)
        {
          UWO res = src2 - src1;
	  cpuSetFlagsSub(cpuIsZeroW(res), cpuMsbW(res), cpuMsbW(src2), cpuMsbW(src1));
          return res;
        }

        /// <summary>
        /// Subtracts src1 from src2. Sets all flags.
        /// </summary>
        /// <returns>The result</returns>
        static ULO cpuSubL(ULO src2, ULO src1)
        {
          ULO res = src2 - src1;
	  cpuSetFlagsSub(cpuIsZeroL(res), cpuMsbL(res), cpuMsbL(src2), cpuMsbL(src1));
          return res;
        }

        /// <summary>
        /// Subtracts src1 from src2 (For address registers). No flags.
        /// </summary>
        /// <returns>The result</returns>
        static ULO cpuSubaW(ULO src2, ULO src1)
        {
          return src2 - src1;
        }

        /// <summary>
        /// Subtracts src1 from src2 (For address registers). No flags.
        /// </summary>
        /// <returns>The result</returns>
        static ULO cpuSubaL(ULO src2, ULO src1)
        {
          return src2 - src1;
        }

        /// <summary>
        /// Subtracts src1 from src2. Sets all flags.
        /// </summary>
        static void cpuCmpB(UBY src2, UBY src1)
        {
          UBY res = src2 - src1;
	  cpuSetFlagsCmp(cpuIsZeroB(res), cpuMsbB(res), cpuMsbB(src2), cpuMsbB(src1));
        }

        /// <summary>
        /// Subtracts src1 from src2. Sets all flags.
        /// </summary>
        static void cpuCmpW(UWO src2, UWO src1)
        {
          UWO res = src2 - src1;
	  cpuSetFlagsCmp(cpuIsZeroW(res), cpuMsbW(res), cpuMsbW(src2), cpuMsbW(src1));
        }

        /// <summary>
        /// Subtracts src1 from src2. Sets all flags.
        /// </summary>
        static void cpuCmpL(ULO src2, ULO src1)
        {
          ULO res = src2 - src1;
	  cpuSetFlagsCmp(cpuIsZeroL(res), cpuMsbL(res), cpuMsbL(src2), cpuMsbL(src1));
        }

	/// <summary>
        /// Ands src1 to src2. Sets NZ00 flags.
        /// </summary>
        /// <returns>The result</returns>
        static UBY cpuAndB(UBY src2, UBY src1)
        {
          UBY res = src2 & src1;
	  cpuSetFlagsNZ00(cpuIsZeroB(res), cpuMsbB(res));
          return res;
        }

	/// <summary>
        /// Ands src1 to src2. Sets NZ00 flags.
        /// </summary>
        /// <returns>The result</returns>
        static UWO cpuAndW(UWO src2, UWO src1)
        {
          UWO res = src2 & src1;
	  cpuSetFlagsNZ00(cpuIsZeroW(res), cpuMsbW(res));
          return res;
        }

	/// <summary>
        /// Ands src1 to src2. Sets NZ00 flags.
        /// </summary>
        /// <returns>The result</returns>
        static ULO cpuAndL(ULO src2, ULO src1)
        {
          ULO res = src2 & src1;
	  cpuSetFlagsNZ00(cpuIsZeroL(res), cpuMsbL(res));
          return res;
        }

	/// <summary>
        /// Eors src1 to src2. Sets NZ00 flags.
        /// </summary>
        /// <returns>The result</returns>
        static UBY cpuEorB(UBY src2, UBY src1)
        {
          UBY res = src2 ^ src1;
	  cpuSetFlagsNZ00(cpuIsZeroB(res), cpuMsbB(res));
          return res;
        }

	/// <summary>
        /// Eors src1 to src2. Sets NZ00 flags.
        /// </summary>
        /// <returns>The result</returns>
        static UWO cpuEorW(UWO src2, UWO src1)
        {
          UWO res = src2 ^ src1;
	  cpuSetFlagsNZ00(cpuIsZeroW(res), cpuMsbW(res));
          return res;
        }

	/// <summary>
        /// Eors src1 to src2. Sets NZ00 flags.
        /// </summary>
        /// <returns>The result</returns>
        static ULO cpuEorL(ULO src2, ULO src1)
        {
          ULO res = src2 ^ src1;
	  cpuSetFlagsNZ00(cpuIsZeroL(res), cpuMsbL(res));
          return res;
        }

	/// <summary>
        /// Ors src1 to src2. Sets NZ00 flags.
        /// </summary>
        /// <returns>The result</returns>
        static UBY cpuOrB(UBY src2, UBY src1)
        {
          UBY res = src2 | src1;
	  cpuSetFlagsNZ00(cpuIsZeroB(res), cpuMsbB(res));
          return res;
        }

	/// <summary>
        /// Ors src1 to src2. Sets NZ00 flags.
        /// </summary>
        /// <returns>The result</returns>
        static UWO cpuOrW(UWO src2, UWO src1)
        {
          UWO res = src2 | src1;
	  cpuSetFlagsNZ00(cpuIsZeroW(res), cpuMsbW(res));
          return res;
        }

	/// <summary>
        /// Ors src1 to src2. Sets NZ00 flags.
        /// </summary>
        /// <returns>The result</returns>
        static ULO cpuOrL(ULO src2, ULO src1)
        {
          ULO res = src2 | src1;
	  cpuSetFlagsNZ00(cpuIsZeroL(res), cpuMsbL(res));
          return res;
        }

	/// <summary>
        /// Changes bit in src. Sets Z flag.
        /// </summary>
        /// <returns>The result</returns>
        static UBY cpuBchgB(UBY src, UBY bit)
        {
	  UBY bit_mask = 1 << (bit & 7);
	  cpuSetFlagZ(!(src & bit_mask));
          return src ^ bit_mask;
        }

	/// <summary>
        /// Changes bit in src. Sets Z flag.
        /// </summary>
        /// <returns>The result</returns>
        static ULO cpuBchgL(ULO src, ULO bit)
        {
	  ULO bit_mask = 1 << (bit & 31);
	  cpuSetFlagZ(!(src & bit_mask));
          return src ^ bit_mask;
        }

	/// <summary>
        /// Clears bit in src. Sets Z flag.
        /// </summary>
        /// <returns>The result</returns>
        static UBY cpuBclrB(UBY src, UBY bit)
        {
	  UBY bit_mask = 1 << (bit & 7);
	  cpuSetFlagZ(!(src & bit_mask));
          return src & ~bit_mask;
        }

	/// <summary>
        /// Clears bit in src. Sets Z flag.
        /// </summary>
        /// <returns>The result</returns>
        static ULO cpuBclrL(ULO src, ULO bit)
        {
	  ULO bit_mask = 1 << (bit & 31);
	  cpuSetFlagZ(!(src & bit_mask));
          return src & ~bit_mask;
        }

	/// <summary>
        /// Sets bit in src. Sets Z flag.
        /// </summary>
        /// <returns>The result</returns>
        static UBY cpuBsetB(UBY src, UBY bit)
        {
	  UBY bit_mask = 1 << (bit & 7);
	  cpuSetFlagZ(!(src & bit_mask));
          return src | bit_mask;
        }

	/// <summary>
        /// Sets bit in src. Sets Z flag.
        /// </summary>
        /// <returns>The result</returns>
        static ULO cpuBsetL(ULO src, ULO bit)
        {
	  ULO bit_mask = 1 << (bit & 31);
	  cpuSetFlagZ(!(src & bit_mask));
          return src | bit_mask;
        }

	/// <summary>
        /// Tests bit in src. Sets Z flag.
        /// </summary>
        static void cpuBtstB(UBY src, UBY bit)
        {
	  UBY bit_mask = 1 << (bit & 7);
	  cpuSetFlagZ(!(src & bit_mask));
        }

	/// <summary>
        /// Tests bit in src. Sets Z flag.
        /// </summary>
        static void cpuBtstL(ULO src, ULO bit)
        {
	  ULO bit_mask = 1 << (bit & 31);
	  cpuSetFlagZ(!(src & bit_mask));
        }

	/// <summary>
        /// Set flags for clr operation.  X0100.
        /// </summary>
        static void cpuClr()
        {
	  cpuSetFlags0100();
        }

	/// <summary>
        /// Neg src1. Sets sub flags.
        /// </summary>
        /// <returns>The result</returns>
        static UBY cpuNegB(UBY src1)
        {
          UBY res = (UBY)-(BYT)src1;
	  cpuSetFlagsNeg(cpuIsZeroB(res), cpuMsbB(res), cpuMsbB(src1));
          return res;
        }

	/// <summary>
        /// Neg src1. Sets sub flags.
        /// </summary>
        /// <returns>The result</returns>
        static UWO cpuNegW(UWO src1)
        {
          UWO res = (UWO)-(WOR)src1;
	  cpuSetFlagsNeg(cpuIsZeroW(res), cpuMsbW(res), cpuMsbW(src1));
          return res;
        }

	/// <summary>
        /// Neg src1. Sets sub flags.
        /// </summary>
        /// <returns>The result</returns>
        static ULO cpuNegL(ULO src1)
        {
          ULO res = (ULO)-(LON)src1;
	  cpuSetFlagsNeg(cpuIsZeroL(res), cpuMsbL(res), cpuMsbL(src1));
          return res;
        }

	/// <summary>
        /// Negx src1.
        /// </summary>
        /// <returns>The result</returns>
        static UBY cpuNegxB(UBY src1)
        {
	  BYT x = (cpuGetFlagX()) ? 1 : 0;
          UBY res = (UBY)-(BYT)src1 - x;
	  cpuSetFlagsNegx(cpuIsZeroB(res), cpuMsbB(res), cpuMsbB(src1));
          return res;
        }

	/// <summary>
        /// Negx src1.
        /// </summary>
        /// <returns>The result</returns>
        static UWO cpuNegxW(UWO src1)
        {
	  WOR x = (cpuGetFlagX()) ? 1 : 0;
          UWO res = (UWO)-(WOR)src1 - x;
	  cpuSetFlagsNegx(cpuIsZeroW(res), cpuMsbW(res), cpuMsbW(src1));
          return res;
        }

	/// <summary>
        /// Negx src1.
        /// </summary>
        /// <returns>The result</returns>
        static ULO cpuNegxL(ULO src1)
        {
	  LON x = (cpuGetFlagX()) ? 1 : 0;
          ULO res = (ULO)-(LON)src1 - x;
	  cpuSetFlagsNegx(cpuIsZeroL(res), cpuMsbL(res), cpuMsbL(src1));
          return res;
        }

	/// <summary>
        /// Not src1. 
        /// </summary>
        /// <returns>The result</returns>
        static UBY cpuNotB(UBY src1)
        {
          UBY res = ~src1;
	  cpuSetFlagsNZ00(cpuIsZeroB(res), cpuMsbB(res));
          return res;
        }

	/// <summary>
        /// Not src1. 
        /// </summary>
        /// <returns>The result</returns>
        static UWO cpuNotW(UWO src1)
        {
          UWO res = ~src1;
	  cpuSetFlagsNZ00(cpuIsZeroW(res), cpuMsbW(res));
          return res;
        }

	/// <summary>
        /// Not src1. 
        /// </summary>
        /// <returns>The result</returns>
        static ULO cpuNotL(ULO src1)
        {
          ULO res = ~src1;
	  cpuSetFlagsNZ00(cpuIsZeroL(res), cpuMsbL(res));
          return res;
        }

	/// <summary>
        /// Tas src1. 
        /// </summary>
        /// <returns>The result</returns>
        static UBY cpuTas(UBY src1)
        {
	  cpuSetFlagsNZ00(cpuIsZeroB(src1), cpuMsbB(src1));
	  return src1 | 0x80;
        }

	/// <summary>
        /// Tst res. 
        /// </summary>
        static void cpuTestB(UBY res)
        {
	  cpuSetFlagsNZ00(cpuIsZeroB(res), cpuMsbB(res));
        }

	/// <summary>
        /// Tst res. 
        /// </summary>
        static void cpuTestW(UWO res)
        {
	  cpuSetFlagsNZ00(cpuIsZeroW(res), cpuMsbW(res));
        }

	/// <summary>
        /// Tst res. 
        /// </summary>
        static void cpuTestL(ULO res)
        {
	  cpuSetFlagsNZ00(cpuIsZeroL(res), cpuMsbL(res));
        }

	/// <summary>
        /// PEA ea. 
        /// </summary>
        static void cpuPeaL(ULO ea)
        {
	  cpuSetAReg(7, cpuGetAReg(7) - 4);
	  memoryWriteLong(ea, cpuGetAReg(7));
        }

	/// <summary>
        /// JMP ea. 
        /// </summary>
        static void cpuJmp(ULO ea)
        {
	  cpuSetPC(ea);
	  cpuReadPrefetch();
        }

	/// <summary>
        /// JSR ea. 
        /// </summary>
        static void cpuJsr(ULO ea)
        {
	  cpuSetAReg(7, cpuGetAReg(7) - 4);
	  memoryWriteLong(cpuGetPC(), cpuGetAReg(7));
	  cpuSetPC(ea);
	  cpuReadPrefetch();
        }

	/// <summary>
        /// Move res
        /// </summary>
        /// <returns>The result</returns>
        static void cpuMoveB(UBY res)
        {
	  cpuSetFlagsNZ00(cpuIsZeroB(res), cpuMsbB(res));
        }

	/// <summary>
        /// Move res
        /// </summary>
        /// <returns>The result</returns>
        static void cpuMoveW(UWO res)
        {
	  cpuSetFlagsNZ00(cpuIsZeroW(res), cpuMsbW(res));
        }

	/// <summary>
        /// Move res 
        /// </summary>
        /// <returns>The result</returns>
        static void cpuMoveL(ULO res)
        {
	  cpuSetFlagsNZ00(cpuIsZeroL(res), cpuMsbL(res));
        }

	/// <summary>
        /// Bra byte offset. 
        /// </summary>
        static void cpuBraB(ULO offset)
        {
	  cpuSetPC(cpuGetPC() + offset);
	  cpuReadPrefetch();
	  cpuSetInstructionTime(10);
        }

	/// <summary>
        /// Bra word offset. 
        /// </summary>
        static void cpuBraW()
        {
	  ULO tmp_pc = cpuGetPC();
	  cpuSetPC(tmp_pc + cpuGetNextOpcode16SignExt());
	  cpuReadPrefetch();
	  cpuSetInstructionTime(10);
        }

	/// <summary>
        /// Bra long offset. 
        /// </summary>
        static void cpuBraL()
        {
	  if (cpu_major < 2) cpuBraB(0xffffffff);
	  else
	  {
	    ULO tmp_pc = cpuGetPC();
	    cpuSetPC(tmp_pc + cpuGetNextOpcode32());
	    cpuReadPrefetch();
	    cpuSetInstructionTime(4);
	  }
        }

	/// <summary>
        /// Bsr byte offset. 
        /// </summary>
        static void cpuBsrB(ULO offset)
        {
	  cpuSetAReg(7, cpuGetAReg(7) - 4);
	  memoryWriteLong(cpuGetPC(), cpuGetAReg(7));
	  cpuSetPC(cpuGetPC() + offset);
	  cpuReadPrefetch();
	  cpuSetInstructionTime(18);
        }

	/// <summary>
        /// Bsr word offset. 
        /// </summary>
        static void cpuBsrW()
        {
	  ULO tmp_pc = cpuGetPC();
	  ULO offset = cpuGetNextOpcode16SignExt();
	  cpuSetAReg(7, cpuGetAReg(7) - 4);
	  memoryWriteLong(cpuGetPC(), cpuGetAReg(7));
	  cpuSetPC(tmp_pc + offset);
	  cpuReadPrefetch();
	  cpuSetInstructionTime(18);
        }

	/// <summary>
        /// Bsr long offset. (020+) 
        /// </summary>
        static void cpuBsrL()
        {
	  if (cpu_major < 2) cpuBsrB(0xffffffff);
	  else
	  {
	    ULO tmp_pc = cpuGetPC();
	    ULO offset = cpuGetNextOpcode32();
	    cpuSetAReg(7, cpuGetAReg(7) - 4);
	    memoryWriteLong(cpuGetPC(), cpuGetAReg(7));
	    cpuSetPC(tmp_pc + offset);
	    cpuReadPrefetch();
	  }
        }

	/// <summary>
        /// Bcc byte offset. 
        /// </summary>
        static void cpuBccB(BOOLE cc, ULO offset)
        {
	  if (cc)
	  {
	    cpuSetPC(cpuGetPC() + offset);
	    cpuReadPrefetch();
	    cpuSetInstructionTime(10);
	  }
	  else
	  {
	    cpuSetInstructionTime(8);
	  }
        }

	/// <summary>
        /// Bcc word offset. 
        /// </summary>
        static void cpuBccW(BOOLE cc)
        {
	  if (cc)
	  {
	    ULO tmp_pc = cpuGetPC();
	    cpuSetPC(tmp_pc + cpuGetNextOpcode16SignExt());
	    cpuSetInstructionTime(10);
	  }
	  else
	  {
	    cpuSetPC(cpuGetPC() + 2);
	    cpuSetInstructionTime(12);
	  }
	  cpuReadPrefetch();
        }

	/// <summary>
        /// Bcc long offset. (020+)
        /// </summary>
        static void cpuBccL(BOOLE cc)
        {
	  if (cpu_major < 2) cpuBccB(cc, 0xffffffff);
	  else
	  {
	    if (cc)
	    {
	      ULO tmp_pc = cpuGetPC();
	      cpuSetPC(tmp_pc + cpuGetNextOpcode32());
	    }
	    else
	    {
	      cpuSetPC(cpuGetPC() + 4);
	    }
	    cpuReadPrefetch();
	    cpuSetInstructionTime(4);
	  }
        }

	/// <summary>
        /// DBcc word offset. 
        /// </summary>
        static void cpuDbcc(BOOLE cc, ULO reg)
        {
	  if (!cc)
	  {
	    WOR val = (WOR)cpuGetDRegWord(reg);
	    val--;
	    cpuSetDRegWord(reg, val);
	    if (val != -1)
	    {
	      ULO tmp_pc = cpuGetPC();
	      cpuSetPC(tmp_pc + cpuGetNextOpcode16SignExt());
	      cpuSetInstructionTime(10);
	    }
	    else
	    {
	      cpuSetPC(cpuGetPC() + 2);
	      cpuSetInstructionTime(14);
	    }
	  }
	  else
	  {
	    cpuSetPC(cpuGetPC() + 2);
	    cpuSetInstructionTime(12);
	  }
	  cpuReadPrefetch();
        }

	/// <summary>
        /// And #imm, ccr 
        /// </summary>
        static void cpuAndCcrB()
        {
	    UWO imm = cpuGetNextOpcode16();
	    cpu_sr = cpu_sr & (0xffe0 | (imm & 0x1f));
	}

	/// <summary>
        /// And #imm, sr 
        /// </summary>
        static void cpuAndSrW()
        {
	  if (cpuGetFlagSupervisor())
	  {
	    UWO imm = cpuGetNextOpcode16();
	    cpuUpdateSr(cpu_sr & imm);
	  }
	  else
	  {
	    cpuPrivilegeViolation();
	  }
	}

	/// <summary>
        /// Or #imm, ccr 
        /// </summary>
        static void cpuOrCcrB()
        {
	    UWO imm = cpuGetNextOpcode16();
	    cpu_sr = cpu_sr | (imm & 0x1f);
	}

	/// <summary>
        /// Or #imm, sr 
        /// </summary>
        static void cpuOrSrW()
        {
	  if (cpuGetFlagSupervisor())
	  {
	    UWO imm = cpuGetNextOpcode16();
	    cpuUpdateSr(cpu_sr | imm);
	  }
	  else
	  {
	    cpuPrivilegeViolation();
	  }
	}

	/// <summary>
        /// Eor #imm, ccr 
        /// </summary>
        static void cpuEorCcrB()
        {
	    UWO imm = cpuGetNextOpcode16();
	    cpu_sr = cpu_sr ^ (imm & 0x1f);
	}

	/// <summary>
        /// Eor #imm, sr 
        /// </summary>
        static void cpuEorSrW()
        {
	  if (cpuGetFlagSupervisor())
	  {
	    UWO imm = cpuGetNextOpcode16();
	    cpuUpdateSr(cpu_sr ^ imm);
	  }
	  else
	  {
	    cpuPrivilegeViolation();
	  }
	}

	/// <summary>
        /// Move ea, ccr 
        /// </summary>
        static void cpuMoveToCcr(UWO src)
        {
	  cpu_sr = (cpu_sr & 0xff00) | (src & 0x1f);
	}

	/// <summary>
        /// Move <ea>, sr 
        /// </summary>
        static void cpuMoveToSr(UWO src)
        {
	  if (cpuGetFlagSupervisor())
	  {
	    cpuUpdateSr(src);
	  }
	  else
	  {
	    cpuPrivilegeViolation();
	  }
	}

	/// <summary>
        /// Move ccr, ea 
        /// </summary>
        static UWO cpuMoveFromCcr()
        {
	  return cpu_sr & 0x1f;
	}

	/// <summary>
        /// Move <ea>, sr 
        /// </summary>
        static UWO cpuMoveFromSr()
        {
	  if (cpu_major == 0 || (cpu_major > 0 && cpuGetFlagSupervisor()))
	  {
	    return cpu_sr;
	  }
	  else
	  {
	    cpuPrivilegeViolation();
	  } 
	  return 0;
	}

	/// <summary>
        /// Scc byte. 
        /// </summary>
        static UBY cpuScc(ULO cc)
        {
	  return (cpuCalculateConditionCode(cc)) ? 0xff : 0;
        }

	/// <summary>
        /// Rts 
        /// </summary>
        static void cpuRts()
        {
	  cpuSetPC(memoryReadLong(cpuGetAReg(7)));
	  cpuSetAReg(7, cpuGetAReg(7) + 4);
	  cpuReadPrefetch();
	  cpuSetInstructionTime(16);
        }

	/// <summary>
        /// Rtr 
        /// </summary>
        static void cpuRtr()
        {
	  cpu_sr = (cpu_sr & 0xffe0) | (memoryReadWord(cpuGetAReg(7)) & 0x1f);
	  cpuSetAReg(7, cpuGetAReg(7) + 2);
	  cpuSetPC(memoryReadLong(cpuGetAReg(7)));
	  cpuSetAReg(7, cpuGetAReg(7) + 4);
	  cpuReadPrefetch();
	  cpuSetInstructionTime(20);
        }

	/// <summary>
        /// Nop 
        /// </summary>
        static void cpuNop()
        {
	  cpuSetInstructionTime(4);
        }

	/// <summary>
        /// Trapv
        /// </summary>
        static void cpuTrapv()
        {
	  if (cpuGetFlagV())
	  {
	    cpuPrepareException(0x1c, cpuGetPC() - 2, FALSE);
	  }
	  cpuSetInstructionTime(4);
        }

	/// <summary>
        /// Muls/u.l
        /// </summary>
        static void cpuMulL(ULO src1, UWO extension)
        {
	  ULO dl = (extension >> 12) & 7;
	  if (extension & 0x0800) // muls.l
	  {
	    LLO result = ((LLO)(LON) src1) * ((LLO)(LON)cpuGetDReg(dl));
	    if (extension & 0x0400) // 32bx32b=64b
	    {  
	      ULO dh = extension & 7;
	      cpuSetDReg(dh, (ULO)(result >> 32));
	      cpuSetDReg(dl, (ULO)result);
	      cpuSetFlagsNZ00(result == 0, result < 0);
	    }
	    else // 32bx32b=32b
	    {
	      BOOLE o;
	      if (result >= 0)
		o = (result & 0xffffffff00000000) != 0;
	      else
		o = (result & 0xffffffff00000000) != 0xffffffff00000000;
	      cpuSetDReg(dl, (ULO)result);
	      cpuSetFlagsNZVC(result == 0, result < 0, o, FALSE);
	    }
	  }
	  else // mulu.l
	  {
	    ULL result = ((ULL) src1) * ((ULL) cpuGetDReg(dl));
	    if (extension & 0x0400) // 32bx32b=64b
	    {  
	      ULO dh = extension & 7;
	      cpuSetDReg(dh, (ULO)(result >> 32));
	      cpuSetDReg(dl, (ULO)result);
	      cpuSetFlagsNZ00(result == 0, !!(result & 0x8000000000000000));
	    }
	    else // 32bx32b=32b
	    {
	      cpuSetDReg(dl, (ULO)result);
	      cpuSetFlagsNZVC(result == 0, !!(result & 0x8000000000000000), (result >> 32) != 0, FALSE);
	    }
	  }
	  cpuSetInstructionTime(4);
        }

	/// <summary>
        /// Muls.w
        /// </summary>
        static ULO cpuMulsW(UWO src2, UWO src1)
        {
	  ULO res = (ULO)(((LON)(WOR)src2)*((LON)(WOR)src1));
	  cpuSetFlagsNZ00(cpuIsZeroL(res), cpuMsbL(res));
	  return res;
        }

	/// <summary>
        /// Mulu.w
        /// </summary>
        static ULO cpuMuluW(UWO src2, UWO src1)
        {
	  ULO res = ((ULO)src2)*((ULO)src1);
	  cpuSetFlagsNZ00(cpuIsZeroL(res), cpuMsbL(res));
	  return res;
        }

	/// <summary>
        /// Divsw, src1 / src2
        /// </summary>
        static ULO cpuDivsW(ULO dst, UWO src1)
        {
	  ULO result;
	  if (src1 == 0)
	  {
	    // Alcatraz odyssey assumes that PC in this exception points after the instruction.
	    cpuPrepareException(0x14, cpuGetPC(), TRUE);
	    result = dst;
	  }
	  else
	  {
	    LON x = (LON) dst;
	    LON y = (LON)(WOR) src1;
	    LON res = x / y;
	    LON rem = x % y;
	    if (res > 32767 || res < -32768)
	    {
	      result = dst;
	      cpuSetFlagsVC(TRUE, FALSE);
	    }
	    else
	    {
	      result = (rem << 16) | (res & 0xffff);
	      cpuSetFlagsNZVC(cpuIsZeroW((UWO) res), cpuMsbW((UWO) res), FALSE, FALSE);
	    }
	  }
	  return result;
        }

	/// <summary>
        /// Divuw, src1 / src2
        /// </summary>
        static ULO cpuDivuW(ULO dst, UWO src1)
        {
	  ULO result;
	  if (src1 == 0)
	  {
	    // Alcatraz odyssey assumes that PC in this exception points after the instruction.
	    cpuPrepareException(0x14, cpuGetPC(), TRUE);
	    result = dst;
	  }
	  else
	  {
	    ULO x = dst;
	    ULO y = (ULO) src1;
	    ULO res = x / y;
	    ULO rem = x % y;
	    if (res > 65535)
	    {
	      result = dst;
	      cpuSetFlagsVC(TRUE, FALSE);
	    }
	    else
	    {
	      result = (rem << 16) | (res & 0xffff);
	      cpuSetFlagsNZVC(cpuIsZeroW((UWO) res), cpuMsbW((UWO) res), FALSE, FALSE);
	    }
	  }
	  return result;
        }

	static void cpuDivL(ULO divisor, ULO ext)
	{
	  if (divisor != 0)
	  {
	    ULO dq_reg = (ext>>12) & 7; /* Get operand registers, size and sign */
	    ULO dr_reg = ext & 7;
	    BOOLE size64 = (ext>>10) & 1;
	    BOOLE sign = (ext>>11) & 1;
	    BOOLE resultsigned = FALSE, restsigned = FALSE;
	    ULL result, rest;
	    ULL x, y;
	    LLO x_signed, y_signed; 
	    
	    if (sign)
	    { 
	      if (size64) x_signed = ((LLO) (LON) cpuGetDReg(dq_reg)) | (((LLO) cpuGetDReg(dr_reg))<<32);
	      else x_signed = (LLO) (LON) cpuGetDReg(dq_reg);
	      y_signed = (LLO) (LON) divisor;

	      if (y_signed < 0)
	      {
		y = (ULL) -y_signed;
		resultsigned = !resultsigned;
	      }
	      else y = y_signed;
	      if (x_signed < 0)
	      {
		x = (ULL) -x_signed;
		resultsigned = !resultsigned;
		restsigned = TRUE;
	      }
	      else x = (ULL) x_signed;
	    }
	    else
	    {
	      if (size64) x = ((ULL) cpuGetDReg(dq_reg)) | (((ULL) cpuGetDReg(dr_reg))<<32);
	      else x = (ULL) cpuGetDReg(dq_reg);
	      y = (ULL) divisor;
	    }

	    result = x / y;
	    rest = x % y;

	    if (sign)
	    {
	      if ((resultsigned && result > 0x80000000) || (!resultsigned && result > 0x7fffffff))
	      {
		/* Overflow */
		cpuSetFlagsVC(TRUE, FALSE);
	      }
	      else
	      {
		LLO result_signed = (resultsigned) ? (-(LLO)result) : ((LLO)result);
		LLO rest_signed = (restsigned) ? (-(LLO)rest) : ((LLO)rest);
		cpuSetDReg(dr_reg, (ULO) rest_signed);
		cpuSetDReg(dq_reg, (ULO) result_signed);
		cpuSetFlagsNZ00(cpuIsZeroL(cpuGetDReg(dq_reg)), cpuMsbL(cpuGetDReg(dq_reg)));
	      }
	    }
	    else
	    {
	      if (result > 0xffffffff)
	      {
		/* Overflow */
		cpuSetFlagsVC(TRUE, FALSE);
	      }
	      else
	      {
		cpuSetDReg(dr_reg, (ULO) rest);
		cpuSetDReg(dq_reg, (ULO) result);
		cpuSetFlagsNZ00(cpuIsZeroL(cpuGetDReg(dq_reg)), cpuMsbL(cpuGetDReg(dq_reg)));
	      }
	    }
	  }
	  else
	  {
	    cpuPrepareException(0x14, cpuGetPC(), FALSE);
	  }
	}

	/// <summary>
        /// Lslb
        /// </summary>
	static UBY cpuLslB(UBY dst, ULO sh, ULO cycles)
	{
	  UBY res;
	  sh &= 0x3f;
	  if (sh == 0)
	  {
	    cpuSetFlagsShiftZero(cpuIsZeroB(dst), cpuMsbB(dst));
	    res = dst;
	  }
	  else if (sh >= 8)
	  {
	    res = 0;
	    cpuSetFlagsShift(TRUE, FALSE, (sh == 8) ? (dst & 1) : FALSE, FALSE);
	  }
	  else
	  {
	    res = dst << sh;
	    cpuSetFlagsShift(cpuIsZeroB(res), cpuMsbB(res), dst & (0x80>>(sh-1)), FALSE);
	  }
	  cpuSetInstructionTime(cycles + sh*2);
	  return res;
	}

	/// <summary>
        /// Lslw
        /// </summary>
	static UWO cpuLslW(UWO dst, ULO sh, ULO cycles)
	{
	  UWO res;
	  sh &= 0x3f;
	  if (sh == 0)
	  {
	    cpuSetFlagsShiftZero(cpuIsZeroW(dst), cpuMsbW(dst));
	    res = dst;
	  }
	  else if (sh >= 16)
	  {
	    res = 0;
	    cpuSetFlagsShift(TRUE, FALSE, (sh == 16) ? (dst & 1) : FALSE, FALSE);
	  }
	  else
	  {
	    res = dst << sh;
	    cpuSetFlagsShift(cpuIsZeroW(res), cpuMsbW(res), dst & (0x8000>>(sh-1)), FALSE);
	  }
	  cpuSetInstructionTime(cycles + sh*2);
	  return res;
	}

	/// <summary>
        /// Lsll
        /// </summary>
	static ULO cpuLslL(ULO dst, ULO sh, ULO cycles)
	{
	  ULO res;
	  sh &= 0x3f;
	  if (sh == 0)
	  {
	    cpuSetFlagsShiftZero(cpuIsZeroL(dst), cpuMsbL(dst));
	    res = dst;
	  }
	  else if (sh >= 32)
	  {
	    res = 0;
	    cpuSetFlagsShift(TRUE, FALSE, (sh == 32) ? (dst & 1) : FALSE, FALSE);
	  }
	  else
	  {
	    res = dst << sh;
	    cpuSetFlagsShift(cpuIsZeroL(res), cpuMsbL(res), dst & (0x80000000>>(sh-1)), FALSE);
	  }
	  cpuSetInstructionTime(cycles + sh*2);
	  return res;
	}

	/// <summary>
        /// Lsrb
        /// </summary>
	static UBY cpuLsrB(UBY dst, ULO sh, ULO cycles)
	{
	  UBY res;
	  sh &= 0x3f;
	  if (sh == 0)
	  {
	    cpuSetFlagsShiftZero(cpuIsZeroB(dst), cpuMsbB(dst));
	    res = dst;
	  }
	  else if (sh >= 8)
	  {
	    res = 0;
	    cpuSetFlagsShift(TRUE, FALSE, (sh == 8) ? cpuMsbB(dst) : FALSE, FALSE);
	  }
	  else
	  {
	    res = dst >> sh;
	    cpuSetFlagsShift(cpuIsZeroB(res), FALSE, dst & (1<<(sh-1)), FALSE);
	  }
	  cpuSetInstructionTime(cycles + sh*2);
	  return res;
	}

	/// <summary>
        /// Lsrw
        /// </summary>
	static UWO cpuLsrW(UWO dst, ULO sh, ULO cycles)
	{
	  UWO res;
	  sh &= 0x3f;
	  if (sh == 0)
	  {
	    cpuSetFlagsShiftZero(cpuIsZeroW(dst), cpuMsbW(dst));
	    res = dst;
	  }
	  else if (sh >= 16)
	  {
	    res = 0;
	    cpuSetFlagsShift(TRUE, FALSE, (sh == 16) ? cpuMsbW(dst) : FALSE, FALSE);
	  }
	  else
	  {
	    res = dst >> sh;
	    cpuSetFlagsShift(cpuIsZeroW(res), FALSE, dst & (1<<(sh-1)), FALSE);
	  }
	  cpuSetInstructionTime(cycles + sh*2);
	  return res;
	}

	/// <summary>
        /// Lsrl
        /// </summary>
	static ULO cpuLsrL(ULO dst, ULO sh, ULO cycles)
	{
	  ULO res;
	  sh &= 0x3f;
	  if (sh == 0)
	  {
	    cpuSetFlagsShiftZero(cpuIsZeroL(dst), cpuMsbL(dst));
	    res = dst;
	  }
	  else if (sh >= 32)
	  {
	    res = 0;
	    cpuSetFlagsShift(TRUE, FALSE, (sh == 32) ? cpuMsbL(dst) : FALSE, FALSE);
	  }
	  else
	  {
	    res = dst >> sh;
	    cpuSetFlagsShift(cpuIsZeroL(res), FALSE, dst & (1<<(sh-1)), FALSE);
	  }
	  cpuSetInstructionTime(cycles + sh*2);
	  return res;
	}

	/// <summary>
        /// Aslb
        /// </summary>
	static UBY cpuAslB(UBY dst, ULO sh, ULO cycles)
	{
	  BYT res;
	  sh &= 0x3f;
	  if (sh == 0)
	  {
	    cpuSetFlagsShiftZero(cpuIsZeroB(dst), cpuMsbB(dst));
	    res = (BYT) dst;
	  }
	  else if (sh >= 8)
	  {
	    res = 0;
	    cpuSetFlagsShift(TRUE, FALSE, (sh == 8) ? (dst & 1) : FALSE, dst != 0);
	  }
	  else
	  {
	    UBY mask = 0xff << (7-sh);
	    UBY out = dst & mask;
	    BOOLE n;
	    res = ((BYT)dst) << sh;
	    n = cpuMsbB(res);
	    cpuSetFlagsShift(cpuIsZeroB(res), n, dst & (0x80>>(sh-1)), (cpuMsbB(dst)) ? (out != mask) : (out != 0));
	  }
	  cpuSetInstructionTime(cycles + sh*2);
	  return (UBY) res;
	}

	/// <summary>
        /// Aslw
        /// </summary>
	static UWO cpuAslW(UWO dst, ULO sh, ULO cycles)
	{
	  WOR res;
	  sh &= 0x3f;
	  if (sh == 0)
	  {
	    cpuSetFlagsShiftZero(cpuIsZeroW(dst), cpuMsbW(dst));
	    res = (WOR) dst;
	  }
	  else if (sh >= 16)
	  {
	    res = 0;
	    cpuSetFlagsShift(TRUE, FALSE, (sh == 16) ? (dst & 1) : FALSE, dst != 0);
	  }
	  else
	  {
	    UWO mask = 0xffff << (15-sh);
	    UWO out = dst & mask;
	    BOOLE n;
	    res = ((WOR)dst) << sh;
	    n = cpuMsbW(res);
	    cpuSetFlagsShift(cpuIsZeroW(res), n, dst & (0x8000>>(sh-1)), (cpuMsbW(dst)) ? (out != mask) : (out != 0));
	  }
	  cpuSetInstructionTime(cycles + sh*2);
	  return (UWO) res;
	}

	/// <summary>
        /// Asll
        /// </summary>
	static ULO cpuAslL(ULO dst, ULO sh, ULO cycles)
	{
	  LON res;
	  sh &= 0x3f;
	  if (sh == 0)
	  {
	    cpuSetFlagsShiftZero(cpuIsZeroL(dst), cpuMsbL(dst));
	    res = (LON) dst;
	  }
	  else if (sh >= 32)
	  {
	    res = 0;
	    cpuSetFlagsShift(TRUE, FALSE, (sh == 32) ? (dst & 1) : FALSE, dst != 0);
	  }
	  else
	  {
	    ULO mask = 0xffffffff << (31-sh);
	    ULO out = dst & mask;
	    BOOLE n;
	    res = ((LON)dst) << sh;
	    n = cpuMsbL(res);
	    cpuSetFlagsShift(cpuIsZeroL(res), n, dst & (0x80000000>>(sh-1)), (cpuMsbL(dst)) ? (out != mask) : (out != 0));
	  }
	  cpuSetInstructionTime(cycles + sh*2);
	  return (ULO) res;
	}

	/// <summary>
        /// Asrb
        /// </summary>
	static UBY cpuAsrB(UBY dst, ULO sh, ULO cycles)
	{
	  BYT res;
	  sh &= 0x3f;
	  if (sh == 0)
	  {
	    cpuSetFlagsShiftZero(cpuIsZeroB(dst), cpuMsbB(dst));
	    res = (BYT) dst;
	  }
	  else if (sh >= 8)
	  {
	    res = (cpuMsbB(dst)) ? 0xff : 0;
	    cpuSetFlagsShift(cpuIsZeroB(res), cpuMsbB(res), cpuMsbB(res), FALSE);
	  }
	  else
	  {
	    res = ((BYT)dst) >> sh;
	    cpuSetFlagsShift(cpuIsZeroB(res), cpuMsbB(res), dst & (1<<(sh-1)), FALSE);
	  }
	  cpuSetInstructionTime(cycles + sh*2);
	  return (UBY) res;
	}

	/// <summary>
        /// Asrw
        /// </summary>
	static UWO cpuAsrW(UWO dst, ULO sh, ULO cycles)
	{
	  WOR res;
	  sh &= 0x3f;
	  if (sh == 0)
	  {
	    cpuSetFlagsShiftZero(cpuIsZeroW(dst), cpuMsbW(dst));
	    res = (WOR) dst;
	  }
	  else if (sh >= 16)
	  {
	    res = (cpuMsbW(dst)) ? 0xffff : 0;
	    cpuSetFlagsShift(cpuIsZeroW(res), cpuMsbW(res), cpuMsbW(res), FALSE);
	  }
	  else
	  {
	    res = ((WOR)dst) >> sh;
	    cpuSetFlagsShift(cpuIsZeroW(res), cpuMsbW(res), dst & (1<<(sh-1)), FALSE);
	  }
	  cpuSetInstructionTime(cycles + sh*2);
	  return (UWO) res;
	}

	/// <summary>
        /// Asrl
        /// </summary>
	static ULO cpuAsrL(ULO dst, ULO sh, ULO cycles)
	{
	  LON res;
	  sh &= 0x3f;

	  if (sh == 0)
	  {
	    cpuSetFlagsShiftZero(cpuIsZeroL(dst), cpuMsbL(dst));
	    res = (LON) dst;
	  }
	  else if (sh >= 32)
	  {
	    res = (cpuMsbL(dst)) ? 0xffffffff : 0;
	    cpuSetFlagsShift(cpuIsZeroL(res), cpuMsbL(res), cpuMsbL(res), FALSE);
	  }
	  else
	  {
	    res = ((LON)dst) >> sh;
	    cpuSetFlagsShift(cpuIsZeroL(res), cpuMsbL(res), dst & (1<<(sh-1)), FALSE);
	  }
	  cpuSetInstructionTime(cycles + sh*2);
	  return (ULO) res;
	}

	/// <summary>
        /// Rolb
        /// </summary>
	static UBY cpuRolB(UBY dst, ULO sh, ULO cycles)
	{
	  UBY res;
	  sh &= 0x3f;
	  cycles += sh*2;
	  if (sh == 0)
	  {
	    cpuSetFlagsShiftZero(cpuIsZeroB(dst), cpuMsbB(dst));
	    res = dst;
	  }
	  else
	  {
	    sh &= 7;
	    res = (dst << sh) | (dst >> (8-sh));
	    cpuSetFlagsRotate(cpuIsZeroB(res), cpuMsbB(res), res & 1);
	  }
	  cpuSetInstructionTime(cycles);
	  return res;
	}

	/// <summary>
        /// Rolw
        /// </summary>
	static UWO cpuRolW(UWO dst, ULO sh, ULO cycles)
	{
	  UWO res;
	  sh &= 0x3f;
	  cycles += sh*2;
	  if (sh == 0)
	  {
	    cpuSetFlagsShiftZero(cpuIsZeroW(dst), cpuMsbW(dst));
	    res = dst;
	  }
	  else
	  {
	    sh &= 15;
	    res = (dst << sh) | (dst >> (16-sh));
	    cpuSetFlagsRotate(cpuIsZeroW(res), cpuMsbW(res), res & 1);
	  }
	  cpuSetInstructionTime(cycles);
	  return res;
	}

	/// <summary>
        /// Roll
        /// </summary>
	static ULO cpuRolL(ULO dst, ULO sh, ULO cycles)
	{
	  ULO res;
	  sh &= 0x3f;
	  cycles += sh*2;
	  if (sh == 0)
	  {
	    cpuSetFlagsShiftZero(cpuIsZeroL(dst), cpuMsbL(dst));
	    res = dst;
	  }
	  else
	  {
	    sh &= 31;
	    res = (dst << sh) | (dst >> (32-sh));
	    cpuSetFlagsRotate(cpuIsZeroL(res), cpuMsbL(res), res & 1);
	  }
	  cpuSetInstructionTime(cycles);
	  return res;
	}

	/// <summary>
        /// Rorb
        /// </summary>
	static UBY cpuRorB(UBY dst, ULO sh, ULO cycles)
	{
	  UBY res;
	  sh &= 0x3f;
	  cycles += sh*2;
	  if (sh == 0)
	  {
	    cpuSetFlagsShiftZero(cpuIsZeroB(dst), cpuMsbB(dst));
	    res = dst;
	  }
	  else
	  {
	    sh &= 7;
	    res = (dst >> sh) | (dst << (8-sh));
	    cpuSetFlagsRotate(cpuIsZeroB(res), cpuMsbB(res), cpuMsbB(res));
	  }
	  cpuSetInstructionTime(cycles);
	  return res;
	}

	/// <summary>
        /// Rorw
        /// </summary>
	static UWO cpuRorW(UWO dst, ULO sh, ULO cycles)
	{
	  UWO res;
	  sh &= 0x3f;
	  cycles += sh*2;
	  if (sh == 0)
	  {
	    cpuSetFlagsShiftZero(cpuIsZeroW(dst), cpuMsbW(dst));
	    res = dst;
	  }
	  else
	  {
	    sh &= 15;
	    res = (dst >> sh) | (dst << (16-sh));
	    cpuSetFlagsRotate(cpuIsZeroW(res), cpuMsbW(res), cpuMsbW(res));
	  }
	  cpuSetInstructionTime(cycles);
	  return res;
	}

	/// <summary>
        /// Rorl
        /// </summary>
	static ULO cpuRorL(ULO dst, ULO sh, ULO cycles)
	{
	  ULO res;
	  sh &= 0x3f;
	  cycles += sh*2;
	  if (sh == 0)
	  {
	    cpuSetFlagsShiftZero(cpuIsZeroL(dst), cpuMsbL(dst));
	    res = dst;
	  }
	  else
	  {
	    sh &= 31;
	    res = (dst >> sh) | (dst << (32-sh));
	    cpuSetFlagsRotate(cpuIsZeroL(res), cpuMsbL(res), cpuMsbL(res));
	  }
	  cpuSetInstructionTime(cycles);
	  return res;
	}

	/// <summary>
        /// Roxlb
        /// </summary>
	static UBY cpuRoxlB(UBY dst, ULO sh, ULO cycles)
	{
	  BOOLE x = cpuGetFlagX();
	  BOOLE x_temp;
	  UBY res = dst;
	  ULO i;
	  sh &= 0x3f;
	  for (i = 0; i < sh; ++i)
	  {
	    x_temp = cpuMsbB(res);
	    res = (res << 1) | ((x) ? 1:0);
	    x = x_temp;
	  }
	  cpuSetFlagsRotateX(cpuIsZeroB(res), cpuMsbB(res), x);
	  cpuSetInstructionTime(cycles + sh*2);
	  return res;
	}

	/// <summary>
        /// Roxlw
        /// </summary>
	static UWO cpuRoxlW(UWO dst, ULO sh, ULO cycles)
	{
	  BOOLE x = cpuGetFlagX();
	  BOOLE x_temp;
	  UWO res = dst;
	  ULO i;
	  sh &= 0x3f;
	  for (i = 0; i < sh; ++i)
	  {
	    x_temp = cpuMsbW(res);
	    res = (res << 1) | ((x) ? 1:0);
	    x = x_temp;
	  }
	  cpuSetFlagsRotateX(cpuIsZeroW(res), cpuMsbW(res), x);
	  cpuSetInstructionTime(cycles + sh*2);
	  return res;
	}

	/// <summary>
        /// Roxll
        /// </summary>
	static ULO cpuRoxlL(ULO dst, ULO sh, ULO cycles)
	{
	  BOOLE x = cpuGetFlagX();
	  BOOLE x_temp;
	  ULO res = dst;
	  ULO i;
	  sh &= 0x3f;
	  for (i = 0; i < sh; ++i)
	  {
	    x_temp = cpuMsbL(res);
	    res = (res << 1) | ((x) ? 1:0);
	    x = x_temp;
	  }
	  cpuSetFlagsRotateX(cpuIsZeroL(res), cpuMsbL(res), x);
	  cpuSetInstructionTime(cycles + sh*2);
	  return res;
	}

	/// <summary>
        /// Roxrb
        /// </summary>
	static UBY cpuRoxrB(UBY dst, ULO sh, ULO cycles)
	{
	  BOOLE x = cpuGetFlagX();
	  BOOLE x_temp;
	  UBY res = dst;
	  ULO i;
	  sh &= 0x3f;
	  for (i = 0; i < sh; ++i)
	  {
	    x_temp = res & 1;
	    res = (res >> 1) | ((x) ? 0x80:0);
	    x = x_temp;
	  }
	  cpuSetFlagsRotateX(cpuIsZeroB(res), cpuMsbB(res), x);
	  cpuSetInstructionTime(cycles + sh*2);
	  return res;
	}

	/// <summary>
        /// Roxrw
        /// </summary>
	static UWO cpuRoxrW(UWO dst, ULO sh, ULO cycles)
	{
	  BOOLE x = cpuGetFlagX();
	  BOOLE x_temp;
	  UWO res = dst;
	  ULO i;
	  sh &= 0x3f;
	  for (i = 0; i < sh; ++i)
	  {
	    x_temp = res & 1;
	    res = (res >> 1) | ((x) ? 0x8000:0);
	    x = x_temp;
	  }
	  cpuSetFlagsRotateX(cpuIsZeroW(res), cpuMsbW(res), x);
	  cpuSetInstructionTime(cycles + sh*2);
	  return res;
	}

	/// <summary>
        /// Roxrl
        /// </summary>
	static ULO cpuRoxrL(ULO dst, ULO sh, ULO cycles)
	{
	  BOOLE x = cpuGetFlagX();
	  BOOLE x_temp;
	  ULO res = dst;
	  ULO i;
	  sh &= 0x3f;
	  for (i = 0; i < sh; ++i)
	  {
	    x_temp = res & 1;
	    res = (res >> 1) | ((x) ? 0x80000000:0);
	    x = x_temp;
	  }
	  cpuSetFlagsRotateX(cpuIsZeroL(res), cpuMsbL(res), x);
	  cpuSetInstructionTime(cycles + sh*2);
	  return res;
	}

	/// <summary>
        /// Stop
        /// </summary>
	static void cpuStop(UWO flags)
	{
	  if (cpuGetFlagSupervisor())
	  {
	    cpu_stop = TRUE;
	    cpuUpdateSr(flags);
	    cpuSetInstructionTime(16);
	  }
	  else
	  {
	    cpuPrivilegeViolation();
	  }
	}

	/// <summary>
        /// Reset
        /// </summary>
	static void cpuReset()
	{
	  fellowSoftReset();
	  cpuSetInstructionTime(132);
	}

	/// <summary>
        /// Rtd
        /// </summary>
	static void cpuRtd()
	{
	  ULO displacement = cpuGetNextOpcode16SignExt();
	  cpuSetAReg(7, cpuGetAReg(7) + 4 + displacement);
	  cpuSetInstructionTime(4);
	}
	static ULO cpuRteStackInc[16] = {0, 0, 4, 4, 8, 0, 0, 52, 50, 10, 24, 84, 16, 18, 0, 0};

	/// <summary>
        /// Rte
        /// </summary>
	static void cpuRte()
	{
	  if (cpuGetFlagSupervisor())
	  {
	    BOOLE redo = TRUE;
	    do
	    {
	      UWO newsr = memoryReadWord(cpuGetAReg(7));
	      cpuSetAReg(7, cpuGetAReg(7) + 2);
  	    
	      cpuSetPC(memoryReadLong(cpuGetAReg(7)));
	      cpuSetAReg(7, cpuGetAReg(7) + 4);
	      cpuReadPrefetch();

	      if (cpu_major > 0)
	      {
		ULO frame_type = (memoryReadWord(cpuGetAReg(7)) >> 12) & 0xf;
		cpuSetAReg(7, cpuGetAReg(7) + 2);
		cpuSetAReg(7, cpuGetAReg(7) + cpuRteStackInc[frame_type]);
		redo = (frame_type == 1 && cpu_major >= 2 && cpu_major < 6);
	      }
	      else redo = FALSE;
	      
	      cpuUpdateSr(newsr);
	    } while (redo);
	  }
	  else
	  {
	    cpuPrivilegeViolation();
	  }
	  cpuSetInstructionTime(20);
	}

	/// <summary>
        /// Swap
        /// </summary>
	static void cpuSwap(ULO reg)
	{
	  cpuSetDReg(reg, cpuJoinWordToLong((UWO)cpuGetDReg(reg), (UWO) (cpuGetDReg(reg) >> 16)));
	  cpuSetFlagsNZ00(cpuIsZeroL(cpuGetDReg(reg)), cpuMsbL(cpuGetDReg(reg)));
	  cpuSetInstructionTime(4);
	}

	/// <summary>
        /// Unlk
        /// </summary>
	static void cpuUnlk(ULO reg)
	{
	  cpuSetAReg(7, cpuGetAReg(reg));
	  cpuSetAReg(reg, memoryReadLong(cpuGetAReg(7)));
	  cpuSetAReg(7, cpuGetAReg(7) + 4);
	  cpuSetInstructionTime(12);
	}

	/// <summary>
        /// Link
        /// </summary>
	static void cpuLinkW(ULO reg)
	{
	  ULO disp = cpuGetNextOpcode16SignExt();
	  cpuSetAReg(7, cpuGetAReg(7) - 4);
	  memoryWriteLong(cpuGetAReg(reg), cpuGetAReg(7));
	  cpuSetAReg(reg, cpuGetAReg(7));
	  cpuSetAReg(7, cpuGetAReg(7) + disp);
	  cpuSetInstructionTime(18);
	}

	/// <summary>
        /// Link.
	/// 68020, 68030 and 68040 only.
        /// </summary>
	static void cpuLinkL(ULO reg)
	{
	  ULO disp = cpuGetNextOpcode32();
	  cpuSetAReg(7, cpuGetAReg(7) - 4);
	  memoryWriteLong(cpuGetAReg(reg), cpuGetAReg(7));
	  cpuSetAReg(reg, cpuGetAReg(7));
	  cpuSetAReg(7, cpuGetAReg(7) + disp);
	  cpuSetInstructionTime(4);
	}

	/// <summary>
        /// Ext.w (byte to word)
        /// </summary>
	static void cpuExtW(ULO reg)
	{
	  cpuSetDRegWord(reg, cpuGetDRegByteSignExtWord(reg));
	  cpuSetFlagsNZ00(cpuIsZeroW(cpuGetDRegWord(reg)), cpuMsbW(cpuGetDRegWord(reg)));
	  cpuSetInstructionTime(4);
	}

	/// <summary>
        /// Ext.l (word to long)
        /// </summary>
	static void cpuExtL(ULO reg)
	{
	  cpuSetDReg(reg, cpuGetDRegWordSignExtLong(reg));
	  cpuSetFlagsNZ00(cpuIsZeroL(cpuGetDReg(reg)), cpuMsbL(cpuGetDReg(reg)));
	  cpuSetInstructionTime(4);
	}

	/// <summary>
        /// ExtB.l (byte to long) (020+)
        /// </summary>
	static void cpuExtBL(ULO reg)
	{
	  cpuSetDReg(reg, cpuGetDRegByteSignExtLong(reg));
	  cpuSetFlagsNZ00(cpuIsZeroL(cpuGetDReg(reg)), cpuMsbL(cpuGetDReg(reg)));
	  cpuSetInstructionTime(4);
	}

	/// <summary>
        /// Exg Rx,Ry
        /// </summary>
	static void cpuExgAll(ULO reg1_type, ULO reg1, ULO reg2_type, ULO reg2)
	{
	  ULO tmp = cpuGetReg(reg1_type, reg1);
	  cpuSetReg(reg1_type, reg1, cpuGetReg(reg2_type, reg2));
	  cpuSetReg(reg2_type, reg2, tmp);
	  cpuSetInstructionTime(6);
	}

	/// <summary>
        /// Exg Dx,Dy
        /// </summary>
	static void cpuExgDD(ULO reg1, ULO reg2)
	{
	  cpuExgAll(0, reg1, 0, reg2);
	}

	/// <summary>
        /// Exg Ax,Ay
        /// </summary>
	static void cpuExgAA(ULO reg1, ULO reg2)
	{
	  cpuExgAll(1, reg1, 1, reg2);
	}

	/// <summary>
        /// Exg Dx,Ay
        /// </summary>
	static void cpuExgDA(ULO reg1, ULO reg2)
	{
	  cpuExgAll(0, reg1, 1, reg2);
	}

	/// <summary>
        /// Movem.w regs, -(Ax)
	/// Order: d0-d7,a0-a7   a7 first
        /// </summary>
	static void cpuMovemwPre(UWO regs, ULO reg)
	{
	  ULO cycles = 8;
	  ULO dstea = cpuGetAReg(reg);
	  ULO index = 1;
	  LON i, j;
	  BOOLE ea_reg_seen = FALSE;
	  ULO ea_reg_ea = 0;

	  for (i = 1; i >= 0; i--)
	  {
	    for (j = 7; j >= 0; j--)
	    {
	      if (regs & index)
	      {
		dstea -= 2;
		if (cpu_major >= 2 && i == 1 && j == reg)
		{
		  ea_reg_seen = TRUE;
		  ea_reg_ea = dstea;
		}
		else
		{
		  memoryWriteWord(cpuGetRegWord(i, j), dstea);
		}
		cycles += 4;
	      }
	      index = index << 1;
	    }
	  }
	  if (cpu_major >= 2 && ea_reg_seen)
	  {
	    memoryWriteWord((UWO)dstea, ea_reg_ea);
	  }
	  cpuSetAReg(reg, dstea);
	  cpuSetInstructionTime(cycles);
	}

	/// <summary>
        /// Movem.l regs, -(Ax)
	/// Order: d0-d7,a0-a7   a7 first
        /// </summary>
	static void cpuMovemlPre(UWO regs, ULO reg)
	{
	  ULO cycles = 8;
	  ULO dstea = cpuGetAReg(reg);
	  ULO index = 1;
	  LON i, j;
	  BOOLE ea_reg_seen = FALSE;
	  ULO ea_reg_ea = 0;

	  for (i = 1; i >= 0; i--)
	  {
	    for (j = 7; j >= 0; j--)
	    {
	      if (regs & index)
	      {
		dstea -= 4;
		if (cpu_major >= 2 && i == 1 && j == reg)
		{
		  ea_reg_seen = TRUE;
		  ea_reg_ea = dstea;
		}
		else
		{
		  memoryWriteLong(cpuGetReg(i, j), dstea);
		}
		cycles += 8;
	      }
	      index = index << 1;
	    }
	  }
	  if (cpu_major >= 2 && ea_reg_seen)
	  {
	    memoryWriteLong(dstea, ea_reg_ea);
	  }
	  cpuSetAReg(reg, dstea);
	  cpuSetInstructionTime(cycles);
	}

	/// <summary>
        /// Movem.w (Ax)+, regs
	/// Order: a7-a0,d7-d0   d0 first
        /// </summary>
	static void cpuMovemwPost(UWO regs, ULO reg)
	{
	  ULO cycles = 12;
	  ULO dstea = cpuGetAReg(reg);
	  ULO index = 1;
	  ULO i, j;

	  for (i = 0; i < 2; ++i)
	  {
	    for (j = 0; j < 8; ++j)
	    {
	      if (regs & index)
	      {
		// Each word, for both data and address registers, is sign-extended before stored.
		cpuSetReg(i, j, (ULO)(LON)(WOR) memoryReadWord(dstea));
		dstea += 2;
		cycles += 4;
	      }
	      index = index << 1;
	    }
	  }
	  cpuSetAReg(reg, dstea);
	  cpuSetInstructionTime(cycles);
	}

	/// <summary>
        /// Movem.l (Ax)+, regs
	/// Order: a7-a0,d7-d0   d0 first
        /// </summary>
	static void cpuMovemlPost(UWO regs, ULO reg)
	{
	  ULO cycles = 12;
	  ULO dstea = cpuGetAReg(reg);
	  ULO index = 1;
	  ULO i, j;

	  for (i = 0; i < 2; ++i)
	  {
	    for (j = 0; j < 8; ++j)
	    {
	      if (regs & index)
	      {
		cpuSetReg(i, j, memoryReadLong(dstea));
		dstea += 4;
		cycles += 8;
	      }
	      index = index << 1;
	    }
	  }
	  cpuSetAReg(reg, dstea);
	  cpuSetInstructionTime(cycles);
	}

	/// <summary>
        /// Movem.w <Control>, regs
	/// Order: a7-a0,d7-d0   d0 first
        /// </summary>
	static void cpuMovemwEa2R(UWO regs, ULO ea, ULO eacycles)
	{
	  ULO cycles = eacycles;
	  ULO dstea = ea;
	  ULO index = 1;
	  ULO i, j;

	  for (i = 0; i < 2; ++i)
	  {
	    for (j = 0; j < 8; ++j)
	    {
	      if (regs & index)
	      {
		// Each word, for both data and address registers, is sign-extended before stored.
		cpuSetReg(i, j, (ULO)(LON)(WOR) memoryReadWord(dstea));
		dstea += 2;
		cycles += 4;
	      }
	      index = index << 1;
	    }
	  }
	  cpuSetInstructionTime(cycles);
	}

	/// <summary>
        /// Movem.l <Control>, regs
	/// Order: a7-a0,d7-d0   d0 first
        /// </summary>
	static void cpuMovemlEa2R(UWO regs, ULO ea, ULO eacycles)
	{
	  ULO cycles = eacycles;
	  ULO dstea = ea;
	  ULO index = 1;
	  ULO i, j;

	  for (i = 0; i < 2; ++i)
	  {
	    for (j = 0; j < 8; ++j)
	    {
	      if (regs & index)
	      {
		cpuSetReg(i, j, memoryReadLong(dstea));
		dstea += 4;
		cycles += 8;
	      }
	      index = index << 1;
	    }
	  }
	  cpuSetInstructionTime(cycles);
	}

	/// <summary>
        /// Movem.w regs, <Control>
	/// Order: a7-a0,d7-d0   d0 first
        /// </summary>
	static void cpuMovemwR2Ea(UWO regs, ULO ea, ULO eacycles)
	{
	  ULO cycles = eacycles;
	  ULO dstea = ea;
	  ULO index = 1;
	  ULO i, j;

	  for (i = 0; i < 2; ++i)
	  {
	    for (j = 0; j < 8; ++j)
	    {
	      if (regs & index)
	      {
		memoryWriteWord(cpuGetRegWord(i, j), dstea);
		dstea += 2;
		cycles += 4;
	      }
	      index = index << 1;
	    }
	  }
	  cpuSetInstructionTime(cycles);
	}

	/// <summary>
        /// Movem.l regs, <Control>
	/// Order: a7-a0,d7-d0   d0 first
        /// </summary>
	static void cpuMovemlR2Ea(UWO regs, ULO ea, ULO eacycles)
	{
	  ULO cycles = eacycles;
	  ULO dstea = ea;
	  ULO index = 1;
	  ULO i, j;

	  for (i = 0; i < 2; ++i)
	  {
	    for (j = 0; j < 8; ++j)
	    {
	      if (regs & index)
	      {
		memoryWriteLong(cpuGetReg(i, j), dstea);
		dstea += 4;
		cycles += 8;
	      }
	      index = index << 1;
	    }
	  }
	  cpuSetInstructionTime(cycles);
	}
	
	/// <summary>
	/// Trap #vectorno
        /// </summary>
	static void cpuTrap(ULO vectorno)
	{
	  // PC written to the exception frame must be pc + 2. Jesus on E's depends on that.
	  cpuPrepareException(0x80 + vectorno*4, cpuGetPC(), FALSE);
	}

	/// <summary>
	/// move.l  Ax,Usp
        /// </summary>
	static void cpuMoveToUsp(ULO reg)
	{
	  if (cpuGetFlagSupervisor())
	  {
	    // In supervisor mode, usp does not affect a7
	    cpu_usp = cpuGetAReg(reg);
	    cpuSetInstructionTime(4);
	  }
	  else
	  {
	    cpuPrivilegeViolation();
	  }
	}

	/// <summary>
	/// move.l  Usp,Ax
        /// </summary>
	static void cpuMoveFromUsp(ULO reg)
	{
	  if (cpuGetFlagSupervisor())
	  {
	    // In supervisor mode, usp is up to date
	    cpuSetAReg(reg, cpu_usp);
	    cpuSetInstructionTime(4);
	  }
	  else
	  {
	    cpuPrivilegeViolation();
	  }
	}

	/// <summary>
	/// cmp.b (Ay)+,(Ax)+
        /// </summary>
	static void cpuCmpMB(ULO regx, ULO regy)
	{
	  UBY src = memoryReadByte(cpuEA03(regy, 1));
	  UBY dst = memoryReadByte(cpuEA03(regx, 1));
	  UBY res = dst - src;
	  cpuSetFlagsCmp(cpuIsZeroB(res), cpuMsbB(res), cpuMsbB(dst), cpuMsbB(src));
	  cpuSetInstructionTime(12);
	}

	/// <summary>
	/// cmp.w (Ay)+,(Ax)+
        /// </summary>
	static void cpuCmpMW(ULO regx, ULO regy)
	{
	  UWO src = memoryReadWord(cpuEA03(regy, 2));
	  UWO dst = memoryReadWord(cpuEA03(regx, 2));
	  UWO res = dst - src;
	  cpuSetFlagsCmp(cpuIsZeroW(res), cpuMsbW(res), cpuMsbW(dst), cpuMsbW(src));
	  cpuSetInstructionTime(12);
	}

	/// <summary>
	/// cmp.l (Ay)+,(Ax)+
        /// </summary>
	static void cpuCmpML(ULO regx, ULO regy)
	{
	  ULO src = memoryReadLong(cpuEA03(regy, 4));
	  ULO dst = memoryReadLong(cpuEA03(regx, 4));
	  ULO res = dst - src;
	  cpuSetFlagsCmp(cpuIsZeroL(res), cpuMsbL(res), cpuMsbL(dst), cpuMsbL(src));
	  cpuSetInstructionTime(20);
	}

        /// <summary>
        /// chk.w Dx, ea
	/// Undocumented features:
	/// Z is set from the register operand,
	/// V and C is always cleared.
        /// </summary>
        static void cpuChkW(UWO src2, UWO src1)
        {
	  cpuSetFlagZ(src2 == 0);
	  cpuSetFlagsVC(FALSE, FALSE);
	  if (((WOR)src1) < 0)
	  {
	    cpuSetFlagN(TRUE);
	    cpuPrepareException(6*4, cpuGetPC() - 2, FALSE);
	  }
	  else if (((WOR)src1) > ((WOR)src2))
	  {
	    cpuSetFlagN(FALSE);
	    cpuPrepareException(6*4, cpuGetPC() - 2, FALSE);
	  }
        }

        /// <summary>
        /// chk.l Dx, ea
	/// 68020+
	/// Undocumented features:
	/// Z is set from the register operand,
	/// V and C is always cleared.
        /// </summary>
        static void cpuChkL(ULO src2, ULO src1)
        {
	  cpuSetFlagZ(src2 == 0);
	  cpuSetFlagsVC(FALSE, FALSE);
	  if (((LON)src1) < 0)
	  {
	    cpuSetFlagN(TRUE);
	    cpuPrepareException(6*4, cpuGetPC() - 2, FALSE);
	  }
	  else if (((LON)src1) > ((LON)src2))
	  {
	    cpuSetFlagN(FALSE);
	    cpuPrepareException(6*4, cpuGetPC() - 2, FALSE);
	  }
        }

        /// <summary>
        /// addx.b dx,dy
        /// </summary>
        static UBY cpuAddXB(UBY dst, UBY src)
        {
	  UBY res = dst + src + ((cpuGetFlagX()) ? 1:0);
	  cpuSetFlagsAddX(cpuIsZeroB(res), cpuMsbB(res), cpuMsbB(dst), cpuMsbB(src));
          return res;
        }

        /// <summary>
        /// addx.w dx,dy
        /// </summary>
        static UWO cpuAddXW(UWO dst, UWO src)
        {
	  UWO res = dst + src + ((cpuGetFlagX()) ? 1:0);
	  cpuSetFlagsAddX(cpuIsZeroW(res), cpuMsbW(res), cpuMsbW(dst), cpuMsbW(src));
          return res;
        }

        /// <summary>
        /// addx.l dx,dy
        /// </summary>
        static ULO cpuAddXL(ULO dst, ULO src)
        {
	  ULO res = dst + src + ((cpuGetFlagX()) ? 1:0);
	  cpuSetFlagsAddX(cpuIsZeroL(res), cpuMsbL(res), cpuMsbL(dst), cpuMsbL(src));
          return res;
        }

        /// <summary>
        /// subx.b dx,dy
        /// </summary>
        static UBY cpuSubXB(UBY dst, UBY src)
        {
          UBY res = dst - src - ((cpuGetFlagX()) ? 1:0);
	  cpuSetFlagsSubX(cpuIsZeroB(res), cpuMsbB(res), cpuMsbB(dst), cpuMsbB(src));
          return res;
        }

        /// <summary>
        /// subx.w dx,dy
        /// </summary>
        static UWO cpuSubXW(UWO dst, UWO src)
        {
          UWO res = dst - src - ((cpuGetFlagX()) ? 1:0);
	  cpuSetFlagsSubX(cpuIsZeroW(res), cpuMsbW(res), cpuMsbW(dst), cpuMsbW(src));
          return res;
        }

        /// <summary>
        /// subx.l dx,dy
        /// </summary>
        static ULO cpuSubXL(ULO dst, ULO src)
        {
          ULO res = dst - src - ((cpuGetFlagX()) ? 1:0);
	  cpuSetFlagsSubX(cpuIsZeroL(res), cpuMsbL(res), cpuMsbL(dst), cpuMsbL(src));
          return res;
        }

        /// <summary>
        /// abcd.b src,dst
	/// Implemented using the information from:
	///     68000 Undocumented Behavior Notes
        ///                Fourth Edition
        ///    by Bart Trzynadlowski, May 12, 2003
        /// </summary>
        static UBY cpuAbcdB(UBY dst, UBY src)
        {
	  UBY xflag = (cpuGetFlagX()) ? 1:0;
	  UWO res = dst + src + xflag;
	  UWO res_unadjusted = res;
	  UBY res_bcd;
	  UBY low_nibble = (dst & 0xf) + (src & 0xf) + xflag;

	  if (low_nibble > 9)
	  {
	    res += 6;
	  }

	  if (res > 0x99)
	  {
	    res += 0x60;
	    cpuSetFlagXC(TRUE);
	  }
	  else
	  {
	    cpuSetFlagXC(FALSE);
	  }
	  
	  res_bcd = (UBY) res;
	  
	  if (res_bcd != 0)
	  {
	    cpuSetFlagZ(FALSE);
	  }
	  if (res_bcd & 0x80)
	  {
	    cpuSetFlagN(TRUE);
	  }
	  cpuSetFlagV(((res_unadjusted & 0x80) == 0) && (res_bcd & 0x80));
          return res_bcd;
        }

        /// <summary>
        /// sbcd.b src,dst
	/// nbcd.b src   (set dst=0)
	/// Implemented using the information from:
	///     68000 Undocumented Behavior Notes
        ///                Fourth Edition
        ///    by Bart Trzynadlowski, May 12, 2003
        /// </summary>
        static UBY cpuSbcdB(UBY dst, UBY src)
        {
	  UBY xflag = (cpuGetFlagX()) ? 1:0;
	  UWO res = dst - src - xflag;
	  UWO res_unadjusted = res;
	  UBY res_bcd;
	  
	  if (((src & 0xf) + xflag) > (dst & 0xf))
	  {
	    res -= 6;
	  }

	  if (res & 0x80)
	  {
	    res -= 0x60;
	    cpuSetFlagXC(TRUE);
	  }
	  else
	  {
	    cpuSetFlagXC(FALSE);
	  }

	  res_bcd = (UBY) res;
	  
	  if (res_bcd != 0)
	  {
	    cpuSetFlagZ(FALSE);
	  }
	  if (res_bcd & 0x80)
	  {
	    cpuSetFlagN(TRUE);
	  }
	  cpuSetFlagV(((res_unadjusted & 0x80) == 0x80) && !(res_bcd & 0x80));
	  return res_bcd;
	}

	/// <summary>
	/// nbcd.b dst
        /// </summary>
        static UBY cpuNbcdB(UBY dst)
        {
	  return cpuSbcdB(0, dst);
	}

	// Bit field functions
	static void cpuGetBfRegBytes(UBY *bytes, ULO regno)
	{
	  bytes[0] = (UBY)(cpu_regs[0][regno] >> 24);
	  bytes[1] = (UBY)(cpu_regs[0][regno] >> 16);
	  bytes[2] = (UBY)(cpu_regs[0][regno] >> 8);
	  bytes[3] = (UBY)cpu_regs[0][regno];
	}

	static void cpuGetBfEaBytes(UBY *bytes, ULO address, ULO count)
	{
	  ULO i;
	  for (i = 0; i < count; ++i)
	  {
	    bytes[i] = memoryReadByte(address + i);
	  }
	}

	static void cpuSetBfRegBytes(UBY *bytes, ULO regno)
	{
	  cpu_regs[0][regno] = (((ULO)bytes[0]) << 24)
	                     | (((ULO)bytes[1]) << 16)
	                     | (((ULO)bytes[2]) << 8)
	                     | ((ULO)bytes[3]);
	}

	static void cpuSetBfEaBytes(UBY *bytes, ULO address, ULO count)
	{
	  ULO i;
	  for (i = 0; i < count; ++i)
	  {
	    memoryWriteByte(bytes[i], address + i);
	  }
	}

	static LON cpuGetBfOffset(UWO ext, BOOLE offsetIsDr)
	{
	  LON offset = (ext >> 6) & 0x1f;
	  if (offsetIsDr)
	  {
	    offset = (LON) cpu_regs[0][offset & 7];
	  }
	  return offset;
	}

	static ULO cpuGetBfWidth(UWO ext, BOOLE widthIsDr)
	{
	  ULO width = (ext & 0x1f);
	  if (widthIsDr)
	  {
	    width = (cpu_regs[0][width & 7] & 0x1f);
	  }
	  if (width == 0)
	  {
	    width = 32;
	  }
	  return width;
	}

	static ULO cpuGetBfField(UBY *bytes, ULO end_offset, ULO byte_count, ULO field_mask)
	{
	  ULO i;
	  ULO field = ((ULO)bytes[byte_count - 1]) >> (8 - end_offset);

	  for (i = 1; i < byte_count; i++)
	  {
	    field |= ((ULO)bytes[byte_count - i - 1]) << (end_offset + 8*(i-1)); 
	  }
	  return field & field_mask;
	}

	static void cpuSetBfField(UBY *bytes, ULO end_offset, ULO byte_count, ULO field, ULO field_mask)
	{
	  ULO i;

	  bytes[byte_count - 1] = (field << (8 - end_offset)) | (bytes[byte_count - 1] & (UBY)~(field_mask << (8 - end_offset)));
	  for (i = 1; i < byte_count - 1; ++i)
	  {
	    bytes[byte_count - i - 1] = (UBY)(field >> (end_offset + 8*(i-1) ));
	  }
	  if (i < byte_count)
	  {
	    bytes[0] = (bytes[0] & (UBY)~(field_mask >> (end_offset + 8*(i-1))) | (UBY)(field >> (end_offset + 8*(i-1))));
	  }
	}

	struct cpuBfData
	{
	  UWO ext;
	  BOOLE offsetIsDr;
	  BOOLE widthIsDr;
	  LON offset;
	  ULO width;
	  ULO base_address;
	  ULO bit_offset;
	  ULO end_offset;
	  ULO byte_count;
	  ULO field;
	  ULO field_mask;
	  ULO dn;
	  UBY b[5];
	};

	void cpuBfExtWord(struct cpuBfData *bf_data, ULO val, BOOLE has_dn, BOOLE has_ea, UWO ext)
	{
	  bf_data->ext = ext;
	  bf_data->offsetIsDr = (bf_data->ext & 0x0800);
	  bf_data->widthIsDr = (bf_data->ext & 0x20);
	  bf_data->offset = cpuGetBfOffset(bf_data->ext, bf_data->offsetIsDr);
	  bf_data->width = cpuGetBfWidth(bf_data->ext, bf_data->widthIsDr);
	  bf_data->bit_offset = bf_data->offset & 7;
	  bf_data->end_offset = (bf_data->offset + bf_data->width) & 7;
	  bf_data->byte_count = ((bf_data->bit_offset + bf_data->width) >> 3) + 1;
	  bf_data->field = 0;
	  bf_data->field_mask = 0xffffffff >> (32 - bf_data->width);
	  if (has_dn)
	  {
	    bf_data->dn = (bf_data->ext & 0x7000) >> 12;
	  }
	  if (has_ea)
	  {
	    bf_data->base_address = val + (bf_data->offset >> 3);
	    cpuGetBfEaBytes(&bf_data->b[0], bf_data->base_address, bf_data->byte_count);
	  }
	  else
	  {
	    cpuGetBfRegBytes(&bf_data->b[0], val);
	  }
	}

	/// <summary>
	/// bfchg common logic
        /// </summary>
	static void cpuBfChgCommon(ULO val, BOOLE has_ea, UWO ext)
	{
	  struct cpuBfData bf_data;
	  cpuBfExtWord(&bf_data, val, FALSE, has_ea, ext);
	  bf_data.field = cpuGetBfField(&bf_data.b[0], bf_data.end_offset, bf_data.byte_count, bf_data.field_mask);
	  cpuSetFlagsNZVC(bf_data.field == 0, bf_data.field & (1 << (bf_data.width - 1)), FALSE, FALSE);
	  cpuSetBfField(&bf_data.b[0], bf_data.end_offset, bf_data.byte_count, (~bf_data.field) & bf_data.field_mask, bf_data.field_mask);
	  if (has_ea)
	  {
	    cpuSetBfEaBytes(&bf_data.b[0], bf_data.base_address, bf_data.byte_count);
	  }
	  else
	  {
	    cpuSetBfRegBytes(&bf_data.b[0], val);
	  }
	}

	/// <summary>
	/// bfchg dx {offset:width}
        /// </summary>
        static void cpuBfChgReg(ULO regno, UWO ext)
        {
	  cpuBfChgCommon(regno, FALSE, ext);
	}

	/// <summary>
	/// bfchg ea {offset:width}
        /// </summary>
        static void cpuBfChgEa(ULO ea, UWO ext)
        {
	  cpuBfChgCommon(ea, TRUE, ext);
	}

	/// <summary>
	/// bfclr common logic
        /// </summary>
	static void cpuBfClrCommon(ULO val, BOOLE has_ea, UWO ext)
	{
	  struct cpuBfData bf_data;
	  cpuBfExtWord(&bf_data, val, FALSE, has_ea, ext);
	  bf_data.field = cpuGetBfField(&bf_data.b[0], bf_data.end_offset, bf_data.byte_count, bf_data.field_mask);
	  cpuSetFlagsNZVC(bf_data.field == 0, bf_data.field & (1 << (bf_data.width - 1)), FALSE, FALSE);
	  cpuSetBfField(&bf_data.b[0], bf_data.end_offset, bf_data.byte_count, 0, bf_data.field_mask);
	  if (has_ea)
	  {
	    cpuSetBfEaBytes(&bf_data.b[0], bf_data.base_address, bf_data.byte_count);
	  }
	  else
	  {
	    cpuSetBfRegBytes(&bf_data.b[0], val);
	  }
	}

	/// <summary>
	/// bfclr dx {offset:width}
        /// </summary>
        static void cpuBfClrReg(ULO regno, UWO ext)
        {
	  cpuBfClrCommon(regno, FALSE, ext);
	}

	/// <summary>
	/// bfclr ea {offset:width}
        /// </summary>
        static void cpuBfClrEa(ULO ea, UWO ext)
        {
	  cpuBfClrCommon(ea, TRUE, ext);
	}

	/// <summary>
	/// bfexts common logic
        /// </summary>
        static void cpuBfExtsCommon(ULO val, BOOLE has_ea, UWO ext)
        {
	  struct cpuBfData bf_data;
	  BOOLE n_flag;
	  cpuBfExtWord(&bf_data, val, TRUE, has_ea, ext);
	  bf_data.field = cpuGetBfField(&bf_data.b[0], bf_data.end_offset, bf_data.byte_count, bf_data.field_mask);
	  n_flag = bf_data.field & (1 << (bf_data.width - 1));
	  cpuSetFlagsNZVC(bf_data.field == 0, n_flag, FALSE, FALSE);
	  if (n_flag)
	  {
	    bf_data.field = ~bf_data.field_mask | bf_data.field;
	  }
	  cpu_regs[0][bf_data.dn] = bf_data.field;
	}

	/// <summary>
	/// bfexts dx {offset:width}, Dn
        /// </summary>
        static void cpuBfExtsReg(ULO regno, UWO ext)
        {
	  cpuBfExtsCommon(regno, FALSE, ext);
	}

	/// <summary>
	/// bfexts ea {offset:width}, Dn
        /// </summary>
        static void cpuBfExtsEa(ULO ea, UWO ext)
        {
	  cpuBfExtsCommon(ea, TRUE, ext);
	}

	/// <summary>
	/// bfextu ea {offset:width}, Dn
        /// </summary>
        static void cpuBfExtuCommon(ULO val, BOOLE has_ea, UWO ext)
        {
	  struct cpuBfData bf_data;
	  cpuBfExtWord(&bf_data, val, TRUE, has_ea, ext);
	  bf_data.field = cpuGetBfField(&bf_data.b[0], bf_data.end_offset, bf_data.byte_count, bf_data.field_mask);
	  cpuSetFlagsNZVC(bf_data.field == 0, bf_data.field & (1 << (bf_data.width - 1)), FALSE, FALSE);
	  cpu_regs[0][bf_data.dn] = bf_data.field;
	}

	/// <summary>
	/// bfextu dx {offset:width}, Dn
        /// </summary>
        static void cpuBfExtuReg(ULO regno, UWO ext)
        {
	  cpuBfExtuCommon(regno, FALSE, ext);
	}

	/// <summary>
	/// bfextu ea {offset:width}, Dn
        /// </summary>
        static void cpuBfExtuEa(ULO ea, UWO ext)
        {
	  cpuBfExtuCommon(ea, TRUE, ext);
	}

	/// <summary>
	/// bfffo common logic
        /// </summary>
        static void cpuBfFfoCommon(ULO val, BOOLE has_ea, UWO ext)
        {
	  struct cpuBfData bf_data;
	  ULO i;
	  cpuBfExtWord(&bf_data, val, TRUE, has_ea, ext);
	  bf_data.field = cpuGetBfField(&bf_data.b[0], bf_data.end_offset, bf_data.byte_count, bf_data.field_mask);
	  cpuSetFlagsNZVC(bf_data.field == 0, bf_data.field & (1 << (bf_data.width - 1)), FALSE, FALSE);
	  for (i = 0; i < bf_data.width; ++i)
	  {
	    if (bf_data.field & (0x1 << (bf_data.width - i - 1)))
	      break;
	  }
	  cpu_regs[0][bf_data.dn] = bf_data.offset + i;
	}

	/// <summary>
	/// bfffo dx {offset:width}, Dn
        /// </summary>
        static void cpuBfFfoReg(ULO regno, UWO ext)
        {
	  cpuBfFfoCommon(regno, FALSE, ext);
	}

	/// <summary>
	/// bfffo ea {offset:width}, Dn
        /// </summary>
        static void cpuBfFfoEa(ULO ea, UWO ext)
        {
	  cpuBfFfoCommon(ea, TRUE, ext);
	}

	/// <summary>
	/// bfins common logic
        /// </summary>
	static void cpuBfInsCommon(ULO val, BOOLE has_ea, UWO ext)
	{
	  struct cpuBfData bf_data;
	  cpuBfExtWord(&bf_data, val, TRUE, has_ea, ext);
	  bf_data.field = cpuGetBfField(&bf_data.b[0], bf_data.end_offset, bf_data.byte_count, bf_data.field_mask);
	  cpuSetFlagsNZVC(bf_data.field == 0, bf_data.field & (1 << (bf_data.width - 1)), FALSE, FALSE);
	  bf_data.field = cpu_regs[0][bf_data.dn] & bf_data.field_mask;
	  cpuSetBfField(&bf_data.b[0], bf_data.end_offset, bf_data.byte_count, bf_data.field, bf_data.field_mask);
	  if (has_ea)
	  {
	    cpuSetBfEaBytes(&bf_data.b[0], bf_data.base_address, bf_data.byte_count);
	  }
	  else
	  {
	    cpuSetBfRegBytes(&bf_data.b[0], val);
	  }
	}

	/// <summary>
	/// bfins Dn, ea {offset:width}
        /// </summary>
        static void cpuBfInsReg(ULO regno, UWO ext)
        {
	  cpuBfInsCommon(regno, FALSE, ext);
	}

	/// <summary>
	/// bfins Dn, ea {offset:width}
        /// </summary>
        static void cpuBfInsEa(ULO ea, UWO ext)
        {
	  cpuBfInsCommon(ea, TRUE, ext);
	}

	/// <summary>
	/// bfset common logic
        /// </summary>
	static void cpuBfSetCommon(ULO val, BOOLE has_ea, UWO ext)
	{
	  struct cpuBfData bf_data;
	  cpuBfExtWord(&bf_data, val, FALSE, has_ea, ext);
	  bf_data.field = cpuGetBfField(&bf_data.b[0], bf_data.end_offset, bf_data.byte_count, bf_data.field_mask);
	  cpuSetFlagsNZVC(bf_data.field == 0, bf_data.field & (1 << (bf_data.width - 1)), FALSE, FALSE);
	  bf_data.field = bf_data.field_mask;
	  cpuSetBfField(&bf_data.b[0], bf_data.end_offset, bf_data.byte_count, bf_data.field, bf_data.field_mask);
	  if (has_ea)
	  {
	    cpuSetBfEaBytes(&bf_data.b[0], bf_data.base_address, bf_data.byte_count);
	  }
	  else
	  {
	    cpuSetBfRegBytes(&bf_data.b[0], val);
	  }
	}

	/// <summary>
	/// bfset dx {offset:width}
        /// </summary>
        static void cpuBfSetReg(ULO regno, UWO ext)
        {
	  cpuBfSetCommon(regno, FALSE, ext);
	}

	/// <summary>
	/// bfset ea {offset:width}
        /// </summary>
        static void cpuBfSetEa(ULO ea, UWO ext)
        {
	  cpuBfSetCommon(ea, TRUE, ext);
	}

	/// <summary>
	/// bftst common logic
        /// </summary>
	static void cpuBfTstCommon(ULO val, BOOLE has_ea, UWO ext)
	{
	  struct cpuBfData bf_data;
	  cpuBfExtWord(&bf_data, val, FALSE, has_ea, ext);
	  cpuGetBfEaBytes(&bf_data.b[0], bf_data.base_address, bf_data.byte_count);
	  bf_data.field = cpuGetBfField(&bf_data.b[0], bf_data.end_offset, bf_data.byte_count, bf_data.field_mask);
	  cpuSetFlagsNZVC(bf_data.field == 0, bf_data.field & (1 << (bf_data.width - 1)), FALSE, FALSE);
	}

	/// <summary>
	/// bftst dx {offset:width}
        /// </summary>
        static void cpuBfTstReg(ULO regno, UWO ext)
        {
	  cpuBfTstCommon(regno, FALSE, ext);
	}

	/// <summary>
	/// bftst ea {offset:width}
        /// </summary>
        static void cpuBfTstEa(ULO ea, UWO ext)
        {
	  cpuBfTstCommon(ea, TRUE, ext);
	}

	/// <summary>
	/// movep.w (d16, Ay), Dx
        /// </summary>
        static void cpuMovepWReg(ULO areg, ULO dreg)
        {
	  ULO ea = cpuGetAReg(areg) + cpuGetNextOpcode16SignExt();
	  memoryWriteByte((UBY) (cpuGetDReg(dreg) >> 8), ea);
	  memoryWriteByte(cpuGetDRegByte(dreg), ea + 2);
	  cpuSetInstructionTime(16);
	}

	/// <summary>
	/// movep.l (d16, Ay), Dx
        /// </summary>
        static void cpuMovepLReg(ULO areg, ULO dreg)
        {
	  ULO ea = cpuGetAReg(areg) + cpuGetNextOpcode16SignExt();
	  memoryWriteByte((UBY)(cpuGetDReg(dreg) >> 24), ea);
	  memoryWriteByte((UBY)(cpuGetDReg(dreg) >> 16), ea + 2);
	  memoryWriteByte((UBY)(cpuGetDReg(dreg) >> 8), ea + 4);
	  memoryWriteByte(cpuGetDRegByte(dreg), ea + 6);
	  cpuSetInstructionTime(24);
	}

	/// <summary>
	/// movep.w Dx, (d16, Ay)
        /// </summary>
        static void cpuMovepWEa(ULO areg, ULO dreg)
        {
	  ULO ea = cpuGetAReg(areg) + cpuGetNextOpcode16SignExt();
	  cpuSetDRegWord(dreg, cpuJoinByteToWord(memoryReadByte(ea), memoryReadByte(ea + 2)));
	  cpuSetInstructionTime(16);
	}

	/// <summary>
	/// movep.l Dx, (d16, Ay)
        /// </summary>
        static void cpuMovepLEa(ULO areg, ULO dreg)
        {
	  ULO ea = cpuGetAReg(areg) + cpuGetNextOpcode16SignExt();
	  cpuSetDReg(dreg, cpuJoinByteToLong(memoryReadByte(ea), memoryReadByte(ea + 2), memoryReadByte(ea + 4), memoryReadByte(ea + 6)));
	  cpuSetInstructionTime(24);
	}

	/// <summary>
	/// pack Dx, Dy, #adjustment
        /// </summary>
        static void cpuPackReg(ULO yreg, ULO xreg)
        {
	  UWO adjustment = cpuGetNextOpcode16();
	  UWO src = cpuGetDRegWord(xreg) + adjustment;
	  cpuSetDRegByte(yreg, (UBY) (((src >> 4) & 0xf0) | (src & 0xf)));
	}

	/// <summary>
	/// pack -(Ax), -(Ay), #adjustment
        /// </summary>
        static void cpuPackEa(ULO yreg, ULO xreg)
        {
	  UWO adjustment = cpuGetNextOpcode16();
	  UBY b1 = memoryReadByte(cpuEA04(xreg, 1));
	  UBY b2 = memoryReadByte(cpuEA04(xreg, 1));
	  UWO result = ((((UWO)b1) << 8) | (UWO) b2) + adjustment;
	  memoryWriteByte((UBY) (((result >> 4) & 0xf0) | (result & 0xf)), cpuEA04(yreg, 1));
	}

	/// <summary>
	/// unpk Dx, Dy, #adjustment
        /// </summary>
        static void cpuUnpkReg(ULO yreg, ULO xreg)
        {
	  UWO adjustment = cpuGetNextOpcode16();
	  UBY b1 = cpuGetDRegByte(xreg);
	  UWO result = ((((UWO)(b1 & 0xf0)) << 4) | ((UWO)(b1 & 0xf))) + adjustment;
	  cpuSetDRegWord(yreg, result);
	}

	/// <summary>
	/// unpk -(Ax), -(Ay), #adjustment
        /// </summary>
        static void cpuUnpkEa(ULO yreg, ULO xreg)
        {
	  UWO adjustment = cpuGetNextOpcode16();
	  UBY b1 = memoryReadByte(cpuEA04(xreg, 1));
	  UWO result = ((((UWO)(b1 & 0xf0)) << 4) | ((UWO)(b1 & 0xf))) + adjustment;
	  memoryWriteByte((UBY) (result >> 8), cpuEA04(yreg, 1));
	  memoryWriteByte((UBY) result, cpuEA04(yreg, 1));
	}

	/// <summary>
	/// movec
        /// </summary>
        static void cpuMoveCFrom()
        {
	  if (cpuGetFlagSupervisor())
	  {
	    UWO extension = (UWO) cpuGetNextOpcode16();
	    ULO da = (extension >> 15) & 1;
	    ULO regno = (extension >> 12) & 7;
	    ULO ctrl_regno = extension & 0xfff;
	    if (cpu_major == 1)
	    {
	      switch (ctrl_regno)
	      {
		case 0x000: cpu_regs[da][regno] = cpuGetSfc(); break;
		case 0x001: cpu_regs[da][regno] = cpuGetDfc(); break;
		case 0x800: cpu_regs[da][regno] = cpu_usp; break;
		case 0x801: cpu_regs[da][regno] = cpuGetVbr(); break;
		default:  cpuPrepareException(0x10, cpuGetPC() - 4, FALSE); return;	  // Illegal instruction
	      }
	    }
	    else if (cpu_major == 2)
	    {
	      switch (ctrl_regno)
	      {
		case 0x000: cpu_regs[da][regno] = cpuGetSfc(); break;
		case 0x001: cpu_regs[da][regno] = cpuGetDfc(); break;
		case 0x002: cpu_regs[da][regno] = cpuGetCacr(); break;
		case 0x800: cpu_regs[da][regno] = cpu_usp; break; // In supervisor mode, usp is up to date.
		case 0x801: cpu_regs[da][regno] = cpuGetVbr(); break;
		case 0x802: cpu_regs[da][regno] = cpuGetCaar() & 0xfc; break;
		case 0x803: cpu_regs[da][regno] = cpuGetMsp(); break;
		case 0x804: cpu_regs[da][regno] = cpuGetIsp(); break;
		default:  cpuPrepareException(0x10, cpuGetPC() - 4, FALSE); return;	  // Illegal instruction
	      }
	    }
	    else if (cpu_major == 3)
	    {
	      switch (ctrl_regno)
	      {
		case 0x000: cpu_regs[da][regno] = cpuGetSfc(); break;
		case 0x001: cpu_regs[da][regno] = cpuGetDfc(); break;
		case 0x002: cpu_regs[da][regno] = cpuGetCacr(); break;
		case 0x800: cpu_regs[da][regno] = cpu_usp; break; // In supervisor mode, usp is up to date.
		case 0x801: cpu_regs[da][regno] = cpuGetVbr(); break;
		case 0x802: cpu_regs[da][regno] = cpuGetCaar() & 0xfc; break;
		case 0x803: cpu_regs[da][regno] = cpuGetMsp(); break;
		case 0x804: cpu_regs[da][regno] = cpuGetIsp(); break;
		default:  cpuPrepareException(0x10, cpuGetPC() - 4, FALSE); return;	  // Illegal instruction
	      }
	    }
	  }
	  else
	  {
	    cpuPrivilegeViolation();
	    return;
	  }
	  cpuSetInstructionTime(4);
	}

	/// <summary>
	/// movec
        /// </summary>
        static void cpuMoveCTo()
        {
	  if (cpuGetFlagSupervisor())
	  {
	    UWO extension = (UWO) cpuGetNextOpcode16();
	    ULO da = (extension >> 15) & 1;
	    ULO regno = (extension >> 12) & 7;
	    ULO ctrl_regno = extension & 0xfff;
	    if (cpu_major == 1)
	    {
	      switch (ctrl_regno)
	      {
		case 0x000: cpuSetSfc(cpu_regs[da][regno] & 7); break;
		case 0x001: cpuSetDfc(cpu_regs[da][regno] & 7); break;
		case 0x800: cpu_usp = cpu_regs[da][regno]; break;
		case 0x801: cpuSetVbr(cpu_regs[da][regno]); break;
		default:  cpuPrepareException(0x10, cpuGetPC() - 4, FALSE); return;	  // Illegal instruction
	      }
	    }
	    else if (cpu_major == 2)
	    {
	      switch (ctrl_regno)
	      {
		case 0x000: cpuSetSfc(cpu_regs[da][regno] & 7); break;
		case 0x001: cpuSetDfc(cpu_regs[da][regno] & 7); break;
		case 0x002: cpuSetCacr(cpu_regs[da][regno] & 0x3); break;
		case 0x800: cpu_usp = cpu_regs[da][regno]; break;
		case 0x801: cpuSetVbr(cpu_regs[da][regno]); break;
		case 0x802: cpuSetCaar(cpu_regs[da][regno] & 0x00fc); break;
		case 0x803: cpuSetMsp(cpu_regs[da][regno]); break;
		case 0x804: cpuSetIsp(cpu_regs[da][regno]); break;
		default:  cpuPrepareException(0x10, cpuGetPC() - 4, FALSE); return;	  // Illegal instruction
	      }
	    }
	    else if (cpu_major == 3)
	    {
	      switch (ctrl_regno)
	      {
		case 0x000: cpuSetSfc(cpu_regs[da][regno] & 7); break;
		case 0x001: cpuSetDfc(cpu_regs[da][regno] & 7); break;
		case 0x002: cpuSetCacr(cpu_regs[da][regno] & 0x3313); break;
		case 0x800: cpu_usp = cpu_regs[da][regno]; break;
		case 0x801: cpuSetVbr(cpu_regs[da][regno]); break;
		case 0x802: cpuSetCaar(cpu_regs[da][regno] & 0x00fc); break;
		case 0x803: cpuSetMsp(cpu_regs[da][regno]); break;
		case 0x804: cpuSetIsp(cpu_regs[da][regno]); break;
		default:  cpuPrepareException(0x10, cpuGetPC() - 4, FALSE); return;	  // Illegal instruction
	      }
	    }
	  }
	  else
	  {
	    cpuPrivilegeViolation();
	    return;
	  }
	  cpuSetInstructionTime(4);
	}

	/// <summary>
	/// moves.b Rn, ea / moves.b ea, Rn
        /// </summary>
        static void cpuMoveSB(ULO ea, UWO extension)
        {
	  if (cpuGetFlagSupervisor())
	  {
	    ULO da = (extension >> 15) & 1;
	    ULO regno = (extension >> 12) & 7;
	    if (extension & 0x0800) // From Rn to ea (in dfc)
	    {
	      memoryWriteByte((UBY)cpu_regs[da][regno], ea);
	    }
	    else  // From ea to Rn (in sfc)
	    {
	      UBY data = memoryReadByte(ea);
	      if (da == 0)
	      {
		cpuSetDRegByte(regno, data);
	      }
	      else
	      {
		cpu_regs[1][regno] = (ULO)(LON)(BYT) data;
	      }
	    }
	  }
	  else
	  {
	    cpuPrivilegeViolation();
	  }
	  cpuSetInstructionTime(4);	  
	}

	/// <summary>
	/// moves.w Rn, ea / moves.w ea, Rn
        /// </summary>
        static void cpuMoveSW(ULO ea, UWO extension)
        {
	  if (cpuGetFlagSupervisor())
	  {
	    ULO da = (extension >> 15) & 1;
	    ULO regno = (extension >> 12) & 7;
	    if (extension & 0x0800) // From Rn to ea (in dfc)
	    {
	      memoryWriteWord((UWO)cpu_regs[da][regno], ea);
	    }
	    else  // From ea to Rn (in sfc)
	    {
	      UWO data = memoryReadWord(ea);
	      if (da == 0)
	      {
		cpuSetDRegWord(regno, data);
	      }
	      else
	      {
		cpu_regs[1][regno] = (ULO)(LON)(WOR) data;
	      }
	    }
	  }
	  else
	  {
	    cpuPrivilegeViolation();
	  }
	  cpuSetInstructionTime(4);	  
	}

	/// <summary>
	/// moves.l Rn, ea / moves.l ea, Rn
        /// </summary>
        static void cpuMoveSL(ULO ea, UWO extension)
        {
	  if (cpuGetFlagSupervisor())
	  {
	    ULO da = (extension >> 15) & 1;
	    ULO regno = (extension >> 12) & 7;
	    if (extension & 0x0800) // From Rn to ea (in dfc)
	    {
	      memoryWriteLong(cpu_regs[da][regno], ea);
	    }
	    else  // From ea to Rn (in sfc)
	    {
	      cpu_regs[0][regno] = memoryReadLong(ea);
	    }
	  }
	  else
	  {
	    cpuPrivilegeViolation();
	  }
	  cpuSetInstructionTime(4);	  
	}

	/// <summary>
	/// Trapcc
        /// </summary>
        static void cpuTrapcc(ULO cc)
        {
	  if (cc)
	  {
	      cpuPrepareException(0x1C, cpuGetPC(), FALSE);
	      return;
	  }
	  cpuSetInstructionTime(4);	  
	}

	/// <summary>
	/// Trapcc.w #
        /// </summary>
        static void cpuTrapccW(ULO cc)
        {
	  UWO imm = cpuGetNextOpcode16();
	  if (cc)
	  {
	      cpuPrepareException(0x1C, cpuGetPC(), FALSE);
	      return;
	  }
	  cpuSetInstructionTime(4);	  
	}

	/// <summary>
	/// trapcc.l #
        /// </summary>
        static void cpuTrapccL(ULO cc)
        {
	  ULO imm = cpuGetNextOpcode32();
	  if (cc)
	  {
	      cpuPrepareException(0x1C, cpuGetPC(), FALSE);
	      return;
	  }
	  cpuSetInstructionTime(4);	  
	}

	/// <summary>
	/// cas.b Dc,Du, ea
        /// </summary>
        static void cpuCasB(ULO ea, UWO extension)
        {
	  UBY dst = memoryReadByte(ea);
	  ULO cmp_regno = extension & 7;
	  UBY res = dst - (UBY) cpu_regs[0][cmp_regno];

	  cpuSetFlagsCmp(cpuIsZeroB(res), cpuMsbB(res), cpuMsbB(dst), cpuMsbB((UBY) cpu_regs[0][cmp_regno]));

	  if (cpuIsZeroB(res))
	  {
	    memoryWriteByte((UBY)cpu_regs[0][(extension >> 6) & 7], ea);
	  }
	  else
	  {
	    cpu_regs[0][cmp_regno] = (cpu_regs[0][cmp_regno] & 0xffffff00) | dst;
	  }
	  cpuSetInstructionTime(4);
	}

	/// <summary>
	/// cas.w Dc,Du, ea
        /// </summary>
        static void cpuCasW(ULO ea, UWO extension)
        {
	  UWO dst = memoryReadWord(ea);
	  ULO cmp_regno = extension & 7;
	  UWO res = dst - (UWO) cpu_regs[0][cmp_regno];

	  cpuSetFlagsCmp(cpuIsZeroW(res), cpuMsbW(res), cpuMsbW(dst), cpuMsbW((UWO) cpu_regs[0][cmp_regno]));

	  if (cpuIsZeroW(res))
	  {
	    memoryWriteWord((UWO)cpu_regs[0][(extension >> 6) & 7], ea);
	  }
	  else
	  {
	    cpuSetDRegWord(cmp_regno, dst);
	  }
	  cpuSetInstructionTime(4);
	}

	/// <summary>
	/// cas.l Dc,Du, ea
        /// </summary>
        static void cpuCasL(ULO ea, UWO extension)
        {
	  ULO dst = memoryReadLong(ea);
	  ULO cmp_regno = extension & 7;
	  ULO res = dst - cpu_regs[0][cmp_regno];

	  cpuSetFlagsCmp(cpuIsZeroL(res), cpuMsbL(res), cpuMsbL(dst), cpuMsbL(cpu_regs[0][cmp_regno]));

	  if (cpuIsZeroL(res))
	  {
	    memoryWriteLong(cpu_regs[0][(extension >> 6) & 7], ea);
	  }
	  else
	  {
	    cpu_regs[0][cmp_regno] = dst;
	  }
	  cpuSetInstructionTime(4);
	}

	/// <summary>
	/// cas2.w Dc1:Dc2,Du1:Du2,(Rn1):(Rn2)
        /// </summary>
        static void cpuCas2W()
        {
	  UWO extension1 = cpuGetNextOpcode16();
	  UWO extension2 = cpuGetNextOpcode16();
	  ULO ea1 = cpu_regs[extension1 >> 15][(extension1 >> 12) & 7];
	  ULO ea2 = cpu_regs[extension2 >> 15][(extension2 >> 12) & 7];
	  UWO dst1 = memoryReadWord(ea1);
	  UWO dst2 = memoryReadWord(ea2);
	  ULO cmp1_regno = extension1 & 7;
	  ULO cmp2_regno = extension2 & 7;
	  UWO res1 = dst1 - (UWO) cpu_regs[0][cmp1_regno];
	  UWO res2 = dst2 - (UWO) cpu_regs[0][cmp2_regno];

	  if (cpuIsZeroW(res1))
	  {
	    cpuSetFlagsCmp(cpuIsZeroW(res2), cpuMsbW(res2), cpuMsbW(dst2), cpuMsbW((UWO) cpu_regs[0][cmp2_regno]));
	  }
	  else
	  {
	    cpuSetFlagsCmp(cpuIsZeroW(res1), cpuMsbW(res1), cpuMsbW(dst1), cpuMsbW((UWO) cpu_regs[0][cmp1_regno]));
	  }

	  if (cpuIsZeroW(res1) && cpuIsZeroW(res2))
	  {
	    memoryWriteWord((UWO)cpu_regs[0][(extension1 >> 6) & 7], ea1);
	    memoryWriteWord((UWO)cpu_regs[0][(extension2 >> 6) & 7], ea2);
	  }
	  else
	  {
	    cpuSetDRegWord(cmp1_regno, dst1);
	    cpuSetDRegWord(cmp2_regno, dst2);
	  }
	  cpuSetInstructionTime(4);
	}

	/// <summary>
	/// cas2.l Dc1:Dc2,Du1:Du2,(Rn1):(Rn2)
        /// </summary>
        static void cpuCas2L()
        {
	  UWO extension1 = cpuGetNextOpcode16();
	  UWO extension2 = cpuGetNextOpcode16();
	  ULO ea1 = cpu_regs[extension1 >> 15][(extension1 >> 12) & 7];
	  ULO ea2 = cpu_regs[extension2 >> 15][(extension2 >> 12) & 7];
	  ULO dst1 = memoryReadLong(ea1);
	  ULO dst2 = memoryReadLong(ea2);
	  ULO cmp1_regno = extension1 & 7;
	  ULO cmp2_regno = extension2 & 7;
	  ULO res1 = dst1 - cpu_regs[0][cmp1_regno];
	  ULO res2 = dst2 - cpu_regs[0][cmp2_regno];

	  if (cpuIsZeroL(res1))
	  {
	    cpuSetFlagsCmp(cpuIsZeroL(res2), cpuMsbL(res2), cpuMsbL(dst2), cpuMsbL(cpu_regs[0][cmp2_regno]));
	  }
	  else
	  {
	    cpuSetFlagsCmp(cpuIsZeroL(res1), cpuMsbL(res1), cpuMsbL(dst1), cpuMsbL(cpu_regs[0][cmp1_regno]));
	  }

	  if (cpuIsZeroL(res1) && cpuIsZeroL(res2))
	  {
	    memoryWriteLong(cpu_regs[0][(extension1 >> 6) & 7], ea1);
	    memoryWriteLong(cpu_regs[0][(extension2 >> 6) & 7], ea2);
	  }
	  else
	  {
	    cpu_regs[0][cmp1_regno] = dst1;
	    cpu_regs[0][cmp2_regno] = dst2;
	  }
	  cpuSetInstructionTime(4);
	}

	/// <summary>
	/// Common code for chk2 ea, Rn / cmp2 ea, Rn
        /// </summary>
	static void cpuChkCmp(ULO lb, ULO ub, ULO val, BOOLE is_chk2)
	{
	  BOOLE z = (val == lb || val == ub);
	  BOOLE c = ((lb <= ub) && (val < lb || val > ub)) || ((lb > ub) && (val < lb) && (val > ub)); 
	  cpuSetFlagZ(z);
	  cpuSetFlagC(c);
	  cpuSetInstructionTime(4);
	  if (is_chk2 && c)
	  {
	    cpuPrepareException(6*4, cpuGetPC() - 2, FALSE);
	  }
	}

	/// <summary>
	/// chk2.b ea, Rn / cmp2.b ea, Rn
        /// </summary>
        static void cpuChkCmp2B(ULO ea, UWO extension)
        {
	  ULO da = (ULO) (extension >> 15);
	  ULO rn = (ULO) (extension >> 12) & 7;
	  BOOLE is_chk2 = (extension & 0x0800);
	  if (da == 1)
	  {
	    cpuChkCmp((ULO)(LON)(BYT)memoryReadByte(ea), (ULO)(LON)(BYT)memoryReadByte(ea + 1), cpuGetAReg(rn), is_chk2);
	  }
	  else
	  {
	    cpuChkCmp((ULO)memoryReadByte(ea), (ULO)memoryReadByte(ea + 1), (ULO)(UBY)cpuGetDReg(rn), is_chk2);
	  }
	}

	/// <summary>
	/// chk2.w ea, Rn / cmp2.w ea, Rn
        /// </summary>
        static void cpuChkCmp2W(ULO ea, UWO extension)
        {
	  ULO da = (ULO) (extension >> 15);
	  ULO rn = (ULO) (extension >> 12) & 7;
	  BOOLE is_chk2 = (extension & 0x0800);
	  if (da == 1)
	  {
	    cpuChkCmp((ULO)(LON)(WOR)memoryReadWord(ea), (ULO)(LON)(WOR)memoryReadWord(ea + 1), cpuGetAReg(rn), is_chk2);
	  }
	  else
	  {
	    cpuChkCmp((ULO)memoryReadWord(ea), (ULO)memoryReadWord(ea + 2), (ULO)(UWO)cpuGetDReg(rn), is_chk2);
	  }
	}

	/// <summary>
	/// chk2.l ea, Rn / cmp2.l ea, Rn
        /// </summary>
        static void cpuChkCmp2L(ULO ea, UWO extension)
        {
	  ULO da = (ULO) (extension >> 15);
	  ULO rn = (ULO) (extension >> 12) & 7;
	  BOOLE is_chk2 = (extension & 0x0800);
	  cpuChkCmp(memoryReadLong(ea), memoryReadLong(ea + 4), cpuGetReg(da, rn), is_chk2);
	}

	/// <summary>
	/// callm
	/// Since this is a coprocessor instruction, this is NOP.
	/// This will likely fail, but anything we do here will be wrong anyhow.
        /// </summary>
        static void cpuCallm(ULO ea, UWO extension)
        {
	  cpuSetInstructionTime(4);
	}

	/// <summary>
	/// rtm
	/// Since this is a coprocessor instruction, this is NOP.
	/// This will likely fail, but anything we do here will be wrong anyhow.
        /// </summary>
        static void cpuRtm(ULO da, ULO regno)
        {
	  cpuSetInstructionTime(4);
	}

	/// <summary>
	/// 68030 version only.
	///
	/// Extension word: 001xxx00xxxxxxxx
	/// pflusha
	/// pflush fc, mask
	/// pflush fc, mask, ea
	///
	/// Extension word: 001000x0000xxxxx
	/// ploadr fc, ea
	/// ploadw fc, ea
	///
	/// Extension word: 010xxxxx00000000 (SRp, CRP, TC)
	/// Extension word: 011000x000000000 (MMU status register)
	/// Extension word: 000xxxxx00000000 (TT)
	/// pmove mrn, ea
	/// pmove ea, mrn
	/// pmovefd ea, mrn
	///
	/// Extension word: 100xxxxxxxxxxxxx
	/// ptestr fc, ea, #level
	/// ptestr fc, ea, #level, An
	/// ptestw fc, ea, #level
	/// ptestw fc, ea, #level, An
	///
	/// Since this is a coprocessor instruction, this is NOP.
        /// </summary>
        static void cpuPflush030(ULO ea, UWO extension)
        {
	  if (cpuGetFlagSupervisor())
	  {
	    if ((extension & 0xfde0) == 0x2000)
	    {
	      // ploadr, ploadw
	    }
	    else if ((extension & 0xe300) == 0x2000)
	    {
	      // pflusha, pflush
	      ULO mode = (extension >> 10) & 7;
	      ULO mask = (extension >> 5) & 7;
	      ULO fc = extension & 0x1f;
	    }
	  }
	  else
	  {
	    cpuPrivilegeViolation();
	  }
	  cpuSetInstructionTime(4);
	}

	/// <summary>
	/// pflusha
	/// pflush fc, mask
	/// pflush fc, mask, ea
	///
	/// 68040 version only.
	///
	/// Since this is a coprocessor instruction, this is NOP.
        /// </summary>
        static void cpuPflush040(ULO opmode, ULO regno)
        {
	  if (cpuGetFlagSupervisor())
	  {
	    if (cpu_minor != 2)	// This is NOP on 68EC040
	    {
	      switch (opmode)
	      {
		case 0: //PFLUSHN (An)
		  break;
		case 1: //PFLUSH (An)
		  break;
		case 2: //PFLUSHAN
		  break;
		case 3: //PFLUSHA
		  break;
	      }
	    }
	  }
	  else
	  {
	    cpuPrivilegeViolation();
	  }
	  cpuSetInstructionTime(4);
	}

	/// <summary>
	/// ptestr (An)
	/// ptestw (An)
	///
	/// 68040 version only.
	///
	/// Since this is a coprocessor instruction, this is NOP.
        /// </summary>
        static void cpuPtest040(ULO rw, ULO regno)
        {
	  if (cpuGetFlagSupervisor())
	  {
	    if (cpu_minor != 2)	// This is NOP on 68EC040
	    {
	      if (rw == 0)
	      {
		// ptestr
	      }
	      else 
	      {
		// ptestw
	      }
	    }
	  }
	  else
	  {
	    cpuPrivilegeViolation();
	  }
	  cpuSetInstructionTime(4);
	}

ULO cpuActivateSSP(void)
{
  ULO currentSP;
  currentSP = cpu_regs[1][7];

  // check supervisor bit number (bit 13) within the system byte of the status register
  if (!cpuGetFlagSupervisor())
  {
    // we are in user mode, thus save user stack pointer (USP)
    cpu_usp = currentSP;
    currentSP = cpu_ssp;

    if (cpu_major >= 2)
    {
      if (cpuGetFlagMaster())
      {
        currentSP = cpu_msp;
      }
    }
    cpu_regs[1][7] = currentSP;
  }
  return currentSP;
}

/*===============================================
  Sets up an exception - ebx - offset of vector
  ===============================================*/

void cpuLogException(STR *s);

void cpuPrepareException(ULO vectorOffset, ULO pcPtr, BOOLE executejmp)
{
  ULO currentSP;
  ULO readMemory;

  // DEBUG start
  char *exname;

  if (vectorOffset == 0x8)
    exname = "Exception: 2 - Access fault";
  else if (vectorOffset == 0xc)
    exname = "Exception: 3 - Address error";
  else if (vectorOffset == 0x10)
    exname = "Exception: 4 - Illegal Instruction";
  else if (vectorOffset == 0x14) exname = "Exception: 5 - Integer division by zero";
  else if (vectorOffset == 0x18) exname = "Exception: 6 - CHK, CHK2";
  else if (vectorOffset == 0x1c) exname = "Exception: 7 - FTRAPcc, TRAPcc, TRAPV";
  else if (vectorOffset == 0x20) exname = "Exception: 8 - Privilege Violation";
  else if (vectorOffset == 0x24) exname = "Exception: 9 - Trace";
  else if (vectorOffset == 0x28) exname = "Exception: 10 - A-Line";
  else if (vectorOffset == 0x2c) exname = "Exception: 11 - F-Line";
  else if (vectorOffset == 0x38) exname = "Exception: 14 - Format error";
  else if (vectorOffset >= 0x80 && vectorOffset <= 0xbc) exname = "Exception: TRAP"; 
  else exname = "Exception: Unknown";

  cpuLogException(exname);

  // DEBUG end
  
  currentSP = cpuActivateSSP();

  //generate stackframe
  cpu_stack_frame_gen[(vectorOffset & 0xfc) >> 2](vectorOffset & 0xfc, pcPtr);

  // read a memory position
  readMemory = memoryReadLong(cpuGetVbr() + vectorOffset);
  if (cpu_major < 2 && readMemory & 0x1 && vectorOffset == 0xc)
  {
    // Avoid endless loop that will crash the emulator.
    cpuReset();
    cpuHardReset();
  }
  else
  {

    cpuSetPC(readMemory);

    // set supermodus
    cpu_sr |= 0x2000; 
    cpu_sr &= 0x3fff;

    // restart cpu, if needed
    cpu_stop = FALSE;

/*  if (cpu_stop == TRUE)
  {
    cpu_stop = FALSE;
//    cpuEvent.cycle = bus_cycle + 4;
  }
*/

    cpuReadPrefetch();
    cpuSetInstructionTime(40);
  }

//  busInsertEvent(&cpuEvent);
  if (executejmp)
  {
    longjmp(cpu_exception_buffer, -1);
  }
}

/* Throws a privilege exception */

void cpuPrivilegeViolation(void)
{
  // The kickstart excpects pc in the stack frame to be the opcode PC.
  cpuPrepareException(0x20, cpuGetPC() - 2, FALSE);
}

/*========================================================================
; Group 1 Frame format
;
; 000:	All, except bus and address error
;
; Assumes esi is loaded with correct pc ptr wherever it should point
; and that SR has been updated to whatever it should contain
;========================================================================*/

void cpuFrameGroup1(UWO vector, ULO pcPtr)
{
  // save PC
  cpuSetAReg(7, cpuGetAReg(7) - 4);
  memoryWriteLong(pcPtr, cpuGetAReg(7));

  // save SR
  cpuSetAReg(7, cpuGetAReg(7) - 2);
  memoryWriteWord(cpuGetSR(), cpuGetAReg(7));
}

/*========================================================================
; Group 2 Frame format
;
; 000:	Bus and address error
;
; Assumes esi is loaded with correct pc ptr wherever it should point
; and that SR has been updated to whatever it should contain
; memory_fault_address contains the violating address
; memory_fault_read is TRUE if the access was a read
;========================================================================*/
	
void cpuFrameGroup2(UWO vector, ULO pcPtr)
{
  // save PC
  cpuSetAReg(7, cpuGetAReg(7) - 4);
  memoryWriteLong(pcPtr, cpuGetAReg(7));

  // save SR
  cpuSetAReg(7, cpuGetAReg(7) - 2);
  memoryWriteWord(cpuGetSR(), cpuGetAReg(7));

  // fault address, skip ireg
  cpuSetAReg(7, cpuGetAReg(7) - 6);
  memoryWriteLong(memory_fault_address, cpuGetAReg(7));

  cpuSetAReg(7, cpuGetAReg(7) - 2);
  memoryWriteLong(memory_fault_read << 4, cpuGetAReg(7));
}

/*========================================================================
; Frame format $0, four word frame
;
; Stack words:
; ------------
; SR
; PCHI
; PCLO
; 0000 Vector no. (4 upper bits are frame no., rest is vector no.)
;
; 010:	All, except bus and address errors
; 020:	Irq, Format error, Trap #N, Illegal inst., A-line, F-line,
;	Priv. violation, copr preinst
; 030:	Same as for 020
; 040:	Irq, Format error, Trap #N, Illegal inst., A-line, F-line,
;	Priv. violation, FPU preinst
; 060:	Irq, Format error, Trap #N, Illegal inst., A-line, F-line,
;	Priv. violation, FPU preinst, Unimpl. Integer, Unimpl. EA
;
;========================================================================
*/

void cpuFrame0(UWO vector, ULO pcPtr)
{
  // save vector word
  cpuSetAReg(7, cpuGetAReg(7) - 2);
  memoryWriteWord(vector, cpuGetAReg(7));

  // save PC
  cpuSetAReg(7, cpuGetAReg(7) - 4);
  memoryWriteLong(pcPtr, cpuGetAReg(7));

  // save SR
  cpuSetAReg(7, cpuGetAReg(7) - 2);
  memoryWriteWord(cpuGetSR(), cpuGetAReg(7));
}

/*========================================================================
; Frame format $1, 4 word throwaway frame
;
; Stack words:
; ------------
; SR
; PCHI
; PCLO
; 0000 Vector no. (4 upper bits are frame no., rest is vector no.)
;
; 020:	Irq, second frame created
; 030:	Same as for 020
; 040:	Same as for 020
;
;========================================================================*/

void cpuFrame1(UWO vector, ULO pcPtr)
{
  // save vector word
  cpuSetAReg(7, cpuGetAReg(7) - 2);
  memoryWriteWord(0x1000 | vector, cpuGetAReg(7));

  // save PC
  cpuSetAReg(7, cpuGetAReg(7) - 4);
  memoryWriteLong(pcPtr, cpuGetAReg(7));

  // save SR
  cpuSetAReg(7, cpuGetAReg(7) - 2);
  memoryWriteWord(cpuGetSR(), cpuGetAReg(7));
}

/*========================================================================
; Frame format $2
;
; 020:	chk, chk2, cpTrapcc, trapcc, trapv, trace, zero divide, MMU config,
;	copr postinst 
; 030:	Same as for 020
; 040:	chk, chk2, FTrapcc, trapcc, trapv, trace, zero divide, address error,
;	Unimplemented FPU inst. 
; 060:	Same as for 040
;
;========================================================================*/

void cpuFrame2(UWO vector, ULO pcPtr)
{
  // save inst address
  cpuSetAReg(7, cpuGetAReg(7) - 4);
  memoryWriteLong(cpuGetPC(), cpuGetAReg(7));

  // save vector word
  cpuSetAReg(7, cpuGetAReg(7) - 2);
  memoryWriteWord(0x2000 | vector, cpuGetAReg(7));

  // save PC
  cpuSetAReg(7, cpuGetAReg(7) - 4);
  memoryWriteLong(cpuGetPC(), cpuGetAReg(7));

  // save SR
  cpuSetAReg(7, cpuGetAReg(7) - 2);
  memoryWriteWord(cpuGetSR(), cpuGetAReg(7));
}

/*========================================================================
; Frame format $3
;
; 040:	FPU post inst.
; 060:	Same as for 040
;
; Fellow never generates this frame
;========================================================================*/

void cpuFrame3(UWO vector, ULO pcPtr)
{
  // save vector offset
  cpuSetAReg(7, cpuGetAReg(7) - 6);
  memoryWriteWord(0x3000 | vector, cpuGetAReg(7));

  // save PC
  cpuSetAReg(7, cpuGetAReg(7) - 4);
  memoryWriteLong(cpuGetPC(), cpuGetAReg(7));

  // save SR
  cpuSetAReg(7, cpuGetAReg(7) - 2);
  memoryWriteWord(cpuGetSR(), cpuGetAReg(7));
}

/*========================================================================
; Frame format $4
;
; 040:	Unimplemented FPU inst. on EC040 and LC040
; 060:	FPU Disabled, Bus error
;
; Not generated in Fellow
;========================================================================*/

void cpuFrame4(UWO vector, ULO pcPtr)
{
  // save vector offset
  cpuSetAReg(7, cpuGetAReg(7) - 10);
  memoryWriteWord(0x4000 | vector, cpuGetAReg(7));

  // save PC
  cpuSetAReg(7, cpuGetAReg(7) - 4);
  memoryWriteLong(cpuGetPC(), cpuGetAReg(7));

  // save SR
  cpuSetAReg(7, cpuGetAReg(7) - 2);
  memoryWriteWord(cpuGetSR(), cpuGetAReg(7));
}

/*========================================================================
; Frame format $7
;
; 040:	Address error for non-inst word accesses
;
; Not generated in Fellow
;========================================================================*/

void cpuFrame7(UWO vector, ULO pcPtr)
{
  // save vector offset
  cpuSetAReg(7, cpuGetAReg(7) - 54);
  memoryWriteWord(0x7000 | vector, cpuGetAReg(7));

  // save PC
  cpuSetAReg(7, cpuGetAReg(7) - 4);
  memoryWriteLong(cpuGetPC(), cpuGetAReg(7));

  // save SR
  cpuSetAReg(7, cpuGetAReg(7) - 2);
  memoryWriteWord(cpuGetSR(), cpuGetAReg(7));
}
	
/*========================================================================
; Frame format $8
;
; 010:	Bus and address error
;
;========================================================================*/

void cpuFrame8(UWO vector, ULO pcPtr)
{
  // save vector offset
  cpuSetAReg(7, cpuGetAReg(7) - 52);
  memoryWriteWord(0x8000 | vector, cpuGetAReg(7));

  // save PC
  cpuSetAReg(7, cpuGetAReg(7) - 4);
  memoryWriteLong(cpuGetPC(), cpuGetAReg(7));

  // save SR
  cpuSetAReg(7, cpuGetAReg(7) - 2);
  memoryWriteWord(cpuGetSR(), cpuGetAReg(7));
}

/*============================================================================
; Frame format $9
;
; 020:	copr midinst, main detected protocol violation, irq during copr inst
; 030:	Same as for 020
;
; Not generated in Fellow
;========================================================================*/

void cpuFrame9(UWO vector, ULO pcPtr)
{
  // save inst address
  cpuSetAReg(7, cpuGetAReg(7) - 12);
  memoryWriteLong(cpuGetPC(), cpuGetAReg(7));

  // save vector offset
  cpuSetAReg(7, cpuGetAReg(7) - 2);
  memoryWriteWord(0x9000 | vector, cpuGetAReg(7));

  // save PC
  cpuSetAReg(7, cpuGetAReg(7) - 4);
  memoryWriteLong(cpuGetPC(), cpuGetAReg(7));

  // save SR
  cpuSetAReg(7, cpuGetAReg(7) - 2);
  memoryWriteWord(cpuGetSR(), cpuGetAReg(7));
}

/*========================================================================
; Frame format $A
;
; 020:	Address or bus-error on instruction boundrary
; 030:	Same as for 020
;
; Will not set any values beyond the format/offset word
; Fellow will always generate this frame for bus/address errors
;========================================================================*/

void cpuFrameA(UWO vector, ULO pcPtr)
{
  // save vector offset
  cpuSetAReg(7, cpuGetAReg(7) - 26);
  memoryWriteWord(0xa000 | vector, cpuGetAReg(7));

  // save PC
  cpuSetAReg(7, cpuGetAReg(7) - 4);
  memoryWriteLong(cpuGetPC(), cpuGetAReg(7));

  // save SR
  cpuSetAReg(7, cpuGetAReg(7) - 2);
  memoryWriteWord(cpuGetSR(), cpuGetAReg(7));
}

/*========================================================================
; Frame format $B
;
; 020:	Address or bus-error mid-instruction
; 030:	Same as for 020
;
; Fellow will not generate this frame for bus/address errors
;========================================================================*/

void cpuFrameB(UWO vector, ULO pcPtr)
{
  // save vector offset
  cpuSetAReg(7, cpuGetAReg(7) - 84);
  memoryWriteWord(0xb000 | vector, cpuGetAReg(7));

  // save PC
  cpuSetAReg(7, cpuGetAReg(7) - 4);
  memoryWriteLong(cpuGetPC(), cpuGetAReg(7));

  // save SR
  cpuSetAReg(7, cpuGetAReg(7) - 2);
  memoryWriteWord(cpuGetSR(), cpuGetAReg(7));
}

/*
;============================================================
; Transfers control to an interrupt routine
;
; 1. Put SSP/MSP into A7
; 2. Generate stack frame with normal format
; 3. T1 = T0 = 0, S = 1, set IRQ level (A7 already correct)
; 4. If 020++ and not 060: Get ISP, generate throwaway frame
;    with SR from step 3
; 5. If 020++ and not 060: Clear M bit (A7 already correct)
; 6. Set new PC and fill prefetch if enabled
; 7. Restart CPU if stopped
; 8. Reschedule events
;============================================================
*/
void cpuSetUpInterrupt(void)
{
  ULO currentSP;

  irqEvent.cycle = BUS_CYCLE_DISABLE; // Scheduler already popped the event.

  currentSP = cpuActivateSSP();
  cpu_stack_frame_gen[cpuGetIrqLevel()>>2](cpuGetIrqLevel() & 0xfc, cpuGetPC());
  cpu_sr = cpu_sr & 0x38ff;
  cpu_sr = cpu_sr | 0x2000;
  cpu_sr = cpu_sr | (UWO)(cpuGetIrqLevel() << 8);
  if (cpu_major >= 2 && cpu_major < 6)
  {
    ULO oldA7 = cpuGetAReg(7);
    if (cpu_sr & 0x1000)
    {
      cpu_msp = oldA7;
      cpuSetAReg(7, cpu_ssp);
    }
    // Frame: vector | 0x1000, pc, sr
    cpuSetAReg(7, cpuGetAReg(7) - 2);
    memoryWriteWord((UWO) (cpuGetIrqLevel() | 0x1000), cpuGetAReg(7));

    cpuSetAReg(7, cpuGetAReg(7) - 4);
    memoryWriteLong(cpuGetPC(), cpuGetAReg(7));

    cpuSetAReg(7, cpuGetAReg(7) - 2);
    memoryWriteWord(cpu_sr, cpuGetAReg(7));

    cpu_sr = cpu_sr & 0xefff;
  }
  cpuSetPC(cpuGetIrqAddress());
  cpuReadPrefetch();

  if (cpu_stop)
  {
    cpuEvent.cycle = bus_cycle + 44;
    cpu_stop = 0;
//    busInsertEvent(&cpuEvent);
  }
  else
  {
    // Let the current instruction finish. Add time for instruction frame generation.
    //busRemoveEvent(&cpuEvent);
    cpuEvent.cycle += 44;
    //busInsertEvent(&cpuEvent);
  }
  cpuRaiseInterrupt();		      // Look for more interrupts.
}

/*============================*/
/* Init stack frame jmptables */
/*============================*/

void cpuStackFrameInit000(void) {
  ULO i;

  for (i = 0; i < 64; i++)
    cpu_stack_frame_gen[i] = cpuFrameGroup1;/* Avoid NULL ptrs */
  cpu_stack_frame_gen[2] = cpuFrameGroup2;  /* 2 - Bus error */
  cpu_stack_frame_gen[3] = cpuFrameGroup2;  /* 3 - Address error */
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

void cpuStackFrameInit(void)
{
  switch (cpu_major)
  {
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

/*============================*/
/* Actually a Reset exception */
/*============================*/

void cpuReset000(void) {
  cpu_sr &= 0x271f;         /* T = 0 */
  cpu_sr |= 0x2700;         /* S = 1, ilvl = 7 */
  cpuSetVbr(0);
  cpu_ssp = memoryInitialSP();        /* ssp = fake vector 0 */
  cpuSetPC(memoryInitialPC()); /* pc = fake vector 1 */
  cpuReadPrefetch();
}

void cpuReset010(void) {
  cpu_sr &= 0x271f;         /* T1T0 = 0 */
  cpu_sr |= 0x2700;         /* S = 1, ilvl = 7 */
  cpuSetVbr(0);
  cpu_ssp = memoryInitialSP();        /* ssp = fake vector 0 */
  cpuSetPC(memoryInitialPC()); /* pc = fake vector 1 */
  cpuReadPrefetch();
}

void cpuReset020(void) {
  cpu_sr &= 0x271f;         /* T1T0 = 0, M = 0 */
  cpu_sr |= 0x2700;         /* S = 1, ilvl = 7 */
  cpuSetVbr(0);
  cpuSetCacr(0);             /* E = 0, F = 0 */
                        /* Invalidate cache, we don't have one */
  cpu_ssp = memoryInitialSP();  /* ssp = fake vector 0 */
  cpuSetPC(memoryInitialPC()); /* pc = fake vector 1 */
  cpuReadPrefetch();
}

void cpuReset030(void) {
  cpu_sr &= 0x271f;         /* T1T0 = 0, M = 0 */
  cpu_sr |= 0x2700;         /* S = 1, ilvl = 7 */
  cpuSetVbr(0);
  cpuSetCacr(0);             /* E = 0, F = 0 */
                        /* Invalidate cache, we don't have one */
  cpu_ssp = memoryInitialSP();        /* ssp = fake vector 0 */
  cpuSetPC(memoryInitialPC()); /* pc = fake vector 1 */
  cpuReadPrefetch();
}

/* 040 and 060, useless right now, will remain so if it is up to me (PS) */

void cpuReset040(void) {
  cpu_sr &= 0x271f;         /* T1T0 = 0, M = 0 */
  cpu_sr |= 0x2700;         /* S = 1, ilvl = 7 */
  cpuSetVbr(0);
  cpuSetCacr(0);             /* E = 0, F = 0 */
                        /* Invalidate cache, we don't have one */
  cpu_ssp = memoryInitialSP();        /* ssp = fake vector 0 */
  cpuSetPC(memoryInitialPC()); /* pc = fake vector 1 */
  cpuReadPrefetch();
}

void cpuReset060(void) {
  cpu_sr &= 0x271f;         /* T = 0 */
  cpu_sr |= 0x2700;         /* S = 1, ilvl = 7 */
  cpuSetVbr(0);
  cpuSetCacr(0);             /* cacr = 0 */
                        /* DTTn[E-bit] = 0 */
                        /* ITTn[E-bit] = 0 */
                        /* TCR = 0 */
                        /* BUSCR = 0 */
                        /* PCR = 0 */
                        /* FPU Data regs = NAN */
                        /* FPU Control regs = 0 */
  cpu_ssp = memoryInitialSP();        /* ssp = fake vector 0 */
  cpuSetPC(memoryInitialPC()); /* pc = fake vector 1 */
  cpuReadPrefetch();
}

/*=====================================*/
/* cpuReset performs a Reset exception */
/*=====================================*/

void cpuHardReset(void) {
  cpu_stop = FALSE;
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
  }
}

#include "cpudecl.h"
#include "cpudata.h"
#include "cpuprofile.h"
#include "cpucode.h"

extern FILE *BUSLOG;
BOOLE cpu_log = FALSE;

void cpuEventLog(bus_event *e, UWO opcode)
{
  char saddress[256], sdata[256], sinstruction[256], soperands[256];
  if (!cpu_log) return;
  if (BUSLOG == NULL) BUSLOG = fopen("buslog.txt", "w");

  saddress[0] = '\0';
  sdata[0] = '\0';
  sinstruction[0] = '\0';
  soperands[0] = '\0';
  cpuDisOpcode(cpuGetPC()-2, saddress, sdata, sinstruction, soperands);
  fprintf(BUSLOG, "%.5d %.8X %s %s\t%s\t%s\n", e->cycle, cpuGetAReg(7), saddress, sdata, sinstruction, soperands);
}

UWO cpu_current_opcode;
ULO cpu_original_pc;
extern ULO draw_frame_count;

void cpuLogException(STR *s)
{
  if (!cpu_log) return;
  if (BUSLOG == NULL) BUSLOG = fopen("buslog.txt", "w");
  fprintf(BUSLOG, "%.5d: %s for opcode %.4X at PC %.8X from PC %.8X\n", cpuEvent.cycle, s, cpu_current_opcode, cpu_original_pc, cpuGetPC());
}

ULO cpuExecuteInstruction(void)
{
  UWO oldSr = cpu_sr;
  UWO opcode;
  
  cpu_original_pc = cpuGetPC(); // For logging
  opcode = cpu_current_opcode = cpuGetNextOpcode16();

  if (cpu_original_pc == 0xC08F4A)// && bus_cycle == 65736)
  {
    int hjkhj = 0;
  }
  cpuEventLog(&cpuEvent, opcode);

  cpu_instruction_time = 0;
  if (cpu_opcode_model_mask[opcode] & cpu_model_mask)
  {
    cpu_opcode_data[opcode].instruction_func(cpu_opcode_data[opcode].data);
  }
  else
  {
    cpuIllegalInstruction(cpu_opcode_data[opcode].data);
  }
  if (oldSr & 0xc000)
  {
    // This instruction was trapped
    ULO cycles = cpu_instruction_time;
    cpuPrepareException(0x24, cpuGetPC(), FALSE);
    cpu_instruction_time += cycles;
  }
  return cpu_instruction_time;
}

