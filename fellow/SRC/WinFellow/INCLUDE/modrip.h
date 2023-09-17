/* @(#) $Id: modrip.h,v 1.8 2008-02-21 00:00:44 peschau Exp $ */
/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* Portable parts of the module ripper                                     */
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

#ifndef _MODRIP_H_
#define _MODRIP_H_

#define MODRIP_TEMPSTRLEN 2048
#define MODRIP_MAXMODLEN (1024 * 1024)

/* memory access wrapper functions */
typedef uint8_t (*MemoryAccessFunc)(uint32_t);

/* module type detection functions */
typedef void (*ModuleDetectFunc)(uint32_t, MemoryAccessFunc); 

/* structure to be filled with module information for passing on */
struct ModuleInfo {
  char filename[MODRIP_TEMPSTRLEN]; /* filename for the module-file */
  char modname[MODRIP_TEMPSTRLEN];  /* name of the module */
  char typedesc[MODRIP_TEMPSTRLEN]; /* detailed file format */
  char typesig[MODRIP_TEMPSTRLEN];  /* file format signature */
  uint32_t start, end;                   /* start and end of the module in mem */
  unsigned samplesize; /* no. of all sample-data used in bytes */
  unsigned patternsize; /* no. of all pattern-data used in bytes */
  unsigned songlength; /* how many patterns does the song play? */
  unsigned maxpattern; 
  unsigned channels;
  unsigned instruments;
};

extern void modripRIP(void);
extern void modripChipDump(void);
extern BOOLE modripSaveMem(struct ModuleInfo *, MemoryAccessFunc);

#endif