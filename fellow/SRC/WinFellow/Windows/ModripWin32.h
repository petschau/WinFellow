#pragma once

/*=========================================================================*/
/* Fellow                                                                  */
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

/* fellow includes */
#include "Defs.h"

/* own includes */
#include "modrip.h"

extern BOOLE modripGuiInitialize();
extern void modripGuiSetBusy();
extern void modripGuiUnSetBusy();
extern BOOLE modripGuiSaveRequest(struct ModuleInfo *, MemoryAccessFunc);
extern void modripGuiErrorSave(struct ModuleInfo *);
extern BOOLE modripGuiRipFloppy(int);
extern BOOLE modripGuiRipMemory();
extern void modripGuiUnInitialize();
extern void modripGuiError(char *);
extern BOOLE modripGuiDumpChipMem();
extern BOOLE modripGuiRunProWiz();
