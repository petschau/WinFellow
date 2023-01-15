/*=========================================================================*/
/* Fellow                                                                  */
/* BitplaneShifter                                                         */
/*                                                                         */
/* Authors: Petter Schau                                                   */
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

#include "fellow/chipset/BitplaneShifter.h"
#include "fellow/chipset/BitplaneUtility.h"
#include "fellow/chipset/BitplaneRegisters.h"
#include "fellow/chipset/Planar2ChunkyDecoder.h"
#include "fellow/chipset/DIWXStateMachine.h"
#include "fellow/chipset/CycleExactSprites.h"
#include "fellow/chipset/HostFrameImmediateRenderer.h"
#include "fellow/chipset/HostFrameDelayedRenderer.h"
#include "fellow/debug/log/DebugLogHandler.h"
#include "fellow/chipset/Sprite.h"
#include "fellow/application/HostRenderer.h"
#include "fellow/scheduler/Scheduler.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/api/Services.h"

using namespace fellow::api;

class BitplaneShifter_AddDataCallback : public IBitplaneShifter_AddDataCallback
{
private:
  BitplaneShifter *_bitplaneShifter;

public:
  void operator()(const UWO *data) override
  {
    _bitplaneShifter->AddData(data);
  }

  BitplaneShifter_AddDataCallback(BitplaneShifter *bitplaneShifter) : _bitplaneShifter(bitplaneShifter)
  {
  }

  virtual ~BitplaneShifter_AddDataCallback() = default;
};

class BitplaneShifter_FlushCallback : public IBitplaneShifter_FlushCallback
{
private:
  BitplaneShifter *_bitplaneShifter;

public:
  void operator()() override
  {
    _bitplaneShifter->Flush();
  }

  BitplaneShifter_FlushCallback(BitplaneShifter *bitplaneShifter) : _bitplaneShifter(bitplaneShifter)
  {
  }

  virtual ~BitplaneShifter_FlushCallback() = default;
};

void BitplaneArmList::Clear()
{
  EntryCount = 0;
  First = 0;
}

BitplaneArmEntry &BitplaneArmList::GetFirst()
{
  return Entries[First % ArmListSize];
}

BitplaneArmEntry &BitplaneArmList::AllocateNext()
{
  F_ASSERT(EntryCount < ArmListSize);

  return Entries[(First + EntryCount++) % ArmListSize];
}

void BitplaneShifter::AddOutputUntilLogEntry(ULO outputLine, ULO startX, ULO pixelCount)
{
  if (DebugLog.Enabled)
  {
    DebugLog.AddBitplaneShifterLogEntry(
        DebugLogSource::BITPLANESHIFTER_ACTION,
        _scheduler->GetRasterFrameCount(),
        _scheduler->GetSHResTimestamp(),
        BitplaneShifterLogReasons::BitplaneShifter_output_until,
        outputLine,
        startX,
        pixelCount);
  }
}

void BitplaneShifter::AddDuplicateLogEntry(ULO outputLine, ULO startX)
{
  if (DebugLog.Enabled)
  {
    DebugLog.AddBitplaneShifterLogEntry(
        DebugLogSource::BITPLANESHIFTER_ACTION,
        _scheduler->GetRasterFrameCount(),
        _scheduler->GetSHResTimestamp(),
        BitplaneShifterLogReasons::BitplaneShifter_output_until,
        outputLine,
        startX,
        0);
  }
}

void BitplaneShifter::SetupEvent(ULO line, ULO cycle)
{
  shifterEvent.cycle = _scheduler->GetBaseClockTimestamp(line, cycle);
  _scheduler->InsertEvent(&shifterEvent);
}

void BitplaneShifter::ShiftActive(const ULO pixelCount)
{
  _input[0].w <<= pixelCount;
  _input[1].w <<= pixelCount;
  _input[2].w <<= pixelCount;
  _input[3].w <<= pixelCount;
  _input[4].w <<= pixelCount;
  _input[5].w <<= pixelCount;
}

void BitplaneShifter::ClearActive()
{
  _input[0].w = 0;
  _input[1].w = 0;
  _input[2].w = 0;
  _input[3].w = 0;
  _input[4].w = 0;
  _input[5].w = 0;
}

