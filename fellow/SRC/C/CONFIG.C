/*=========================================================================*/
/* Fellow                                                                  */
/* Configuration file handling                                             */
/*                                                                         */
/* Author: Petter Schau, Worfje (worfje@gmx.net), Torsten Enderling        */
/*                                                                         */
/* Based on an Amiga emulator configuration format specified by Brian King */
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

#ifdef _FELLOW_DEBUG_CRT_MALLOC
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "defs.h"
#include "versioninfo.h"
#include "chipset.h"
#include "floppy.h"
#include "fmem.h"
#include "gameport.h"
#include "sound.h"
#include "graph.h"
#include "draw.h"
#include "blit.h"
#include "fellow.h"
#include "listtree.h"
#include "eventid.h"
#include "fswrap.h"
#include "config.h"
#include "fhfile.h"
#include "ffilesys.h"
#include "ini.h"
#include "CpuIntegration.h"
#include "fileops.h"
#include "rtc.h"
#include "RetroPlatform.h"
#include "draw_interlace_control.h"
#include "wgui.h"
#include "KBDDRV.H"

ini *cfg_initdata;								 /* CONFIG copy of initialization data */

/*============================================================================*/
/* The actual cfgManager instance                                             */
/*============================================================================*/

cfgManager cfg_manager;


/*============================================================================*/
/* Configuration                                                              */
/*============================================================================*/

void cfgSetDescription(cfg *config, STR *description)
{
  strncpy(config->m_description, description, 255);
}

STR *cfgGetDescription(cfg *config)
{
  return config->m_description;
}


/*============================================================================*/
/* Floppy disk configuration property access                                  */
/*============================================================================*/

void cfgSetDiskImage(cfg *config, ULO index, STR *diskimage)
{
  if (index < 4) 
  {
    strncpy(&(config->m_diskimage[index][0]), diskimage, CFG_FILENAME_LENGTH);
  }
}

STR *cfgGetDiskImage(cfg *config, ULO index) 
{
  if (index < 4)
  {
    return &(config->m_diskimage[index][0]);
  }
  return "";
}

void cfgSetDiskEnabled(cfg *config, ULO index, BOOLE enabled)
{
  if (index < 4)
  {
    config->m_diskenabled[index] = enabled;
  }
}

BOOLE cfgGetDiskEnabled(cfg *config, ULO index)
{
  if (index < 4)
  {
    return config->m_diskenabled[index];
  }
  return FALSE;
}

void cfgSetDiskReadOnly(cfg *config, ULO index, BOOLE readonly)
{
  if (index < 4)
  {
    config->m_diskreadonly[index] = readonly;
  }
}

BOOLE cfgGetDiskReadOnly(cfg *config, ULO index)
{
  if (index < 4)
  {
    return config->m_diskreadonly[index];
  }
  return FALSE;
}

void cfgSetDiskFast(cfg *config, BOOLE fast)
{
  config->m_diskfast = fast;
}

BOOLE cfgGetDiskFast(cfg *config)
{
  return config->m_diskfast;
}

void cfgSetLastUsedDiskDir(cfg *config, STR *directory)
{
  if(directory != nullptr) {
    strncpy(config->m_lastuseddiskdir, directory, CFG_FILENAME_LENGTH);
  }
}

STR *cfgGetLastUsedDiskDir(cfg *config)
{
  return config->m_lastuseddiskdir;
}


/*============================================================================*/
/* Memory configuration property access                                       */
/*============================================================================*/

void cfgSetChipSize(cfg *config, ULO chipsize)
{
  chipsize &= 0x3c0000;
  if (chipsize == 0)
  {
    chipsize = 0x40000;
  }
  else if (chipsize > 0x200000)
  {
    chipsize = 0x200000;
  }
  config->m_chipsize = chipsize;
}

ULO cfgGetChipSize(cfg *config)
{
  return config->m_chipsize;
}

void cfgSetFastSize(cfg *config, ULO fastsize)
{
  if (fastsize >= 0x800000)
  {
    config->m_fastsize = 0x800000;
  }
  else if (fastsize >= 0x400000)
  {
    config->m_fastsize = 0x400000;
  }
  else if (fastsize >= 0x200000)
  {
    config->m_fastsize = 0x200000;
  }
  else if (fastsize >= 0x100000)
  {
    config->m_fastsize = 0x100000;
  }
  else
  {
    config->m_fastsize = 0;
  }
}

ULO cfgGetFastSize(cfg *config)
{
  return config->m_fastsize;
}

void cfgSetBogoSize(cfg *config, ULO bogosize)
{
  config->m_bogosize = bogosize & 0x1c0000;
}

ULO cfgGetBogoSize(cfg *config)
{
  return config->m_bogosize;
}

void cfgSetKickImage(cfg *config, STR *kickimage)
{
  strncpy(config->m_kickimage, kickimage, CFG_FILENAME_LENGTH);
  //fsNavigMakeRelativePath(config->m_kickimage);
}

STR *cfgGetKickImage(cfg *config)
{
  return config->m_kickimage;
}

void cfgSetKickImageExtended(cfg *config, STR *kickimageext)
{
  strncpy(config->m_kickimage_ext, kickimageext, CFG_FILENAME_LENGTH);
}

STR *cfgGetKickImageExtended(cfg *config)
{
  return config->m_kickimage_ext;
}

void cfgSetKickDescription(cfg *config, STR *kickdescription)
{
  strncpy(config->m_kickdescription, kickdescription, CFG_FILENAME_LENGTH);
}

STR *cfgGetKickDescription(cfg *config)
{
  return config->m_kickdescription;
}

void cfgSetKickCRC32(cfg *config, ULO kickcrc32)
{
  config->m_kickcrc32 = kickcrc32;
}

ULO cfgGetKickCRC32(cfg *config)
{
  return config->m_kickcrc32;
}

void cfgSetKey(cfg *config, STR *key)
{
  strncpy(config->m_key, key, CFG_FILENAME_LENGTH);
  //fsNavigMakeRelativePath(config->m_key);
}

STR *cfgGetKey(cfg *config)
{
  return config->m_key;
}

void cfgSetUseAutoconfig(cfg *config, BOOLE useautoconfig)
{
  config->m_useautoconfig = useautoconfig;
}

BOOLE cfgGetUseAutoconfig(cfg *config)
{
  return config->m_useautoconfig;
}

BOOLE cfgGetAddress32Bit(cfg *config)
{ /* CPU type decides this */
  return (cfgGetCPUType(config) == M68020) || (cfgGetCPUType(config) == M68030);
}

void cfgSetRtc(cfg *config, bool rtc)
{
  config->m_rtc = rtc;
}

bool cfgGetRtc(cfg *config)
{
  return config->m_rtc;
}

/*============================================================================*/
/* Screen configuration property access                                       */
/*============================================================================*/

void cfgSetScreenWidth(cfg *config, ULO screenwidth)
{
  config->m_screenwidth = screenwidth;
}

ULO cfgGetScreenWidth(cfg *config)
{
  return config->m_screenwidth;
}

void cfgSetScreenHeight(cfg *config, ULO screenheight)
{
  config->m_screenheight = screenheight;
}

ULO cfgGetScreenHeight(cfg *config)
{
  return config->m_screenheight;
}

void cfgSetScreenColorBits(cfg *config, ULO screencolorbits)
{
  config->m_screencolorbits = screencolorbits;
}

ULO cfgGetScreenColorBits(cfg *config)
{
  return config->m_screencolorbits;
}

void cfgSetScreenWindowed(cfg *config, bool screenwindowed)
{
  config->m_screenwindowed = screenwindowed;
}

bool cfgGetScreenWindowed(cfg *config)
{
  return config->m_screenwindowed;
}

void cfgSetScreenRefresh(cfg *config, ULO screenrefresh)
{
  config->m_screenrefresh = screenrefresh;
}

ULO cfgGetScreenRefresh(cfg *config)
{
  return config->m_screenrefresh;
}

void cfgSetScreenDrawLEDs(cfg *config, bool drawleds)
{
  config->m_screendrawleds = drawleds;
}

bool cfgGetScreenDrawLEDs(cfg *config)
{
  return config->m_screendrawleds;
}

void cfgSetUseMultipleGraphicalBuffers(cfg *config, BOOLE use_multiple_graphical_buffers)
{
  config->m_use_multiple_graphical_buffers = use_multiple_graphical_buffers;
}

BOOLE cfgGetUseMultipleGraphicalBuffers(cfg *config)
{
  return config->m_use_multiple_graphical_buffers;
}

void cfgSetDisplayDriver(cfg *config, DISPLAYDRIVER display_driver)
{
  config->m_displaydriver = display_driver;

  if (!gfxDrvDXGIValidateRequirements())
  {
    fellowAddLog("cfgSetDisplayDriver(): Direct3D requirements not met, falling back to DirectDraw.\n");
    config->m_displaydriver = DISPLAYDRIVER_DIRECTDRAW;
  }
}

DISPLAYDRIVER cfgGetDisplayDriver(cfg *config)
{
  return config->m_displaydriver;
}


/*===========================================================================*/
/* Graphics emulation configuration property access                          */
/*===========================================================================*/

void cfgSetFrameskipRatio(cfg *config, ULO frameskipratio)
{
  config->m_frameskipratio = frameskipratio;
}

ULO cfgGetFrameskipRatio(cfg *config)
{
  return config->m_frameskipratio;
}

void cfgSetClipMode(cfg *config, DISPLAYCLIP_MODE clipmode)
{
  config->m_clipmode = clipmode;
}

DISPLAYCLIP_MODE cfgGetClipMode(cfg *config)
{
  return config->m_clipmode;
}

void cfgSetClipLeft(cfg *config, ULO clipleft)
{
  config->m_clipleft = clipleft;
}

ULO cfgGetClipLeft(cfg *config)
{
  return config->m_clipleft;
}

