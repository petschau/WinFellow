/* @(#) $Id: interrupt.c,v 1.16 2012-12-07 14:05:43 carfesh Exp $ */
/*=========================================================================*/
/* WinFellow                                                               */
/* Chipset side of interrupt control                                       */
/* Author: Petter Schau                                                    */
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
#include "defs.h"
#include "BUS.H"
#include "FMEM.H"
#include "CIA.H"
#include "CpuModule.h"
#include "CpuIntegration.h"


#include "fileops.h"

/* The process of servicing interrupts is asynchronous in several steps

  Case 1: Chipset requests an interrupt, or program sets intreq or intena.

  1. interruptRaisePending() is called to evaluate the current requested and enabled 
     interrupts.

  2. If one is found, to emulate the chipset latency before actually sending the desired interrupt level
     to the CPU, the interrupt event is used (bus.c), scheduled to fire some cycles from now.

  3. The interrupt event fires, calls interruptHandleEvent() which will set the new
     interrupt level in the cpu using cpuSetIrqLevel(). The rest in the hands of the
     cpu module.
  
  (4.) cpuSetIrqLevel() will set an internal flag, record the new interrupt level, 
       and unstop the CPU if needed. CPU state is not changed here.

  (5.) The next time cpuExecuteIntruction runs, it will switch to the new interrupt level
       and make the necessary state changes.

  Case 2: CPU lowers the interrupt level (RTE, write to SR etc).

  1. The CPU module calls the checkPendingInterrupt hook to force us to re-evaluate the interrupt sources.
     This hook points to interruptRaisePending(). Continues as Case 1.

*/

UWO intena;
UWO intreq;
ULO interrupt_pending_cpu_level;
ULO interrupt_pending_chip_interrupt_number;
static unsigned int interrupt_cpu_level[16] = {1,1,1,2, 3,3,3,4, 4,4,4,5, 5,6,6,7};


STR *interruptGetInterruptName(ULO interrupt_number)
{
  switch (interrupt_number)
  {
    case 0: return "TBE: Output buffer of the serial port is empty.";
    case 1: return "DSKBLK: Disk DMA transfer ended.";
    case 2: return "SOFT: Software interrupt.";
    case 3: return "PORTS: From CIA-A or expansion port.";
    case 4: return "COPER: Copper interrupt.";
    case 5: return "VERTB: Start of vertical blank.";
    case 6: return "BLIT: Blitter done.";
    case 7: return "AUD0: Audio data on channel 0.";
    case 8: return "AUD1: Audio data on channel 1.";
    case 9: return "AUD2: Audio data on channel 2.";
    case 10: return "AUD3: Audio data on channel 3.";
    case 11: return "RBF: Input buffer of the serial port full.";
    case 12: return "DSKSYN: Disk sync value recognized.";
    case 13: return "EXTER: From CIA-B or expansion port.";
    case 14: return "INTEN: BUG! Not an interrupt.";
    case 15: return "NMI: BUG! Not an interrupt.";
  }
  return "Illegal interrupt source!";
}

void interruptSetPendingChipInterruptNumber(ULO pending_chip_interrupt_number)
{
  interrupt_pending_chip_interrupt_number = pending_chip_interrupt_number;
}

ULO interruptGetPendingChipInterruptNumber(void)
{
  return interrupt_pending_chip_interrupt_number;
}

void interruptSetPendingCpuLevel(ULO pending_cpu_level)
{
  interrupt_pending_cpu_level = pending_cpu_level;
}

ULO interruptGetPendingCpuLevel(void)
{
  return interrupt_pending_cpu_level;
}

unsigned int interruptGetScheduleLatency(void)
{
  return 10;
}

unsigned int interruptGetCpuLevel(int interrupt_number)
{
  return interrupt_cpu_level[interrupt_number];
}

bool interruptIsPending(int interrupt_number, unsigned int pending_interrupts)
{
  return (pending_interrupts & (1 << interrupt_number)) != 0;
}

bool interruptMasterSwitchIsEnabled(void)
{
  return !!(intena & 0x4000);
}

bool interruptHasSetModeBit(UWO interrupt_bitmask)
{
  return !!(interrupt_bitmask & 0x8000);
}

