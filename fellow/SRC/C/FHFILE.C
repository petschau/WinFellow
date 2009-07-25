/* @(#) $Id: FHFILE.C,v 1.11 2009-07-25 03:09:00 peschau Exp $ */
/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* Hardfile device                                                         */
/*                                                                         */
/* Authors: Petter Schau                                                   */
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

#include "defs.h"
#include "fellow.h"
#include "fhfile.h"
#include "fmem.h"
#include "draw.h"
#include "CpuModule.h"
#include "fswrap.h"


/*====================*/
/* fhfile.device      */
/*====================*/


/*===================================*/
/* Fixed locations in fhfile_rom:    */
/* ------------------------------    */
/* offset 4088 - max number of units */
/* offset 4092 - configdev pointer   */
/*===================================*/


/*===================================================================*/
/* The hardfile device is at least in spirit based on ideas found in */
/* the sources below:                                                */
/* - The device driver source found on Fish 39                       */
/*   and other early Fish-disks                                      */
/* - UAE was useful for test and compare when the device was written */
/*   and didn't work                                                 */
/*===================================================================*/


fhfile_dev fhfile_devs[FHFILE_MAX_DEVICES];
ULO fhfile_romstart;
ULO fhfile_bootcode;
ULO fhfile_configdev;
UBY fhfile_rom[65536];



/*============================================================================*/
/* Configuration properties                                                   */
/*============================================================================*/

BOOLE fhfile_enabled;

static BOOLE fhfileHasZeroDevices(void) {
  ULO i;
  ULO dev_count = 0;

  for (i = 0; i < FHFILE_MAX_DEVICES; i++) if (fhfile_devs[i].F != NULL) dev_count++;
  return (dev_count == 0);
}

static void fhfileInitializeHardfile(ULO index) {
  ULO size;
  fs_navig_point *fsnp;

  if (fhfile_devs[index].F != NULL)                     /* Close old hardfile */
    fclose(fhfile_devs[index].F);
  fhfile_devs[index].F = NULL;                           /* Set config values */
  fhfile_devs[index].status = FHFILE_NONE;
  if ((fsnp = fsWrapMakePoint(fhfile_devs[index].filename)) != NULL) {
    fhfile_devs[index].readonly |= (!fsnp->writeable);
    size = fsnp->size;
    fhfile_devs[index].F = fopen(fhfile_devs[index].filename,
      (fhfile_devs[index].readonly) ? "rb" : "r+b");
    if (fhfile_devs[index].F != NULL) {                          /* Open file */
      ULO track_size = (fhfile_devs[index].sectorspertrack *
	fhfile_devs[index].surfaces *
	fhfile_devs[index].bytespersector);
      if (size < track_size) {
	/* Error: File must be at least one track long */
	fclose(fhfile_devs[index].F);
	fhfile_devs[index].F = NULL;
	fhfile_devs[index].status = FHFILE_NONE;
      }
      else {                                                    /* File is OK */
	fhfile_devs[index].tracks = size / track_size;
	fhfile_devs[index].size = fhfile_devs[index].tracks * track_size;
	fhfile_devs[index].status = FHFILE_HDF;
      }
    }
    free(fsnp);
  }
}

/* Returns TRUE if a hardfile was inserted */

BOOLE fhfileRemoveHardfile(ULO index) {
  BOOLE result = FALSE;
  if (index >= FHFILE_MAX_DEVICES) return result;
  if (fhfile_devs[index].F != NULL) {
    fflush(fhfile_devs[index].F);
    fclose(fhfile_devs[index].F);
    result = TRUE;
  }
  memset(&(fhfile_devs[index]), 0, sizeof(fhfile_dev));
  fhfile_devs[index].status = FHFILE_NONE;
  return result;
}


void fhfileSetEnabled(BOOLE enabled) {
  fhfile_enabled = enabled;
}


BOOLE fhfileGetEnabled(void) {
  return fhfile_enabled;
}


void fhfileSetHardfile(fhfile_dev hardfile, ULO index) {
  if (index >= FHFILE_MAX_DEVICES) return;
  fhfileRemoveHardfile(index);
  strncpy(fhfile_devs[index].filename, hardfile.filename, CFG_FILENAME_LENGTH);
  fhfile_devs[index].readonly = hardfile.readonly_original;
  fhfile_devs[index].readonly_original = hardfile.readonly_original;
  fhfile_devs[index].bytespersector = (hardfile.bytespersector_original & 0xfffffffc);
  fhfile_devs[index].bytespersector_original = hardfile.bytespersector_original;
  fhfile_devs[index].sectorspertrack = hardfile.sectorspertrack;
  fhfile_devs[index].surfaces = hardfile.surfaces;
  fhfile_devs[index].reservedblocks = hardfile.reservedblocks_original;
  fhfile_devs[index].reservedblocks_original = hardfile.reservedblocks_original;
  if (fhfile_devs[index].reservedblocks < 1)
    fhfile_devs[index].reservedblocks = 1;
  fhfileInitializeHardfile(index);
}


