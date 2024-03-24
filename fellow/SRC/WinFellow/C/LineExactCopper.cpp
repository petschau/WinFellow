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

void LineExactCopper::RemoveEvent()
{
  if (_copperEvent.IsEnabled())
  {
    _scheduler.RemoveEvent(&_copperEvent);
    _copperEvent.Disable();
  }
}

void LineExactCopper::InsertEvent(uint32_t cycle)
{
  if (cycle != SchedulerEvent::EventDisableCycle)
  {
    _copperEvent.cycle = cycle;
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
    InsertEvent(_clocks.GetFrameMasterCycle() + Clocks::ToMasterCycleFrom280ns(4));
  }
  else
  {
    // DMA is off
    if (_copperRegisters.copper_suspended_wait == SchedulerEvent::EventDisableCycle)
    {
      _copperRegisters.copper_suspended_wait = _clocks.GetFrameMasterCycle();
    }
  }
}

uint32_t LineExactCopper::GetCheckedWaitCycle(uint32_t waitCycle)
{
  const auto currentFrame280nsCycle = _clocks.GetFrame280nsCycle();

  if (waitCycle <= currentFrame280nsCycle)
  {
    // Do not ever go back in time
    waitCycle = currentFrame280nsCycle + 4;
  }

  return waitCycle;
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
      const auto currentFrameMasterCycle = _clocks.GetFrameMasterCycle();
      auto nextCopperMasterCycle = _copperRegisters.copper_suspended_wait;

      if (nextCopperMasterCycle <= currentFrameMasterCycle)
      {
        nextCopperMasterCycle = currentFrameMasterCycle + Clocks::ToMasterCycleFrom280ns(4);
      }

      InsertEvent(nextCopperMasterCycle);

      _copperRegisters.copper_suspended_wait = SchedulerEvent::EventDisableCycle;
    }
  }

  _copperRegisters.copper_dma = new_dma_enable_state;
}