void BitplaneShifter::AddArmEntry(unsigned int first, unsigned int stride, ULO armX, const UWO *dat)
{
  BitplaneArmEntry &armEntry = _armList.AllocateNext();
  armEntry.X = armX;
  armEntry.Count = (stride == 1) ? 6 : 3;

  F_ASSERT(armX > _lastOutputX);

  unsigned int i = 0;
  for (unsigned int bitplaneNumber = first; bitplaneNumber < 6; bitplaneNumber += stride)
  {
    armEntry.Data[i].Number = bitplaneNumber;
    armEntry.Data[i].Data = dat[bitplaneNumber];
    i++;
  }
}

// The commit command adds bitplane data-words to the arm list. This does not necessarily flush the pipeline.
// The committed data will be armed in OCS/ECS at the first cycle position in the next cylinder
void BitplaneShifter::AddData(const UWO *dat)
{
  SHResTimestamp addedAtPosition(_scheduler->GetRasterY(), _scheduler->GetLastCycleInCurrentCylinder());

  if (addedAtPosition.Line < _scheduler->GetVerticalBlankEnd())
  {
    // Ignore bpldat commits on lines that can not be displayed
    return;
  }

  if (!_activated)
  {
    Flush(addedAtPosition);
    _activated = true;
  }

  bool isLores = BitplaneUtility::IsLores();
  ULO baseX = (addedAtPosition.Pixel + 8) & 0xfffffff8; // In the next cylinder
  ULO armEvenX = CalculateArmX(baseX, BitplaneUtility::GetEvenScrollMask(), isLores);
  ULO armOddX = CalculateArmX(baseX, BitplaneUtility::GetOddScrollMask(), isLores);

  if (armEvenX == armOddX)
  {
    AddArmEntry(0, 1, armEvenX, dat);
  }
  else if (armEvenX < armOddX)
  {
    AddArmEntry(0, 2, armEvenX, dat);
    AddArmEntry(1, 2, armOddX, dat);
  }
  else
  {
    AddArmEntry(1, 2, armOddX, dat);
    AddArmEntry(0, 2, armEvenX, dat);
  }
}

ULO BitplaneShifter::CalculateArmX(ULO baseX, ULO waitMask, bool isLores)
{
  if (isLores)
  {
    ULO armX = (baseX & 0xffffffc0) | (waitMask << 2);
    ULO adjustedArmX = (armX < baseX) ? armX + (16 * 4) : armX;
    return (adjustedArmX < _scheduler->GetHorisontalBlankStart()) ? (adjustedArmX + _scheduler->GetCyclesInLine()) : adjustedArmX;
  }

  // hires
  ULO armX = (baseX & 0xffffffe0) | ((waitMask & 0x7) << 2); // Remove the upper bit of the arm position mask, hires arms data twice as fast
  ULO adjustedArmX = (armX < baseX) ? (armX + (8 * 4)) : armX;
  return ((adjustedArmX < _scheduler->GetHorisontalBlankStart()) ? (adjustedArmX + _scheduler->GetCyclesInLine()) : adjustedArmX) + 4;
}

void BitplaneShifter::ShiftPixels(ULO pixelCount, bool isVisible)
{
  if (isVisible)
  {
    if (pixelCount >= 16)
    {
      Planar2ChunkyDecoder::P2CNext8Pixels(_input[0].b[1], _input[1].b[1], _input[2].b[1], _input[3].b[1], _input[4].b[1], _input[5].b[1]);

      Planar2ChunkyDecoder::P2CNext8Pixels(_input[0].b[0], _input[1].b[0], _input[2].b[0], _input[3].b[0], _input[4].b[0], _input[5].b[0]);

      ClearActive();

      if (pixelCount > 16)
      {
        Planar2ChunkyDecoder::P2CNextBackgroundPixels(pixelCount - 16);
      }
    }
    else
    {
      if (pixelCount & 8)
      {
        Planar2ChunkyDecoder::P2CNext8Pixels(_input[0].b[1], _input[1].b[1], _input[2].b[1], _input[3].b[1], _input[4].b[1], _input[5].b[1]);

        ShiftActive(8);
      }

      if (pixelCount & 4)
      {
        Planar2ChunkyDecoder::P2CNext4Pixels(_input[0].b[1], _input[1].b[1], _input[2].b[1], _input[3].b[1], _input[4].b[1], _input[5].b[1]);

        ShiftActive(4);
      }

      ULO remainingPixels = pixelCount & 3;
      if (remainingPixels > 0)
      {
        Planar2ChunkyDecoder::P2CNextPixels(
            remainingPixels, _input[0].b[1], _input[1].b[1], _input[2].b[1], _input[3].b[1], _input[4].b[1], _input[5].b[1]);

        ShiftActive(remainingPixels);
      }
    }
  }
  else
  {
    // Shift pipeline active, output disabled due to diw x
    if (pixelCount >= 16)
    {
      ClearActive();
    }
    else
    {
      ShiftActive(pixelCount);
    }
    Planar2ChunkyDecoder::P2CNextBackgroundPixelsDual(pixelCount);
  }
}

