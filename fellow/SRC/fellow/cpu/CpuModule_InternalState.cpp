/*=========================================================================*/
/* Fellow                                                                  */
/* 68000 internal state                                                    */
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
#include "fellow/api/defs.h"
#include "CpuModule.h"
#include "CpuModule_Memory.h"
#include "CpuModule_Internal.h"

/* M68k registers */
static ULO cpu_regs[2][8]; /* 0 - data, 1 - address */
static ULO cpu_pc;
static ULO cpu_usp;
static ULO cpu_ssp;
static ULO cpu_msp;
static ULO cpu_sfc;
static ULO cpu_dfc;
ULO cpu_sr; // Not static because flags calculation use it extensively
static ULO cpu_vbr;
static UWO cpu_prefetch_word;
static ULO cpu_cacr;
static ULO cpu_caar;

/* Irq management */
static BOOLE cpu_raise_irq;
static ULO cpu_raise_irq_level;

/* Reset values */
static ULO cpu_initial_pc;
static ULO cpu_initial_sp;

/* Flag set if CPU is stopped */
static BOOLE cpu_stop;

/* The current CPU model */
static ULO cpu_model_major = -1;
static ULO cpu_model_minor;
static UBY cpu_model_mask;

/* For exception handling */
#ifdef CPU_INSTRUCTION_LOGGING

static UWO cpu_current_opcode;

#endif

static ULO cpu_original_pc;

/* Number of cycles taken by the last intstruction */
static ULO cpu_instruction_time;

/* Getters and setters */

void cpuSetDReg(ULO i, ULO value)
{
  cpu_regs[0][i] = value;
}
ULO cpuGetDReg(ULO i)
{
  return cpu_regs[0][i];
}

void cpuSetAReg(ULO i, ULO value)
{
  cpu_regs[1][i] = value;
}
ULO cpuGetAReg(ULO i)
{
  return cpu_regs[1][i];
}

void cpuSetReg(ULO da, ULO i, ULO value)
{
  cpu_regs[da][i] = value;
}
ULO cpuGetReg(ULO da, ULO i)
{
  return cpu_regs[da][i];
}

/// <summary>
/// Get the supervisor bit from sr.
/// </summary>
BOOLE cpuGetFlagSupervisor()
{
  return cpu_sr & 0x2000;
}

/// <summary>
/// Get the master/irq state bit from sr.
/// </summary>
BOOLE cpuGetFlagMaster()
{
  return cpu_sr & 0x1000;
}

void cpuSetUspDirect(ULO usp)
{
  cpu_usp = usp;
}
ULO cpuGetUspDirect()
{
  return cpu_usp;
}
ULO cpuGetUspAutoMap()
{
  return (cpuGetFlagSupervisor()) ? cpuGetUspDirect() : cpuGetAReg(7);
}

void cpuSetSspDirect(ULO ssp)
{
  cpu_ssp = ssp;
}
ULO cpuGetSspDirect()
{
  return cpu_ssp;
}
ULO cpuGetSspAutoMap()
{
  return (cpuGetFlagSupervisor()) ? cpuGetAReg(7) : cpuGetSspDirect();
}

void cpuSetMspDirect(ULO msp)
{
  cpu_msp = msp;
}
ULO cpuGetMspDirect()
{
  return cpu_msp;
}

/// <summary>
/// Returns the master stack pointer.
/// </summary>
ULO cpuGetMspAutoMap()
{
  if (cpuGetFlagSupervisor() && cpuGetFlagMaster())
  {
    return cpuGetAReg(7);
  }
  return cpuGetMspDirect();
}

/// <summary>
/// Sets the master stack pointer.
/// </summary>
void cpuSetMspAutoMap(ULO new_msp)
{
  if (cpuGetFlagSupervisor() && cpuGetFlagMaster())
  {
    cpuSetAReg(7, new_msp);
  }
  else
  {
    cpuSetMspDirect(new_msp);
  }
}

/// <summary>
/// Returns the interrupt stack pointer. ssp is used as isp.
/// </summary>
ULO cpuGetIspAutoMap()
{
  if (cpuGetFlagSupervisor() && !cpuGetFlagMaster())
  {
    return cpuGetAReg(7);
  }
  return cpuGetSspDirect();
}

/// <summary>
/// Sets the interrupt stack pointer. ssp is used as isp.
/// </summary>
void cpuSetIspAutoMap(ULO new_isp)
{
  if (cpuGetFlagSupervisor() && !cpuGetFlagMaster())
  {
    cpuSetAReg(7, new_isp);
  }
  else
  {
    cpuSetSspDirect(new_isp);
  }
}

void cpuSetPC(ULO address)
{
  cpu_pc = address;
}
ULO cpuGetPC()
{
  return cpu_pc;
}

void cpuSetStop(BOOLE stop)
{
  cpu_stop = stop;
}
BOOLE cpuGetStop()
{
  return cpu_stop;
}

void cpuSetVbr(ULO vbr)
{
  cpu_vbr = vbr;
}
ULO cpuGetVbr()
{
  return cpu_vbr;
}

void cpuSetSfc(ULO sfc)
{
  cpu_sfc = sfc;
}
ULO cpuGetSfc()
{
  return cpu_sfc;
}

void cpuSetDfc(ULO dfc)
{
  cpu_dfc = dfc;
}
ULO cpuGetDfc()
{
  return cpu_dfc;
}

void cpuSetCacr(ULO cacr)
{
  cpu_cacr = cacr;
}
ULO cpuGetCacr()
{
  return cpu_cacr;
}

void cpuSetCaar(ULO caar)
{
  cpu_caar = caar;
}
ULO cpuGetCaar()
{
  return cpu_caar;
}

