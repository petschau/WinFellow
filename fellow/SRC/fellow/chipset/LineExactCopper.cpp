#include "fellow/chipset/CopperRegisters.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/chipset/BitplaneRegisters.h"
#include "fellow/chipset/CopperUtility.h"
#include "fellow/chipset/LineExactCopper.h"
#include "fellow/scheduler/Scheduler.h"
#include "FMEM.H"
#include "fellow/chipset/Blitter.h"

LineExactCopper line_exact_copper;

ULO LineExactCopper::_cycletable[16] = {4, 4, 4, 4, 4, 5, 6, 4, 4, 4, 4, 8, 16, 4, 4, 4};

void LineExactCopper::YTableInit()
{
  const int ex = scheduler.GetCycleFromCycle280ns(16);

  for (int i = 0; i < 512; i++)
  {
    _ytable[i] = i * scheduler.GetCyclesInLine() + ex;
  }
}

void LineExactCopper::RemoveEvent()
{
  if (copperEvent.IsEnabled())
  {
    scheduler.RemoveEvent(&copperEvent);
    copperEvent.Disable();
  }
}

void LineExactCopper::InsertEvent(const ULO cycle)
{
  if (cycle != SchedulerEvent::EventDisableCycle)
  {
    ULO lineCycle280ns = scheduler.GetCycle280nsFromCycle(cycle % scheduler.GetCyclesInLine());
    if (lineCycle280ns == 0xe2)
    {
      copperEvent.cycle = cycle + scheduler.GetCycleFromCycle280ns(2);
    }
    else
    {
      copperEvent.cycle = cycle;
    }
    scheduler.InsertEvent(&copperEvent);
  }
}

void LineExactCopper::Load(const ULO new_copper_pc)
{
  copper_registers.PC = new_copper_pc;
  copper_registers.InstructionStart = new_copper_pc;

  if (copper_registers.IsDMAEnabled == true)
  {
    RemoveEvent();
    InsertEvent(scheduler.GetFrameCycle() + scheduler.GetCycleFromCycle280ns(4));
  }
  else
  {
    // DMA is off
    if (copper_registers.SuspendedWait == SchedulerEvent::EventDisableCycle)
    {
      copper_registers.SuspendedWait = scheduler.GetFrameCycle();
    }
  }
}

ULO LineExactCopper::GetCheckedWaitCycle(const ULO waitCycle)
{
  const ULO frameCycle = scheduler.GetFrameCycle();
  if (waitCycle <= frameCycle)
  {
    // Do not ever go back in time
    return frameCycle + scheduler.GetCycleFromCycle280ns(4);
  }
  return waitCycle;
}

//----------------------------------------------------------------
// Called by wdmacon every time that register is written
// This routine takes action when the copper DMA state is changed
//----------------------------------------------------------------

void LineExactCopper::NotifyDMAEnableChanged(const bool new_dma_enable_state)
{
  if (copper_registers.IsDMAEnabled == new_dma_enable_state)
  {
    return;
  }

  // here copper was on, test if it is still on
  if (new_dma_enable_state == false)
  {
    // here copper DMA is being turned off
    // record which cycle the copper was waiting for
    // remove copper from the event list
    copper_registers.SuspendedWait = copperEvent.cycle;
    RemoveEvent();
  }
  else
  {
    // here copper is being turned on
    // reactivate the cycle the copper was waiting for the last time it was on.
    // if we have passed it in the mean-time, execute immediately.
    RemoveEvent();
    if (copper_registers.SuspendedWait != SchedulerEvent::EventDisableCycle)
    {
      const ULO frameCycle = scheduler.GetFrameCycle();
      // dma not hanging
      if (copper_registers.SuspendedWait <= frameCycle)
      {
        InsertEvent(frameCycle + scheduler.GetCycleFromCycle280ns(4));
      }
      else
      {
        InsertEvent(copper_registers.SuspendedWait);
      }
    }
    else
    {
      InsertEvent(copper_registers.SuspendedWait);
    }
  }
  copper_registers.IsDMAEnabled = new_dma_enable_state;
}