void cfgSetClipTop(cfg *config, ULO cliptop)
{
  config->m_cliptop = cliptop;
}

ULO cfgGetClipTop(cfg *config)
{
  return config->m_cliptop;
}

void cfgSetClipRight(cfg *config, ULO clipright)
{
  config->m_clipright = clipright;
}

ULO cfgGetClipRight(cfg *config)
{
  return config->m_clipright;
}

void cfgSetClipBottom(cfg *config, ULO clipbottom)
{
  config->m_clipbottom = clipbottom;
}

ULO cfgGetClipBottom(cfg *config)
{
  return config->m_clipbottom;
}

void cfgSetDisplayScale(cfg *config, DISPLAYSCALE displayscale)
{
  config->m_displayscale = displayscale;
}

DISPLAYSCALE cfgGetDisplayScale(cfg *config)
{
  return config->m_displayscale;
}

void cfgSetDisplayScaleStrategy(cfg *config, DISPLAYSCALE_STRATEGY displayscalestrategy)
{
  config->m_displayscalestrategy = displayscalestrategy;
}

DISPLAYSCALE_STRATEGY cfgGetDisplayScaleStrategy(cfg *config)
{
  return config->m_displayscalestrategy;
}

void cfgSetDeinterlace(cfg *config, bool deinterlace)
{
  config->m_deinterlace = deinterlace;
}

bool cfgGetDeinterlace(cfg *config)
{
  return config->m_deinterlace;
}

void cfgSetGraphicsEmulationMode(cfg *config, GRAPHICSEMULATIONMODE graphicsemulationmode)
{
  config->m_graphicsemulationmode = graphicsemulationmode;
}

GRAPHICSEMULATIONMODE cfgGetGraphicsEmulationMode(cfg *config)
{
  return config->m_graphicsemulationmode;
}


/*============================================================================*/
/* Sound configuration property access                                        */
/*============================================================================*/

void cfgSetSoundEmulation(cfg *config, sound_emulations soundemulation) 
{
  config->m_soundemulation = soundemulation;
}

sound_emulations cfgGetSoundEmulation(cfg *config) 
{
  return config->m_soundemulation;
}

void cfgSetSoundRate(cfg *config, sound_rates soundrate) 
{
  config->m_soundrate = soundrate;
}

sound_rates cfgGetSoundRate(cfg *config) 
{
  return config->m_soundrate;
}

void cfgSetSoundStereo(cfg *config, bool soundstereo) 
{
  config->m_soundstereo = soundstereo;
}

bool cfgGetSoundStereo(cfg *config) 
{
  return config->m_soundstereo;
}

void cfgSetSound16Bits(cfg *config, bool sound16bits) 
{
  config->m_sound16bits = sound16bits;
}

bool cfgGetSound16Bits(cfg *config) 
{
  return config->m_sound16bits;
}

void cfgSetSoundFilter(cfg *config, sound_filters soundfilter) 
{
  config->m_soundfilter = soundfilter;
}

sound_filters cfgGetSoundFilter(cfg *config) 
{
  return config->m_soundfilter;
}

void cfgSetSoundVolume(cfg *config, const ULO soundvolume) 
{
  config->m_soundvolume = soundvolume;
}

ULO cfgGetSoundVolume(cfg *config) 
{
  return config->m_soundvolume;
}

void cfgSetSoundWAVDump(cfg *config, BOOLE soundWAVdump) 
{
  config->m_soundWAVdump = soundWAVdump;
}

BOOLE cfgGetSoundWAVDump(cfg *config)
{
  return config->m_soundWAVdump;
}

void cfgSetSoundNotification(cfg *config, sound_notifications soundnotification)
{
  config->m_notification = soundnotification;
}

sound_notifications cfgGetSoundNotification(cfg *config)
{
  return config->m_notification;
}

void cfgSetSoundBufferLength(cfg *config, ULO buffer_length)
{
  config->m_bufferlength = buffer_length;
}

ULO cfgGetSoundBufferLength(cfg *config)
{
  return config->m_bufferlength;
}

/*============================================================================*/
/* CPU configuration property access                                          */
/*============================================================================*/

void cfgSetCPUType(cfg *config, cpu_integration_models CPUtype)
{
  config->m_CPUtype = CPUtype;
}

cpu_integration_models cfgGetCPUType(cfg *config)
{
  return config->m_CPUtype;
}

void cfgSetCPUSpeed(cfg *config, ULO CPUspeed)
{
  config->m_CPUspeed = CPUspeed;
}

ULO cfgGetCPUSpeed(cfg *config)
{
  return config->m_CPUspeed;
}


/*============================================================================*/
/* Custom chipset configuration property access                               */
/*============================================================================*/

void cfgSetBlitterFast(cfg *config, BOOLE blitterfast)
{
  config->m_blitterfast = blitterfast;
}

BOOLE cfgGetBlitterFast(cfg *config)
{
  return config->m_blitterfast;
}

void cfgSetECS(cfg *config, bool ecs)
{
  config->m_ECS = ecs;
}

bool cfgGetECS(cfg *config)
{
  return config->m_ECS;
}


/*============================================================================*/
/* Hardfile configuration property access                                     */
/*============================================================================*/

cfg_hardfile cfgGetHardfile(cfg *config, ULO index)
{
  return *static_cast<cfg_hardfile *>(listNode(listIndex(config->m_hardfiles, index)));
}

ULO cfgGetHardfileCount(cfg *config)
{
  return listCount(config->m_hardfiles);
}

void cfgHardfileAdd(cfg *config, cfg_hardfile *hardfile)
{
  cfg_hardfile *hf = static_cast<cfg_hardfile *>(malloc(sizeof(cfg_hardfile)));
  *hf = *hardfile;
  config->m_hardfiles = listAddLast(config->m_hardfiles, listNew(hf));
}

void cfgHardfileRemove(cfg *config, ULO index)
{
  felist *node = listIndex(config->m_hardfiles, index);
  if (index == 0)
  {
    config->m_hardfiles = listNext(node);
  }
  free(listNode(node));
  listFree(node);
}      

void cfgHardfilesFree(cfg *config)
{
  listFreeAll(config->m_hardfiles, TRUE);
  config->m_hardfiles = nullptr;
}

void cfgSetHardfileUnitDefaults(cfg_hardfile *hardfile)
{
  memset(hardfile, 0, sizeof(cfg_hardfile));
  hardfile->readonly = FALSE;
  hardfile->bytespersector = 512;
  hardfile->sectorspertrack = 32;
  hardfile->surfaces = 1;
  hardfile->reservedblocks = 2;
}

void cfgHardfileChange(cfg *config, cfg_hardfile *hardfile, ULO index)
{
  felist *node = listIndex(config->m_hardfiles, index);
  cfg_hardfile *hf = static_cast<cfg_hardfile *>(listNode(node));
  *hf = *hardfile;
}


/*============================================================================*/
/* Filesystem configuration property access                                   */
/*============================================================================*/

cfg_filesys cfgGetFilesystem(cfg *config, ULO index)
{
  return *static_cast<cfg_filesys *>(listNode(listIndex(config->m_filesystems, index)));
}

ULO cfgGetFilesystemCount(cfg *config)
{
  return listCount(config->m_filesystems);
}

void cfgFilesystemAdd(cfg *config, cfg_filesys *filesystem)
{
  cfg_filesys *fsys = static_cast<cfg_filesys *>(malloc(sizeof(cfg_filesys)));
  *fsys = *filesystem;
  config->m_filesystems = listAddLast(config->m_filesystems, listNew(fsys));
}

void cfgFilesystemRemove(cfg *config, ULO index)
{
  felist *node = listIndex(config->m_filesystems, index);
  if (index == 0)
  {
    config->m_filesystems = listNext(node);
  }
  free(listNode(node));
  listFree(node);
}

void cfgFilesystemsFree(cfg *config)
{
  listFreeAll(config->m_filesystems, TRUE);
  config->m_filesystems = nullptr;
}

void cfgSetFilesystemUnitDefaults(cfg_filesys *unit)
{
  memset(unit, 0, sizeof(cfg_filesys));
  unit->readonly = FALSE;
}

void cfgFilesystemChange(cfg *config, cfg_filesys *unit, ULO index)
{
  felist *node = listIndex(config->m_filesystems, index);
  cfg_filesys *fsys = static_cast<cfg_filesys *>(listNode(node));
  *fsys = *unit;
}

void cfgSetFilesystemAutomountDrives(cfg *config, BOOLE automount_drives)
{
  config->m_automount_drives = automount_drives;
}

BOOLE cfgGetFilesystemAutomountDrives(cfg *config)
{
  return config->m_automount_drives;
}

/*============================================================================*/
/* Game port configuration property access                                    */
/*============================================================================*/

void cfgSetGameport(cfg *config, ULO index, gameport_inputs gameport)
{
  if (index < 2)
  {
    config->m_gameport[index] = gameport;
  }
}

gameport_inputs cfgGetGameport(cfg *config, ULO index)
{
  if (index < 2)
  {
    return config->m_gameport[index];
  }
  return GP_NONE;
}


/*============================================================================*/
/* GUI configuration property access                                          */
/*============================================================================*/

void cfgSetUseGUI(cfg *config, BOOLE useGUI)
{
  config->m_useGUI = useGUI;
}

BOOLE cfgGetUseGUI(cfg *config)
{
  return config->m_useGUI;
}


/*============================================================================*/
/* Various configuration property access                                      */
/*============================================================================*/

void cfgSetMeasureSpeed(cfg *config, bool measurespeed)
{
  config->m_measurespeed = measurespeed;
}

bool cfgGetMeasureSpeed(cfg *config)
{
  return config->m_measurespeed;
}

/*============================================================================*/
/* Sets all options to default values                                         */
/*============================================================================*/

