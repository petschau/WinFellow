/*=========================================================================*/
/* Fellow                                                                  */
/* Translates Amiga pixels to host pixels using change lists               */
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

#include "fellow/chipset/HostFrameDelayedRenderer.h"
#include "fellow/chipset/HostFrame.h"
#include "fellow/chipset/DualPlayfieldMapper.h"
#include "fellow/chipset/Planar2ChunkyDecoder.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/api/Services.h"

#include "fellow/application/HostRenderer.h"
#include "fellow/scheduler/Scheduler.h"

#include <sstream>
#include <iomanip>

using namespace std;
using namespace fellow::api;

extern ULO draw_HAM_modify_table[4][2];

constexpr auto drawHAMModifyTableBitIndex = 0;
constexpr auto drawHAMModifyTableHoldMask = 4;

HostFrameDelayedRenderer host_frame_delayed_renderer;

void ChangeList::AddChangeReference(ChangeReferenceType type, ULO line, ULO timestamp, void *payload)
{
  unsigned int itemIndex = ChangeReferencePool.GetNew();
  ChangeReference *item = ChangeReferencePool.Get(itemIndex);
  item->Type = type;
  item->Timestamp = timestamp;
  item->Payload = payload;

  Changes[line].Add(itemIndex);
}

void ChangeList::AddColorChange(ULO line, ULO timestamp, unsigned int index, UWO value, UWO halfbriteValue)
{
  unsigned int itemIndex = ColorChangePool.GetNew();
  ColorChangeItem *newColor = ColorChangePool.Get(itemIndex);
  newColor->Index = index;
  newColor->Value = value;
  newColor->Value2 = halfbriteValue;
  AddChangeReference(ChangeReferenceType::ColorChange, line, timestamp, newColor);
}

void ChangeList::AddBufferChange(ULO line, ULO timestamp, UBY *oddPlayfield, UBY *evenPlayfield, UBY *hamSpritePlayfield)
{
  unsigned int itemIndex = BufferChangePool.GetNew();
  BufferChangeItem *newBuffer = BufferChangePool.Get(itemIndex);
  newBuffer->OddPlayfield = oddPlayfield;
  newBuffer->EvenPlayfield = evenPlayfield;
  newBuffer->HamSpritePlayfield = hamSpritePlayfield;
  newBuffer->DrawFunc = host_frame_delayed_renderer.GetDrawBatchFunc();
  newBuffer->ShiftMode = BitplaneUtility::IsHires() ? SHIFTMODE_70NS : SHIFTMODE_140NS;
  AddChangeReference(ChangeReferenceType::BufferChange, line, timestamp, newBuffer);
}

void ChangeList::InitializeNewFrame(unsigned int linesInFrame)
{
  LinesInFrame = linesInFrame;
  ChangeReferencePool.Clear();
  ColorChangePool.Clear();
  BufferChangePool.Clear();

  for (auto &lineChanges : Changes)
  {
    lineChanges.Clear();
  }
}

void HostFrameDelayedRenderer::SetPixel1xULL(ULL *dst, ULL hostColor)
{
  *dst = hostColor;
}

void HostFrameDelayedRenderer::SetPixel2xULL(ULL *dst, ULL hostColor)
{
  dst[0] = hostColor;
  dst[1] = hostColor;
}

void HostFrameDelayedRenderer::SetPixel4xULO(ULO *dst, ULO hostColor)
{
  dst[0] = hostColor;
  dst[1] = hostColor;
  dst[2] = hostColor;
  dst[3] = hostColor;
}

void HostFrameDelayedRenderer::DrawLoresChangelist(ULO line, ULO startX, ULO pixelCount)
{
  ULL *tmpline = (ULL *)(host_frame.GetLine(line) + startX);
  UBY *playfield = _currentPlayfieldBuffers->OddPlayfield + _currentPlayfieldBufferIndex;
  const ULL *hostColors = Draw.GetHostColors();

  for (ULO i = 0; i < pixelCount; i++)
  {
    // Each lores pixel is 4 host pixels wide (shres host scale)
    SetPixel2xULL(tmpline, hostColors[playfield[i]]);
    tmpline += 2;
  }
}

