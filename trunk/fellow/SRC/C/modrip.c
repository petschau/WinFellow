/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Portable parts of the module ripper                                        */
/* Author: Torsten Enderling (carfesh@gmx.net)                                */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/
/* Changelog:                                                                 */
/* ----------                                                                 */
/* 2000/12/15:                                                                */
/* - SoundMon support added                                                   */
/* 2000/12/14:                                                                */
/* - added support for ripping from floppies; currently this sometimes gives  */
/*   different results from a memory rip                                      */
/* - first release; it now features separate ripping and GUI code             */
/*   as well as the access being done via memory access wrapper functions     */
/*   formats detected are some ProTracker clones and SoundFX 1.3/2.0          */
/* ToDo:                                                                      */
/* -----                                                                      */
/* - the detection routines do only very rough checks of the values' validity */
/*   against MODRIP_MAXMODLEN; better look up the really allowed values       */
/*   somewhere                                                                */
/*============================================================================*/

/* fellow includes */
#include "defs.h"
#include "fmem.h"
#include "floppy.h"

/* own includes */
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>

#include "modrip.h"
#ifdef WIN32
#include "modrip_win32.h"
#else
#include "modrip_linux.h"
#endif

/* define this to have logfile output */
#define MODRIP_DEBUG

static unsigned modripModsFound;

/*===============================================================*/
/* saves mem for a detect module with a filled ModuleInfo struct */
/* gets the values via memory access function func               */
/*===============================================================*/

