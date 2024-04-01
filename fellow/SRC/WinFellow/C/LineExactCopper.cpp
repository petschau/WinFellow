#include "LineExactCopper.h"
#include "chipset.h"
#include "MemoryInterface.h"
#include "Blitter.h"
#include "VirtualHost/Core.h"

uint32_t LineExactCopper::cycletable[16] = {4, 4, 4, 4, 4, 5, 6, 4, 4, 4, 4, 8, 16, 4, 4, 4};

void LineExactCopper::YTableInit()
{
  const int ex = 16;
  const uint32_t cyclesInLine = 227;

  for (int i = 0; i < 512; i++)
  {
    ytable[i] = i * cyclesInLine + ex;
  }
}

ChipTimestamp LineExactCopper::MakeTimestamp(const ChipTimestamp timestamp, const ChipTimeOffset offset) const
{
  return ChipTimestamp::FromOriginAndOffset(timestamp, offset, _currentFrameParameters);
}

void LineExactCopper::RemoveEvent()
{
  if (_copperEvent.IsEnabled())
  {
    _scheduler.RemoveEvent(&_copperEvent);
    _copperEvent.Disable();
  }
}

void LineExactCopper::InsertEvent(MasterTimestamp timestamp)
{
  if (timestamp != SchedulerEvent::EventDisableCycle)
  {
    _copperEvent.cycle = timestamp;
    _scheduler.InsertEvent(&_copperEvent);
  }
}

void LineExactCopper::InitializeCopperEvent()
{
  _copperEvent.Initialize([this]() { this->EventHandler(); }, "Line exact copper");
}

void LineExactCopper::Load(uint32_t new_copper_pc)
{
  _copperRegisters.copper_pc = new_copper_pc;

  if (_copperRegisters.copper_dma == true)
  {
    RemoveEvent();

    auto startChipTime = MakeTimestamp(_clocks.GetChipTime(), LoadStartDelay);
    InsertEvent(_clocks.ToMasterTime(GetCheckedWaitTime(startChipTime)));
  }
  else
  {
    // DMA is off
    // TODO: Seems more correct to simply assign "now" to the suspended wait time regardless, as it can be earlier than the suspended wait
    //       Keeping the wait could prevent the load from starting right await when DMA is turned on
    if (_copperRegisters.copper_suspended_wait == SchedulerEvent::EventDisableCycle)
    {
      _copperRegisters.copper_suspended_wait = _clocks.GetMasterTime();
    }
  }
}

ChipTimestamp LineExactCopper::GetCheckedWaitTime(ChipTimestamp waitTimestamp)
{
  ChipTimestamp currentChipTime = _clocks.GetChipTime();
  if (waitTimestamp <= currentChipTime)
  {
    return MakeTimestamp(currentChipTime, LoadStartDelay);
  }

  return waitTimestamp;
}

//---------------------------------------------------------------
// Called by wdmacon every time that register is written
// This routine takes action when the copper DMA state is changed
//---------------------------------------------------------------

void LineExactCopper::NotifyDMAEnableChanged(bool new_dma_enable_state)
{
  if (_copperRegisters.copper_dma == new_dma_enable_state)
  {
    return;
  }

  // here copper was on, test if it is still on
  if (new_dma_enable_state == false)
  {
    // here copper DMA is being turned off
    // record which cycle the copper was waiting for
    // remove copper from the event list
    _copperRegisters.copper_suspended_wait = _copperEvent.cycle;
    RemoveEvent();
  }
  else
  {
    // here copper is being turned on
    // reactivate the cycle the copper was waiting for the last time it was on.
    // if we have passed it in the mean-time, execute immediately.
    RemoveEvent();

    if (_copperRegisters.copper_suspended_wait != SchedulerEvent::EventDisableCycle)
    {
      // Copper wants to do work but has not been able to due to DMA off
      auto startChipTime = ChipTimestamp::FromMasterTime(_copperRegisters.copper_suspended_wait, _currentFrameParameters);
      InsertEvent(_clocks.ToMasterTime(GetCheckedWaitTime(startChipTime)));

      _copperRegisters.copper_suspended_wait = SchedulerEvent::EventDisableCycle;
    }
  }

  _copperRegisters.copper_dma = new_dma_enable_state;
}

