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

#include <algorithm>
#include <sstream>
#include <vector>

#include "Defs.h"
#include "versioninfo.h"
#include "chipset.h"
#include "FloppyDisk.h"
#include "MemoryInterface.h"
#include "Gameports.h"
#include "GraphicsPipeline.h"
#include "Renderer.h"
#include "Blitter.h"
#include "FellowMain.h"
#include "FellowList.h"
#include "Configuration.h"
#include "Module/Hardfile/IHardfileHandler.h"
#include "FilesystemIntegration.h"
#include "IniFile.h"
#include "CpuIntegration.h"
#include "rtc.h"
#include "RetroPlatform.h"
#include "draw_interlace_control.h"
#include "WindowsUI.h"
#include "KeyboardDriver.h"

#include "../automation/Automator.h"
#include "VirtualHost/Core.h"

using namespace std;
using namespace Module::Hardfile;

ini *cfg_initdata; /* CONFIG copy of initialization data */

/*============================================================================*/
/* The actual cfgManager instance                                             */
/*============================================================================*/

cfgManager cfg_manager;

/*============================================================================*/
/* Configuration                                                              */
/*============================================================================*/

void cfgSetConfigFileVersion(cfg *config, uint32_t version)
{
  config->m_configfileversion = version;
}

uint32_t cfgGetConfigFileVersion(cfg *config)
{
  return config->m_configfileversion;
}

void cfgSetDescription(cfg *config, const string &description)
{
  strncpy(config->m_description, description.c_str(), 255);
}

char *cfgGetDescription(cfg *config)
{
  return config->m_description;
}

/*============================================================================*/
/* Floppy disk configuration property access                                  */
/*============================================================================*/

void cfgSetDiskImage(cfg *config, uint32_t index, const string &diskimage)
{
  if (index < 4)
  {
    strncpy(&(config->m_diskimage[index][0]), diskimage.c_str(), CFG_FILENAME_LENGTH);
  }
}

const char *cfgGetDiskImage(cfg *config, uint32_t index)
{
  if (index < 4)
  {
    return &(config->m_diskimage[index][0]);
  }
  return "";
}

void cfgSetDiskEnabled(cfg *config, uint32_t index, BOOLE enabled)
{
  if (index < 4)
  {
    config->m_diskenabled[index] = enabled;
  }
}

BOOLE cfgGetDiskEnabled(cfg *config, uint32_t index)
{
  if (index < 4)
  {
    return config->m_diskenabled[index];
  }
  return FALSE;
}

void cfgSetDiskReadOnly(cfg *config, uint32_t index, BOOLE readonly)
{
  if (index < 4)
  {
    config->m_diskreadonly[index] = readonly;
  }
}

BOOLE cfgGetDiskReadOnly(cfg *config, uint32_t index)
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

void cfgSetLastUsedDiskDir(cfg *config, const string &directory)
{
  strncpy(config->m_lastuseddiskdir, directory.c_str(), CFG_FILENAME_LENGTH);
}

const char *cfgGetLastUsedDiskDir(cfg *config)
{
  return config->m_lastuseddiskdir;
}

/*============================================================================*/
/* Memory configuration property access                                       */
/*============================================================================*/

void cfgSetChipSize(cfg *config, uint32_t chipsize)
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

uint32_t cfgGetChipSize(cfg *config)
{
  return config->m_chipsize;
}

void cfgSetFastSize(cfg *config, uint32_t fastsize)
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

uint32_t cfgGetFastSize(cfg *config)
{
  return config->m_fastsize;
}

void cfgSetBogoSize(cfg *config, uint32_t bogosize)
{
  config->m_bogosize = bogosize & 0x1c0000;
}

uint32_t cfgGetBogoSize(cfg *config)
{
  return config->m_bogosize;
}

void cfgSetKickImage(cfg *config, const string &kickimage)
{
  strncpy(config->m_kickimage, kickimage.c_str(), CFG_FILENAME_LENGTH);
}

const char *cfgGetKickImage(cfg *config)
{
  return config->m_kickimage;
}

void cfgSetKickImageExtended(cfg *config, const string &kickimageext)
{
  strncpy(config->m_kickimage_ext, kickimageext.c_str(), CFG_FILENAME_LENGTH);
}

const char *cfgGetKickImageExtended(cfg *config)
{
  return config->m_kickimage_ext;
}

void cfgSetKickDescription(cfg *config, const string &kickdescription)
{
  strncpy(config->m_kickdescription, kickdescription.c_str(), CFG_FILENAME_LENGTH);
}

const char *cfgGetKickDescription(cfg *config)
{
  return config->m_kickdescription;
}

void cfgSetKickCRC32(cfg *config, uint32_t kickcrc32)
{
  config->m_kickcrc32 = kickcrc32;
}

uint32_t cfgGetKickCRC32(cfg *config)
{
  return config->m_kickcrc32;
}

void cfgSetKey(cfg *config, const string &key)
{
  strncpy(config->m_key, key.c_str(), CFG_FILENAME_LENGTH);
}

const char *cfgGetKey(cfg *config)
{
  return config->m_key;
}

void cfgSetUseAutoconfig(cfg *config, bool useautoconfig)
{
  config->m_useautoconfig = useautoconfig;
}

bool cfgGetUseAutoconfig(cfg *config)
{
  return config->m_useautoconfig;
}

BOOLE cfgGetAddress32Bit(cfg *config)
{ /* CPU type decides this */
  return (cfgGetCPUType(config) == cpu_integration_models::M68020) || (cfgGetCPUType(config) == cpu_integration_models::M68030);
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

void cfgSetScreenWidth(cfg *config, uint32_t screenwidth)
{
  config->m_screenwidth = screenwidth;
}

uint32_t cfgGetScreenWidth(cfg *config)
{
  return config->m_screenwidth;
}

void cfgSetScreenHeight(cfg *config, uint32_t screenheight)
{
  config->m_screenheight = screenheight;
}

uint32_t cfgGetScreenHeight(cfg *config)
{
  return config->m_screenheight;
}

void cfgSetScreenColorBits(cfg *config, uint32_t screencolorbits)
{
  config->m_screencolorbits = screencolorbits;
}

uint32_t cfgGetScreenColorBits(cfg *config)
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

void cfgSetScreenRefresh(cfg *config, uint32_t screenrefresh)
{
  config->m_screenrefresh = screenrefresh;
}

uint32_t cfgGetScreenRefresh(cfg *config)
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
    _core.Log->AddLog("cfgSetDisplayDriver(): Direct3D requirements not met, falling back to DirectDraw.\n");
    config->m_displaydriver = DISPLAYDRIVER::DISPLAYDRIVER_DIRECTDRAW;
  }
}

DISPLAYDRIVER cfgGetDisplayDriver(cfg *config)
{
  return config->m_displaydriver;
}

/*===========================================================================*/
/* Graphics emulation configuration property access                          */
/*===========================================================================*/

void cfgSetFrameskipRatio(cfg *config, uint32_t frameskipratio)
{
  config->m_frameskipratio = frameskipratio;
}

uint32_t cfgGetFrameskipRatio(cfg *config)
{
  return config->m_frameskipratio;
}

void cfgSetClipLeft(cfg *config, uint32_t left)
{
  config->m_clipleft = left;
}

uint32_t cfgGetClipLeft(cfg *config)
{
  return config->m_clipleft;
}

void cfgSetClipTop(cfg *config, uint32_t top)
{
  config->m_cliptop = top;
}

uint32_t cfgGetClipTop(cfg *config)
{
  return config->m_cliptop;
}

void cfgSetClipRight(cfg *config, uint32_t right)
{
  config->m_clipright = right;
}

