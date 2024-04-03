#include "Defs.h"

#include "chipset.h"
#include "MemoryInterface.h"
#include "CustomChipset/Copper/CopperRegisters.h"
#include "CycleExactCopper.h"
#include "GraphicsPipeline.h"
#include <cassert>

uint16_t CycleExactCopper::ReadWord()
{
  return chipmemReadWord(_copperRegisters.copper_pc);
}

void CycleExactCopper::IncreasePtr()
{
  _copperRegisters.copper_pc = chipsetMaskPtr(_copperRegisters.copper_pc + 2);
}

void CycleExactCopper::SetState(CopperStates newState, ChipTimestamp timestamp)
{
  const auto chipLineCycleCount = _currentFrameParameters.LongLineChipCycles.Offset;

  assert(timestamp.IsEvenCycle());
  assert(timestamp.Cycle < chipLineCycleCount);

  _scheduler.RemoveEvent(&_copperEvent);
  _state = newState;
  _copperEvent.cycle = _clocks.ToMasterTime(timestamp);
  if (_copperRegisters.copper_dma)
  {
    _scheduler.InsertEvent(&_copperEvent);
  }
}

void CycleExactCopper::Load(uint32_t new_copper_ptr)
{
  _copperRegisters.copper_pc = new_copper_ptr;

  _skip_next = false;
  ChipTimestamp startChipCycle = ChipTimestamp::FromOriginAndOffset(_clocks.GetChipTime(), ChipTimeOffset::FromValue(6), _currentFrameParameters);

  if (startChipCycle.IsOddCycle())
  {
    startChipCycle = ChipTimestamp::FromOriginAndOffset(startChipCycle, ChipTimeOffset::FromValue(1), _currentFrameParameters);
  }

  SetState(CopperStates::COPPER_STATE_READ_FIRST_WORD, startChipCycle);
}

void CycleExactCopper::SetStateNone()
{
  _scheduler.RemoveEvent(&_copperEvent);
  _state = CopperStates::COPPER_STATE_NONE;
  _skip_next = false;
  _copperEvent.Disable();
}

bool CycleExactCopper::IsRegisterAllowed(uint32_t regno)
{
  return (regno >= 0x80) || ((regno >= 0x40) && ((_copperRegisters.copcon & 0xffff) != 0x0));
}

void CycleExactCopper::Move()
{
  uint32_t regno = (uint32_t)(_first & 0x1fe);
  uint16_t value = _second;

  if (!IsRegisterAllowed(regno))
  {
    SetStateNone();
    return;
  }

  ChipTimestamp nextEventTime = ChipTimestamp::FromOriginAndOffset(_clocks.GetChipTime(), ChipTimeOffset::FromValue(2), _currentFrameParameters);
  SetState(CopperStates::COPPER_STATE_READ_FIRST_WORD, nextEventTime);

  if (!_skip_next)
  {
    memory_iobank_write[regno >> 1](value, regno);
  }

  _skip_next = false;
}

