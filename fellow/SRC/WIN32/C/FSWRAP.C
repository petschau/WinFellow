/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* Filesystem browsing wrapper for the GUI                                 */
/*                                                                         */
/* Author: Petter Schau                                                    */
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
#include <AclAPI.h>

#include "portable.h"
#include "defs.h"
#include "fswrap.h"
#include "fellow.h"
#include "errno.h"


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

// this function is a very limited stat() workaround implementation to address
// the stat() bug on Windows XP using later platform toolsets; the problem was 
// addressed by Micosoft, but still persists for statically linked projects
// https://connect.microsoft.com/VisualStudio/feedback/details/1600505/stat-not-working-on-windows-xp-using-v14-xp-platform-toolset-vs2015
// https://stackoverflow.com/questions/32452777/visual-c-2015-express-stat-not-working-on-windows-xp
// limitations: only sets a limited set of mode flags and calculates size information
int fsWrapStat(const char *szFilename, struct stat *pStatBuffer)
{
  int result;

#ifdef _DEBUG
  fellowAddLog("fsWrapStat(szFilename=%s, pStatBuffer=0x%08x)\n", 
    szFilename, pStatBuffer);
#endif

  result = stat(szFilename, pStatBuffer);

#ifdef _DEBUG
  fellowAddLog(" native result=%d mode=0x%04x nlink=%d size=%lu\n",
    result,
    pStatBuffer->st_mode,
    pStatBuffer->st_nlink,
    pStatBuffer->st_size);
#endif

#if (_MSC_VER >= 1900) // Visual Studio 2015 or higher?
  WIN32_FILE_ATTRIBUTE_DATA hFileAttributeData;
  // mark files as readable by default; set flags for user, group and other
  unsigned short mode = _S_IREAD  | (_S_IREAD  >> 3) | (_S_IREAD  >> 6);

  memset(pStatBuffer, 0, sizeof(struct stat));
  pStatBuffer->st_nlink = 1;

  if(!GetFileAttributesEx(szFilename, GetFileExInfoStandard, &hFileAttributeData))
  {

#ifdef _DEBUG
    LPTSTR szErrorMessage=NULL;
    DWORD hResult = GetLastError();

    fellowAddLog("  fsWrapStat(): GetFileAttributesEx() failed, return code=%d", 
      hResult);

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
      NULL, hResult, MAKELANGID(0, SUBLANG_ENGLISH_US), (LPTSTR)&szErrorMessage, 0, NULL);
    if (szErrorMessage != NULL)
    {
        fellowAddTimelessLog(" (%s)\n", szErrorMessage);
        LocalFree(szErrorMessage);
        szErrorMessage = NULL;
    }
    else
      fellowAddTimelessLog("\n");
#endif

    *_errno() = ENOENT;
    result = -1;
  }
  else
  {
    // directory?
    if(hFileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      mode |= _S_IFDIR | _S_IEXEC  | (_S_IEXEC  >> 3) | (_S_IEXEC  >> 6);
    }
    else
    {
      mode |= _S_IFREG;
      // detection of executable files is not supported for the time being

      pStatBuffer->st_size  = ((__int64)hFileAttributeData.nFileSizeHigh << 32) + hFileAttributeData.nFileSizeLow;
    }

    if(!(hFileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    {
      mode |= _S_IWRITE | (_S_IWRITE >> 3) | (_S_IWRITE >> 6);
    }
      
    pStatBuffer->st_mode  = mode;

    result = 0;
  }

#ifdef _DEBUG
  fellowAddLog(" fswrap result=%d mode=0x%04x nlink=%d size=%lu\n",
    result,
    pStatBuffer->st_mode,
    pStatBuffer->st_nlink,
    pStatBuffer->st_size);
#endif

#endif

  return result;
}

/*===========================================================================*/
/* Fills in file attributes in point structure                               */
/* Return NULL on error                                                      */
/*===========================================================================*/

fs_navig_point *fsWrapMakePoint(STR *point) {
  struct stat mystat;
  fs_navig_point *fsnp = NULL;
  FILE *file_ptr;

  // check file permissions
  if(fsWrapStat(point, &mystat) == 0) {
    fsnp = (fs_navig_point *) malloc(sizeof(fs_navig_point));
    strcpy(fsnp->name, point);
    if(mystat.st_mode & _S_IFREG)
      fsnp->type = FS_NAVIG_FILE;
    else if (mystat.st_mode & _S_IFDIR)
      fsnp->type = FS_NAVIG_DIR;
    else
      fsnp->type = FS_NAVIG_OTHER;
    fsnp->writeable = !!(mystat.st_mode & _S_IWRITE);
    if(fsnp->writeable)
    {
      file_ptr = fopen(point, "a");
      if(file_ptr == NULL)
      {
        fsnp->writeable = FALSE;
      }
      else
      {
        fclose(file_ptr);
      }
    }
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