uint32_t cfgGetClipRight(cfg *config)
{
  return config->m_clipright;
}

void cfgSetClipBottom(cfg *config, uint32_t bottom)
{
  config->m_clipbottom = bottom;
}

uint32_t cfgGetClipBottom(cfg *config)
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

void cfgSetSoundVolume(cfg *config, const uint32_t soundvolume)
{
  config->m_soundvolume = soundvolume;
}

uint32_t cfgGetSoundVolume(cfg *config)
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

void cfgSetSoundBufferLength(cfg *config, uint32_t buffer_length)
{
  config->m_bufferlength = buffer_length;
}

uint32_t cfgGetSoundBufferLength(cfg *config)
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

void cfgSetCPUSpeed(cfg *config, uint32_t CPUspeed)
{
  config->m_CPUspeed = CPUspeed;
}

uint32_t cfgGetCPUSpeed(cfg *config)
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

cfg_hardfile cfgGetHardfile(cfg *config, uint32_t index)
{
  return *static_cast<cfg_hardfile *>(listNode(listIndex(config->m_hardfiles, index)));
}

uint32_t cfgGetHardfileCount(cfg *config)
{
  return listCount(config->m_hardfiles);
}

void cfgHardfileAdd(cfg *config, cfg_hardfile *hardfile)
{
  cfg_hardfile *hf = static_cast<cfg_hardfile *>(malloc(sizeof(cfg_hardfile)));
  *hf = *hardfile;
  config->m_hardfiles = listAddLast(config->m_hardfiles, listNew(hf));
}

void cfgHardfileRemove(cfg *config, uint32_t index)
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
  listFreeAll(config->m_hardfiles, true);
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

void cfgHardfileChange(cfg *config, cfg_hardfile *hardfile, uint32_t index)
{
  felist *node = listIndex(config->m_hardfiles, index);
  cfg_hardfile *hf = static_cast<cfg_hardfile *>(listNode(node));
  *hf = *hardfile;
}

/*============================================================================*/
/* Filesystem configuration property access                                   */
/*============================================================================*/

cfg_filesys cfgGetFilesystem(cfg *config, uint32_t index)
{
  return *static_cast<cfg_filesys *>(listNode(listIndex(config->m_filesystems, index)));
}

uint32_t cfgGetFilesystemCount(cfg *config)
{
  return listCount(config->m_filesystems);
}

void cfgFilesystemAdd(cfg *config, cfg_filesys *filesystem)
{
  cfg_filesys *fsys = static_cast<cfg_filesys *>(malloc(sizeof(cfg_filesys)));
  *fsys = *filesystem;
  config->m_filesystems = listAddLast(config->m_filesystems, listNew(fsys));
}

void cfgFilesystemRemove(cfg *config, uint32_t index)
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
  listFreeAll(config->m_filesystems, true);
  config->m_filesystems = nullptr;
}

void cfgSetFilesystemUnitDefaults(cfg_filesys *unit)
{
  memset(unit, 0, sizeof(cfg_filesys));
  unit->readonly = FALSE;
}

void cfgFilesystemChange(cfg *config, cfg_filesys *unit, uint32_t index)
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

void cfgSetFilesystemDeviceNamePrefix(cfg *config, const string &prefix)
{
  if (!prefix.empty())
  {
    strncpy(config->m_filesystem_device_name_prefix, prefix.c_str(), CFG_FILENAME_LENGTH);
  }
}

const char *cfgGetFilesystemDeviceNamePrefix(cfg *config)
{
  return config->m_filesystem_device_name_prefix;
}

/*============================================================================*/
/* Game port configuration property access                                    */
/*============================================================================*/

void cfgSetGameport(cfg *config, uint32_t index, gameport_inputs gameport)
{
  if (index < 2)
  {
    config->m_gameport[index] = gameport;
  }
}

gameport_inputs cfgGetGameport(cfg *config, uint32_t index)
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

void cfgSetUseGUI(cfg *config, bool useGUI)
{
  config->m_useGUI = useGUI;
}

bool cfgGetUseGUI(cfg *config)
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
  if (config == nullptr) return;

  /*==========================================================================*/
  /* Default configuration version and description                            */
  /*==========================================================================*/

  cfgSetConfigFileVersion(config, CONFIG_CURRENT_FILE_VERSION);
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

  cfgSetScreenWidth(config, 800);
  cfgSetScreenHeight(config, 600);
  cfgSetScreenColorBits(config, 32);
  cfgSetScreenRefresh(config, 0);
  cfgSetScreenWindowed(config, true);
  cfgSetUseMultipleGraphicalBuffers(config, FALSE);
  cfgSetScreenDrawLEDs(config, true);
  cfgSetDeinterlace(config, true);
  cfgSetDisplayDriver(config, DISPLAYDRIVER::DISPLAYDRIVER_DIRECT3D11);

  /*==========================================================================*/
  /* Default graphics emulation configuration                                 */
  /*==========================================================================*/

  cfgSetFrameskipRatio(config, 0);
  cfgSetDisplayScale(config, DISPLAYSCALE::DISPLAYSCALE_1X);
  cfgSetDisplayScaleStrategy(config, DISPLAYSCALE_STRATEGY::DISPLAYSCALE_STRATEGY_SOLID);
  cfgSetGraphicsEmulationMode(config, GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_LINEEXACT);
  // cfgSetGraphicsEmulationMode(config, GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT);
  cfgSetClipLeft(config, 96);
  cfgSetClipTop(config, 26);
  cfgSetClipRight(config, 472);
  cfgSetClipBottom(config, 314);

  /*==========================================================================*/
  /* Default sound configuration                                              */
  /*==========================================================================*/

  cfgSetSoundEmulation(config, sound_emulations::SOUND_PLAY);
  cfgSetSoundRate(config, sound_rates::SOUND_44100);
  cfgSetSoundStereo(config, TRUE);
  cfgSetSound16Bits(config, TRUE);
  cfgSetSoundFilter(config, sound_filters::SOUND_FILTER_ORIGINAL);
  cfgSetSoundVolume(config, 100);
  cfgSetSoundWAVDump(config, FALSE);
  cfgSetSoundNotification(config, sound_notifications::SOUND_MMTIMER_NOTIFICATION);
  cfgSetSoundBufferLength(config, 60);

  /*==========================================================================*/
  /* Default CPU configuration                                                */
  /*==========================================================================*/

  cfgSetCPUType(config, cpu_integration_models::M68000);
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
  cfgSetFilesystemDeviceNamePrefix(config, "FS");

  /*==========================================================================*/
  /* Default game port configuration                                          */
  /*==========================================================================*/

  cfgSetGameport(config, 0, GP_MOUSE0);
  cfgSetGameport(config, 1, GP_NONE);

  /*==========================================================================*/
  /* Default GUI configuration                                                */
  /*==========================================================================*/

  cfgSetUseGUI(config, true);

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

string cfgGetLowercaseString(const string &s)
{
  string copy = s;
  transform(copy.begin(), copy.end(), copy.begin(), [](const char v) { return static_cast<char>(std::tolower(v)); });
  return copy;
}

vector<string> cfgSplitString(const string &source, char splitchar)
{
  stringstream sourcestream(source);
  string part;
  vector<std::string> parts;

  while (std::getline(sourcestream, part, splitchar))
  {
    parts.push_back(part);
  }

  return parts;
}

static BOOLE cfgGetBOOLEFromString(const string &value)
{
  const char lowercaseValue = std::tolower(value[0]);
  return (lowercaseValue == 'y' || lowercaseValue == 't');
}

static const char *cfgGetBOOLEToString(BOOLE value)
{
  return (value) ? "yes" : "no";
}