void HostFrameDelayedRenderer::DrawLoresDualChangelist(ULO line, ULO startX, ULO pixelCount)
{
  DualPlayfieldMapper dualPlayfieldMapper(BitplaneUtility::IsPlayfield1Pri());
  ULL *tmpline = (ULL *)(host_frame.GetLine(line) + startX);

  const ULL *hostColors = Draw.GetHostColors();
  UBY *playfieldOdd = _currentPlayfieldBuffers->OddPlayfield + _currentPlayfieldBufferIndex;
  UBY *playfieldEven = _currentPlayfieldBuffers->EvenPlayfield + _currentPlayfieldBufferIndex;

  for (ULO i = 0; i < pixelCount; i++)
  {
    // Each lores pixel is 4 host pixels wide (shres host scale)
    SetPixel2xULL(tmpline, hostColors[dualPlayfieldMapper.Map(playfieldOdd[i], playfieldEven[i])]);
    tmpline += 2;
  }
}

void HostFrameDelayedRenderer::DrawLoresHamChangelist(ULO line, ULO startX, ULO pixelCount)
{
  ULO *tmpline = host_frame.GetLine(line) + startX;
  static ULO pixelColor;
  ULO outputColor;
  UBY *playfield = _currentPlayfieldBuffers->OddPlayfield + _currentPlayfieldBufferIndex;
  UBY *spritePlayfield = _currentPlayfieldBuffers->HamSpritePlayfield + _currentPlayfieldBufferIndex;
  const ULL *hostColors = Draw.GetHostColors();

  for (ULO i = 0; i < pixelCount; i++)
  {
    UBY amigaColorIndex = playfield[i];
    UBY controlBits = amigaColorIndex & 0x30;
    if (controlBits == 0)
    {
      pixelColor = outputColor = (ULO)hostColors[amigaColorIndex];
    }
    else
    {
      UBY *holdMask = (UBY *)draw_HAM_modify_table + (controlBits >> 1);
      ULO bitIndex = *(ULO *)(holdMask + drawHAMModifyTableBitIndex);
      pixelColor &= *(ULO *)(holdMask + drawHAMModifyTableHoldMask);
      pixelColor |= (amigaColorIndex & 0xf) << (bitIndex & 0xff);
      outputColor = pixelColor;
    }

    if (spritePlayfield[i] != 0)
    {
      outputColor = (ULO)hostColors[spritePlayfield[i]];
    }

    // Each lores pixel is 4 host pixels wide (shres host scale)
    SetPixel4xULO(tmpline, outputColor);
    tmpline += 4;
  }
}

void HostFrameDelayedRenderer::DrawHiresChangelist(ULO line, ULO startX, ULO pixelCount)
{
  ULL *tmpline = (ULL *)(host_frame.GetLine(line) + startX);
  UBY *playfield = _currentPlayfieldBuffers->OddPlayfield + _currentPlayfieldBufferIndex;
  const ULL *hostColors = Draw.GetHostColors();

  for (ULO i = 0; i < pixelCount; i++)
  {
    SetPixel1xULL(tmpline + i, hostColors[playfield[i]]);
  }
}

void HostFrameDelayedRenderer::DrawHiresDualChangelist(ULO line, ULO startX, ULO pixelCount)
{
  DualPlayfieldMapper dualPlayfieldMapper(BitplaneUtility::IsPlayfield1Pri());
  ULL *tmpline = (ULL *)(host_frame.GetLine(line) + startX);

  const ULL *hostColors = Draw.GetHostColors();
  UBY *playfieldOdd = _currentPlayfieldBuffers->OddPlayfield + _currentPlayfieldBufferIndex;
  UBY *playfieldEven = _currentPlayfieldBuffers->EvenPlayfield + _currentPlayfieldBufferIndex;
  for (ULO i = 0; i < pixelCount; i++)
  {
    SetPixel1xULL(tmpline + i, hostColors[dualPlayfieldMapper.Map(playfieldOdd[i], playfieldEven[i])]);
  }
}