BOOLE fhfileCompareHardfile(fhfile_dev hardfile, ULO index) {
  if (index >= FHFILE_MAX_DEVICES) return FALSE;
  return (fhfile_devs[index].readonly_original == hardfile.readonly_original) &&
    (fhfile_devs[index].bytespersector_original == hardfile.bytespersector_original) &&
    (fhfile_devs[index].sectorspertrack == hardfile.sectorspertrack) &&
    (fhfile_devs[index].surfaces == hardfile.surfaces) &&
    (fhfile_devs[index].reservedblocks_original == hardfile.reservedblocks_original) &&
    (strncmp(fhfile_devs[index].filename, hardfile.filename, CFG_FILENAME_LENGTH) == 0);
}


void fhfileClear(void) {
  ULO i;

  for (i = 0; i < FHFILE_MAX_DEVICES; i++) fhfileRemoveHardfile(i);
}


/*===================*/
/* Set HD led symbol */
/*===================*/

static void fhfileSetLed(BOOLE state) {
  drawSetLED(4, state);
}


/*==================*/
/* BeginIO Commands */
/*==================*/

static void fhfileIgnore(ULO index) {
  memoryWriteLong(0, cpuGetAReg(1) + 32);
  cpuSetDReg(0, 0);
}

static BYT fhfileRead(ULO index) {
  ULO dest = memoryReadLong(cpuGetAReg(1) + 40);
  ULO offset = memoryReadLong(cpuGetAReg(1) + 44);
  ULO length = memoryReadLong(cpuGetAReg(1) + 36);

  if ((offset + length) > fhfile_devs[index].size)
    return -3;
  fhfileSetLed(TRUE);
  fseek(fhfile_devs[index].F, offset, SEEK_SET);
  fread(memoryAddressToPtr(dest), 1, length, fhfile_devs[index].F);
  memoryWriteLong(length, cpuGetAReg(1) + 32);
  fhfileSetLed(FALSE);
  return 0;
}

static BYT fhfileWrite(ULO index) {
  ULO dest = memoryReadLong(cpuGetAReg(1) + 40);
  ULO offset = memoryReadLong(cpuGetAReg(1) + 44);
  ULO length = memoryReadLong(cpuGetAReg(1) + 36);

  if (fhfile_devs[index].readonly ||
    ((offset + length) > fhfile_devs[index].size))
    return -3;
  fhfileSetLed(TRUE);
  fseek(fhfile_devs[index].F, offset, SEEK_SET);
  fwrite(memoryAddressToPtr(dest),1, length, fhfile_devs[index].F);
  memoryWriteLong(length, cpuGetAReg(1) + 32);
  fhfileSetLed(FALSE);
  return 0;
}

static void fhfileGetNumberOfTracks(ULO index) {
  memoryWriteLong(fhfile_devs[index].tracks, cpuGetAReg(1) + 32);
}

static void fhfileGetDriveType(ULO index) {
  memoryWriteLong(1, cpuGetAReg(1) + 32);
}

static void fhfileWriteProt(ULO index) {
  memoryWriteLong(fhfile_devs[index].readonly, cpuGetAReg(1) + 32);
}



/*======================================================*/
/* fhfileDiag native callback                           */
/*                                                      */
/* Pointer to our configdev struct is stored in $f40ffc */
/* For later use when filling out the bootnode          */
/*======================================================*/

void fhfileDiag(void)
{
  fhfile_configdev = cpuGetAReg(3);
  memoryDmemSetLongNoCounter(FHFILE_MAX_DEVICES, 4088);
  memoryDmemSetLongNoCounter(fhfile_configdev, 4092);
  cpuSetDReg(0, 1);
}


/*======================================*/
/* Native callbacks for device commands */
/*======================================*/

