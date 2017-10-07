/*=========================================================================*/
/* Fellow                                                                  */
/* Bitplane serializer                                                     */
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

#ifndef BITPLANESERIALIZER_H
#define BITPLANESERIALIZER_H

#include "DEFS.H"
#include "GraphicsEventQueue.h"
#include "BitplaneSerializerState.h"

class BitplaneSerializer : public GraphicsEvent
{
private:
  BitplaneSerializerState _state;

  static void LogEndOfLine(unsigned int lineNumber, unsigned int cylinder);
  static void LogOutputUntilCylinder(unsigned int lineNumber, unsigned int cylinder, unsigned int startCylinder, unsigned int untilCylinder);

  void EventSetup(unsigned int arriveTime);
  void MaybeCopyPendingToActive(unsigned int currentCylinder, unsigned int shiftMatchMask);

  unsigned int GetRightExtendedCylinder(unsigned int lineNumber, unsigned int cylinder);
  static unsigned int GetCylinderWrappedByHPos(unsigned int cylinder);

  static bool IsInVerticalBlank(unsigned int lineNumber);
  static bool IsFirstLineInVerticalBlank(unsigned int lineNumber);

  void OutputBitplanes(unsigned int cylinderCount);
  static void OutputBackground(unsigned int cylinderCount);
  void OutputCylinders(unsigned int cylinderCount);

public:
  void Commit(UWO dat1, UWO dat2, UWO dat3, UWO dat4, UWO dat5, UWO dat6);
  void OutputCylindersUntil(unsigned int lineNumber, unsigned int cylinder);

  virtual void Handler(unsigned int lineNumber, unsigned int cylinder);
  virtual void InitializeEvent(GraphicsEventQueue *queue);

  void EndOfFrame();
  void SoftReset();
  void HardReset();
  void EmulationStart();
  void EmulationStop();
  void Startup();
  void Shutdown();

  BitplaneSerializer();
};

#endif
