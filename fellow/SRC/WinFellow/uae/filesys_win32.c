/*=========================================================================*/
/* UAE - The Un*x Amiga Emulator                                           */
/*                                                                         */
/* Unix file system handler for AmigaDOS                                   */
/* Windows related functions                                               */
/*                                                                         */
/* (w)2004 by Torsten Enderling <carfesh@gmx.net>                          */
/* based on posixemu.c (Copyright 1997 Mathias Ortmann)                    */
/*                                                                         */
/* Version 0.1: 9A0903                                                     */
/*                                                                         */
/* This file only contains functions required to compile filesys.c and     */
/* fsdb* on the win32 platform; posixemu.c should be regarded obsolete     */
/* through the functionality implemented in the fsdb and here.             */
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

#undef __ENV_UAE__
#undef __ENV_FELLOW__

/* #define __ENV_UAE__ */ /* compile this for UAE */
#define __ENV_FELLOW__    /* compile this for Fellow */

#ifdef __ENV_UAE__
#include "sysconfig.h"
#include "sysdeps.h"

#include "Configuration.h"
#include "threaddep/thread.h"
#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "filesys.h"
#include "autoconf.h"
#include "compiler.h"
#include "fsusage.h"
#include "native2amiga.h"
#include "scsidev.h"
#endif

#ifdef __ENV_FELLOW__
#include "uae2fell.h"
#endif

/* common includes */
#include <windows.h>
#include "filesys.h"
#include "fsdb.h"

static DWORD lasterror; /* parse this using FormatMessage... later. */

/************************************
 * directory handling functionality *
 ************************************/

DIR
{
  WIN32_FIND_DATA finddata;
  HANDLE hDir;
  int getnext;
};

DIR *win32_opendir(const char *path)
{
  char buf[1024];
  DIR *dir;

  if (!(dir = (DIR *)GlobalAlloc(GPTR, sizeof(DIR))))
  {
    lasterror = GetLastError();
    return nullptr;
  }

  strcpy(buf, path);
  strcat(buf, "\\*");

  if ((dir->hDir = FindFirstFile(buf, &dir->finddata)) == INVALID_HANDLE_VALUE)
  {
    lasterror = GetLastError();
    GlobalFree(dir);
    return nullptr;
  }

  return dir;
}

struct dirent *win32_readdir(DIR *dir)
{
  if (dir == nullptr) return nullptr;

  if (dir->getnext)
  {
    if (!FindNextFile(dir->hDir, &dir->finddata))
    {
      lasterror = GetLastError();
      return nullptr;
    }
  }
  dir->getnext = TRUE;

  return (struct dirent *)dir->finddata.cFileName;
}

void win32_closedir(DIR *dir)
{
  if (dir == nullptr) return;

  FindClose(dir->hDir);
  GlobalFree(dir);
}

int win32_mkdir(const char *name, int mode)
{
  if (CreateDirectory(name, nullptr)) return 0;

  lasterror = GetLastError();

  return -1;
}

/*********************
 * file manipulation *
 *********************/

int win32_truncate(const char *name, long int len)
{
  HANDLE hFile;
  BOOL bResult = FALSE;
  int result = -1;

  if ((hFile = CreateFile(name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)) !=
      INVALID_HANDLE_VALUE)
  {
    if (SetFilePointer(hFile, len, nullptr, FILE_BEGIN) == (DWORD)len)
    {
      if (SetEndOfFile(hFile) == TRUE) result = 0;
    }
    else
    {
      write_log("SetFilePointer() failure for %s to posn %d\n", name, len);
    }
    CloseHandle(hFile);
  }
  else
  {
    write_log("CreateFile() failed to open %s\n", name);
  }
  if (result == -1) lasterror = GetLastError();
  return result;
}

/****************************************************
 * Translate lasterror to valid AmigaDOS error code *
 * The mapping is probably not 100% correct yet     *
 ****************************************************/
long dos_errno()
{
  int i;
  static DWORD errtbl[][2] = {
      {ERROR_FILE_NOT_FOUND, ERROR_OBJECT_NOT_AROUND}, /* 2 */
      {ERROR_PATH_NOT_FOUND, ERROR_OBJECT_NOT_AROUND}, /* 3 */
      {ERROR_SHARING_VIOLATION, ERROR_OBJECT_IN_USE},
      {ERROR_ACCESS_DENIED, ERROR_WRITE_PROTECTED},              /* 5 */
      {ERROR_ARENA_TRASHED, ERROR_NO_FREE_STORE},                /* 7 */
      {ERROR_NOT_ENOUGH_MEMORY, ERROR_NO_FREE_STORE},            /* 8 */
      {ERROR_INVALID_BLOCK, ERROR_SEEK_ERROR},                   /* 9 */
      {ERROR_INVALID_DRIVE, ERROR_DIR_NOT_FOUND},                /* 15 */
      {ERROR_CURRENT_DIRECTORY, ERROR_DISK_WRITE_PROTECTED},     /* 16 */
      {ERROR_NO_MORE_FILES, ERROR_NO_MORE_ENTRIES},              /* 18 */
      {ERROR_SHARING_VIOLATION, ERROR_OBJECT_IN_USE},            /* 32 */
      {ERROR_LOCK_VIOLATION, ERROR_DISK_WRITE_PROTECTED},        /* 33 */
      {ERROR_BAD_NETPATH, ERROR_DIR_NOT_FOUND},                  /* 53 */
      {ERROR_NETWORK_ACCESS_DENIED, ERROR_DISK_WRITE_PROTECTED}, /* 65 */
      {ERROR_BAD_NET_NAME, ERROR_DIR_NOT_FOUND},                 /* 67 */
      {ERROR_FILE_EXISTS, ERROR_OBJECT_EXISTS},                  /* 80 */
      {ERROR_CANNOT_MAKE, ERROR_DISK_WRITE_PROTECTED},           /* 82 */
      {ERROR_FAIL_I24, ERROR_WRITE_PROTECTED},                   /* 83 */
      {ERROR_DRIVE_LOCKED, ERROR_OBJECT_IN_USE},                 /* 108 */
      {ERROR_DISK_FULL, ERROR_DISK_IS_FULL},                     /* 112 */
      {ERROR_NEGATIVE_SEEK, ERROR_SEEK_ERROR},                   /* 131 */
      {ERROR_SEEK_ON_DEVICE, ERROR_SEEK_ERROR},                  /* 132 */
      {ERROR_DIR_NOT_EMPTY, ERROR_DIRECTORY_NOT_EMPTY},          /* 145 */
      {ERROR_ALREADY_EXISTS, ERROR_OBJECT_EXISTS},               /* 183 */
      {ERROR_FILENAME_EXCED_RANGE, ERROR_OBJECT_NOT_AROUND},     /* 206 */
      {ERROR_NOT_ENOUGH_QUOTA, ERROR_DISK_IS_FULL},              /* 1816 */
      {ERROR_DIRECTORY, ERROR_OBJECT_WRONG_TYPE}};

  for (i = sizeof(errtbl) / sizeof(errtbl[0]); i--;)
  {
    if (errtbl[i][0] == lasterror) return errtbl[i][1];
  }
  return 236;
}
