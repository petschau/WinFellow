#pragma once

/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* C.A.P.S. Support - The Classic Amiga Preservation Society               */
/* http://www.softpres.org                                                 */
/*                                                                         */
/* (w)2004-2019 by Torsten Enderling                                       */
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

#ifdef FELLOW_SUPPORT_CAPS
#include "defs.h"

/* function prototypes */

extern BOOLE capsStartup();
extern BOOLE capsShutdown();
extern BOOLE capsLoadImage(uint32_t, FILE *, uint32_t *);
extern BOOLE capsUnloadImage(uint32_t);
extern BOOLE capsLoadNextRevolution(uint32_t, uint32_t, uint8_t *, uint32_t *);
extern BOOLE capsLoadTrack(uint32_t, uint32_t, uint8_t *, uint32_t *, uint32_t *, uint32_t *, BOOLE *);

#endif
