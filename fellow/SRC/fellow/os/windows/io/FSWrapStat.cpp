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
#include <string>

#include "fellow/api/defs.h"
#include "fellow/api/Services.h"
#include "errno.h"

using namespace std;
using namespace fellow::api;

#include <shlwapi.h>

// TODO: Perhaps this can be standardized?
// All that remains here is a stat wrapper that returns useful info about a file or directory

// this function is a very limited stat() workaround implementation to address
// the stat() bug on Windows XP using later platform toolsets; the problem was
// addressed by Micosoft, but still persists for statically linked projects
// https://connect.microsoft.com/VisualStudio/feedback/details/1600505/stat-not-working-on-windows-xp-using-v14-xp-platform-toolset-vs2015
// https://stackoverflow.com/questions/32452777/visual-c-2015-express-stat-not-working-on-windows-xp
// limitations: only sets a limited set of mode flags and calculates size information

// Returns non-zero when error
int fsWrapStat(const string &filename, struct stat *pStatBuffer)
{
  int result;

#ifdef _DEBUG
  Service->Log.AddLog("fsWrapStat(szFilename=%s, pStatBuffer=0x%08x)\n", filename.c_str(), pStatBuffer);
#endif

  result = stat(filename.c_str(), pStatBuffer);

#ifdef _DEBUG
  Service->Log.AddLog(" native result=%d mode=0x%04x nlink=%d size=%lu\n", result, pStatBuffer->st_mode, pStatBuffer->st_nlink, pStatBuffer->st_size);
#endif

#if (_MSC_VER >= 1900) // compiler version is Visual Studio 2015 or higher?
  // check OS version, only execute replacement code on Windows XP/OS versions before Vista
  OSVERSIONINFO osvi{};
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

  GetVersionEx(&osvi);

  bool bIsLegacyOS = osvi.dwMajorVersion == 5; // Windows 2000, XP, 2003

  if (bIsLegacyOS)
  {
    WIN32_FILE_ATTRIBUTE_DATA hFileAttributeData{};
    // mark files as readable by default; set flags for user, group and other
    unsigned short mode = _S_IREAD | (_S_IREAD >> 3) | (_S_IREAD >> 6);

    memset(pStatBuffer, 0, sizeof(struct stat));
    pStatBuffer->st_nlink = 1;

    if (!GetFileAttributesEx(filename.c_str(), GetFileExInfoStandard, &hFileAttributeData))
    {

#ifdef _DEBUG
      LPTSTR szErrorMessage = NULL;
      DWORD hResult = GetLastError();

      Service->Log.AddLog("  fsWrapStat(): GetFileAttributesEx() failed, return code=%d", hResult);

      FormatMessage(
          FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
          NULL,
          hResult,
          MAKELANGID(0, SUBLANG_ENGLISH_US),
          (LPTSTR)&szErrorMessage,
          0,
          NULL);
      if (szErrorMessage != NULL)
      {
        Service->Log.AddTimelessLog(" (%s)\n", szErrorMessage);
        LocalFree(szErrorMessage);
        szErrorMessage = NULL;
      }
      else
        Service->Log.AddTimelessLog("\n");
#endif

      *_errno() = ENOENT;
      result = -1;
    }
    else
    {
      // directory?
      if (hFileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
        mode |= _S_IFDIR | _S_IEXEC | (_S_IEXEC >> 3) | (_S_IEXEC >> 6);
      }
      else
      {
        mode |= _S_IFREG;
        // detection of executable files is not supported for the time being

        pStatBuffer->st_size = ((__int64)hFileAttributeData.nFileSizeHigh << 32) + hFileAttributeData.nFileSizeLow;
      }

      if (!(hFileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
      {
        mode |= _S_IWRITE | (_S_IWRITE >> 3) | (_S_IWRITE >> 6);
      }

      pStatBuffer->st_mode = mode;

      result = 0;
    } // GetFileAttributesEx() successful

#ifdef _DEBUG
    Service->Log.AddLog(" fswrap result=%d mode=0x%04x nlink=%d size=%lu\n", result, pStatBuffer->st_mode, pStatBuffer->st_nlink, pStatBuffer->st_size);
#endif
  } // legacy OS

#endif // newer compiler version check

  return result;
}
