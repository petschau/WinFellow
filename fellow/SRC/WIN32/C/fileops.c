/* @(#) $Id: fileops.c,v 1.1 2008-02-09 19:47:46 carfesh Exp $ */
/*=========================================================================*/
/* Fellow Amiga Emulator                                                   */
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

/* fileopsGetFellowLogfileName                                      */
/* build fellow.log filename pointing to Application Data\WinFellow */

BOOLE fileopsGetFellowLogfileName(char *szPath)
{
  HRESULT hr;

  if (SUCCEEDED (hr = SHGetFolderPathAndSubDir(NULL,                // hWnd	
                                               CSIDL_APPDATA | CSIDL_FLAG_CREATE,       // csidl
                                               NULL,                // hToken
                                               SHGFP_TYPE_CURRENT,  // dwFlags
                                               TEXT("WinFellow"),    // pszSubDir
                                               szPath)))             // pszPath
  {
	  PathAppend(szPath, TEXT("fellow.log"));
    return TRUE;
  }
  else
    return FALSE;
}