/*=========================================================================*/
/* Fellow Amiga Emulator                                                   */
/*                                                                         */
/* @(#) $Id: modrip.c,v 1.24 2004-05-27 12:27:26 carfesh Exp $         */
/*                                                                         */
/* Portable parts of the module ripper                                     */
/*                                                                         */
/* Author: Torsten Enderling (carfesh@gmx.net)                             */
/*                                                                         */
/* based on information found on Exotica (http://exotica.fix.no), the xmp  */
/* source code (http://xmp.helllabs.org), Amiga Mod Packers Described      */
/* (http://www.multimania.com/asle/ampd.html) and too many other web       */
/* sources                                                                 */
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
#include "defs.h"
#include "fmem.h"
#include "floppy.h"
#include "fellow.h"

/* own includes */
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <io.h>

#include "modrip.h"
#ifdef WIN32
#include "modrip_win32.h"
#else
#include "modrip_linux.h"
#endif

#define MODRIPDOLOGGING 1

#if MODRIPDOLOGGING
/* define this to have logfile output */
#define RIPLOG1(x)       fellowAddLog(x);
#define RIPLOG2(x, y)    fellowAddLog(x, y);
#define RIPLOG3(x, y, z) fellowAddLog(x, y, z);
#else
#define RIPLOG1(x)
#define RIPLOG2(x)
#define RIPLOG3(x)
#endif

static ULO modripModsFound;

/*==============================================*/
/* macros for accessing big endian style values */
/*==============================================*/

#define BYTE(x)    ((*func)((x)))
#define BEWORD(x)  ((*func)((x)) << 8 | (*func)((x)+1))
#define BEDWORD(x) ((*func)((x)) << 24 | (*func)((x)+1) << 16 | (*func)((x)+2) << 8 | (*func)((x)+3))

/*===============================================================*/
/* saves mem for a detect module with a filled ModuleInfo struct */
/* gets the values via memory access function func               */
/*===============================================================*/

BOOLE modripSaveMem(struct ModuleInfo *info, MemoryAccessFunc func)
{
  ULO i;
  FILE *modfile;

  if(info == NULL) return FALSE;

  RIPLOG3("mod-ripper saving range 0x%06x - 0x%06x\n", info->start, info->end);

  if ((modfile = fopen(info->filename, "w+b")) == NULL) return FALSE;
  for (i = info->start; i <= info->end; i++)
    fputc((*func)(i), modfile);
  fclose(modfile);

  RIPLOG2("mod-ripper wrote file %s.\n", info->filename);

  return TRUE;
}

/*============================================*/
/* save a memory dump 1:1 into a file         */
/* useful for running other rippers over that */
/* Thanks to Sylvain for the idea :)          */
/*============================================*/

static BOOLE modripSaveChipMem(char *filename)
{
  FILE *memfile;
  size_t written;
  if(!filename || !(*filename)) return FALSE;
  if((memfile = fopen(filename, "wb")) == NULL) return FALSE;
  written = fwrite(memory_chip, 1, memoryGetChipSize(), memfile);
  fclose(memfile);
  if(written < memoryGetChipSize()) return FALSE;
  return TRUE;
}

static BOOLE modripSaveBogoMem(char *filename)
{
  FILE *memfile;
  size_t written;
  if(!filename || !(*filename)) return FALSE;
  if((memfile = fopen(filename, "wb")) == NULL) return FALSE;
  written = fwrite(memory_bogo, 1, memoryGetBogoSize(), memfile);
  fclose(memfile);
  if(written < memoryGetBogoSize()) return FALSE;
  return TRUE;
}

static BOOLE modripSaveFastMem(char *filename)
{
  FILE *memfile;
  size_t written;
  if(!filename || !(*filename)) return FALSE;
  if((memfile = fopen(filename, "wb")) == NULL) return FALSE;
  written = fwrite(memory_fast, 1, memoryGetFastSize(), memfile);
  fclose(memfile);
  if(written < memoryGetFastSize()) return FALSE;
  return TRUE;
}

/*=======================================*/
/* fills a ModuleInfo struct with zeroes */
/*=======================================*/

static void modripModuleInfoInitialize(struct ModuleInfo *info)
{
  if(info) memset(info, 0, sizeof(struct ModuleInfo));
}

/*======================================*/
/* detect ProTracker and clones         */
/* games: SWIV, Lotus II, First Samurai */
/*======================================*/

