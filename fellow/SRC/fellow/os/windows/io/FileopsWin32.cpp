/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* This file contains filesystem abstraction code to handle OS specific    */
/* filesystem requirements.                                                */
/*                                                                         */
/* Author: Torsten Enderling (carfesh@gmx.net)                             */
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

#include <shlobj.h>
#include <shlwapi.h>

#include "fellow/api/defs.h"
#include "fellow/api/Services.h"

#include "zlib.h"                 // crc32 function
#include "fellow/memory/Memory.h" // decrypt AF2 kickstart

#include <time.h>
#include <io.h>

#include "fellow/os/windows/io/FileopsWin32.h"

using namespace fellow::api;

/** @file
 * The fileops module contains abtract functions to generate filenames in a
 * platform specific manner.
 */

/** resolve environment variables in file/folder names.
 * @param[in] szPath path name to resolve
 * @param[out] szNewPath path name with resolved variables
 * @return TRUE, if variable was successfully resolved, FALSE otherwise.
 */
bool FileopsWin32::ResolveVariables(const char *szPath, char *szNewPath)
{
  DWORD nRet = ExpandEnvironmentStrings(szPath, szNewPath, CFG_FILENAME_LENGTH);

  if (nRet < CFG_FILENAME_LENGTH)
    if (nRet)
    {
      if (strcmp(szPath, szNewPath) == 0)
        return false;
      else
        return true;
    }
  szNewPath = nullptr;
  return false;
}

/** build generic filename pointing to "Application Data\Roaming\WinFellow";
 * AmigaForever Amiga files path will be preferred over AppData when
 * compiling a RetroPlatform specific build.
 * @return TRUE if successful, FALSE otherwise
 */
bool FileopsWin32::GetGenericFileName(char *szPath, const char *szSubDir, const char *filename)
{
  HRESULT hr;

#ifdef RETRO_PLATFORM
  // first check if AmigaForever is installed, prefer Amiga files path
  if (!ResolveVariables("%AMIGAFOREVERDATA%", szPath))
  {
#endif
    if (SUCCEEDED(hr = SHGetFolderPathAndSubDir(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, szSubDir, szPath)))
    {
      PathAppend(szPath, TEXT(filename));
      return true;
    }
    else
    {
      strcpy(szPath, filename);
      return false;
    }
#ifdef RETRO_PLATFORM
  }
  else
  {
    PathAppend(szPath, szSubDir);
    PathAppend(szPath, TEXT(filename));
    return true;
  }
#endif
}

/** generate screenshot filename (below my pictures folder)
 * @return TRUE if successful, FALSE otherwise
 */

BOOLE FileopsWin32::GetScreenshotFileName(char *szFilename)
{
  HRESULT hr;
  char szFolderPath[MAX_PATH];

  hr = SHGetFolderPath(NULL, CSIDL_MYPICTURES, NULL, SHGFP_TYPE_CURRENT, szFolderPath);
  if (hr == S_OK)
  {
    time_t rawtime;
    struct tm *timeinfo;
    char szTime[255] = "";
    ULO i = 1;
    bool done = false;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(szTime, 255, "%Y%m%d%H%M%S", timeinfo);
    while (done != true)
    {
      sprintf(szFilename, "%s\\WinFellow-%s_%u.bmp", szFolderPath, szTime, i);
      if (access(szFilename, 00) != -1)
      { // file exists
        i++;
      }
      else
        done = true;
    }
    return TRUE;
  }
  return FALSE;
}

/* GetFellowLogfileName                                      */
/* build fellow.log filename pointing to Application Data\WinFellow */
/* return TRUE if successfull, FALSE otherwise                      */

bool FileopsWin32::GetFellowLogfileName(char *szPath)
{
  return GetGenericFileName(szPath, "WinFellow", "fellow.log");
}

/* GetDefaultConfigFileName                                  */
/* build default.wfc filename pointing to                           */
/* Application Data\WinFellow\configurations                        */
/* return TRUE if successfull, FALSE otherwise                      */

bool FileopsWin32::GetDefaultConfigFileName(char *szPath)
{
  return GetGenericFileName(szPath, "WinFellow\\configurations", "default.wfc");
}

/* GetWinFellowExecutablePath                                */
/* writes WinFellow executable full path into strBuffer             */
BOOLE FileopsWin32::GetWinFellowExecutablePath(char *strBuffer, const DWORD lBufferSize)
{
  if (GetModuleFileName(NULL, strBuffer, lBufferSize) != 0)
    return TRUE;
  else
    return FALSE;
}

/* GetWinFellowExecutablePath                                */
/* writes WinFellow installation path into strBuffer                */
bool FileopsWin32::GetWinFellowInstallationPath(char *strBuffer, const size_t lBufferSize)
{
  STR strWinFellowExePath[CFG_FILENAME_LENGTH] = "";

  if (GetWinFellowExecutablePath(strWinFellowExePath, CFG_FILENAME_LENGTH))
  {
    char *strLastBackslash = strrchr(strWinFellowExePath, '\\');

    if (strLastBackslash) *strLastBackslash = '\0';

    strncpy(strBuffer, strWinFellowExePath, lBufferSize);

    return true;
  }
  else
    return false;
}

