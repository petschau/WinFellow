/*=========================================================================*/
/* Fellow Amiga Emulator                                                   */
/*                                                                         */
/* @(#) $Id: modrip_win32.h,v 1.5.2.2 2004-05-27 10:10:39 carfesh Exp $         */
/*                                                                         */
/* OS-dependant parts of the module ripper - Windows GUI code              */
/*                                                                         */
/* Author: Torsten Enderling (carfesh@gmx.net)                             */
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

#ifndef _MODRIP_WIN32_H_
#define _MODRIP_WIN32_H_

/* fellow includes */
#include "defs.h"

/* own includes */
#include "modrip.h"

extern BOOLE modripGuiInitialize(void);
extern void modripGuiSetBusy(void);
extern void modripGuiUnSetBusy(void);
extern BOOLE modripGuiSaveRequest(struct ModuleInfo *, MemoryAccessFunc);
extern void modripGuiErrorSave(struct ModuleInfo *);
extern BOOLE modripGuiRipFloppy(int);
extern BOOLE modripGuiRipMemory(void);
extern void modripGuiUnInitialize(void);
extern void modripGuiError(char *);
extern BOOLE modripGuiDumpChipMem(void);
extern BOOLE modripGuiRunProWiz(void);

#endif