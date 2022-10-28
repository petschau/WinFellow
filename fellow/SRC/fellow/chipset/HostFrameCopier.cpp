/*=========================================================================*/
/* Fellow                                                                  */
/* Bitplane draw (in host buffer)                                          */
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

#include "fellow/api/Services.h"
#include "fellow/chipset/HostFrameCopier.h"
#include "fellow/chipset/ChipsetInfo.h"

using namespace fellow::api;

HostFrameCopier host_frame_copier;

void HostFrameCopier::DrawNothing_1X_1X(const unsigned int pixelCountShres, uint64_t *bufferPtr, const uint64_t padColor)
{
  const unsigned int iterations = pixelCountShres / 4;

  for (unsigned int index = 0; index < iterations; index++)
  {
    bufferPtr[index] = padColor;
  }

  if ((pixelCountShres & 3) == 0x2)
  {
    *(uint32_t *)(bufferPtr + iterations) = (uint32_t)padColor;
  }
}

void HostFrameCopier::DrawNothing_1X_2X(const unsigned int pixelCountShres, uint64_t *bufferFirstLinePtr, uint64_t *bufferSecondLinePtr, const uint64_t padColor)
{
  const unsigned int iterations = pixelCountShres / 4;

  for (unsigned int index = 0; index < iterations; index++)
  {
    bufferFirstLinePtr[index] = padColor;
    bufferSecondLinePtr[index] = padColor;
  }

  if ((pixelCountShres & 3) == 0x2)
  {
    *(uint32_t *)(bufferFirstLinePtr + iterations) = (uint32_t)padColor;
    *(uint32_t *)(bufferSecondLinePtr + iterations) = (uint32_t)padColor;
  }
}

void HostFrameCopier::DrawNothing_2X_2X(const unsigned int pixelCountShres, uint64_t *bufferFirstLinePtr, uint64_t *bufferSecondLinePtr, const uint64_t padColor)
{
  const unsigned int iterations = pixelCountShres / 2;

  for (unsigned int index = 0; index < iterations; index++)
  {
    bufferFirstLinePtr[index] = padColor;
    bufferSecondLinePtr[index] = padColor;
  }

  if (pixelCountShres & 1)
  {
    *(uint32_t *)(bufferFirstLinePtr + iterations) = (uint32_t)padColor;
    *(uint32_t *)(bufferSecondLinePtr + iterations) = (uint32_t)padColor;
  }
}

void HostFrameCopier::DrawNothing_2X_4X(
    const unsigned int pixelCountShres, uint64_t *bufferFirstLinePtr, uint64_t *bufferSecondLinePtr, uint64_t *bufferThirdLinePtr, uint64_t *bufferFourthLinePtr, const uint64_t padColor)
{
  const unsigned int iterations = pixelCountShres / 2;

  for (unsigned int index = 0; index < iterations; index++)
  {
    bufferFirstLinePtr[index] = padColor;
    bufferSecondLinePtr[index] = padColor;
    bufferThirdLinePtr[index] = padColor;
    bufferFourthLinePtr[index] = padColor;
  }

  if (pixelCountShres & 1)
  {
    *(uint32_t *)(bufferFirstLinePtr + iterations) = (uint32_t)padColor;
    *(uint32_t *)(bufferSecondLinePtr + iterations) = (uint32_t)padColor;
    *(uint32_t *)(bufferThirdLinePtr + iterations) = (uint32_t)padColor;
    *(uint32_t *)(bufferFourthLinePtr + iterations) = (uint32_t)padColor;
  }
}

uint64_t HostFrameCopier::CombineTwoPixels1X(const uint32_t *source)
{
  return ((uint64_t)source[0]) | (((uint64_t)source[2]) << 32);
}

// hires width, source line expands to 1 host line
void HostFrameCopier::DrawLineInHost_1X_1X(const unsigned int pixelCountShres, const uint32_t *source, uint64_t *bufferFirstLinePtr)
{
  const unsigned int iterations = pixelCountShres / 4;

  for (unsigned int index = 0; index < iterations; index++)
  {
    bufferFirstLinePtr[index] = CombineTwoPixels1X(source);
    source += 4;
  }
}