void cpuSetSR(ULO sr)
{
  cpu_sr = sr;
}
ULO cpuGetSR()
{
  return cpu_sr;
}

void cpuSetInstructionTime(ULO cycles)
{
  cpu_instruction_time = cycles;
}
ULO cpuGetInstructionTime()
{
  return cpu_instruction_time;
}

void cpuSetOriginalPC(ULO pc)
{
  cpu_original_pc = pc;
}
ULO cpuGetOriginalPC()
{
  return cpu_original_pc;
}

#ifdef CPU_INSTRUCTION_LOGGING

void cpuSetCurrentOpcode(UWO opcode)
{
  cpu_current_opcode = opcode;
}
UWO cpuGetCurrentOpcode()
{
  return cpu_current_opcode;
}

#endif

void cpuSetRaiseInterrupt(BOOLE raise_irq)
{
  cpu_raise_irq = raise_irq;
}
BOOLE cpuGetRaiseInterrupt()
{
  return cpu_raise_irq;
}
void cpuSetRaiseInterruptLevel(ULO raise_irq_level)
{
  cpu_raise_irq_level = raise_irq_level;
}
ULO cpuGetRaiseInterruptLevel()
{
  return cpu_raise_irq_level;
}

ULO cpuGetIrqLevel()
{
  return (cpu_sr & 0x0700) >> 8;
}

void cpuSetInitialPC(ULO pc)
{
  cpu_initial_pc = pc;
}
ULO cpuGetInitialPC()
{
  return cpu_initial_pc;
}

void cpuSetInitialSP(ULO sp)
{
  cpu_initial_sp = sp;
}
ULO cpuGetInitialSP()
{
  return cpu_initial_sp;
}

void cpuSetModelMask(UBY model_mask)
{
  cpu_model_mask = model_mask;
}
UBY cpuGetModelMask()
{
  return cpu_model_mask;
}

ULO cpuGetModelMajor()
{
  return cpu_model_major;
}
ULO cpuGetModelMinor()
{
  return cpu_model_minor;
}

static void cpuCalculateModelMask()
{
  switch (cpuGetModelMajor())
  {
    case 0: cpuSetModelMask(0x01); break;
    case 1: cpuSetModelMask(0x02); break;
    case 2: cpuSetModelMask(0x04); break;
    case 3: cpuSetModelMask(0x08); break;
  }
}

void cpuSetModel(ULO major, ULO minor)
{
  BOOLE makeOpcodeTable = (cpu_model_major != major);
  cpu_model_major = major;
  cpu_model_minor = minor;
  cpuCalculateModelMask();
  cpuStackFrameInit();
  if (makeOpcodeTable) cpuMakeOpcodeTableForModel();
}

void cpuSetDRegWord(ULO regno, UWO val)
{
  *((WOR *)&cpu_regs[0][regno]) = val;
}
void cpuSetDRegByte(ULO regno, UBY val)
{
  *((UBY *)&cpu_regs[0][regno]) = val;
}
UWO cpuGetRegWord(ULO i, ULO regno)
{
  return (UWO)cpu_regs[i][regno];
}

UWO cpuGetDRegWord(ULO regno)
{
  return (UWO)cpu_regs[0][regno];
}
UBY cpuGetDRegByte(ULO regno)
{
  return (UBY)cpu_regs[0][regno];
}

ULO cpuGetDRegWordSignExtLong(ULO regno)
{
  return cpuSignExtWordToLong(cpuGetDRegWord(regno));
}
UWO cpuGetDRegByteSignExtWord(ULO regno)
{
  return cpuSignExtByteToWord(cpuGetDRegByte(regno));
}
ULO cpuGetDRegByteSignExtLong(ULO regno)
{
  return cpuSignExtByteToLong(cpuGetDRegByte(regno));
}

UWO cpuGetARegWord(ULO regno)
{
  return (UWO)cpu_regs[1][regno];
}
UBY cpuGetARegByte(ULO regno)
{
  return (UBY)cpu_regs[1][regno];
}

typedef UWO (*cpuGetWordFunc)();
typedef ULO (*cpuGetLongFunc)();

static UWO cpuGetNextWordInternal()
{
  UWO data = memoryReadWord(cpuGetPC() + 2);
  return data;
}

static ULO cpuGetNextLongInternal()
{
  ULO data = memoryReadLong(cpuGetPC() + 2);
  return data;
}

UWO cpuGetNextWord()
{
  UWO tmp = cpu_prefetch_word;
  cpu_prefetch_word = cpuGetNextWordInternal();
  cpuSetPC(cpuGetPC() + 2);
  return tmp;
}

ULO cpuGetNextWordSignExt()
{
  return cpuSignExtWordToLong(cpuGetNextWord());
}

ULO cpuGetNextLong()
{
  ULO tmp = cpu_prefetch_word << 16;
  ULO data = cpuGetNextLongInternal();
  cpu_prefetch_word = (UWO)data;
  cpuSetPC(cpuGetPC() + 4);
  return tmp | (data >> 16);
}

void cpuInitializePrefetch()
{
  cpu_prefetch_word = memoryReadWord(cpuGetPC());
}

void cpuClearPrefetch()
{
  cpu_prefetch_word = 0;
}

void cpuSkipNextWord()
{
  cpuSetPC(cpuGetPC() + 2);
  cpuInitializePrefetch();
}

void cpuSkipNextLong()
{
  cpuSetPC(cpuGetPC() + 4);
  cpuInitializePrefetch();
}

void cpuInitializeFromNewPC(ULO new_pc)
{
  cpuSetPC(new_pc);
  cpuInitializePrefetch();
}
