/* @(#) $Id: zlibwrap.h,v 1.4 2004-06-08 11:09:48 carfesh Exp $ */
/*=========================================================================*/
/* Fellow Amiga Emulator - zlib wrapper                                    */
/*                                                                         */
/* Author: Torsten Enderling (carfesh@gmx.net)                             */
/*         (Wraps zlib code to have one simple call for decompression.)    */
/*                                                                         */
/* originates from minigzip.c                                              */
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

#include "defs.h"

BOOLE gzUnpack(const char *src, const char *dest);
BOOLE gzPack  (const char *src, const char *dest);