void cfgSetDefaults(cfg *config)
{
  if(config == nullptr) return;

  /*==========================================================================*/
  /* Default configuration description                                        */
  /*==========================================================================*/

  cfgSetDescription(config, FELLOWLONGVERSION);

  /*==========================================================================*/
  /* Default floppy disk configuration                                        */
  /*==========================================================================*/

  for (int i = 0; i < 4; i++)
  {
    cfgSetDiskImage(config, i, "");
    cfgSetDiskEnabled(config, i, TRUE);
    cfgSetDiskReadOnly(config, i, FALSE);
  }
  cfgSetDiskFast(config, FALSE);
  cfgSetLastUsedDiskDir(config, "");

  /*==========================================================================*/
  /* Default memory configuration                                             */
  /*==========================================================================*/

  cfgSetChipSize(config, 0x80000);
  cfgSetFastSize(config, 0);
  cfgSetBogoSize(config, 0x80000);
  cfgSetKickImage(config, "");
  cfgSetKickImageExtended(config, "");
  cfgSetKickDescription(config, "");
  cfgSetKickCRC32(config, 0);
  cfgSetKey(config, "");
  cfgSetUseAutoconfig(config, FALSE);
  cfgSetRtc(config, true);

  /*==========================================================================*/
  /* Default screen configuration                                             */
  /*==========================================================================*/

  cfgSetScreenWidth(config, 640);
  cfgSetScreenHeight(config, 400);
  cfgSetScreenColorBits(config, 16);
  cfgSetScreenWindowed(config, true);
  cfgSetScreenRefresh(config, 0);
  cfgSetUseMultipleGraphicalBuffers(config, FALSE);
  cfgSetScreenDrawLEDs(config, true);
  cfgSetDeinterlace(config, true);
  cfgSetDisplayDriver(config, DISPLAYDRIVER_DIRECTDRAW);

  /*==========================================================================*/
  /* Default graphics emulation configuration                                 */
  /*==========================================================================*/

  cfgSetFrameskipRatio(config, 0);
  cfgSetDisplayScale(config, DISPLAYSCALE_1X);
  cfgSetDisplayScaleStrategy(config, DISPLAYSCALE_STRATEGY_SOLID);
  cfgSetGraphicsEmulationMode(config, GRAPHICSEMULATIONMODE_LINEEXACT);
  cfgSetClipMode(config, DISPLAYCLIP_MODE::AUTOMATIC_CLIP);
  cfgSetClipLeft(config, 129); // Match for 640x400 at 1x
  cfgSetClipTop(config, 44);
  cfgSetClipRight(config, 449);
  cfgSetClipRight(config, 244);

  /*==========================================================================*/
  /* Default sound configuration                                              */
  /*==========================================================================*/

  cfgSetSoundEmulation(config, SOUND_PLAY);
  cfgSetSoundRate(config, SOUND_44100);
  cfgSetSoundStereo(config, TRUE);
  cfgSetSound16Bits(config, TRUE);
  cfgSetSoundFilter(config, SOUND_FILTER_ORIGINAL);
  cfgSetSoundVolume(config, 100);
  cfgSetSoundWAVDump(config, FALSE);
  cfgSetSoundNotification(config, SOUND_MMTIMER_NOTIFICATION);
  cfgSetSoundBufferLength(config, 60);

  /*==========================================================================*/
  /* Default CPU configuration                                                */
  /*==========================================================================*/

  cfgSetCPUType(config, M68000);
  cfgSetCPUSpeed(config, 4);


  /*==========================================================================*/
  /* Default custom chipset configuration                                     */
  /*==========================================================================*/

  cfgSetBlitterFast(config, FALSE);
  cfgSetECS(config, false);


  /*==========================================================================*/
  /* Default hardfile configuration                                           */
  /*==========================================================================*/

  cfgHardfilesFree(config);


  /*==========================================================================*/
  /* Default filesystem configuration                                         */
  /*==========================================================================*/

  cfgFilesystemsFree(config);
  cfgSetFilesystemAutomountDrives(config, FALSE);


  /*==========================================================================*/
  /* Default game port configuration                                          */
  /*==========================================================================*/

  cfgSetGameport(config, 0, GP_MOUSE0);
  cfgSetGameport(config, 1, GP_NONE);


  /*==========================================================================*/
  /* Default GUI configuration                                                */
  /*==========================================================================*/

  cfgSetUseGUI(config, TRUE);


  /*==========================================================================*/
  /* Default various configuration                                            */
  /*==========================================================================*/

  cfgSetMeasureSpeed(config, false);

  cfgSetConfigAppliedOnce(config, false);
  cfgSetConfigChangedSinceLastSave(config, FALSE);
}


/*============================================================================*/
/* Read specific options from a string                                        */
/* These verify the options, or at least return a default value on error      */
/*============================================================================*/

static BOOLE cfgGetBOOLEFromString(STR *value)
{
  return (value[0] == 'y' || value[0] == 't');
}

static const STR *cfgGetBOOLEToString(BOOLE value)
{
  return (value) ? "yes" : "no";
}

static bool cfgGetboolFromString(STR *value)
{
  return (value[0] == 'y' || value[0] == 't');
}

static const STR *cfgGetboolToString(bool value)
{
  return (value) ? "yes" : "no";
}

static ULO cfgGetULOFromString(STR *value)
{
  return atoi(value);
}

static gameport_inputs cfgGetGameportFromString(STR *value)
{
  if (stricmp(value, "mouse") == 0)
  {
    return GP_MOUSE0;
  }
  if (stricmp(value, "joy0") == 0)
  {
    return GP_ANALOG0;
  }
  if (stricmp(value, "joy1") == 0)
  {
    return GP_ANALOG1;
  }
  if (stricmp(value, "kbd1") == 0)
  {
    return GP_JOYKEY0;
  }
  if (stricmp(value, "kbd2") == 0)
  {
    return GP_JOYKEY1;
  }
  return GP_NONE;
}

static STR *cfgGetGameportToString(gameport_inputs gameport)
{
  switch (gameport)
  {
    case GP_NONE:    return "none";
    case GP_MOUSE0:  return "mouse";
    case GP_ANALOG0: return "joy0";
    case GP_ANALOG1: return "joy1";
    case GP_JOYKEY0: return "kbd1";
    case GP_JOYKEY1: return "kbd2";
  }
  return "none";
}

static cpu_integration_models cfgGetCPUTypeFromString(STR *value)
{
  if (stricmp(value, "68000") == 0)
  {
    return M68000;
  }
  if (stricmp(value, "68010") == 0)
  {
    return M68010;
  }
  if (stricmp(value, "68020") == 0)
  {
    return M68020;
  }
  if (stricmp(value, "68020/68881") == 0)
  {
    return M68020;  /* Unsupp */
  }
  if (stricmp(value, "68ec20") == 0)
  {
    return M68EC20;
  }
  if (stricmp(value, "68ec20/68881") == 0)
  {
    return M68EC20; /* Unsupp */
  }
  if (stricmp(value, "68030") == 0)
  {
    return M68030;
  }
  if (stricmp(value, "68ec30") == 0)
  {
    return M68EC30;
  }
  return M68000;
}

static STR *cfgGetCPUTypeToString(cpu_integration_models cputype)
{
  switch (cputype)
  {
    case M68000:  return "68000";
    case M68010:  return "68010";
    case M68020:  return "68020";
    case M68EC20: return "68ec20";
    case M68030:  return "68030";
    case M68EC30: return "68ec30";
  }
  return "68000";
}

static ULO cfgGetCPUSpeedFromString(STR *value)
{
  ULO speed;

  if (stricmp(value, "real") == 0)
  {
    return 4;
  }
  if (stricmp(value, "max") == 0)
  {
    return 1;
  }
  speed = cfgGetULOFromString(value);
  if (speed > 20)
  {
    speed = 8;
  }
  return speed;
}

static sound_notifications cfgGetSoundNotificationFromString(STR *value)
{
  if (stricmp(value, "directsound") == 0)
  {
    return SOUND_DSOUND_NOTIFICATION;
  }
  if (stricmp(value, "mmtimer") == 0)
  {
    return SOUND_MMTIMER_NOTIFICATION;
  }
  return SOUND_MMTIMER_NOTIFICATION;
}

static STR *cfgGetSoundNotificationToString(sound_notifications soundnotification)
{
  switch (soundnotification)
  {
    case SOUND_DSOUND_NOTIFICATION:  return "directsound";
    case SOUND_MMTIMER_NOTIFICATION: return "mmtimer";
  }
  return "mmtimer";
}

static sound_emulations cfgGetSoundEmulationFromString(STR *value)
{
  if (stricmp(value, "none") == 0)
  {
    return SOUND_NONE;
  }
  if (stricmp(value, "interrupts") == 0)
  {
    return SOUND_EMULATE;
  }
  if (stricmp(value, "normal") == 0 ||
      stricmp(value, "exact") == 0 ||
      stricmp(value, "good") == 0 ||
      stricmp(value, "best") == 0)
  {
    return SOUND_PLAY;
  }
  return SOUND_NONE;
}

static STR *cfgGetSoundEmulationToString(sound_emulations soundemulation)
{
  switch (soundemulation)
  {
    case SOUND_NONE:    return "none";
    case SOUND_EMULATE: return "interrupts";
    case SOUND_PLAY:    return "normal";
  }
  return "none";
}

static bool cfgGetSoundStereoFromString(STR *value)
{
  if (stricmp(value, "mono") == 0 ||
      stricmp(value, "m") == 0 ||
      stricmp(value, "1") == 0)
  {
    return false;
  }
  if ((stricmp(value, "stereo") == 0) ||
    (stricmp(value, "s") == 0) ||
    (stricmp(value, "2") == 0))
  {
    return true;
  }
  return false;
}

static const STR *cfgGetSoundStereoToString(bool soundstereo)
{
  return (soundstereo) ? "stereo" : "mono";
}