void BitplaneShifter::ShiftPixelsDual(ULO pixelCount, bool isVisible)
{
  if (isVisible)
  {
    if (pixelCount >= 16)
    {
      Planar2ChunkyDecoder::P2CNext8PixelsDual(_input[0].b[1], _input[1].b[1], _input[2].b[1], _input[3].b[1], _input[4].b[1], _input[5].b[1]);

      Planar2ChunkyDecoder::P2CNext8PixelsDual(_input[0].b[0], _input[1].b[0], _input[2].b[0], _input[3].b[0], _input[4].b[0], _input[5].b[0]);

      ClearActive();

      if (pixelCount > 16)
      {
        Planar2ChunkyDecoder::P2CNextBackgroundPixelsDual(pixelCount - 16);
      }
    }
    else
    {
      if (pixelCount & 8)
      {
        Planar2ChunkyDecoder::P2CNext8PixelsDual(_input[0].b[1], _input[1].b[1], _input[2].b[1], _input[3].b[1], _input[4].b[1], _input[5].b[1]);

        ShiftActive(8);
      }

      if (pixelCount & 4)
      {
        Planar2ChunkyDecoder::P2CNext4PixelsDual(_input[0].b[1], _input[1].b[1], _input[2].b[1], _input[3].b[1], _input[4].b[1], _input[5].b[1]);

        ShiftActive(4);
      }

      ULO remainingPixels = pixelCount & 3;
      if (remainingPixels > 0)
      {
        Planar2ChunkyDecoder::P2CNextPixelsDual(
            remainingPixels, _input[0].b[1], _input[1].b[1], _input[2].b[1], _input[3].b[1], _input[4].b[1], _input[5].b[1]);

        ShiftActive(remainingPixels);
      }
    }
  }
  else
  {
    // Shift pipeline active, output disabled due to diw x
    if (pixelCount >= 16)
    {
      ClearActive();
    }
    else
    {
      ShiftActive(pixelCount);
    }
    Planar2ChunkyDecoder::P2CNextBackgroundPixelsDual(pixelCount);
  }
}

void BitplaneShifter::ShiftSubBatch(ULO pixelCount, bool isVisible)
{
  ULO pixelDivider = BitplaneUtility::IsLores() ? 4 : 2;

  if (BitplaneUtility::IsDualPlayfield())
  {
    ShiftPixelsDual(pixelCount / pixelDivider, isVisible);
  }
  else
  {
    ShiftPixels(pixelCount / pixelDivider, isVisible);
  }
}

void BitplaneShifter::ShiftBackgroundBatch(ULO pixelCount)
{
  ULO pixelDivider = BitplaneUtility::IsLores() ? 4 : 2;

  if (BitplaneUtility::IsDualPlayfield())
  {
    Planar2ChunkyDecoder::P2CNextBackgroundPixelsDual(pixelCount / pixelDivider);
  }
  else
  {
    Planar2ChunkyDecoder::P2CNextBackgroundPixels(pixelCount / pixelDivider);
  }
}

ULO BitplaneShifter::GetNextArmSplitX(ULO batchEndX)
{
  if (_armList.EntryCount == 0)
  {
    return batchEndX;
  }

  BitplaneArmEntry &armEntry = _armList.GetFirst();
  return (batchEndX < armEntry.X) ? batchEndX : armEntry.X;
}

void BitplaneShifter::ActivateArmedBitplaneData(ULO x)
{
  if (_armList.EntryCount == 0)
  {
    return;
  }

  const BitplaneArmEntry &armEntry = _armList.GetFirst();
  if (armEntry.X != x)
  {
    // Went here because we are at the end of a batch that did not reach the arm x
    return;
  }

  ULO enabledBitplaneCount = BitplaneUtility::GetEnabledBitplaneCount();

  for (ULO i = 0; i < armEntry.Count; i++)
  {
    const BitplaneArmData &armData = armEntry.Data[i];

    if (armData.Number >= enabledBitplaneCount)
    {
      break;
    }

    _input[armData.Number].w = armData.Data;
  }

  _armList.EntryCount--;
  _armList.First++;
}