static void fhfileOpen(void) {
  if (cpuGetDReg(0) < FHFILE_MAX_DEVICES) {
    memoryWriteByte(7, cpuGetAReg(1) + 8);                     /* ln_type (NT_REPLYMSG) */
    memoryWriteByte(0, cpuGetAReg(1) + 31);                    /* io_error */
    memoryWriteLong(cpuGetDReg(0), cpuGetAReg(1) + 24);                 /* io_unit */
    memoryWriteLong(memoryReadLong(cpuGetAReg(6) + 32) + 1, cpuGetAReg(6) + 32);  /* LIB_OPENCNT */
    cpuSetDReg(0, 0);                              /* ? */
  }
  else {
    memoryWriteLong(-1, cpuGetAReg(1) + 20);            
    memoryWriteByte(-1, cpuGetAReg(1) + 31);                   /* io_error */
    cpuSetDReg(0, -1);                             /* ? */
  }
}

static void fhfileClose(void) {
  memoryWriteLong(memoryReadLong(cpuGetAReg(6) + 32) - 1, cpuGetAReg(6) + 32);    /* LIB_OPENCNT */
  cpuSetDReg(0, 0);                                /* ? */
}

static void fhfileExpunge(void) {
  cpuSetDReg(0, 0);                                /* ? */
}

static void fhfileNULL(void) {}

static void fhfileBeginIO(void) {
  BYT error = 0;
  ULO unit = memoryReadLong(cpuGetAReg(1) + 24);

  switch (memoryReadWord(cpuGetAReg(1) + 28)) {
    case 2:
      error = fhfileRead(unit);
      break;
    case 3:
    case 11:
      error = fhfileWrite(unit);
      break;
    case 18:
      fhfileGetDriveType(unit);
      break;
    case 19:
      fhfileGetNumberOfTracks(unit);
      break;
    case 15:
      fhfileWriteProt(unit);
      break;
    case 4:
    case 5:
    case 9:
    case 10:
    case 12:
    case 13:
    case 14:
    case 20:
    case 21:
      fhfileIgnore(unit);
      break;
    default:
      error = -3;
      cpuSetDReg(0, 0);
      break;
  }
  memoryWriteByte(5, cpuGetAReg(1) + 8);      /* ln_type */
  memoryWriteByte(error, cpuGetAReg(1) + 31); /* ln_error */
}


static void fhfileAbortIO(void) {
  cpuSetDReg(0, -3);
}


/*=================================================*/
/* fhfile_do                                       */
/* The M68000 stubs entered in the device tables   */
/* write a longword to $f40000, which is forwarded */
/* by the memory system to this procedure.         */
/* Hardfile commands are issued by 0x0001XXXX      */
/*=================================================*/

void fhfileDo(ULO data) {
  switch (data & 0xffff) {
    case 1:
      fhfileDiag();
      break;
    case 2:
      fhfileOpen();
      break;
    case 3:
      fhfileClose();
      break;
    case 4:
      fhfileExpunge();
      break;
    case 5:
      fhfileNULL();
      break;
    case 6:
      fhfileBeginIO();
      break;
    case 7:
      fhfileAbortIO();
      break;
    default:
      break;
  }
}


/*===========================================================================*/
/* Hardfile card init                                                        */
/*===========================================================================*/

/*================================================================*/
/* fhfile_card_init                                               */
/* We want a configDev struct.  AmigaDOS won't give us one unless */
/* we pretend to be an expansion card.                            */
/*================================================================*/

void fhfileCardInit(void) {
  memoryEmemSet(0, 0xd1);
  memoryEmemSet(8, 0xc0);
  memoryEmemSet(4, 2);
  memoryEmemSet(0x10, 2011>>8);
  memoryEmemSet(0x14, 2011 & 0xf);
  memoryEmemSet(0x18, 0);
  memoryEmemSet(0x1c, 0);
  memoryEmemSet(0x20, 0);
  memoryEmemSet(0x24, 1);
  memoryEmemSet(0x28, 0x10);
  memoryEmemSet(0x2c, 0);
  memoryEmemSet(0x40, 0);
  memoryEmemMirror(0x1000, fhfile_rom + 0x1000, 0xa0);
}

/*============================================================================*/
/* Functions to get and set data in the fhfile memory area                    */
/*============================================================================*/

UBY fhfileReadByte(ULO address)
{
  return fhfile_rom[address & 0xffff];
}

UWO fhfileReadWord(ULO address)
{
  UBY *p = fhfile_rom + (address & 0xffff);
  return (((UWO)p[0]) << 8) | ((UWO)p[1]);
}

ULO fhfileReadLong(ULO address)
{
  UBY *p = fhfile_rom + (address & 0xffff);
  return (((ULO)p[0]) << 24) | (((ULO)p[1]) << 16) | (((ULO)p[2]) << 8) | ((ULO)p[3]);
}