static bool cfgGetSound16BitsFromString(STR *value)
{
  return stricmp(value, "16") == 0;
}

static const STR *cfgGetSound16BitsToString(bool sound16bits)
{
  return (sound16bits) ? "16" : "8";
}

static sound_rates cfgGetSoundRateFromString(STR *value)
{
  ULO rate = cfgGetULOFromString(value);

  if (rate < 22050)
  {
    return SOUND_15650;
  }
  if (rate < 31300)
  {
    return SOUND_22050;
  }
  if (rate < 44100)
  {
    return SOUND_31300;
  }
  return SOUND_44100;
}

static STR *cfgGetSoundRateToString(sound_rates soundrate)
{
  switch (soundrate)
  {
    case SOUND_15650: return "15650";
    case SOUND_22050: return "22050";
    case SOUND_31300: return "31300";
    case SOUND_44100: return "44100";
  }
  return "44100";
}

static sound_filters cfgGetSoundFilterFromString(STR *value)
{
  if (stricmp(value, "never") == 0)
  {
    return SOUND_FILTER_NEVER;
  }
  if (stricmp(value, "original") == 0)
  {
    return SOUND_FILTER_ORIGINAL;
  }
  if (stricmp(value, "always") == 0)
  {
    return SOUND_FILTER_ALWAYS;
  }
  return SOUND_FILTER_ORIGINAL;
}

static STR *cfgGetSoundFilterToString(sound_filters filter)
{
  switch (filter)
  {
    case SOUND_FILTER_NEVER:    return "never";
    case SOUND_FILTER_ORIGINAL: return "original";
    case SOUND_FILTER_ALWAYS:   return "always";
  }
  return "original";
}

static ULO cfgGetBufferLengthFromString(STR *value)
{
  ULO buffer_length = cfgGetULOFromString(value);

  if (buffer_length < 10)
  {
    return 10;
  }
  if (buffer_length > 80)
  {
    return 80;
  }
  return buffer_length;
}

static DISPLAYCLIP_MODE cfgGetClipModeFromString(STR *value)
{
  if (stricmp(value, "auto") == 0)
  {
    return DISPLAYCLIP_MODE::AUTOMATIC_CLIP;
  }
  if (stricmp(value, "fixed") == 0)
  {
    return DISPLAYCLIP_MODE::FIXED_CLIP;
  }
  return DISPLAYCLIP_MODE::AUTOMATIC_CLIP; // Default
}

static STR* cfgGetClipModeToString(DISPLAYCLIP_MODE clipmode)
{
  switch (clipmode)
  {
    case DISPLAYCLIP_MODE::AUTOMATIC_CLIP:  return "auto";
    case DISPLAYCLIP_MODE::FIXED_CLIP:      return "fixed";
  }
  return "fixed";
}

static DISPLAYSCALE cfgGetDisplayScaleFromString(STR *value)
{
  if (stricmp(value, "auto") == 0)
  {
    return DISPLAYSCALE_AUTO;
  }
  if (stricmp(value, "quadruple") == 0)
  {
    return DISPLAYSCALE_4X;
  }
  if (stricmp(value, "triple") == 0)
  {
    return DISPLAYSCALE_3X;
  }
  if (stricmp(value, "double") == 0)
  {
    return DISPLAYSCALE_2X;
  }
  if (stricmp(value, "single") == 0)
  {
    return DISPLAYSCALE_1X;
  }
  return DISPLAYSCALE_1X; // Default
}

static STR* cfgGetDisplayScaleToString(DISPLAYSCALE displayscale)
{
  switch (displayscale)
  {
    case DISPLAYSCALE_AUTO: return "auto";
    case DISPLAYSCALE_1X:   return "single";
    case DISPLAYSCALE_2X:   return "double";
    case DISPLAYSCALE_3X:   return "triple";
    case DISPLAYSCALE_4X:   return "quadruple";
  }
  return "single";
}

static DISPLAYDRIVER cfgGetDisplayDriverFromString(STR *value)
{
  if (stricmp(value, "directdraw") == 0)
  {
    return DISPLAYDRIVER_DIRECTDRAW;
  }
  if (stricmp(value, "direct3d11") == 0)
  {
    return DISPLAYDRIVER_DIRECT3D11;
  }
  return DISPLAYDRIVER_DIRECTDRAW; // Default
}

static STR *cfgGetDisplayDriverToString(DISPLAYDRIVER displaydriver)
{
  switch (displaydriver)
  {
    case DISPLAYDRIVER_DIRECTDRAW:
      return "directdraw";
    case DISPLAYDRIVER_DIRECT3D11:
      return "direct3d11";
  }
  return "directdraw";
}

static DISPLAYSCALE_STRATEGY cfgGetDisplayScaleStrategyFromString(STR *value)
{
  if (stricmp(value, "scanlines") == 0)
  {
    return DISPLAYSCALE_STRATEGY_SCANLINES;
  }
  if (stricmp(value, "solid") == 0)
  {
    return DISPLAYSCALE_STRATEGY_SOLID;
  }
  return DISPLAYSCALE_STRATEGY_SOLID; // Default
}

static STR* cfgGetDisplayScaleStrategyToString(DISPLAYSCALE_STRATEGY displayscalestrategy)
{
  switch (displayscalestrategy)
  {
    case DISPLAYSCALE_STRATEGY_SOLID: return "solid";
    case DISPLAYSCALE_STRATEGY_SCANLINES: return "scanlines";
  }
  return "solid";
}

static ULO cfgGetColorBitsFromString(STR *value)
{
  if ((stricmp(value, "8bit") == 0) ||
    (stricmp(value, "8") == 0))
  {
    return 16; // Fallback to 16 bits, 8 bit support is removed
  }
  if ((stricmp(value, "15bit") == 0) ||
    (stricmp(value, "15") == 0))
  {
    return 15;
  }
  if ((stricmp(value, "16bit") == 0) ||
    (stricmp(value, "16") == 0))
  {
    return 16;
  }
  if ((stricmp(value, "24bit") == 0) ||
    (stricmp(value, "24") == 0))
  {
    return 24;
  }
  if ((stricmp(value, "32bit") == 0) ||
    (stricmp(value, "32") == 0))
  {
    return 32;
  }
  return 16;
}

static STR *cfgGetColorBitsToString(ULO colorbits)
{
  switch (colorbits)
  {
    case 16: return "16bit";
    case 24: return "24bit";
    case 32: return "32bit";
  }
  return "8bit";
}

static bool cfgGetECSFromString(STR *value)
{
  if ((stricmp(value, "ocs") == 0) ||
    (stricmp(value, "0") == 0))
  {
    return false;
  }
  if ((stricmp(value, "ecs agnes") == 0) ||
    (stricmp(value, "ecs denise") == 0) ||
    (stricmp(value, "ecs") == 0) ||
    (stricmp(value, "aga") == 0) ||
    (stricmp(value, "2") == 0) ||
    (stricmp(value, "3") == 0) ||
    (stricmp(value, "4") == 0))
  {
    return true;
  }
  return false;
}

static const STR *cfgGetECSToString(bool chipset_ecs)
{
  return (chipset_ecs) ? "ecs" : "ocs";
}

void cfgSetConfigAppliedOnce(cfg *config, BOOLE bApplied) {
  config->m_config_applied_once = bApplied;
}

BOOLE cfgGetConfigAppliedOnce(cfg *config) {
  return config->m_config_applied_once;
}

void cfgSetConfigChangedSinceLastSave(cfg *config, BOOLE bChanged) {
  config->m_config_changed_since_save = bChanged;
}

BOOLE cfgGetConfigChangedSinceLastSave(cfg *config) {
  return config->m_config_changed_since_save;
}

static GRAPHICSEMULATIONMODE cfgGetGraphicsEmulationModeFromString(STR *value)
{
  if (stricmp(value, "lineexact") == 0)
  {
    return GRAPHICSEMULATIONMODE_LINEEXACT;
  }
  if (stricmp(value, "cycleexact") == 0)
  {
    return GRAPHICSEMULATIONMODE_CYCLEEXACT;
  }
  return GRAPHICSEMULATIONMODE_LINEEXACT;
}

static STR *cfgGetGraphicsEmulationModeToString(GRAPHICSEMULATIONMODE graphicsemulationmode)
{
  switch (graphicsemulationmode)
  {
    case GRAPHICSEMULATIONMODE_LINEEXACT:
      return "lineexact";
    case GRAPHICSEMULATIONMODE_CYCLEEXACT:
      return "cycleexact";
  }
  return "lineexact";
}

/*============================================================================*/
/* Command line option synopsis                                               */
/*============================================================================*/

void cfgSynopsis(cfg *config)
{
  fprintf(stderr, 
    "Synopsis: WinFellow.exe [-h] | [[-f configfile] | [-s option=value]]*\n\n"
    "Command-line options:\n"
    "-h              : Print this command-line symmary, then stop.\n"
    "-f configfile   : Specify configuration file to use.\n"
    "-s option=value : Set option to value. Legal options listed below.\n");
}


/*============================================================================*/
/* Set configuration option                                                   */
/* Returns TRUE if the option was recognized                                  */
/*============================================================================*/