static BOOLE modripSaveMem(struct ModuleInfo *info, MemoryAccessFunc func)
{
  ULO i;
  FILE *modfile;

  if(info == NULL) return FALSE;

  if ((modfile = fopen(info->filename, "w+b")) == NULL)
    return FALSE;
  for (i = info->start; i <= info->end; i++)
    fputc((*func)(i), modfile);
  fclose(modfile);
#ifdef MODRIP_DEBUG
  fellowAddLog("mod-ripper wrote file %s.\n", info->filename);
#endif
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
  struct { char *ID; int channels; } formats[8] = {
	  {"M.K.", 4},  /* Standard ProTracker */
      {"M!K!", 4},  /* ProTracker w. more than 64 patterns */
	  {"4CHN", 4},  /* 4 channels */
	  {"6CHN", 6},  /* 6 channel ProTracker */
	  {"8CHN", 8},  /* 8 channels */
	  {"FLT4", 4},	/* Startrekker 4 channel */
	  {"FLT8", 8},  /* Startrekker 8 channel */
	  {"M&K!", 4}   /* Noisetracker */
  };
  struct ModuleInfo info;
  BOOLE ScratchyName;

  for(type = 0; type < 8; type++) {
    if ( ((*func)(address + 0) == formats[type].ID[0])
      && ((*func)(address + 1) == formats[type].ID[1])
      && ((*func)(address + 2) == formats[type].ID[2])
      && ((*func)(address + 3) == formats[type].ID[3])
       ) {
#ifdef MODRIP_DEBUG
      fellowAddLog("mod-ripper ProTracker %s match\n", formats[type].ID);
#endif
      modripModuleInfoInitialize(&info);

      /* store general info */
      strcpy(info.typedesc, "ProTracker and clones");
      strncpy(info.typesig, formats[type].ID, 4);
      info.typesig[4] = '\0';
      info.channels = formats[type].channels;

      /* searchstring found, now calc size */
      info.start = address - 0x438;

      /* get sample size */
      info.samplesize = 0;		

      for (i = 0; i <= 30; i++) {
        info.samplesize += (256 * (*func)(info.start + 0x2a + i * 0x1e)
                        +         (*func)(info.start + 0x2b + i * 0x1e)) * 2;
	  }

	  /* some disks like messing around :) */
#ifdef MODRIP_DEBUG
      fellowAddLog("samplesize = %u\n", info.samplesize);
#endif
      if(info.samplesize > MODRIP_MAXMODLEN) return;

      info.songlength = (*func)(info.start + 0x3b6);
#ifdef MODRIP_DEBUG
	  fellowAddLog("songlength = %u\n", info.songlength);
#endif
      if((info.songlength > MODRIP_MAXMODLEN) 
      || (info.songlength == 0)) return;

      /* scan for max. amount of patterns */
      info.maxpattern = 0;		

      for (i = 0; i <= info.songlength; i++) {
        if (info.maxpattern < (*func)(info.start + 0x3b8 + i)) {
          info.maxpattern = (*func)(info.start + 0x3b8 + i);
		}
	  }

#ifdef MODRIP_DEBUG
	  fellowAddLog("maxpattern = %u\n", info.maxpattern);
#endif
	  if(info.maxpattern > 127) return;		/* @@@@@ is this value correct ? */

      info.patternsize = (info.maxpattern + 1) * 64 * 4 * info.channels;

#ifdef MODRIP_DEBUG
	  fellowAddLog("patternsize = %u\n", info.patternsize);
#endif
      if(info.patternsize > MODRIP_MAXMODLEN) return;

      info.end = info.start + info.samplesize + info.patternsize + 0x43b;

#ifdef MODRIP_DEBUG
	  fellowAddLog("calculated range 0x%06x - 0x%06x\n", info.start, info.end);
#endif
	  if(info.end < info.start) return;

      if ((info.end - info.start < MODRIP_MAXMODLEN)) {
      /* get module name */
      for (i = 0; i < 20; i++) {
        info.modname[i] = (*func)(info.start + i);
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
          strcat(info.filename, ".mod");
		}
		else
          sprintf(info.filename, "mod%d.mod", modripModsFound++);
	  }
      else {
        sprintf(info.filename, "mod%d.mod", modripModsFound++);
	  }

	  if(modripGuiSaveRequest(&info)) 
        if(!modripSaveMem(&info, func))
          modripGuiErrorSave(&info);
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
#ifdef MODRIP_DEBUG
      fellowAddLog("mod-ripper SoundFX 1.3 (SONG) match\n");
#endif
      match = TRUE;
      info.start = address - 60;
      info.instruments = 15;
      strcpy(info.typedesc, "SoundFX 1.3");
      strcpy(info.typesig, "SONG");
	}
    if (((*func)(address + 2) == '3') && ((*func)(address + 3) == '1')) {
#ifdef MODRIP_DEBUG
      fellowAddLog("mod-ripper SoundFX 2.0 (SO31) match\n");
#endif
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
        size += (*func)(info.start + i*4 + 0) << 8     
              | (*func)(info.start + i*4 + 1)
              | (*func)(info.start + i*4 + 2) << 8 
              | (*func)(info.start + i*4 + 3);

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
#ifdef MODRIP_DEBUG
      fellowAddLog("patterns = %u\n", patterns);
#endif

      offset += 2;
	  size += 2;

      /* pattern table */
      for (i = 0; i < patterns; i++)
        info.maxpattern = max(info.maxpattern, (*func)(info.start + offset + i));

  	  if((info.maxpattern > MODRIP_MAXMODLEN) ||
         (info.maxpattern == 0)) return;
#ifdef MODRIP_DEBUG
      fellowAddLog("maxpattern = %u\n", info.maxpattern);
#endif
      size += 128 + ((info.maxpattern + 1) * 1024);
	  info.end = info.start + size;

#ifdef MODRIP_DEBUG
	  fellowAddLog("calculated range 0x%06x - 0x%06x\n", info.start, info.end);
#endif
	  if(info.end < info.start) return;
		
      if (size < MODRIP_MAXMODLEN) {
        /* set filename for the module file */
        sprintf(info.filename, "mod%d.cus", modripModsFound++);

      if(modripGuiSaveRequest(&info)) 
        if(!modripSaveMem(&info, func))
          modripGuiErrorSave(&info);
      }
    }
  }
}

/*===========================*/
/* detect SoundMon           */
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
#ifdef MODRIP_DEBUG
  fellowAddLog("mod-ripper found match for SoundMon (%s) at 0x%06x.\n",
    info.typesig, info.start);
#endif
  info.end = info.start;

  /* get number of instruments */
  if(version > 1) info.instruments = (*func)(info.start + 29);

  /* get patterns */
  patterns = (((*func)(info.start + 30)) << 8) | ((*func)(info.start + 31));
#ifdef MODRIP_DEBUG
  fellowAddLog("patterns = %u\n", patterns);
