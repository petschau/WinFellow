/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Bus Event Scheduler Initialization                                         */
/*                                                                            */
/* Author: Petter Schau (peschau@online.no)                                   */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

#include "portable.h"
#include "renaming.h"

#include "defs.h"
#include "cpu.h"
#include "cia.h"
#include "copper.h"
#include "fmem.h"
#include "blit.h"
#include "bus.h"


ULO debugging;
ULO curcycle;
ULO eol_next, eof_next;
ULO lvl2_next;
ULO lvl3_next;
ULO lvl4_next;
ULO lvl5_next;
ULO lvl6_next;
ULO lvl7_next;
buseventfunc lvl2_ptr, lvl3_ptr, lvl4_ptr, lvl5_ptr, lvl6_ptr, lvl7_ptr;

UWO bus_cycle_to_ypos[0x20000];
UBY bus_cycle_to_xpos[0x20000];

extern ULO copper_ptr, copper_next;

/*===========================================================================*/
/* Cycle counter to raster beam position table                               */
/*===========================================================================*/

void busCycleToRasterPosTableInit(void) {
  int i;

  for (i = 0; i < 0x20000; i++) {
    bus_cycle_to_ypos[i>>2] = (UWO) (i / 228);
    bus_cycle_to_xpos[i] = (UBY) (i % 228);
  }
}


/*===========================================================================*/
/* Set up events for initial execution                                       */
/*===========================================================================*/

void busEventlistClear(void) {
  lvl7_next = CYCLESPERFRAME;
  lvl7_ptr = endOfFrame;
  lvl6_next = CYCLESPERFRAME;
  lvl6_ptr = endOfFrame;
  lvl5_next = CYCLESPERFRAME;
  lvl5_ptr = endOfFrame;
  lvl4_next = CYCLESPERFRAME;
  lvl4_ptr = endOfFrame;
  lvl3_next = CYCLESPERLINE - 1;
  lvl3_ptr = endOfLine;
  lvl2_next = CYCLESPERLINE - 1;
  lvl2_ptr = endOfLine;
}


/*===========================================================================*/
/* Called on emulation start / stop and reset                                */
/*===========================================================================*/

void busEmulationStart(void) {
}

void busEmulationStop(void) {
}

void busHardReset(void) {
  busEventlistClear();
  irq_next = 0xffffffff;
  curcycle = 0;
  cpu_next = 0;
  eol_next = CYCLESPERLINE - 1;
  eof_next = CYCLESPERFRAME;
}


/*===========================================================================*/
/* Called on emulator startup / shutdown                                     */
/*===========================================================================*/

void busStartup(void) {
  busCycleToRasterPosTableInit();
  curcycle = 0;
}

void busShutdown(void) {
}

void busScanLevel6(ULO* lvlx_next, void (**lvlx_ptr)(void))
{ 
  if ((*lvlx_next) > cia_next_event_time)
  {
    *lvlx_next = cia_next_event_time;
    *lvlx_ptr = ciaHandleEventWrapper;
  }
  lvl6_next = *lvlx_next;
  lvl6_ptr = *lvlx_ptr; 
}

void busScanLevel5(ULO* lvlx_next, void (**lvlx_ptr)(void))
{ 
  if ((*lvlx_next) > irq_next)
  {
    *lvlx_next = irq_next;
    *lvlx_ptr = cpuSetUpInterrupt;
  }
  lvl5_next = *lvlx_next;
  lvl5_ptr = *lvlx_ptr; 
}

void busScanLevel4(ULO* lvlx_next, void (**lvlx_ptr)(void))
{ 
  if ((*lvlx_next) > blitend)
  {
    *lvlx_next = blitend;
    *lvlx_ptr = finish_blit;
  }
  lvl4_next = *lvlx_next;
  lvl4_ptr = *lvlx_ptr; 
}

void busScanLevel3(ULO* lvlx_next, void (**lvlx_ptr)(void))
{ 
  if ((*lvlx_next) > eol_next)
  {
    *lvlx_next = eol_next;
    *lvlx_ptr = endOfLine;
  }
  lvl3_next = *lvlx_next;
  lvl3_ptr = *lvlx_ptr; 
}

void busScanLevel2(ULO* lvlx_next, void (**lvlx_ptr)(void))
{ 
  if ((*lvlx_next) > copper_next)
  {
    *lvlx_next = copper_next;
    *lvlx_ptr = copperEmulate;
  }
  lvl2_next = *lvlx_next;
  lvl2_ptr = *lvlx_ptr; 
}

void busScanEventsLevel2(void)
{ 
  ULO lvlx_next;
  void (*lvlx_ptr)(void);

  lvlx_next = lvl3_next;
  lvlx_ptr = lvl3_ptr;

  busScanLevel2(&lvlx_next, &lvlx_ptr);
}

void busScanEventsLevel3(void)
{ 
  ULO lvlx_next;
  void (*lvlx_ptr)(void);

  lvlx_next = lvl4_next;
  lvlx_ptr = lvl4_ptr;

  busScanLevel3(&lvlx_next, &lvlx_ptr);
  busScanLevel2(&lvlx_next, &lvlx_ptr);
}

void busScanEventsLevel4(void)
{ 
  ULO lvlx_next;
  void (*lvlx_ptr)(void);

  lvlx_next = lvl5_next;
  lvlx_ptr = lvl5_ptr;

  busScanLevel4(&lvlx_next, &lvlx_ptr);
  busScanLevel3(&lvlx_next, &lvlx_ptr);
  busScanLevel2(&lvlx_next, &lvlx_ptr);
}

void busScanEventsLevel5(void)
{ 
  ULO lvlx_next;
  void (*lvlx_ptr)(void);

  lvlx_next = lvl6_next;
  lvlx_ptr = lvl6_ptr;

  busScanLevel5(&lvlx_next, &lvlx_ptr);
  busScanLevel4(&lvlx_next, &lvlx_ptr);
  busScanLevel3(&lvlx_next, &lvlx_ptr);
  busScanLevel2(&lvlx_next, &lvlx_ptr);
}

void busScanEventsLevel6(void)
{ 
  ULO lvlx_next;
  void (*lvlx_ptr)(void);

  lvlx_next = CYCLESPERFRAME;
  lvlx_ptr = endOfFrame;

  busScanLevel6(&lvlx_next, &lvlx_ptr);
  busScanLevel5(&lvlx_next, &lvlx_ptr);
  busScanLevel4(&lvlx_next, &lvlx_ptr);
  busScanLevel3(&lvlx_next, &lvlx_ptr);
  busScanLevel2(&lvlx_next, &lvlx_ptr);
}
