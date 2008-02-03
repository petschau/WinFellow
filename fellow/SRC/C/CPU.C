/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Initialization of 68000 core                                               */
/* Integrates the 68k emulation with custom chips			      */
/*                                                                            */
/* Authors: Petter Schau (peschau@online.no)                                  */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

#include "defs.h"
#include "fellow.h"
#include "fmem.h"
#include "cpu.h"
#include "bus.h"

ULO cpu_major;
ULO cpu_minor;
BOOLE cpu_opcode_table_is_invalid;

/* custom chip intreq bit-number to irq-level */
ULO cpu_int_to_level[16] = {1,1,1,2, 3,3,3,4, 4,4,4,5, 5,6,6,7};

/*============================================================================*/
/* profiling help functions                                                   */
/*============================================================================*/

static __inline cpuTscBefore(LLO* a)
{
  LLO local_a = *a;
  __asm 
  {
    push    eax
    push    edx
    push    ecx
    mov     ecx,10h
    rdtsc
    pop     ecx
    mov     dword ptr [local_a], eax
    mov     dword ptr [local_a + 4], edx
    pop     edx
    pop     eax
  }
  *a = local_a;
}

static __inline cpuTscAfter(LLO* a, LLO* b, ULO* c)
{
  LLO local_a = *a;
  LLO local_b = *b;
  ULO local_c = *c;

  __asm 
  {
    push    eax
    push    edx
    push    ecx
    mov     ecx, 10h
    rdtsc
    pop     ecx
    sub     eax, dword ptr [local_a]
    sbb     edx, dword ptr [local_a + 4]
    add     dword ptr [local_b], eax
    adc     dword ptr [local_b + 4], edx
    inc     dword ptr [local_c]
    pop     edx
    pop     eax
  }
  *a = local_a;
  *b = local_b;
  *c = local_c;
}
 
/* Cycles spent by chips (Blitter) as a result of an instruction */

ULO cpu_chip_cycles;

/*===========================================================================*/
/* CPU properties                                                            */
/*===========================================================================*/

cpu_types cpu_type;
ULO cpu_speed;
ULO cpu_speed_property; /* cpu_speed used RT    */


/*====================*/
/* Extended registers */
/*====================*/

/* ssp is used for isp */

ULO cpu_sfc, cpu_dfc;  /* X12346 */
ULO cpu_iacr0, cpu_iacr1, cpu_dacr0, cpu_dacr1; /* 68EC40 */
ULO cpu_pcr, cpu_buscr;                 /* XXXXX6 */

/*===========================================================================*/
/* CPU properties                                                            */
/*===========================================================================*/

BOOLE cpuSetType(cpu_types type)
{
  BOOLE needreset = (cpu_type != type);
  cpu_type = type;
  switch (type) {
    case M68000: cpu_major = 0; cpu_minor = 0; break;
    case M68010: cpu_major = 1; cpu_minor = 0; break;
    case M68020: cpu_major = 2; cpu_minor = 0; break;
    case M68030: cpu_major = 3; cpu_minor = 0; break;
    case M68EC20: cpu_major = 2; cpu_minor = 1; break;
    case M68EC30: cpu_major = 3; cpu_minor = 1; break;  
  }
  cpu_opcode_table_is_invalid |= needreset;
  return needreset;
}

cpu_types cpuGetType(void)
{
  return cpu_type;
}

void cpuSetSpeed(ULO speed)
{
  cpu_speed_property = speed;
}

ULO cpuGetSpeed(void)
{
  return cpu_speed_property;
}

/*==================================================================*/
/* cpuInit is called whenever the opcode tables needs to be changed */
/* 1. At emulator startup or                                        */
/* 2. Cpu type has changed                                          */
/*==================================================================*/

void cpuInit()
{
  ULO i, j;

  for (j = 0; j < 2; j++)
    for (i = 0; i < 8; i++)
      cpuSetReg(j, i, 0);

  cpuStackFrameInit();
  cpu_opcode_table_is_invalid = FALSE;
}