void CycleExactCopper::Wait()
{
  bool blitter_finish_disable = (_second & 0x8000) == 0x8000;
  uint32_t ve = (((uint32_t)_second >> 8) & 0x7f) | 0x80;
  uint32_t he = (uint32_t)_second & 0xfe;

  uint32_t vp = (uint32_t)(_first >> 8);
  uint32_t hp = (uint32_t)(_first & 0xfe);

  const auto currentLineChipCycleCount = _currentFrameParameters.LongLineChipCycles.Offset;
  // const auto currentChipCycle = _clocks.GetAgnusChipFrameCycle();
  // uint32_t test_cycle = currentCycle + 2;
  // uint32_t rasterY = test_cycle / currentLineCycleCount;
  // uint32_t rasterX = test_cycle % currentLineCycleCount;

  const ChipTimestamp chipTime = _clocks.GetChipTime();
  uint32_t rasterY = chipTime.Line;
  uint32_t rasterX = chipTime.Cycle;

  // This should not be happening
  // if (rasterX & 1)
  //{
  //  rasterX++;
  //}

  assert(chipTime.IsEvenCycle());

  // (rasterY, rasterX) is the starting point, the first possible compare

  _skip_next = false;

  // Does current rasterY compare as larger?
  if ((rasterY & ve) > (vp & ve))
  {
    // Wait completed immediately
    _skip_next = false;
    ChipTimestamp nextEventTime = ChipTimestamp::FromOriginAndOffset(chipTime, ChipTimeOffset::FromValue(2), _currentFrameParameters);
    SetState(CopperStates::COPPER_STATE_READ_FIRST_WORD, nextEventTime);
    return;
  }

  // Is the vertical position an exact match? Examine the rest of the line for matching horisontal positions.
  if ((rasterY & ve) == (vp & ve))
  {
    uint32_t initial_wait_rasterX = rasterX;
    while ((rasterX < currentLineChipCycleCount) && ((rasterX & he) < (hp & he)))
    {
      rasterX += 2;
    }

    if (rasterX < currentLineChipCycleCount)
    {
      if (initial_wait_rasterX == rasterX)
      {
        rasterX += 2;
      }

      // Wait completed on current line with horisontal match now or later on the line
      SetState(CopperStates::COPPER_STATE_READ_FIRST_WORD, ChipTimestamp::FromLineAndCycle(rasterY, rasterX, _currentFrameParameters));
      return;
    }
  }

  // Try later lines
  rasterY++;

  // Find the first horisontal position on a line that match when comparing from the start of a line
  for (rasterX = 0; (rasterX < currentLineChipCycleCount) && ((rasterX & he) < (hp & he)); rasterX += 2)
    ;

  // Find the first vertical position that match
  if (rasterX >= currentLineChipCycleCount)
  {
    // There is no match on the horisontal position. The vertical position must be larger than vp to match
    rasterX = 0;

    while ((rasterY < _currentFrameParameters.LinesInFrame) && ((rasterY & ve) <= (vp & ve)))
    {
      rasterY++;
    }
  }
  else
  {
    // A match can either be exact on vp and hp, or the vertical position must be larger than vp.
    while ((rasterY < _currentFrameParameters.LinesInFrame) && ((rasterY & ve) < (vp & ve)))
    {
      rasterY++;
    }
  }

  if (rasterY >= _currentFrameParameters.LinesInFrame)
  {
    // No match was found
    SetStateNone();
  }
  else if ((rasterY & ve) == (vp & ve))
  {
    // An exact match on both vp and hp was found
    SetState(CopperStates::COPPER_STATE_READ_FIRST_WORD, ChipTimestamp::FromLineAndCycle(rasterY, rasterX, _currentFrameParameters));
  }
  else
  {
    // A match on vp being larger was found
    SetState(CopperStates::COPPER_STATE_READ_FIRST_WORD, ChipTimestamp::FromLineAndCycle(rasterY, 0, _currentFrameParameters));
  }
}

void CycleExactCopper::Skip()
{
  uint32_t ve = (((uint32_t)_second >> 8) & 0x7f) | 0x80;
  uint32_t he = (uint32_t)_second & 0xfe;

  uint32_t vp = (uint32_t)(_first >> 8);
  uint32_t hp = (uint32_t)(_first & 0xfe);

  const auto cyclesInCurrentLine = _currentFrameParameters.LongLineChipCycles.Offset;
  const ChipTimestamp chipTime = _clocks.GetChipTime();
  uint32_t rasterY = chipTime.Line;
  uint32_t rasterX = chipTime.Cycle;

  // This should not happen
  // if (rasterX & 1)
  //{
  //  rasterX++;
  //}

  assert(chipTime.IsEvenCycle());

  _skip_next = (((rasterY & ve) > (vp & ve)) || (((rasterY & ve) == (vp & ve)) && ((rasterX & he) >= (hp & he))));
  SetState(CopperStates::COPPER_STATE_READ_FIRST_WORD, ChipTimestamp::FromOriginAndOffset(chipTime, ChipTimeOffset::FromValue(2), _currentFrameParameters));
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
  CopperStates next_state = (IsMove()) ? CopperStates::COPPER_STATE_TRANSFER_SECOND_WORD : CopperStates::COPPER_STATE_READ_SECOND_WORD;
  SetState(next_state, ChipTimestamp::FromOriginAndOffset(_clocks.GetChipTime(), ChipTimeOffset::FromValue(2), _currentFrameParameters));
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
  _debugLog.Log(DebugLogKind::EventHandler, _copperEvent.Name);

  ChipTimestamp chipTime = _clocks.GetChipTime();

  // TODO: Should not be handled here, copper must not be given this cycle
  if ((_currentFrameParameters.LongLineChipCycles.Offset & 1) == 1 && chipTime.Cycle == (_currentFrameParameters.LongLineChipCycles.Offset - 1))
  {
    // TODO: Add assert instead
    SetState(_state, ChipTimestamp::FromOriginAndOffset(_clocks.GetChipTime(), ChipTimeOffset::FromValue(1), _currentFrameParameters));
    return;
  }

  if (_cpuEvent.IsEnabled())
  {
    _cpuEvent.cycle = _cpuEvent.cycle + MasterTimeOffset::FromChipTimeOffset(ChipTimeOffset::FromValue(2));
  }

  switch (_state)
  {
    case CopperStates::COPPER_STATE_READ_FIRST_WORD: ReadFirstWord(); break;
    case CopperStates::COPPER_STATE_READ_SECOND_WORD: ReadSecondWord(); break;
    case CopperStates::COPPER_STATE_TRANSFER_SECOND_WORD: TransferSecondWord(); break;
  }
}

