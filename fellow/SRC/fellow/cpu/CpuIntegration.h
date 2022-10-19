#pragma once

enum class cpu_integration_models
{
  M68000 = 0,
  M68010 = 1,
  M68020 = 2,
  M68030 = 3,
  M68EC30 = 4,
  M68EC20 = 9
};

extern void cpuIntegrationCalculateMultiplier();

extern void cpuIntegrationExecuteInstructionEventHandler68000Fast();
extern void cpuIntegrationExecuteInstructionEventHandler68000General();
extern void cpuIntegrationExecuteInstructionEventHandler68020();

extern bool cpuIntegrationSetModel(cpu_integration_models model);
extern cpu_integration_models cpuIntegrationGetModel();

void cpuIntegrationSetIrqLevel(ULO new_interrupt_level, ULO chip_interrupt_number);
extern void cpuIntegrationSetSpeed(ULO speed);
extern ULO cpuIntegrationGetSpeed();
extern void cpuIntegrationSetChipCycles(ULO chip_cycles);
extern ULO cpuIntegrationGetChipCycles();
extern void cpuIntegrationSetChipSlowdown(ULO chip_slowdown);
extern ULO cpuIntegrationGetChipSlowdown();
extern ULO cpuIntegrationGetTimeUsedRemainder();

extern jmp_buf cpu_integration_exception_buffer;

// Fellow limecycle events
extern void cpuIntegrationEmulationStart();
extern void cpuIntegrationEmulationStop();
extern void cpuIntegrationHardReset();
extern void cpuIntegrationStartup();
extern void cpuIntegrationShutdown();