BOOLE cfgSetOption(cfg *config, STR *optionstr)
{
  STR *option, *value;
  BOOLE result;

  value = strchr(optionstr, '=');
  result = (value != nullptr);
  if (result)
  {
    option = optionstr;
    *value++ = '\0';

    /* Standard configuration options */

    if (stricmp(option, "help") == 0)
    {
      cfgSynopsis(config);
      exit(EXIT_SUCCESS);
    }
    else if (stricmp(option, "autoconfig") == 0)
    {
      cfgSetUseAutoconfig(config, cfgGetBOOLEFromString(value));
    }
    else if (stricmp(option, "config_description") == 0)
    {
      cfgSetDescription(config, value);
    }
    else if (stricmp(option, "floppy0") == 0)
    {
      cfgSetDiskImage(config, 0, value);
    }
    else if (stricmp(option, "floppy1") == 0)
    {
      cfgSetDiskImage(config, 1, value);
    }
    else if (stricmp(option, "floppy2") == 0)
    {
      cfgSetDiskImage(config, 2, value);
    }
    else if (stricmp(option, "floppy3") == 0)
    {
      cfgSetDiskImage(config, 3, value);
    }
    else if (stricmp(option, "fellow.last_used_disk_dir") == 0)
    {
      cfgSetLastUsedDiskDir(config, value);
    }
    else if ((stricmp(option, "fellow.floppy0_enabled") == 0) ||
      (stricmp(option, "floppy0_enabled") == 0))
    {
      cfgSetDiskEnabled(config, 0, cfgGetBOOLEFromString(value));
    }
    else if ((stricmp(option, "fellow.floppy1_enabled") == 0) ||
      (stricmp(option, "floppy1_enabled") == 0))
    {      
      cfgSetDiskEnabled(config, 1, cfgGetBOOLEFromString(value));
    }
    else if ((stricmp(option, "fellow.floppy2_enabled") == 0) ||
      (stricmp(option, "floppy2_enabled") == 0))
    {
      cfgSetDiskEnabled(config, 2, cfgGetBOOLEFromString(value));
    }
    else if ((stricmp(option, "fellow.floppy3_enabled") == 0) ||
      (stricmp(option, "floppy3_enabled") == 0))
    {
      cfgSetDiskEnabled(config, 3, cfgGetBOOLEFromString(value));
    }
    else if ((stricmp(option, "fellow.floppy_fast_dma") == 0) ||
      (stricmp(option, "floppy_fast_dma") == 0))
    {
      cfgSetDiskFast(config, cfgGetBOOLEFromString(value));
    }
    else if ((stricmp(option, "fellow.floppy0_readonly") == 0) ||
      (stricmp(option, "floppy0_readonly") == 0))
    {
      cfgSetDiskReadOnly(config, 0, cfgGetBOOLEFromString(value));
    }
    else if ((stricmp(option, "fellow.floppy1_readonly") == 0) ||
      (stricmp(option, "floppy1_readonly") == 0))
    {      
      cfgSetDiskReadOnly(config, 1, cfgGetBOOLEFromString(value));
    }
    else if ((stricmp(option, "fellow.floppy2_readonly") == 0) ||
      (stricmp(option, "floppy2_readonly") == 0))
    {
      cfgSetDiskReadOnly(config, 2, cfgGetBOOLEFromString(value));
    }
    else if ((stricmp(option, "fellow.floppy3_readonly") == 0) ||
      (stricmp(option, "floppy3_readonly") == 0))
    {
      cfgSetDiskReadOnly(config, 3, cfgGetBOOLEFromString(value));
    }
    else if ((stricmp(option, "fellow.floppy_fast_dma") == 0) ||
      (stricmp(option, "floppy_fast_dma") == 0))
    {
      cfgSetDiskFast(config, cfgGetBOOLEFromString(value));
    }
    else if (stricmp(option, "joyport0") == 0)
    {
      cfgSetGameport(config, 0, cfgGetGameportFromString(value));
    }
    else if (stricmp(option, "joyport1") == 0)
    {
      cfgSetGameport(config, 1, cfgGetGameportFromString(value));
    }
    else if (stricmp(option, "use_gui") == 0)
    {
      cfgSetUseGUI(config, cfgGetBOOLEFromString(value));
    }
    else if (stricmp(option, "cpu_speed") == 0)
    {
      cfgSetCPUSpeed(config, cfgGetCPUSpeedFromString(value));
    }
    else if (stricmp(option, "cpu_type") == 0)
    {
      cfgSetCPUType(config, cfgGetCPUTypeFromString(value));
    }
    else if (stricmp(option, "sound_output") == 0)
    {
      cfgSetSoundEmulation(config, cfgGetSoundEmulationFromString(value));
    }
    else if (stricmp(option, "sound_channels") == 0)
    {
      cfgSetSoundStereo(config, cfgGetSoundStereoFromString(value));
    }
    else if (stricmp(option, "sound_bits") == 0)
    {
      cfgSetSound16Bits(config, cfgGetSound16BitsFromString(value));
    }
    else if (stricmp(option, "sound_frequency") == 0)
    {
      cfgSetSoundRate(config, cfgGetSoundRateFromString(value));
    }
    else if (stricmp(option, "sound_volume") == 0)
    {
      cfgSetSoundVolume(config, cfgGetULOFromString(value));
    }
    else if ((stricmp(option, "fellow.sound_wav") == 0) ||
      (stricmp(option, "sound_wav") == 0))
    {
      cfgSetSoundWAVDump(config, cfgGetBOOLEFromString(value));
    }
    else if ((stricmp(option, "fellow.sound_filter") == 0) ||
      (stricmp(option, "sound_filter") == 0))
    {
      cfgSetSoundFilter(config, cfgGetSoundFilterFromString(value));
    }
    else if (stricmp(option, "sound_notification") == 0)
    {
      cfgSetSoundNotification(config, cfgGetSoundNotificationFromString(value));
    }
    else if (stricmp(option, "sound_buffer_length") == 0)
    {
      cfgSetSoundBufferLength(config, cfgGetBufferLengthFromString(value));
    }
    else if (stricmp(option, "chipmem_size") == 0)
    {
      cfgSetChipSize(config, cfgGetULOFromString(value)*262144);
    }
    else if (stricmp(option, "fastmem_size") == 0)
    {
      cfgSetFastSize(config, cfgGetULOFromString(value)*1048576);
    }
    else if (stricmp(option, "bogomem_size") == 0)
    {
      cfgSetBogoSize(config, cfgGetULOFromString(value)*262144);
    }
    else if (stricmp(option, "kickstart_rom_file") == 0)
    {
      cfgSetKickImage(config, value);
    }
    else if (stricmp(option, "kickstart_rom_file_ext") == 0)
    {
      cfgSetKickImageExtended(config, value);
    }
    else if (stricmp(option, "kickstart_rom_description") == 0)
    {
      cfgSetKickDescription(config, value);
    }
    else if (stricmp(option, "kickstart_rom_crc32") == 0)
    {
      ULO crc32;
      sscanf(value,"%lX", &crc32); 
      cfgSetKickCRC32(config, crc32);
    }
    else if (stricmp(option, "kickstart_key_file") == 0)
    {
      cfgSetKey(config, value);
    }
    else if (stricmp(option, "gfx_immediate_blits") == 0)
    {
      cfgSetBlitterFast(config, cfgGetBOOLEFromString(value));
    }
    else if (stricmp(option, "gfx_chipset") == 0)
    {
      cfgSetECS(config, cfgGetECSFromString(value));
    }
    else if (stricmp(option, "gfx_width") == 0)
    {
      cfgSetScreenWidth(config, cfgGetULOFromString(value));
    }
    else if (stricmp(option, "gfx_height") == 0)
    {
      cfgSetScreenHeight(config, cfgGetULOFromString(value));
    }
    else if ((stricmp(option, "fellow.gfx_refresh") == 0) ||
      (stricmp(option, "gfx_refresh") == 0))
    {
      cfgSetScreenRefresh(config, cfgGetULOFromString(value));
    }
    else if (stricmp(option, "gfx_fullscreen_amiga") == 0)
    {
      cfgSetScreenWindowed(config, !cfgGetBOOLEFromString(value));
    }
    else if (stricmp(option, "use_multiple_graphical_buffers") == 0)
    {
      cfgSetUseMultipleGraphicalBuffers(config, cfgGetBOOLEFromString(value));
    }
    else if (stricmp(option, "gfx_driver") == 0)
    {
      cfgSetDisplayDriver(config, cfgGetDisplayDriverFromString(value));
    }
    else if (stricmp(option, "gfx_emulation_mode") == 0)
    {
      cfgSetGraphicsEmulationMode(config, cfgGetGraphicsEmulationModeFromString(value));
    }
    else if (stricmp(option, "gfx_colour_mode") == 0)
    {
      cfgSetScreenColorBits(config, cfgGetColorBitsFromString(value));
    }
    else if (stricmp(option, "show_leds") == 0)
    {
      cfgSetScreenDrawLEDs(config, cfgGetboolFromString(value));
    }
    else if (stricmp(option, "gfx_clip_mode") == 0)
    {
      cfgSetClipMode(config, cfgGetClipModeFromString(value));
    }
    else if (stricmp(option, "gfx_clip_left") == 0)
    {
      cfgSetClipLeft(config, cfgGetULOFromString(value));
    }
    else if (stricmp(option, "gfx_clip_top") == 0)
    {
      cfgSetClipTop(config, cfgGetULOFromString(value));
    }
    else if (stricmp(option, "gfx_clip_right") == 0)
    {
      cfgSetClipRight(config, cfgGetULOFromString(value));
    }
    else if (stricmp(option, "gfx_clip_bottom") == 0)
    {
      cfgSetClipBottom(config, cfgGetULOFromString(value));
    }
    else if (stricmp(option, "gfx_display_scale") == 0)
    {
      cfgSetDisplayScale(config, cfgGetDisplayScaleFromString(value));
    }
    else if (stricmp(option, "gfx_display_scale_strategy") == 0)
    {
      cfgSetDisplayScaleStrategy(config, cfgGetDisplayScaleStrategyFromString(value));
    }
    else if (stricmp(option, "gfx_framerate") == 0)
    {
      cfgSetFrameskipRatio(config, cfgGetULOFromString(value));
    }
    else if ((stricmp(option, "fellow.gfx_deinterlace") == 0) ||
      (stricmp(option, "gfx_deinterlace") == 0))
    {
      cfgSetDeinterlace(config, cfgGetboolFromString(value));
    }
    else if ((stricmp(option, "fellow.measure_speed") == 0) ||
      (stricmp(option, "measure_speed") == 0))
    {
      cfgSetMeasureSpeed(config, cfgGetboolFromString(value));
    }
    else if (stricmp(option, "rtc") == 0)
    {
      cfgSetRtc(config, cfgGetboolFromString(value));
    }
    else if (stricmp(option, "hardfile") == 0)
    {
      STR *curpos = value;
      STR *nextpos;
      cfg_hardfile hf;

      if ((nextpos = strchr(curpos, ',')) == nullptr)
      {
  	return FALSE; /* Access */
      }
      *nextpos = '\0';
      if (stricmp(curpos, "ro") == 0)
      {
	hf.readonly = TRUE;
      }
      else if (stricmp(curpos, "rw") == 0)
      {
	hf.readonly = FALSE;
      }
      else
      {
	return FALSE;
      }
      curpos = nextpos + 1;
      if ((nextpos = strchr(curpos, ',')) == nullptr)
      {
	return FALSE; /* Secs */
      }
      *nextpos = '\0';
      hf.sectorspertrack = atoi(curpos);
      curpos = nextpos + 1;
      if ((nextpos = strchr(curpos, ',')) == nullptr)
      {
	return FALSE; /* Surfaces */
      }
      *nextpos = '\0';
      hf.surfaces = atoi(curpos);
      curpos = nextpos + 1;
      if ((nextpos = strchr(curpos, ',')) == nullptr)
      {
	return FALSE; /* Reserved */
      }
      *nextpos = '\0';
      hf.reservedblocks = atoi(curpos);
      curpos = nextpos + 1;
      if ((nextpos = strchr(curpos, ',')) == nullptr)
      {
	return FALSE; /* Blocksize */
      }
      *nextpos = '\0';
      hf.bytespersector = atoi(curpos);
      curpos = nextpos + 1;
      if ((nextpos = strchr(curpos, ',')) != nullptr)
      {
	return FALSE; /* Filename */
      }
      strncpy(hf.filename, curpos, CFG_FILENAME_LENGTH);
      cfgHardfileAdd(config, &hf);
    }
    else if (stricmp(option, "filesystem") == 0)
    {
      STR *curpos = value;
      STR *nextpos;
      cfg_filesys fs;

      if ((nextpos = strchr(curpos, ',')) == nullptr)
      {
	return FALSE;    /* Access */
      }
      *nextpos = '\0';
      if (stricmp(curpos, "ro") == 0)
      {
	fs.readonly = TRUE;
      }
      else if (stricmp(curpos, "rw") == 0)
      {
	fs.readonly = FALSE;
      }
      else
      {
	return FALSE;
      }
      curpos = nextpos + 1;
      if ((nextpos = strchr(curpos, ':')) == nullptr)
      {
	return FALSE;   /* Volname */
      }
      *nextpos = '\0';
      strncpy(fs.volumename, curpos, 64);
      curpos = nextpos + 1;
      strncpy(fs.rootpath, curpos, CFG_FILENAME_LENGTH);          /* Rootpath */
      cfgFilesystemAdd(config, &fs);
    }
    else if ((stricmp(option, "fellow.map_drives") == 0) ||
      (stricmp(option, "fellow.win32.map_drives") == 0) ||
      (stricmp(option, "win32.map_drives") == 0) ||
      (stricmp(option, "map_drives") == 0))
    {
      cfgSetFilesystemAutomountDrives(config, cfgGetBOOLEFromString(value));
    }
    else if (stricmp(option, "gxfcard_size") == 0) {           /* Unsupported */
    }
    else if (stricmp(option, "use_debugger") == 0) {           /* Unsupported */
    }
    else if (stricmp(option, "log_illegal_mem") == 0) {        /* Unsupported */
    }
    else if (stricmp(option, "gfx_correct_aspect") == 0) {     /* Unsupported */
    }
    else if (stricmp(option, "gfx_center_vertical") == 0) {    /* Unsupported */
    }
    else if (stricmp(option, "gfx_center_horizontal") == 0) {  /* Unsupported */
    }
    else if (stricmp(option, "gfx_fullscreen_picasso") == 0) { /* Unsupported */
    }
    else if (stricmp(option, "z3mem_size") == 0) {             /* Unsupported */
    }
    else if (stricmp(option, "a3000mem_size") == 0) {          /* Unsupported */
    }
    else if (stricmp(option, "sound_min_buff") == 0) {         /* Unsupported */
    }
    else if (stricmp(option, "sound_max_buff") == 0) {         /* Unsupported */
    }
    else if (stricmp(option, "accuracy") == 0) {               /* Unsupported */
    }
    else if (stricmp(option, "gfx_32bit_blits") == 0) {        /* Unsupported */
    }
    else if (stricmp(option, "gfx_test_speed") == 0) {         /* Unsupported */
    }
    else if (stricmp(option, "gfxlib_replacement") == 0) {     /* Unsupported */
    }
    else if (stricmp(option, "bsdsocket_emu") == 0) {          /* Unsupported */
    }
    else if (stricmp(option, "parallel_on_demand") == 0) {     /* Unsupported */
    }
    else if (stricmp(option, "serial_on_demand") == 0) {       /* Unsupported */
    }
    else if (stricmp(option, "kickshifter") == 0) {            /* Unsupported */
    }
    else if (stricmp(option, "enforcer") == 0) {               /* Unsupported */
    }
    else if (stricmp(option, "cpu_compatible") == 0) {      /* Static support */
    }
    else if (stricmp(option, "cpu_24bit_addressing") == 0) {     /* Redundant */
    }
    else
#ifdef RETRO_PLATFORM
      if(RP.GetHeadlessMode())
      {
	if (stricmp(option, "gfx_offset_left") == 0)
	{
	  RP.SetClippingOffsetLeft(cfgGetULOFromString(value));
	}
	else if (stricmp(option, "gfx_offset_top") == 0)
	{
	  RP.SetClippingOffsetTop(cfgGetULOFromString(value));
	}
      }
      else
#endif
	result = FALSE;
  }
  else
  {
    result = FALSE;
  }
  return result;
}

