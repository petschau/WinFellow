/*=========================================================================*/
/* Fellow                                                                  */
/* Renders Amiga pixels as host pixels using a change list                 */
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

#pragma once

#include <sstream>

#include "fellow/api/defs.h"
#include "fellow/api/service/IPerformanceCounter.h"
#include "fellow/chipset/BitplaneShifter.h"

class HostFrameDelayedRenderer;
typedef void (HostFrameDelayedRenderer::*DelayedDrawBatchFunc)(ULO line, ULO startX, ULO pixelCount);

enum ChangeReferenceType
{
  ColorChange = 0,
  BufferChange = 1
};

struct ChangeReference
{
  ChangeReferenceType Type;
  ULO Timestamp;
  void *Payload;

  template <typename T> T *GetPayloadAs() const
  {
    return (T *)Payload;
  }

  const char *GetTypeName() const
  {
    switch (Type)
    {
      case ChangeReferenceType::BufferChange: return "BufferChange";
      case ChangeReferenceType::ColorChange: return "ColorChange";
      default: return "Unknown";
    }
  }

  void ToStream(std::ostream &os) const;
};

struct ColorChangeItem
{
  unsigned int Index;
  UWO Value;
  UWO Value2;

  void ToStream(std::ostream &os) const;
};

struct BufferChangeItem
{
  BitplaneShiftMode ShiftMode;
  UBY *OddPlayfield;
  UBY *EvenPlayfield;
  UBY *HamSpritePlayfield;
  DelayedDrawBatchFunc DrawFunc;

  void ToStream(std::ostream &os) const;
};

constexpr auto LineChangesSize = 256;
struct LineChanges
{
  unsigned int Changes[LineChangesSize];
  unsigned int Count;

  void Add(unsigned int changeItemIndex)
  {
    Changes[Count] = changeItemIndex;
    Count++;
  }
  void Clear()
  {
    Count = 0;
  }
};

constexpr auto ChangeReferencePoolSize = 65536;
template <typename T> struct ChangePool
{
  T Items[ChangeReferencePoolSize];
  unsigned int Count;

  unsigned int GetNew()
  {
    unsigned int nextIndex = Count;
    Count++;
    return nextIndex;
  }
  T *Get(unsigned int index)
  {
    return &Items[index];
  }
  void Clear()
  {
    Count = 0;
  }
};

// ChangeList -> LineChanges[line] -> Changes[Count] -> ChangeReference -> ColorChange or BufferChange
struct ChangeList
{
  unsigned int LinesInFrame;
  ChangePool<ColorChangeItem> ColorChangePool;
  ChangePool<BufferChangeItem> BufferChangePool;
  ChangePool<ChangeReference> ChangeReferencePool;

  LineChanges Changes[314];

  void AddChangeReference(ChangeReferenceType type, ULO line, ULO timestamp, void *payload);
  void AddColorChange(ULO line, ULO timestamp, unsigned int index, UWO value, UWO halfbriteValue);
  void AddBufferChange(ULO line, ULO timestamp, UBY *oddPlayfield, UBY *evenPlayfield, UBY *hamSpritePlayfield);
  void InitializeNewFrame(unsigned int linesInFrame);

  ChangeReference *Get(unsigned int line, unsigned int index)
  {
    const LineChanges &lineChanges = Changes[line];
    return (index >= lineChanges.Count) ? nullptr : ChangeReferencePool.Get(lineChanges.Changes[index]);
  }
};

class HostFrameDelayedRenderer
{
private:
  DelayedDrawBatchFunc _currentDrawBatchFunc;
  BufferChangeItem *_currentPlayfieldBuffers;
  unsigned int _currentPlayfieldBufferIndex;
  unsigned int _currentChangeItemIndex;
  unsigned int _currentPixelCountShift;
  unsigned int _lastX;

  static ULO GetPixelCountDividerForShiftMode(BitplaneShiftMode mode);

  static void SetPixel1xULL(ULL *dst, ULL hostColor);
  static void SetPixel2xULL(ULL *dst, ULL hostColor);
  static void SetPixel4xULO(ULO *dst, ULO hostColor);

  void DrawLoresChangelist(ULO line, ULO startX, ULO pixelCount);
  void DrawLoresDualChangelist(ULO line, ULO startX, ULO pixelCount);
  void DrawLoresHamChangelist(ULO line, ULO startX, ULO pixelCount);
  void DrawHiresChangelist(ULO line, ULO startX, ULO pixelCount);
  void DrawHiresDualChangelist(ULO line, ULO startX, ULO pixelCount);
  void DrawNothingChangelist(ULO line, ULO startX, ULO pixelCount);

  ChangeReference *ProcessChangeList(unsigned int line);
  void DrawLineFromChangeList(ULO line);

public:
  fellow::api::IPerformanceCounter *PerformanceCounter;
  ChangeList ChangeList;

  void ToStream(std::ostringstream &os);
  void ToStream(std::ostringstream &os, unsigned int line);

  void UpdateDrawBatchFunc();
  DelayedDrawBatchFunc GetDrawBatchFunc() const
  {
    return _currentDrawBatchFunc;
  }

  void DrawFrameFromChangeLists();

  void Startup();
  void Shutdown();

  HostFrameDelayedRenderer();
  ~HostFrameDelayedRenderer();
};

extern HostFrameDelayedRenderer host_frame_delayed_renderer;
