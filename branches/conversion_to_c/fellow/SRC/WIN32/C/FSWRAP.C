/*=========================================================================*/
/* Fellow Amiga Emulator                                                   */
/*                                                                         */
/* @(#) $Id: FSWRAP.C,v 1.2.2.4 2004-05-27 09:58:13 carfesh Exp $          */
/*                                                                         */
/* Filesystem browsing wrapper for the GUI                                 */
/*                                                                         */
/* Author: Petter Schau (peschau@online.no)                                */
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

#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <windows.h>
#include <winbase.h>

#include "portable.h"
#include "defs.h"
#include "fswrap.h"


/*===========================================================================*/
/* Data                                                                      */
/*===========================================================================*/

HANDLE fs_wrap_dirent_handle;
WIN32_FIND_DATA fs_wrap_dirent;
BOOLE fs_wrap_dirent_open;
BOOLE fs_wrap_opendir_firsttime;

#include <shlwapi.h>


/*===========================================================================*/
/* Unix has no drives                                                        */
/*===========================================================================*/

BOOLE fsWrapHasDrives(void) {
  return TRUE;
}

/*===========================================================================*/
/* Unix has no drives                                                        */
/*===========================================================================*/

BOOLE *fsWrapGetDriveMap(void) {
  return NULL;
}

/*===========================================================================*/
/* Returns the absolute full path for a filename                             */
/*===========================================================================*/

void fsWrapFullPath(STR *dst, STR *src) {
  _fullpath(dst, src, FS_WRAP_MAX_PATH_LENGTH);
}

/*===========================================================================*/
/* Make relative path                                                        */
/*===========================================================================*/

/*
void fsWrapMakeRelativePath(STR *root_dir, STR *file_path) {
  STR tmpdst[MAX_PATH];
  STR tmpsrc[MAX_PATH];
  if (stricmp(file_path, "") == 0) return;
  fsWrapFullPath(tmpsrc, file_path);
  if (PathRelativePathTo(tmpdst,
		         root_dir,
		         FILE_ATTRIBUTE_DIRECTORY, 
		         tmpsrc, 
		         FILE_ATTRIBUTE_NORMAL))
  {
    strcpy(file_path, tmpdst);
  }
}
*/

/*===========================================================================*/
/* Fills in file attributes in point structure                               */
/* Return NULL on error                                                      */
/*===========================================================================*/

fs_navig_point *fsWrapMakePoint(STR *point) {
  struct stat mystat;
  fs_navig_point *fsnp = NULL;

  if (stat(point, &mystat) == 0) {
    fsnp = (fs_navig_point *) malloc(sizeof(fs_navig_point));
    strcpy(fsnp->name, point);
    if (mystat.st_mode & _S_IFREG)
      fsnp->type = FS_NAVIG_FILE;
    else if (mystat.st_mode & _S_IFDIR)
      fsnp->type = FS_NAVIG_DIR;
    else
      fsnp->type = FS_NAVIG_OTHER;
    fsnp->writeable = !!(mystat.st_mode & _S_IWRITE);
    fsnp->size = mystat.st_size;
    fsnp->drive = 0;
    fsnp->relative = FALSE;
    fsnp->lnode = NULL;
  }
  return fsnp;
}


/*===========================================================================*/
/* Sets current directory                                                    */
/*===========================================================================*/

BOOLE fsWrapSetCWD(fs_navig_point *fs_point) {
  return SetCurrentDirectory(fs_point->name);
}


/*===========================================================================*/
/* Returns current directory                                                 */
/*===========================================================================*/

fs_navig_point *fsWrapGetCWD(void) {
  STR tmpcwd[FS_WRAP_MAX_PATH_LENGTH];

  _getcwd(tmpcwd, FS_WRAP_MAX_PATH_LENGTH);
  return fsWrapMakePoint(tmpcwd);
}


/*===========================================================================*/
/* Free allocated dirents                                                    */
/*===========================================================================*/

void fsWrapDirentsFree(void) {
}


/*===========================================================================*/
/* Reads information about named directory                                   */
/*===========================================================================*/

BOOLE fsWrapOpenDir(fs_navig_point *fs_point) {
  STR srcpath[256];
  fsWrapCloseDir();
  strcpy(srcpath, fs_point->name);
  strcat(srcpath, "\\*.*");
  fs_wrap_dirent_handle = FindFirstFile(srcpath, &fs_wrap_dirent);
  fs_wrap_opendir_firsttime = TRUE;
  return (fs_wrap_dirent_open = (fs_wrap_dirent_handle!=INVALID_HANDLE_VALUE));
}


/*===========================================================================*/
/* Returns current entry in the dirlisting, and advance the index            */
/*===========================================================================*/

fs_navig_point *fsWrapReadDir(void) {
  if (fs_wrap_dirent_open) {
    if (fs_wrap_opendir_firsttime) {
      fs_wrap_opendir_firsttime = FALSE;
      return fsWrapMakePoint(fs_wrap_dirent.cFileName);
    }
    else if (FindNextFile(fs_wrap_dirent_handle, &fs_wrap_dirent))
      return fsWrapMakePoint(fs_wrap_dirent.cFileName);
  }
  return NULL;
}


/*===========================================================================*/
/* Terminates the current directory listing                                  */
/*===========================================================================*/

void fsWrapCloseDir(void) {
  if (fs_wrap_dirent_open) {
    FindClose(fs_wrap_dirent_handle);
    fs_wrap_dirent_open = FALSE;
  }
}


/*===========================================================================*/
/* Module startup                                                            */
/*===========================================================================*/

void fsWrapStartup(void) {
  fs_wrap_dirent_open = FALSE;
}


/*===========================================================================*/
/* Module shutdown                                                           */
/*===========================================================================*/

void fsWrapShutdown(void) {
  fsWrapCloseDir();
}