// hires width, source line expands to 2 host lines
void HostFrameCopier::DrawLineInHost_1X_2X(const unsigned int pixelCountShres, const uint32_t *source, uint64_t *bufferFirstLinePtr, uint64_t *bufferSecondLinePtr)
{
  const unsigned int iterations = pixelCountShres / 4;

  for (unsigned int index = 0; index < iterations; index++)
  {
    const uint64_t pixelColor = CombineTwoPixels1X(source);

    bufferFirstLinePtr[index] = pixelColor;
    bufferSecondLinePtr[index] = pixelColor;
    source += 4;
  }
}

// shres width, source line expands to 2 host lines
void HostFrameCopier::DrawLineInHost_2X_2X(const unsigned int pixelCountShres, const uint32_t *source, uint64_t *bufferFirstLinePtr, uint64_t *bufferSecondLinePtr)
{
  uint64_t *source_uint64 = (uint64_t *)source;
  const unsigned int iterations = pixelCountShres / 2;

  for (unsigned int index = 0; index < iterations; index++)
  {
    const uint64_t pixelColor = *source_uint64++;

    bufferFirstLinePtr[index] = pixelColor;
    bufferSecondLinePtr[index] = pixelColor;
  }

  if (pixelCountShres & 1)
  {
    const uint32_t pixelColor = *(uint32_t *)source_uint64;

    *(uint32_t *)(bufferFirstLinePtr + iterations) = pixelColor;
    *(uint32_t *)(bufferSecondLinePtr + iterations) = pixelColor;
  }
}

// shres width, source line expands to 4 host lines
void HostFrameCopier::DrawLineInHost_2X_4X(
    const unsigned int pixelCountShres, const uint32_t *source, uint64_t *bufferFirstLinePtr, uint64_t *bufferSecondLinePtr, uint64_t *bufferThirdLinePtr, uint64_t *bufferFourthLinePtr)
{
  const uint64_t *source_uint64 = (const uint64_t *)source;
  const unsigned int iterations = pixelCountShres / 2;

  for (unsigned int index = 0; index < iterations; index++)
  {
    const uint64_t pixelColor = *source_uint64++;

    bufferFirstLinePtr[index] = pixelColor;
    bufferSecondLinePtr[index] = pixelColor;
    bufferThirdLinePtr[index] = pixelColor;
    bufferFourthLinePtr[index] = pixelColor;
  }

  if (pixelCountShres & 1)
  {
    const uint32_t pixelColor = *(uint32_t *)source_uint64;

    *(uint32_t *)(bufferFirstLinePtr + iterations) = pixelColor;
    *(uint32_t *)(bufferSecondLinePtr + iterations) = pixelColor;
    *(uint32_t *)(bufferThirdLinePtr + iterations) = pixelColor;
    *(uint32_t *)(bufferFourthLinePtr + iterations) = pixelColor;
  }
}

void HostFrameCopier::IncreaseBufferPtrs(uint64_t *&bufferFirstLinePtr, uint64_t *&bufferSecondLinePtr, const ptrdiff_t add)
{
  bufferFirstLinePtr += add;
  bufferSecondLinePtr += add;
}

void HostFrameCopier::IncreaseBufferPtrs(uint64_t *&bufferFirstLinePtr, uint64_t *&bufferSecondLinePtr, uint64_t *&bufferThirdLinePtr, uint64_t *&bufferFourthLinePtr, const ptrdiff_t add)
{
  bufferFirstLinePtr += add;
  bufferSecondLinePtr += add;
  bufferThirdLinePtr += add;
  bufferFourthLinePtr += add;
}

