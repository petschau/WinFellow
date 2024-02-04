#include "Defs.h"

#include "chipset.h"
#include "MemoryInterface.h"
#include "CustomChipset/Copper/CopperRegisters.h"
#include "CycleExactCopper.h"
#include "GraphicsPipeline.h"

uint16_t CycleExactCopper::ReadWord()
{
  return chipmemReadWord(_copperRegisters.copper_pc);
}

void CycleExactCopper::IncreasePtr()
{
  _copperRegisters.copper_pc = chipsetMaskPtr(_copperRegisters.copper_pc + 2);
}

void CycleExactCopper::SetState(CopperStates newState, uint32_t cycle)
{
  const auto currentLineCycleCount = _currentFrameParameters.GetAgnusCyclesInLine(_timekeeper.GetAgnusLineCycle());
  if ((cycle % currentLineCycleCount) & 1)
  {
    cycle++;
  }

  _scheduler.RemoveEvent(&_copperEvent);
  _state = newState;
  _copperEvent.cycle = cycle;
  if (_copperRegisters.copper_dma)
  {
    _scheduler.InsertEvent(&_copperEvent);
  }
}

void CycleExactCopper::Load(uint32_t new_copper_ptr)
{
  _copperRegisters.copper_pc = new_copper_ptr;

  _skip_next = false;
  uint32_t start_cycle = _timekeeper.GetFrameCycle() + 6;
  const auto currentLineCycleCount = _currentFrameParameters.GetAgnusCyclesInLine(_timekeeper.GetAgnusLine());
  if ((start_cycle % currentLineCycleCount) & 1)
  {
    start_cycle++;
  }

  SetState(CopperStates::COPPER_STATE_READ_FIRST_WORD, start_cycle);
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

  if (IsRegisterAllowed(regno))
  {
    SetState(CopperStates::COPPER_STATE_READ_FIRST_WORD, _timekeeper.GetFrameCycle() + 2);
    if (!_skip_next)
    {
      memory_iobank_write[regno >> 1](value, regno);
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
  uint32_t ve = (((uint32_t)_second >> 8) & 0x7f) | 0x80;
  uint32_t he = (uint32_t)_second & 0xfe;

  uint32_t vp = (uint32_t)(_first >> 8);
  uint32_t hp = (uint32_t)(_first & 0xfe);

  const auto currentLineCycleCount = _currentFrameParameters.GetAgnusCyclesInLine(_timekeeper.GetAgnusLine());
  const auto currentCycle = _timekeeper.GetFrameCycle();
  uint32_t test_cycle = currentCycle + 2;
  uint32_t rasterY = test_cycle / currentLineCycleCount;
  uint32_t rasterX = test_cycle % currentLineCycleCount;

  if (rasterX & 1)
  {
    rasterX++;
  }

  _skip_next = false;

  // Is the vertical position already larger?
  if ((rasterY & ve) > (vp & ve))
  {
    _skip_next = false;
    SetState(CopperStates::COPPER_STATE_READ_FIRST_WORD, _timekeeper.GetFrameCycle() + 2);
    return;
  }

  // Is the vertical position an exact match? Examine the rest of the line for matching horisontal positions.
  if ((rasterY & ve) == (vp & ve))
  {
    uint32_t initial_wait_rasterX = rasterX;
    while ((rasterX <= 0xe2) && ((rasterX & he) < (hp & he)))
      rasterX += 2;
    if (rasterX < 0xe4)
    {
      if (initial_wait_rasterX == rasterX) rasterX += 2;
      SetState(CopperStates::COPPER_STATE_READ_FIRST_WORD, rasterY * currentLineCycleCount + rasterX);
      return;
    }
  }

  // Try later lines
  rasterY++;

  // Find the first horisontal position on a line that match when comparing from the start of a line
  for (rasterX = 0; (rasterX <= 0xe2) && ((rasterX & he) < (hp & he)); rasterX += 2)
    ;

  // Find the first vertical position that match
  if (rasterX == 0xe4)
  {
    // There is no match on the horisontal position. The vertical position must be larger than vp to match
    while ((rasterY < _currentFrameParameters.LinesInFrame) && ((rasterY & ve) <= (vp & ve)))
      rasterY++;
  }
  else
  {
    // A match can either be exact on vp and hp, or the vertical position must be larger than vp.
    while ((rasterY < _currentFrameParameters.LinesInFrame) && ((rasterY & ve) < (vp & ve)))
      rasterY++;
  }

  if (rasterY >= _currentFrameParameters.LinesInFrame)
  {
    // No match was found
    SetStateNone();
  }
  else if ((rasterY & ve) == (vp & ve))
  {
    // An exact match on both vp and hp was found
    SetState(CopperStates::COPPER_STATE_READ_FIRST_WORD, rasterY * currentLineCycleCount + rasterX); // +2);
  }
  else
  {
    // A match on vp being larger (not equal) was found
    SetState(CopperStates::COPPER_STATE_READ_FIRST_WORD, rasterY * currentLineCycleCount + rasterX); // +2);
  }
}

void CycleExactCopper::Skip()
{
  uint32_t ve = (((uint32_t)_second >> 8) & 0x7f) | 0x80;
  uint32_t he = (uint32_t)_second & 0xfe;

  uint32_t vp = (uint32_t)(_first >> 8);
  uint32_t hp = (uint32_t)(_first & 0xfe);

  const auto cyclesInCurrentLine = _currentFrameParameters.GetAgnusCyclesInLine(_timekeeper.GetAgnusLine());
  const auto currentFrameCycle = _timekeeper.GetFrameCycle();
  uint32_t rasterY = _timekeeper.GetAgnusLine();
  uint32_t rasterX = _timekeeper.GetAgnusLineCycle();

  if (rasterX & 1)
  {
    rasterX++;
  }

  _skip_next = (((rasterY & ve) > (vp & ve)) || (((rasterY & ve) == (vp & ve)) && ((rasterX & he) >= (hp & he))));
  SetState(CopperStates::COPPER_STATE_READ_FIRST_WORD, currentFrameCycle + 2);
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
  SetState(next_state, _timekeeper.GetFrameCycle() + 2);
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

  if (_timekeeper.GetAgnusLineCycle() == 0xe2)
  {
    SetState(_state, _timekeeper.GetFrameCycle() + 1);
    return;
  }

  if (_cpuEvent.IsEnabled())
  {
    _cpuEvent.cycle += 2;
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
      const auto currentFrameCycle = _timekeeper.GetFrameCycle();
      if (_copperEvent.cycle <= currentFrameCycle)
      {
        _copperEvent.cycle = currentFrameCycle + 2;
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
  SetState(CopperStates::COPPER_STATE_READ_FIRST_WORD, 40);
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
    Timekeeper &timekeeper,
    CopperRegisters &copperRegisters,
    DebugLog &debugLog)
  : _scheduler(scheduler),
    _copperEvent(copperEvent),
    _cpuEvent(cpuEvent),
    _currentFrameParameters(currentFrameParameters),
    _timekeeper(timekeeper),
    _copperRegisters(copperRegisters),
    _debugLog(debugLog),
    _skip_next(false)
{
}

CycleExactCopper::~CycleExactCopper()
{
}
