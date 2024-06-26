/*=========================================================================*/
/* Fellow                                                                  */
/* CPU 68k effective address calculation functions                         */
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

#include "Defs.h"
#include "CpuModule_Memory.h"

#include "CpuModule.h"
#include "CpuModule_Internal.h"

/* Calculates EA for (Ax). */
uint32_t cpuEA02(uint32_t regno)
{
  return cpuGetAReg(regno);
}

/* Calculates EA for (Ax)+ */
uint32_t cpuEA03(uint32_t regno, uint32_t size)
{
  uint32_t tmp = cpuGetAReg(regno);
  if (regno == 7 && size == 1) size++;
  cpuSetAReg(regno, tmp + size);
  return tmp;
}

/* Calculates EA for -(Ax) */
uint32_t cpuEA04(uint32_t regno, uint32_t size)
{
  if (regno == 7 && size == 1) size++;
  cpuSetAReg(regno, cpuGetAReg(regno) - size);
  return cpuGetAReg(regno);
}

/* Calculates EA for disp16(Ax) */
uint32_t cpuEA05(uint32_t regno)
{
  return cpuGetAReg(regno) + cpuGetNextWordSignExt();
}

/* Calculates EA for disp8(Ax,Ri.size) with 68020 extended modes. */
static uint32_t cpuEA06Ext(uint16_t ext, uint32_t base_reg_value, uint32_t index_value)
{
  uint32_t base_displacement;
  uint32_t outer_displacement;
  BOOLE index_register_suppressed = (ext & 0x0040);
  BOOLE base_register_suppressed = (ext & 0x0080);
  uint32_t base_displacement_size = (ext >> 4) & 3;
  uint32_t memory_indirect_action = (ext & 7);

  if (memory_indirect_action == 4 || (memory_indirect_action > 4 && index_register_suppressed))
  {
    cpuThrowIllegalInstructionException(TRUE); /* Illegal instruction */
                                               // Never returns
  }

  if (index_register_suppressed)
  {
    index_value = 0;
  }

  if (base_register_suppressed)
  {
    base_reg_value = 0;
  }

  switch (base_displacement_size)
  {
    case 0:                                      /* Reserved */
      cpuThrowIllegalInstructionException(TRUE); /* Illegal instruction */
      break;
    case 1: /* Null base displacement */ base_displacement = 0; break;
    case 2: /* Word base displacement */ base_displacement = cpuGetNextWordSignExt(); break;
    case 3: /* Long base displacement */ base_displacement = cpuGetNextLong(); break;
  }

  switch (memory_indirect_action)
  {
    case 0: /* No memory indirect action */ return base_reg_value + base_displacement + index_value;
    case 1: /* Indirect preindexed with null outer displacement */ return memoryReadLong(base_reg_value + base_displacement + index_value);
    case 2: /* Indirect preindexed with word outer displacement */
      outer_displacement = cpuGetNextWordSignExt();
      return memoryReadLong(base_reg_value + base_displacement + index_value) + outer_displacement;
    case 3: /* Indirect preindexed with long outer displacement */
      outer_displacement = cpuGetNextLong();
      return memoryReadLong(base_reg_value + base_displacement + index_value) + outer_displacement;
    case 5: /* Indirect postindexed with null outer displacement, reserved for index register suppressed */
      return memoryReadLong(base_reg_value + base_displacement) + index_value;
    case 6: /* Indirect postindexed with word outer displacement, reserved for index register suppressed */
      outer_displacement = cpuGetNextWordSignExt();
      return memoryReadLong(base_reg_value + base_displacement) + index_value + outer_displacement;
    case 7: /* Indirect postindexed with long outer displacement, reserved for index register suppressed */
      outer_displacement = cpuGetNextLong();
      return memoryReadLong(base_reg_value + base_displacement) + index_value + outer_displacement;
  }
  return 0; /* Should never come here. */
}

/* Calculates EA for disp8(Ax,Ri.size), calls cpuEA06Ext() for 68020 extended modes. */
uint32_t cpuEA06(uint32_t regno)
{
  uint32_t reg_value = cpuGetAReg(regno);
  uint16_t ext = cpuGetNextWord();
  uint32_t index_value = cpuGetReg(ext >> 15, (ext >> 12) & 7);
  if (!(ext & 0x0800))
  {
    index_value = cpuSignExtWordToLong((uint16_t)index_value);
  }
  if (cpuGetModelMajor() >= 2)
  {
    index_value = index_value << ((ext >> 9) & 3); /* Scaling index value */
    if (ext & 0x0100)                              /* Full extension word */
    {
      return cpuEA06Ext(ext, reg_value, index_value);
    }
  }
  return reg_value + index_value + cpuSignExtByteToLong((uint8_t)ext);
}

/* Calculates EA for xxxx.W */
uint32_t cpuEA70()
{
  return cpuGetNextWordSignExt();
}

/* Calculates EA for xxxxxxxx.L */
uint32_t cpuEA71()
{
  return cpuGetNextLong();
}

/// <summary>
/// Calculates EA for disp16(PC)
/// </summary>
/// <returns>Address</returns>
uint32_t cpuEA72()
{
  uint32_t pc_tmp = cpuGetPC();
  return pc_tmp + cpuGetNextWordSignExt();
}

/// <summary>
/// Calculates EA for disp8(PC,Ri.size). Calls cpuEA06Ext() to calculate extended 68020 modes.
/// </summary>
/// <returns>Address</returns>
uint32_t cpuEA73()
{
  uint32_t reg_value = cpuGetPC();
  uint16_t ext = cpuGetNextWord();
  uint32_t index_value = cpuGetReg(ext >> 15, (ext >> 12) & 0x7);
  if (!(ext & 0x0800))
  {
    index_value = cpuSignExtWordToLong((uint16_t)index_value);
  }
  if (cpuGetModelMajor() >= 2)
  {
    index_value = index_value << ((ext >> 9) & 0x3); // Scaling index value
    if (ext & 0x0100)                                // Full extension word
    {
      return cpuEA06Ext(ext, reg_value, index_value);
    }
  }
  return reg_value + index_value + cpuSignExtByteToLong((uint8_t)ext);
}
