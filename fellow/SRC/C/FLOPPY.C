/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Floppy Emulation                                                           */
/*                                                                            */
/* Author: Petter Schau (peschau@online.no)                                   */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

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

#define MFM_FILLB 0xaa
#define MFM_FILLL 0xaaaaaaaa
#define FLOPPY_INSERTED_DELAY 200   

#define WIN_SHORTPATHNAMES

/*---------------*/
/* Configuration */
/*---------------*/

floppyinfostruct floppy[4];              /* Info about drives */ 
BOOLE floppy_fast;                       /* Select fast floppy transfer */
UBY tmptrack[512*11];                    /* Temporary track buffer */
floppyDMAinfostruct floppy_DMA;          /* Info about a DMA transfer */
BOOLE floppy_DMA_started;                /* Disk DMA started */
BOOLE floppy_DMA_read;                   /* DMA read or write */


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
    if (floppy[i].sel)
      return i;
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

/* This is the only place where we test for enabled drives          */
/* It is sufficient for the OS to decide which drives are present,  */
/* but an unlikely program could detect singals on other lines that */
/* shouldn't be there.                                              */

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
  if (drive != -1)
    return floppy[drive].changed;
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
  else if (!floppy[drive].motor && !mtr)
    floppy[drive].idmode = FALSE;
  if (floppy[drive].motor != (!mtr)) drawSetLED(drive, !mtr);
  floppy[drive].motor = !mtr;
}

void floppySideSet(BOOLE s) {
  ULO i;
  
  for (i = 0; i < 4; i++)
    floppy[i].side = !s;
}
    