void fhfileWriteByte(UBY data, ULO address)
{
  // NOP
}

void fhfileWriteWord(UWO data, ULO address)
{
  // NOP
}

void fhfileWriteLong(ULO data, ULO address)
{
  // NOP
}

/*====================================================*/
/* fhfile_card_map                                    */
/* Our rom must be remapped to the location specified */
/* by Amiga OS                                        */
/*====================================================*/

void fhfileCardMap(ULO mapping) {
  ULO bank;

  fhfile_romstart = (mapping<<8) & 0xff0000;
  bank = fhfile_romstart>>16;
  memoryBankSet(fhfileReadByte,
    fhfileReadWord,
    fhfileReadLong,
    fhfileWriteByte,
    fhfileWriteWord,
    fhfileWriteLong,
    fhfile_rom,
    bank,
    bank);
}


/*=================================================*/
/* Make a dosdevice packet about the device layout */
/*=================================================*/

static void fhfileMakeDOSDevPacket(ULO devno, ULO unitnameptr, ULO devnameptr){
  if (fhfile_devs[devno].F) {
    memoryDmemSetLong(devno);				     /* Flag to initcode */
    memoryDmemSetLong(unitnameptr);			     /*  0 Device driver name "FELLOWX" */
    memoryDmemSetLong(devnameptr);			     /*  4 Device name "fhfile.device" */
    memoryDmemSetLong(devno);				     /*  8 Unit # */
    memoryDmemSetLong(0);				     /* 12 OpenDevice flags */
    memoryDmemSetLong(16);                                   /* 16 Environment size */
    memoryDmemSetLong(fhfile_devs[devno].bytespersector>>2); /* 20 Longwords in a block */
    memoryDmemSetLong(0);				     /* 24 sector origin (unused) */
    memoryDmemSetLong(fhfile_devs[devno].surfaces);	     /* 28 Heads */
    memoryDmemSetLong(1);				     /* 32 Sectors per logical block (unused) */
    memoryDmemSetLong(fhfile_devs[devno].sectorspertrack);   /* 36 Sectors per track */
    memoryDmemSetLong(fhfile_devs[devno].reservedblocks);    /* 40 Reserved blocks, min. 1 */
    memoryDmemSetLong(0);				     /* 44 mdn_prefac - Unused */
    memoryDmemSetLong(0);				     /* 48 Interleave */
    memoryDmemSetLong(0);				     /* 52 Lower cylinder */
    memoryDmemSetLong(fhfile_devs[devno].tracks - 1);	     /* 56 Upper cylinder */
    memoryDmemSetLong(0);				     /* 60 Number of buffers */
    memoryDmemSetLong(0);				     /* 64 Type of memory for buffers */
    memoryDmemSetLong(0x7fffffff);			     /* 68 Largest transfer */
    memoryDmemSetLong(~1U);				     /* 72 Add mask */
    memoryDmemSetLong(-1);				     /* 76 Boot priority */
    memoryDmemSetLong(0x444f5300);			     /* 80 DOS file handler name */
    memoryDmemSetLong(0);
  }
  if (devno == (FHFILE_MAX_DEVICES - 1))
    memoryDmemSetLong(-1);
}


/*===========================================================*/
/* fhfileHardReset                                           */
/* This will set up the device structures and stubs          */
/* Can be called at every reset, but really only needed once */
/*===========================================================*/

