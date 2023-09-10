#include "defs.h"

#include "chipset.h"
#include "bus.h"
#include "fmem.h"
#include "CopperRegisters.h"
#include "CycleExactCopper.h"
#include "GRAPH.H"

UWO CycleExactCopper::ReadWord()
{
  return chipmemReadWord(copper_registers.copper_pc);
}

void CycleExactCopper::IncreasePtr()
{
  copper_registers.copper_pc = chipsetMaskPtr(copper_registers.copper_pc + 2);
}

void CycleExactCopper::SetState(CopperStates newState, ULO cycle)
{
  if ((cycle % busGetCyclesInThisLine()) & 1)
  {
    cycle++;
  }
  busRemoveEvent(&copperEvent);
  _state = newState;
  copperEvent.cycle = cycle;
  if (copper_registers.copper_dma)
  {
    busInsertEvent(&copperEvent);
  }
}

void CycleExactCopper::Load(ULO new_copper_ptr)
{
  copper_registers.copper_pc = new_copper_ptr;

  _skip_next = false;
  ULO start_cycle = busGetCycle() + 6;
  if ((start_cycle % busGetCyclesInThisLine()) & 1)
  {
    start_cycle++;
  }
  SetState(COPPER_STATE_READ_FIRST_WORD, start_cycle);
}

void CycleExactCopper::SetStateNone()
{
  busRemoveEvent(&copperEvent);
  _state = COPPER_STATE_NONE;
  _skip_next = false;
  copperEvent.cycle = BUS_CYCLE_DISABLE;
}

bool CycleExactCopper::IsRegisterAllowed(ULO regno)
{
  return (regno >= 0x80) || ((regno >= 0x40) && ((copper_registers.copcon & 0xffff) != 0x0));
}

void CycleExactCopper::Move()
{
  ULO regno = (ULO) (_first & 0x1fe);
  UWO value = _second;

  if (IsRegisterAllowed(regno))
  {
    SetState(COPPER_STATE_READ_FIRST_WORD, busGetCycle() + 2);
    if (!_skip_next)
    {
      memory_iobank_write[regno>>1](value, regno);
    }
    _skip_next = false;
  }
  else
  {
    SetStateNone();
  }
}

void CycleExactCopper::Wait()
{
  bool blitter_finish_disable = (_second & 0x8000) == 0x8000;
  ULO ve = (((ULO) _second >> 8) & 0x7f) | 0x80;
  ULO he = (ULO) _second & 0xfe;

  ULO vp = (ULO) (_first >> 8);
  ULO hp = (ULO) (_first & 0xfe);

  ULO test_cycle = busGetCycle() + 2;
  ULO rasterY = test_cycle / busGetCyclesInThisLine();
  ULO rasterX = test_cycle % busGetCyclesInThisLine();

  if (rasterX & 1)
  {
    rasterX++;
  }

  _skip_next = false;

  // Is the vertical position already larger? 
  if ((rasterY & ve) > (vp & ve))
  {
    _skip_next = false;
    SetState(COPPER_STATE_READ_FIRST_WORD, busGetCycle() + 2);
    return;
  }

  // Is the vertical position an exact match? Examine the rest of the line for matching horisontal positions.
  if ((rasterY & ve) == (vp & ve))
  {
    ULO initial_wait_rasterX = rasterX;
    while ((rasterX <= 0xe2) && ((rasterX & he) < (hp & he))) rasterX += 2;
    if (rasterX < 0xe4)
    {
      if (initial_wait_rasterX == rasterX) rasterX += 2;
      SetState(COPPER_STATE_READ_FIRST_WORD, rasterY*busGetCyclesInThisLine() + rasterX);
      return;
    }
  }

  // Try later lines
  rasterY++;

  // Find the first horisontal position on a line that match when comparing from the start of a line
  for (rasterX = 0; (rasterX <= 0xe2) && ((rasterX & he) < (hp & he)); rasterX += 2);

  // Find the first vertical position that match
  if (rasterX == 0xe4)
  {
    // There is no match on the horisontal position. The vertical position must be larger than vp to match
    while ((rasterY < busGetLinesInThisFrame()) && ((rasterY & ve) <= (vp & ve))) rasterY++;
  }
  else
  {
    // A match can either be exact on vp and hp, or the vertical position must be larger than vp.
    while ((rasterY < busGetLinesInThisFrame()) && ((rasterY & ve) < (vp & ve))) rasterY++;
  }

  if (rasterY >= busGetLinesInThisFrame())
  {
    // No match was found
    SetStateNone();
  }
  else if ((rasterY & ve) == (vp & ve))
  {
    // An exact match on both vp and hp was found
    SetState(COPPER_STATE_READ_FIRST_WORD, rasterY*busGetCyclesInThisLine() + rasterX); // +2);
  }
  else
  {
    // A match on vp being larger (not equal) was found
    SetState(COPPER_STATE_READ_FIRST_WORD, rasterY*busGetCyclesInThisLine() + rasterX); // +2);
  }
}