/*============================================================================*/
/* Save options supported by this class to file                               */
/*============================================================================*/

BOOLE cfgSaveOptions(cfg *config, FILE *cfgfile)
{
  fprintf(cfgfile, "config_description=%s\n", cfgGetDescription(config));
  fprintf(cfgfile, "autoconfig=%s\n", cfgGetBOOLEToString(cfgGetUseAutoconfig(config)));
  for (ULO i = 0; i < 4; i++)
  {
    fprintf(cfgfile, "floppy%u=%s\n", i, cfgGetDiskImage(config, i));
    fprintf(cfgfile, "fellow.floppy%u_enabled=%s\n", i, cfgGetBOOLEToString(cfgGetDiskEnabled(config, i)));
    fprintf(cfgfile, "fellow.floppy%u_readonly=%s\n", i, cfgGetBOOLEToString(cfgGetDiskReadOnly(config, i)));
  }
  fprintf(cfgfile, "fellow.floppy_fast_dma=%s\n", cfgGetBOOLEToString(cfgGetDiskFast(config)));
  fprintf(cfgfile, "fellow.last_used_disk_dir=%s\n", cfgGetLastUsedDiskDir(config));
  fprintf(cfgfile, "joyport0=%s\n", cfgGetGameportToString(cfgGetGameport(config, 0)));
  fprintf(cfgfile, "joyport1=%s\n", cfgGetGameportToString(cfgGetGameport(config, 1)));
  fprintf(cfgfile, "usegui=%s\n", cfgGetBOOLEToString(cfgGetUseGUI(config)));
  fprintf(cfgfile, "cpu_speed=%u\n", cfgGetCPUSpeed(config));
  fprintf(cfgfile, "cpu_compatible=%s\n", cfgGetBOOLEToString(TRUE));
  fprintf(cfgfile, "cpu_type=%s\n", cfgGetCPUTypeToString(cfgGetCPUType(config)));
  fprintf(cfgfile, "sound_output=%s\n", cfgGetSoundEmulationToString(cfgGetSoundEmulation(config)));
  fprintf(cfgfile, "sound_channels=%s\n", cfgGetSoundStereoToString(cfgGetSoundStereo(config)));
  fprintf(cfgfile, "sound_bits=%s\n", cfgGetSound16BitsToString(cfgGetSound16Bits(config)));
  fprintf(cfgfile, "sound_frequency=%s\n", cfgGetSoundRateToString(cfgGetSoundRate(config)));
  fprintf(cfgfile, "sound_volume=%u\n", cfgGetSoundVolume(config));
  fprintf(cfgfile, "fellow.sound_wav=%s\n", cfgGetBOOLEToString(cfgGetSoundWAVDump(config)));
  fprintf(cfgfile, "fellow.sound_filter=%s\n", cfgGetSoundFilterToString(cfgGetSoundFilter(config)));
  fprintf(cfgfile, "sound_notification=%s\n", cfgGetSoundNotificationToString(cfgGetSoundNotification(config)));
  fprintf(cfgfile, "sound_buffer_length=%u\n", cfgGetSoundBufferLength(config));
  fprintf(cfgfile, "chipmem_size=%u\n", cfgGetChipSize(config) / 262144);
  fprintf(cfgfile, "fastmem_size=%u\n", cfgGetFastSize(config) / 1048576);
  fprintf(cfgfile, "bogomem_size=%u\n", cfgGetBogoSize(config) / 262144);
  fprintf(cfgfile, "kickstart_rom_file=%s\n", cfgGetKickImage(config));
  fprintf(cfgfile, "kickstart_rom_file_ext=%s\n", cfgGetKickImageExtended(config));
  if(strcmp(cfgGetKickDescription(config), "") != 0)
  {
    fprintf(cfgfile, "kickstart_rom_description=%s\n", cfgGetKickDescription(config));
  }
  if(cfgGetKickCRC32(config) != 0)
  {
    fprintf(cfgfile, "kickstart_rom_crc32=%X\n", cfgGetKickCRC32(config));
  }
  fprintf(cfgfile, "kickstart_key_file=%s\n", cfgGetKey(config));
  fprintf(cfgfile, "gfx_immediate_blits=%s\n", cfgGetBOOLEToString(cfgGetBlitterFast(config)));
  fprintf(cfgfile, "gfx_chipset=%s\n", cfgGetECSToString(cfgGetECS(config)));
  fprintf(cfgfile, "gfx_width=%u\n", cfgGetScreenWidth(config));
  fprintf(cfgfile, "gfx_height=%u\n", cfgGetScreenHeight(config));
  fprintf(cfgfile, "gfx_fullscreen_amiga=%s\n", cfgGetBOOLEToString(!cfgGetScreenWindowed(config)));
  fprintf(cfgfile, "use_multiple_graphical_buffers=%s\n", cfgGetBOOLEToString(cfgGetUseMultipleGraphicalBuffers(config)));
  fprintf(cfgfile, "gfx_driver=%s\n", cfgGetDisplayDriverToString(cfgGetDisplayDriver(config)));
  // fprintf(cfgfile, "gfx_emulation_mode=%s\n"), cfgGetGraphicsEmulationModeToString(cfgGetGraphicsEmulationMode(config)));
  fprintf(cfgfile, "fellow.gfx_refresh=%u\n", cfgGetScreenRefresh(config));
  fprintf(cfgfile, "gfx_colour_mode=%s\n", cfgGetColorBitsToString(cfgGetScreenColorBits(config)));
  fprintf(cfgfile, "gfx_clip_mode=%s\n", cfgGetClipModeToString(cfgGetClipMode(config)));
  fprintf(cfgfile, "gfx_clip_left=%u\n", cfgGetClipLeft(config));
  fprintf(cfgfile, "gfx_clip_top=%u\n", cfgGetClipTop(config));
  fprintf(cfgfile, "gfx_clip_right=%u\n", cfgGetClipRight(config));
  fprintf(cfgfile, "gfx_clip_bottom=%u\n", cfgGetClipBottom(config));
  fprintf(cfgfile, "gfx_display_scale=%s\n", cfgGetDisplayScaleToString(cfgGetDisplayScale(config)));
  fprintf(cfgfile, "gfx_display_scale_strategy=%s\n", cfgGetDisplayScaleStrategyToString(cfgGetDisplayScaleStrategy(config)));
  fprintf(cfgfile, "gfx_framerate=%u\n", cfgGetFrameskipRatio(config));
  fprintf(cfgfile, "show_leds=%s\n", cfgGetboolToString(cfgGetScreenDrawLEDs(config)));
  fprintf(cfgfile, "fellow.gfx_deinterlace=%s\n", cfgGetBOOLEToString(cfgGetDeinterlace(config)));
  fprintf(cfgfile, "fellow.measure_speed=%s\n", cfgGetboolToString(cfgGetMeasureSpeed(config)));
  fprintf(cfgfile, "rtc=%s\n", cfgGetboolToString(cfgGetRtc(config)));
  fprintf(cfgfile, "win32.map_drives=%s\n", cfgGetBOOLEToString(cfgGetFilesystemAutomountDrives(config)));
  for (ULO i = 0; i < cfgGetHardfileCount(config); i++)
  {
    cfg_hardfile hf = cfgGetHardfile(config, i);
    fprintf(cfgfile, "hardfile=%s,%u,%u,%u,%u,%s\n", 
      (hf.readonly) ? "ro" : "rw", 
      hf.sectorspertrack,
      hf.surfaces,
      hf.reservedblocks,
      hf.bytespersector,
      hf.filename);
  }
  for (ULO i = 0; i < cfgGetFilesystemCount(config); i++)
  {
    cfg_filesys fs = cfgGetFilesystem(config, i);
    fprintf(cfgfile, "filesystem=%s,%s:%s\n", (fs.readonly) ? "ro" : "rw", fs.volumename, fs.rootpath);
  }
  return TRUE;
}