UWO interruptGetPendingBitMask(void)
{
  return intena & intreq;
}

UWO interruptSetBits(UWO original, UWO set_bitmask)
{
  return original | (set_bitmask & 0x7fff);
}

UWO interruptClearBits(UWO original, UWO clear_bitmask)
{
  return original & ~(clear_bitmask & 0x7fff);
}

BOOLE interruptIsRequested(UWO bitmask)
{
  return !!(intreq & bitmask);
}

/*=====================================================
  Set the interrupt in the CPU
  =====================================================*/
void interruptHandleEvent(void)
{
  interruptEvent.cycle = BUS_CYCLE_DISABLE;
  cpuIntegrationSetIrqLevel(interruptGetPendingCpuLevel(), interruptGetPendingChipInterruptNumber());
}

extern BOOLE cpuGetRaiseInterrupt(void);

/*=====================================================
  Raise any waiting interrupts
  =====================================================*/
void interruptRaisePending(void)
{
  if (!interruptMasterSwitchIsEnabled())
  {
    return;
  }

  UWO pending_chip_interrupts = interruptGetPendingBitMask();
  if (pending_chip_interrupts == 0)
  {
    return;
  }

  if (interruptEvent.cycle != BUS_CYCLE_DISABLE || cpuGetRaiseInterrupt())
  {
    // Interrupt already scheduled, wait for it to finish being set up
    return;
  }

  ULO current_cpu_level = cpuGetIrqLevel();
  for (int interrupt_number = 13; interrupt_number >= 0; interrupt_number--)
  {
    if (interruptIsPending(interrupt_number, pending_chip_interrupts))
    {
      // Found a chip-irq that is both flagged and enabled.
      unsigned int interrupt_level = interruptGetCpuLevel(interrupt_number);
      if (interrupt_level > current_cpu_level)
      {
	interruptSetPendingChipInterruptNumber(interrupt_number);
	interruptSetPendingCpuLevel(interrupt_level);
	interruptEvent.cycle = busGetCycle() + interruptGetScheduleLatency();
	busInsertEvent(&interruptEvent);
	return;
      }
      else
      {
	// When we get here, we are below the current cpu level, skip the rest
	return;
      }
    }
  }
}

/*
======
INTREQ
======

$dff09c (Write) / $dff01e (Read)

Paula
*/

UWO rintreqr(ULO address)
{
  return intreq;
}

void wintreq(UWO data, ULO address)
{
  if (interruptHasSetModeBit(data))
  {
    intreq = interruptSetBits(intreq, data);
  }
  else
  {
    intreq = interruptClearBits(intreq, data);
    ciaUpdateIRQ(0);
    ciaUpdateIRQ(1);
  }
  interruptRaisePending();
}

/*
======
INTENA
======

$dff09a (Write) / $dff01c (Read)

Paula
*/

UWO rintenar(ULO address)
{
  return intena;
}

// If master bit is off, then INTENA is 0, else INTENA = INTENAR
// The master bit can not be read, the memory test in the kickstart
// depends on this.

void wintena(UWO data, ULO address)
{
  if (interruptHasSetModeBit(data))
  {
    intena = interruptSetBits(intena, data);
  }
  else
  {
    intena = interruptClearBits(intena, data);
  }

  if (interruptMasterSwitchIsEnabled())
  {
    // Interrupts are enabled, check if any has not been serviced
    interruptRaisePending();
  }
}

void interruptClearInternalState(void)
{
  intreq = intena = 0;
}

void interruptIoHandlersInstall(void)
{
  memorySetIoReadStub(0x01c, rintenar);
  memorySetIoReadStub(0x01e, rintreqr);
  memorySetIoWriteStub(0x09a, wintena);
  memorySetIoWriteStub(0x09c, wintreq);
}

/* Fellow standard module events */

void interruptSoftReset(void)
{
  interruptClearInternalState();
}

void interruptHardReset(void)
{
  interruptClearInternalState();
}

void interruptEmulationStart(void)
{
  interruptIoHandlersInstall();
}

void interruptEmulationStop(void)
{
}

void interruptStartup(void)
{
}

void interruptShutdown(void)
{
}
