/* @(#) $Id: fileops.c,v 1.5 2012-12-07 14:05:43 carfesh Exp $             */
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

/* fileopsResolveVariables                                          */
/* resolve environment variables in file/folder names               */

BOOLE fileopsResolveVariables(const char *szPath, char *szNewPath)
{
  DWORD nRet;

  nRet = ExpandEnvironmentStrings(szPath, szNewPath, CFG_FILENAME_LENGTH);

  if(nRet < CFG_FILENAME_LENGTH)
    if( nRet )
    {
      szPath = szNewPath;
      return TRUE;
    }
  szNewPath = NULL;
  return FALSE;
}

/* fileopsGetGenericFileName                                        */
/* build generic filename pointing to Application Data\WinFellow    */
/* AmigaForever Amiga files path will be preferred over AppData     */
/* return TRUE if successfull, FALSE otherwise                      */

BOOLE fileopsGetGenericFileName(char *szPath, const char *szSubDir, const char *filename)
{
  HRESULT hr;
  
  // first check if AmigaForever is installed, use Amiga files path first
  if(!fileopsResolveVariables("%AMIGAFOREVERDATA%", szPath)) {
    if (SUCCEEDED (hr = SHGetFolderPathAndSubDir(NULL,                              // hWnd	
                                                 CSIDL_APPDATA | CSIDL_FLAG_CREATE, // csidl
                                                 NULL,                              // hToken
                                                 SHGFP_TYPE_CURRENT,                // dwFlags
                                                 szSubDir,                          // pszSubDir
                                                 szPath)))                          // pszPath
    {
	    PathAppend(szPath, TEXT(filename));
      return TRUE;
    }
    else 
    {
      strcpy(szPath, filename);
      return FALSE;
    }
  }
  else 
  {
    PathAppend(szPath, szSubDir);
    PathAppend(szPath, TEXT(filename));
    return TRUE;
  }
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