void CycleExactCopper::Skip()
{
  ULO ve = (((ULO) _second >> 8) & 0x7f) | 0x80;
  ULO he = (ULO) _second & 0xfe;

  ULO vp = (ULO) (_first >> 8);
  ULO hp = (ULO) (_first & 0xfe);

  ULO test_cycle = busGetCycle();
  ULO rasterY = test_cycle / busGetCyclesInThisLine();
  ULO rasterX = test_cycle % busGetCyclesInThisLine();

  if (rasterX & 1)
  {
    rasterX++;
  }

  _skip_next = (((rasterY & ve) > (vp & ve)) || (((rasterY & ve) == (vp & ve)) && ((rasterX & he) >= (hp & he))));
  SetState(COPPER_STATE_READ_FIRST_WORD, busGetCycle() + 2);
}

bool CycleExactCopper::IsMove()
{
  return (_first & 1) == 0;
}

bool CycleExactCopper::IsWait()
{
  return (_second & 1) == 0;
}

void CycleExactCopper::ReadFirstWord()
{
  _first = ReadWord();
  IncreasePtr();
  CopperStates next_state = (IsMove()) ? COPPER_STATE_TRANSFER_SECOND_WORD : COPPER_STATE_READ_SECOND_WORD;
  SetState(next_state, busGetCycle() + 2);
}

void CycleExactCopper::ReadSecondWord()
{
  _second = ReadWord();
  IncreasePtr();

  if (IsWait())
  {
    Wait();
  }
  else
  {
    Skip();
  }
}

void CycleExactCopper::TransferSecondWord()
{
  _second = ReadWord();
  IncreasePtr();
  Move();
}

void CycleExactCopper::EventHandler()
{
  if (busGetRasterX() == 0xe2)
  {
    SetState(_state, busGetCycle() + 1);
    return;
  }
  if (cpuEvent.cycle != BUS_CYCLE_DISABLE)
  {
    cpuEvent.cycle += 2;
  }
  switch (_state)
  {
    case COPPER_STATE_READ_FIRST_WORD:
      ReadFirstWord();
      break;
    case COPPER_STATE_READ_SECOND_WORD:
      ReadSecondWord();
      break;
    case COPPER_STATE_TRANSFER_SECOND_WORD:
      TransferSecondWord();
      break;
  }
}

void CycleExactCopper::NotifyDMAEnableChanged(bool new_dma_enable_state)
{
  if (copper_registers.copper_dma == new_dma_enable_state)
  {
    return;
  }

  if (new_dma_enable_state == false) // Copper DMA is being turned off
  {
    busRemoveEvent(&copperEvent);
  }
  else // Copper DMA is being turned on
  {
    // reactivate the cycle the copper was waiting for the last time it was on.
    // if we have passed it in the mean-time, execute immediately.
    busRemoveEvent(&copperEvent);
    if (copperEvent.cycle != BUS_CYCLE_DISABLE)
    {
      // dma not hanging
      if (copperEvent.cycle <= bus.cycle)
      {
        copperEvent.cycle = busGetCycle() + 2;
        busInsertEvent(&copperEvent);
      }
      else
      {
        busInsertEvent(&copperEvent);
      }
    }
  }
  copper_registers.copper_dma = new_dma_enable_state;
}

void CycleExactCopper::NotifyCop1lcChanged()
{
  // Line based code has a "hack" here. Don't want to transfer it, yet. Should re-check at some time and see if it can be solved inside the model.
}

void CycleExactCopper::EndOfFrame()
{
  copper_registers.copper_pc = copper_registers.cop1lc;
  _skip_next = false;
  SetState(COPPER_STATE_READ_FIRST_WORD, 40);
}

void CycleExactCopper::HardReset()
{
}

void CycleExactCopper::EmulationStart()
{
}

void CycleExactCopper::EmulationStop()
{
}

CycleExactCopper::CycleExactCopper()
  : _skip_next(false)
{
}

CycleExactCopper::~CycleExactCopper()
{
}
