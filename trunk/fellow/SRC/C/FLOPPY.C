/* @(#) $Id: FLOPPY.C,v 1.16 2004-05-28 13:30:19 carfesh Exp $ */
/*=========================================================================*/
/* Fellow Amiga Emulator                                                   */
/*                                                                         */
/* Floppy Emulation                                                        */
/*                                                                         */
/* Authors: Petter Schau (peschau@online.no)                               */
/*          Torsten Enderling (carfesh@gmx.net)                            */
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


#include "portable.h"
#include "renaming.h"

#include "defs.h"
#include "fellow.h"
#include "fmem.h"
#include "cpu.h"
#include "floppy.h"
#include "draw.h"
#include "fswrap.h"
#include "graph.h"
#include "cia.h"

#include "xdms.h"
#include "zlibwrap.h"

#define MFM_FILLB 0xaa
#define MFM_FILLL 0xaaaaaaaa
#define MFM_MASK  0x55555555
#define FLOPPY_INSERTED_DELAY 150
/* Andromeda Sequential assumes at least 640 bytes of gap,
   Bernd uses 720 */
#define FLOPPY_GAP_BYTES 720
#define FLOPPY_FAST_WORDS 128

/*---------------*/
/* Configuration */
/*---------------*/

floppyinfostruct floppy[4];              /* Info about drives */ 
BOOLE floppy_fast;                       /* Select fast floppy transfer */
UBY tmptrack[20*1024*11];                /* Temporary track buffer */
floppyDMAinfostruct floppy_DMA;          /* Info about a DMA transfer */
BOOLE floppy_DMA_started;                /* Disk DMA started */
BOOLE floppy_DMA_read;                   /* DMA read or write */
BOOLE floppy_has_sync;

/*-----------------------------------*/
/* Disk registers and help variables */
/*-----------------------------------*/

ULO dsklen, dsksync, dskpt, adcon;       /* Registers */
ULO diskDMAen;                           /* Write counter for dsklen */

/*=======================*/
/* Register access stubs */
/*=======================*/

/*----------*/
/* DSKBYTR  */
/* $dff01a  */
/*----------*/

ULO rdskbytrC(ULO address) {
  ULO tmp = floppy_DMA_started<<14;
  if (dsklen & 0x4000) tmp |= 0x2000;
  if (floppy_has_sync) tmp |= 0x1000;
  return tmp;
}

/*----------*/
/* DSKPTH   */
/* $dff020  */
/*----------*/

void wdskpthC(ULO data, ULO address) {
  *(((UWO *) &dskpt) + 1) = (UWO) (data & 0x1f);
}

/*----------*/
/* DSKPTL   */
/* $dff022  */
/*----------*/

void wdskptlC(ULO data, ULO address) {
  *((UWO *) &dskpt) = (UWO) (data & 0xfffe);
}

/*----------*/
/* DSKLEN   */
/* $dff024  */
/*----------*/

void wdsklenC(ULO data, ULO address) {
  dsklen = data;
  if (data & 0x8000)
    if (++diskDMAen >= 2)
      floppyDMAStart();
}

/*----------*/
/* DSKSYNC  */
/* $dff07e  */
/*----------*/

void wdsksyncC(ULO data, ULO address) {
  dsksync = data;
}

/*==================================*/
/* Some functions used from cia.c   */
/*==================================*/

/* Several drives can be selected, we simply return the first */

LON floppySelectedGet(void) {
  LON i = 0;
  while (i < 4) {
    if (floppy[i].sel) return i;
    i++;
  }
  return -1;
}

void floppySelectedSet(ULO selbits) {
  ULO i;
  for (i = 0; i < 4; i++) {
    floppy[i].sel = ((selbits & 1) == 0);
    selbits >>= 1;
  }
}

BOOLE floppyIsTrack0(ULO drive) {
  if (drive != -1)
    return (floppy[drive].track == 0);
  return FALSE;
}

BOOLE floppyIsWriteProtected(ULO drive) {
  if (drive != -1)
    return floppy[drive].writeprot;
  return FALSE;
}

BOOLE floppyIsReady(ULO drive) {
  if (drive != -1)
    if (floppy[drive].enabled) {
      if (floppy[drive].idmode)
        return (floppy[drive].idcount++ < 32);
      else
        return (floppy[drive].motor && floppy[drive].inserted);
    }
  return FALSE;
}

BOOLE floppyIsChanged(ULO drive) {
  if (drive != -1) return floppy[drive].changed;
  return TRUE;
}

/* If motor is turned off, idmode is reset and on */
/* If motor is turned on, idmode is off */
/* Cia detects the change in SEL from high to low and calls this when needed */