void HostFrameDelayedRenderer::DrawNothingChangelist(ULO line, ULO startX, ULO pixelCount)
{
  ULL *tmpline = (ULL *)(host_frame.GetLine(line) + startX);
  const ULL pixelColor = Draw.GetHostColors()[0];

  for (ULO i = 0; i < pixelCount; i++)
  {
    SetPixel2xULL(tmpline, pixelColor);
    tmpline += 2;
  }
}

void HostFrameDelayedRenderer::UpdateDrawBatchFunc()
{
  if (BitplaneUtility::IsHires())
  {
    if (BitplaneUtility::IsDualPlayfield())
    {
      _currentDrawBatchFunc = &HostFrameDelayedRenderer::DrawHiresDualChangelist;
    }
    else
    {
      _currentDrawBatchFunc = &HostFrameDelayedRenderer::DrawHiresChangelist;
    }
  }
  else
  {
    if (BitplaneUtility::IsDualPlayfield())
    {
      _currentDrawBatchFunc = &HostFrameDelayedRenderer::DrawLoresDualChangelist;
    }
    else if (BitplaneUtility::IsHam())
    {
      _currentDrawBatchFunc = &HostFrameDelayedRenderer::DrawLoresHamChangelist;
    }
    else
    {
      _currentDrawBatchFunc = &HostFrameDelayedRenderer::DrawLoresChangelist;
    }
  }
}

ULO HostFrameDelayedRenderer::GetPixelCountDividerForShiftMode(BitplaneShiftMode mode)
{
  switch (mode)
  {
    case BitplaneShiftMode::SHIFTMODE_140NS: return 2;
    case BitplaneShiftMode::SHIFTMODE_70NS: return 1;
    default: return 0;
  }
}

void HostFrameDelayedRenderer::DrawFrameFromChangeLists()
{
  PerformanceCounter->Start();

#ifdef _DEBUG

  // Assert no recorded changes in vertical blank
  for (unsigned int line = 0; line < scheduler.GetVerticalBlankEnd(); line++)
  {
    auto *change = ChangeList.Get(line, 0);
    F_ASSERT(change == nullptr);
  }

#endif

  if (!chipset_info.GfxDebugDisableRenderer)
  {
    ostringstream oss;
    ToStream(oss);

    // FILE *F = fopen("c:\\temp\\changelists.txt", "w+");
    // fputs(oss.str().c_str(), F);
    // fclose(F);
  }

  PerformanceCounter->Stop();
}

void HostFrameDelayedRenderer::DrawLineFromChangeList(ULO line)
{
  // ChangeList always has at least one entry, the initial playfield buffers to read from
  // Assume this is always the first entry

  ULO currentX = scheduler.GetHorisontalBlankStart();
  _lastX = scheduler.GetCyclesInLine() + scheduler.GetHorisontalBlankStart();
  _currentChangeItemIndex = 0;

  do
  {
    ChangeReference *nextChangeItem = ProcessChangeList(line);
    ULO nextChangeX = (nextChangeItem != nullptr) ? nextChangeItem->Timestamp : _lastX;

    // Render from the playfield buffer until next change
    ULO shresPixelCount = nextChangeX - currentX;
    ULO playfieldBufferPixelCount = shresPixelCount >> _currentPixelCountShift;

    (this->*_currentPlayfieldBuffers->DrawFunc)(line, currentX, playfieldBufferPixelCount);

    _currentPlayfieldBufferIndex += playfieldBufferPixelCount;
    currentX += shresPixelCount;
  } while (currentX < _lastX);
}

