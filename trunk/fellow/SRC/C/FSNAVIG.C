/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Filesystem navigation                                                      */
/*                                                                            */
/* Author: Petter Schau (peschau@online.no)                                   */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

#include <string.h>

#include "defs.h"
#include "portable.h"
#include "fswrap.h"

/*===========================================================================*/
/* Module variables                                                          */
/*===========================================================================*/

fs_navig_point *fs_navig_install_dir;
fs_navig_point *fs_navig_startup_dir;


/*===========================================================================*/
/* Returns TRUE if the local filesystem is drive based                       */
/*===========================================================================*/

BOOLE fsNavigHasDrives(void) {
  return fsWrapHasDrives();
}


/*===========================================================================*/
/* Returns a BOOLE [27] array with available drives                          */
/*===========================================================================*/

BOOLE *fsNavigGetDriveMap(void) {
  return fsWrapGetDriveMap();
}


/*===========================================================================*/
/* Finds Fellow startup-directory                                            */
/*===========================================================================*/

void fsNavigInitializeStartupDir(void) {
  fs_navig_startup_dir = fsWrapGetCWD();
}


/*===========================================================================*/
/* Find Fellow install-directory                                             */
/*===========================================================================*/

void fsNavigInitializeInstallDir(STR *cmd) {
  STR *locc;
  STR tmpname[FS_WRAP_MAX_PATH_LENGTH];

  fsWrapFullPath(tmpname, cmd);
  locc = strrchr(tmpname, FS_WRAP_PATH_SEPARATOR_CHAR);
  if (locc == NULL) strcpy(tmpname, ".");
  else *locc = '\0';
  fs_navig_install_dir = fsWrapMakePoint(tmpname);
}


/*===========================================================================*/
/* Turns the name path into a relative path                                  */
/*===========================================================================*/

void fsNavigMakeRelativePath(STR *name) {
  fsWrapMakeRelativePath(fs_navig_install_dir->name, name);
}


/*===========================================================================*/
/* Get current dir                                                           */
/*===========================================================================*/

fs_navig_point *fsNavigGetCWD(void) {
  return fsWrapGetCWD();
}


/*===========================================================================*/
/* Set current dir                                                           */
/*===========================================================================*/

void fsNavigSetCWD(fs_navig_point *fs_point) {
  fsWrapSetCWD(fs_point);
}

/*===========================================================================*/
/* Set current dir to install dir                                            */
/*===========================================================================*/

void fsNavigSetCWDInstallDir(void) {
  fsNavigSetCWD(fs_navig_install_dir);
}


/*===========================================================================*/
/* Set current dir to startup dir                                            */
/*===========================================================================*/

void fsNavigSetCWDStartupDir(void) {
  fsNavigSetCWD(fs_navig_startup_dir);
}


/*===========================================================================*/
/* Returns a list of directory entries in the specified dir                  */
/*===========================================================================*/

felist *fsNavigGetDirList(fs_navig_point *fs_point)
{
  fs_navig_point *fsnpnode;
  felist *l = NULL;

  if (fsWrapOpenDir(fs_point))
    while ((fsnpnode = fsWrapReadDir()) != NULL)
      l = listAddLast(l, listNew(fsnpnode));
  return l;
}


/*===========================================================================*/
/* Filesystem navigator startup                                              */
/*===========================================================================*/

void fsNavigStartup(char *argv[]) {
  fsNavigInitializeStartupDir();
  fsNavigInitializeInstallDir(argv[0]);
}


/*===========================================================================*/
/* Filesystem navigator shutdown                                             */
/*===========================================================================*/

void fsNavigShutdown(void) {
  if (fs_navig_install_dir != NULL)
    free(fs_navig_install_dir);
  if (fs_navig_startup_dir != NULL)
    free(fs_navig_startup_dir);
  
}

