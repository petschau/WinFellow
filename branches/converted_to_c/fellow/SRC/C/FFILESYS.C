/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Filesystem wrapper                                                         */
/*                                                                            */
/* Authors: Petter Schau (peschau@online.no)                                  */
/*          Torsten Enderling (carfesh@gmx.net)                               */
/*          (Wraps code that originates in the UAE project.)                  */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

/*============================================================================*/
/* Changelog:                                                                 */
/* ==========                                                                 */
/* 06/11/2001: another bug in ffilesysCompareFilesys fixed that occured when  */
/*             using trailing backslashes                                     */
/* 09/19/2000: fixed bug in ffilesysInstall that caused chaos when using      */
/*             trailing backslashs in filesystem root paths                   */
/* 09/01/2000: removed use of fellow_add_filesys_unit; redesigned to use the  */
/*             UAE add_filesys_unit                                           */
/* 07/11/2000: fixed up checks using filesys device status                    */
/*============================================================================*/

#include "defs.h"
#include "fellow.h"
#include "ffilesys.h"

#ifdef UAE_FILESYS
#include "filesys.h"
#endif

/*============================================================================*/
/* Filesys device data                                                        */
/*============================================================================*/

ffilesys_dev ffilesys_devs[FFILESYS_MAX_DEVICES];
BOOLE ffilesys_enabled;
BOOLE ffilesys_automount_drives;

/*============================================================================*/
/* Filesys device configuration                                               */
/*============================================================================*/

/* Returns TRUE if a hardfile was inserted */

BOOLE ffilesysRemoveFilesys(ULO index)
{
  BOOLE result = FALSE;

  if (index >= FFILESYS_MAX_DEVICES)
    return result;
  result = (ffilesys_devs[index].status == FFILESYS_INSERTED);
  memset(&(ffilesys_devs[index]), 0, sizeof(ffilesys_dev));
  ffilesys_devs[index].status = FFILESYS_NONE;
  return result;
}

void ffilesysSetEnabled(BOOLE enabled)
{
  ffilesys_enabled = enabled;
}

BOOLE ffilesysGetEnabled(void)
{
  return ffilesys_enabled;
}

void ffilesysSetFilesys(ffilesys_dev filesys, ULO index)
{
  if (index >= FFILESYS_MAX_DEVICES)
    return;
  ffilesysRemoveFilesys(index);
  ffilesys_devs[index] = filesys;
}

BOOLE ffilesysCompareFilesys(ffilesys_dev filesys, ULO index)
{
  size_t len;

  if (index >= FFILESYS_MAX_DEVICES)
    return FALSE;

  len = strlen(filesys.rootpath) - 1;
  if (filesys.rootpath[len] == '\\')
	filesys.rootpath[len] = '\0';

  return (ffilesys_devs[index].readonly == filesys.readonly) &&
    (strncmp
     (ffilesys_devs[index].volumename, filesys.volumename,
      FFILESYS_MAX_VOLUMENAME) == 0) &&
    (strncmp
     (ffilesys_devs[index].rootpath, filesys.rootpath,
      CFG_FILENAME_LENGTH) == 0);
}

void ffilesysSetAutomountDrives(BOOLE automount_drives)
{
  ffilesys_automount_drives = automount_drives;
}

BOOLE ffilesysGetAutomountDrives(void)
{
  return ffilesys_automount_drives;
}

static BOOLE ffilesysHasZeroDevices(void)
{
  ULO i;
  ULO dev_count = 0;

  for (i = 0; i < FFILESYS_MAX_DEVICES; i++)
    if (ffilesys_devs[i].status == FFILESYS_INSERTED)
      dev_count++;
  return (dev_count == 0) && !ffilesysGetAutomountDrives();
}

void ffilesysClear(void)
{
  ULO i;

  for (i = 0; i < FFILESYS_MAX_DEVICES; i++)
    ffilesysRemoveFilesys(i);
}

/*============================================================================*/
/* Check out configuration                                                    */
/*============================================================================*/