void fhfileHardReset(void) {
  ULO devicename, idstr;
  ULO romtagstart;
  ULO initstruct, functable, datatable;
  ULO fhfile_t_init = 0;
  ULO fhfile_t_open = 0;
  ULO fhfile_t_close = 0;
  ULO fhfile_t_expunge = 0;
  ULO fhfile_t_null = 0;
  ULO fhfile_t_beginio = 0;
  ULO fhfile_t_abortio = 0;
  ULO unitnames[FHFILE_MAX_DEVICES];
  ULO doslibname;
  STR tmpunitname[32];
  ULO i;

  if ((!fhfileHasZeroDevices()) &&
    fhfileGetEnabled() && 
    (memoryGetKickImageVersion() >= 36)) {
      memoryDmemSetCounter(0);

      /* Device-name and ID string */

      devicename = memoryDmemGetCounter();
      memoryDmemSetString("fhfile.device");
      idstr = memoryDmemGetCounter();
      memoryDmemSetString("Fellow Hardfile device V3");

      /* Device name as seen in Amiga DOS */

      for (i = 0; i < FHFILE_MAX_DEVICES; i++) {
	unitnames[i] = memoryDmemGetCounter();
	sprintf(tmpunitname, "FELLOW%d", i);
	memoryDmemSetString(tmpunitname);
      }

      /* dos.library name */

      doslibname = memoryDmemGetCounter();
      memoryDmemSetString("dos.library");

      /* fhfile.open */

      fhfile_t_open = memoryDmemGetCounter();
      memoryDmemSetWord(0x23fc);
      memoryDmemSetLong(0x00010002); memoryDmemSetLong(0xf40000); /* move.l #$00010002,$f40000 */
      memoryDmemSetWord(0x4e75);					/* rts */

      /* fhfile.close */

      fhfile_t_close = memoryDmemGetCounter();
      memoryDmemSetWord(0x23fc);
      memoryDmemSetLong(0x00010003); memoryDmemSetLong(0xf40000); /* move.l #$00010003,$f40000 */
      memoryDmemSetWord(0x4e75);					/* rts */

      /* fhfile.expunge */

      fhfile_t_expunge = memoryDmemGetCounter();
      memoryDmemSetWord(0x23fc);
      memoryDmemSetLong(0x00010004); memoryDmemSetLong(0xf40000); /* move.l #$00010004,$f40000 */
      memoryDmemSetWord(0x4e75);					/* rts */

      /* fhfile.null */

      fhfile_t_null = memoryDmemGetCounter();
      memoryDmemSetWord(0x23fc);
      memoryDmemSetLong(0x00010005); memoryDmemSetLong(0xf40000); /* move.l #$00010005,$f40000 */
      memoryDmemSetWord(0x4e75);                                  /* rts */

      /* fhfile.beginio */

      fhfile_t_beginio = memoryDmemGetCounter();
      memoryDmemSetWord(0x23fc);
      memoryDmemSetLong(0x00010006); memoryDmemSetLong(0xf40000); /* move.l #$00010006,$f40000 */
      memoryDmemSetLong(0x48e78002);				/* movem.l d0/a6,-(a7) */
      memoryDmemSetLong(0x08290000); memoryDmemSetWord(0x001e);   /* btst   #$0,30(a1)   */
      memoryDmemSetWord(0x6608);					/* bne    (to rts)     */
      memoryDmemSetLong(0x2c780004);				/* move.l $4.w,a6      */
      memoryDmemSetLong(0x4eaefe86);				/* jsr    -378(a6)     */
      memoryDmemSetLong(0x4cdf4001);				/* movem.l (a7)+,d0/a6 */
      memoryDmemSetWord(0x4e75);					/* rts */

      /* fhfile.abortio */

      fhfile_t_abortio = memoryDmemGetCounter();
      memoryDmemSetWord(0x23fc);
      memoryDmemSetLong(0x00010007); memoryDmemSetLong(0xf40000); /* move.l #$00010007,$f40000 */
      memoryDmemSetWord(0x4e75);					/* rts */

      /* Func-table */

      functable = memoryDmemGetCounter();
      memoryDmemSetLong(fhfile_t_open);
      memoryDmemSetLong(fhfile_t_close);
      memoryDmemSetLong(fhfile_t_expunge);
      memoryDmemSetLong(fhfile_t_null);
      memoryDmemSetLong(fhfile_t_beginio);
      memoryDmemSetLong(fhfile_t_abortio);
      memoryDmemSetLong(0xffffffff);

      /* Data-table */

      datatable = memoryDmemGetCounter();
      memoryDmemSetWord(0xE000);          /* INITBYTE */
      memoryDmemSetWord(0x0008);          /* LN_TYPE */
      memoryDmemSetWord(0x0300);          /* NT_DEVICE */
      memoryDmemSetWord(0xC000);          /* INITLONG */
      memoryDmemSetWord(0x000A);          /* LN_NAME */
      memoryDmemSetLong(devicename);
      memoryDmemSetWord(0xE000);          /* INITBYTE */
      memoryDmemSetWord(0x000E);          /* LIB_FLAGS */
      memoryDmemSetWord(0x0600);          /* LIBF_SUMUSED+LIBF_CHANGED */
      memoryDmemSetWord(0xD000);          /* INITWORD */
      memoryDmemSetWord(0x0014);          /* LIB_VERSION */
      memoryDmemSetWord(0x0002);
      memoryDmemSetWord(0xD000);          /* INITWORD */
      memoryDmemSetWord(0x0016);          /* LIB_REVISION */
      memoryDmemSetWord(0x0000);
      memoryDmemSetWord(0xC000);          /* INITLONG */
      memoryDmemSetWord(0x0018);          /* LIB_IDSTRING */
      memoryDmemSetLong(idstr);
      memoryDmemSetLong(0);               /* END */

      /* bootcode */

      fhfile_bootcode = memoryDmemGetCounter();
      memoryDmemSetWord(0x227c); memoryDmemSetLong(doslibname); /* move.l #doslibname,a1 */
      memoryDmemSetLong(0x4eaeffa0);			      /* jsr    -96(a6) */
      memoryDmemSetWord(0x2040);				      /* move.l d0,a0 */
      memoryDmemSetLong(0x20280016);			      /* move.l 22(a0),d0 */
      memoryDmemSetWord(0x2040);				      /* move.l d0,a0 */
      memoryDmemSetWord(0x4e90);				      /* jsr    (a0) */
      memoryDmemSetWord(0x4e75);				      /* rts */

      /* fhfile.init */

      fhfile_t_init = memoryDmemGetCounter();

      memoryDmemSetByte(0x48); memoryDmemSetByte(0xE7);
      memoryDmemSetByte(0xFF); memoryDmemSetByte(0xFE);
      memoryDmemSetByte(0x2C); memoryDmemSetByte(0x78);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0x04);
      memoryDmemSetByte(0x43); memoryDmemSetByte(0xFA);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0xA6);
      memoryDmemSetByte(0x4E); memoryDmemSetByte(0xAE);
      memoryDmemSetByte(0xFE); memoryDmemSetByte(0x68);
      memoryDmemSetByte(0x28); memoryDmemSetByte(0x40);
      memoryDmemSetByte(0x41); memoryDmemSetByte(0xFA);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0xAE);
      memoryDmemSetByte(0x2E); memoryDmemSetByte(0x08);
      memoryDmemSetByte(0x20); memoryDmemSetByte(0x47);
      memoryDmemSetByte(0x4A); memoryDmemSetByte(0x90);
      memoryDmemSetByte(0x6B); memoryDmemSetByte(0x00);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0x82);
      memoryDmemSetByte(0x58); memoryDmemSetByte(0x87);
      memoryDmemSetByte(0x20); memoryDmemSetByte(0x3C);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0x00);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0x58);
      memoryDmemSetByte(0x72); memoryDmemSetByte(0x01);
      memoryDmemSetByte(0x2C); memoryDmemSetByte(0x78);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0x04);
      memoryDmemSetByte(0x4E); memoryDmemSetByte(0xAE);
      memoryDmemSetByte(0xFF); memoryDmemSetByte(0x3A);
      memoryDmemSetByte(0x2A); memoryDmemSetByte(0x40);
      memoryDmemSetByte(0x20); memoryDmemSetByte(0x47);
      memoryDmemSetByte(0x70); memoryDmemSetByte(0x54);
      memoryDmemSetByte(0x2B); memoryDmemSetByte(0xB0);
      memoryDmemSetByte(0x08); memoryDmemSetByte(0x00);
      memoryDmemSetByte(0x08); memoryDmemSetByte(0x00);
      memoryDmemSetByte(0x59); memoryDmemSetByte(0x80);
      memoryDmemSetByte(0x64); memoryDmemSetByte(0xF6);
      memoryDmemSetByte(0x20); memoryDmemSetByte(0x4D);
      memoryDmemSetByte(0x4E); memoryDmemSetByte(0xAC);
      memoryDmemSetByte(0xFF); memoryDmemSetByte(0x70);
      memoryDmemSetByte(0x26); memoryDmemSetByte(0x40);
      memoryDmemSetByte(0x70); memoryDmemSetByte(0x00);
      memoryDmemSetByte(0x27); memoryDmemSetByte(0x40);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0x08);
      memoryDmemSetByte(0x27); memoryDmemSetByte(0x40);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0x10);
      memoryDmemSetByte(0x27); memoryDmemSetByte(0x40);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0x20);
      memoryDmemSetByte(0x2C); memoryDmemSetByte(0x78);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0x04);
      memoryDmemSetByte(0x70); memoryDmemSetByte(0x14);
      memoryDmemSetByte(0x72); memoryDmemSetByte(0x00);
      memoryDmemSetByte(0x4E); memoryDmemSetByte(0xAE);
      memoryDmemSetByte(0xFF); memoryDmemSetByte(0x3A);
      memoryDmemSetByte(0x22); memoryDmemSetByte(0x47);
      memoryDmemSetByte(0x2C); memoryDmemSetByte(0x29);
      memoryDmemSetByte(0xFF); memoryDmemSetByte(0xFC);
      memoryDmemSetByte(0x22); memoryDmemSetByte(0x40);
      memoryDmemSetByte(0x70); memoryDmemSetByte(0x00);
      memoryDmemSetByte(0x22); memoryDmemSetByte(0x80);
      memoryDmemSetByte(0x23); memoryDmemSetByte(0x40);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0x04);
      memoryDmemSetByte(0x33); memoryDmemSetByte(0x40);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0x0E);
      memoryDmemSetByte(0x33); memoryDmemSetByte(0x7C);
      memoryDmemSetByte(0x10); memoryDmemSetByte(0xFF);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0x08);
      memoryDmemSetByte(0x9D); memoryDmemSetByte(0x69);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0x08);
      memoryDmemSetByte(0x23); memoryDmemSetByte(0x79);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0xF4);
      memoryDmemSetByte(0x0F); memoryDmemSetByte(0xFC);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0x0A);
      memoryDmemSetByte(0x23); memoryDmemSetByte(0x4B);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0x10);
      memoryDmemSetByte(0x41); memoryDmemSetByte(0xEC);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0x4A);
      memoryDmemSetByte(0x4E); memoryDmemSetByte(0xAE);
      memoryDmemSetByte(0xFE); memoryDmemSetByte(0xF2);
      memoryDmemSetByte(0x06); memoryDmemSetByte(0x87);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0x00);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0x58);
      memoryDmemSetByte(0x60); memoryDmemSetByte(0x00);
      memoryDmemSetByte(0xFF); memoryDmemSetByte(0x7A);
      memoryDmemSetByte(0x2C); memoryDmemSetByte(0x78);
      memoryDmemSetByte(0x00); memoryDmemSetByte(0x04);
      memoryDmemSetByte(0x22); memoryDmemSetByte(0x4C);
      memoryDmemSetByte(0x4E); memoryDmemSetByte(0xAE);
      memoryDmemSetByte(0xFE); memoryDmemSetByte(0x62);
      memoryDmemSetByte(0x4C); memoryDmemSetByte(0xDF);
      memoryDmemSetByte(0x7F); memoryDmemSetByte(0xFF);
      memoryDmemSetByte(0x4E); memoryDmemSetByte(0x75);
      memoryDmemSetByte(0x65); memoryDmemSetByte(0x78);
      memoryDmemSetByte(0x70); memoryDmemSetByte(0x61);
      memoryDmemSetByte(0x6E); memoryDmemSetByte(0x73);
      memoryDmemSetByte(0x69); memoryDmemSetByte(0x6F);
      memoryDmemSetByte(0x6E); memoryDmemSetByte(0x2E);
      memoryDmemSetByte(0x6C); memoryDmemSetByte(0x69);
      memoryDmemSetByte(0x62); memoryDmemSetByte(0x72);
      memoryDmemSetByte(0x61); memoryDmemSetByte(0x72);
      memoryDmemSetByte(0x79); memoryDmemSetByte(0x00);



      /* The mkdosdev packets */

      for (i = 0; i < FHFILE_MAX_DEVICES; i++)
	fhfileMakeDOSDevPacket(i, unitnames[i], devicename);

      /* Init-struct */

      initstruct = memoryDmemGetCounter();
      memoryDmemSetLong(0x100);                   /* Data-space size, min LIB_SIZE */
      memoryDmemSetLong(functable);               /* Function-table */
      memoryDmemSetLong(datatable);               /* Data-table */
      memoryDmemSetLong(fhfile_t_init);           /* Init-routine */

      /* RomTag structure */

      romtagstart = memoryDmemGetCounter();
      memoryDmemSetWord(0x4afc);                  /* Start of structure */
      memoryDmemSetLong(romtagstart);             /* Pointer to start of structure */
      memoryDmemSetLong(romtagstart+26);          /* Pointer to end of code */
      memoryDmemSetByte(0x81);                    /* Flags, AUTOINIT+COLDSTART */
      memoryDmemSetByte(0x1);                     /* Version */
      memoryDmemSetByte(3);                       /* DEVICE */
      memoryDmemSetByte(0);                       /* Priority */
      memoryDmemSetLong(devicename);              /* Pointer to name (used in opendev)*/
      memoryDmemSetLong(idstr);                   /* ID string */
      memoryDmemSetLong(initstruct);              /* Init_struct */

      /* Clear hardfile rom */

      memset(fhfile_rom, 0, 65536);

      /* Struct DiagArea */

      fhfile_rom[0x1000] = 0x90; /* da_Config */
      fhfile_rom[0x1001] = 0;    /* da_Flags */
      fhfile_rom[0x1002] = 0;    /* da_Size */
      fhfile_rom[0x1003] = 0x96;
      fhfile_rom[0x1004] = 0;    /* da_DiagPoint */
      fhfile_rom[0x1005] = 0x80;
      fhfile_rom[0x1006] = 0;    /* da_BootPoint */
      fhfile_rom[0x1007] = 0x90;
      fhfile_rom[0x1008] = 0;    /* da_Name */
      fhfile_rom[0x1009] = 0;
      fhfile_rom[0x100a] = 0;    /* da_Reserved01 */
      fhfile_rom[0x100b] = 0;
      fhfile_rom[0x100c] = 0;    /* da_Reserved02 */
      fhfile_rom[0x100d] = 0;

      fhfile_rom[0x1080] = 0x23; /* DiagPoint */
      fhfile_rom[0x1081] = 0xfc; /* move.l #$00010001,$f40000 */
      fhfile_rom[0x1082] = 0x00;
      fhfile_rom[0x1083] = 0x01;
      fhfile_rom[0x1084] = 0x00;
      fhfile_rom[0x1085] = 0x01;
      fhfile_rom[0x1086] = 0x00;
      fhfile_rom[0x1087] = 0xf4;
      fhfile_rom[0x1088] = 0x00;
      fhfile_rom[0x1089] = 0x00;
      fhfile_rom[0x108a] = 0x4e; /* rts */
      fhfile_rom[0x108b] = 0x75;

      fhfile_rom[0x1090] = 0x4e; /* BootPoint */
      fhfile_rom[0x1091] = 0xf9; /* JMP fhfile_bootcode */
      fhfile_rom[0x1092] = (UBY) (fhfile_bootcode>>24);
      fhfile_rom[0x1093] = (UBY) (fhfile_bootcode>>16);
      fhfile_rom[0x1094] = (UBY) (fhfile_bootcode>>8);
      fhfile_rom[0x1095] = (UBY) fhfile_bootcode;

      /* NULLIFY pointer to configdev */

      memoryDmemSetLongNoCounter(0, 4092);
      memoryEmemCardAdd(fhfileCardInit, fhfileCardMap);
  }
  else
    memoryDmemClear();
}


