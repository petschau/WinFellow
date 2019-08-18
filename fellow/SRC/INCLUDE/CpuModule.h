#pragma once

// This header file defines the internal interfaces of the CPU module.

#ifdef _DEBUG
#define CPU_INSTRUCTION_LOGGING
#endif

// Function to check if there are any external interrupt sources wanting to issue interrupts
typedef void (*cpuCheckPendingInterruptsFunc)();
extern void cpuSetCheckPendingInterruptsFunc(cpuCheckPendingInterruptsFunc func);
extern void cpuCheckPendingInterrupts();
extern void cpuSetUpInterrupt(ULO new_interrupt_level);
extern void cpuInitializeFromNewPC(ULO new_pc);

// Logging interface
#ifdef CPU_INSTRUCTION_LOGGING

typedef void (*cpuInstructionLoggingFunc)();
extern void cpuSetInstructionLoggingFunc(cpuInstructionLoggingFunc func);
typedef void (*cpuExceptionLoggingFunc)(const STR *description, ULO original_pc, UWO opcode);
extern void cpuSetExceptionLoggingFunc(cpuExceptionLoggingFunc func);
typedef void (*cpuInterruptLoggingFunc)(ULO level, ULO vector_address);
extern void cpuSetInterruptLoggingFunc(cpuInterruptLoggingFunc func);

#endif

// CPU register and control properties
extern void cpuSetPC(ULO pc);
extern ULO cpuGetPC();

extern void cpuSetReg(ULO da, ULO i, ULO value);
extern ULO cpuGetReg(ULO da, ULO i);

extern void cpuSetDReg(ULO i, ULO value);
extern ULO cpuGetDReg(ULO i);

extern void cpuSetAReg(ULO i, ULO value);
extern ULO cpuGetAReg(ULO i);

extern void cpuSetSR(ULO sr);
extern ULO cpuGetSR();

extern void cpuSetUspDirect(ULO usp);
extern ULO cpuGetUspDirect();
extern ULO cpuGetUspAutoMap();

extern void cpuSetMspDirect(ULO msp);
extern ULO cpuGetMspDirect();

extern void cpuSetSspDirect(ULO ssp);
extern ULO cpuGetSspDirect();
extern ULO cpuGetSspAutoMap();

extern ULO cpuGetVbr();

extern void cpuSetStop(BOOLE stop);
extern BOOLE cpuGetStop();

extern void cpuSetInitialPC(ULO pc);
extern ULO cpuGetInitialPC();

extern void cpuSetInitialSP(ULO sp);
extern ULO cpuGetInitialSP();

extern ULO cpuGetInstructionTime();

extern BOOLE cpuSetIrqLevel(ULO irq_level);
extern ULO cpuGetIrqLevel();

extern ULO cpuExecuteInstruction();
extern ULO cpuDisOpcode(ULO disasm_pc, STR *saddress, STR *sdata, STR *sinstruction, STR *soperands);

extern void cpuHardReset();
extern void cpuStartup();

typedef void (*cpuMidInstructionExceptionFunc)();
extern void cpuSetMidInstructionExceptionFunc(cpuMidInstructionExceptionFunc func);
extern void cpuThrowAddressErrorException();

typedef void (*cpuResetExceptionFunc)();
extern void cpuSetResetExceptionFunc(cpuResetExceptionFunc func);

// Configuration settings
extern void cpuSetModel(ULO major, ULO minor);
extern ULO cpuGetModelMajor();
extern ULO cpuGetModelMinor();
