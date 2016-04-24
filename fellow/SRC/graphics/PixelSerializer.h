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

#ifndef PIXELSERIALIZER_H
#define PIXELSERIALIZER_H

#include "DEFS.H"
#include "GraphicsEventQueue.h"

class PixelSerializer : public GraphicsEvent
{
private:
  const static ULO FIRST_CYLINDER = 56;
  const static ULO LAST_CYLINDER = 25;

  ByteWordUnion _activeWord[6];
  UWO _pendingWord[6];

  ULO _tmpline[960];

  ULO _lastCylinderOutput;
  bool _newLine;
  bool _activated;

  void LogEndOfLine(ULO rasterY, ULO cylinder);
  void LogOutput(ULO rasterY, ULO cylinder, ULO startCylinder, ULO untilCylinder);
  void EventSetup(ULO arriveTime);
  void ShiftActive(ULO pixelCount);
  void CopyPendingToActive(unsigned int playfieldNumber);

  ULO GetOutputLine(ULO rasterY, ULO cylinder);
  ULO GetOutputCylinder(ULO cylinder);
  
  void SerializeCylinders(ULO cylinderCount);

public:
  bool GetActivated();
  bool GetIsWaitingForActivationOnLine();
  ULO GetLastCylinderOutput();
  ULO GetFirstPossibleOutputCylinder();
  ULO GetLastPossibleOutputCylinder();
  UWO GetPendingWord(unsigned int bitplaneNumber);

  void Commit(UWO dat1, UWO dat2, UWO dat3, UWO dat4, UWO dat5, UWO dat6);

  void OutputCylindersUntil(ULO rasterY, ULO cylinder);

  virtual void Handler(ULO rasterY, ULO cylinder);
  virtual void InitializeEvent(GraphicsEventQueue *queue);

  void EndOfFrame(void);
  void SoftReset(void);
  void HardReset(void);
  void EmulationStart(void);
  void EmulationStop(void);
  void Startup(void);
  void Shutdown(void);

  PixelSerializer(void) : GraphicsEvent(), _lastCylinderOutput(FIRST_CYLINDER - 1), _newLine(true), _activated(false) {};
};

#endif
