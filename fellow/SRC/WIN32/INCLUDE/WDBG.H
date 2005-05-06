/* @(#) $Id: WDBG.H,v 1.1.1.1.2.3 2004-06-08 14:05:36 carfesh Exp $ */
/*=========================================================================*/
/* Fellow Amiga Emulator                                                   */
/*                                                                         */
/* Windows GUI code for debugger                                           */
/*                                                                         */
/* Authors: Torsten Enderling (carfesh@gmx.net)                            */
/*          Petter Schau (peschau@online.no)                               */
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

#ifndef wdbg_H
#define wdbg_H

extern void wdbgDebugSessionRun(HWND parent);

typedef enum {
  DBG_STEP,
  DBG_STEP_OVER,
  DBG_RUN
} dbg_operations;



#endif /* wdbg_H */

