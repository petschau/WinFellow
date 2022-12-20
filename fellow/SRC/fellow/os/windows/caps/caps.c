/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* C.A.P.S. Support - The Classic Amiga Preservation Society               */
/* http://www.softpres.org                                                 */
/*                                                                         */
/* (w)2004-2019 by Torsten Enderling                                       */
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

#include <assert.h>
#include "caps.h"

#ifdef FELLOW_SUPPORT_CAPS

#include "fellow/api/Services.h"
#include "fellow/api/Drivers.h"
#include "fellow/chipset/Floppy.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "Comtype.h"
#include "CapsAPI.h"

#ifdef __cplusplus
}
#endif

#include "CapsPlug.h"

using namespace fellow::api;

#ifdef _DEBUG
#define TRACECAPS 1
#else
#define TRACECAPS 0
#endif

static int capsFlags = DI_LOCK_INDEX | DI_LOCK_DENVAR | DI_LOCK_DENNOISE | DI_LOCK_NOISE | DI_LOCK_UPDATEFD;
static BOOLE capsDriveIsLocked[4] = {FALSE, FALSE, FALSE, FALSE};
static SDWORD capsDriveContainer[4] = {-1, -1, -1, -1};

static HMODULE capshModule = NULL;       /* handle for the library */
static BOOLE capsIsInitialized = FALSE;  /* is the module initialized? */
static BOOLE capsUserIsNotified = FALSE; /* if the library is missing, did we already notify the user? */

BOOLE capsStartup()
{
  int i;

  if (capsIsInitialized) return TRUE;

  SDWORD result;
  result = CapsInit("CapsImg.dll");

  if (result != 0)
  {
    const char* errorMessage = "IPF Images need a current C.A.P.S. Plug-In!\nYou can download it from:\nhttp://www.softpres.org/download";
    Service->Log.AddLog(errorMessage);
    Driver->Gui.Requester(FELLOW_REQUESTER_TYPE::FELLOW_REQUESTER_TYPE_INFO, errorMessage);
    capsUserIsNotified = TRUE;
    Service->Log.AddLog("capsStartup(): Unable to open the CAPS Plug-In.\n");
    return FALSE;
  }
  else
  {
    capsIsInitialized = TRUE;
  }

  for (i = 0; i < 4; i++)
    capsDriveContainer[i] = CapsAddImage();

  capsIsInitialized = TRUE;
  Service->Log.AddLog("capsStartup(): CAPS IPF Image library loaded successfully.\n");

  return TRUE;
}

BOOLE capsShutdown()
{
  return CapsExit();
}

BOOLE capsUnloadImage(ULO drive)
{
  if (!capsDriveIsLocked[drive]) return FALSE;

  CapsUnlockAllTracks(capsDriveContainer[drive]);
  CapsUnlockImage(capsDriveContainer[drive]);
  capsDriveIsLocked[drive] = FALSE;
  Service->Log.AddLog("capsUnloadImage(): Image %s unloaded from drive no %u.\n", floppy[drive].imagename, drive);
  return TRUE;
}

static void capsLogImageInfo(struct CapsImageInfo *capsImageInfo, ULO drive)
{
  int i;
  char DateString[100], TypeString[100], PlatformString[100];
  struct CapsDateTimeExt *capsDateTimeExt;

  if (!capsImageInfo) return;

  /* extract the date from information */
  capsDateTimeExt = &capsImageInfo->crdt;
  sprintf(DateString, "%02u.%02u.%04u %02u:%02u:%02u", capsDateTimeExt->day, capsDateTimeExt->month, capsDateTimeExt->year, capsDateTimeExt->hour, capsDateTimeExt->min, capsDateTimeExt->sec);

  /* generate a type string */
  switch (capsImageInfo->type)
  {
    case ciitNA: sprintf(TypeString, "ciitNA (invalid image)"); break;
    case ciitFDD: sprintf(TypeString, "ciitFDD (floppy disk)"); break;
    default: sprintf(TypeString, "N/A ()"); break;
  }

  /* generate a platform string */
  for (i = 0; capsImageInfo->platform[i] != 0; i++)
  {
    if (i > 0)
    {
      char AppendString[100];
      sprintf(AppendString, CapsGetPlatformName(capsImageInfo->platform[i]));
      strcat(PlatformString, ", ");
      strcat(PlatformString, AppendString);
    }
    else
      sprintf(PlatformString, CapsGetPlatformName(capsImageInfo->platform[i]));
  }

  /* log the information */
  Service->Log.AddTimelessLog("\nCAPS Image Information:\n");
  Service->Log.AddTimelessLog("Floppy Drive No: %u\n", drive);
  Service->Log.AddTimelessLog("Filename: %s\n", floppy[drive].imagename);
  Service->Log.AddTimelessLog("Type:%s\n", TypeString);
  Service->Log.AddTimelessLog("Date:%s\n", DateString);
  Service->Log.AddTimelessLog("Release:%04d Revision:%d\n", capsImageInfo->release, capsImageInfo->revision);
  Service->Log.AddTimelessLog("Intended platform(s):%s\n\n", PlatformString);
}

