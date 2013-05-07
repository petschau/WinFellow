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
  ByteLongUnion _active[6];

  ULO _tmpline[960];

  bool _enableLog;
  FILE *_logfile;

  void Log(ULO rasterY, ULO rasterX);
  void EventSetup(ULO cycle);
  void ShiftActive(ULO pixelCount);
  void SerializeBatch(void);
  bool OutputCylinders(ULO rasterY, ULO rasterX);

public:
  void Commit(UWO dat1, UWO dat2, UWO dat3, UWO dat4, UWO dat5, UWO dat6);

  virtual void Handler(ULO rasterY, ULO rasterX);
  virtual void InitializeEvent(GraphicsEventQueue *queue);

  // Standard Fellow methods
  void EndOfFrame(void);
  void SoftReset(void);
  void HardReset(void);
  void EmulationStart(void);
  void EmulationStop(void);
  void Startup(void);
  void Shutdown(void);

  PixelSerializer(void) : GraphicsEvent() {};

};


#endif