void LineExactCopper::NotifyCop1lcChanged()
{
  // Have been hanging since end of frame
  if (_copperRegisters.copper_dma == false && _copperRegisters.copper_suspended_wait == CopperFrameStartMasterCycleDelay)
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
  const auto currentY = _clocks.GetAgnusLine();
  const auto currentX = _clocks.GetAgnusLineCycle();
  const auto currentFrameCycle = _clocks.GetFrameCycle();

  _copperEvent.Disable();
  if (_cpuEvent.IsEnabled())
  {
    _cpuEvent.cycle += 2;
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

      // check if access to $40 - $7f (if so, Copper is using Blitter)
      if ((bswapRegC >= 0x80) || ((bswapRegC >= 0x40) && ((_copperRegisters.copcon & 0xffff) != 0x0)))
      {
        // move data to Blitter register
        InsertEvent(cycletable[(_core.Registers.BplCon0 >> 12) & 0xf] + currentFrameCycle);
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
        if ((_core.Events->blitterEvent.cycle + 4) <= currentFrameCycle)
        {
          InsertEvent(currentFrameCycle + 4);
        }
        else
        {
          InsertEvent(_core.Events->blitterEvent.cycle + 4);
        }
      }
      else
      {
        // Copper will not wait for Blitter to finish for executing wait or skip

        // zero is wait (works??)
        // (not really!)
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
            // cexit

            // to help the copper to wait line 256, do some tests here
            // Here we have detected a wait position that we are past.
            // Check if current line is 255, then if line we are supposed
            // to wait for are less than 60, do the wait afterall.
            // Here we have detected a wait that has been passed.

            // do some tests if line is 255
            if (currentY != 255)
            {
              // ctrueexit
              // Here we must do a new copper access immediately (in 4 cycles)
              InsertEvent(currentFrameCycle + 4);
            }
            else
            {
              // test line to be waited for
              if ((bswapRegC >> 8) > 0x40)
              {
                // line is 256-313, wrong to not wait
                // ctrueexit
                // Here we must do a new copper access immediately (in 4 cycles)
                InsertEvent(currentFrameCycle + 4);
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
                maskedY *= _currentFrameParameters.LongLineCycles;
                bswapRegC &= 0xfe;
                bswapRegD &= 0xfe;

                maskedX = (currentX & bswapRegD);
                // mask in horizontal
                bswapRegC |= maskedX;
                bswapRegC = bswapRegC + maskedY + 4;
                if (bswapRegC <= currentFrameCycle)
                {
                  // fix
                  bswapRegC = currentFrameCycle + 4;
                }
                InsertEvent(bswapRegC);
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
                InsertEvent(GetCheckedWaitCycle(waitY + ((bswapRegD & currentX) | bswapRegC) + 4));
              }
              else
              {
                // copwaitnotsameline
                InsertEvent(GetCheckedWaitCycle(waitY + ((bswapRegD & 0) | bswapRegC) + 4));
              }
            }
            else
            {
              // copwaitnotsameline
              InsertEvent(GetCheckedWaitCycle(waitY + ((bswapRegD & 0) | bswapRegC) + 4));
            }
          }
          else
          {
            // here we are on the correct line
            // calculate our xposition, masked with hmask
            // al - masked graph_raster_x

            correctLine = true;
            // use mask on graph_raster_x
            maskedX = currentX;
            *((uint8_t *)&maskedX) &= (bswapRegD & 0xff);
            // compare masked x with wait x
            if (*((uint8_t *)&maskedX) < (uint8_t)(bswapRegC & 0xff))
            {
              // here the wait position is not reached yet, calculate cycle when wait is true
              // previous position checks should assure that a calculated position is not less than the current cycle
              // notnever

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
                  InsertEvent(GetCheckedWaitCycle(waitY + ((bswapRegD & currentX) | bswapRegC) + 4));
                }
                else
                {
                  // copwaitnotsameline
                  InsertEvent(GetCheckedWaitCycle(waitY + ((bswapRegD & 0) | bswapRegC) + 4));
                }
              }
              else
              {
                // copwaitnotsameline
                InsertEvent(GetCheckedWaitCycle(waitY + ((bswapRegD & 0) | bswapRegC) + 4));
              }
            }
            else
            {
              // position reached, set up next instruction immediately
              // cexit

              // to help the copper to wait line 256, do some tests here
              // Here we have detected a wait position that we are past.
              // Check if current line is 255, then if line we are supposed
              // to wait for are less than 60, do the wait afterall.

              // Here we have detected a wait that has been passed.

              // do some tests if line is 255
              if (currentY != 255)
              {
                // ctrueexit
                // Here we must do a new copper access immediately (in 4 cycles)
                InsertEvent(currentFrameCycle + 4);
              }
              else
              {
                // test line to be waited for
                if ((bswapRegC >> 8) > 0x40)
                {
                  // line is 256-313, wrong to not wait
                  // ctrueexit
                  // Here we must do a new copper access immediately (in 4 cycles)
                  InsertEvent(currentFrameCycle + 4);
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
                  maskedY *= _currentFrameParameters.LongLineCycles;
                  bswapRegC &= 0xfe;
                  bswapRegD &= 0xfe;

                  maskedX = (currentX & bswapRegD);
                  // mask in horizontal
                  bswapRegC |= maskedX;
                  bswapRegC = bswapRegC + maskedY + 4;
                  if (bswapRegC <= currentFrameCycle)
                  {
                    // fix
                    bswapRegC = currentFrameCycle + 4;
                  }
                  InsertEvent(GetCheckedWaitCycle(bswapRegC));
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
            InsertEvent(currentFrameCycle + 4);
          }
          else if (*((uint8_t *)&maskedY) < (uint8_t)(bswapRegC >> 8))
          {
            // above line, don't skip
            InsertEvent(currentFrameCycle + 4);
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
            InsertEvent(currentFrameCycle + 4);
          }
        }
      }
    }
  }
}

void LineExactCopper::EndOfFrame()
{
  _copperRegisters.copper_pc = _copperRegisters.cop1lc;
  _copperRegisters.copper_suspended_wait = CopperFrameStartMasterCycleDelay;
  if (_copperRegisters.copper_dma == true)
  {
    RemoveEvent();
    InsertEvent(CopperFrameStartMasterCycleDelay);
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
    Scheduler &scheduler,
    SchedulerEvent &copperEvent,
    SchedulerEvent &cpuEvent,
    FrameParameters &currentFrameParameters,
    Clocks &clocks,
    CopperRegisters &copperRegisters)
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