/*============================================================================*/
/* Remove unwanted newline chars on the end of a string                       */
/*============================================================================*/

static void cfgStripTrailingNewlines(STR *line)
{
  size_t length = strlen(line);
  while ((length > 0) && 
    ((line[length - 1] == '\n') || (line[length - 1] == '\r')))
  {
    line[--length] = '\0';
  }
}


/*============================================================================*/
/* Load configuration from file                                               */
/*============================================================================*/

static BOOLE cfgLoadFromFile(cfg *config, FILE *cfgfile)
{
  char line[256];
  while (!feof(cfgfile))
  {
    fgets(line, 256, cfgfile);
    cfgStripTrailingNewlines(line);
    cfgSetOption(config, line);
  }
  cfgSetConfigAppliedOnce(config, true);
  return TRUE;
}

BOOLE cfgLoadFromFilename(cfg *config, const STR *filename, const bool bIsPreset)
{
  FILE *cfgfile;
  BOOLE result;
  STR newfilename[CFG_FILENAME_LENGTH];

  fileopsResolveVariables(filename, newfilename);

  if(!bIsPreset) {
    fellowAddLog("cfg: loading configuration filename %s...\n", filename);

    // remove existing hardfiles
    cfgHardfilesFree(config);
    cfgFilesystemsFree(config);
  }

  cfgfile = fopen(newfilename, "r");
  result = (cfgfile != nullptr);
  if (result)
  {
    result = cfgLoadFromFile(config, cfgfile);
    fclose(cfgfile);
  }
  return result;
}


/*============================================================================*/
/* Save configuration to file                                                 */
/*============================================================================*/

static BOOLE cfgSaveToFile(cfg *config, FILE *cfgfile)
{
  return cfgSaveOptions(config, cfgfile);
}

BOOLE cfgSaveToFilename(cfg *config, STR *filename)
{
  FILE *cfgfile;
  BOOLE result;

  cfgfile = fopen(filename, "w");
  result = (cfgfile != nullptr);
  if (result)
  {
    result = cfgSaveToFile(config, cfgfile);
    fclose(cfgfile);
  }
  return result;
}

/*============================================================================*/
/* Parse command line                                                         */
/*============================================================================*/

static BOOLE cfgParseCommandLine(cfg *config, int argc, char *argv[])
{
  int i;

  fellowAddLog("cfg: list of commandline parameters: ");
  for(i=1; i < argc; i++)
  {
    fellowAddLog("%s ", argv[i]);
  }
  fellowAddLog("\n");

  i = 1;
  while (i < argc)
  {
    // fellowAddLog("cfg: parsing parameter: %s\n", argv[i]);
    if (stricmp(argv[i], "-h") == 0)
    { /* Command line synposis */
      cfgSynopsis(config);
      return FALSE;
    }
#ifdef RETRO_PLATFORM
    if (stricmp(argv[i], "-rphost") == 0)
    {
      i++;
      if (i < argc)
      {
        RP.SetHeadlessMode(true); 
        RP.SetHostID(argv[i]);
      }
      i++;
    }
    else if (stricmp(argv[i], "-datapath") == 0)
    {
      i++;
      if (i < argc)
      {
        fellowAddLog("cfg: RetroPlatform data path: %s\n", argv[i]);
      }
      i++;
    }
    else if (stricmp(argv[i], "-rpescapekey") == 0)
    {
      i++;
      if (i < argc)
      {      
        ULO lEscapeKey = atoi(argv[i]);
        lEscapeKey = kbddrv_DIK_to_symbol[lEscapeKey];
        RP.SetEscapeKey(lEscapeKey);
      }
      i++;
    }
    else if (stricmp(argv[i], "-rpescapeholdtime") == 0)
    {
      i++;
      if (i < argc)
      {   
        RP.SetEscapeKeyHoldTime(atoi(argv[i]));
      }
      i++;
    }
    else if (stricmp(argv[i], "-rpscreenmode") == 0)
    {
      i++;
      if (i < argc)
      {
        RP.SetScreenMode(argv[i]);
      }
      i++;
    }
    else if (stricmp(argv[i], "-rpdpiawareness") == 0)
    {
      i++;
      if (i < argc)
      {
        wguiSetProcessDPIAwareness(argv[i]);
      }
      i++;
    }
    /* else if (stricmp(argv[i], "-logflush") == 0)
    {
    i++;
    fellowAddLog("cfg: RetroPlatform log flush set.\n");
    } */
#endif
    else if (stricmp(argv[i], "-f") == 0)
    { /* Load configuration file */
      i++;
      if (i < argc)
      {
        fellowAddLog("cfg: configuration file: %s\n", argv[i]);
        if(!cfgLoadFromFilename(config, argv[i], false))
        {
          fellowAddLog("cfg: ERROR using -f option, failed reading configuration file %s\n", argv[i]);
        }
        i++;
      }
      else
      {
        fellowAddLog("cfg: ERROR using -f option, please supply a filename\n");
      }
    }
    else if (stricmp(argv[i], "-s") == 0)
    { /* Configuration option */
      i++;
      if (i < argc)
      {
        if (!cfgSetOption(config, argv[i]))
          fellowAddLog("cfg: -s option, unrecognized setting %s\n", argv[i]);
        i++;
      }
      else
      {
        fellowAddLog("cfg: -s option, please supply a configuration setting\n");
      }
    }
    else
    { /* unrecognized configuration option */
      fellowAddLog("cfg: parameter %s not recognized.\n", argv[i]);
      i++;
    }
  }
  return TRUE;
}


/*============================================================================*/
/* struct cfgManager property access functions                                */
/*============================================================================*/

void cfgManagerSetCurrentConfig(cfgManager *configmanager, cfg *currentconfig)
{
  configmanager->m_currentconfig = currentconfig;
}

cfg *cfgManagerGetCurrentConfig(cfgManager *configmanager)
{
  return configmanager->m_currentconfig;
}

