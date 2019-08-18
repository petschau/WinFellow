#pragma once

/*=========================================================================*/
/* Fellow                                                                  */
/* Copy host frame into graphics system buffer                             */
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

#include "fellow/api/service/IPerformanceCounter.h"
#include "fellow/chipset/HostFrame.h"
#include "MappedChipsetFramebuffer.h"
#include "fellow/application/DrawDimensionTypes.h"

class HostFrameCopier
{
private:
  static uint64_t CombineTwoPixels1X(const uint32_t *source);

  static void DrawNothing_1X_1X(const unsigned int pixelCount_shres, uint64_t *bufferPtr, const uint64_t padColor);
  static void DrawNothing_1X_2X(const unsigned int pixelCount_shres, uint64_t *bufferFirstLinePtr, uint64_t *bufferSecondLinePtr, const uint64_t padColor);
  static void DrawNothing_2X_2X(const unsigned int pixelCount_shres, uint64_t *bufferFirstLinePtr, uint64_t *bufferSecondLinePtr, const uint64_t padColor);
  static void DrawNothing_2X_4X(const unsigned int pixelCount_shres, uint64_t *bufferFirstLinePtr, uint64_t *bufferSecondLinePtr, uint64_t *bufferThirdLinePtr, uint64_t *bufferFourthLinePtr,
                                const uint64_t padColor);

  static void DrawLineInHost_1X_1X(const unsigned int pixelCount_shres, const uint32_t *source, uint64_t *bufferFirstLinePtr);
  static void DrawLineInHost_1X_2X(const unsigned int pixelCount_shres, const uint32_t *source, uint64_t *bufferFirstLinePtr, uint64_t *bufferSecondLinePtr);
  static void DrawLineInHost_2X_2X(const unsigned int pixelCount_shres, const uint32_t *source, uint64_t *bufferFirstLinePtr, uint64_t *bufferSecondLinePtr);
  static void DrawLineInHost_2X_4X(const unsigned int pixelCount_SHRes, const uint32_t *source, uint64_t *bufferFirstLinePtr, uint64_t *bufferSecondLinePtr, uint64_t *bufferThirdLinePtr,
                                   uint64_t *bufferFourthLinePtr);

  static void IncreaseBufferPtrs(uint64_t *&bufferFirstLinePtr, uint64_t *&bufferSecondLinePtr, const ptrdiff_t add);
  static void IncreaseBufferPtrs(uint64_t *&bufferFirstLinePtr, uint64_t *&bufferSecondLinePtr, uint64_t *&bufferThirdLinePtr, uint64_t *&bufferFourthLinePtr, const ptrdiff_t add);

  static void DrawFrameInHost1X(HostFrame &frame, const MappedChipsetFramebuffer &mappedChipsetFramebuffer, const RectShresi &outputClipShresi, const uint64_t padColor);
  static void DrawFrameInHost2X(HostFrame &frame, const MappedChipsetFramebuffer &mappedChipsetFramebuffer, const RectShresi &outputClipShresi, const uint64_t padColor);

  static void DrawFrameInHostInternal(const MappedChipsetFramebuffer &mappedChipsetFramebuffer, const unsigned int chipsetBufferScaleFactor, const RectShresi &outputClipShresi,
                                      const uint64_t padColor);

public:
  fellow::api::IPerformanceCounter *PerformanceCounter;

  void DrawFrameInHost(const MappedChipsetFramebuffer &mappedChipsetFramebuffer, const unsigned int chipsetBufferScaleFactor, const RectShresi &outputClipShresi, const uint64_t padColor);

  void Startup();

  HostFrameCopier() noexcept;
  ~HostFrameCopier();
};

extern HostFrameCopier host_frame_copier;
