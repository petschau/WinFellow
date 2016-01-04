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

#ifndef BITPLANEDRAW_H
#define BITPLANEDRAW_H

class BitplaneDraw
{
private:
  ULO *_tmpframe[1024];

  void TempLores(ULO rasterY, ULO pixel_index, ULO pixel_count);
  void TempLoresDual(ULO rasterY, ULO pixel_index, ULO pixel_count);
  void TempLoresHam(ULO rasterY, ULO pixel_index, ULO pixel_count);
  void TempHires(ULO rasterY, ULO pixel_index, ULO pixel_count);
  void TempHiresDual(ULO rasterY, ULO pixel_index, ULO pixel_count);
  void TempNothing(ULO rasterY, ULO pixel_index, ULO pixel_count);

public:
  void DrawBatch(ULO rasterY, ULO rasterX);
  void TmpFrame(ULO next_line_offset);
};


#endif