void floppyMotorSet(ULO drive, BOOLE mtr) {
  if (floppy[drive].motor && mtr) {
    floppy[drive].idmode = TRUE;
    floppy[drive].idcount = 0;
  }
  else if (!floppy[drive].motor && !mtr) {
    floppy[drive].idmode = FALSE;
    floppy[drive].motor_ticks = 0;
  }
  if (floppy[drive].motor != (!mtr)) drawSetLED(drive, !mtr);
  floppy[drive].motor = !mtr;
}

void floppySideSet(BOOLE s) {
  ULO i;
  for (i = 0; i < 4; i++) floppy[i].side = !s;
}
    
void floppyDirSet(BOOLE dr) {
  ULO i;
  for (i = 0; i < 4; i++) floppy[i].dir = dr;
}

void floppyStepSet(BOOLE stp) {
  ULO i;
  for (i = 0; i < 4; i++) {
    if (floppy[i].sel) {
      if (!stp && floppy[i].changed && floppy[i].inserted &&
	  ((draw_frame_count - floppy[i].insertedframe) > FLOPPY_INSERTED_DELAY))
	floppy[i].changed = FALSE;
      if (!floppy[i].step && !stp) {
        if (!floppy[i].dir) {
          if ((floppy[i].track + 1) < floppy[i].tracks)
	    floppy[i].track++;
          }
        else {
          if (floppy[i].track > 0) floppy[i].track--;
        }
      }
      floppy[i].step = !stp;
    }
  }
}

/*=============================================*/
/* Will MFM encode one sector from src to dest */
/*=============================================*/

void floppySectorMfmEncode(ULO tra, ULO sec, UBY *src, UBY *dest, ULO sync) {
  ULO tmp, x, odd, even, hck = 0, dck = 0;

  /* Preamble and sync */

  *(dest + 0) = 0xaa;
  *(dest + 1) = 0xaa;
  *(dest + 2) = 0xaa;
  *(dest + 3) = 0xaa;
  *(dest + 4) = (UBY) (sync>>8);
  *(dest + 5) = (UBY) (sync & 0xff);
  *(dest + 6) = (UBY) (sync>>8);
  *(dest + 7) = (UBY) (sync & 0xff);

  /* Track and sector info */

  tmp = 0xff000000 | (tra<<16) | (sec<<8) | (11 - sec);
  odd = (tmp & MFM_MASK) | MFM_FILLL;
  even = ((tmp>>1) & MFM_MASK) | MFM_FILLL;
  *(dest +  8) = (UBY) ((even & 0xff000000)>>24);
  *(dest +  9) = (UBY) ((even & 0xff0000)>>16);
  *(dest + 10) = (UBY) ((even & 0xff00)>>8);
  *(dest + 11) = (UBY) ((even & 0xff));
  *(dest + 12) = (UBY) ((odd & 0xff000000)>>24);
  *(dest + 13) = (UBY) ((odd & 0xff0000)>>16);
  *(dest + 14) = (UBY) ((odd & 0xff00)>>8);
  *(dest + 15) = (UBY) ((odd & 0xff));

  /* Fill unused space */

  for (x = 16 ; x < 48; x++)
    *(dest + x) = 0x55;

  /* Encode data section of sector */

  for (x = 64 ; x < 576; x++) {
    tmp = *(src + x - 64);
    odd = (tmp & 0x55);
    even = (tmp>>1) & 0x55;
    *(dest + x) = (UBY) (even | MFM_FILLB);
    *(dest + x + 512) = (UBY) (odd | MFM_FILLB);
  }

  /* Calculate checksum for unused space */

  for(x = 8; x < 48; x += 4)
    hck ^= (((ULO) *(dest + x))<<24) | (((ULO) *(dest + x + 1))<<16) |
           (((ULO) *(dest + x + 2))<<8) |  ((ULO) *(dest + x + 3));
  even = odd = hck; 
  odd >>= 1;
  even |= MFM_FILLL;
  odd |= MFM_FILLL;
  *(dest + 48) = (UBY) ((odd & 0xff000000)>>24);
  *(dest + 49) = (UBY) ((odd & 0xff0000)>>16);
  *(dest + 50) = (UBY) ((odd & 0xff00)>>8);
  *(dest + 51) = (UBY) (odd & 0xff);
  *(dest + 52) = (UBY) ((even & 0xff000000)>>24);
  *(dest + 53) = (UBY) ((even & 0xff0000)>>16);
  *(dest + 54) = (UBY) ((even & 0xff00)>>8);
  *(dest + 55) = (UBY) (even & 0xff);

  /* Calculate checksum for data section */

  for(x = 64; x < 1088; x += 4)
    dck ^= (((ULO) *(dest + x))<<24) | (((ULO) *(dest + x + 1))<<16) |
           (((ULO) *(dest + x + 2))<< 8) |  ((ULO) *(dest + x + 3));
  even = odd = dck;
  odd >>= 1;
  even |= MFM_FILLL;
  odd |= MFM_FILLL;
  *(dest + 56) = (UBY) ((odd & 0xff000000)>>24);
  *(dest + 57) = (UBY) ((odd & 0xff0000)>>16);
  *(dest + 58) = (UBY) ((odd & 0xff00)>>8);
  *(dest + 59) = (UBY) (odd & 0xff);
  *(dest + 60) = (UBY) ((even & 0xff000000)>>24);
  *(dest + 61) = (UBY) ((even & 0xff0000)>>16);
  *(dest + 62) = (UBY) ((even & 0xff00)>>8);
  *(dest + 63) = (UBY) (even & 0xff);
}

