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

#pragma once

#define FLOPPY_TRACKS 180

/* Status symbols */

enum class FLOPPY_STATUS_CODE
{
  FLOPPY_STATUS_NORMAL_OK,
  FLOPPY_STATUS_EXTENDED_OK,
  FLOPPY_STATUS_EXTENDED2_OK,
#ifdef FELLOW_SUPPORT_CAPS
  FLOPPY_STATUS_IPF_OK,
#endif
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

struct floppytrackinfostruct
{
  ULO file_offset; /* Track starts at this offset in image file, used for writing back */
  ULO mfm_length;  /* Length of mfm data in bytes */
  UBY *mfm_data;   /* Pointer to MFM data for track, including GAP, pointer to somewhere in floppy[].mfm_data */
};

/* Info about a drive */

struct floppyinfostruct
{
  FILE *F;                                        /* Open file for disk image */
  ULO tracks;                                     /* Number of tracks on drive */
  BOOLE zipped;                                   /* This image is zipped */
  ULO compress_serno;                             /* Tmp file serial number */
  BOOLE sel;                                      /* Drive selected when TRUE */
  ULO track;                                      /* Current head position */
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
  ULO motor_ticks;                                /* EOLs since motor was started */
  ULO insertedframe;                              /* Will not be detected until some time after this */
  ULO idcount;                                    /* Number of times ID has been read */
  UBY *mfm_data;                                  /* Memory allocated to hold mfm data for the entire drive */
  floppytrackinfostruct trackinfo[FLOPPY_TRACKS]; /* Info about each track */
  FLOPPY_STATUS_CODE imagestatus;                 /* Status of drive (kind of image inserted) */
  FLOPPY_ERROR_CODE imageerror;                   /* What kind of error if status reports error */
  STR imagename[CFG_FILENAME_LENGTH];             /* Image name presented to user */
  STR imagenamereal[CFG_FILENAME_LENGTH];         /* Image name used internally */
#ifdef FELLOW_SUPPORT_CAPS
  BOOLE flakey; /* introduced for CAPS support */
  ULO *timebuf; /* dito */
#endif
};

/* Info about a started disk transfer */

struct floppyDMAinfostruct
{
  ULO dskpt;     /* Amiga memory pt */
  ULO wordsleft; /* Words left to transfer */
  ULO wait;      /* Used to delay irq for writes */
  BOOLE wait_for_sync;
  BOOLE sync_found;
  BOOLE dont_use_gap;
};

/* Config */

#define FLOPPY_WAIT_INITIAL 10

extern floppyinfostruct floppy[4];
extern floppyDMAinfostruct floppy_DMA;
extern BOOLE floppy_fast;
extern BOOLE floppy_DMA_started;
extern ULO diskDMAen; /* Counts the number of writes to dsklen */

extern void floppyDMAStart();
extern LON floppySelectedGet();
extern void floppySelectedSet(ULO selbits);
extern BOOLE floppyIsTrack0(ULO drive);
extern BOOLE floppyIsWriteProtected(ULO drive);
extern BOOLE floppyIsReady(ULO drive);
extern BOOLE floppyIsChanged(ULO drive);
extern void floppyMotorSet(ULO drive, bool mtr);
extern void floppySideSet(BOOLE s);
extern void floppyDirSet(BOOLE dr);
extern void floppyStepSet(BOOLE stp);
extern bool floppyImageADFCreate(STR *, STR *, bool, bool, bool);
extern void floppyImageRemove(ULO drive);
extern bool floppyValidateAmigaDOSVolumeName(const STR *);

extern bool floppyGetEnabled(unsigned int driveIndex);
extern bool floppyGetMotor(unsigned int driveIndex);
unsigned int floppyGetTrack(unsigned int driveIndex);
extern bool floppyGetIsWriting(unsigned int driveIndex);

/* Configuration */

extern void floppySetDiskImage(ULO drive, const STR *diskname);
extern void floppySetEnabled(ULO drive, BOOLE enabled);
extern void floppySetReadOnlyConfig(ULO drive, BOOLE readonly);
extern void floppySetFastDMA(BOOLE fastDMA);

/* Module control */

extern void floppyHardReset();
extern void floppyEmulationStart();
extern void floppyEmulationStop();
extern void floppyStartup();
extern void floppyShutdown();

extern void floppyEndOfLine();
