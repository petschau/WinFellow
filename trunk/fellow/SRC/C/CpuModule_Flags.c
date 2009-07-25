/* @(#) $Id: CpuModule_Flags.c,v 1.2 2009-07-25 09:40:59 peschau Exp $ */
/*=========================================================================*/
/* Fellow                                                                  */
/* 68000 flag and condition code handling                                  */
/*                                                                         */
/* Author: Petter Schau                                                    */
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
#include "CpuModule_Internal.h"

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
/// Set the X and C flags to the value f.
/// </summary>
/// <param name="f">The new state of the flags.</param>        
void cpuSetFlagXC(BOOLE f)
{
  cpu_sr = (cpu_sr & 0xffee) | ((f) ? 0x11 : 0);
}

/// <summary>
/// Set the C flag to the value f.
/// </summary>
/// <param name="f">The new state of the flag.</param>        
void cpuSetFlagC(BOOLE f)
{
  cpu_sr = (cpu_sr & 0xfffe) | ((f) ? 1 : 0);
}

/// <summary>
/// Set the V flag.
/// </summary>
/// <param name="f">The new state of the flag.</param>        
void cpuSetFlagV(BOOLE f)
{
  cpu_sr = (cpu_sr & 0xfffd) | ((f) ? 2 : 0);
}

/// <summary>
/// Clear the V flag.
/// </summary>
static void cpuClearFlagV(void)
{
  cpu_sr = cpu_sr & 0xfffd;
}

/// <summary>
/// Get the V flag.
/// </summary>
BOOLE cpuGetFlagV(void)
{
  return cpu_sr & 0x2;
}

/// <summary>
/// Set the N flag.
/// </summary>
/// <param name="f">The new state of the flag.</param>        
void cpuSetFlagN(BOOLE f)
{
  cpu_sr = (cpu_sr & 0xfff7) | ((f) ? 8 : 0);
}

/// <summary>
/// Set the Z flag.
/// </summary>
/// <param name="f">The new state of the flag.</param>        
void cpuSetFlagZ(BOOLE f)
{
  cpu_sr = (cpu_sr & 0xfffb) | ((f) ? 4 : 0);
}

/// <summary>
/// Clear the Z flag.
/// </summary>
static void cpuClearFlagZ(void)
{
  cpu_sr = cpu_sr & 0xfffb;
}

/// <summary>
/// Get the Z flag.
/// </summary>
static BOOLE cpuGetFlagZ(void)
{
  return cpu_sr & 0x4;
}

/// <summary>
/// Get the X flag.
/// </summary>
BOOLE cpuGetFlagX(void)
{
  return cpu_sr & 0x10;
}

/// <summary>
/// Set the flags.
/// </summary>
void cpuSetFlags0100(void)
{
  cpu_sr = (cpu_sr & 0xfff0) | 4;
}

/// <summary>
/// Clear V and C.
/// </summary>
static void cpuClearFlagsVC(void)
{
  cpu_sr = cpu_sr & 0xfffc;
}

UWO cpuGetZFlagB(UBY res) {return (UWO)((res) ? 0 : 4);}
UWO cpuGetZFlagW(UWO res) {return (UWO)((res) ? 0 : 4);}
UWO cpuGetZFlagL(ULO res) {return (UWO)((res) ? 0 : 4);}

UWO cpuGetNFlagB(UBY res) {return (UWO)((res & 0x80) >> 4);}
UWO cpuGetNFlagW(UWO res) {return (UWO)((res & 0x8000) >> 12);}
UWO cpuGetNFlagL(ULO res) {return (UWO)((res & 0x80000000) >> 28);}

