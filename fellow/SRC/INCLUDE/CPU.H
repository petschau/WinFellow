#ifndef CPU_H
#define CPU_H

/*===========================================================================*/
/* CPU properties                                                            */
/*===========================================================================*/

typedef enum {
  M68000  = 0,
  M68010  = 1,
  M68020  = 2,
  M68030  = 3,
  M68EC30 = 4,
  M68EC20 = 9
} cpu_types;

extern cpu_types cpu_type;

/* Major is x in 680X0, minor is: 0 - full, 1 - EC */

extern ULO cpu_major, cpu_minor;
extern ULO cpu_speed;

extern ULO cpu_usp;
extern ULO cpu_ssp;
extern ULO cpu_msp;

/* Configuration settings */
extern BOOLE cpuSetType(cpu_types type);
extern cpu_types cpuGetType(void);
extern void cpuSetSpeed(ULO speed);
extern ULO cpuGetSpeed(void);

extern void cpuEventHandler(void);
extern void cpuStackFrameInit(void);
extern ULO cpuExecuteInstruction(void);

extern ULO cpu_chip_cycles;
extern ULO cpu_chip_slowdown;
extern ULO cpu_instruction_time;

extern void cpuSetPC(ULO address);
extern ULO cpuGetPC(void);
extern void cpuSetDReg(ULO i, ULO value);
extern ULO cpuGetDReg(ULO i);
extern void cpuSetAReg(ULO i, ULO value);
extern ULO cpuGetAReg(ULO i);
extern void cpuSetReg(ULO da, ULO i, ULO value);
extern ULO cpuGetReg(ULO da, ULO i);
extern void cpuSetStop(BOOLE stop);
extern BOOLE cpuGetStop(void);
extern void cpuSetSR(UWO sr);
extern UWO cpuGetSR(void);
extern void cpuSetVbr(ULO vbr);
extern ULO cpuGetVbr(void);
extern void cpuSetIrqLevel(ULO irq_level);
extern ULO cpuGetIrqLevel(void);
extern void cpuSetIrqAddress(ULO irq_address);
extern ULO cpuGetIrqAddress(void);
extern void cpuSetModelMask(UBY model_mask);

extern void cpuReadPrefetch(void);

extern void cpuReset010(void);
extern void cpuReset020(void);
extern void cpuReset030(void);
extern void cpuInit000(void);
extern void cpuInit010(void);
extern void cpuInit020(void);
extern void cpuInit030(void);

extern void cpuRaiseInterrupt(void);
extern void cpuSetUpInterrupt(void);
extern void cpuPrepareException(ULO vectorOffset, ULO pcPtr, BOOLE executejmp);
extern void cpuPrivilegeViolation(void);
extern void cpuUpdateSr(UWO new_sr);
extern ULO cpuDisOpcode(ULO disasm_pc, STR *saddress, STR *sdata, STR *sinstruction, STR *soperands);

extern void cpuHardReset(void);
extern void cpuEmulationStart(void);
extern void cpuEmulationStop(void);
extern void cpuStartup(void);
extern void cpuShutdown(void);

extern void cpuEventHandler(void);

#endif