void floppyGapMfmEncode(UBY *dst) {
  ULO i;
  for (i = 0; i < FLOPPY_GAP_BYTES; i++) *dst++ = MFM_FILLB;
}

/*===========================================================*/
/* Decode one MFM sector from src to dst                     */
/* Warning: Amiga OS appears to tag the gap with a sync..?   */
/* src is pointing after sync words on entry                 */
/* Returns the sector number found in the MFM encoded header */
/*===========================================================*/

ULO floppySectorMfmDecode(UBY *src, UBY *dst, ULO track) {
  ULO src_sector, src_track, src_ff;
  ULO odd, even, i;
  odd = (src[0]<<24) | (src[1]<<16) | (src[2]<<8) | src[3];
  even = (src[4]<<24) | (src[5]<<16) | (src[6]<<8) | src[7];
  even &= MFM_MASK;
  odd  = (odd & MFM_MASK) << 1;
  even |= odd;
  src_ff = (even & 0xff000000)>>24;
  src_sector = (even & 0xff00)>>8;
  src_track = (even & 0xff0000)>>16;
  if ((src_ff != 0xff) || (src_sector > 10) || (src_track != track))
    return 0xffffffff;
  src += (48 + 8);
  for (i = 0; i < 512; i++) {
    even = (*(src + i)) & MFM_MASK;
    odd = (*(src + i + 512)) & MFM_MASK;
    *(dst++) = (UBY) ((even<<1) | odd);
  }
  return src_sector;
}

/*=================================================================*/
/* Save one sector to disk and cache                               */
/* mfmsrc points to after the sync words somewhere in Amiga memory */
/* It uses tmptrack as scratchpad                                  */
/* returns TRUE if this really was a sector                        */
/*=================================================================*/

BOOLE floppySectorSave(ULO drive, ULO track, UBY *mfmsrc) {
  ULO sector;
  if (!floppy[drive].writeprot) {
    if ((sector = floppySectorMfmDecode(mfmsrc, tmptrack, track)) < 11) {
      fseek(floppy[drive].F,
	    floppy[drive].trackinfo[track].file_offset + sector*512, SEEK_SET);
      fwrite(tmptrack, 1, 512, floppy[drive].F);
      memcpy(floppy[drive].trackinfo[track].mfm_data + sector*1088, mfmsrc - 8, 1088);
    }
    else {
      return FALSE;
    }
  }
  return TRUE;
}

/*===============================*/
/* Make one track of MFM data    */
/* Track is MFM tracks (0 - 163) */
/*===============================*/

void floppyTrackMfmEncode(ULO track, UBY *src, UBY *dst, ULO sync) {
  ULO i;
  for (i = 0; i < 11; i++)
    floppySectorMfmEncode(track, i, src + i*512, dst + i*1088, sync);
  floppyGapMfmEncode(dst + 11*1088);
}

/*=======================================================*/
/* Load AmigaDOS track from disk into the track buffer   */
/* Track is MFM tracks (0 - 163)                         */
/*=======================================================*/

void floppyTrackLoad(ULO drive, ULO track) {
  fseek(floppy[drive].F, track*5632, SEEK_SET);
  fread(tmptrack, 1, 5632, floppy[drive].F);
  floppyTrackMfmEncode(track, tmptrack, floppy[drive].trackinfo[track].mfm_data, 0x4489);
}

/*======================*/
/* Set error conditions */
/*======================*/

void floppyError(ULO drive, ULO errorID) {
  floppy[drive].imagestatus = FLOPPY_STATUS_ERROR;
  floppy[drive].imageerror = errorID;
  floppy[drive].inserted = FALSE;
  if (floppy[drive].F != NULL) {
    fclose(floppy[drive].F);
    floppy[drive].F = NULL;
  }
}

/*===============================*/
/* Handling of Compressed Images */
/*===============================*/