void CycleExactCopper::NotifyDMAEnableChanged(bool new_dma_enable_state)
{
  if (_copperRegisters.copper_dma == new_dma_enable_state)
  {
    return;
  }

  if (new_dma_enable_state == false) // Copper DMA is being turned off
  {
    _scheduler.RemoveEvent(&_copperEvent);
  }
  else // Copper DMA is being turned on
  {
    // reactivate the cycle the copper was waiting for the last time it was on.
    // if we have passed it in the mean-time, execute immediately.
    _scheduler.RemoveEvent(&_copperEvent);
    if (_copperEvent.IsEnabled())
    {
      // dma not hanging
      const auto masterTimestamp = _clocks.GetMasterTime();
      if (_copperEvent.cycle <= masterTimestamp)
      {
        // TODO: Make sure copper asks for an even cycle
        _copperEvent.cycle = masterTimestamp + MasterTimeOffset::FromChipTimeOffset(ChipTimeOffset::FromValue(2));
        _scheduler.InsertEvent(&_copperEvent);
      }
      else
      {
        _scheduler.InsertEvent(&_copperEvent);
      }
    }
  }

  _copperRegisters.copper_dma = new_dma_enable_state;
}

void CycleExactCopper::NotifyCop1lcChanged()
{
  // Line based code has a "hack" here. Don't want to transfer it, yet.
}

void CycleExactCopper::InitializeCopperEvent()
{
  _copperEvent.Initialize([this]() { this->EventHandler(); }, "Cycle exact copper");
}

void CycleExactCopper::EndOfFrame()
{
  _copperRegisters.copper_pc = _copperRegisters.cop1lc;
  _skip_next = false;

  // TODO: This delay is from the legacy copper, cannot possibly be an actual thing
  SetState(CopperStates::COPPER_STATE_READ_FIRST_WORD, ChipTimestamp::FromLineAndCycle(0, 40, _currentFrameParameters));
}

void CycleExactCopper::HardReset()
{
  _copperRegisters.ClearState();
}

void CycleExactCopper::EmulationStart()
{
  _copperRegisters.InstallIOHandlers();
}

void CycleExactCopper::EmulationStop()
{
}

CycleExactCopper::CycleExactCopper(
    Scheduler &scheduler,
    SchedulerEvent &copperEvent,
    SchedulerEvent &cpuEvent,
    FrameParameters &currentFrameParameters,
    Clocks &clocks,
    CopperRegisters &copperRegisters,
    DebugLog &debugLog)
  : _scheduler(scheduler),
    _copperEvent(copperEvent),
    _cpuEvent(cpuEvent),
    _currentFrameParameters(currentFrameParameters),
    _clocks(clocks),
    _copperRegisters(copperRegisters),
    _debugLog(debugLog),
    _skip_next(false)
{
}

CycleExactCopper::~CycleExactCopper()
{
}
