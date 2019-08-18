/*=========================================================================*/
/* Fellow                                                                  */
/* Copper Emulation Initialization                                         */
/*                                                                         */
/* Author: Petter Schau                                                    */
/*                                                                         */
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
#include "fellow/chipset/CopperRegisters.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/chipset/CycleExactCopper.h"
#include "LineExactCopper.h"

void copperNotifyDMAEnableChanged(bool new_dma_enable_state)
{
  if (chipset_info.IsCycleExact)
  {
    cycle_exact_copper.NotifyDMAEnableChanged(new_dma_enable_state);
  }
  else
  {
    line_exact_copper.NotifyDMAEnableChanged(new_dma_enable_state);
  }
}

void copperEventHandler()
{
  if (!chipset_info.IsCycleExact)
  {
    line_exact_copper.EventHandler();
  }
  else
  {
    cycle_exact_copper.EventHandler();
  }
}

void copperEndOfFrame()
{
  if (chipset_info.IsCycleExact)
  {
    cycle_exact_copper.EndOfFrame();
  }
  else
  {
    line_exact_copper.EndOfFrame();
  }
}

void copperEmulationStart()
{
  copper_registers.InstallIOHandlers();
  if (chipset_info.IsCycleExact)
  {
    cycle_exact_copper.EmulationStart();
  }
  else
  {
    line_exact_copper.EmulationStart();
  }
}

void copperEmulationStop()
{
  if (chipset_info.IsCycleExact)
  {
    cycle_exact_copper.EmulationStop();
  }
  else
  {
    line_exact_copper.EmulationStop();
  }
}

void copperHardReset()
{
  copper_registers.ClearState();
  if (chipset_info.IsCycleExact)
  {
    cycle_exact_copper.HardReset();
  }
  else
  {
    line_exact_copper.HardReset();
  }
}

void copperStartup()
{
  copper_registers.ClearState();
  if (chipset_info.IsCycleExact)
  {
    cycle_exact_copper.Startup();
  }
  else
  {
    line_exact_copper.Startup();
  }
}

void copperShutdown()
{
  if (chipset_info.IsCycleExact)
  {
    cycle_exact_copper.Shutdown();
  }
  else
  {
    line_exact_copper.Shutdown();
  }
}