void ffilesysDumpConfig(void)
{
  ULO i;
  FILE *F = fopen("fsysdump.txt", "w");
  for (i = 0; i < FFILESYS_MAX_DEVICES; i++) {
    if (ffilesys_devs[i].status == FFILESYS_INSERTED)
      fprintf(F, "Slot: %d, %s, %s, %s\n",
	      i,
	      ffilesys_devs[i].volumename,
	      ffilesys_devs[i].rootpath,
	      (ffilesys_devs[i].readonly) ? "R" : "RW");
    else
      fprintf(F, "Slot: %d, No filesystem defined.\n", i);
  }
  fclose(F);
}

/*============================================================================*/
/* Install the user defined Reset filesys device                                                       */
/*============================================================================*/

void ffilesysInstall(void)
{
  ULO i;
  size_t len;

  for (i = 0; i < FFILESYS_MAX_DEVICES; i++)
    if (ffilesys_devs[i].status == FFILESYS_INSERTED) {
      len = strlen(ffilesys_devs[i].rootpath) - 1;
      if (ffilesys_devs[i].rootpath[len] == '\\') {
	ffilesys_devs[i].rootpath[len] = '\0';
      }
      add_filesys_unit(&mountinfo,
		       ffilesys_devs[i].volumename,
		       ffilesys_devs[i].rootpath,
		       ffilesys_devs[i].readonly, 0, 0, 0, 0);
    }
}

/*============================================================================*/
/* Reset filesys device                                                       */
/*============================================================================*/

void ffilesysHardReset(void)
{
#ifdef UAE_FILESYS
  if ((!ffilesysHasZeroDevices()) &&
      ffilesysGetEnabled() && (memoryGetKickImageVersion() > 36)) {
	rtarea_setup();		/* Maps the trap memory area into memory */
    rtarea_init();		/* Sets up a lot of traps */
    hardfile_install();
    filesys_install();		/* Sets some traps and information in the trap memory area */
    filesys_init(ffilesysGetAutomountDrives());	/* Mounts all Windows drives as filesystems */
    filesys_prepare_reset();	/* Cleans up mounted filesystems(?) */
    filesys_reset();		/* More cleaning up(?) */
    ffilesysInstall();		/* Install user defined filesystems */
    filesys_start_threads();	/* Installs registersed filesystems mounts */
    memoryEmemCardAdd(expamem_init_filesys, expamem_map_filesys);
  }
#endif
}

/*============================================================================*/
/* Start filesys device for actual emulation                                  */
/*============================================================================*/

void ffilesysEmulationStart(void)
{
#ifdef _DEBUG
  ffilesysDumpConfig();
#endif
}

/*============================================================================*/
/* Stop filesys device after actual emulation                                 */
/*============================================================================*/

void ffilesysEmulationStop(void)
{
	/*
	filesys_prepare_reset();
	filesys_reset();	
	*/
}

/*============================================================================*/
/* Startup filesys device                                                     */
/*============================================================================*/

void ffilesysStartup(void)
{
  ffilesysClear();
  ffilesysSetAutomountDrives(FALSE);
}

/*=======================================*/
/* clean up rests from copy of mountinfo */
/*=======================================*/

void ffilesysClearMountinfo(void)
{
  for(mountinfo.num_units; mountinfo.num_units>0; mountinfo.num_units--)
  {
    if(mountinfo.ui[mountinfo.num_units-1].volname) 
    {
      free(mountinfo.ui[mountinfo.num_units-1].volname);
      mountinfo.ui[mountinfo.num_units-1].volname = NULL;
    }
    if(mountinfo.ui[mountinfo.num_units-1].rootdir) 
    {
      free(mountinfo.ui[mountinfo.num_units-1].rootdir);
      mountinfo.ui[mountinfo.num_units-1].rootdir = NULL;
    }
  }
}

/*============================================================================*/
/* Shutdown filesys device                                                    */
/*============================================================================*/

void ffilesysShutdown(void)
{
  filesys_prepare_reset();
  filesys_reset();
  ffilesysClearMountinfo();
}