BOOLE capsLoadImage(ULO drive, FILE *F, ULO *tracks)
{
  struct CapsImageInfo capsImageInfo;
  ULO ImageSize, ReturnCode;
  UBY *ImageBuffer;

  /* make sure we're up and running beforehand */
  if (!capsIsInitialized)
    if (!capsStartup()) return FALSE;

  capsUnloadImage(drive);

  Service->Log.AddLog("capsLoadImage(): Attempting to load IPF Image %s into drive %u.\n", floppy[drive].imagename, drive);

  fseek(F, 0, SEEK_END);
  ImageSize = ftell(F);
  fseek(F, 0, SEEK_SET);

  ImageBuffer = (UBY *)malloc(ImageSize);
  if (!ImageBuffer) return FALSE;

  if (fread(ImageBuffer, ImageSize, 1, F) == 0) return FALSE;

  ReturnCode = CapsLockImageMemory(capsDriveContainer[drive], ImageBuffer, ImageSize, 0);
  free(ImageBuffer);

  if (ReturnCode != imgeOk) return FALSE;

  capsDriveIsLocked[drive] = TRUE;

  CapsGetImageInfo(&capsImageInfo, capsDriveContainer[drive]);
  *tracks = ((capsImageInfo.maxcylinder - capsImageInfo.mincylinder + 1) * (capsImageInfo.maxhead - capsImageInfo.minhead + 1) + 1) / 2;

  CapsLoadImage(capsDriveContainer[drive], capsFlags);
  capsLogImageInfo(&capsImageInfo, drive);
  Service->Log.AddLog("capsLoadImage(): Image loaded successfully.\n");
  return TRUE;
}

BOOLE capsLoadTrack(ULO drive, ULO track, UBY *mfm_data, ULO *tracklength, ULO *maxtracklength, ULO *timebuf, BOOLE *flakey)
{
  ULO i, len, type;
  struct CapsTrackInfo capsTrackInfo;

  *timebuf = 0;
  CapsLockTrack(&capsTrackInfo, capsDriveContainer[drive], track / 2, track & 1, capsFlags);
  *flakey = (capsTrackInfo.type & CTIT_FLAG_FLAKEY) ? TRUE : FALSE;
  type = capsTrackInfo.type & CTIT_MASK_TYPE;
  len = capsTrackInfo.tracksize[0];
  *tracklength = len;
  *maxtracklength = 0;
  /* trackcnt contains number of valid entries, we need to determine the max tracklen value for
  correct sizing of the MFM buffer in case these lengths differ between revolutions; each
  track should be loaded once with capsLoadTrack() to reserve the correct space in the MFM
  buffer, later capsLoadRevolution() can be called successively to update the existing MFM buffer
  */
  for (i = 0; i < capsTrackInfo.trackcnt; i++)
    if (capsTrackInfo.tracksize[i] > *maxtracklength) *maxtracklength = capsTrackInfo.tracksize[i];

  if (*maxtracklength % 2 == 1) /* like it better always even */
    *maxtracklength += 1;

  memset(mfm_data, 0, *maxtracklength);
  memcpy(mfm_data, capsTrackInfo.trackdata[0], len);

#if TRACECAPS
  {
    FILE *f;
    char filename[MAX_PATH];

    Service->Fileops.GetGenericFileName(filename, "WinFellow", "CAPSDump.txt");
    f = fopen(filename, "wb");
    fwrite(capsTrackInfo.trackdata[0], len, 1, f);
    fclose(f);
  }
#endif

  if (capsTrackInfo.timelen > 0)
    for (i = 0; i < capsTrackInfo.timelen; i++)
      timebuf[i] = (ULO)capsTrackInfo.timebuf[i];

#if TRACECAPS
  Service->Log.AddTimelessLog(
      "CAPS Track Information: drive:%u track:%03u flakey:%s trackcnt:%d timelen:%05d type:%d\n", drive, track, *flakey ? "TRUE " : "FALSE", capsTrackInfo.trackcnt, capsTrackInfo.timelen, type);
#endif

  return TRUE;
}

BOOLE capsLoadNextRevolution(ULO drive, ULO track, UBY *mfm_data, ULO *tracklength)
{
  static ULO revolutioncount = 0;
  ULO revolution, len;
  struct CapsTrackInfo capsTrackInfo;

  revolutioncount++;
  CapsLockTrack(&capsTrackInfo, capsDriveContainer[drive], track / 2, track & 1, capsFlags);
  revolution = revolutioncount % capsTrackInfo.trackcnt;
  len = capsTrackInfo.tracksize[revolution];
  /*if(*tracklength != len)
  {
  Service->Log.AddLog("capsLoadRevolution(): Variable track size not implemented, will result in MFM buffer corruption!!!\n");
  assert(0);
  }*/
  *tracklength = len;
  memcpy(mfm_data, capsTrackInfo.trackdata[revolution], len);

  return TRUE;
}

#endif