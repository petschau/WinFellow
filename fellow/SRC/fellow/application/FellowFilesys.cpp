/*=========================================================================*/
/* Fellow                                                                  */
/* Filesystem wrapper                                                      */
/*                                                                         */
/* Authors: Petter Schau                                                   */
/*          Torsten Enderling (carfesh@gmx.net)                            */
/*          (Wraps code that originates in the UAE project.)               */
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

#include "fellow/api/defs.h"
#include "fellow/application/FellowFilesys.h"
#include "fellow/api/Services.h"
#include "fellow/os/windows/uae/FILESYS.H"

#include <string>

using namespace std;
using namespace fellow::api;

/*============================================================================*/
/* Filesys device data                                                        */
/*============================================================================*/

ffilesys_dev ffilesys_devs[FFILESYS_MAX_DEVICES];
bool ffilesys_enabled;
BOOLE ffilesys_automount_drives;
string ffilesys_device_name_prefix;

/*============================================================================*/
/* Filesys device configuration                                               */
/*============================================================================*/

/* Returns true if a hardfile was inserted */

bool ffilesysRemoveFilesys(ULO index)
{
  bool result = false;

  if (index >= FFILESYS_MAX_DEVICES) return result;
  result = (ffilesys_devs[index].status == FFILESYS_INSERTED);
  memset(&(ffilesys_devs[index]), 0, sizeof(ffilesys_dev));
  ffilesys_devs[index].status = FFILESYS_NONE;
  return result;
}

void ffilesysSetEnabled(bool enabled)
{
  ffilesys_enabled = enabled;
}

bool ffilesysGetEnabled()
{
  return ffilesys_enabled;
}

void ffilesysSetFilesys(ffilesys_dev filesys, ULO index)
{
  if (index >= FFILESYS_MAX_DEVICES) return;
  ffilesysRemoveFilesys(index);
  ffilesys_devs[index] = filesys;
}

BOOLE ffilesysCompareFilesys(ffilesys_dev filesys, ULO index)
{
  if (index >= FFILESYS_MAX_DEVICES) return FALSE;

  size_t len = strlen(filesys.rootpath) - 1;
  if (filesys.rootpath[len] == '\\') filesys.rootpath[len] = '\0';

  return (ffilesys_devs[index].readonly == filesys.readonly) && (strncmp(ffilesys_devs[index].volumename, filesys.volumename, FFILESYS_MAX_VOLUMENAME) == 0) &&
         (strncmp(ffilesys_devs[index].rootpath, filesys.rootpath, CFG_FILENAME_LENGTH) == 0);
}

void ffilesysSetAutomountDrives(BOOLE automount_drives)
{
  ffilesys_automount_drives = automount_drives;
}

BOOLE ffilesysGetAutomountDrives()
{
  return ffilesys_automount_drives;
}

void ffilesysSetDeviceNamePrefix(const string &prefix)
{
  ffilesys_device_name_prefix = prefix;
}

const string &ffilesysGetDeviceNamePrefix()
{
  return ffilesys_device_name_prefix;
}

static BOOLE ffilesysHasZeroDevices()
{
  ULO dev_count = 0;

  for (ULO i = 0; i < FFILESYS_MAX_DEVICES; i++)
    if (ffilesys_devs[i].status == FFILESYS_INSERTED) dev_count++;
  return (dev_count == 0) && !ffilesysGetAutomountDrives();
}

void ffilesysClear()
{
  for (ULO i = 0; i < FFILESYS_MAX_DEVICES; i++)
    ffilesysRemoveFilesys(i);
}

/*============================================================================*/
/* Check out configuration                                                    */
/*============================================================================*/

