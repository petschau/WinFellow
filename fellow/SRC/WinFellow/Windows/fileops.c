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

#include "defs.h"
#include "fellow.h"

#include "zlib.h" // crc32 function
#include "fmem.h" // decrypt AF2 kickstart

#include <ctime>
#include <io.h>

/** @file
 * The fileops module contains abtract functions to generate filenames in a
 * platform specific manner.
 */

/** resolve environment variables in file/folder names.
 * @param[in] szPath path name to resolve
 * @param[out] szNewPath path name with resolved variables
 * @return TRUE, if variable was successfully resolved, FALSE otherwise.
 */
BOOLE fileopsResolveVariables(const char *szPath, char *szNewPath) {
  DWORD nRet = ExpandEnvironmentStrings(szPath, szNewPath, CFG_FILENAME_LENGTH);

  if(nRet < CFG_FILENAME_LENGTH)
    if( nRet ) {
      if(strcmp(szPath, szNewPath) == 0)
        return FALSE;
      else
        return TRUE;
    }
  szNewPath = nullptr;
  return FALSE;
}

/** build generic filename pointing to "Application Data\Roaming\WinFellow";
 * AmigaForever Amiga files path will be preferred over AppData when 
 * compiling a RetroPlatform specific build.
 * @return TRUE if successful, FALSE otherwise
 */
BOOLE fileopsGetGenericFileName(char *szPath, const char *szSubDir, const char *filename)
{
  HRESULT hr;
  
#ifdef RETRO_PLATFORM
  // first check if AmigaForever is installed, prefer Amiga files path
  if(!fileopsResolveVariables("%AMIGAFOREVERDATA%", szPath)) {
#endif
    if (SUCCEEDED (hr = SHGetFolderPathAndSubDir(NULL,                              	
                                                 CSIDL_APPDATA | CSIDL_FLAG_CREATE,
                                                 NULL,
                                                 SHGFP_TYPE_CURRENT,
                                                 szSubDir,
                                                 szPath))) {
	    PathAppend(szPath, TEXT(filename));
      return TRUE;
    }
    else {
      strcpy(szPath, filename);
      return FALSE;
    }
#ifdef RETRO_PLATFORM
  }
  else {
    PathAppend(szPath, szSubDir);
    PathAppend(szPath, TEXT(filename));
    return TRUE;
  }
#endif
}

/** generate screenshot filename (below my pictures folder)
* @return TRUE if successful, FALSE otherwise
*/