void BitplaneShifter::ShiftBitplaneBatch(ULO pixelCount)
{
  const bool isVisible = diwx_state_machine.IsVisible();
  const ULO batchEndX = _lastOutputX + 1 + pixelCount;
  ULO currentX = _lastOutputX + 1;

  F_ASSERT(currentX < 2048);
  F_ASSERT(batchEndX <= 1928);

  do
  {
    ULO nextArmSplitX = GetNextArmSplitX(batchEndX);

    F_ASSERT(nextArmSplitX >= currentX);

    ShiftSubBatch(nextArmSplitX - currentX, isVisible);

    ActivateArmedBitplaneData(nextArmSplitX);
    currentX = nextArmSplitX;
  } while (currentX != batchEndX);
}

void BitplaneShifter::Flush()
{
  Flush(SHResTimestamp(_scheduler->GetRasterY(), _scheduler->GetLastCycleInCurrentCylinder()));
}

void BitplaneShifter::Flush(const SHResTimestamp &untilPosition)
{
  ULO outputLine = untilPosition.GetUnwrappedLine();
  if (outputLine < _scheduler->GetVerticalBlankEnd())
  {
    // Above hard start
    return;
  }

  ULO outputUntilX = untilPosition.GetUnwrappedPixel();
  if (outputUntilX == _lastOutputX)
  {
    AddDuplicateLogEntry(outputLine, _lastOutputX);
    // Multiple commits can happen on the same emulated cycle by CPU writes not coordinated with chipset writes.
    return;
  }

  if (outputUntilX < _lastOutputX)
  {
    F_ASSERT(false);
  }

  ULO pixelCount = outputUntilX - _lastOutputX;

  AddOutputUntilLogEntry(outputLine, _lastOutputX + 1, pixelCount);

  PerformanceCounter->Start();

  if (!chipset_info.GfxDebugDisableShifter)
  {
    if (_activated)
    {
      ShiftBitplaneBatch(pixelCount);
    }
    else
    {
      ShiftBackgroundBatch(pixelCount);
    }

    cycle_exact_sprites->OutputSprites(_lastOutputX + 1, pixelCount);

    if (chipset_info.GfxDebugImmediateRendering)
    {
      host_frame_immediate_renderer.DrawBatchImmediate(outputLine, _lastOutputX + 1);
      Planar2ChunkyDecoder::NewImmediateBatch();
    }
    else if (outputUntilX < (_scheduler->GetCyclesInLine() + _scheduler->GetHorisontalBlankStart() - 1))
    {
      // Not at end of line, initialize next batch
      NewChangelistBatch(outputLine, outputUntilX + 1);
    }
  }

  _lastOutputX = outputUntilX;

  PerformanceCounter->Stop();
}

void BitplaneShifter::HandleEvent()
{
  bitplane_shifter.Handler();
}

// Finishes the output of the current line, arrives some time after the line has ended
// Called on the exact shres position the line ends
// First call in a frame is one line before the display starts, to set up changelists
void BitplaneShifter::Handler()
{
  shifterEvent.Disable();

  const SHResTimestamp &shresPosition = _scheduler->GetSHResTimestamp();

  if (shresPosition.Line == _scheduler->GetVerticalBlankEnd())
  {
    // Finishing last line in vertical blank, pixel output starts on next line

    // TODO: Verify nothing exists in the list
    // TODO: Verify nothing exists in the arm list
    if (!chipset_info.GfxDebugImmediateRendering)
    {
      NewChangelistBatch(shresPosition.Line, shresPosition.Pixel + 1);
    }
    SetupEvent(shresPosition.Line + 1, _scheduler->GetHorisontalBlankStart() - 1);
    return;
  }

  Flush(shresPosition);

  InitializeNewLine(shresPosition);
}

void BitplaneShifter::InitializeNewLine(const SHResTimestamp &currentSHResPosition)
{
  _activated = false;
  _lastOutputX = _scheduler->GetHorisontalBlankStart() - 1;
  _armList.Clear();

  if (currentSHResPosition.Line == 0)
  {
    InitializeNewFrame();
    return;
  }

  if (!chipset_info.GfxDebugImmediateRendering)
  {
    NewChangelistBatch(currentSHResPosition.Line, currentSHResPosition.Pixel + 1);
  }

  SetupEvent(currentSHResPosition.Line + 1, currentSHResPosition.Pixel);
}

