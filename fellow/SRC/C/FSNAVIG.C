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


/*==========================================================================*/
/* Access to files happens relative to the fellow homedirectory if the file */
/* is on the same drive                                                     */
/*==========================================================================*/

void fsNavigDecomposePath(STR *decomposed[], STR *str) {
  LON i, until, j = 0;
  BOOLE ended = FALSE;

  if (fsNavigHasDrives())
    i = until = 3;
  else
    i = until = 1;
  if (str[i] == '\0')
    decomposed[0] = NULL;
  else {
    while (!ended) {
      decomposed[j] = &str[i];
      while (str[until] != '\0' && str[until] != FS_WRAP_PATH_SEPARATOR_CHAR)
	until++;
      if (str[until] == '\0')
	ended = TRUE;
      else
	str[until] = '\0';
      until++;
      j++;
      i = until;
    }
    decomposed[j] = NULL;
  }
}
  
/*===================================================================*/
/* How this works:                                                   */
/* As long as hpath and npath is the same, call recursively with one */
/* element less in the path arrays                                   */
/* then build relative path from remaining elements                  */
/*===================================================================*/

void fsNavigBuildRelativePath(STR *hpath[], STR *npath[], STR *buf) {
  if (hpath[0] == NULL) { /* Bottom of hpath, name is below home */
    LON i = 0;
    while (npath[i] != NULL) {
      strcat(buf, npath[i]);
      i++;
      if (npath[i] != NULL)
	strcat(buf, FS_WRAP_PATH_SEPARATOR_STR);
    }
  }
  else if (stricmp(hpath[0], npath[0]) == 0) /* path element same, take away */
    fsNavigBuildRelativePath(&hpath[1], &npath[1], buf);
  else {
    /* Path no longer the same, add .. for each hpath left, and add */
    /* remaining part of npath */
    LON i = 0;

    while (hpath[i++] != NULL)
      strcat(buf, ".." FS_WRAP_PATH_SEPARATOR_STR);
    i = 0;
    while (npath[i] != NULL) {
      strcat(buf, npath[i]);
      i++;
      if (npath[i] != NULL)
	strcat(buf, FS_WRAP_PATH_SEPARATOR_STR);
    }
  }
}


/*===========================================================================*/
/* Turns the name path into a relative path                                  */
/*===========================================================================*/

void fsNavigMakeRelativePath(STR *name) {
  static STR tmpname[FS_WRAP_MAX_PATH_LENGTH];
  static STR tmphome[FS_WRAP_MAX_PATH_LENGTH];
  static STR *homedecomposed[128];
  static STR *namedecomposed[128];
  
  /* Expand to full path */

  fsWrapFullPath(name, tmpname);
  strcpy(tmphome, fs_navig_install_dir->name);
  fsNavigDecomposePath(homedecomposed, tmphome);
  fsNavigDecomposePath(namedecomposed, tmpname);
  name[0] = '\0';
  fsNavigBuildRelativePath(homedecomposed, namedecomposed, name);
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