static bool cfgGetboolFromString(const string &value)
{
  const char lowercaseValue = std::tolower(value[0]);
  return (value[0] == 'y' || value[0] == 't');
}

static const char *cfgGetboolToString(bool value)
{
  return (value) ? "yes" : "no";
}

static uint32_t cfgGetUint32FromString(const string &value)
{
  return atoi(value.c_str());
}

static gameport_inputs cfgGetGameportFromString(const string &value)
{
  string lowercaseValue = cfgGetLowercaseString(value);

  if (lowercaseValue == "mouse")
  {
    return GP_MOUSE0;
  }
  if (lowercaseValue == "joy0")
  {
    return GP_ANALOG0;
  }
  if (lowercaseValue == "joy1")
  {
    return GP_ANALOG1;
  }
  if (lowercaseValue == "kbd1")
  {
    return GP_JOYKEY0;
  }
  if (lowercaseValue == "kbd2")
  {
    return GP_JOYKEY1;
  }

  return GP_NONE;
}

static const char *cfgGetGameportToString(gameport_inputs gameport)
{
  switch (gameport)
  {
    case GP_NONE: return "none";
    case GP_MOUSE0: return "mouse";
    case GP_ANALOG0: return "joy0";
    case GP_ANALOG1: return "joy1";
    case GP_JOYKEY0: return "kbd1";
    case GP_JOYKEY1: return "kbd2";
  }
  return "none";
}

static cpu_integration_models cfgGetCPUTypeFromString(const string &value)
{
  string lowercaseValue = cfgGetLowercaseString(value);

  if (lowercaseValue == "68000")
  {
    return cpu_integration_models::M68000;
  }
  if (lowercaseValue == "68010")
  {
    return cpu_integration_models::M68010;
  }
  if (lowercaseValue == "68020")
  {
    return cpu_integration_models::M68020;
  }
  if (lowercaseValue == "68020/68881")
  {
    return cpu_integration_models::M68020; /* Unsupp */
  }
  if (lowercaseValue == "68ec20")
  {
    return cpu_integration_models::M68EC20;
  }
  if (lowercaseValue == "68ec20/68881")
  {
    return cpu_integration_models::M68EC20; /* Unsupp */
  }
  if (lowercaseValue == "68030")
  {
    return cpu_integration_models::M68030;
  }
  if (lowercaseValue == "68ec30")
  {
    return cpu_integration_models::M68EC30;
  }
  return cpu_integration_models::M68000;
}

static const char *cfgGetCPUTypeToString(cpu_integration_models cputype)
{
  switch (cputype)
  {
    case cpu_integration_models::M68000: return "68000";
    case cpu_integration_models::M68010: return "68010";
    case cpu_integration_models::M68020: return "68020";
    case cpu_integration_models::M68EC20: return "68ec20";
    case cpu_integration_models::M68030: return "68030";
    case cpu_integration_models::M68EC30: return "68ec30";
  }

  return "68000";
}

static uint32_t cfgGetCPUSpeedFromString(const string &value)
{
  string lowercaseValue = cfgGetLowercaseString(value);

  if (lowercaseValue == "real")
  {
    return 4;
  }
  if (lowercaseValue == "max")
  {
    return 1;
  }

  uint32_t speed = cfgGetUint32FromString(value);
  if (speed > 20)
  {
    speed = 8;
  }

  return speed;
}

static sound_notifications cfgGetSoundNotificationFromString(const string &value)
{
  string lowercaseValue = cfgGetLowercaseString(value);

  if (lowercaseValue == "directsound")
  {
    return sound_notifications::SOUND_DSOUND_NOTIFICATION;
  }

  if (lowercaseValue == "mmtimer")
  {
    return sound_notifications::SOUND_MMTIMER_NOTIFICATION;
  }

  return sound_notifications::SOUND_MMTIMER_NOTIFICATION;
}

static const char *cfgGetSoundNotificationToString(sound_notifications soundnotification)
{
  switch (soundnotification)
  {
    case sound_notifications::SOUND_DSOUND_NOTIFICATION: return "directsound";
    case sound_notifications::SOUND_MMTIMER_NOTIFICATION: return "mmtimer";
  }
  return "mmtimer";
}

static sound_emulations cfgGetSoundEmulationFromString(const string &value)
{
  string lowercaseValue = cfgGetLowercaseString(value);

  if (lowercaseValue == "none")
  {
    return sound_emulations::SOUND_NONE;
  }

  if (lowercaseValue == "interrupts")
  {
    return sound_emulations::SOUND_EMULATE;
  }

  if (lowercaseValue == "normal" || lowercaseValue == "exact" || lowercaseValue == "good" || lowercaseValue == "best")
  {
    return sound_emulations::SOUND_PLAY;
  }

  return sound_emulations::SOUND_NONE;
}

static const char *cfgGetSoundEmulationToString(sound_emulations soundemulation)
{
  switch (soundemulation)
  {
    case sound_emulations::SOUND_NONE: return "none";
    case sound_emulations::SOUND_EMULATE: return "interrupts";
    case sound_emulations::SOUND_PLAY: return "normal";
  }
  return "none";
}

static bool cfgGetSoundStereoFromString(const string &value)
{
  string lowercaseValue = cfgGetLowercaseString(value);

  if (lowercaseValue == "mono" || lowercaseValue == "m" || lowercaseValue == "1")
  {
    return false;
  }

  if (lowercaseValue == "stereo" || lowercaseValue == "s" || lowercaseValue == "2")
  {
    return true;
  }

  return false;
}

static const char *cfgGetSoundStereoToString(bool soundstereo)
{
  return soundstereo ? "stereo" : "mono";
}

static bool cfgGetSound16BitsFromString(const string &value)
{
  return value == "16";
}

static const char *cfgGetSound16BitsToString(bool sound16bits)
{
  return sound16bits ? "16" : "8";
}

static sound_rates cfgGetSoundRateFromString(const string &value)
{
  uint32_t rate = cfgGetUint32FromString(value);

  if (rate < 22050)
  {
    return sound_rates::SOUND_15650;
  }
  if (rate < 31300)
  {
    return sound_rates::SOUND_22050;
  }
  if (rate < 44100)
  {
    return sound_rates::SOUND_31300;
  }
  return sound_rates::SOUND_44100;
}

static const char *cfgGetSoundRateToString(sound_rates soundrate)
{
  switch (soundrate)
  {
    case sound_rates::SOUND_15650: return "15650";
    case sound_rates::SOUND_22050: return "22050";
    case sound_rates::SOUND_31300: return "31300";
    case sound_rates::SOUND_44100: return "44100";
  }
  return "44100";
}

static sound_filters cfgGetSoundFilterFromString(const string &value)
{
  string lowercaseValue = cfgGetLowercaseString(value);

  if (lowercaseValue == "never")
  {
    return sound_filters::SOUND_FILTER_NEVER;
  }
  if (lowercaseValue == "original")
  {
    return sound_filters::SOUND_FILTER_ORIGINAL;
  }
  if (lowercaseValue == "always")
  {
    return sound_filters::SOUND_FILTER_ALWAYS;
  }
  return sound_filters::SOUND_FILTER_ORIGINAL;
}

static const char *cfgGetSoundFilterToString(sound_filters filter)
{
  switch (filter)
  {
    case sound_filters::SOUND_FILTER_NEVER: return "never";
    case sound_filters::SOUND_FILTER_ORIGINAL: return "original";
    case sound_filters::SOUND_FILTER_ALWAYS: return "always";
  }
  return "original";
}

