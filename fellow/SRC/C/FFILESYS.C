/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Filesystem wrapper                                                         */
/*                                                                            */
/* Authors: Petter Schau (peschau@online.no)                                  */
/*          (Wraps code that originates in the UAE project.)                  */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

#include "portable.h"
#include "renaming.h"

#include "defs.h"
#include "fellow.h"
#include "ffilesys.h"

#ifdef UAE_FILESYS
#include "uaefsys.h"
#endif

extern char *fellow_add_filesys_unit(char *volname,
			             char *rootdir, 
			             int readonly,
			             int removable);


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

BOOLE ffilesysRemoveFilesys(ULO index) {
  BOOLE result = FALSE;

  if (index >= FFILESYS_MAX_DEVICES) return result;
  result = (ffilesys_devs[index].status == FFILESYS_INSERTED);
  memset(&(ffilesys_devs[index]), 0, sizeof(ffilesys_dev));
  ffilesys_devs[index].status = FFILESYS_NONE;
  return result;
}


void ffilesysSetEnabled(BOOLE enabled) {
  ffilesys_enabled = enabled;
}


BOOLE ffilesysGetEnabled(void) {
  return ffilesys_enabled;
}


void ffilesysSetFilesys(ffilesys_dev filesys, ULO index) {
  if (index >= FFILESYS_MAX_DEVICES) return;
  ffilesysRemoveFilesys(index);
  ffilesys_devs[index] = filesys;
}


BOOLE ffilesysCompareFilesys(ffilesys_dev filesys, ULO index) {
  if (index >= FFILESYS_MAX_DEVICES) return FALSE;
  return (ffilesys_devs[index].readonly == filesys.readonly) &&
	 (strncmp(ffilesys_devs[index].volumename, filesys.volumename, FFILESYS_MAX_VOLUMENAME) == 0) &&
	 (strncmp(ffilesys_devs[index].rootpath, filesys.rootpath, CFG_FILENAME_LENGTH) == 0);
}


void ffilesysSetAutomountDrives(BOOLE automount_drives) {
  ffilesys_automount_drives = automount_drives;
}


BOOLE ffilesysGetAutomountDrives(void) {
  return ffilesys_automount_drives;
}

static BOOLE ffilesysHasZeroDevices(void) {
  ULO i;
  ULO dev_count = 0;

  for (i = 0; i < FFILESYS_MAX_DEVICES; i++)
	/* warning, ugly check due to ffilesys_devs[i].status not being set
	   correctly  */
    //if (ffilesys_devs[i].status == FFILESYS_INSERTED) dev_count++;
	if((ffilesys_devs[i].volumename[0] != '\0') &&
	   (ffilesys_devs[i].rootpath[0] != '\0')) dev_count++;
  return (dev_count == 0) && !ffilesysGetAutomountDrives();
}

void ffilesysClear(void) {
  ULO i;

  for (i = 0; i < FFILESYS_MAX_DEVICES; i++) ffilesysRemoveFilesys(i);
}


/*============================================================================*/
/* Check out configuration                                                    */
/*============================================================================*/

void ffilesysDumpConfig(void) {
  ULO i;
  FILE *F = fopen("c:\\fsysdump.txt", "w");
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

void ffilesysInstall(void) {
  ULO i;

  for (i = 0; i < FFILESYS_MAX_DEVICES; i++)
	/* again ugly check */
	if((ffilesys_devs[i].volumename[0] != '\0') &&
	   (ffilesys_devs[i].rootpath[0] != '\0'))
      fellow_add_filesys_unit(ffilesys_devs[i].volumename,
			    ffilesys_devs[i].rootpath,
			    ffilesys_devs[i].readonly,
			    FALSE);
}


/*============================================================================*/
/* Reset filesys device                                                       */
/*============================================================================*/

void ffilesysHardReset(void) {
#ifdef UAE_FILESYS
  if ((!ffilesysHasZeroDevices()) &&
      ffilesysGetEnabled() && 
      (memoryGetKickImageVersion() > 36)) {
    rtarea_init();  /* Sets up a lot of traps */
    hardfile_install();
    filesys_install(); /* Sets some traps and information in the trap memory area */
    filesys_init(ffilesysGetAutomountDrives());  /* Mounts all Windows drives as filesystems */
    filesys_prepare_reset(); /* Cleans up mounted filesystems(?) */
    filesys_reset(); /* More cleaning up(?) */
    ffilesysInstall(); /* Install user defined filesystems */
    filesys_start_threads(); /* Installs registersed filesystems mounts */
    memoryEmemCardAdd(expamem_init_filesys, expamem_map_filesys);
  }
#endif
}


/*============================================================================*/
/* Start filesys device for actual emulation                                  */
/*============================================================================*/

void ffilesysEmulationStart(void) {
  ffilesysDumpConfig();
  if ((!ffilesysHasZeroDevices()) &&
      ffilesysGetEnabled() && 
      (memoryGetKickImageVersion() > 36))
    rtarea_setup(); /* Maps the trap memory area into memory */
}



/*============================================================================*/
/* Stop filesys device after actual emulation                                 */
/*============================================================================*/

void ffilesysEmulationStop(void) {
}


/*============================================================================*/
/* Startup filesys device                                                     */
/*============================================================================*/

void ffilesysStartup(void) {
  ffilesysClear();
  ffilesysSetAutomountDrives(FALSE);
}


/*============================================================================*/
/* Shutdown filesys device                                                    */
/*============================================================================*/

void ffilesysShutdown(void) {
}

