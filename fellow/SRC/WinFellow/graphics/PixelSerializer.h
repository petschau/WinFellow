#pragma once

/*=========================================================================*/
/* Fellow                                                                  */
/* Pixel serializer                                                        */
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

#include "DEFS.H"
#include "GraphicsEventQueue.h"

class PixelSerializer : public GraphicsEvent
{
private:
  const static uint32_t FIRST_CYLINDER = 56;
  const static uint32_t LAST_CYLINDER = 25;

  ByteLongUnion _active[6];

  uint32_t _tmpline[960];

  uint32_t _lastCylinderOutput;
  bool _newLine;
  bool _activated;

  void LogEndOfLine(uint32_t rasterY, uint32_t cylinder);
  void LogOutput(uint32_t rasterY, uint32_t cylinder, uint32_t startCylinder, uint32_t untilCylinder);
  void EventSetup(uint32_t arriveTime);
  void ShiftActive(uint32_t pixelCount);

  uint32_t GetOutputLine(uint32_t rasterY, uint32_t cylinder);
  uint32_t GetOutputCylinder(uint32_t cylinder);

  void SerializePixels(uint32_t pixelCount);
  void SerializeBatch(uint32_t cylinderCount);

public:
  void Commit(uint16_t dat1, uint16_t dat2, uint16_t dat3, uint16_t dat4, uint16_t dat5, uint16_t dat6);

  void OutputCylindersUntil(uint32_t rasterY, uint32_t cylinder);

  virtual void Handler(uint32_t rasterY, uint32_t cylinder);
  virtual void InitializeEvent(GraphicsEventQueue *queue);

  void EndOfFrame();
  void SoftReset();
  void HardReset();
  void EmulationStart();
  void EmulationStop();
  void Startup();
  void Shutdown();

  PixelSerializer() : GraphicsEvent(), _lastCylinderOutput(FIRST_CYLINDER - 1), _newLine(true), _activated(false){};
};