static uint32_t cfgGetBufferLengthFromString(const string &value)
{
  uint32_t buffer_length = cfgGetUint32FromString(value);

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

static DISPLAYSCALE cfgGetDisplayScaleFromString(const string &value)
{
  string lowercaseValue = cfgGetLowercaseString(value);

  if (lowercaseValue == "auto")
  {
    return DISPLAYSCALE::DISPLAYSCALE_AUTO;
  }
  if (lowercaseValue == "quadruple")
  {
    return DISPLAYSCALE::DISPLAYSCALE_4X;
  }
  if (lowercaseValue == "triple")
  {
    return DISPLAYSCALE::DISPLAYSCALE_3X;
  }
  if (lowercaseValue == "double")
  {
    return DISPLAYSCALE::DISPLAYSCALE_2X;
  }
  if (lowercaseValue == "single")
  {
    return DISPLAYSCALE::DISPLAYSCALE_1X;
  }
  return DISPLAYSCALE::DISPLAYSCALE_1X; // Default
}

static const char *cfgGetDisplayScaleToString(DISPLAYSCALE displayscale)
{
  switch (displayscale)
  {
    case DISPLAYSCALE::DISPLAYSCALE_AUTO: return "auto";
    case DISPLAYSCALE::DISPLAYSCALE_1X: return "single";
    case DISPLAYSCALE::DISPLAYSCALE_2X: return "double";
    case DISPLAYSCALE::DISPLAYSCALE_3X: return "triple";
    case DISPLAYSCALE::DISPLAYSCALE_4X: return "quadruple";
  }
  return "single";
}

static DISPLAYDRIVER cfgGetDisplayDriverFromString(const string &value)
{
  string lowercaseValue = cfgGetLowercaseString(value);

  if (lowercaseValue == "directdraw")
  {
    return DISPLAYDRIVER::DISPLAYDRIVER_DIRECTDRAW;
  }
  if (lowercaseValue == "direct3d11")
  {
    return DISPLAYDRIVER::DISPLAYDRIVER_DIRECT3D11;
  }
  return DISPLAYDRIVER::DISPLAYDRIVER_DIRECTDRAW; // Default
}

static const char *cfgGetDisplayDriverToString(DISPLAYDRIVER displaydriver)
{
  switch (displaydriver)
  {
    case DISPLAYDRIVER::DISPLAYDRIVER_DIRECTDRAW: return "directdraw";
    case DISPLAYDRIVER::DISPLAYDRIVER_DIRECT3D11: return "direct3d11";
  }
  return "directdraw";
}

static DISPLAYSCALE_STRATEGY cfgGetDisplayScaleStrategyFromString(const string &value)
{
  string lowercaseValue = cfgGetLowercaseString(value);

  if (lowercaseValue == "scanlines")
  {
    return DISPLAYSCALE_STRATEGY::DISPLAYSCALE_STRATEGY_SCANLINES;
  }
  if (lowercaseValue == "solid")
  {
    return DISPLAYSCALE_STRATEGY::DISPLAYSCALE_STRATEGY_SOLID;
  }
  return DISPLAYSCALE_STRATEGY::DISPLAYSCALE_STRATEGY_SOLID; // Default
}

static const char *cfgGetDisplayScaleStrategyToString(DISPLAYSCALE_STRATEGY displayscalestrategy)
{
  switch (displayscalestrategy)
  {
    case DISPLAYSCALE_STRATEGY::DISPLAYSCALE_STRATEGY_SOLID: return "solid";
    case DISPLAYSCALE_STRATEGY::DISPLAYSCALE_STRATEGY_SCANLINES: return "scanlines";
  }
  return "solid";
}

static uint32_t cfgGetColorBitsFromString(const string &value)
{
  string lowercaseValue = cfgGetLowercaseString(value);

  if (lowercaseValue == "8bit" || lowercaseValue == "8")
  {
    return 16; // Fallback to 16 bits, 8 bit support is removed
  }
  if (lowercaseValue == "15bit" || lowercaseValue == "15")
  {
    return 15;
  }
  if (lowercaseValue == "16bit" || lowercaseValue == "16")
  {
    return 16;
  }
  if (lowercaseValue == "24bit" || lowercaseValue == "24")
  {
    return 24;
  }
  if (lowercaseValue == "32bit" || lowercaseValue == "32")
  {
    return 32;
  }
  return 16;
}

static const char *cfgGetColorBitsToString(uint32_t colorbits)
{
  switch (colorbits)
  {
    case 16: return "16bit";
    case 24: return "24bit";
    case 32: return "32bit";
  }
  return "8bit";
}

static bool cfgGetECSFromString(const string &value)
{
  string lowercaseValue = cfgGetLowercaseString(value);

  if (lowercaseValue == "ocs" || lowercaseValue == "0")
  {
    return false;
  }
  if (lowercaseValue == "ecs agnes" || lowercaseValue == "ecs denise" || lowercaseValue == "ecs" || lowercaseValue == "aga" || lowercaseValue == "2" ||
      lowercaseValue == "3" || lowercaseValue == "4")
  {
    return true;
  }
  return false;
}

static const char *cfgGetECSToString(bool chipset_ecs)
{
  return (chipset_ecs) ? "ecs" : "ocs";
}

void cfgSetConfigAppliedOnce(cfg *config, BOOLE bApplied)
{
  config->m_config_applied_once = bApplied;
}

BOOLE cfgGetConfigAppliedOnce(cfg *config)
{
  return config->m_config_applied_once;
}

void cfgSetConfigChangedSinceLastSave(cfg *config, BOOLE bChanged)
{
  config->m_config_changed_since_save = bChanged;
}

BOOLE cfgGetConfigChangedSinceLastSave(cfg *config)
{
  return config->m_config_changed_since_save;
}

static GRAPHICSEMULATIONMODE cfgGetGraphicsEmulationModeFromString(const string &value)
{
  string lowercaseValue = cfgGetLowercaseString(value);

  if (lowercaseValue == "lineexact")
  {
    return GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_LINEEXACT;
  }
  if (lowercaseValue == "cycleexact")
  {
    return GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT;
  }
  return GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_LINEEXACT;
}

static const char *cfgGetGraphicsEmulationModeToString(GRAPHICSEMULATIONMODE graphicsemulationmode)
{
  switch (graphicsemulationmode)
  {
    case GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_LINEEXACT: return "lineexact";
    case GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT: return "cycleexact";
  }
  return "lineexact";
}

/*============================================================================*/
/* Command line option synopsis                                               */
/*============================================================================*/

void cfgSynopsis(cfg *config)
{
  fprintf(
      stderr,
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

BOOLE cfgSetOption(cfg *config, const char *optionstr)
{
  vector<string> option = cfgSplitString(optionstr, '=');
  if (option.size() < 1)
  {
    return FALSE;
  }

  string name = cfgGetLowercaseString(option[0]);
  string value;
  if (option.size() == 2)
  {
    value = option[1];
  }
  else
  {
    value = "";
  }

  BOOLE result = TRUE;

  /* Standard configuration options */

  if (name == "help")
  {
    cfgSynopsis(config);
    exit(EXIT_SUCCESS);
  }
  else if (name == "autoconfig")
  {
    cfgSetUseAutoconfig(config, cfgGetboolFromString(value));
  }
  else if (name == "config_version")
  {
    cfgSetConfigFileVersion(config, cfgGetUint32FromString(value));
  }
  else if (name == "config_description")
  {
    cfgSetDescription(config, value);
  }
  else if (name == "floppy0")
  {
    cfgSetDiskImage(config, 0, value);
  }
  else if (name == "floppy1")
  {
    cfgSetDiskImage(config, 1, value);
  }
  else if (name == "floppy2")
  {
    cfgSetDiskImage(config, 2, value);
  }
  else if (name == "floppy3")
  {
    cfgSetDiskImage(config, 3, value);
  }
  else if (name == "fellow.last_used_disk_dir")
  {
    cfgSetLastUsedDiskDir(config, value);
  }
  else if (name == "fellow.floppy0_enabled" || name == "floppy0_enabled")
  {
    cfgSetDiskEnabled(config, 0, cfgGetBOOLEFromString(value));
  }
  else if (name == "fellow.floppy1_enabled" || name == "floppy1_enabled")
  {
    cfgSetDiskEnabled(config, 1, cfgGetBOOLEFromString(value));
  }
  else if (name == "fellow.floppy2_enabled" || name == "floppy2_enabled")
  {
    cfgSetDiskEnabled(config, 2, cfgGetBOOLEFromString(value));
  }
  else if (name == "fellow.floppy3_enabled" || name == "floppy3_enabled")
  {
    cfgSetDiskEnabled(config, 3, cfgGetBOOLEFromString(value));
  }
  else if (name == "fellow.floppy_fast_dma" || name == "floppy_fast_dma")
  {
    cfgSetDiskFast(config, cfgGetBOOLEFromString(value));
  }
  else if (name == "fellow.floppy0_readonly" || name == "floppy0_readonly")
  {
    cfgSetDiskReadOnly(config, 0, cfgGetBOOLEFromString(value));
  }
  else if (name == "fellow.floppy1_readonly" || name == "floppy1_readonly")
  {
    cfgSetDiskReadOnly(config, 1, cfgGetBOOLEFromString(value));
  }
  else if (name == "fellow.floppy2_readonly" || name == "floppy2_readonly")
  {
    cfgSetDiskReadOnly(config, 2, cfgGetBOOLEFromString(value));
  }
  else if (name == "fellow.floppy3_readonly" || name == "floppy3_readonly")
  {
    cfgSetDiskReadOnly(config, 3, cfgGetBOOLEFromString(value));
  }
  else if (name == "joyport0")
  {
    cfgSetGameport(config, 0, cfgGetGameportFromString(value));
  }
  else if (name == "joyport1")
  {
    cfgSetGameport(config, 1, cfgGetGameportFromString(value));
  }
  else if (name == "usegui")
  {
    cfgSetUseGUI(config, cfgGetboolFromString(value));
  }
  else if (name == "cpu_speed")
  {
    cfgSetCPUSpeed(config, cfgGetCPUSpeedFromString(value));
  }
  else if (name == "cpu_type")
  {
    cfgSetCPUType(config, cfgGetCPUTypeFromString(value));
  }
  else if (name == "sound_output")
  {
    cfgSetSoundEmulation(config, cfgGetSoundEmulationFromString(value));
  }
  else if (name == "sound_channels")
  {
    cfgSetSoundStereo(config, cfgGetSoundStereoFromString(value));
  }
  else if (name == "sound_bits")
  {
    cfgSetSound16Bits(config, cfgGetSound16BitsFromString(value));
  }
  else if (name == "sound_frequency")
  {
    cfgSetSoundRate(config, cfgGetSoundRateFromString(value));
  }
  else if (name == "sound_volume")
  {
    cfgSetSoundVolume(config, cfgGetUint32FromString(value));
  }
  else if (name == "fellow.sound_wav" || name == "sound_wav")
  {
    cfgSetSoundWAVDump(config, cfgGetBOOLEFromString(value));
  }
  else if (name == "fellow.sound_filter" || name == "sound_filter")
  {
    cfgSetSoundFilter(config, cfgGetSoundFilterFromString(value));
  }
  else if (name == "sound_notification")
  {
    cfgSetSoundNotification(config, cfgGetSoundNotificationFromString(value));
  }
  else if (name == "sound_buffer_length")
  {
    cfgSetSoundBufferLength(config, cfgGetBufferLengthFromString(value));
  }
  else if (name == "chipmem_size")
  {
    cfgSetChipSize(config, cfgGetUint32FromString(value) * 262144);
  }
  else if (name == "fastmem_size")
  {
    cfgSetFastSize(config, cfgGetUint32FromString(value) * 1048576);
  }
  else if (name == "bogomem_size")
  {
    cfgSetBogoSize(config, cfgGetUint32FromString(value) * 262144);
  }
  else if (name == "kickstart_rom_file")
  {
    cfgSetKickImage(config, value);
  }
  else if (name == "kickstart_rom_file_ext")
  {
    cfgSetKickImageExtended(config, value);
  }
  else if (name == "kickstart_rom_description")
  {
    cfgSetKickDescription(config, value);
  }
  else if (name == "kickstart_rom_crc32")
  {
    uint32_t crc32;
    sscanf(value.c_str(), "%lX", &crc32);
    cfgSetKickCRC32(config, crc32);
  }
  else if (name == "kickstart_key_file")
  {
    cfgSetKey(config, value);
  }
  else if (name == "gfx_immediate_blits")
  {
    cfgSetBlitterFast(config, cfgGetBOOLEFromString(value));
  }
  else if (name == "gfx_chipset")
  {
    cfgSetECS(config, cfgGetECSFromString(value));
  }
  else if (name == "gfx_width")
  {
    cfgSetScreenWidth(config, cfgGetUint32FromString(value));
  }
  else if (name == "gfx_height")
  {
    cfgSetScreenHeight(config, cfgGetUint32FromString(value));
  }
  else if (name == "fellow.gfx_refresh" || name == "gfx_refresh")
  {
    cfgSetScreenRefresh(config, cfgGetUint32FromString(value));
  }
  else if (name == "gfx_fullscreen_amiga")
  {
    cfgSetScreenWindowed(config, !cfgGetboolFromString(value));
  }
  else if (name == "use_multiple_graphical_buffers")
  {
    cfgSetUseMultipleGraphicalBuffers(config, cfgGetBOOLEFromString(value));
  }
  else if (name == "gfx_driver")
  {
    cfgSetDisplayDriver(config, cfgGetDisplayDriverFromString(value));
  }
  else if (name == "gfx_emulation_mode")
  {
    cfgSetGraphicsEmulationMode(config, cfgGetGraphicsEmulationModeFromString(value));
  }
  else if (name == "gfx_colour_mode")
  {
    cfgSetScreenColorBits(config, cfgGetColorBitsFromString(value));
  }
  else if (name == "show_leds")
  {
    cfgSetScreenDrawLEDs(config, cfgGetboolFromString(value));
  }
  else if (name == "gfx_clip_left")
  {
    cfgSetClipLeft(config, cfgGetUint32FromString(value));
  }
  else if (name == "gfx_clip_top")
  {
    cfgSetClipTop(config, cfgGetUint32FromString(value));
  }
  else if (name == "gfx_clip_right")
  {
    cfgSetClipRight(config, cfgGetUint32FromString(value));
  }
  else if (name == "gfx_clip_bottom")
  {
    cfgSetClipBottom(config, cfgGetUint32FromString(value));
  }
  else if (name == "gfx_display_scale")
  {
    cfgSetDisplayScale(config, cfgGetDisplayScaleFromString(value));
  }
  else if (name == "gfx_display_scale_strategy")
  {
    cfgSetDisplayScaleStrategy(config, cfgGetDisplayScaleStrategyFromString(value));
  }
  else if (name == "gfx_framerate")
  {
    cfgSetFrameskipRatio(config, cfgGetUint32FromString(value));
  }
  else if (name == "fellow.gfx_deinterlace" || name == "gfx_deinterlace")
  {
    cfgSetDeinterlace(config, cfgGetboolFromString(value));
  }
  else if (name == "fellow.measure_speed" || name == "measure_speed")
  {
    cfgSetMeasureSpeed(config, cfgGetboolFromString(value));
  }
  else if (name == "rtc")
  {
    cfgSetRtc(config, cfgGetboolFromString(value));
  }
  else if (name == "hardfile")
  {
    // Format is <access flags "rw" or "ro">,<sector count>,<surface count>,<reserved count>,<block size>,<filename>
    vector<string> valueparts = cfgSplitString(value, ',');

    if (valueparts.size() != 6)
    {
      return FALSE;
    }

    cfg_hardfile hf;

    string accessFlags = cfgGetLowercaseString(valueparts[0]);
    if (accessFlags == "ro")
    {
      hf.readonly = TRUE;
    }
    else if (accessFlags == "rw")
    {
      hf.readonly = FALSE;
    }
    else
    {
      return FALSE;
    }

    hf.sectorspertrack = cfgGetUint32FromString(valueparts[1]);
    hf.surfaces = cfgGetUint32FromString(valueparts[2]);
    hf.reservedblocks = cfgGetUint32FromString(valueparts[3]);
    hf.bytespersector = cfgGetUint32FromString(valueparts[4]);

    strncpy(hf.filename, valueparts[5].c_str(), CFG_FILENAME_LENGTH);

    hf.rdbstatus = _core.HardfileHandler->HasRDB(hf.filename);
    if (hf.rdbstatus == rdb_status::RDB_FOUND)
    {
      HardfileConfiguration rdbConfiguration = _core.HardfileHandler->GetConfigurationFromRDBGeometry(hf.filename);
      hf.bytespersector = rdbConfiguration.Geometry.BytesPerSector;
      hf.sectorspertrack = rdbConfiguration.Geometry.SectorsPerTrack;
      hf.surfaces = rdbConfiguration.Geometry.Surfaces;
      hf.reservedblocks = rdbConfiguration.Geometry.ReservedBlocks;
    }

    cfgHardfileAdd(config, &hf);
  }
  else if (name == "filesystem")
  {
    // Format is <access flags "rw" or "ro">,<volume name>:<root path>
    // Issue, volume name and path could contain commas, so this split only reliably identifies access flags
    vector<string> valueparts = cfgSplitString(value, ',');

    if (valueparts.size() < 2)
    {
      return FALSE;
    }

    cfg_filesys fs;

    string accessFlags = cfgGetLowercaseString(valueparts[0]);
    if (accessFlags == "ro")
    {
      fs.readonly = TRUE;
    }
    else if (accessFlags == "rw")
    {
      fs.readonly = FALSE;
    }
    else
    {
      return FALSE;
    }

    string rest = value.substr(3);
    vector<string> restparts = cfgSplitString(rest, ':');
    if (restparts.size() < 2)
    {
      return FALSE;
    }

    strncpy(fs.volumename, restparts[0].c_str(), 64);

    string rootpath = rest.substr(rest.find_first_of(':') + 1);
    strncpy(fs.rootpath, rootpath.c_str(), CFG_FILENAME_LENGTH); /* Rootpath */
    cfgFilesystemAdd(config, &fs);
  }
  else if (name == "fellow.map_drives" || name == "fellow.win32.map_drives" || name == "win32.map_drives" || name == "map_drives")
  {
    cfgSetFilesystemAutomountDrives(config, cfgGetBOOLEFromString(value));
  }
  else if (name == "filesystem_device_name_prefix")
  {
    cfgSetFilesystemDeviceNamePrefix(config, value);
  }
  else if (name == "use_debugger")
  { /* Unsupported */
  }
  else if (name == "log_illegal_mem")
  { /* Unsupported */
  }
  else if (name == "gfx_correct_aspect")
  { /* Unsupported */
  }
  else if (name == "gfx_center_vertical")
  { /* Unsupported */
  }
  else if (name == "gfx_center_horizontal")
  { /* Unsupported */
  }
  else if (name == "gfx_fullscreen_picasso")
  { /* Unsupported */
  }
  else if (name == "z3mem_size")
  { /* Unsupported */
  }
  else if (name == "a3000mem_size")
  { /* Unsupported */
  }
  else if (name == "sound_min_buff")
  { /* Unsupported */
  }
  else if (name == "sound_max_buff")
  { /* Unsupported */
  }
  else if (name == "accuracy")
  { /* Unsupported */
  }
  else if (name == "gfx_32bit_blits")
  { /* Unsupported */
  }
  else if (name == "gfx_test_speed")
  { /* Unsupported */
  }
  else if (name == "gfxlib_replacement")
  { /* Unsupported */
  }
  else if (name == "bsdsocket_emu")
  { /* Unsupported */
  }
  else if (name == "parallel_on_demand")
  { /* Unsupported */
  }
  else if (name == "serial_on_demand")
  { /* Unsupported */
  }
  else if (name == "kickshifter")
  { /* Unsupported */
  }
  else if (name == "enforcer")
  { /* Unsupported */
  }
  else if (name == "cpu_compatible")
  { /* Static support */
  }
  else if (name == "cpu_24bit_addressing")
  { /* Redundant */
  }
  else
#ifdef RETRO_PLATFORM
      if (RP.GetHeadlessMode())
  {
    if (name == "gfx_offset_left")
    {
      RP.SetClippingOffsetLeft(cfgGetUint32FromString(value));
    }
    else if (name == "gfx_offset_top")
    {
      RP.SetClippingOffsetTop(cfgGetUint32FromString(value));
    }
  }
  else
#endif
    result = FALSE;

  return result;
}

/*============================================================================*/
/* Save options supported by this class to file                               */
/*============================================================================*/

BOOLE cfgSaveOptions(cfg *config, FILE *cfgfile)
{
  fprintf(cfgfile, "config_version=%u\n", cfgGetConfigFileVersion(config));
  fprintf(cfgfile, "config_description=%s\n", cfgGetDescription(config));
  fprintf(cfgfile, "autoconfig=%s\n", cfgGetboolToString(cfgGetUseAutoconfig(config)));
  for (uint32_t i = 0; i < 4; i++)
  {
    fprintf(cfgfile, "floppy%u=%s\n", i, cfgGetDiskImage(config, i));
    fprintf(cfgfile, "fellow.floppy%u_enabled=%s\n", i, cfgGetBOOLEToString(cfgGetDiskEnabled(config, i)));
    fprintf(cfgfile, "fellow.floppy%u_readonly=%s\n", i, cfgGetBOOLEToString(cfgGetDiskReadOnly(config, i)));
  }
  fprintf(cfgfile, "fellow.floppy_fast_dma=%s\n", cfgGetBOOLEToString(cfgGetDiskFast(config)));
  fprintf(cfgfile, "fellow.last_used_disk_dir=%s\n", cfgGetLastUsedDiskDir(config));
  fprintf(cfgfile, "joyport0=%s\n", cfgGetGameportToString(cfgGetGameport(config, 0)));
  fprintf(cfgfile, "joyport1=%s\n", cfgGetGameportToString(cfgGetGameport(config, 1)));
  fprintf(cfgfile, "usegui=%s\n", cfgGetboolToString(cfgGetUseGUI(config)));
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
  if (strcmp(cfgGetKickDescription(config), "") != 0)
  {
    fprintf(cfgfile, "kickstart_rom_description=%s\n", cfgGetKickDescription(config));
  }
  if (cfgGetKickCRC32(config) != 0)
  {
    fprintf(cfgfile, "kickstart_rom_crc32=%X\n", cfgGetKickCRC32(config));
  }
  fprintf(cfgfile, "kickstart_key_file=%s\n", cfgGetKey(config));
  fprintf(cfgfile, "gfx_immediate_blits=%s\n", cfgGetBOOLEToString(cfgGetBlitterFast(config)));
  fprintf(cfgfile, "gfx_chipset=%s\n", cfgGetECSToString(cfgGetECS(config)));
  fprintf(cfgfile, "gfx_width=%u\n", cfgGetScreenWidth(config));
  fprintf(cfgfile, "gfx_height=%u\n", cfgGetScreenHeight(config));
  fprintf(cfgfile, "gfx_fullscreen_amiga=%s\n", cfgGetboolToString(!cfgGetScreenWindowed(config)));
  fprintf(cfgfile, "use_multiple_graphical_buffers=%s\n", cfgGetBOOLEToString(cfgGetUseMultipleGraphicalBuffers(config)));
  fprintf(cfgfile, "gfx_driver=%s\n", cfgGetDisplayDriverToString(cfgGetDisplayDriver(config)));
  // fprintf(cfgfile, "gfx_emulation_mode=%s\n"), cfgGetGraphicsEmulationModeToString(cfgGetGraphicsEmulationMode(config)));
  fprintf(cfgfile, "fellow.gfx_refresh=%u\n", cfgGetScreenRefresh(config));
  fprintf(cfgfile, "gfx_colour_mode=%s\n", cfgGetColorBitsToString(cfgGetScreenColorBits(config)));
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
  fprintf(cfgfile, "filesystem_device_name_prefix=%s\n", cfgGetFilesystemDeviceNamePrefix(config));
  for (uint32_t i = 0; i < cfgGetHardfileCount(config); i++)
  {
    cfg_hardfile hf = cfgGetHardfile(config, i);
    fprintf(cfgfile, "hardfile=%s,%u,%u,%u,%u,%s\n", (hf.readonly) ? "ro" : "rw", hf.sectorspertrack, hf.surfaces, hf.reservedblocks, hf.bytespersector, hf.filename);
  }
  for (uint32_t i = 0; i < cfgGetFilesystemCount(config); i++)
  {
    cfg_filesys fs = cfgGetFilesystem(config, i);
    fprintf(cfgfile, "filesystem=%s,%s:%s\n", (fs.readonly) ? "ro" : "rw", fs.volumename, fs.rootpath);
  }
  return TRUE;
}

void cfgUpgradeLegacyConfigToCurrentVersion(cfg *config)
{
  //  New options:
  //  ------------
  //  config_version = <integer>
  //  * Current file version is 2.

  //  gfx_driver = <text>
  //  * Values: "directdraw" (default if missing) and "direct3d11".

  //  gfx_clip_left = <integer>
  //  * Lores cylinder for left output border.

  //  gfx_clip_top = <integer>
  //  * Lores line number for top output border.

  //  gfx_clip_right = <integer>
  //  * Lores cylinder for right output border.

  //  gfx_clip_bottom = <integer>
  //  * Lores line number for bottom output border.

  //  Extended options:
  //  -----------------
  //  gfx_display_scale = <text>
  //  * Added : "auto", "triple" and "quadruple".

  // Scale
  if (cfgGetDisplayScale(config) != DISPLAYSCALE::DISPLAYSCALE_1X && cfgGetDisplayScale(config) != DISPLAYSCALE::DISPLAYSCALE_2X)
  {
    cfgSetDisplayScale(config, DISPLAYSCALE::DISPLAYSCALE_1X);
  }

  // Set max pal clip, with scale limited to 1X or 2X, the actual integer clip values will be adjusted perfectly once the emulator starts up
  cfgSetClipLeft(config, 88);
  cfgSetClipTop(config, 26);
  cfgSetClipRight(config, 472);
  cfgSetClipBottom(config, 314);

  cfgSetConfigFileVersion(config, CONFIG_CURRENT_FILE_VERSION);
}

void cfgUpgradeConfig(cfg *config)
{
  if (cfgGetConfigFileVersion(config) < CONFIG_CURRENT_FILE_VERSION)
  {
    cfgUpgradeLegacyConfigToCurrentVersion(config);
  }
}

/*============================================================================*/
/* Remove unwanted newline chars on the end of a string                       */
/*============================================================================*/

static void cfgStripTrailingNewlines(char *line)
{
  size_t length = strlen(line);
  while ((length > 0) && ((line[length - 1] == '\n') || (line[length - 1] == '\r')))
  {
    line[--length] = '\0';
  }
}

/*============================================================================*/
/* Load configuration from file                                               */
/*============================================================================*/

static bool cfgLoadFromFile(cfg *config, FILE *cfgfile)
{
  char line[256];
  while (!feof(cfgfile))
  {
    const char *result = fgets(line, 256, cfgfile);
    if (result != nullptr)
    {
      cfgStripTrailingNewlines(line);
      cfgSetOption(config, line);
    }
  }
  cfgSetConfigAppliedOnce(config, true);
  return true;
}

bool cfgLoadFromFilename(cfg *config, const char *filename, const bool bIsPreset)
{
  char newfilename[CFG_FILENAME_LENGTH];

  _core.Fileops->ResolveVariables(filename, newfilename);

  if (!bIsPreset)
  {
    _core.Log->AddLog("cfg: loading configuration filename %s...\n", filename);

    // remove existing hardfiles
    cfgHardfilesFree(config);
    cfgFilesystemsFree(config);

    // Full load from file, make sure we don't inherit the old file version number
    cfgSetConfigFileVersion(config, 0);
  }

  FILE *cfgfile = fopen(newfilename, "r");
  bool result = (cfgfile != nullptr);
  if (result)
  {
    result = cfgLoadFromFile(config, cfgfile);
    fclose(cfgfile);

    if (!bIsPreset && cfgGetConfigFileVersion(config) < CONFIG_CURRENT_FILE_VERSION)
    {
      _core.Log->AddLog("cfg: Upgrading config from old version.\n");
      cfgUpgradeConfig(config);
    }
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

BOOLE cfgSaveToFilename(cfg *config, const char *filename)
{
  FILE *cfgfile = fopen(filename, "w");
  BOOLE result = (cfgfile != nullptr);
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

static BOOLE cfgParseCommandLine(cfg *config, int argc, const char *argv[])
{
  int i;

  _core.Log->AddLog("cfg: list of commandline parameters: ");
  for (i = 1; i < argc; i++)
  {
    _core.Log->AddLog("%s ", argv[i]);
  }
  _core.Log->AddLog("\n");

  i = 1;
  while (i < argc)
  {
    // _core.Log->AddLog("cfg: parsing parameter: %s\n", argv[i]);
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
        _core.Log->AddLog("cfg: RetroPlatform data path: %s\n", argv[i]);
      }
      i++;
    }
#ifndef FELLOW_SUPPORT_RP_API_VERSION_71
    else if (stricmp(argv[i], "-rpescapekey") == 0)
    {
      i++;
      if (i < argc)
      {
        uint32_t lEscapeKey = atoi(argv[i]);
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
#endif
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
#endif
    else if (stricmp(argv[i], "-f") == 0)
    { /* Load configuration file */
      i++;
      if (i < argc)
      {
        _core.Log->AddLog("cfg: configuration file: %s\n", argv[i]);
        if (!cfgLoadFromFilename(config, argv[i], false))
        {
          _core.Log->AddLog("cfg: ERROR using -f option, failed reading configuration file %s\n", argv[i]);
        }
        i++;
      }
      else
      {
        _core.Log->AddLog("cfg: ERROR using -f option, please supply a filename\n");
      }
    }
    else if (stricmp(argv[i], "-autosnap") == 0)
    {
      i++;
      automator.SnapshotDirectory = argv[i];
      automator.SnapshotEnable = true;
      i++;
    }
    else if (stricmp(argv[i], "-autosnapfrequency") == 0)
    {
      i++;
      automator.SnapshotFrequency = atoi(argv[i]);
      i++;
    }
    else if (stricmp(argv[i], "-autoscript") == 0)
    {
      i++;
      automator.ScriptFilename = argv[i];
      i++;
    }
    else if (stricmp(argv[i], "-autorecord") == 0)
    {
      i++;
      automator.RecordScript = true;
    }
    else if (stricmp(argv[i], "-s") == 0)
    { /* Configuration option */
      i++;
      if (i < argc)
      {
        if (!cfgSetOption(config, argv[i])) _core.Log->AddLog("cfg: -s option, unrecognized setting %s\n", argv[i]);
        i++;
      }
      else
      {
        _core.Log->AddLog("cfg: -s option, please supply a configuration setting\n");
      }
    }
    else
    { /* unrecognized configuration option */
      _core.Log->AddLog("cfg: parameter %s not recognized.\n", argv[i]);
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
  newconfig->m_filesystems = listCopy(configmanager->m_currentconfig->m_filesystems, sizeof(cfg_filesys));
  newconfig->m_hardfiles = listCopy(configmanager->m_currentconfig->m_hardfiles, sizeof(cfg_hardfile));

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
  uint32_t i;
  BOOLE needreset = FALSE;

  /*==========================================================================*/
  /* Floppy configuration                                                     */
  /*==========================================================================*/

  for (i = 0; i < 4; i++)
  {
    floppySetEnabled(i, cfgGetDiskEnabled(config, i));
    floppySetDiskImage(i, cfgGetDiskImage(config, i));
    floppySetReadOnlyConfig(i, cfgGetDiskReadOnly(config, i));
  }
  floppySetFastDMA(cfgGetDiskFast(config));

  /*==========================================================================*/
  /* Memory configuration                                                     */
  /*==========================================================================*/

  needreset |= memorySetUseAutoconfig(cfgGetUseAutoconfig(config)) == true;
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
#ifdef RETRO_PLATFORM
  // internal clipping values in RetroPlatform headless mode are configured by
  // RetroPlatform::RegisterRetroPlatformScreenMode, skip configuration
  if (!RP.GetHeadlessMode())
#endif
    drawSetInternalClip(draw_rect(cfgGetClipLeft(config), cfgGetClipTop(config), cfgGetClipRight(config), cfgGetClipBottom(config)));
  drawSetOutputClip(draw_rect(cfgGetClipLeft(config), cfgGetClipTop(config), cfgGetClipRight(config), cfgGetClipBottom(config)));
  drawSetDisplayScale(cfgGetDisplayScale(config));
  drawSetDisplayScaleStrategy(cfgGetDisplayScaleStrategy(config));
  drawSetAllowMultipleBuffers(cfgGetUseMultipleGraphicalBuffers(config));
  drawSetDeinterlace(cfgGetDeinterlace(config));
  drawSetDisplayDriver(cfgGetDisplayDriver(config));
  drawSetGraphicsEmulationMode(cfgGetGraphicsEmulationMode(config));

  if (cfgGetScreenWindowed(config))
  {
    drawSetWindowedMode(cfgGetScreenWidth(config), cfgGetScreenHeight(config));
  }
  else
  {
    drawSetFullScreenMode(cfgGetScreenWidth(config), cfgGetScreenHeight(config), cfgGetScreenColorBits(config), cfgGetScreenRefresh(config));
  }

  /*==========================================================================*/
  /* Sound configuration                                                      */
  /*==========================================================================*/

  _core.Sound->SetEmulation(cfgGetSoundEmulation(config));
  _core.Sound->SetRate(cfgGetSoundRate(config));
  _core.Sound->SetIsStereo(cfgGetSoundStereo(config));
  _core.Sound->SetIs16Bits(cfgGetSound16Bits(config));
  _core.Sound->SetFilter(cfgGetSoundFilter(config));
  _core.Sound->SetVolume(cfgGetSoundVolume(config));
  _core.Sound->SetWAVDump(cfgGetSoundWAVDump(config));
  _core.Sound->SetNotification(cfgGetSoundNotification(config));
  _core.Sound->SetBufferLength(cfgGetSoundBufferLength(config));

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

  if (cfgGetUseAutoconfig(config) != _core.HardfileHandler->GetEnabled())
  {
    needreset = TRUE;
    _core.HardfileHandler->Clear();
    _core.HardfileHandler->SetEnabled(cfgGetUseAutoconfig(config));
  }
  if (_core.HardfileHandler->GetEnabled())
  {
    for (i = 0; i < cfgGetHardfileCount(config); i++)
    {
      HardfileConfiguration fhardfile;
      cfg_hardfile hardfile = cfgGetHardfile(config, i);
      fhardfile.Geometry.BytesPerSector = hardfile.bytespersector;
      fhardfile.Readonly = hardfile.readonly;
      fhardfile.Geometry.ReservedBlocks = hardfile.reservedblocks;
      fhardfile.Geometry.SectorsPerTrack = hardfile.sectorspertrack;
      fhardfile.Geometry.Surfaces = hardfile.surfaces;
      fhardfile.Filename = hardfile.filename;

      if (!_core.HardfileHandler->CompareHardfile(fhardfile, i))
      {
        needreset = TRUE;
        _core.HardfileHandler->SetHardfile(fhardfile, i);
      }
    }
    const unsigned int maxDeviceCount = _core.HardfileHandler->GetMaxHardfileCount();
    for (unsigned int i = cfgGetHardfileCount(config); i < maxDeviceCount; i++)
    {
      needreset |= (_core.HardfileHandler->RemoveHardfile(i) == true);
    }
  }

  /*==========================================================================*/
  /* Filesystem configuration                                                 */
  /*==========================================================================*/

  if ((cfgGetUseAutoconfig(config) != ffilesysGetEnabled()) || (cfgGetFilesystemAutomountDrives(config) != ffilesysGetAutomountDrives()))
  {
    needreset = TRUE;
    ffilesysClear();
    ffilesysSetEnabled(cfgGetUseAutoconfig(config));
  }

  if (ffilesysGetEnabled())
  {
    for (i = 0; i < cfgGetFilesystemCount(config); i++)
    {
      ffilesys_dev ffilesys;
      cfg_filesys filesys = cfgGetFilesystem(config, i);
      strncpy(ffilesys.volumename, filesys.volumename, FFILESYS_MAX_VOLUMENAME);
      strncpy(ffilesys.rootpath, filesys.rootpath, CFG_FILENAME_LENGTH);
      ffilesys.readonly = filesys.readonly;
      ffilesys.status = ffilesys_status::FFILESYS_INSERTED;
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
    ffilesysSetDeviceNamePrefix(cfgGetFilesystemDeviceNamePrefix(config));
  }

  /*==========================================================================*/
  /* Game port configuration                                                  */
  /*==========================================================================*/

  gameportSetInput(0, cfgGetGameport(config, 0));
  gameportSetInput(1, cfgGetGameport(config, 1));

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

void cfgManagerStartup(cfgManager *configmanager, int argc, const char *argv[])
{
  cfg *config = cfgManagerGetNewConfig(configmanager);
  cfgParseCommandLine(config, argc, argv);
#ifdef RETRO_PLATFORM
  if (!RP.GetHeadlessMode())
  {
#endif
    if (!cfgGetConfigAppliedOnce(config))
    {
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

void cfgStartup(int argc, const char **argv)
{
  cfgManagerStartup(&cfg_manager, argc, argv);
}

void cfgShutdown()
{
  cfgManagerShutdown(&cfg_manager);
}