BOOLE fileopsGetScreenshotFileName(char *szFilename)
{
  char szFolderPath[MAX_PATH];
  
  HRESULT hr = SHGetFolderPath(nullptr, CSIDL_MYPICTURES, nullptr, SHGFP_TYPE_CURRENT, szFolderPath);
  if (hr == S_OK)
  {
    time_t rawtime;
    char szTime[255] = "";
    uint32_t i = 1;
    bool done = false;
    
    time(&rawtime);
    tm* timeinfo = localtime(&rawtime);
    
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

/* fileopsGetFellowLogfileName                                      */
/* build fellow.log filename pointing to Application Data\WinFellow */
/* return TRUE if successfull, FALSE otherwise                      */

BOOLE fileopsGetFellowLogfileName(char *szPath)
{
  return fileopsGetGenericFileName(szPath, "WinFellow", "fellow.log");
}

/* fileopsGetDefaultConfigFileName                                  */
/* build default.wfc filename pointing to                           */
/* Application Data\WinFellow\configurations                        */
/* return TRUE if successfull, FALSE otherwise                      */

BOOLE fileopsGetDefaultConfigFileName(char *szPath)
{
  return fileopsGetGenericFileName(szPath, "WinFellow\\configurations", "default.wfc");
}

/* fileopsGetWinFellowExecutablePath                                */
/* writes WinFellow executable full path into strBuffer             */
static BOOLE fileopsGetWinFellowExecutablePath(char *strBuffer, const DWORD lBufferSize)
{
  if(GetModuleFileName(nullptr, strBuffer, lBufferSize) != 0)
    return TRUE;
  else
    return FALSE;
}

/* fileopsGetWinFellowExecutablePath                                */
/* writes WinFellow installation path into strBuffer                */
bool fileopsGetWinFellowInstallationPath(char *strBuffer, const DWORD lBufferSize)
{
  char strWinFellowExePath[CFG_FILENAME_LENGTH] = "";

  if(fileopsGetWinFellowExecutablePath(strWinFellowExePath, CFG_FILENAME_LENGTH))
  {
    char *strLastBackslash = strrchr(strWinFellowExePath, '\\');

    if (strLastBackslash)
        *strLastBackslash = '\0';

    strncpy(strBuffer, strWinFellowExePath, lBufferSize);

    return true;
  }
  else
    return false;
}

static bool fileopsDirectoryExists(const char *strPath)
{
  DWORD dwAttrib = GetFileAttributes(strPath);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

/* fileopsGetWinFellowExecutablePath                                */
/* writes WinFellow preset directory path into strBuffer            */
BOOLE fileopsGetWinFellowPresetPath(char *strBuffer, const DWORD lBufferSize)
{
  char strWinFellowInstallPath[CFG_FILENAME_LENGTH] = "";

  if(fileopsGetWinFellowInstallationPath(strWinFellowInstallPath, CFG_FILENAME_LENGTH))
  {
    strncat(strWinFellowInstallPath, "\\Presets", 9);

    if(fileopsDirectoryExists(strWinFellowInstallPath)) {
      strncpy(strBuffer, strWinFellowInstallPath, lBufferSize);
      return TRUE;
    }
    else {
#ifdef _DEBUG
      // in debug mode, look for presets directory also with relative path from output exe
      fileopsGetWinFellowInstallationPath(strWinFellowInstallPath, CFG_FILENAME_LENGTH);
#ifdef X64
      strncat(strWinFellowInstallPath, "\\..\\..\\..\\..\\..\\Presets", 24);
#else
      strncat(strWinFellowInstallPath, "\\..\\..\\..\\..\\Presets", 21);
#endif
      
      if(fileopsDirectoryExists(strWinFellowInstallPath)) {
        strncpy(strBuffer, strWinFellowInstallPath, lBufferSize);
        return TRUE;
      }
#endif
      return FALSE;
    }
  }
  else
    return FALSE;
}

/*=========================================*/
/* Get a temporary file name               */
/* if TEMP environment variable is set try */
/* to create in temporary folder, else in  */
/* the volumes rootdir                     */
/*=========================================*/

char *fileopsGetTemporaryFilename()
{
  char *result;

  char* tempvar = getenv("TEMP");
  if( tempvar != nullptr)
  {
    result = _tempnam(tempvar, "wftemp");
  }
  else
  {
    result = tmpnam(nullptr);
  }
  return result;
}

bool fileopsGetKickstartByCRC32(const char *strSearchPath, const uint32_t lCRC32, char *strDestFilename, const uint32_t strDestLen)
{
  char strSearchPattern[CFG_FILENAME_LENGTH] = "";
  WIN32_FIND_DATA ffd;
  HANDLE hFind = INVALID_HANDLE_VALUE;
  uint8_t memory_kick[0x080000 + 32];
  FILE *F = nullptr;
  char strFilename[CFG_FILENAME_LENGTH] = "";
  uint32_t lCurrentCRC32 = 0;
#ifdef FILEOPS_ROMSEARCH_RECURSIVE
  char strSubDir[CFG_FILENAME_LENGTH] = "";
#endif
  
  strncpy(strSearchPattern, strSearchPath, CFG_FILENAME_LENGTH);
  strncat(strSearchPattern, "\\*", 3);

  hFind = FindFirstFile(strSearchPattern, &ffd);
  if (hFind == INVALID_HANDLE_VALUE) {
    fellowAddLog("fileopsGetKickstartByCRC32(): FindFirstFile failed.\n");
    return false;
  }

  do {
    if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
#ifdef FILEOPS_ROMSEARCH_RECURSIVE
      if(strcmp(ffd.cFileName, ".") != 0 && strcmp(ffd.cFileName, "..") != 0) {    
        strncpy(strSubDir, strSearchPath, CFG_FILENAME_LENGTH);
        strncat(strSubDir, "\\", 2);
        strncat(strSubDir, ffd.cFileName, CFG_FILENAME_LENGTH);

        if(fileopsGetKickstartByCRC32(strSubDir, lCRC32, strDestFilename, strDestLen))
          return true;
      }
#endif
    }
    else {
      if(ffd.nFileSizeHigh == 0 && (ffd.nFileSizeLow == 262144 || ffd.nFileSizeLow == 524288)) {
        // possibly an unencrypted ROM, read and build checksum
        strncpy(strFilename, strSearchPath, CFG_FILENAME_LENGTH);
        strncat(strFilename, "\\", 2);
        strncat(strFilename, ffd.cFileName, CFG_FILENAME_LENGTH);

        if(F = fopen(strFilename, "rb")) {
          fread(memory_kick, ffd.nFileSizeLow, 1, F);

          fclose(F);
          F = nullptr;

          lCurrentCRC32 = crc32(0, memory_kick, ffd.nFileSizeLow);

          if(lCurrentCRC32 == lCRC32) {
            strncpy(strDestFilename, strFilename, strDestLen);
            return true;
          }
        }
      }
      else if(ffd.nFileSizeHigh == 0 && (ffd.nFileSizeLow == 262155 || ffd.nFileSizeLow == 524299)) {
        // possibly an encrypted ROM, read and build checksum
        strncpy(strFilename, strSearchPath, CFG_FILENAME_LENGTH);
        strncat(strFilename, "\\", 2);
        strncat(strFilename, ffd.cFileName, CFG_FILENAME_LENGTH);

        if(F = fopen(strFilename, "rb")) {
          int result = memoryKickLoadAF2(strFilename, F, memory_kick, true);

          fclose(F);
          F = nullptr;

          if(result == TRUE) {
            lCurrentCRC32 = crc32(0, memory_kick, ffd.nFileSizeLow - 11);

            if(lCurrentCRC32 == lCRC32) {
              strncpy(strDestFilename, strFilename, strDestLen);
              return true;
            }
          }
        }
      }
    }

  } while(FindNextFile(hFind, &ffd) != 0);
  FindClose(hFind);

  return false;
}