void floppyDirSet(BOOLE dr) {
  ULO i;

  for (i = 0; i < 4; i++)
    floppy[i].dir = dr;
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


/*========================================*/
/* Will MFM encode the sector in tmptrack */
/*========================================*/

void floppySectorMFMEncode(ULO tra, ULO sec, UBY *src, UBY *dest, ULO sync) {
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
  odd = tmp & 0x55555555;
  even = (tmp>>1) & 0x55555555;
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


/*==================================*/
/* Decode one MFM sector            */
/* src is pointing after sync words */
/*==================================*/

ULO floppySectorMFMDecode(UBY *src, UBY *dst) {
  ULO sector;
  ULO odd, even, i;

  odd = (src[0]<<24) | (src[1]<<16) | (src[2]<<8) | src[3];
  even = (src[4]<<24) | (src[5]<<16) | (src[6]<<8) | src[7];
  even &= 0x55555555;
  odd  = (odd & 0x55555555) << 1;
  even |= odd;
  sector = (even & 0xff00)>>8;
  src += (48 + 8);
  for (i = 0; i < 512; i++) {
    even = (*(src + i)) & 0x55;
    odd = (*(src + i + 512)) & 0x55;
    *(dst++) = (UBY) ((even<<1) | odd);
  }
  return sector;
}


/*=======================================*/
/* Save one sector to disk and cache     */
/* mfmsrc points to after the sync words */
/*=======================================*/

void floppySectorSave(ULO drive, ULO track, UBY *mfmsrc) {
  ULO sector;
  
  if (!floppy[drive].writeprot) {
    if ((sector = floppySectorMFMDecode(mfmsrc, tmptrack)) < 11) {
      fseek(floppy[drive].F,
	    floppy[drive].trackinfo[track].fileoffset + sector*512, SEEK_SET);
      fwrite(tmptrack, 1, 512, floppy[drive].F);
      if (floppy[drive].trackinfo[track].valid)
	memcpy(floppy[drive].trackinfo[track].buffer + sector*1088, mfmsrc - 8,
	       1088);
    }
  }
}


/*===============================*/
/* Make one track of MFM data    */
/* Track is MFM tracks (0 - 159) */
/*===============================*/

void floppyTrackMFMEncode(ULO track, UBY *src, UBY *dst, ULO sync) {
  ULO i;
  
  for (i = 0; i < 11; i++)
    floppySectorMFMEncode(track, i, src + i*512, dst + i*1088, sync);
}


/*==============================================*/
/* Load track from disk into the track buffer   */
/* Track is MFM tracks (0 - 159)                */
/*==============================================*/

void floppyTrackLoad(ULO drive, ULO track) {
  fseek(floppy[drive].F, track*5632, SEEK_SET);
  fread(tmptrack, 1, 5632, floppy[drive].F);
  floppyTrackMFMEncode(track, tmptrack, floppy[drive].trackinfo[track].buffer,
		       floppy[drive].trackinfo[track].sync);
}


/*=================================*/
/* Insert gzip, bzip2 or DMS image */
/* Unzip the image to tempdir      */
/*=================================*/

static UBY gzbuff[512];
static STR gzname[L_tmpnam];
static STR cmdline[512];
#ifdef WIN_SHORTPATHNAMES
static STR gzshortname[512];
#endif


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

/*=========================*/
/* Uncompress a BZip image */
/*=========================*/

BOOLE floppyImageCompressedBZipPrepare(STR *diskname, ULO drive) {
  FILE *F;

  F = fopen(tmpnam(gzname), "wb");
  if (F == NULL) floppyError(drive, FLOPPY_ERROR_COMPRESS_TMPFILEOPEN);
  else {
    fclose(F);
    sprintf(cmdline, "bzip2.exe -k -d -s -c %s > %s", diskname, gzname);
    system(cmdline);
    strcpy(floppy[drive].imagenamereal, gzname);
    floppy[drive].zipped = TRUE;
  }
  return TRUE;
}


/*========================*/
/* Uncompress a DMS image */
/*========================*/

BOOLE floppyImageCompressedDMSPrepare(STR *diskname, ULO drive) {
#ifdef WIN_SHORTPATHNAMES
  HRESULT res;
  res = GetShortPathName(diskname, gzshortname, 512);
  if(!res) return FALSE;
  sprintf(cmdline, "xdms.exe -q u \"%s\" +\"%s\"", gzshortname, tmpnam(gzname));
#else
  sprintf(cmdline, "xdms.exe -q u \"%s\" +\"%s\"", diskname, tmpnam(gzname));
#endif
  system(cmdline);
  strcpy(floppy[drive].imagenamereal, gzname);
  floppy[drive].zipped = TRUE;
  return TRUE;
}


/*============================*/
/* Uncompress a GZipped image */
/*============================*/

BOOLE floppyImageCompressedGZipPrepare(STR *diskname, ULO drive) {
#ifdef WIN_SHORTPATHNAMES
  HRESULT res;
  res = GetShortPathName(diskname, gzshortname, 512);
  if(!res) return FALSE;
  sprintf(cmdline, "gzip -c -d \"%s\" > \"%s\"", gzshortname, tmpnam(gzname));
#else
  sprintf(cmdline, "gzip -c -d \"%s\" > \"%s\"", diskname, tmpnam(gzname));
#endif
  system(cmdline);
  strcpy(floppy[drive].imagenamereal, gzname);
  floppy[drive].zipped = TRUE;
  return TRUE;
}


/*=================================*/
/* Remove TMP image of zipped file */
/*=================================*/

void floppyImageCompressedRemove(ULO drive) {
  if (floppy[drive].zipped) {
    floppy[drive].zipped = FALSE;
    sprintf(cmdline, "del \"%s\"", floppy[drive].imagenamereal);
    system(cmdline);
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


/*========================*/
/* Top Handling of Images */
/*========================*/

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
    floppy[drive].trackinfo[i].fileoffset = i*5632;
    floppy[drive].trackinfo[i].sync = 0x4489;
    floppy[drive].trackinfo[i].length = 11968;
    if (floppy[drive].cached) {
      floppy[drive].trackinfo[i].buffer = floppy[drive].cache + i*11968; 
      floppy[drive].trackinfo[i].valid = TRUE;
      floppyTrackLoad(drive, i);
    }
    else {
      floppy[drive].trackinfo[i].buffer = floppy[drive].cache;
      floppy[drive].trackinfo[i].valid = FALSE;
    }
  }
  floppy[drive].inserted = TRUE;
  floppy[drive].insertedframe = draw_frame_count;
}


/*========================*/
/* Load extended ADF file */
/*========================*/

void floppyImageExtendedLoad(ULO drive) {
  ULO i, offset;
  UBY tinfo[4];
  
  fseek(floppy[drive].F, SEEK_SET, 8);
  for (i = 0; i < floppy[drive].tracks*2; i++) {
    fread(tinfo, 1, 4, floppy[drive].F);
    floppy[drive].trackinfo[i].sync = (((ULO) tinfo[0])<<8) | tinfo[1];
    floppy[drive].trackinfo[i].length = (((ULO) tinfo[2])<<8) | tinfo[3];
  }
  offset = 160*8 + 8;
  fseek(floppy[drive].F, SEEK_SET, offset);
  for (i = 0; i < floppy[drive].tracks*2; i++) {
    floppy[drive].trackinfo[i].fileoffset = offset;
    if (floppy[drive].cached) {
      fread(tmptrack, 1, floppy[drive].trackinfo[i].length, floppy[drive].F);
      floppy[drive].trackinfo[i].buffer = floppy[drive].cache + offset -
	160*8 - 8;
      floppy[drive].trackinfo[i].valid = TRUE;
      floppyTrackMFMEncode(i, tmptrack, floppy[drive].trackinfo[i].buffer,
			   floppy[drive].trackinfo[i].sync);
    }
    else {
      floppy[drive].trackinfo[i].buffer = floppy[drive].cache;
      floppy[drive].trackinfo[i].valid = FALSE;
    }
    offset += floppy[drive].trackinfo[i].length;
  }      
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


/*==============================================*/
/* Final setup of a disk DMA read operation     */
/* Called when all other parameters are checked */
/* Will fill inn needed information in the  	*/
/* diskDMAinfo structure, and set a flag so   	*/
/* that the end of line handler will move       */
/* words into the Amiga memory space until	    */
/* the number of words requested are transferred*/
/*==============================================*/

void floppyDMAReadInit(ULO drive) {
  ULO track;
  
  track = floppy[drive].track*2 + floppy[drive].side;
  floppy_DMA.drive = drive;
  floppy_DMA.wordsleft = dsklen & 0x3fff;
  floppy_DMA.wait = FLOPPY_WAIT_INITIAL; 
  floppy_DMA_read = TRUE;
  floppy_DMA.src = floppy[drive].trackinfo[track].buffer;
  floppy_DMA.dst = memory_chip + (dskpt & 0x1fffff);
  floppy_DMA.tracklength = floppy[drive].trackinfo[track].length;
  if (!floppy[drive].trackinfo[track].valid)
    floppyTrackLoad(drive, track);
  floppy_DMA.offset = 1088*(floppy[drive].lasttrackcount++ % 11);
  if (dsksync == floppy[drive].trackinfo[track].sync)
    floppy_DMA.offset += 6;
  floppy_DMA_started = TRUE;  /* Set flag used by the end of line handler */
}



/*==========================================*/
/* Process a disk DMA write operation       */
/* Do it all in one piece, and delay irq    */
/* a bit.				    */
/* If anyone is doing tricks during a write */
/* they deserve problems.....               */
/*==========================================*/

void floppyDMAWriteInit(ULO drive) {
  ULO length = (dsklen & 0x3fff)*2;
  ULO pos = dskpt & 0x1ffffe;
  ULO i = 0;
  BOOLE syncfound = FALSE, ended = FALSE;

  while (!ended) {

    /* Search for sync */

    while (!syncfound && !ended) {
      if (memory_chip[pos + i] == 0x44 && memory_chip[pos + i + 1] == 0x89)
	syncfound = TRUE;
      i += 2;
      if (i >= length)
	ended = TRUE;
    }
    if (!ended) {
      if (memory_chip[pos + i] == 0x44 && memory_chip[pos + i + 1] == 0x89) {
	i += 2;
	if (i >= length)
	  ended = TRUE;
      }
    }

    /* Here i either points to first word after a sync, or beyond length */

    if (!ended) {

      /* If less words left than a full sector in transfer, ignore sector */

      if ((length - i) >= 1080) {

	/* Now we have a full sector to decode */
	/* and i points to first byte after sync */

	floppySectorSave(drive, floppy[drive].track*2 + floppy[drive].side,
			 memory_chip + pos + i);
			
	/* Update pointer to after sector */

	i += 1080;
	syncfound = FALSE;
      }
      else
	ended = TRUE;
    }
  }
  floppy_DMA_read = FALSE;
  floppy_DMA.wait = (length / (floppy_fast ? 4096 : 2)) + FLOPPY_WAIT_INITIAL;
  floppy_DMA_started = TRUE;
}


/*========================================*/
/* Called by the end of line handler      */
/* every line until the number of words   */
/* in the disk DMA transfer has been      */
/* copied into the Amiga memory space     */
/*========================================*/

void floppyDMARead(void) {
  ULO wcount;

  /* Initial wait to give programs some time to set up things */

  if (floppy_DMA.wait != 0) floppy_DMA.wait--;
  else {
    wcount = floppy_fast ? 4096 : 2;          /* Number of words to transfer */
    while ((wcount != 0) && (floppy_DMA.wordsleft != 0)) {
      *(floppy_DMA.dst++) = *(floppy_DMA.src +
		          (floppy_DMA.offset++ % floppy_DMA.tracklength));
      *(floppy_DMA.dst++) = *(floppy_DMA.src +
			      (floppy_DMA.offset++ % floppy_DMA.tracklength));
      floppy_DMA.wordsleft--;
      wcount--;
    }  
    if (floppy_DMA.wordsleft == 0) {  /* Raise irq when finished */
      wriw(0x8002, 0xdff09c);
      floppy_DMA_started = FALSE;
    }
  }
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


/*========================================*/
/* Called when the dsklen register        */
/* has been written twice                 */
/* Will initiate a disk DMA read or write */
/*========================================*/

void floppyDMAStart(void) {
  ULO drive;

  /* BUG? Some programs read from disk with the master DMA bit off? */
  /* Why?  Test dmaconr instead to ignore the master bit */

  /* Later: This has been a question since the early days of Fellow. */
  /* Now in its eve, I decide to leave this as it is simply beacuse */
  /* it works best this way..... PS  */

  if (dmaconr & 0x0010) {          /* DMA channel must be enabled */
    if (dsklen & 0x8000) {	/* Disk DMA must be on */
      drive = 0;     /* Use the first selected drive if several are selected */
      while (!floppy[drive].sel && drive < 4)
	drive++;
      if (floppy[drive].sel && floppy[drive].inserted) {
	if (TRUE/*floppy[drive].motor*/) {	    /* The motor must be on */
	  if (TRUE /*adcon & 0x1000*/) {    /* Must be in MFM mode */
	    if (TRUE /*adcon & 0x100*/) {   /* Must be in FAST MFM mode */
	      if (dsklen & 0x4000)
		floppyDMAWriteInit(drive);
	      else
		floppyDMAReadInit(drive);
	    }
	  }
	}
      }
    }
  }
  diskDMAen = 0;
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
    floppy[i].side = FALSE;
    floppy[i].insertedframe = draw_frame_count + FLOPPY_INSERTED_DELAY + 1;
    floppy[i].idmode = FALSE;
    floppy[i].idcount = 0;
  }
}


/*=========================================*/
/* Try to allcate space                    */
/* for full-image cache                    */
/* Leave some room for other mallocs later */
/*=========================================*/

void floppyCacheAllocate(void) {
  ULO i, j;

  for (i = 0; i < 4; i++) {
    floppy[i].cache = NULL;
    floppy[i].cache = (UBY *) malloc(FLOPPY_TRACKS*11968);
    floppy[i].cached = (floppy[i].cache != NULL);
    if (!floppy[i].cached)
      floppy[i].cache = (UBY *) malloc(11968);
    for (j = 0; j < FLOPPY_TRACKS; j++) {
      floppy[i].trackinfo[j].buffer = floppy[i].cache;
      if (floppy[i].cached)
	floppy[i].trackinfo[j].buffer += j*11968;
    }
  }
}


/*==================*/
/* Free memory used */
/*==================*/

void floppyCacheFree(void) {
  ULO i;

  for (i = 0; i < 4; i++)
    if (floppy[i].cache != NULL)
      free(floppy[i].cache);
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


/*===========================================================================*/
/* Clear running state of this module                                        */
/*===========================================================================*/

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
  floppy_DMA.drive = 0;
  floppy_DMA.dst = 0;
  floppy_DMA.offset = 0;
  floppy_DMA.src = 0;
  floppy_DMA.tracklength = 0;
  floppy_DMA.wait = 0;
  floppy_DMA.wordsleft = 0;
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
  floppyCacheAllocate();
}

void floppyShutdown(void) {
  ULO i;
  
  for (i = 0; i < 4; i++)
    if (floppy[i].zipped)
      floppyImageRemove(i);
  floppyCacheFree();
}