void cpuEmulationStart(void)
{
  ULO multiplier = 12;

  switch (cpu_major)
  {
    case 0:
      multiplier = 12;
      cpuSetModelMask(0x01);
      break;
    case 1:
      multiplier = 12;
      cpuSetModelMask(0x02);
      break;
    case 2:
      multiplier = 11;
      cpuSetModelMask(0x04);
      break;
    case 3:
      multiplier = 11;
      cpuSetModelMask(0x08);
      break;
  }

  if (cpuGetSpeed() >= 8) cpu_speed = multiplier;
  else if (cpuGetSpeed() >= 4) cpu_speed = multiplier - 1;
  else if (cpuGetSpeed() >= 2) cpu_speed = multiplier - 2;
  else if (cpuGetSpeed() >= 1) cpu_speed = multiplier - 3;
  else cpu_speed = multiplier - 4;

  if (cpu_opcode_table_is_invalid) cpuInit();
}

void cpuEmulationStop(void)
{
}

void cpuStartup(void)
{
  cpuSetType(M68000);
  cpu_opcode_table_is_invalid = TRUE;
  cpuSetStop(FALSE);
}

void cpuShutdown(void)
{
}

/*
;=====================================================
; No parameters, just checking for waiting interrupts
;
; First, check which irq that should be generated
; Then check if it already has been scheduled.
; If not, then schedule it
;=====================================================
*/

void cpuRaiseInterrupt(void)
{
  ULO current_cpu_level = (cpuGetSR() >> 8) & 7;
  BOOLE chip_irqs_enabled = !!(intena & 0x4000);

  if (irqEvent.cycle != BUS_CYCLE_DISABLE)
  {
    // Let this irq go through.
    return;
  }
  if (chip_irqs_enabled)
  {
    ULO chip_irqs = intreq & intena;
    LON highest_chip_irq = 13;
    
    for (highest_chip_irq = 13; highest_chip_irq >= 0; highest_chip_irq--)
    {
      if (chip_irqs & (1 << highest_chip_irq))
      {
	// Found a chip-irq that is both flagged and enabled.
	ULO highest_chip_level = cpu_int_to_level[highest_chip_irq];
	if (highest_chip_level > current_cpu_level)
	{
	  cpuSetIrqLevel(highest_chip_level);
	  cpuSetIrqAddress(memoryReadLong(cpuGetVbr() + 0x60 + highest_chip_level*4));
	  irqEvent.cycle = bus_cycle + 1;
	  busInsertEvent(&irqEvent);
	  break;
	}
      }    
    }
  }
}

/*
;=========================================================================
; Takes new SR in edx and maintains the integrity of the super/user state
;=========================================================================
*/

void cpuUpdateSr(UWO new_sr)
{
  BOOLE trace_was_set = !!(cpuGetSR() & 0xc000);
  BOOLE supermode_was_set = !!(cpuGetSR() & 0x2000);
  BOOLE master_was_set = (cpu_major >= 2) && !!(cpuGetSR() & 0x1000);

  BOOLE trace_is_set = !!(new_sr & 0xc000);
  BOOLE supermode_is_set = !!(new_sr & 0x2000);
  BOOLE master_is_set = (cpu_major >= 2) && !!(new_sr & 0x1000);

  ULO runlevel_old = (cpuGetSR() >> 8) & 7;
  ULO runlevel_new = (new_sr >> 8) & 7;
  
  if (!supermode_was_set) cpu_usp = cpuGetAReg(7);
  else if (master_was_set) cpu_msp = cpuGetAReg(7);
  else cpu_ssp = cpuGetAReg(7);

  if (!supermode_is_set) cpuSetAReg(7, cpu_usp);
  else if (master_is_set) cpuSetAReg(7, cpu_msp);
  else cpuSetAReg(7, cpu_ssp);

  cpuSetSR(new_sr);

  if (runlevel_old != runlevel_new)
  {
    cpuRaiseInterrupt();
  }
}

void cpuEventHandler(void)
{
  ULO cycles = 0;
  ULO time_used = 0;
  do
  {
    cycles = cpuExecuteInstruction();
    if (cpu_major > 1) cycles = 4;
    time_used += (cpu_chip_cycles<<12) + (cycles<<cpu_speed);
  }
  while (time_used < 8192 && !cpuGetStop());

  if (cpuGetStop()) cpuEvent.cycle = BUS_CYCLE_DISABLE;
  else  
  {
    cpuEvent.cycle = bus_cycle + (time_used>>12);
  }
  cpu_chip_cycles = 0;
}