static void modripDetectProTracker(ULO address, MemoryAccessFunc func)
{
  ULO i;
  int type;
  struct { char *ID; char *Desc; int channels; } formats[9] = {
	  {"M.K.", "Noisetracker",  4},
      {"N.T.", "Noisetracker",  4},
      {"M!K!", "Protracker",    4},
	  {"4CHN", "4 channel",     4},
	  {"6CHN", "6 channel",     6},
	  {"8CHN", "8 channel",     8},
	  {"FLT4", "Startrekker 4", 4},
	  {"FLT8", "Startrekker 8", 8},
	  {"M&K!", "Noisetracker",  4}
  };
  struct ModuleInfo info;
  BOOLE ScratchyName;

  for(type = 0; type < 9; type++) {
    if ( ((*func)(address + 0) == formats[type].ID[0])
      && ((*func)(address + 1) == formats[type].ID[1])
      && ((*func)(address + 2) == formats[type].ID[2])
      && ((*func)(address + 3) == formats[type].ID[3])
       ) {

      RIPLOG2("mod-ripper ProTracker %s match\n", formats[type].ID);

      modripModuleInfoInitialize(&info);

      /* store general info */
	  strncpy(info.typedesc, formats[type].Desc, 30);
      strncpy(info.typesig, formats[type].ID, 4);
      info.typesig[4] = '\0';
      info.channels = formats[type].channels;

      /* searchstring found, now calc size */
      info.start = address - 0x438;

      /* get sample size */
      info.samplesize = 0;		

      for (i = 0; i <= 30; i++) {
        info.samplesize += BEWORD(info.start + 0x2a + i*0x1e) * 2;
	  }

	  /* some disks like messing around :) */

      RIPLOG2("samplesize = %u\n", info.samplesize);

      if(info.samplesize > MODRIP_MAXMODLEN) return;

      info.songlength = (*func)(info.start + 0x3b6);

	  RIPLOG2("songlength = %u\n", info.songlength);

      if((info.songlength > MODRIP_MAXMODLEN) 
      || (info.songlength == 0)) return;

      /* scan for max. amount of patterns */
      info.maxpattern = 0;		

      for (i = 0; i <= info.songlength; i++) {
        if (info.maxpattern < (*func)(info.start + 0x3b8 + i)) {
          info.maxpattern = (*func)(info.start + 0x3b8 + i);
		}
	  }

	  RIPLOG2("maxpattern = %u\n", info.maxpattern);

	  if(info.maxpattern > 127) return;		/* @@@@@ is this value correct ? */

      info.patternsize = (info.maxpattern + 1) * 64 * 4 * info.channels;

	  RIPLOG2("patternsize = %u\n", info.patternsize);

      if(info.patternsize > MODRIP_MAXMODLEN) return;

      info.end = info.start + info.samplesize + info.patternsize + 0x43b;

	  if(info.end < info.start) return;

      if ((info.end - info.start < MODRIP_MAXMODLEN)) {
      /* get module name */
      for (i = 0; i < 20; i++) {
        info.modname[i] = (char)((*func)(info.start + i));
	  }
      info.modname[20] = 0;	

      /* set filename for the module file */
      if (strlen(info.modname) > 2) {
        ScratchyName = FALSE;
        for(i = 0; (i < 20) && (info.modname[i] != 0) ; i++) {
          if(!isprint(info.modname[i])) ScratchyName = TRUE;
		 }
         if(!ScratchyName) {
          strcpy(info.filename, info.modname);
          strcat(info.filename, ".amod");
		}
		else
          sprintf(info.filename, "mod%d.amod", modripModsFound++);
	  }
    else {
        sprintf(info.filename, "mod%d.mod", modripModsFound++);
	  }

	  modripGuiSaveRequest(&info, func);
	  }
	}
  }
}


/*============================*/
/* detect SoundFX 1.3 and 2.0 */
/* games: TwinWorld (1.3),    */
/* Future Wars, Rolling Ronny */
/*============================*/

