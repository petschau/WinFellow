/* @(#) $Id: fileops.c,v 1.6 2013-01-05 23:20:44 carfesh Exp $             */
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
  DWORD nRet;

  nRet = ExpandEnvironmentStrings(szPath, szNewPath, CFG_FILENAME_LENGTH);

  if(nRet < CFG_FILENAME_LENGTH)
    if( nRet ) {
      if(strcmp(szPath, szNewPath) == 0)
        return FALSE;
      else
        return TRUE;
    }
  szNewPath = NULL;
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