void LineExactCopper::NotifyCop1lcChanged()
{
  // Have been hanging since end of frame
  if (_copperRegisters.copper_dma == false && _copperRegisters.copper_suspended_wait.Cycle == FrameStartDelay.Offset)
  {
    _copperRegisters.copper_pc = _copperRegisters.cop1lc;
  }
}

/*-------------------------------------------------------------------------------
; Emulates one copper instruction
;-------------------------------------------------------------------------------*/

void LineExactCopper::EventHandler()
{
  _core.DebugLog->Log(DebugLogKind::EventHandler, _copperEvent.Name);

  bool correctLine;
  uint32_t maskedY;
  uint32_t maskedX;
  uint32_t waitY;
  const auto currentChipTime = _clocks.GetChipTime();
  const auto currentY = currentChipTime.Line;
  const auto currentX = currentChipTime.Cycle;
  const auto currentFrameChipCycle = currentY * _currentFrameParameters.LongLineChipCycles.Offset + currentX;

  _copperEvent.Disable();
  if (_cpuEvent.IsEnabled())
  {
    _cpuEvent.cycle += CopperCycleCpuDelay;
  }

  // retrieve Copper command (two words)
  uint32_t bswapRegC = chipmemReadWord(_copperRegisters.copper_pc);
  _copperRegisters.copper_pc = chipsetMaskPtr(_copperRegisters.copper_pc + 2);
  uint32_t bswapRegD = chipmemReadWord(_copperRegisters.copper_pc);
  _copperRegisters.copper_pc = chipsetMaskPtr(_copperRegisters.copper_pc + 2);

  if (bswapRegC != 0xffff || bswapRegD != 0xfffe)
  {
    // check bit 0 of first instruction word, zero is move
    if ((bswapRegC & 0x1) == 0x0)
    {
      // MOVE instruction
      bswapRegC &= 0x1fe;

      // check if access enabled to $40 - $7f
      if ((bswapRegC >= 0x80) || ((bswapRegC >= 0x40) && ((_copperRegisters.copcon & 0xffff) != 0x0)))
      {
        ChipTimeOffset nextEventOffsetWithBitplaneDelay = ChipTimeOffset::FromValue(cycletable[(_core.Registers.BplCon0 >> 12) & 0xf]);
        ChipTimestamp nextEventTimestamp = MakeTimestamp(currentChipTime, nextEventOffsetWithBitplaneDelay);

        InsertEvent(_clocks.ToMasterTime(nextEventTimestamp));

        memory_iobank_write[bswapRegC >> 1]((uint16_t)bswapRegD, bswapRegC);
      }
    }
    else
    {
      // wait instruction or skip instruction
      bswapRegC &= 0xfffe;
      // check bit BFD (Bit Finish Disable)
      if (((bswapRegD & 0x8000) == 0x0) && blitterIsStarted())
      {
        // Copper waits until Blitter is finished
        _copperRegisters.copper_pc = chipsetMaskPtr(_copperRegisters.copper_pc - 4);

        ChipTimestamp blitterFinishedCycle = MakeTimestamp(ChipTimestamp::FromMasterTime(_core.Events->blitterEvent.cycle, _currentFrameParameters), InstructionInterval);

        if (blitterFinishedCycle <= currentChipTime)
        {
          ChipTimestamp nextEventTimestamp = MakeTimestamp(currentChipTime, InstructionInterval);

          InsertEvent(_clocks.ToMasterTime(nextEventTimestamp));
        }
        else
        {
          InsertEvent(_clocks.ToMasterTime(blitterFinishedCycle));
        }
      }
      else
      {
        // Copper will not wait for Blitter to finish before executing wait or skip
        // zero is wait
        if ((bswapRegD & 0x1) == 0x0)
        {
          // WAIT instruction

          // calculate our line, masked with vmask
          // bl - masked graph_raster_y

          // use mask on graph_raster_y
          bswapRegD |= 0x8000;
          // used to indicate correct line or not
          correctLine = false;

          // compare masked y with wait line
          // maskedY = graph_raster_y;
          maskedY = currentY;

          *((uint8_t *)&maskedY) &= (uint8_t)(bswapRegD >> 8);
          if (*((uint8_t *)&maskedY) > ((uint8_t)(bswapRegC >> 8)))
          {
            // we have passed the line, set up next instruction immediately

            // to help the copper to wait line 256, do some tests here
            // Here we have detected a wait position that we are past.
            // Check if current line is 255, then if line we are supposed
            // to wait for are less than 60, do the wait afterall.
            // Here we have detected a wait that has been passed.

            // do some tests if line is 255
            if (currentY != 255)
            {
              // Here we must do a new copper access immediately (in 4 cycles)
              ChipTimestamp nextEventTimestamp = MakeTimestamp(currentChipTime, InstructionInterval);

              InsertEvent(_clocks.ToMasterTime(nextEventTimestamp));
            }
            else
            {
              // test line to be waited for
              if ((bswapRegC >> 8) > 0x40)
              {
                // line is 256-313, wrong to not wait
                // Here we must do a new copper access immediately (in 4 cycles)
                ChipTimestamp nextEventTimestamp = MakeTimestamp(currentChipTime, InstructionInterval);
                InsertEvent(_clocks.ToMasterTime(nextEventTimestamp));
              }
              else
              {
                // better to do the wait afterall
                // here we recalculate the wait stuff

                // invert masks
                bswapRegD = ~bswapRegD;

                // get missing bits
                maskedY = (currentY | 0x100);
                *((uint8_t *)&maskedY) &= (bswapRegD >> 8);
                // mask them into vertical position
                *((uint8_t *)&maskedY) |= (bswapRegC >> 8);
                maskedY *= _currentFrameParameters.LongLineChipCycles.Offset;
                bswapRegC &= 0xfe;
                bswapRegD &= 0xfe;

                maskedX = (currentX & bswapRegD);
                // mask in horizontal
                bswapRegC |= maskedX;
                bswapRegC = bswapRegC + maskedY + 4;

                ChipTimestamp nextEventTimestamp = MakeTimestamp(ChipTimestamp{.Line = 0, .Cycle = 0}, ChipTimeOffset::FromValue(bswapRegC));
                InsertEvent(_clocks.ToMasterTime(GetCheckedWaitTime(nextEventTimestamp)));
              }
            }
          }
          else if (*((uint8_t *)&maskedY) < (uint8_t)(bswapRegC >> 8))
          {
            // we are above the line, calculate the cycle when wait is true
            // notnever

            // invert masks
            bswapRegD = ~bswapRegD;

            // get bits that is not masked out
            maskedY = currentY;
            *((uint8_t *)&maskedY) &= (bswapRegD >> 8);
            // mask in bits from waitgraph_raster_y
            *((uint8_t *)&maskedY) |= (bswapRegC >> 8);
            waitY = ytable[maskedY];

            // when wait is on same line, use masking stuff on graph_raster_x
            // else on graph_raster_x = 0
            // prepare waitxpos
            bswapRegC &= 0xfe;
            bswapRegD &= 0xfe;
            if (correctLine == true)
            {
              if (bswapRegD < currentX)
              {
                // get unmasked bits from current x
                ChipTimestamp nextEventTimestamp =
                    MakeTimestamp(ChipTimestamp{.Line = 0, .Cycle = 0}, ChipTimeOffset::FromValue(waitY + ((bswapRegD & currentX) | bswapRegC) + 4));
                InsertEvent(_clocks.ToMasterTime(GetCheckedWaitTime(nextEventTimestamp)));
              }
              else
              {
                ChipTimestamp nextEventTimestamp = MakeTimestamp(ChipTimestamp{.Line = 0, .Cycle = 0}, ChipTimeOffset::FromValue(waitY + ((bswapRegD & 0) | bswapRegC) + 4));
                InsertEvent(_clocks.ToMasterTime(GetCheckedWaitTime(nextEventTimestamp)));
              }
            }
            else
            {
              ChipTimestamp nextEventTimestamp = MakeTimestamp(ChipTimestamp{.Line = 0, .Cycle = 0}, ChipTimeOffset::FromValue(waitY + ((bswapRegD & 0) | bswapRegC) + 4));
              InsertEvent(_clocks.ToMasterTime(GetCheckedWaitTime(nextEventTimestamp)));
            }
          }
          else
          {
            // here we are on the correct line
            // calculate our xposition, masked with hmask

            correctLine = true;
            // use mask on graph_raster_x
            maskedX = currentX;
            *((uint8_t *)&maskedX) &= (bswapRegD & 0xff);
            // compare masked x with wait x
            if (*((uint8_t *)&maskedX) < (uint8_t)(bswapRegC & 0xff))
            {
              // here the wait position is not reached yet, calculate cycle when wait is true
              // previous position checks should assure that a calculated position is not less than the current cycle

              // invert masks
              bswapRegD = ~bswapRegD;
              // bswapRegD &= 0xffff;

              // get bits that is not masked out
              maskedY = currentY;
              *((uint8_t *)&maskedY) &= (uint8_t)(bswapRegD >> 8);
              // mask in bits from waitgraph_raster_y
              *((uint8_t *)&maskedY) |= (uint8_t)(bswapRegC >> 8);
              waitY = ytable[maskedY];

              // when wait is on same line, use masking stuff on graph_raster_x
              // else on graph_raster_x = 0

              // prepare waitxpos
              bswapRegC &= 0xfe;
              bswapRegD &= 0xfe;
              if (correctLine == true)
              {
                if (bswapRegD < currentX)
                {
                  // get unmasked bits from current x
                  ChipTimestamp nextEventTimestamp =
                      MakeTimestamp(ChipTimestamp{.Line = 0, .Cycle = 0}, ChipTimeOffset::FromValue(waitY + ((bswapRegD & currentX) | bswapRegC) + 4));
                  InsertEvent(_clocks.ToMasterTime(GetCheckedWaitTime(nextEventTimestamp)));
                }
                else
                {
                  ChipTimestamp nextEventTimestamp = MakeTimestamp(ChipTimestamp{.Line = 0, .Cycle = 0}, ChipTimeOffset::FromValue(waitY + ((bswapRegD & 0) | bswapRegC) + 4));
                  InsertEvent(_clocks.ToMasterTime(GetCheckedWaitTime(nextEventTimestamp)));
                }
              }
              else
              {
                ChipTimestamp nextEventTimestamp = MakeTimestamp(ChipTimestamp{.Line = 0, .Cycle = 0}, ChipTimeOffset::FromValue(waitY + ((bswapRegD & 0) | bswapRegC) + 4));
                InsertEvent(_clocks.ToMasterTime(GetCheckedWaitTime(nextEventTimestamp)));
              }
            }
            else
            {
              // position reached, set up next instruction immediately

              // to help the copper to wait line 256, do some tests here
              // Here we have detected a wait position that we are past.
              // Check if current line is 255, then if line we are supposed
              // to wait for are less than 60, do the wait afterall.

              // Here we have detected a wait that has been passed.

              // do some tests if line is 255
              if (currentY != 255)
              {
                // Here we must do a new copper access immediately (in 4 cycles)
                ChipTimestamp nextEventTimestamp = MakeTimestamp(currentChipTime, InstructionInterval);
                InsertEvent(_clocks.ToMasterTime(nextEventTimestamp));
              }
              else
              {
                // test line to be waited for
                if ((bswapRegC >> 8) > 0x40)
                {
                  // line is 256-313, wrong to not wait
                  // Here we must do a new copper access immediately (in 4 cycles)
                  ChipTimestamp nextEventTimestamp = MakeTimestamp(currentChipTime, InstructionInterval);
                  InsertEvent(_clocks.ToMasterTime(nextEventTimestamp));
                }
                else
                {
                  // better to do the wait afterall
                  // here we recalculate the wait stuff

                  // invert masks
                  bswapRegD = ~bswapRegD;

                  // get missing bits
                  maskedY = (currentY | 0x100);
                  *((uint8_t *)&maskedY) &= (bswapRegD >> 8);
                  // mask them into vertical position
                  *((uint8_t *)&maskedY) |= (bswapRegC >> 8);
                  maskedY *= _currentFrameParameters.LongLineChipCycles.Offset;
                  bswapRegC &= 0xfe;
                  bswapRegD &= 0xfe;

                  maskedX = (currentX & bswapRegD);
                  // mask in horizontal
                  bswapRegC |= maskedX;
                  bswapRegC = bswapRegC + maskedY + 4;

                  ChipTimestamp nextEventTimestamp = MakeTimestamp(ChipTimestamp{.Line = 0, .Cycle = 0}, ChipTimeOffset::FromValue(bswapRegC));
                  InsertEvent(_clocks.ToMasterTime(GetCheckedWaitTime(nextEventTimestamp)));
                }
              }
            }
          }
        }
        else
        {
          // SKIP instruction

          // new skip, copied from cwait just to try something
          // cskip

          // calculate our line, masked with vmask
          // bl - masked graph_raster_y

          // use mask on graph_raster_y
          bswapRegD |= 0x8000;
          // used to indicate correct line or not
          correctLine = false;
          maskedY = currentY;
          *((uint8_t *)&maskedY) &= ((bswapRegD >> 8));
          if (*((uint8_t *)&maskedY) > (uint8_t)(bswapRegC >> 8))
          {
            // do skip
            // we have passed the line, set up next instruction immediately
            _copperRegisters.copper_pc = chipsetMaskPtr(_copperRegisters.copper_pc + 4);
            ChipTimestamp nextEventTimestamp = MakeTimestamp(currentChipTime, InstructionInterval);
            InsertEvent(_clocks.ToMasterTime(nextEventTimestamp));
          }
          else if (*((uint8_t *)&maskedY) < (uint8_t)(bswapRegC >> 8))
          {
            // above line, don't skip
            ChipTimestamp nextEventTimestamp = MakeTimestamp(currentChipTime, InstructionInterval);
            InsertEvent(_clocks.ToMasterTime(nextEventTimestamp));
          }
          else
          {
            // here we are on the correct line

            // calculate our xposition, masked with hmask
            // al - masked graph_raster_x
            correctLine = true;

            // use mask on graph_raster_x
            // Compare masked x with wait x
            maskedX = currentX;
            *((uint8_t *)&maskedX) &= bswapRegD;
            if (*((uint8_t *)&maskedX) >= (uint8_t)(bswapRegC & 0xff))
            {
              // position reached, set up next instruction immediately
              _copperRegisters.copper_pc = chipsetMaskPtr(_copperRegisters.copper_pc + 4);
            }
            ChipTimestamp nextEventTimestamp = MakeTimestamp(currentChipTime, InstructionInterval);
            InsertEvent(_clocks.ToMasterTime(nextEventTimestamp));
          }
        }
      }
    }
  }
}