static void modripDetectSoundFX(ULO address, MemoryAccessFunc func)
{
  BOOLE match = FALSE;
  ULO i, size, offset;
  unsigned patterns;
  struct ModuleInfo info;
    
  if (((*func)(address + 0) == 'S') && ((*func)(address + 1) == 'O')) {
    modripModuleInfoInitialize(&info);
    if (((*func)(address + 2) == 'N') && ((*func)(address + 3) == 'G')) {

      RIPLOG1("mod-ripper SoundFX 1.3 (SONG) match\n");

      match = TRUE;
      info.start = address - 60;
      info.instruments = 15;
      strcpy(info.typedesc, "SoundFX 1.3");
      strcpy(info.typesig, "SONG");
	}
    if (((*func)(address + 2) == '3') && ((*func)(address + 3) == '1')) {

      RIPLOG1("mod-ripper SoundFX 2.0 (SO31) match\n");

      match = TRUE;
      info.start = address - 124;
      info.instruments = 31;
      strcpy(info.typedesc, "SoundFX 2.0");
      strcpy(info.typesig, "SO31");
    }
    if (match) {
      offset = 0; size = 0;

	  /* add instrument lengths to size */
      for (i = 0; i < info.instruments; i++)
        size += BEDWORD(info.start + i*4);

      /* move to instrument table */
      if (info.instruments == 15) {
        /* SoundFX 1.3 */
	    offset += 80;
	    size += 80;
	  }
      else {
        /* SoundFX 2.0 */
        offset += 144;
        size += 144;
	  }

	  /* walk over instrument table */
      for (i = 0; i < info.instruments; i++) {
        offset += 30;
        size += 30;
	  }

      patterns = (*func)(info.start + offset);
      if((patterns > MODRIP_MAXMODLEN) 
      || (patterns == 0)) return;

      RIPLOG2("patterns = %u\n", patterns);

      offset += 2;
	  size += 2;

      /* pattern table */
      for (i = 0; i < patterns; i++)
        info.maxpattern = max(info.maxpattern, (*func)(info.start + offset + i));

  	  if((info.maxpattern > MODRIP_MAXMODLEN) ||
         (info.maxpattern == 0)) return;

      RIPLOG2("maxpattern = %u\n", info.maxpattern);

      size += 128 + ((info.maxpattern + 1) * 1024);
	  info.end = info.start + size;

	  if(info.end < info.start) return;
		
      if (size < MODRIP_MAXMODLEN) {
        /* set filename for the module file */
        sprintf(info.filename, "SFX.Mod%d.amod", modripModsFound++);

    modripGuiSaveRequest(&info, func);
 }
    }
  }
}


/*===========================*/
/* detect BP-SoundMon        */
/* games: Alien Breed (+SE), */
/*   ProjectX (+SE)          */
/*===========================*/

