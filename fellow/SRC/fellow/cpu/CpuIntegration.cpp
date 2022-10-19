/*=========================================================================*/
/* Fellow                                                                  */
/* Initialization of 68000 core                                            */
/* Integrates the 68k emulation with custom chips			   */
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
#include "FELLOW.H"
#include "FMEM.H"
#include "CpuModule.h"
#include "CpuIntegration.h"
#include "CpuModule_Internal.h"
#include "fellow/scheduler/Scheduler.h"
#include "fellow/api/Services.h"
#include "interrupt.h"

using namespace fellow::api;

jmp_buf cpu_integration_exception_buffer;
ULO cpu_integration_chip_interrupt_number;

/* Cycles spent by chips (Blitter) as a result of an instruction */
static ULO cpu_integration_chip_cycles;
static ULO cpu_integration_chip_slowdown;

/*===========================================================================*/
/* CPU properties                                                            */
/*===========================================================================*/

ULO cpu_integration_speed;            // The speed as expressed in the fellow configuration settings
ULO cpu_integration_speed_multiplier; // The cycle multiplier used to adjust the cpu-speed, calculated from cpu_integration_speed
ULO cpu_time_used_remainder;
cpu_integration_models cpu_integration_model; // The cpu model as expressed in the fellow configuration settings

/*===========================================================================*/
/* CPU properties                                                            */
/*===========================================================================*/

void cpuIntegrationSetSpeed(ULO speed)
{
  cpu_integration_speed = speed;
}

ULO cpuIntegrationGetSpeed()
{
  return cpu_integration_speed;
}

ULO cpuIntegrationGetTimeUsedRemainder()
{
  return cpu_time_used_remainder;
}

static void cpuIntegrationSetSpeedMultiplier(ULO multiplier)
{
  cpu_integration_speed_multiplier = multiplier;
}

static ULO cpuIntegrationGetSpeedMultiplier()
{
  return cpu_integration_speed_multiplier;
}

void cpuIntegrationCalculateMultiplier()
{
  ULO multiplier = 12;

  switch (cpuGetModelMajor())
  {
    case 0: multiplier = 12; break;
    case 1: multiplier = 12; break;
    case 2: multiplier = 11; break;
    case 3: multiplier = 11; break;
  }

  if (cpuIntegrationGetSpeed() >= 8)
    cpuIntegrationSetSpeedMultiplier(multiplier);
  else if (cpuIntegrationGetSpeed() >= 4)
    cpuIntegrationSetSpeedMultiplier(multiplier - 1);
  else if (cpuIntegrationGetSpeed() >= 2)
    cpuIntegrationSetSpeedMultiplier(multiplier - 2);
  else if (cpuIntegrationGetSpeed() >= 1)
    cpuIntegrationSetSpeedMultiplier(multiplier - 3);
  else
    cpuIntegrationSetSpeedMultiplier(multiplier - 4);
}

bool cpuIntegrationSetModel(cpu_integration_models model)
{
  bool needreset = (cpu_integration_model != model);
  cpu_integration_model = model;

  switch (cpu_integration_model)
  {
    case cpu_integration_models::M68000: cpuSetModel(0, 0); break;
    case cpu_integration_models::M68010: cpuSetModel(1, 0); break;
    case cpu_integration_models::M68020: cpuSetModel(2, 0); break;
    case cpu_integration_models::M68030: cpuSetModel(3, 0); break;
    case cpu_integration_models::M68EC20: cpuSetModel(2, 1); break;
    case cpu_integration_models::M68EC30: cpuSetModel(3, 1); break;
  }
  return needreset;
}

cpu_integration_models cpuIntegrationGetModel()
{
  return cpu_integration_model;
}

void cpuIntegrationSetChipCycles(ULO chip_cycles)
{
  cpu_integration_chip_cycles = chip_cycles;
}

ULO cpuIntegrationGetChipCycles()
{
  return cpu_integration_chip_cycles;
}

void cpuIntegrationSetChipSlowdown(ULO chip_slowdown)
{
  cpu_integration_chip_slowdown = chip_slowdown;
}

ULO cpuIntegrationGetChipSlowdown()
{
  return cpu_integration_chip_slowdown;
}