/*=========================================*/
/* Get a temporary file name               */
/* if TEMP environment variable is set try */
/* to create in temporary folder, else in  */
/* the volumes rootdir                     */
/*=========================================*/

static char *TemporaryFilename(void)
{
  char *tempvar;
  char *result;

  tempvar = getenv("TEMP");
  if( tempvar != NULL )
    result = tempnam(tempvar, "wftemp");
  else
	result = tmpnam(NULL);
  return result;
}

/*=========================*/
/* Uncompress a BZip image */
/*=========================*/

BOOLE floppyImageCompressedBZipPrepare(STR *diskname, ULO drive) {
  char *gzname;
  STR cmdline[512];

  if( (gzname = TemporaryFilename()) == NULL)
  {
    floppyError(drive, FLOPPY_ERROR_COMPRESS_TMPFILEOPEN);
	return FALSE;
  }

  sprintf(cmdline, "bzip2.exe -k -d -s -c %s > %s", diskname, gzname);
  system(cmdline);
  strcpy(floppy[drive].imagenamereal, gzname);
  free(gzname);
  floppy[drive].zipped = TRUE;
  return TRUE;
}

/*========================*/
/* Uncompress a DMS image */
/*========================*/

BOOLE floppyImageCompressedDMSPrepare(STR *diskname, ULO drive) {
  char *gzname;
  if( (gzname = TemporaryFilename()) == NULL)
  {
    floppyError(drive, FLOPPY_ERROR_COMPRESS_TMPFILEOPEN);
	return FALSE;
  }

  if(dmsUnpack(diskname, gzname) != 0) return FALSE;

  strcpy(floppy[drive].imagenamereal, gzname);
  free(gzname);
  floppy[drive].zipped = TRUE;
  return TRUE;
}

/*============================*/
/* Uncompress a GZipped image */
/*============================*/

BOOLE floppyImageCompressedGZipPrepare(STR *diskname, ULO drive) {
  char *gzname;
  if( (gzname = TemporaryFilename()) == NULL)
  {
    floppyError(drive, FLOPPY_ERROR_COMPRESS_TMPFILEOPEN);
	return FALSE;
  }

  if(!gzUnpack(diskname, gzname)) return FALSE;

  strcpy(floppy[drive].imagenamereal, gzname);
  free(gzname);
  floppy[drive].zipped = TRUE;
  if((access(diskname, 2 )) == -1 )
    floppy[drive].writeprot = TRUE;
  return TRUE;
}

/*=================================*/
/* Remove TMP image of zipped file */
/*=================================*/

void floppyImageCompressedRemove(ULO drive) {
  if (floppy[drive].zipped) {
	if( (!floppy[drive].writeprot) && 
        ((access(floppy[drive].imagename, 2 )) != -1 )) {
      STR *dotptr = strrchr(floppy[drive].imagename, '.');
      if (dotptr != NULL) {
        if ((strcmpi(dotptr, ".gz") == 0) ||
	       (strcmpi(dotptr, ".z") == 0) ||
		   (strcmpi(dotptr, ".adz") == 0)) {
          if(!gzPack(floppy[drive].imagenamereal, floppy[drive].imagename))
		    fellowAddLog("floppyImageCompressedRemove(): Couldn't recompress file %s\n", 
              floppy[drive].imagename);
          else
            fellowAddLog("floppyImageCompressedRemove(): Succesfully recompressed file %s\n",
		    floppy[drive].imagename);
		}
	  }
	}
	floppy[drive].zipped = FALSE;
	remove(floppy[drive].imagenamereal);
  }
}

/*============================*/
/* Uncompress an image to TMP */
/* Returns TRUE if image was  */
/* compressed.                */
/*============================*/

BOOLE floppyImageCompressedPrepare(STR *diskname, ULO drive) {
  STR *dotptr = strrchr(diskname, '.');
  if (dotptr == NULL) return FALSE;
  if((strcmpi(dotptr, ".bz2") == 0) ||
     (strcmpi(dotptr, ".bz") == 0))
    return floppyImageCompressedBZipPrepare(diskname, drive);
  else if ((strcmpi(dotptr, ".gz") == 0) ||
	   (strcmpi(dotptr, ".z") == 0) ||
	   (strcmpi(dotptr, ".adz") == 0))
    return floppyImageCompressedGZipPrepare(diskname, drive);
  else if (strcmpi(dotptr, ".dms") == 0)
    return floppyImageCompressedDMSPrepare(diskname, drive);
  return FALSE;
}

/*==============================*/
/* Remove disk image from drive */
/*==============================*/