static void modripDetectSoundMon(ULO address, MemoryAccessFunc func)
{
  BOOLE FoundHeader = FALSE, ScratchyName;
  struct ModuleInfo info;
  int version = 0;
  ULO offset = 0, patterns = 0;
  ULO temp = 0, i = 0;
	
  if( ((*func)(address + 0) == 'B')
   && ((*func)(address + 1) == 'P')
   && ((*func)(address + 2) == 'S')
   && ((*func)(address + 3) == 'M') ) {
    FoundHeader = TRUE;
    modripModuleInfoInitialize(&info);
    strcpy(info.typedesc, "SoundMon v1.0");
    strcpy(info.typesig, "BPSM");
    version = 1;
  }
  if( ((*func)(address + 0) == 'V')
   && ((*func)(address + 1) == '.') ) {
    modripModuleInfoInitialize(&info);
    if( (*func)(address + 2) == '2' ) {
      FoundHeader = TRUE;
      modripModuleInfoInitialize(&info);
      strcpy(info.typedesc, "SoundMon v2.0");
      strcpy(info.typesig, "V.2");
      version = 2;
	}
    if( (*func)(address + 2) == '3' ) {
      FoundHeader = TRUE;
      modripModuleInfoInitialize(&info);
      strcpy(info.typedesc, "SoundMon v2.2");
      strcpy(info.typesig, "V.3");
      version = 3;
	}
  }
  if(!FoundHeader) return;

  info.start = address - 26;

  RIPLOG3("mod-ripper found match for SoundMon (%s) at 0x%06x.\n",
    info.typesig, info.start);

  info.end = info.start;

  /* get number of instruments */
  if(version > 1) info.instruments = (*func)(info.start + 29);

  /* get patterns */
  patterns = BEWORD(info.start + 30);

  RIPLOG2("patterns = %u\n", patterns);

  if((patterns > MODRIP_MAXMODLEN) 
  || (patterns == 0)) return;

  offset += 32;
  for(i = 0; i < 15; i++) {
    if( (*func)(info.start + offset) != 255 ) {
	  temp = BEWORD(info.start + offset + 24) * 2;
      if(temp > 0)
        info.end += temp;
	}
    offset += 32;
  }
  info.end += 512;

  for(i = 0; i < patterns * 4; i++) {
    temp = BEWORD(info.start + offset + i*4);
    info.maxpattern = max(info.maxpattern, temp);
  }

  RIPLOG2("maxpattern = %u\n", info.maxpattern);

  if((info.maxpattern > MODRIP_MAXMODLEN) 
  || (info.maxpattern == 0)) return;

  info.end += (patterns * 16 + info.maxpattern * 48);
  info.end += info.instruments * 64;

  if(info.end < info.start) return;

  if ((info.end - info.start < MODRIP_MAXMODLEN)) {
    /* get song name */
    for (i = 0; i < 26; i++) {
      info.modname[i] = (char)((*func)(info.start + i));
	}
    info.modname[26] = 0;	

    /* set filename */
    if (strlen(info.modname) > 2) {
      ScratchyName = FALSE;
      for(i = 0; (i < 26) && (info.modname[i] != 0) ; i++) {
        if(!isprint(info.modname[i])) {
          ScratchyName = TRUE;
		  break;
		}
		if(isupper(info.modname[i]))
		  info.modname[i] = tolower(info.modname[i]);
	  }
      if(!ScratchyName) {
		sprintf(info.filename, "BP.");
        strcat(info.filename, info.modname);
        strcat(info.filename, ".amod");
	  }
	  else
        sprintf(info.filename, "BP.Mod%d.amod", modripModsFound++);
	  }
     else {
      sprintf(info.filename, "BP.Mod%d.amod", modripModsFound++);
	 }

   modripGuiSaveRequest(&info, func);  
 }     
}

/*==================================*/
/* detect FredEditor                */
/* games: Fuzzball, Ilyad           */
/*==================================*/

static void modripDetectFred(ULO address, MemoryAccessFunc func)
{
  ULO offset = 0, i, j;
  BOOLE match = FALSE;
  struct ModuleInfo info;
  long instData = 0, instDataOffset = 0, instNo = 0, instMax = 0;
  long songData = 0, songDataOffset = 0, songNo = 0;
  long sampSize = 0, sampDataOffset = 0,             sampMax = 0;
  long ModuleStart = 0;

  /* 68k instructions to search for in the header */
  const ULO jmp_68k = 0x4efa; 
  const ULO mov_68k = 0x123a; 
  const ULO cmp_68k = 0xb0016200;

  /* Fred files start with a jump table */
  if(
        (BEWORD(address +  0) == jmp_68k)
	 && (BEWORD(address +  4) == jmp_68k)
	 && (BEWORD(address +  8) == jmp_68k) 
	 && (BEWORD(address + 12) == jmp_68k)
    ) {

    RIPLOG1("mod-ripper possible match for FredEditor.\n");

    offset = 2;

	/* search for beginning of init block */
    for(i = 0; i < 64; i++) {
      if(
            (BEWORD (address + offset + 0) == mov_68k)
         && (BEDWORD(address + offset + 4) == cmp_68k)
        ) {
			match = TRUE;
			break;
	  }
      offset += 2;
    }
  }

  if(!match) return;

  RIPLOG1("mod-ripper match for FredEditor.\n");

  modripModuleInfoInitialize(&info);

  strcpy(info.typedesc, "FredEditor");

  info.start = address;
  info.end = address;

  for(i = 0; 
     (i < 512) 
       && (BEWORD(address + i + 0) != 0x4bfa) 
	   && (BEWORD(address + i + 4) != 0xdbfa);
	  i+=2) ;

  RIPLOG2("mod-ripper checkpoint i (%u)\n", i);

  if(i == 512) return;

  offset = i + 2;

  ModuleStart = offset - 0x10000 + BEWORD(address + offset);
  instDataOffset = BEWORD(address + offset + 4) + offset + 4;

  for(j = 0;
      (j < 254) 
        && (BEWORD(address + i + j + 0) != 0x47fa)
		&& (BEWORD(address + i + j + 4) != 0xd7fa);
	  j+=2) ;

  RIPLOG2("mod-ripper checkpoint j (%u)\n", j);

  if(j == 254) return;

  offset += j;

  RIPLOG2("mod-ripper checkpoint ModuleStart (%d)\n", ModuleStart);

  if((offset - 0x10000 + BEWORD(address + offset)) != ModuleStart) return;

  songDataOffset = BEWORD(address + offset + 4) + offset + 4;

  if(ModuleStart < 0) {
    instData = BEDWORD(address + instDataOffset) + ModuleStart;
    songData = BEDWORD(address + songDataOffset) + ModuleStart;
  }
  else {
    instData = BEDWORD(address + instDataOffset);
    songData = BEDWORD(address + songDataOffset);
  }

  songNo = (*func)(address + instDataOffset - 13) + 1;

  for (i = songData; i < (ULO) instData; i++) {
    if ((*func)(address + i) == 0x83)
      instMax = max((*func)(address + i + 1), (ULO)instMax);
  }
  instMax++;
 
  for(i = 0; i < (ULO) instMax; i++) {
    sampDataOffset = BEDWORD(address + instData + i*64);
    if (
        (BEWORD(address + instData + i*64 + 4) == 0)
     && (sampDataOffset != 0) 
     && (sampDataOffset < 0x2ffff) 
       ) {
            sampSize = BEWORD(address + instData + i*64 + 6);
            sampMax = max(sampMax, (sampSize*2 + sampDataOffset));
            instNo++;
    }
  }

  if(sampMax)
    info.end += sampMax;
  else
    info.end += instData + instMax * 64;

  if ((info.end - info.start < MODRIP_MAXMODLEN)) {
    sprintf(info.filename, "FRED.Mod%d.amod", modripModsFound++);

  modripGuiSaveRequest(&info, func);
}
  }