void HostFrameCopier::DrawFrameInHost1X(HostFrame &frame, const MappedChipsetFramebuffer &mappedChipsetFramebuffer, const RectShresi &outputClipShresi, const uint64_t padColor)
{
  const ptrdiff_t hostLinePitch = mappedChipsetFramebuffer.HostLinePitch;
  const ptrdiff_t amigaLinePitchUint64 = mappedChipsetFramebuffer.AmigaLinePitch / 8;

  unsigned int startLine = outputClipShresi.Top / 2;
  const unsigned int bottomLine = outputClipShresi.Bottom / 2; // First line after the bottom if shres bottom clip is even, contains one half height line if clip is odd
  const unsigned int linePixelCountShres = outputClipShresi.GetWidth();

  const uint8_t *startOfFramePtr = mappedChipsetFramebuffer.TopPointer;
  uint64_t *bufferFirstLinePtr = (uint64_t *)startOfFramePtr;
  uint64_t *bufferSecondLinePtr = (uint64_t *)(startOfFramePtr + hostLinePitch);

  // In case first line is half height in host window due to clipping
  if (outputClipShresi.Top & 1)
  {
    DrawLineInHost_1X_1X(linePixelCountShres, frame.GetLine(startLine) + outputClipShresi.Left, bufferFirstLinePtr);
    IncreaseBufferPtrs(bufferFirstLinePtr, bufferSecondLinePtr, amigaLinePitchUint64 / 2);
    startLine++;
  }

  // Full height lines
  const unsigned int maxLineWithData = std::min<unsigned int>(bottomLine, frame.LinesInFrame);
  for (unsigned int line = startLine; line < maxLineWithData; line++)
  {
    DrawLineInHost_1X_2X(linePixelCountShres, frame.GetLine(line) + outputClipShresi.Left, bufferFirstLinePtr, bufferSecondLinePtr);
    IncreaseBufferPtrs(bufferFirstLinePtr, bufferSecondLinePtr, amigaLinePitchUint64);
  }

  // Padding on the bottom beyond actual lines in the frame (long, short frames, or in case the clip is too large)
  if (frame.LinesInFrame < bottomLine)
  {
    for (unsigned int line = maxLineWithData; line < bottomLine; line++)
    {
      DrawNothing_1X_2X(linePixelCountShres, bufferFirstLinePtr, bufferSecondLinePtr, padColor);
      IncreaseBufferPtrs(bufferFirstLinePtr, bufferSecondLinePtr, amigaLinePitchUint64);
    }
  }

  // In case last line is half height in host window due to clipping
  if (outputClipShresi.Bottom & 1)
  {
    if (frame.LinesInFrame <= bottomLine)
    {
      DrawNothing_1X_1X(linePixelCountShres, bufferFirstLinePtr, padColor);
    }
    else
    {
      DrawLineInHost_1X_1X(linePixelCountShres, frame.GetLine(bottomLine) + outputClipShresi.Left, bufferFirstLinePtr);
    }
  }
}