#endif
  if((patterns > MODRIP_MAXMODLEN) 
  || (patterns == 0)) return;

  offset += 32;
  for(i = 0; i < 15; i++) {
    if( (*func)(info.start + offset) != 255 ) {
      temp = (  (*func)(info.start + offset + 24) << 8 
              | (*func)(info.start + offset + 25)      ) * 2;
      if(temp > 0)
        info.end += temp;
	}
    offset += 32;
  }
  info.end += 512;

  for(i = 0; i < patterns * 4; i++) {
    temp = (*func)(info.start + offset + (i*4)) << 8
         | (*func)(info.start + offset + (i*4) + 1);
    info.maxpattern = max(info.maxpattern, temp);
  }
#ifdef MODRIP_DEBUG
  fellowAddLog("maxpattern = %u\n", info.maxpattern);
#endif
  if((info.maxpattern > MODRIP_MAXMODLEN) 
  || (info.maxpattern == 0)) return;

  info.end += (patterns * 16 + info.maxpattern * 48);
  info.end += info.instruments * 64;

#ifdef MODRIP_DEBUG
  fellowAddLog("calculated range 0x%06x - 0x%06x\n",
    info.start, info.end);
#endif
  if(info.end < info.start) return;

  if ((info.end - info.start < MODRIP_MAXMODLEN)) {
    /* get song name */
    for (i = 0; i < 26; i++) {
      info.modname[i] = (*func)(info.start + i);
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
        strcpy(info.filename, info.modname);
        strcat(info.filename, ".cus");
	  }
	  else
        sprintf(info.filename, "mod%d.cus", modripModsFound++);
	  }
     else {
      sprintf(info.filename, "mod%d.cus", modripModsFound++);
	 }

    if(modripGuiSaveRequest(&info)) 
      if(!modripSaveMem(&info, func))
        modripGuiErrorSave(&info);
  }     
}

/*====================================================*/
/* from here on starts the actual fellow ripping code */
/*====================================================*/

/*===================================================*/
/* here we define the formats that are actually used */
/*===================================================*/

#define MODRIP_KNOWNFORMATS 3

static ModuleDetectFunc DetectFunctions[MODRIP_KNOWNFORMATS] = {
  modripDetectProTracker,
  modripDetectSoundFX,
  modripDetectSoundMon
};

/*==============================================*/
/* scan the emulated amiga's memory for modules */
/*==============================================*/

void modripScanFellowMemory(void)
{
  ULO i;
  int j;

  if(modripGuiRipMemory()) {
#ifdef MODRIP_DEBUG
    fellowAddLog("mod-ripper now scanning memory...\n");
#endif
    for(i = 0; i <= 0xffffff; i++)
      for(j = 0; j < MODRIP_KNOWNFORMATS; j++)
	    (*DetectFunctions[j])(i, fetb);
  }
}

/*====================================*/
/* floppy cache memory access wrapper */
/*====================================*/

#define MODRIP_ADFSIZE   0xDC000
#define MODRIP_FLOPCACHE 0xFFFFF

/* meant to hold the read floppy cache */
static char *modripCurrentFloppyCache = NULL;

ULO modripFloppyCacheRead(ULO address)
{
  return(modripCurrentFloppyCache[address & MODRIP_FLOPCACHE]);
}

/*========================================*/
/* read a floppy image into a given cache */
/*========================================*/

BOOLE modripReadFloppyImage(char *filename, char *cache)
{
  FILE *f;
  char message[MODRIP_TEMPSTRLEN];
  ULO i;
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

void modripScanFellowFloppies(void)
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
#ifdef MODRIP_DEBUG
			fellowAddLog("mod-ripper %s\n", floppy[driveNo].imagenamereal);
#endif
          if(modripReadFloppyImage(floppy[driveNo].imagenamereal, cache))
            Read = TRUE;
		}
		else if(*floppy[driveNo].imagename) {
#ifdef MODRIP_DEBUG
			fellowAddLog("mod-ripper %s\n", floppy[driveNo].imagename);
#endif
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

/*============================================================================*/
/* Invokes the mod-ripping                                                    */
/*============================================================================*/

void modripRIP(void)
{
  modripGuiInitialize();

  modripGuiSetBusy();
  modripScanFellowMemory();
  modripScanFellowFloppies();
  modripGuiUnSetBusy();

  modripGuiUnInitialize();
}