void floppyImageRemove(ULO drive) {
  if (floppy[drive].F != NULL) {
    fclose(floppy[drive].F);
    floppy[drive].F = NULL;
  }
  if (floppy[drive].imagestatus == FLOPPY_STATUS_NORMAL_OK ||
      floppy[drive].imagestatus == FLOPPY_STATUS_EXTENDED_OK) {
    if (floppy[drive].zipped)
      floppyImageCompressedRemove(drive);
  }
  floppy[drive].imagestatus = FLOPPY_STATUS_NONE;
  floppy[drive].inserted = FALSE;
  floppy[drive].changed = TRUE;
}

/*========================================================*/
/* Prepare a disk image, ie. uncompress and report errors */
/*========================================================*/

void floppyImagePrepare(STR *diskname, ULO drive) {
  if (!floppyImageCompressedPrepare(diskname, drive)) {
    if (floppy[drive].imagestatus != FLOPPY_STATUS_ERROR) {
      strcpy(floppy[drive].imagenamereal, diskname);
      floppy[drive].zipped = FALSE;
    }
  }
}

/*=======================================*/
/* Check type of image, number of tracks */
/* Returns the image status              */
/*=======================================*/

ULO floppyImageGeometryCheck(fs_navig_point *fsnp, ULO drive) {
  STR head[8];
  fread(head, 1, 8, floppy[drive].F);
  if (strncmp(head, "UAE--ADF", 8) == 0) {
    floppy[drive].imagestatus = FLOPPY_STATUS_EXTENDED_OK;
    floppy[drive].tracks = 80;
  }
  else {
    floppy[drive].tracks = fsnp->size / 11264;
    if ((floppy[drive].tracks*11264) != (ULO) fsnp->size)
      floppyError(drive, FLOPPY_ERROR_SIZE);
    else
      floppy[drive].imagestatus = FLOPPY_STATUS_NORMAL_OK;
    if (floppy[drive].track >= floppy[drive].tracks)
      floppy[drive].track = 0;
  }
  return floppy[drive].imagestatus;
}

/*======================*/
/* Load normal ADF file */
/*======================*/

void floppyImageNormalLoad(ULO drive) {
  ULO i;
  for (i = 0; i < floppy[drive].tracks*2; i++) {
    floppy[drive].trackinfo[i].file_offset = i*5632;
    floppy[drive].trackinfo[i].mfm_length = 11968 + FLOPPY_GAP_BYTES;
    floppy[drive].trackinfo[i].mfm_data = floppy[drive].mfm_data + i*(11968 + FLOPPY_GAP_BYTES); 
    floppyTrackLoad(drive, i);
  }
  floppy[drive].inserted = TRUE;
  floppy[drive].insertedframe = draw_frame_count;
}

/*========================*/
/* Load extended ADF file */
/*========================*/

void floppyImageExtendedLoad(ULO drive) {
  ULO i;
  ULO file_offset; /* position of current track in the image file */
  ULO mfm_offset;  /* position of current track in the floppy cache */
  UBY tinfo[4];
  ULO syncs[160], lengths[160];

  /* read table from header containing sync and length words */
  fseek(floppy[drive].F, SEEK_SET, 8);
  for (i = 0; i < floppy[drive].tracks*2; i++) {
    fread(tinfo, 1, 4, floppy[drive].F);
    syncs[i] = (((ULO)tinfo[0]) << 8) | ((ULO) tinfo[1]);
    lengths[i] = (((ULO)tinfo[2]) << 8) | ((ULO) tinfo[3]);
  }

  /* initial offset of track data in file, and in the internal mfm_buffer */
  file_offset = floppy[drive].tracks*8 + 8;
  mfm_offset = 0;
  /* read the actual track data */
  fseek(floppy[drive].F, SEEK_SET, file_offset);
  for (i = 0; i < floppy[drive].tracks*2; i++) {
    floppy[drive].trackinfo[i].mfm_data = floppy[drive].mfm_data + mfm_offset;
    floppy[drive].trackinfo[i].file_offset = file_offset;
    if (!syncs[i]) /* Sync = 0 means AmigaDOS tracks (Stored non-MFM encoded) */
    { 
      floppy[drive].trackinfo[i].mfm_length = 11968 + FLOPPY_GAP_BYTES;
      mfm_offset += floppy[drive].trackinfo[i].mfm_length;
      fread(tmptrack, 1, lengths[i], floppy[drive].F);
      floppyTrackMfmEncode(i, tmptrack, floppy[drive].trackinfo[i].mfm_data, 0x4489);
    }
    else /* raw MFM tracks */
    {
      mfm_offset  += lengths[i] + 2;
      floppy[drive].trackinfo[i].mfm_length = lengths[i] + 2;
      floppy[drive].trackinfo[i].mfm_data[0] = (UBY) (syncs[i] >> 8);
      floppy[drive].trackinfo[i].mfm_data[1] = (UBY) (syncs[i] & 0xff);
      fread(floppy[drive].trackinfo[i].mfm_data + 2, 1, lengths[i], floppy[drive].F);
    }
    file_offset += lengths[i];
  }     
  floppy[drive].inserted = TRUE;
  floppy[drive].insertedframe = draw_frame_count;
}