ChangeReference *HostFrameDelayedRenderer::ProcessChangeList(unsigned int line)
{
  ChangeReference *changeReference = ChangeList.Get(line, _currentChangeItemIndex);
  ULO changeX = changeReference->Timestamp;

  do
  {
    if (changeReference->Type == ChangeReferenceType::ColorChange)
    {
      auto *colorChangeItem = changeReference->GetPayloadAs<ColorChangeItem>();
      bitplane_registers.PublishColorChanged(colorChangeItem->Index, colorChangeItem->Value, colorChangeItem->Value2);
    }
    else if (changeReference->Type == ChangeReferenceType::BufferChange)
    {
      _currentPlayfieldBuffers = changeReference->GetPayloadAs<BufferChangeItem>();
      _currentPixelCountShift = GetPixelCountDividerForShiftMode(_currentPlayfieldBuffers->ShiftMode);
      _currentPlayfieldBufferIndex = 0;
    }

    changeReference = ChangeList.Get(line, ++_currentChangeItemIndex);
  } while (changeReference != nullptr && changeReference->Timestamp == changeX);

  return changeReference;
}

void HostFrameDelayedRenderer::ToStream(ostringstream &os)
{
  unsigned int firstLineInFrame = scheduler.GetVerticalBlankEnd();
  unsigned int linesInFrame = ChangeList.LinesInFrame;

  os << "Frame changelist:" << endl;
  os << "VerticalBlankEnd: " << firstLineInFrame << endl;
  os << "LinesInFrame: " << linesInFrame << endl;

  for (auto line = 0; line < ChangeList.LinesInFrame; line++)
  {
    ToStream(os, line);
  }
}

void HostFrameDelayedRenderer::ToStream(ostringstream &os, unsigned int line)
{
  unsigned int count = ChangeList.Changes[line].Count;

  os << "Line " << setw(3) << line << endl << "---------" << endl;
  for (unsigned int j = 0; j < count; j++)
  {
    unsigned int itemIndex = ChangeList.Changes[line].Changes[j];

    os << "Line item " << setw(4) << j << " (" << setw(4) << itemIndex << ")";

    ChangeReference *item = ChangeList.Get(line, j);

    if (item != nullptr)
    {

      os << endl;
    }
    else
    {
      os << ": Null!" << endl;
    }
  }

  os << endl;
}

void HostFrameDelayedRenderer::Startup()
{
  PerformanceCounter = Service->PerformanceCounterFactory.Create("Delayed renderer");
}

void HostFrameDelayedRenderer::Shutdown()
{
}

HostFrameDelayedRenderer::HostFrameDelayedRenderer() : PerformanceCounter(nullptr), _currentDrawBatchFunc(&HostFrameDelayedRenderer::DrawLoresChangelist)
{
}

HostFrameDelayedRenderer::~HostFrameDelayedRenderer()
{
  delete PerformanceCounter;
}

void ChangeReference::ToStream(ostream &os) const
{
  os << fixed << "(" << setw(4) << Timestamp << "/" << setw(5) << setprecision(1) << ((float)Timestamp) / 2.0 << "/" << setw(6) << setprecision(2) << ((float)Timestamp) / 4.0 << "):";

  if (Type == ChangeReferenceType::ColorChange)
  {
    ColorChangeItem *colorChangeItem = GetPayloadAs<ColorChangeItem>();
    colorChangeItem->ToStream(os);
  }
  else if (Type == ChangeReferenceType::BufferChange)
  {
    BufferChangeItem *bufferChangeItem = GetPayloadAs<BufferChangeItem>();
    bufferChangeItem->ToStream(os);
  }
}

void BufferChangeItem::ToStream(ostream &os) const
{
  os << "BufferChange";
  os << " ShiftMode = " << ShiftMode;
  os << " DrawFunc = " << DrawFunc;
  os << " EvenPlayfield = " << EvenPlayfield;
  os << " OddPlayfield=" << OddPlayfield;
  os << " HamSpritePlayfield=" << HamSpritePlayfield;
}

void ColorChangeItem::ToStream(ostream &os) const
{
  os << "ColorChange";
  os << " Color " << Index << "=" << Value;
}
