#include "Agnus.h"

#include "Graphics.h"
#include "Cia.h"
#include "FloppyDisk.h"
#include "Keyboard.h"
#include "Renderer.h"
#include "Blitter.h"
#include "automation/Automator.h"
#include "KeyboardDriver.h"
#include "draw_interlace_control.h"
#include "PaulaInterrupt.h"
#include "RetroPlatform.h"
#include <cassert>

void Agnus::InitializeEndOfLineEvent()
{
  _eolEvent.Initialize([this]() { this->EndOfLine(); }, "End of line");
}

void Agnus::InitializeEndOfFrameEvent()
{
  _eofEvent.Initialize([this]() { this->EndOfFrame(); }, "End of frame");
}

void Agnus::EndOfLine()
{
  _core.DebugLog->Log(DebugLogKind::EventHandler, _eolEvent.Name);

  /*==============================================================*/
  /* Handles graphics planar to chunky conversion                 */
  /* and updates the graphics emulation for a new line            */
  /*==============================================================*/
  graphEndOfLine();
  _core.CurrentSprites->EndOfLine(_clocks.GetChipTime().Line);

  /*==============================================================*/
  /* Update the CIA B event counter                               */
  /*==============================================================*/
  ciaUpdateEventCounter(1);

  /*==============================================================*/
  /* Handles disk DMA if it is running                            */
  /*==============================================================*/
  floppyEndOfLine();

  /*==============================================================*/
  /* Update the sound emulation                                   */
  /*==============================================================*/
  _core.Sound->EndOfLine();

  /*==============================================================*/
  /* Handle keyboard events                                       */
  /*==============================================================*/
  kbdQueueHandler();
  kbdEventEOLHandler();

  _core.Uart->EndOfLine();
  automator.EndOfLine();

  //==============================================================
  // Set up the next end of line event
  //==============================================================

  _eolEvent.cycle += _core.CurrentFrameParameters->LongLineMasterCycles;
  _core.Scheduler->InsertEvent(&_eolEvent);
}

// End of Agnus frame
// This event occurs after all cycles in the Agnus frame has been completed, at (0, linesInFrame + 1)
// The frame origin is reset to 0,0
// All scheduled event arrival cycles are rebased to the frame origin
// It also calls out to corresponding event handlers in various other modules
void Agnus::EndOfFrame()
{
  _core.DebugLog->Log(DebugLogKind::EventHandler, _eofEvent.Name);

  if (_clocks.GetFrameNumber() % 10 == 0) _core.DebugLog->Flush();

  // Produce/finalise frame image and publish it in the host
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_LINEEXACT)
  {
    drawEndOfFrame();
  }

  // Handle keyboard events
#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode()) kbdDrvEOFHandler();
#endif

  // Process special emulator actions triggered by special keypresses
  // Such as insert/eject floppies, screenshot, reset etc.
  kbdEventEOFHandler();

  // Switch scheduler and clocks frame/cycle origin
  const auto cyclesInEndedFrame = _core.CurrentFrameParameters->CyclesInFrame;

  drawDecideInterlaceStatusForNextFrame();
  _core.Agnus->SetFrameParameters(drawGetFrameIsLong());

  _core.Scheduler->NewFrame(*_core.CurrentFrameParameters);

  // Rebase CPU event cycle, the CPU is never in the queue
  if (_core.Events->cpuEvent.IsEnabled())
  {
    _core.Events->cpuEvent.cycle -= cyclesInEndedFrame;

    assert(_core.Events->cpuEvent.cycle.IsEnabledAndValid());
  }

  // Restart copper
  _core.CurrentCopper->EndOfFrame();

  // Rebase CIA timer counters
  ciaUpdateTimersEOF(cyclesInEndedFrame);

  // Reset sprite state for new frame
  _core.CurrentSprites->EndOfFrame();

  _core.Uart->RebaseTransmitReceiveDoneTimes(cyclesInEndedFrame);

  // Raise vertical blank irq
  wintreq_direct(0x8020, 0xdff09c, true);

  // Perform graphics end of frame
  graphEndOfFrame();

  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT) GraphicsContext.EndOfFrame();

  automator.EndOfFrame();

  _eofEvent.cycle = MasterTimestamp{.Cycle = _core.CurrentFrameParameters->CyclesInFrame.Offset};
  _core.Scheduler->InsertEvent(&_eofEvent);
}

void Agnus::InitializePalLongFrameParameters()
{
  _palLongFrameParameters = {
      .HorisontalBlankStart = _clocks.ToMasterTimeOffset(ChipTimeOffset::FromValue(9)),
      .HorisontalBlankEnd = _clocks.ToMasterTimeOffset(ChipTimeOffset::FromValue(44)),
      .VerticalBlankEnd = 26,
      .ShortLineMasterCycles = _clocks.ToMasterTimeOffset(ChipTimeOffset::FromValue(227)),
      .LongLineMasterCycles = _clocks.ToMasterTimeOffset(ChipTimeOffset::FromValue(227)),
      .ShortLineChipCycles = ChipTimeOffset::FromValue(227),
      .LongLineChipCycles = ChipTimeOffset::FromValue(227),
      .LinesInFrame = 313,
      .CyclesInFrame = _clocks.ToMasterTimeOffset(ChipTimeOffset::FromValue(313 * 227))};
}

void Agnus::InitializePalShortFrameParameters()
{
  _palShortFrameParameters = {
      .HorisontalBlankStart = _clocks.ToMasterTimeOffset(ChipTimeOffset::FromValue(9)),
      .HorisontalBlankEnd = _clocks.ToMasterTimeOffset(ChipTimeOffset::FromValue(44)),
      .VerticalBlankEnd = 26,
      .ShortLineMasterCycles = _clocks.ToMasterTimeOffset(ChipTimeOffset::FromValue(227)),
      .LongLineMasterCycles = _clocks.ToMasterTimeOffset(ChipTimeOffset::FromValue(227)),
      .ShortLineChipCycles = ChipTimeOffset::FromValue(227),
      .LongLineChipCycles = ChipTimeOffset::FromValue(227),
      .LinesInFrame = 312,
      .CyclesInFrame = _clocks.ToMasterTimeOffset(ChipTimeOffset::FromValue(312 * 227))};
}

void Agnus::InitializePredefinedFrameParameters()
{
  InitializePalLongFrameParameters();
  InitializePalShortFrameParameters();
}

void Agnus::SetFrameParameters(bool isLongFrame)
{
  _currentFrameParameters = isLongFrame ? _palLongFrameParameters : _palShortFrameParameters;
}

void Agnus::HardReset()
{
  SetFrameParameters(true);
}

Agnus::Agnus(SchedulerEvent &eolEvent, SchedulerEvent &eofEvent, Clocks &clocks, FrameParameters &currentFrameParameters)
  : _eolEvent(eolEvent), _eofEvent(eofEvent), _clocks(clocks), _currentFrameParameters(currentFrameParameters)
{
  InitializePredefinedFrameParameters();
}

Agnus::~Agnus() = default;