void LineExactCopper::EndOfFrame()
{
  MasterTimestamp copperStartTimestamp = MasterTimestamp{.Cycle = 0} + FrameStartDelay;
  _copperRegisters.copper_pc = _copperRegisters.cop1lc;

  _copperRegisters.copper_suspended_wait = copperStartTimestamp;

  if (_copperRegisters.copper_dma == true)
  {
    RemoveEvent();
    InsertEvent(copperStartTimestamp);
  }
}

void LineExactCopper::HardReset()
{
  _copperRegisters.ClearState();
}

void LineExactCopper::EmulationStart()
{
  _copperRegisters.InstallIOHandlers();
}

void LineExactCopper::EmulationStop()
{
}

LineExactCopper::LineExactCopper(
    Scheduler &scheduler, SchedulerEvent &copperEvent, SchedulerEvent &cpuEvent, FrameParameters &currentFrameParameters, Clocks &clocks, CopperRegisters &copperRegisters)
  : ICopper(),
    _scheduler(scheduler),
    _copperEvent(copperEvent),
    _cpuEvent(cpuEvent),
    _currentFrameParameters(currentFrameParameters),
    _clocks(clocks),
    _copperRegisters(copperRegisters)
{
  YTableInit();
}

LineExactCopper::~LineExactCopper()
{
}