void BitplaneShifter::InitializeNewFrame()
{
  if (!chipset_info.GfxDebugImmediateRendering)
  {
    host_frame_delayed_renderer.DrawFrameFromChangeLists();
  }

  if (chipset_info.GfxDebugImmediateRenderingFromConfig != chipset_info.GfxDebugImmediateRendering)
  {
    chipset_info.GfxDebugImmediateRendering = chipset_info.GfxDebugImmediateRenderingFromConfig;
  }

  if (!chipset_info.GfxDebugImmediateRendering)
  {
    Planar2ChunkyDecoder::InitializeNewFrame();
    host_frame_delayed_renderer.ChangeList.InitializeNewFrame(_scheduler->GetLinesInFrame());
  }
  else
  {
    host_frame_immediate_renderer.InitializeNewFrame(_scheduler->GetLinesInFrame());
  }

  Draw.EndOfFrame();

  UpdateDrawBatchFunc();

  SetupEvent(_scheduler->GetVerticalBlankEnd(), _scheduler->GetHorisontalBlankStart() - 1);
}

void BitplaneShifter::UpdateDrawBatchFunc()
{
  if (chipset_info.GfxDebugImmediateRendering)
  {
    host_frame_immediate_renderer.UpdateDrawBatchFunc();
  }
  else
  {
    host_frame_delayed_renderer.UpdateDrawBatchFunc();
  }
}

void BitplaneShifter::NewChangelistBatch(ULO line, ULO pixel) const
{
  Planar2ChunkyDecoder::NewChangelistBatch();
  host_frame_delayed_renderer.ChangeList.AddBufferChange(
      line,
      pixel,
      Planar2ChunkyDecoder::GetOddPlayfieldStart<UBY>(),
      Planar2ChunkyDecoder::GetEvenPlayfieldStart<UBY>(),
      Planar2ChunkyDecoder::GetHamSpritesPlayfieldStart<UBY>());
}

void BitplaneShifter::Clear()
{
  _lastOutputX = _scheduler->GetHorisontalBlankStart() - 1;
  _armList.Clear();
  _activated = false;

  for (auto &i : _input)
  {
    i.w = 0;
  }

  Planar2ChunkyDecoder::InitializeNewFrame();
  if (chipset_info.GfxDebugImmediateRendering)
  {
    host_frame_immediate_renderer.UpdateDrawBatchFunc();
  }
  else
  {
    host_frame_delayed_renderer.UpdateDrawBatchFunc();
    host_frame_delayed_renderer.ChangeList.InitializeNewFrame(_scheduler->GetLinesInFrame());
  }
}

void BitplaneShifter::Reset()
{
  Clear();
  SetupEvent(_scheduler->GetVerticalBlankEnd(), _scheduler->GetHorisontalBlankStart() - 1);
}

/* Fellow events */

void BitplaneShifter::EndOfFrame()
{
  if (shifterEvent.IsEnabled())
  {
    _scheduler->RemoveEvent(&shifterEvent);
  }

  SetupEvent(0, _scheduler->GetHorisontalBlankStart() - 1);
}

void BitplaneShifter::SoftReset()
{
  Reset();
}

void BitplaneShifter::HardReset()
{
  Reset();
}

void BitplaneShifter::EmulationStart()
{
}

void BitplaneShifter::EmulationStop()
{
}

void BitplaneShifter::Startup()
{
  PerformanceCounter = Service->PerformanceCounterFactory.Create("BitplaneShifter");
  Clear();

  _bitplaneRegisters->SetAddDataCallback(new BitplaneShifter_AddDataCallback(this));
  _bitplaneRegisters->SetFlushCallback(new BitplaneShifter_FlushCallback(this));
}

void BitplaneShifter::Shutdown()
{
  delete PerformanceCounter;
  PerformanceCounter = nullptr;
}

BitplaneShifter::BitplaneShifter(Scheduler *scheduler, BitplaneRegisters *bitplaneRegisters)
  : _scheduler(scheduler), _bitplaneRegisters(bitplaneRegisters), PerformanceCounter(nullptr)
{
}

BitplaneShifter::~BitplaneShifter()
{
  delete PerformanceCounter;
}