bool FileopsWin32::DirectoryExists(const char *strPath)
{
  DWORD dwAttrib = GetFileAttributes(strPath);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

/* GetWinFellowExecutablePath                                */
/* writes WinFellow preset directory path into strBuffer            */
bool FileopsWin32::GetWinFellowPresetPath(char *strBuffer, const size_t lBufferSize)
{
  STR strWinFellowInstallPath[CFG_FILENAME_LENGTH] = "";

  if (GetWinFellowInstallationPath(strWinFellowInstallPath, CFG_FILENAME_LENGTH))
  {
    strncat(strWinFellowInstallPath, "\\Presets", 9);

    if (DirectoryExists(strWinFellowInstallPath))
    {
      strncpy(strBuffer, strWinFellowInstallPath, lBufferSize);
      return true;
    }
    else
    {
#ifdef _DEBUG
      // in debug mode, look for presets directory also with relative path from output exe
      GetWinFellowInstallationPath(strWinFellowInstallPath, CFG_FILENAME_LENGTH);
#ifdef X64
      strncat(strWinFellowInstallPath, "\\..\\..\\..\\..\\..\\Presets", 24);
#else
      strncat(strWinFellowInstallPath, "\\..\\..\\..\\..\\Presets", 21);
#endif

      if (DirectoryExists(strWinFellowInstallPath))
      {
        strncpy(strBuffer, strWinFellowInstallPath, lBufferSize);
        return true;
      }
#endif
      return false;
    }
  }
  else
    return false;
}

/*=========================================*/
/* Get a temporary file name               */
/* if TEMP environment variable is set try */
/* to create in temporary folder, else in  */
/* the volumes rootdir                     */
/*=========================================*/

char *FileopsWin32::GetTemporaryFilename()
{
  char *tempvar;
  char *result;

  tempvar = getenv("TEMP");
  if (tempvar != NULL)
  {
    result = _tempnam(tempvar, "wftemp");
  }
  else
  {
    result = tmpnam(NULL);
  }
  return result;
}

bool FileopsWin32::GetKickstartByCRC32(const char *strSearchPath, const ULO lCRC32, char *strDestFilename, const ULO strDestLen)
{
  STR strSearchPattern[CFG_FILENAME_LENGTH] = "";
  WIN32_FIND_DATA ffd;
  HANDLE hFind = INVALID_HANDLE_VALUE;
  UBY memory_kick[0x080000 + 32];
  FILE *F = NULL;
  STR strFilename[CFG_FILENAME_LENGTH] = "";
  ULO lCurrentCRC32 = 0;
#ifdef FILEOPS_ROMSEARCH_RECURSIVE
  STR strSubDir[CFG_FILENAME_LENGTH] = "";
#endif

  strncpy(strSearchPattern, strSearchPath, CFG_FILENAME_LENGTH);
  strncat(strSearchPattern, "\\*", 3);

  hFind = FindFirstFile(strSearchPattern, &ffd);
  if (hFind == INVALID_HANDLE_VALUE)
  {
    Service->Log.AddLog("fileopsGetKickstartByCRC32(): FindFirstFile failed.\n");
    return false;
  }

  do
  {
    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
#ifdef FILEOPS_ROMSEARCH_RECURSIVE
      if (strcmp(ffd.cFileName, ".") != 0 && strcmp(ffd.cFileName, "..") != 0)
      {
        strncpy(strSubDir, strSearchPath, CFG_FILENAME_LENGTH);
        strncat(strSubDir, "\\", 2);
        strncat(strSubDir, ffd.cFileName, CFG_FILENAME_LENGTH);

        if (fileopsGetKickstartByCRC32(strSubDir, lCRC32, strDestFilename, strDestLen)) return true;
      }
#endif
    }
    else
    {
      if (ffd.nFileSizeHigh == 0 && (ffd.nFileSizeLow == 262144 || ffd.nFileSizeLow == 524288))
      {
        // possibly an unencrypted ROM, read and build checksum
        strncpy(strFilename, strSearchPath, CFG_FILENAME_LENGTH);
        strncat(strFilename, "\\", 2);
        strncat(strFilename, ffd.cFileName, CFG_FILENAME_LENGTH);

        if (F = fopen(strFilename, "rb"))
        {
          fread(memory_kick, ffd.nFileSizeLow, 1, F);

          fclose(F);
          F = NULL;

          lCurrentCRC32 = crc32(0, memory_kick, ffd.nFileSizeLow);

          if (lCurrentCRC32 == lCRC32)
          {
            strncpy(strDestFilename, strFilename, strDestLen);
            return true;
          }
        }
      }
      else if (ffd.nFileSizeHigh == 0 && (ffd.nFileSizeLow == 262155 || ffd.nFileSizeLow == 524299))
      {
        // possibly an encrypted ROM, read and build checksum
        strncpy(strFilename, strSearchPath, CFG_FILENAME_LENGTH);
        strncat(strFilename, "\\", 2);
        strncat(strFilename, ffd.cFileName, CFG_FILENAME_LENGTH);

        if (F = fopen(strFilename, "rb"))
        {
          int result = memoryKickLoadAF2(strFilename, F, memory_kick, true);

          fclose(F);
          F = NULL;

          if (result == TRUE)
          {
            lCurrentCRC32 = crc32(0, memory_kick, ffd.nFileSizeLow - 11);

            if (lCurrentCRC32 == lCRC32)
            {
              strncpy(strDestFilename, strFilename, strDestLen);
              return true;
            }
          }
        }
      }
    }

  } while (FindNextFile(hFind, &ffd) != 0);
  FindClose(hFind);

  return false;
}