/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Portable parts of the module ripper                                        */
/* Author: Torsten Enderling (carfesh@gmx.net)                                */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

#ifndef _MODRIP_H_
#define _MODRIP_H_

#define MODRIP_TEMPSTRLEN 2048
#define MODRIP_MAXMODLEN (1024 * 1024)

/* memory access wrapper functions */
typedef ULO (*MemoryAccessFunc)(ULO);

/* module type detection functions */
typedef void (*ModuleDetectFunc)(ULO, MemoryAccessFunc); 

/* structure to be filled with module information for passing on */
struct ModuleInfo {
  char filename[MODRIP_TEMPSTRLEN]; /* filename for the module-file */
  char modname[MODRIP_TEMPSTRLEN];  /* name of the module */
  char typedesc[MODRIP_TEMPSTRLEN]; /* detailed file format */
  char typesig[MODRIP_TEMPSTRLEN];  /* file format signature */
  ULO start, end;                   /* start and end of the module in mem */
  unsigned samplesize; /* no. of all sample-data used in bytes */
  unsigned patternsize; /* no. of all pattern-data used in bytes */
  unsigned songlength; /* how many patterns does the song play? */
  unsigned maxpattern; 
  unsigned channels;
  unsigned instruments;
};

extern void modripRIP(void);
extern void modripChipDump(void);

#endif