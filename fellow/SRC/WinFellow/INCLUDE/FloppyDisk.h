#pragma once

/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* Floppy Emulation                                                        */
/*                                                                         */
/* Authors: Petter Schau                                                   */
/*          Torsten Enderling                                              */
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

#include <cstdint>
#include <cstdio>

#include "Defs.h"

#define FLOPPY_TRACKS 180

/* Status symbols */

enum class FLOPPY_STATUS_CODE
{
  FLOPPY_STATUS_NORMAL_OK,
  FLOPPY_STATUS_EXTENDED_OK,
  FLOPPY_STATUS_EXTENDED2_OK,
  FLOPPY_STATUS_IPF_OK,
  FLOPPY_STATUS_ERROR,
  FLOPPY_STATUS_NONE
};

enum class FLOPPY_ERROR_CODE
{
  FLOPPY_ERROR_EXISTS_NOT,
  FLOPPY_ERROR_COMPRESS,
  FLOPPY_ERROR_COMPRESS_NOTMP,
  FLOPPY_ERROR_COMPRESS_TMPFILEOPEN,
  FLOPPY_ERROR_FILE,
  FLOPPY_ERROR_SIZE
};

/* Info about a track */

typedef struct
{
  uint32_t file_offset; /* Track starts at this offset in image file, used for writing back */
  uint32_t mfm_length;  /* Length of mfm data in bytes */
  uint8_t *mfm_data;    /* Pointer to MFM data for track, including GAP, pointer to somewhere in floppy[].mfm_data */
} floppytrackinfostruct;

/* Info about a drive */

typedef struct
{
  FILE *F;                                        /* Open file for disk image */
  uint32_t tracks;                                /* Number of tracks on drive */
  BOOLE zipped;                                   /* This image is zipped */
  uint32_t compress_serno;                        /* Tmp file serial number */
  BOOLE sel;                                      /* Drive selected when TRUE */
  uint32_t track;                                 /* Current head position */
  BOOLE writeprotconfig;                          /* Drive is write-protected via user configuration */
  BOOLE writeprotenforce;                         /* Drive is write-protected because of read-only filesystem or IPF image */
  BOOLE dir;                                      /* Direction for step */
  BOOLE motor;                                    /* Motor on or off */
  BOOLE side;                                     /* Which side is active */
  BOOLE step;                                     /* Step line */
  BOOLE enabled;                                  /* Drive enabled */
  BOOLE changed;                                  /* Disk has been changed */
  BOOLE idmode;                                   /* Drive is in ID-mode */
  BOOLE inserted;                                 /* Disk is inserted */
  uint32_t motor_ticks;                           /* EOLs since motor was started */
  uint32_t insertedframe;                         /* Will not be detected until some time after this */
  uint32_t idcount;                               /* Number of times ID has been read */
  uint8_t *mfm_data;                              /* Memory allocated to hold mfm data for the entire drive */
  floppytrackinfostruct trackinfo[FLOPPY_TRACKS]; /* Info about each track */
  FLOPPY_STATUS_CODE imagestatus;                 /* Status of drive (kind of image inserted) */
  FLOPPY_ERROR_CODE imageerror;                   /* What kind of error if status reports error */
  char imagename[CFG_FILENAME_LENGTH];            /* Image name presented to user */
  char imagenamereal[CFG_FILENAME_LENGTH];        /* Image name used internally */
#ifdef FELLOW_SUPPORT_CAPS
  BOOLE flakey;      /* introduced for CAPS support */
  uint32_t *timebuf; /* dito */
#endif
} floppyinfostruct;

/* Info about a started disk transfer */

typedef struct
{
  uint32_t dskpt;     /* Amiga memory pt */
  uint32_t wordsleft; /* Words left to transfer */
  uint32_t wait;      /* Used to delay irq for writes */
  BOOLE wait_for_sync;
  BOOLE sync_found;
  BOOLE dont_use_gap;
} floppyDMAinfostruct;

/* Config */

#define FLOPPY_WAIT_INITIAL 10

extern floppyinfostruct floppy[4];
extern floppyDMAinfostruct floppy_DMA;
extern BOOLE floppy_fast;
extern BOOLE floppy_DMA_started;
extern uint32_t diskDMAen; /* Counts the number of writes to dsklen */

extern void floppyDMAStart();
extern void floppyStartup();
extern void floppyShutdown();
extern int32_t floppySelectedGet();
extern void floppySelectedSet(uint32_t selbits);
extern BOOLE floppyIsTrack0(uint32_t drive);
extern BOOLE floppyIsWriteProtected(uint32_t drive);
extern BOOLE floppyIsReady(uint32_t drive);
extern BOOLE floppyIsChanged(uint32_t drive);
extern void floppyMotorSet(uint32_t drive, BOOLE mtr);
extern void floppySideSet(BOOLE s);
extern void floppyDirSet(BOOLE dr);
extern void floppyStepSet(BOOLE stp);
extern bool floppyImageADFCreate(char *, char *, bool, bool, bool);
extern void floppyImageRemove(uint32_t drive);
extern bool floppyValidateAmigaDOSVolumeName(const char *);
/* Configuration */

extern void floppySetDiskImage(uint32_t drive, const char *diskname);
extern void floppySetEnabled(uint32_t drive, BOOLE enabled);
extern void floppySetReadOnlyConfig(uint32_t drive, BOOLE readonly);
extern void floppySetFastDMA(BOOLE fastDMA);

/* Module control */

extern void floppyHardReset();
extern void floppyEmulationStart();
extern void floppyEmulationStop();
extern void floppyStartup();
extern void floppyShutdown();

extern void floppyEndOfLine();
