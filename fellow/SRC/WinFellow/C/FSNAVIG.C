/* @(#) $Id: FSNAVIG.C,v 1.6 2009-07-25 03:09:00 peschau Exp $ */
/*=========================================================================*/
/* Fellow                                                                  */
/* Filesystem navigation                                                   */
/*                                                                         */
/* Author: Petter Schau                                                    */
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

void fsNavigInitializeInstallDir(char *cmd) {
  char *locc;
  char tmpname[FS_WRAP_MAX_PATH_LENGTH];

  fsWrapFullPath(tmpname, cmd);
  locc = strrchr(tmpname, FS_WRAP_PATH_SEPARATOR_CHAR);
  if (locc == NULL) strcpy(tmpname, ".");
  else *locc = '\0';
  fs_navig_install_dir = fsWrapMakePoint(tmpname);
}


/*===========================================================================*/
/* Turns the name path into a relative path                                  */
/*===========================================================================*/

//void fsNavigMakeRelativePath(char *name) {
//  fsWrapMakeRelativePath(fs_navig_install_dir->name, name);
//}


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

