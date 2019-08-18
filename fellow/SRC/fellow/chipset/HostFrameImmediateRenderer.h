/*=========================================================================*/
/* Fellow                                                                  */
/* Renders Amiga pixels as host pixels                                     */
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

#include "fellow/api/defs.h"

class HostFrameImmediateRenderer;
typedef void (HostFrameImmediateRenderer::*ImmediateDrawBatchFunc)(ULO line, ULO startX, ULO pixelCount);

class HostFrameImmediateRenderer
{
private:
  ImmediateDrawBatchFunc _currentDrawBatchFunc;

  static void SetPixel1xULL(ULL *dst, ULL hostColor);
  static void SetPixel2xULL(ULL *dst, ULL hostColor);
  static void SetPixel4xULO(ULO *dst, ULO hostColor);

  void DrawLores(ULO line, ULO startX, ULO pixelCount);
  void DrawLoresDual(ULO line, ULO startX, ULO pixelCount);
  void DrawLoresHam(ULO line, ULO startX, ULO pixelCount);
  void DrawHires(ULO line, ULO startX, ULO pixelCount);
  void DrawHiresDual(ULO line, ULO startX, ULO pixelCount);
  void DrawNothing(ULO line, ULO startX, ULO pixelCount);

public:
  void UpdateDrawBatchFunc();
  ImmediateDrawBatchFunc GetDrawBatchFunc() const
  {
    return _currentDrawBatchFunc;
  }

  void DrawBatchImmediate(ULO line, ULO startX);

  void InitializeNewFrame(unsigned int linesInFrame);

  HostFrameImmediateRenderer();
  ~HostFrameImmediateRenderer() = default;
};

extern HostFrameImmediateRenderer host_frame_immediate_renderer;