void ffilesysDumpConfig()
{
  char filename[CFG_FILENAME_LENGTH];

  Service->Fileops.GetGenericFileName(filename, "WinFellow", "fsysdump.txt");
  FILE *F = fopen(filename, "w");
  for (ULO i = 0; i < FFILESYS_MAX_DEVICES; i++)
  {
    if (ffilesys_devs[i].status == FFILESYS_INSERTED)
      fprintf(F, "Slot: %u, %s, %s, %s\n", i, ffilesys_devs[i].volumename, ffilesys_devs[i].rootpath, ffilesys_devs[i].readonly ? "R" : "RW");
    else
      fprintf(F, "Slot: %u, No filesystem defined.\n", i);
  }
  fclose(F);
}

/*============================================================================*/
/* Install the user defined Reset filesys device                              */
/*============================================================================*/

void ffilesysInstall()
{
  for (ULO i = 0; i < FFILESYS_MAX_DEVICES; i++)
    if (ffilesys_devs[i].status == FFILESYS_INSERTED)
    {
      size_t len = strlen(ffilesys_devs[i].rootpath) - 1;
      if (ffilesys_devs[i].rootpath[len] == '\\')
      {
        ffilesys_devs[i].rootpath[len] = '\0';
      }
      add_filesys_unit(&mountinfo, ffilesys_devs[i].volumename, ffilesys_devs[i].rootpath, ffilesys_devs[i].readonly ? TRUE : FALSE, 0, 0, 0, 0);
    }
}

/*============================================================================*/
/* Reset filesys device                                                       */
/*============================================================================*/

void ffilesysHardReset()
{
  if ((!ffilesysHasZeroDevices()) && ffilesysGetEnabled() && (memoryGetKickImageVersion() > 36))
  {
    rtarea_setup(); /* Maps the trap memory area into memory */
    rtarea_init();  /* Sets up a lot of traps */
    hardfile_install();
    filesys_install();                          /* Sets some traps and information in the trap memory area */
    filesys_init(ffilesysGetAutomountDrives()); /* Mounts all Windows drives as filesystems */
    filesys_prepare_reset();                    /* Cleans up mounted filesystems(?) */
    filesys_reset();                            /* More cleaning up(?) */
    ffilesysInstall();                          /* Install user defined filesystems */
    filesys_start_threads();                    /* Installs registered filesystem mounts, this also names the device "<prefix>x" */
    memoryEmemCardAdd(expamem_init_filesys, expamem_map_filesys);
  }
}

/*============================================================================*/
/* Start filesys device for actual emulation                                  */
/*============================================================================*/

void ffilesysEmulationStart()
{
#ifdef _DEBUG
  ffilesysDumpConfig();
#endif
}

/*============================================================================*/
/* Stop filesys device after actual emulation                                 */
/*============================================================================*/

void ffilesysEmulationStop()
{
  /*
  filesys_prepare_reset();
  filesys_reset();
  */
}

/*============================================================================*/
/* Startup filesys device                                                     */
/*============================================================================*/

void ffilesysStartup()
{
  ffilesysClear();
  ffilesysSetAutomountDrives(FALSE);
  ffilesysSetDeviceNamePrefix("FS");
}

/*=======================================*/
/* clean up rests from copy of mountinfo */
/*=======================================*/

void ffilesysClearMountinfo()
{
  for (; mountinfo.num_units > 0; mountinfo.num_units--)
  {
    if (mountinfo.ui[mountinfo.num_units - 1].volname)
    {
      free(mountinfo.ui[mountinfo.num_units - 1].volname);
      mountinfo.ui[mountinfo.num_units - 1].volname = nullptr;
    }
    if (mountinfo.ui[mountinfo.num_units - 1].rootdir)
    {
      free(mountinfo.ui[mountinfo.num_units - 1].rootdir);
      mountinfo.ui[mountinfo.num_units - 1].rootdir = nullptr;
    }
  }
}

/*============================================================================*/
/* Shutdown filesys device                                                    */
/*============================================================================*/

void ffilesysShutdown()
{
  filesys_prepare_reset();
  filesys_reset();
  ffilesysClearMountinfo();
}