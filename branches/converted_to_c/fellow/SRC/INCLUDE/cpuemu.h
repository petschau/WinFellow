#ifndef CPU_EMU
#define CPU_EMU

/* Interface to the cpu-emulation module. (Generic side) */

/* Prototype for a function that is called when the cpu-emulation needs */
/* to reevaluate pending interrupts. Such as when the current irq level */
/* has changed. */
ULO (*cpuReevaluateIrqFunction)(ULO current_irq_level);

extern void cpuSetIrqLevel(ULO irq_level);
extern ULO cpuGetIrqLevel(void);

extern ULO cpuDisOpcode(ULO disasm_pc, STR *saddress, STR *sdata, STR *sinstruction, STR *soperands);
extern ULO cpuExecuteInstruction(void);
extern void cpuReset(BOOLE hard);
extern void cpuInit(ULO cpu_major, ULO cpu_minor, cpuReevaluateIrqFunction reevaluateIrqFunction);

#endif