void HostFrameCopier::DrawFrameInHost2X(HostFrame &frame, const MappedChipsetFramebuffer &mappedChipsetFramebuffer, const RectShresi &outputClipShresi, const uint64_t padColor)
{
  const ptrdiff_t hostLinePitch = mappedChipsetFramebuffer.HostLinePitch;
  const ptrdiff_t amigaLinePitchUint64 = mappedChipsetFramebuffer.AmigaLinePitch / 8;

  unsigned int startLine = outputClipShresi.Top / 2;
  const unsigned int bottomLine = outputClipShresi.Bottom / 2; // First line after the bottom if shres bottom clip is even, contains one half height line if clip is odd
  const unsigned int linePixelCountShres = outputClipShresi.GetWidth();

  uint8_t *startOfFramePtr = mappedChipsetFramebuffer.TopPointer;
  uint64_t *bufferFirstLinePtr = (uint64_t *)startOfFramePtr;
  uint64_t *bufferSecondLinePtr = (uint64_t *)(startOfFramePtr + hostLinePitch);
  uint64_t *bufferThirdLinePtr = (uint64_t *)(startOfFramePtr + 2 * hostLinePitch);
  uint64_t *bufferFourthLinePtr = (uint64_t *)(startOfFramePtr + 3 * hostLinePitch);

  // In case first line is half height in host window due to clipping
  if (outputClipShresi.Top & 1)
  {
    DrawLineInHost_2X_2X(linePixelCountShres, frame.GetLine(startLine) + outputClipShresi.Left, bufferFirstLinePtr, bufferSecondLinePtr);
    IncreaseBufferPtrs(bufferFirstLinePtr, bufferSecondLinePtr, bufferThirdLinePtr, bufferFourthLinePtr, amigaLinePitchUint64 / 2);
    startLine++;
  }

  // Full height lines
  const unsigned int maxLineWithData = std::min<unsigned int>(bottomLine, frame.LinesInFrame);
  for (unsigned int line = startLine; line < maxLineWithData; line++)
  {
    DrawLineInHost_2X_4X(linePixelCountShres, frame.GetLine(line) + outputClipShresi.Left, bufferFirstLinePtr, bufferSecondLinePtr, bufferThirdLinePtr, bufferFourthLinePtr);
    IncreaseBufferPtrs(bufferFirstLinePtr, bufferSecondLinePtr, bufferThirdLinePtr, bufferFourthLinePtr, amigaLinePitchUint64);
  }

  // Padding on the bottom beyond actual lines in the frame (long, short frames, or in case the clip is too large)
  if (frame.LinesInFrame < bottomLine)
  {
    for (unsigned int line = maxLineWithData; line < bottomLine; line++)
    {
      DrawNothing_2X_4X(linePixelCountShres, bufferFirstLinePtr, bufferSecondLinePtr, bufferThirdLinePtr, bufferFourthLinePtr, padColor);
      IncreaseBufferPtrs(bufferFirstLinePtr, bufferSecondLinePtr, bufferThirdLinePtr, bufferFourthLinePtr, amigaLinePitchUint64);
    }
  }

  if (outputClipShresi.Bottom & 1)
  {
    // In case last line is half height in host window due to clipping
    if (frame.LinesInFrame <= bottomLine)
    {
      DrawNothing_2X_2X(linePixelCountShres, bufferFirstLinePtr, bufferSecondLinePtr, padColor);
    }
    else
    {
      DrawLineInHost_2X_2X(linePixelCountShres, frame.GetLine(bottomLine) + outputClipShresi.Left, bufferFirstLinePtr, bufferSecondLinePtr);
    }
  }
}

void HostFrameCopier::DrawFrameInHostInternal(
    const MappedChipsetFramebuffer &mappedChipsetFramebuffer, const unsigned int chipsetBufferScaleFactor, const RectShresi &outputClipShresi, const uint64_t padColor)
{
  if (chipset_info.GfxDebugDisableCopier) return;

  if (chipsetBufferScaleFactor == 1)
  {
    DrawFrameInHost1X(host_frame, mappedChipsetFramebuffer, outputClipShresi, padColor);
  }
  else
  {
    DrawFrameInHost2X(host_frame, mappedChipsetFramebuffer, outputClipShresi, padColor);
  }
}

void HostFrameCopier::DrawFrameInHost(
    const MappedChipsetFramebuffer &mappedChipsetFramebuffer, const unsigned int chipsetBufferScaleFactor, const RectShresi &outputClipShresi, const uint64_t padColor)
{
  PerformanceCounter->Start();

  DrawFrameInHostInternal(mappedChipsetFramebuffer, chipsetBufferScaleFactor, outputClipShresi, padColor);

  PerformanceCounter->Stop();
}

void HostFrameCopier::Startup()
{
  PerformanceCounter = Service->PerformanceCounterFactory.Create("Frame copier");
}

HostFrameCopier::HostFrameCopier() noexcept : PerformanceCounter(nullptr)
{
}

HostFrameCopier::~HostFrameCopier()
{
  delete PerformanceCounter;
}