/*==============================*/
/* Insert an image into a drive */
/*==============================*/

void floppySetDiskImage(ULO drive, STR *diskname) {
  fs_navig_point *fsnp;
  if (strcmp(diskname, floppy[drive].imagename) == 0) return; /* Same image */
  floppyImageRemove(drive);
  if (strcmp(diskname, "") == 0) {
    floppy[drive].inserted = FALSE;
    floppy[drive].imagestatus = FLOPPY_STATUS_NONE;
    strcpy(floppy[drive].imagename, "");
  }
  else {
    if ((fsnp = fsWrapMakePoint(diskname)) == NULL)
      floppyError(drive, FLOPPY_ERROR_EXISTS_NOT);
    else {
      if (fsnp->type != FS_NAVIG_FILE)
	floppyError(drive, FLOPPY_ERROR_FILE);
      else {
	floppyImagePrepare(diskname, drive);
	if (floppy[drive].zipped) {
	  free(fsnp);
	  if ((fsnp = fsWrapMakePoint(floppy[drive].imagenamereal)) == NULL)
	    floppyError(drive, FLOPPY_ERROR_COMPRESS);
	}
	if (floppy[drive].imagestatus != FLOPPY_STATUS_ERROR) {
	  floppy[drive].writeprot = !fsnp->writeable;
	  if ((floppy[drive].F = fopen(floppy[drive].imagenamereal,
			  (floppy[drive].writeprot ? "rb" : "r+b")))
		  == NULL)
	    floppyError(drive, (floppy[drive].zipped) ?
				   FLOPPY_ERROR_COMPRESS : FLOPPY_ERROR_FILE);
	  else {
	    strcpy(floppy[drive].imagename, diskname);
	    switch (floppyImageGeometryCheck(fsnp, drive)) {
	      case FLOPPY_STATUS_NORMAL_OK:
		floppyImageNormalLoad(drive);
		break;
	      case FLOPPY_STATUS_EXTENDED_OK:
		floppyImageExtendedLoad(drive);
		break;
	      default:
		/* Error already set by floppyImageGeometryCheck() */
		break;
	    }
	  }
	}
      }
    free(fsnp);
    }
  }
}

/*============================================================================*/
/* Set enabled flag for a drive                                               */
/*============================================================================*/

void floppySetEnabled(ULO drive, BOOLE enabled) {
  floppy[drive].enabled = enabled;
}

/*============================================================================*/
/* Set read-only flag for a drive                                             */
/*============================================================================*/

void floppySetReadOnly(ULO drive, BOOLE readonly) {
  floppy[drive].writeprot = readonly;
}

/*============================================================================*/
/* Set fast DMA flag                                                          */
/*============================================================================*/

void floppySetFastDMA(BOOLE fastDMA) {
  floppy_fast = fastDMA;
}

/*========================*/
/* Initial drive settings */
/*========================*/

void floppyDriveTableInit(void) {
  ULO i;
  for (i = 0; i < 4; i++) {
    floppy[i].F = NULL;
    floppy[i].sel = FALSE;
    floppy[i].track = 0;
    floppy[i].writeprot = FALSE;
    floppy[i].dir = FALSE;
    floppy[i].motor = FALSE;
    floppy[i].motor_ticks = 0;
    floppy[i].side = FALSE;
    floppy[i].inserted = FALSE;
    floppy[i].insertedframe = 0;
    floppy[i].idmode = FALSE;
    floppy[i].idcount = 0;
    floppy[i].imagename[0] = '\0';
    floppy[i].imagenamereal[0] = '\0';
    floppy[i].imagestatus = FLOPPY_STATUS_NONE;
    floppy[i].zipped = FALSE;
    floppy[i].changed = TRUE;
    /* Need to be large enough to hold UAE--ADF encoded tracks */
    floppy[i].mfm_data = (UBY *) malloc(FLOPPY_TRACKS*25000);
  }
}

void floppyDriveTableReset(void) {
  ULO i;
  for (i = 0; i < 4; i++) {
    floppy[i].sel = FALSE;
    floppy[i].track = 0;
    floppy[i].writeprot = FALSE;
    floppy[i].dir = FALSE;
    floppy[i].motor = FALSE;
    floppy[i].motor_ticks = 0;
    floppy[i].side = FALSE;
    floppy[i].insertedframe = draw_frame_count + FLOPPY_INSERTED_DELAY + 1;
    floppy[i].idmode = FALSE;
    floppy[i].idcount = 0;
    drawSetLED(i, FALSE);
  }
}

