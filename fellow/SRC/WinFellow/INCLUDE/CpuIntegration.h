#pragma once

#include <cstdint>
#include "Defs.h"
#include "Scheduler/ChipTimeOffset.h"

enum class cpu_integration_models
{
  M68000 = 0,
  M68010 = 1,
  M68020 = 2,
  M68030 = 3,
  M68EC30 = 4,
  M68EC20 = 9
};

extern void cpuIntegrationExecuteInstructionEventHandler68000();
extern void cpuIntegrationExecuteInstructionEventHandler68020();

extern void cpuIntegrationCalculateMasterCycleMultiplier();
extern uint32_t cpuIntegrationGetMasterCycleMultiplier();

extern BOOLE cpuIntegrationSetModel(cpu_integration_models model);
extern cpu_integration_models cpuIntegrationGetModel();

void cpuIntegrationSetIrqLevel(uint32_t new_interrupt_level, uint32_t chip_interrupt_number);
extern void cpuIntegrationSetSpeed(uint32_t speed);
extern uint32_t cpuIntegrationGetSpeed();
extern void cpuIntegrationSetChipCycles(ChipTimeOffset chip_cycles);
extern ChipTimeOffset cpuIntegrationGetChipCycles();
extern void cpuIntegrationSetChipSlowdown(uint32_t chip_slowdown);
extern uint32_t cpuIntegrationGetChipSlowdown();

extern jmp_buf cpu_integration_exception_buffer;

extern void cpuIntegrationEmulationStart();
extern void cpuIntegrationEmulationStop();
extern void cpuIntegrationHardReset();
extern void cpuIntegrationStartup();
extern void cpuIntegrationShutdown();