/*==========================*/
/* detect ProRunner 2.0     */
/* games: Pinball Illusions */
/*==========================*/

static void modripDetectProRunner2(ULO address, MemoryAccessFunc func)
{
  ULO i;
  ULO sampSize = 0, sampPtr = 0;
  struct ModuleInfo info;
  
  if(
      (BYTE(address + 0) != 'S')
   || (BYTE(address + 1) != 'N')
   || (BYTE(address + 2) != 'T')
   || (BYTE(address + 3) != '!')
    ) return;

  RIPLOG1("mod-ripper possible ProRunner 2.0 match...\n");

  RIPLOG1("checkpoint 1: finetune values...\n");
  /* check finetune values */
  for(i = 0; i < 31; i++) {
    if(BYTE(address + 10 + 8*i) > 0xf)
      return;
  }
  
  RIPLOG1("checkpoint 2: volume values...\n");
  /* check volume values */
  for(i = 0; i < 31; i++) {
    if(BYTE(address + 11 + 8*i) > 0x40)
      return;
  }

  modripModuleInfoInitialize(&info);
  info.start = address;
  info.end = address;
  strcpy(info.typesig, "SNT!");
  strcpy(info.typedesc, "ProRunner 2.0");

  sampPtr = BEDWORD(address + 4);
  RIPLOG2("found sample pointer %u\n", sampPtr);

  for(i = 0; i < 31; i++)
    sampSize += BEWORD(address + 8 + 8*i) << 1;
  RIPLOG2("sample size %u\n", sampSize);

  info.end += sampPtr + sampSize;

  if ((info.end - info.start < MODRIP_MAXMODLEN)) {
    sprintf(info.filename, "PR2.Mod%d.amod", modripModsFound++);

  modripGuiSaveRequest(&info, func);
  }   
}

/*=========================*/
/* detect ThePlayer 4      */
/* games: Colonization,    */
/* some Alien Breed titles */
/*=========================*/