void cpuIntegrationSetChipInterruptNumber(ULO chip_interrupt_number)
{
  cpu_integration_chip_interrupt_number = chip_interrupt_number;
}

ULO cpuIntegrationGetChipInterruptNumber()
{
  return cpu_integration_chip_interrupt_number;
}

// A wrapper for cpuSetIrqLevel that restarts the
// scheduling of cpu events if the cpu was stoppped
void cpuIntegrationSetIrqLevel(ULO new_interrupt_level, ULO chip_interrupt_number)
{
  if (cpuSetIrqLevel(new_interrupt_level))
  {
    cpuEvent.cycle = scheduler.GetFrameCycle();
  }
  cpuIntegrationSetChipInterruptNumber(chip_interrupt_number);
}

/*=========================================*/
/* Exception mid-instruction exit function */
/*=========================================*/

void cpuIntegrationMidInstructionExceptionFunc()
{
  longjmp(cpu_integration_exception_buffer, -1);
}

/*===================================================*/
/* Handles reset exception event from the cpu-module */
/*===================================================*/

void cpuIntegrationResetExceptionFunc()
{
  fellowSoftReset();
}

/*=========*/
/* Logging */
/*=========*/

#ifdef CPU_INSTRUCTION_LOGGING

FILE *CPUINSTRUCTIONLOG;
int cpu_disable_instruction_log = TRUE;

void cpuInstructionLogOpen()
{
  if (CPUINSTRUCTIONLOG == nullptr)
  {
    char filename[MAX_PATH];
    Service->Fileops.GetGenericFileName(filename, "WinFellow", "cpuinstructions.log");
    CPUINSTRUCTIONLOG = fopen(filename, "w");
  }
}

void cpuIntegrationPrintBusCycle()
{
  fprintf(CPUINSTRUCTIONLOG, "%I64u:%.5u ", scheduler.GetRasterFrameCount(), scheduler.GetFrameCycle());
}

void cpuIntegrationInstructionLogging()
{
  if (cpu_disable_instruction_log) return;

  char saddress[256], sdata[256], sinstruction[256], soperands[256];
  cpuInstructionLogOpen();
  /*
  fprintf(CPUINSTRUCTIONLOG,
      "D0:%.8X D1:%.8X D2:%.8X D3:%.8X D4:%.8X D5:%.8X D6:%.8X D7:%.8X\n",
      cpuGetDReg(0),
      cpuGetDReg(1),
      cpuGetDReg(2),
      cpuGetDReg(3),
      cpuGetDReg(4),
      cpuGetDReg(5),
      cpuGetDReg(6),
      cpuGetDReg(7));

  fprintf(CPUINSTRUCTIONLOG,
      "A0:%.8X A1:%.8X A2:%.8X A3:%.8X A4:%.8X A5:%.8X A6:%.8X A7:%.8X\n",
      cpuGetAReg(0),
      cpuGetAReg(1),
      cpuGetAReg(2),
      cpuGetAReg(3),
      cpuGetAReg(4),
      cpuGetAReg(5),
      cpuGetAReg(6),
      cpuGetAReg(7));
      */
  saddress[0] = '\0';
  sdata[0] = '\0';
  sinstruction[0] = '\0';
  soperands[0] = '\0';
  cpuDisOpcode(cpuGetPC(), saddress, sdata, sinstruction, soperands);
  fprintf(CPUINSTRUCTIONLOG, "SSP:%.6X USP:%.6X SP:%.4X %s %s\t%s\t%s\n", cpuGetSspDirect(), cpuGetUspDirect(), cpuGetSR(), saddress, sdata, sinstruction, soperands);
}

void cpuIntegrationExceptionLogging(const STR *description, ULO original_pc, UWO opcode)
{
  if (cpu_disable_instruction_log) return;
  cpuInstructionLogOpen();

  cpuIntegrationPrintBusCycle();
  fprintf(CPUINSTRUCTIONLOG, "%s for opcode %.4X at PC %.8X from PC %.8X\n", description, opcode, original_pc, cpuGetPC());
}