/*============================================================================*/
/* get copy of current configuration                                          */
/* the copy is intended to be a completely separate structure, that can be    */
/* freed independently; all targets behind pointers in the config struct must */
/* be cloned and allocated independently.                                     */
/*============================================================================*/
cfg *cfgManagerGetCopyOfCurrentConfig(cfgManager *configmanager)
{
  cfg *newconfig = static_cast<cfg *>(malloc(sizeof(cfg)));
  memcpy(newconfig, configmanager->m_currentconfig, sizeof(cfg));
  newconfig->m_filesystems = listCopy(configmanager->m_currentconfig->m_filesystems, sizeof(cfg_filesys ));
  newconfig->m_hardfiles   = listCopy(configmanager->m_currentconfig->m_hardfiles,   sizeof(cfg_hardfile));

  return newconfig;
}

void cfgManagerUseDefaultConfiguration(cfgManager *configmanager)
{
  configmanager->m_currentconfig = configmanager->m_currentconfig;
}


/*============================================================================*/
/* struct cfgManager utility functions                                        */
/* Returns TRUE if reset is needed to activate the config changes             */
/*============================================================================*/

BOOLE cfgManagerConfigurationActivate(cfgManager *configmanager)
{
  cfg *config = cfgManagerGetCurrentConfig(&cfg_manager);
  ULO i;
  BOOLE needreset = FALSE;


  /*==========================================================================*/
  /* Floppy configuration                                                     */
  /*==========================================================================*/

  for (i = 0; i < 4; i++)
  {
    floppySetEnabled(i, cfgGetDiskEnabled(config, i));
    floppySetDiskImage(i, cfgGetDiskImage(config, i));
    if(!floppyIsWriteProtected(i))  /* IPF images are marked read-only by the floppy module itself */
    {
      floppySetReadOnly(i, cfgGetDiskReadOnly(config, i));
    }
  }
  floppySetFastDMA(cfgGetDiskFast(config));


  /*==========================================================================*/
  /* Memory configuration                                                     */
  /*==========================================================================*/

  needreset |= memorySetUseAutoconfig(cfgGetUseAutoconfig(config));
  needreset |= memorySetChipSize(cfgGetChipSize(config));
  needreset |= memorySetFastSize(cfgGetFastSize(config));
  needreset |= memorySetSlowSize(cfgGetBogoSize(config));
  memorySetKey(cfgGetKey(config));
  needreset |= memorySetKickImage(cfgGetKickImage(config));
  needreset |= memorySetKickImageExtended(cfgGetKickImageExtended(config));
  needreset |= memorySetAddress32Bit(cfgGetAddress32Bit(config));
  needreset |= rtcSetEnabled(cfgGetRtc(config)) == true;


  /*==========================================================================*/
  /* Screen configuration                                                     */
  /*==========================================================================*/

  drawSetLEDsEnabled(cfgGetScreenDrawLEDs(config));
  drawSetFPSCounterEnabled(cfgGetMeasureSpeed(config));
  drawSetFrameskipRatio(cfgGetFrameskipRatio(config));
  drawSetClipMode(cfgGetClipMode(config));
  drawSetClipLeft(cfgGetClipLeft(config));
  drawSetClipTop(cfgGetClipTop(config));
  drawSetClipRight(cfgGetClipRight(config));
  drawSetClipBottom(cfgGetClipBottom(config));
  drawSetDisplayScale(cfgGetDisplayScale(config));
  drawSetDisplayScaleStrategy(cfgGetDisplayScaleStrategy(config));
  drawSetAllowMultipleBuffers(cfgGetUseMultipleGraphicalBuffers(config));
  drawSetDeinterlace(cfgGetDeinterlace(config));
  drawSetDisplayDriver(cfgGetDisplayDriver(config));
  drawSetGraphicsEmulationMode(cfgGetGraphicsEmulationMode(config));
  drawSetMode(cfgGetScreenWidth(config),
    cfgGetScreenHeight(config),
    cfgGetScreenColorBits(config),
    cfgGetScreenRefresh(config),
    cfgGetScreenWindowed(config));


  /*==========================================================================*/
  /* Sound configuration                                                      */
  /*==========================================================================*/

  soundSetEmulation(cfgGetSoundEmulation(config));
  soundSetRate(cfgGetSoundRate(config));
  soundSetStereo(cfgGetSoundStereo(config));
  soundSet16Bits(cfgGetSound16Bits(config));
  soundSetFilter(cfgGetSoundFilter(config));
  soundSetVolume(cfgGetSoundVolume(config));
  soundSetWAVDump(cfgGetSoundWAVDump(config));
  soundSetNotification(cfgGetSoundNotification(config));
  soundSetBufferLength(cfgGetSoundBufferLength(config));


  /*==========================================================================*/
  /* CPU configuration                                                        */
  /*==========================================================================*/

  needreset |= cpuIntegrationSetModel(cfgGetCPUType(config));
  cpuIntegrationSetSpeed(cfgGetCPUSpeed(config));


  /*==========================================================================*/
  /* Custom chipset configuration                                             */
  /*==========================================================================*/

  blitterSetFast(cfgGetBlitterFast(config));
  needreset |= chipsetSetECS(cfgGetECS(config));


  /*==========================================================================*/
  /* Hardfile configuration                                                   */
  /*==========================================================================*/

  if (cfgGetUseAutoconfig(config) != fhfileGetEnabled())
  {
    needreset = TRUE;
    fhfileClear();
    fhfileSetEnabled(cfgGetUseAutoconfig(config));
  }
  if (fhfileGetEnabled())
  {
    for (i = 0; i < cfgGetHardfileCount(config); i++)
    {
      cfg_hardfile hardfile;
      fhfile_dev fhardfile;
      hardfile = cfgGetHardfile(config, i);
      fhardfile.bytespersector_original = hardfile.bytespersector;
      fhardfile.readonly_original = hardfile.readonly;
      fhardfile.reservedblocks_original = hardfile.reservedblocks;
      fhardfile.sectorspertrack = hardfile.sectorspertrack;
      fhardfile.surfaces = hardfile.surfaces;
      strncpy(fhardfile.filename, hardfile.filename, CFG_FILENAME_LENGTH);
      if (!fhfileCompareHardfile(fhardfile, i))
      {
	needreset = TRUE;
	fhfileSetHardfile(fhardfile, i);
      }
    }
    for (i = cfgGetHardfileCount(config); i < FHFILE_MAX_DEVICES; i++)
    {
      needreset |= fhfileRemoveHardfile(i);
    }
  }


  /*==========================================================================*/
  /* Filesystem configuration                                                 */
  /*==========================================================================*/

  if ((cfgGetUseAutoconfig(config) != ffilesysGetEnabled()) ||
    (cfgGetFilesystemAutomountDrives(config) != ffilesysGetAutomountDrives()))
  {
    needreset = TRUE;
    ffilesysClear();
    ffilesysSetEnabled(cfgGetUseAutoconfig(config));
  }

  if (ffilesysGetEnabled())
  {
    for (i = 0; i < cfgGetFilesystemCount(config); i++)
    {
      cfg_filesys filesys;
      ffilesys_dev ffilesys;
      filesys = cfgGetFilesystem(config, i);
      strncpy(ffilesys.volumename, filesys.volumename, FFILESYS_MAX_VOLUMENAME);
      strncpy(ffilesys.rootpath, filesys.rootpath, CFG_FILENAME_LENGTH);
      ffilesys.readonly = filesys.readonly;
      ffilesys.status = FFILESYS_INSERTED;
      if (!ffilesysCompareFilesys(ffilesys, i))
      {
	needreset = TRUE;
	ffilesysSetFilesys(ffilesys, i);
      }
    }
    for (i = cfgGetFilesystemCount(config); i < FFILESYS_MAX_DEVICES; i++)
    {
      needreset |= ffilesysRemoveFilesys(i);
    }
    ffilesysSetAutomountDrives(cfgGetFilesystemAutomountDrives(config));
  }

  /*==========================================================================*/
  /* Game port configuration                                                  */
  /*==========================================================================*/

  gameportSetInput(0, cfgGetGameport(config, 0));
  gameportSetInput(1, cfgGetGameport(config, 1));


  /*==========================================================================*/
  /* GUI configuration                                                        */
  /*==========================================================================*/

  fellowSetUseGUI(cfgGetUseGUI(config));
  return needreset;
}

cfg *cfgManagerGetNewConfig(cfgManager *configmanager)
{
  cfg *config = static_cast<cfg *>(malloc(sizeof(cfg)));
  config->m_hardfiles = nullptr;
  config->m_filesystems = nullptr;
  cfgSetDefaults(config);
  return config;
}

void cfgManagerFreeConfig(cfgManager *configmanager, cfg *config)
{
  cfgSetDefaults(config);
  free(config);
}

void cfgManagerStartup(cfgManager *configmanager, int argc, char *argv[])
{
  cfg *config = cfgManagerGetNewConfig(configmanager);
  cfgParseCommandLine(config, argc, argv);
#ifdef RETRO_PLATFORM
  if(!RP.GetHeadlessMode()) {
#endif
  if(!cfgGetConfigAppliedOnce(config)) {
	  // load configuration that the initdata contains
	  cfg_initdata = iniManagerGetCurrentInitdata(&ini_manager);
	  cfgLoadFromFilename(config, iniGetCurrentConfigurationFilename(cfg_initdata), false);
  }
#ifdef RETRO_PLATFORM
  }
#endif

  cfgManagerSetCurrentConfig(configmanager, config);
}

void cfgManagerShutdown(cfgManager *configmanager)
{
  cfgManagerFreeConfig(configmanager, configmanager->m_currentconfig);
}

void cfgStartup(int argc, char **argv)
{
  cfgManagerStartup(&cfg_manager, argc, argv);
}

void cfgShutdown(void)
{
  cfgManagerShutdown(&cfg_manager);
}