static void modripDetectThePlayer4(ULO address, MemoryAccessFunc func)
{
  struct ModuleInfo info;
  ULO pattNo = 0, sampNo = 0, sampSize = 0;
  ULO sampDataPtr = 0, sampDataCurr = 0;
  ULO sampSizeCurr, loopSizeCurr;
  ULO i;
  BOOLE match = FALSE;;

  if(
      (BYTE(address + 0) == 'P') 
   && (BYTE(address + 1) == '4')
   ) {
    RIPLOG1("mod-ripper found possible ThePlayer 4 match...\n");
	
	modripModuleInfoInitialize(&info);

	if( (BYTE(address + 2) == '0') && (BYTE(address + 3) == 'A') ) {
	  match = TRUE;
	  strcpy(info.typesig, "P40A");
	}
	if( (BYTE(address + 2) == '0') && (BYTE(address + 3) == 'B') ) {
      match = TRUE;
	  strcpy(info.typesig, "P40B");
	}
	if( (BYTE(address + 2) == '1') && (BYTE(address + 3) == 'A') ) {
	  match = TRUE;
	  strcpy(info.typesig, "P41A");
	}
  }
  
  if(!match) return;

  RIPLOG2("mod-ripper found possible ThePlayer 4 (%s) match...\n", info.typesig);
  info.start = address;
  strcpy(info.typedesc, "ThePlayer 4");

  /* number of patterns */
  if((pattNo = BYTE(address + 4)) > 0x7f) return;
  RIPLOG2("number of patterns %u\n", pattNo);

  /* number of samples */
  sampNo = BYTE(address + 6);
  if ((sampNo == 0) || (sampNo > 0x1F)) return;
  RIPLOG2("number of samples %u\n", sampNo);

  /* check sample sizes */
  for(i = 0; i < sampNo; i++) {
    if((sampSizeCurr = BEWORD(address + 24) << 1) > 0xffff) return;
    if((loopSizeCurr = BEWORD(address + 30) << 1) > 0xffff) return;
    if(sampSizeCurr + 2 < loopSizeCurr) return;
    sampSize += sampSizeCurr;
  }
  if(sampSize < 5) return;
  RIPLOG2("sample size %u\n", sampSize);

  /* check volume values */
  for(i = 0; i < sampNo; i++)
    if(BYTE(address + 33 + 16*i) > 0x40) return;

  sampDataPtr = BEDWORD(address + 16) + 4;
  RIPLOG2("sample data pointer %u\n", sampDataPtr);

  /* determine real sample size */
  sampSize = 0;
  for(i = 0; i < sampNo; i++) {
    if((sampDataCurr = BEDWORD(address + 20)) > sampSize) {
      sampSize = sampDataCurr;
      loopSizeCurr = BEWORD(address + 24);
    }
  }
  RIPLOG2("sample size %u\n", sampSize);
  RIPLOG2("last loop size %u\n", loopSizeCurr);

  if(sampSize == 0) return;

  info.end = info.start + sampDataPtr + sampSize + (loopSizeCurr << 1);

  if ((info.end - info.start < MODRIP_MAXMODLEN)) {
    sprintf(info.filename, "%s.Mod%d.amod", info.typesig, modripModsFound++);
    modripGuiSaveRequest(&info, func);
  }   
}

/*====================================================*/
/* from here on starts the actual fellow ripping code */
/*====================================================*/

/*===================================================*/
/* here we define the formats that are actually used */
/*===================================================*/

#define MODRIP_KNOWNFORMATS 6

static ModuleDetectFunc DetectFunctions[MODRIP_KNOWNFORMATS] = {
  modripDetectProTracker,
  modripDetectSoundFX,
  modripDetectSoundMon,
  modripDetectFred,
  modripDetectProRunner2,
  modripDetectThePlayer4
};

/*==============================================*/
/* scan the emulated amiga's memory for modules */
/*==============================================*/

static void modripScanFellowMemory(void)
{
  ULO i, j;
  ULO ChipSize = 0, BogoSize = 0, FastSize = 0;

  if(modripGuiRipMemory()) {
    ChipSize = memoryGetChipSize();
    BogoSize = memoryGetBogoSize();
    FastSize = memoryGetFastSize();

    RIPLOG1("mod-ripper now scanning memory...\n");

	if(ChipSize) {
	  RIPLOG2("mod-ripper running over chip memory (%u KB allocated)...\n",
        ChipSize >> 10);

	  /* chip memory starts at amiga address $0 */
      for(i = 0; i < ChipSize; i++)
        for(j = 0; j < MODRIP_KNOWNFORMATS; j++)
          (*DetectFunctions[j])(i, fetb);
	}

	if(BogoSize) {
	  RIPLOG2("mod-ripper running over bogo memory (%u KB allocated)...\n", 
        BogoSize >> 10);

      /* bogo memory starts at amiga address $C00000 */
      for(i = 0xc00000; i < (0xc00000 + BogoSize); i++)
        for(j = 0; j < MODRIP_KNOWNFORMATS; j++)
          (*DetectFunctions[j])(i, fetb);
	}

	if(FastSize) {
      RIPLOG2("mod-ripper running over fast memory (%u KB allocated)...\n",
	    FastSize >> 10);

      /* fast memory usually starts at amiga address $200000 */
	  for(i = 0x200000; i < (0x200000 + FastSize); i++)
        for(j = 0; j < MODRIP_KNOWNFORMATS; j++)
	      (*DetectFunctions[j])(i, fetb);
	}
  }
}