void LineExactCopper::NotifyCop1lcChanged()
{
  // Have been hanging since end of frame
  if (copper_registers.IsDMAEnabled == false && copper_registers.SuspendedWait == _firstCopperCycle)
  {
    copper_registers.PC = copper_registers.cop1lc;
    copper_registers.InstructionStart = copper_registers.cop1lc;
  }
}

UWO LineExactCopper::ReadWord()
{
  const ULO word = chipmemReadWord(copper_registers.PC);
  copper_registers.PC = chipsetMaskPtr(copper_registers.PC + 2);
  return word;
}

//---------------------------------
// Emulates one copper instruction
//---------------------------------

void LineExactCopper::EventHandler()
{
  ULO maskedY;
  ULO maskedX;
  ULO waitY;
  ULO frameCycle = scheduler.GetFrameCycle(); // Base cycle unit
  ULO currentLine = scheduler.GetRasterY();
  ULO currentLineCycle280ns = scheduler.GetLineCycle280ns();

  copperEvent.Disable();
  if (cpuEvent.IsEnabled())
  {
    cpuEvent.cycle += scheduler.GetCycleFromCycle280ns(2);
  }

  // retrieve Copper command (two words)
  ULO firstWord = ReadWord();
  ULO secondWord = ReadWord();

  copper_registers.InstructionStart = copper_registers.PC;

  if (firstWord != 0xffff || secondWord != 0xfffe)
  {
    // check bit 0 of first instruction word, zero is move
    if (CopperUtility::IsMove(firstWord))
    {
      // MOVE instruction
      firstWord &= 0x1fe;

      if (CopperUtility::IsRegisterAllowed(firstWord))
      {
        // move data to Blitter register
        InsertEvent(scheduler.GetCycleFromCycle280ns(_cycletable[(bitplane_registers.bplcon0 >> 12) & 0xf]) + frameCycle);
        memory_iobank_write[firstWord >> 1]((UWO)secondWord, firstWord);
      }
    }
    else
    {
      // wait instruction or skip instruction
      firstWord &= 0xfffe;

      // check bit BFD (Bit Finish Disable)
      if (!CopperUtility::HasBlitterFinishedDisable(secondWord) && blitterIsStarted())
      {
        // Copper waits until Blitter is finished
        copper_registers.PC = chipsetMaskPtr(copper_registers.PC - 4);
        copper_registers.InstructionStart = copper_registers.PC;

        if ((blitterEvent.cycle + scheduler.GetCycleFromCycle280ns(4)) <= frameCycle)
        {
          InsertEvent(frameCycle + scheduler.GetCycleFromCycle280ns(4));
        }
        else
        {
          InsertEvent(blitterEvent.cycle + scheduler.GetCycleFromCycle280ns(4));
        }
      }
      else
      {
        // Copper will not wait for Blitter to finish for executing wait or skip

        // zero is wait
        if (CopperUtility::IsWait(secondWord))
        {
          // WAIT instruction

          // calculate our line, masked with vmask
          // use mask on graph_raster_y
          secondWord |= 0x8000;
          bool correctLine = false;

          // compare masked y with wait line
          maskedY = currentLine;

          *((UBY *)&maskedY) &= (UBY)(secondWord >> 8);
          if (*((UBY *)&maskedY) > ((UBY)(firstWord >> 8)))
          {
            // we have passed the line, set up next instruction immediately

            // to help the copper to wait line 256, do some tests here
            // Here we have detected a wait position that we are past.
            // Check if current line is 255, then if line we are supposed
            // to wait for are less than 60, do the wait afterall.
            // Here we have detected a wait that has been passed.

            // do some tests if line is 255
            if (currentLine != 255)
            {
              // Here we must do a new copper access immediately (in 4 cycles)
              InsertEvent(frameCycle + scheduler.GetCycleFromCycle280ns(4));
            }
            else
            {
              // test line to be waited for
              if ((firstWord >> 8) > 0x40)
              {
                // line is 256-313, wrong to not wait
                // Here we must do a new copper access immediately (in 4 cycles)
                InsertEvent(frameCycle + scheduler.GetCycleFromCycle280ns(4));
              }
              else
              {
                // better to do the wait afterall
                // here we recalculate the wait stuff

                // invert masks
                secondWord = ~secondWord;

                // get missing bits
                maskedY = (currentLine | 0x100);
                *((UBY *)&maskedY) &= (secondWord >> 8);
                // mask them into vertical position
                *((UBY *)&maskedY) |= (firstWord >> 8);
                maskedY *= scheduler.GetCyclesInLine();
                firstWord &= 0xfe;
                secondWord &= 0xfe;

                maskedX = (currentLineCycle280ns & secondWord);
                // mask in horizontal
                firstWord |= maskedX;
                firstWord = scheduler.GetCycleFromCycle280ns(firstWord + 4) + maskedY;
                if (firstWord <= frameCycle)
                {
                  firstWord = frameCycle + scheduler.GetCycleFromCycle280ns(4);
                }
                InsertEvent(firstWord);
              }
            }
          }
          else if (*((UBY *)&maskedY) < (UBY)(firstWord >> 8))
          {
            // we are above the line, calculate the cycle when wait is true
            // invert masks
            secondWord = ~secondWord;

            // get bits that is not masked out
            maskedY = currentLine;
            *((UBY *)&maskedY) &= (secondWord >> 8);
            // mask in bits from waitgraph_raster_y
            *((UBY *)&maskedY) |= (firstWord >> 8);
            waitY = _ytable[maskedY];

            // when wait is on same line, use masking stuff on graph_raster_x
            // else on graph_raster_x = 0
            // prepare waitxpos
            firstWord &= 0xfe;
            secondWord &= 0xfe;
            if (correctLine == true)
            {
              if (secondWord < currentLineCycle280ns)
              {
                // get unmasked bits from current x
                InsertEvent(GetCheckedWaitCycle(waitY + scheduler.GetCycleFromCycle280ns(((secondWord & currentLineCycle280ns) | firstWord) + 4)));
              }
              else
              {
                InsertEvent(GetCheckedWaitCycle(waitY + scheduler.GetCycleFromCycle280ns(((secondWord & 0) | firstWord) + 4)));
              }
            }
            else
            {
              InsertEvent(GetCheckedWaitCycle(waitY + scheduler.GetCycleFromCycle280ns(((secondWord & 0) | firstWord) + 4)));
            }
          }
          else
          {
            // here we are on the correct line
            // calculate our xposition, masked with hmask

            correctLine = true;
            // use mask on graph_raster_x
            maskedX = currentLineCycle280ns;
            *((UBY *)&maskedX) &= (secondWord & 0xff);
            // compare masked x with wait x
            if (*((UBY *)&maskedX) < (UBY)(firstWord & 0xff))
            {
              // here the wait position is not reached yet, calculate cycle when wait is true
              // previous position checks should assure that a calculated position is not less than the current cycle

              // invert masks
              secondWord = ~secondWord;

              // get bits that is not masked out
              maskedY = currentLine;
              *((UBY *)&maskedY) &= (UBY)(secondWord >> 8);
              // mask in bits from waitgraph_raster_y
              *((UBY *)&maskedY) |= (UBY)(firstWord >> 8);
              waitY = _ytable[maskedY];

              // when wait is on same line, use masking stuff on graph_raster_x

              // prepare waitxpos
              firstWord &= 0xfe;
              secondWord &= 0xfe;
              if (correctLine == true)
              {
                if (secondWord < currentLineCycle280ns)
                {
                  // get unmasked bits from current x
                  InsertEvent(GetCheckedWaitCycle(waitY + scheduler.GetCycleFromCycle280ns(((secondWord & currentLineCycle280ns) | firstWord) + 4)));
                }
                else
                {
                  InsertEvent(GetCheckedWaitCycle(waitY + scheduler.GetCycleFromCycle280ns(((secondWord & 0) | firstWord) + 4)));
                }
              }
              else
              {
                InsertEvent(GetCheckedWaitCycle(waitY + scheduler.GetCycleFromCycle280ns(((secondWord & 0) | firstWord) + 4)));
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
              if (currentLine != 255)
              {
                // Here we must do a new copper access immediately (in 4 cycles)
                InsertEvent(frameCycle + scheduler.GetCycleFromCycle280ns(4));
              }
              else
              {
                // test line to be waited for
                if ((firstWord >> 8) > 0x40)
                {
                  // line is 256-313, wrong to not wait
                  // Here we must do a new copper access immediately (in 4 cycles)
                  InsertEvent(frameCycle + 4);
                }
                else
                {
                  // better to do the wait afterall
                  // here we recalculate the wait stuff

                  // invert masks
                  secondWord = ~secondWord;

                  // get missing bits
                  maskedY = (currentLine | 0x100);
                  *((UBY *)&maskedY) &= (secondWord >> 8);
                  // mask them into vertical position
                  *((UBY *)&maskedY) |= (firstWord >> 8);
                  maskedY *= scheduler.GetCyclesInLine();
                  firstWord &= 0xfe;
                  secondWord &= 0xfe;

                  maskedX = (currentLineCycle280ns & secondWord);
                  // mask in horizontal
                  firstWord |= maskedX;
                  firstWord = scheduler.GetCycleFromCycle280ns(firstWord + 4) + maskedY;
                  if (firstWord <= frameCycle)
                  {
                    firstWord = frameCycle + scheduler.GetCycleFromCycle280ns(4);
                  }
                  InsertEvent(GetCheckedWaitCycle(firstWord));
                }
              }
            }
          }
        }
        else
        {
          // SKIP instruction
          // calculate our line, masked with vmask
          // use mask on graph_raster_y
          secondWord |= 0x8000;
          // used to indicate correct line or not
          maskedY = currentLine;
          *((UBY *)&maskedY) &= ((secondWord >> 8));
          if (*((UBY *)&maskedY) > (UBY)(firstWord >> 8))
          {
            // do skip
            // we have passed the line, set up next instruction immediately
            copper_registers.PC = chipsetMaskPtr(copper_registers.PC + 4);
            copper_registers.InstructionStart = copper_registers.PC;

            InsertEvent(frameCycle + scheduler.GetCycleFromCycle280ns(4));
          }
          else if (*((UBY *)&maskedY) < (UBY)(firstWord >> 8))
          {
            // above line, don't skip
            InsertEvent(frameCycle + scheduler.GetCycleFromCycle280ns(4));
          }
          else
          {
            // here we are on the correct line

            // calculate our xposition, masked with hmask
            // use mask on graph_raster_x
            // Compare masked x with wait x
            maskedX = currentLineCycle280ns;
            *((UBY *)&maskedX) &= secondWord;
            if (*((UBY *)&maskedX) >= (UBY)(firstWord & 0xff))
            {
              // position reached, set up next instruction immediately
              copper_registers.PC = chipsetMaskPtr(copper_registers.PC + 4);
              copper_registers.InstructionStart = copper_registers.PC;
            }
            InsertEvent(frameCycle + scheduler.GetCycleFromCycle280ns(4));
          }
        }
      }
    }
  }
}

void LineExactCopper::EndOfFrame()
{
  copper_registers.PC = copper_registers.cop1lc;
  copper_registers.InstructionStart = copper_registers.PC;

  copper_registers.SuspendedWait = _firstCopperCycle;
  if (copper_registers.IsDMAEnabled == true)
  {
    RemoveEvent();
    InsertEvent(_firstCopperCycle);
  }
}

void LineExactCopper::HardReset()
{
}

void LineExactCopper::EmulationStart()
{
}

void LineExactCopper::EmulationStop()
{
}

void LineExactCopper::Startup()
{
  _firstCopperCycle = scheduler.GetCycleFromCycle280ns(40);
  YTableInit();
}

void LineExactCopper::Shutdown()
{
}

LineExactCopper::LineExactCopper() = default;