void cpuIntegrationInterruptLogging(ULO level, ULO vector_address)
{
  if (cpu_disable_instruction_log) return;
  cpuInstructionLogOpen();

  cpuIntegrationPrintBusCycle();
  fprintf(CPUINSTRUCTIONLOG, "Irq %u to %.6X (%s)\n", level, vector_address, interruptGetInterruptName(cpuIntegrationGetChipInterruptNumber()));
}

#endif

// This handler is hardcoded for regular 7 MHz CPU speed
void cpuIntegrationExecuteInstructionEventHandler68000Fast()
{
  ULO cycles = cpuExecuteInstruction();

  if (cpuGetStop())
  {
    cpuEvent.Disable();
  }
  else
  {
    cpuEvent.cycle += scheduler.GetCycleFromCycle280ns(((cycles * cpuIntegrationGetChipSlowdown()) >> 1) + cpuIntegrationGetChipCycles());
  }
  cpuIntegrationSetChipCycles(0);
}

// This handler keeps track of CPU time at 4096 times the resolution of the bus in order to run the CPU at higher than normal speeds
// set by the speed multiplier
void cpuIntegrationExecuteInstructionEventHandler68000General()
{
  ULO time_used = cpu_time_used_remainder;

  ULO cycles = cpuExecuteInstruction();
  cycles = cycles * cpuIntegrationGetChipSlowdown(); // Compensate for blitter time
  time_used += (cpuIntegrationGetChipCycles() << 12) + (cycles << cpuIntegrationGetSpeedMultiplier());

  if (cpuGetStop())
  {
    cpuEvent.Disable();
    cpu_time_used_remainder = 0;
  }
  else
  {
    cpuEvent.cycle += (time_used >> 10); // Actually >> 12 then times 4
    cpu_time_used_remainder = time_used & 0x3ff;
  }

  cpuIntegrationSetChipCycles(0);
}

// This handler keeps track of CPU time at 4096 times the resolution of the bus in order to run the CPU at higher than normal speeds
// set by the speed multiplier
// It simplifies CPU instruction time by executing all instructions at the same speed.
void cpuIntegrationExecuteInstructionEventHandler68020()
{
  ULO time_used = cpu_time_used_remainder;
  cpuExecuteInstruction();
  time_used += (cpuIntegrationGetChipCycles() << 12) + (4 << cpuIntegrationGetSpeedMultiplier());

  if (cpuGetStop())
  {
    cpuEvent.Disable();
    cpu_time_used_remainder = 0;
  }
  else
  {
    cpuEvent.cycle += (time_used >> 10); // Actually >> 12 then times 4
    cpu_time_used_remainder = time_used & 0x3ff;
  }
  cpuIntegrationSetChipCycles(0);
}

void cpuIntegrationSetDefaultConfig()
{
  cpuIntegrationSetModel(cpu_integration_models::M68000);
  cpuIntegrationSetChipCycles(0);
  cpuIntegrationSetChipSlowdown(1);
  cpuIntegrationSetSpeed(4);

  cpuSetCheckPendingInterruptsFunc(interruptRaisePending);
  cpuSetMidInstructionExceptionFunc(cpuIntegrationMidInstructionExceptionFunc);
  cpuSetResetExceptionFunc(cpuIntegrationResetExceptionFunc);

#ifdef CPU_INSTRUCTION_LOGGING
  cpuSetInstructionLoggingFunc(cpuIntegrationInstructionLogging);
  cpuSetExceptionLoggingFunc(cpuIntegrationExceptionLogging);
  cpuSetInterruptLoggingFunc(cpuIntegrationInterruptLogging);
#endif
}

/*=========================*/
/* Fellow lifecycle events */
/*=========================*/

void cpuIntegrationEmulationStart()
{
  cpuIntegrationCalculateMultiplier();
}

void cpuIntegrationEmulationStop()
{
}

void cpuIntegrationHardReset()
{
  cpu_time_used_remainder = 0;
  cpuIntegrationSetChipCycles(0);
  cpuIntegrationSetChipSlowdown(1);
  cpuSetInitialPC(memoryInitialPC());
  cpuSetInitialSP(memoryInitialSP());
  cpuHardReset();
}

void cpuIntegrationStartup()
{
  cpuStartup();
  cpuIntegrationSetDefaultConfig();
  cpuCreateMulTimeTables();
}

void cpuIntegrationShutdown()
{
  cpuProfileWrite();
}