/*==================*/
/* Free memory used */
/*==================*/

void floppyMfmDataFree(void) {
  ULO i;
  for (i = 0; i < 4; i++)
    if (floppy[i].mfm_data != NULL)
      free(floppy[i].mfm_data);
}

/*============================*/
/* Install IO register stubs  */
/*============================*/

void floppyIOHandlersInstall(void) {
  memorySetIOReadStub(0x1a, rdskbytr);
  memorySetIOWriteStub(0x20, wdskpth);
  memorySetIOWriteStub(0x22, wdskptl);
  memorySetIOWriteStub(0x24, wdsklen);
  memorySetIOWriteStub(0x7e, wdsksync);
}

void floppyIORegistersClear(void) {
  dskpt = 0;
  dsklen = 0;
  dsksync = 0;
  adcon = 0;
  diskDMAen = 0;
}

void floppyClearDMAState(void) {
  floppy_DMA_started = FALSE;
  floppy_DMA_read = TRUE;
  floppy_DMA.dskpt = 0;
  floppy_DMA.wait = 0;
  floppy_DMA.wordsleft = 0;
  floppy_DMA.wait_for_sync = FALSE;
  floppy_DMA.sync_found = FALSE;
}

BOOLE floppyDMAReadStarted(void) {
  return floppy_DMA_started && floppy_DMA_read;
}

BOOLE floppyDMAWriteStarted(void) {
  return floppy_DMA_started && !floppy_DMA_read;
}

BOOLE floppyDMAChannelOn(void) {
  return (dmaconr & 0x0010) && (dsklen & 0x8000);
}

BOOLE floppyHasIndex(ULO sel_drv) {
  return (floppy[sel_drv].motor_ticks == 0);
}

ULO floppyGetLinearTrack(ULO sel_drv) {
  return floppy[sel_drv].track*2 + floppy[sel_drv].side;
}

BOOLE floppyIsSpinning(ULO sel_drv) {
  return floppy[sel_drv].motor && floppy[sel_drv].enabled && floppy[sel_drv].inserted;
}

/*======================================================================*/
/* Initialize disk DMA reading                                          */
/* I'm having a problem with KS3.1, which will eventually write corrupt */
/* mfm-sectors if there is a gap in the mfm read from the disk.         */
/* I don't understand why this happens, but for now, get around the     */
/* problem by not using gap for disk accesses performed by the KS.      */
/*======================================================================*/

void floppyDMAReadInit(ULO drive) {
  floppy_DMA_started = TRUE;
  floppy_DMA_read = TRUE;
  floppy_DMA.wordsleft = dsklen & 0x3fff;
  floppy_DMA.dskpt = dskpt & 0x1ffffe;
  floppy_DMA.wait_for_sync = (dsksync != 0);
  floppy_DMA.sync_found = FALSE;
  floppy_DMA.dont_use_gap = ((cpuGetPC(pc) & 0xf80000) == 0xf80000);
  if (floppy_DMA.dont_use_gap && (floppy[drive].motor_ticks >= 11968))
    floppy[drive].motor_ticks = 0;
}

/*==========================================*/
/* Process a disk DMA write operation       */
/* Do it all in one piece, and delay irq.   */
/*==========================================*/

ULO floppyFindNextSync(ULO pos, LON length) {
  ULO offset = pos;
  BOOLE was_sync = FALSE;
  BOOLE is_sync = FALSE;
  BOOLE past_sync = FALSE;
  while ((length > 0) && (!past_sync)) {
    was_sync = is_sync;
    is_sync = (memory_chip[offset] == 0x44 && memory_chip[offset + 1] == 0x89);
    past_sync = (was_sync && !is_sync);
    length -= 2;
    offset += 2;
  }
  return offset - pos - ((past_sync) ? 2 : 0);
}

void floppyDMAWriteInit(LON drive) {
  LON length = (dsklen & 0x3fff)*2;
  ULO pos = dskpt & 0x1ffffe;
  ULO track_lin;
  BOOLE is_sync = FALSE, past_sync = FALSE, ended = FALSE;
  if ((drive == -1) || !floppyDMAChannelOn()) ended = TRUE;
  track_lin = floppyGetLinearTrack(drive);
  while (length > 0) {
    ULO next_after_sync_offset = floppyFindNextSync(pos, length);
    length -= next_after_sync_offset;
    pos += next_after_sync_offset;
    if (length > 0) {
      if (floppySectorSave(drive, track_lin, memory_chip + pos)) {
        length -= 1080;
        pos += 1080;
      }
    }
  }
  floppy_DMA_read = FALSE;
  floppy_DMA.wait = (length / (floppy_fast ? FLOPPY_FAST_WORDS : 2)) + FLOPPY_WAIT_INITIAL;
  floppy_DMA_started = TRUE;
}

