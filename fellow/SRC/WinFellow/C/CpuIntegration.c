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

#include "Defs.h"
#include "FellowMain.h"
#include "MemoryInterface.h"
#include "CpuModule.h"
#include "CpuIntegration.h"
#include "CpuModule_Internal.h"
#include "BusScheduler.h"
#include "interrupt.h"

#include "VirtualHost/Core.h"

jmp_buf cpu_integration_exception_buffer;
uint32_t cpu_integration_chip_interrupt_number;

/* Cycles spent by chips (Blitter) as a result of an instruction */
static uint32_t cpu_integration_chip_cycles;
static uint32_t cpu_integration_chip_slowdown;

/*===========================================================================*/
/* CPU properties                                                            */
/*===========================================================================*/

uint32_t cpu_integration_speed;               // The speed as expressed in the fellow configuration settings
uint32_t cpu_integration_speed_multiplier;    // The cycle multiplier used to adjust the cpu-speed, calculated from cpu_integration_speed
cpu_integration_models cpu_integration_model; // The cpu model as expressed in the fellow configuration settings

/*===========================================================================*/
/* CPU properties                                                            */
/*===========================================================================*/

void cpuIntegrationSetSpeed(uint32_t speed)
{
  cpu_integration_speed = speed;
}

uint32_t cpuIntegrationGetSpeed()
{
  return cpu_integration_speed;
}

static void cpuIntegrationSetSpeedMultiplier(uint32_t multiplier)
{
  cpu_integration_speed_multiplier = multiplier;
}

static uint32_t cpuIntegrationGetSpeedMultiplier()
{
  return cpu_integration_speed_multiplier;
}

void cpuIntegrationCalculateMultiplier()
{
  uint32_t multiplier = 12;

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

BOOLE cpuIntegrationSetModel(cpu_integration_models model)
{
  BOOLE needreset = (cpu_integration_model != model);
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

void cpuIntegrationSetChipCycles(uint32_t chip_cycles)
{
  cpu_integration_chip_cycles = chip_cycles;
}

uint32_t cpuIntegrationGetChipCycles()
{
  return cpu_integration_chip_cycles;
}

void cpuIntegrationSetChipSlowdown(uint32_t chip_slowdown)
{
  cpu_integration_chip_slowdown = chip_slowdown;
}

uint32_t cpuIntegrationGetChipSlowdown()
{
  return cpu_integration_chip_slowdown;
}

void cpuIntegrationSetChipInterruptNumber(uint32_t chip_interrupt_number)
{
  cpu_integration_chip_interrupt_number = chip_interrupt_number;
}

uint32_t cpuIntegrationGetChipInterruptNumber()
{
  return cpu_integration_chip_interrupt_number;
}

// A wrapper for cpuSetIrqLevel that restarts the
// scheduling of cpu events if the cpu was stoppped
void cpuIntegrationSetIrqLevel(uint32_t new_interrupt_level, uint32_t chip_interrupt_number)
{
  if (cpuSetIrqLevel(new_interrupt_level))
  {
    cpuEvent.cycle = busGetCycle();
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
    _core.Fileops->GetGenericFileName(filename, "WinFellow", "cpuinstructions.log");
    CPUINSTRUCTIONLOG = fopen(filename, "w");
  }
}

void cpuIntegrationPrintBusCycle()
{
  fprintf(CPUINSTRUCTIONLOG, "%I64u:%.5u ", bus.frame_no, bus.cycle);
}

void cpuIntegrationInstructionLogging()
{
  char saddress[256], sdata[256], sinstruction[256], soperands[256];

  if (cpu_disable_instruction_log) return;
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

void cpuIntegrationExceptionLogging(const char *description, uint32_t original_pc, uint16_t opcode)
{
  if (cpu_disable_instruction_log) return;
  cpuInstructionLogOpen();

  cpuIntegrationPrintBusCycle();
  fprintf(CPUINSTRUCTIONLOG, "%s for opcode %.4X at PC %.8X from PC %.8X\n", description, opcode, original_pc, cpuGetPC());
}

void cpuIntegrationInterruptLogging(uint32_t level, uint32_t vector_address)
{
  if (cpu_disable_instruction_log) return;
  cpuInstructionLogOpen();

  cpuIntegrationPrintBusCycle();
  fprintf(CPUINSTRUCTIONLOG, "Irq %u to %.6X (%s)\n", level, vector_address, interruptGetInterruptName(cpuIntegrationGetChipInterruptNumber()));
}

#endif

void cpuIntegrationExecuteInstructionEventHandler68000Fast()
{
  uint32_t cycles = cpuExecuteInstruction();

  if (cpuGetStop())
  {
    cpuEvent.cycle = BUS_CYCLE_DISABLE;
  }
  else
  {
    cpuEvent.cycle += ((cycles * cpuIntegrationGetChipSlowdown()) >> 1) + cpuIntegrationGetChipCycles();
  }
  cpuIntegrationSetChipCycles(0);
}

void cpuIntegrationExecuteInstructionEventHandler68000General()
{
  uint32_t cycles = 0;
  uint32_t time_used = 0;

  do
  {
    cycles = cpuExecuteInstruction();
    cycles = cycles * cpuIntegrationGetChipSlowdown(); // Compensate for blitter time
    time_used += (cpuIntegrationGetChipCycles() << 12) + (cycles << cpuIntegrationGetSpeedMultiplier());
  } while (time_used < 8192 && !cpuGetStop());

  if (cpuGetStop())
  {
    cpuEvent.cycle = BUS_CYCLE_DISABLE;
  }
  else
  {
    cpuEvent.cycle += (time_used >> 12);
  }
  cpuIntegrationSetChipCycles(0);
}

void cpuIntegrationExecuteInstructionEventHandler68020()
{
  uint32_t time_used = 0;
  do
  {
    cpuExecuteInstruction();
    time_used += (cpuIntegrationGetChipCycles() << 12) + (4 << cpuIntegrationGetSpeedMultiplier());
  } while (time_used < 8192 && !cpuGetStop());

  if (cpuGetStop())
  {
    cpuEvent.cycle = BUS_CYCLE_DISABLE;
  }
  else
  {
    cpuEvent.cycle += (time_used >> 12);
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

void cpuIntegrationSaveState(FILE *F)
{
  cpuSaveState(F);

  fwrite(&cpu_integration_chip_slowdown, sizeof(cpu_integration_chip_slowdown), 1, F);
  // Everything else is configuration options which will be set when the associated config-file is loaded.
}

void cpuIntegrationLoadState(FILE *F)
{
  cpuLoadState(F);

  fread(&cpu_integration_chip_slowdown, sizeof(cpu_integration_chip_slowdown), 1, F);
  // Everything else is configuration options which will be set when the associated config-file is loaded.
}

void cpuIntegrationEmulationStart()
{
  cpuIntegrationCalculateMultiplier();
}

void cpuIntegrationEmulationStop()
{
}

void cpuIntegrationHardReset()
{
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