/*====================================*/
/* floppy cache memory access wrapper */
/*====================================*/

#define MODRIP_ADFSIZE   0xDC000
#define MODRIP_FLOPCACHE 0xFFFFF

/* meant to hold the read floppy cache */
static char *modripCurrentFloppyCache = NULL;

static ULO modripFloppyCacheRead(ULO address)
{
  return(modripCurrentFloppyCache[address & MODRIP_FLOPCACHE]);
}

/*========================================*/
/* read a floppy image into a given cache */
/*========================================*/

static BOOLE modripReadFloppyImage(char *filename, char *cache)
{
  FILE *f;
  char message[MODRIP_TEMPSTRLEN];
  int readbytes;

  if(f = fopen(filename, "rb")) {
    if(readbytes = fread(cache, sizeof(char), MODRIP_ADFSIZE, f) != MODRIP_ADFSIZE) {
      fclose(f); 
      sprintf(message, "The disk image %s is of a wrong size (read %d bytes).", 
        filename, readbytes);
	  modripGuiError(message);
      return FALSE;
	}
	fclose(f);
    return TRUE;
  }
  else {
    sprintf(message, "Couldn't open file %s for reading.", filename);
    modripGuiError(message);
    return FALSE;
  }
}

/*====================================*/
/* scan inserted floppies for modules */
/*====================================*/

static void modripScanFellowFloppies(void)
{
  int driveNo, j;
  ULO i;
  char cache[MODRIP_FLOPCACHE];
  BOOLE Read;

  modripCurrentFloppyCache = cache;

  for(driveNo = 0; driveNo < 4; driveNo++) { /* for each drive */
    if(floppy[driveNo].inserted) { /* check if a floppy is inserted */
      if(modripGuiRipFloppy(driveNo)) { /* does the user want to rip? */
        memset(cache, 0, MODRIP_FLOPCACHE);
		Read = FALSE;
        if(*floppy[driveNo].imagenamereal) {
          RIPLOG2("mod-ripper %s\n", floppy[driveNo].imagenamereal);
          if(modripReadFloppyImage(floppy[driveNo].imagenamereal, cache))
            Read = TRUE;
		}
		else if(*floppy[driveNo].imagename) {
          RIPLOG2("mod-ripper %s\n", floppy[driveNo].imagename);
          if(modripReadFloppyImage(floppy[driveNo].imagename, cache))
            Read = TRUE;
		}
        if(Read) {
          /* now really begin the ripping */
            for(i = 0; i <= MODRIP_ADFSIZE; i++)
              for(j = 0; j < MODRIP_KNOWNFORMATS; j++)
                (*DetectFunctions[j])(i, modripFloppyCacheRead);
		}
	  }
	}
  }
}

/*===================*/
/* do a chipmem dump */
/*===================*/

void modripChipDump(void)
{
  BOOLE Saved = FALSE;

  if(modripGuiDumpChipMem()) {
    Saved = modripSaveChipMem("chip.mem");
    if(memoryGetBogoSize()) modripSaveBogoMem("bogo.mem");
    if(memoryGetFastSize()) modripSaveFastMem("fast.mem");
  }
  if(Saved) {
    if(!access("prowiz.exe", 04)) {
      /* prowiz.exe has been found */
      if(modripGuiRunProWiz())
        system("prowiz.exe chip.mem");
	}
  }
}

/*============================================================================*/
/* Invokes the mod-ripping                                                    */
/*============================================================================*/

void modripRIP(void)
{
  if(!modripGuiInitialize()) return;
  modripGuiSetBusy();
  modripScanFellowMemory();
  modripScanFellowFloppies();
  modripGuiUnSetBusy();

  modripGuiUnInitialize();
}