/*=========================*/
/* Startup hardfile device */
/*=========================*/

void fhfileStartup(void) {
  /* Clear first to ensure that F is NULL */
  memset(fhfile_devs, 0, sizeof(fhfile_dev)*FHFILE_MAX_DEVICES);
  fhfileClear();
}


/*==========================*/
/* Shutdown hardfile device */
/*==========================*/

void fhfileShutdown(void) {
  fhfileClear();
}

/*==========================*/
/* Create hardfile          */
/*==========================*/

BOOLE fhfileCreate(fhfile_dev hfile)
{
  BOOLE result = FALSE;

#ifdef WIN32
  HANDLE hf;

  if(*hfile.filename && hfile.size) 
  {   
    if((hf = CreateFile(hfile.filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL)) != INVALID_HANDLE_VALUE)
    {
      if( SetFilePointer(hf, hfile.size, NULL, FILE_BEGIN) == hfile.size )
	result = SetEndOfFile(hf);
      else
	fellowAddLog("SetFilePointer() failure.\n");
      CloseHandle(hf);
    }
    else
      fellowAddLog("CreateFile() failed.\n");
  }
  return result;
#else	/* os independent implementation */
#define BUFSIZE 32768
  ULO tobewritten;
  char buffer[BUFSIZE];
  FILE *hf;

  tobewritten = hfile.size;
  errno = 0;

  if(*hfile.filename && hfile.size) 
  {   
    if(hf = fopen(hfile.filename, "wb"))
    {
      memset(buffer, 0, sizeof(buffer));

      while(tobewritten >= BUFSIZE)
      {
	fwrite(buffer, sizeof(char), BUFSIZE, hf);
	if (errno != 0)
	{
	  fellowAddLog("Creating hardfile failed. Check the available space.\n");
	  fclose(hf);
	  return result;
	}
	tobewritten -= BUFSIZE;
      }
      fwrite(buffer, sizeof(char), tobewritten, hf);
      if (errno != 0)
      {
	fellowAddLog("Creating hardfile failed. Check the available space.\n");
	fclose(hf);
	return result;
      }
      fclose(hf);
      result = TRUE;
    }
    else
      fellowAddLog("fhfileCreate is unable to open output file.\n");
  }
  return result;
#endif
}