/// <summary>
/// Set the flags NZVC.
/// </summary>
/// <param name="z">The Z flag.</param>        
/// <param name="n">The N flag.</param>        
/// <param name="v">The V flag.</param>        
/// <param name="c">The C flag.</param>        
void cpuSetFlagsNZVC(BOOLE z, BOOLE n, BOOLE v, BOOLE c)
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
void cpuSetFlagsVC(BOOLE v, BOOLE c)
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
void cpuSetFlagsAdd(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm)
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
void cpuSetFlagsSub(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm)
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
void cpuSetFlagsAddX(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm)
{
  if (!z) cpuClearFlagZ();
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
void cpuSetFlagsSubX(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm)
{
  if (!z) cpuClearFlagZ();
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
void cpuSetFlagsNeg(BOOLE z, BOOLE rm, BOOLE dm)
{
  cpuSetFlagZ(z);
  cpuSetFlagN(rm);
  cpuSetFlagXC(!z);
  cpuSetFlagV(dm && rm);
}

/// <summary>
/// Set the flags (all) of a negx operation.
/// </summary>
/// <param name="z">The Z flag.</param>        
/// <param name="rm">The MSB of the result.</param>        
/// <param name="dm">The MSB of the destination source.</param>        
void cpuSetFlagsNegx(BOOLE z, BOOLE rm, BOOLE dm)
{
  if (!z) cpuClearFlagZ();
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
void cpuSetFlagsCmp(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm)
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
void cpuSetFlagsShiftZero(BOOLE z, BOOLE rm)
{
  cpuSetFlagZ(z);
  cpuSetFlagN(rm);
  cpuClearFlagsVC();
}

/// <summary>
/// Set the flags of a shift operation.
/// </summary>
/// <param name="z">The Z flag.</param>        
/// <param name="rm">The MSB of the result.</param>        
/// <param name="c">The carry of the result.</param>        
/// <param name="c">The overflow of the result.</param>        
void cpuSetFlagsShift(BOOLE z, BOOLE rm, BOOLE c, BOOLE v)
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
void cpuSetFlagsRotate(BOOLE z, BOOLE rm, BOOLE c)
{
  cpuSetFlagZ(z);
  cpuSetFlagN(rm);
  cpuSetFlagC(c);
  cpuClearFlagV();
}

/// <summary>
/// Set the flags of a rotate with x operation.
/// </summary>
/// <param name="z">The Z flag.</param>        
/// <param name="rm">The MSB of the result.</param>        
/// <param name="c">The extend bit and carry of the result.</param>        
void cpuSetFlagsRotateX(UWO z, UWO rm, UWO x)
{
  cpu_sr = (cpu_sr & 0xffe0) | z | rm | x;
}

/// <summary>
/// Set the flags (ZN00).
/// </summary>
/// <param name="z">The Z flag.</param>        
/// <param name="rm">The MSB of the result.</param>        
void cpuSetFlagsNZ00(BOOLE z, BOOLE rm)
{
  cpuSetFlagZ(z);
  cpuSetFlagN(rm);
  cpuClearFlagsVC();
}

/// <summary>
/// Set the 4 flags absolute.
/// </summary>
/// <param name="f">flags</param>        
void cpuSetFlagsAbs(UWO f)
{
  cpu_sr = (cpu_sr & 0xfff0) | f;
}

/// <summary>
/// Calculates the values for the condition codes.
/// </summary>
/// <returns>TRUE or FALSE</returns>
BOOLE cpuCalculateConditionCode0(void)
{
  return TRUE;
}

BOOLE cpuCalculateConditionCode1(void)
{
  return FALSE;
}

BOOLE cpuCalculateConditionCode2(void)
{
  return !(cpu_sr & 5);     // HI - !C && !Z
}

BOOLE cpuCalculateConditionCode3(void)
{
  return cpu_sr & 5;	      // LS - C || Z
}

BOOLE cpuCalculateConditionCode4(void)
{
  return (~cpu_sr) & 1;	      // CC - !C
}

BOOLE cpuCalculateConditionCode5(void)
{
  return cpu_sr & 1;	      // CS - C
}

BOOLE cpuCalculateConditionCode6(void)
{
  return (~cpu_sr) & 4;	      // NE - !Z
}

BOOLE cpuCalculateConditionCode7(void)
{
  return cpu_sr & 4;	      // EQ - Z
}

BOOLE cpuCalculateConditionCode8(void)
{
  return (~cpu_sr) & 2;	      // VC - !V
}

BOOLE cpuCalculateConditionCode9(void)
{
  return cpu_sr & 2;	      // VS - V
}

BOOLE cpuCalculateConditionCode10(void)
{
  return (~cpu_sr) & 8;      // PL - !N
}

BOOLE cpuCalculateConditionCode11(void)
{
  return cpu_sr & 8;	      // MI - N
}

BOOLE cpuCalculateConditionCode12(void)
{
  UWO tmp = cpu_sr & 0xa;
  return (tmp == 0xa) || (tmp == 0);  // GE - (N && V) || (!N && !V)
}

BOOLE cpuCalculateConditionCode13(void)
{
  UWO tmp = cpu_sr & 0xa;
  return (tmp == 0x8) || (tmp == 0x2);	// LT - (N && !V) || (!N && V)
}

BOOLE cpuCalculateConditionCode14(void)
{
  UWO tmp = cpu_sr & 0xa;
  return (!(cpu_sr & 0x4)) && ((tmp == 0xa) || (tmp == 0)); // GT - (N && V && !Z) || (!N && !V && !Z) 
}

BOOLE cpuCalculateConditionCode15(void)
{
  UWO tmp = cpu_sr & 0xa;
  return (cpu_sr & 0x4) || (tmp == 0x8) || (tmp == 2);// LE - Z || (N && !V) || (!N && V)
}

BOOLE cpuCalculateConditionCode(ULO cc)
{
  switch (cc & 0xf)
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
  return FALSE;
}