/*========================================*/
/* Called when the dsklen register        */
/* has been written twice                 */
/* Will initiate a disk DMA read or write */
/*========================================*/

void floppyDMAStart(void) {
  if (dsklen & 0x4000) floppyDMAWriteInit(floppySelectedGet());
  else floppyDMAReadInit(floppySelectedGet());
  diskDMAen = 0;
}

/*========================================================================*/
/* Data was copied when the write started, here we just delay the irq for */
/* some time.                                                             */
/*========================================================================*/

void floppyDMAWrite(void) {
  if (--floppy_DMA.wait == 0) {
    floppy_DMA_started = FALSE;
    wriw(0x8002, 0xdff09c);
  }
}

UWO floppyGetWordUnderHead(ULO sel_drv, ULO track) {
  return ((floppy[sel_drv].trackinfo[track].mfm_data[floppy[sel_drv].motor_ticks] << 8) |
          floppy[sel_drv].trackinfo[track].mfm_data[floppy[sel_drv].motor_ticks + 1]);
}

/* Returns TRUE is sync is seen for the first time */
BOOLE floppyCheckSync(UWO word_under_head) {
  BOOLE word_is_sync = (word_under_head == dsksync);
  BOOLE found_sync = !floppy_has_sync && word_is_sync;
  if (found_sync) wriw(0x9000, 0xdff09c);
  floppy_has_sync = word_is_sync;
  return found_sync;
}

void floppyReadWord(UWO word_under_head, BOOLE found_sync) {
  if (found_sync && (floppy_DMA.wait_for_sync && !floppy_DMA.sync_found))
    floppy_DMA.sync_found = TRUE;
  else if (floppy_DMA.wait_for_sync && floppy_DMA.sync_found)
    floppy_DMA.wait_for_sync = floppy_DMA.sync_found = FALSE;
  if (floppyDMAChannelOn() && !floppy_DMA.wait_for_sync) {
    memory_chip[floppy_DMA.dskpt] = word_under_head >> 8;
    memory_chip[floppy_DMA.dskpt + 1] = word_under_head & 0xff;
    floppy_DMA.dskpt = (floppy_DMA.dskpt + 2) & 0x1ffffe;
    floppy_DMA.wordsleft--;
    if (floppy_DMA.wordsleft == 0) {
      wriw(0x8002, 0xdff09c);
      floppy_DMA_started = FALSE;
    }
  }
}

void floppyNextTick(ULO sel_drv, ULO track) {
  ULO modulo = (floppyDMAReadStarted() && floppy_DMA.dont_use_gap) ? ((11968 < floppy[sel_drv].trackinfo[track].mfm_length) ? 11968 : floppy[sel_drv].trackinfo[track].mfm_length) :
								     floppy[sel_drv].trackinfo[track].mfm_length;
  floppy[sel_drv].motor_ticks = (floppy[sel_drv].motor_ticks + 2) % modulo;
}

void floppyEndOfLineC(void) {
  LON sel_drv = floppySelectedGet();
  if (floppyDMAWriteStarted()) {floppyDMAWrite(); floppy_has_sync = FALSE; return;}
  if (sel_drv == -1) {floppy_has_sync = FALSE; return;}
  if (floppyIsSpinning(sel_drv)) {
    ULO i;
    ULO track = floppyGetLinearTrack(sel_drv);
    ULO words = (floppy_fast) ? FLOPPY_FAST_WORDS : 2;
    for (i = 0; i < words; i++) {
      UWO word_under_head = floppyGetWordUnderHead(sel_drv, track);
      BOOLE found_sync = floppyCheckSync(word_under_head);
      if (floppyHasIndex(sel_drv)) ciaRaiseIndexIRQ();
      if (floppyDMAReadStarted()) floppyReadWord(word_under_head, found_sync);
      floppyNextTick(sel_drv, track);
    }
  }
  else floppy_has_sync = FALSE;
}

/*===========================================================================*/
/* Top level disk-emulation initialization                                   */
/*===========================================================================*/

void floppyHardReset(void) {
  floppyIORegistersClear();
  floppyClearDMAState();
  floppyDriveTableReset();
}

void floppyEmulationStart(void) {
  floppyIOHandlersInstall();
}

void floppyEmulationStop(void) {
}

void floppyStartup(void) {
  floppyIORegistersClear();
  floppyClearDMAState();
  floppyDriveTableInit();
}

void floppyShutdown(void) {
  ULO i;
  for (i = 0; i < 4; i++) if (floppy[i].zipped) floppyImageRemove(i);
  floppyMfmDataFree();
}
