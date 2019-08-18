#pragma once

/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* Filesystem operations                                                   */
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

#include "fellow/api/service/IFileops.h"

class FileopsWin32 : public fellow::api::IFileops
{
private:
  BOOLE GetWinFellowExecutablePath(char *strBuffer, const DWORD lBufferSize);
  bool DirectoryExists(const char *strPath);

public:
  bool GetFellowLogfileName(char *szPath) override;
  bool GetGenericFileName(char *szPath, const char *szSubDir, const char *filename) override;
  bool GetDefaultConfigFileName(char *szPath) override;
  bool ResolveVariables(const char *szPath, char *szNewPath) override;
  bool GetWinFellowPresetPath(char *strBuffer, const size_t lBufferSize) override;
  BOOLE GetScreenshotFileName(char *szFilename) override;
  char *GetTemporaryFilename() override;
  bool GetWinFellowInstallationPath(char *strBuffer, const size_t lBufferSize) override;
  bool GetKickstartByCRC32(const char *strSearchPath, const ULO lCRC32, char *strDestFilename, const ULO strDestLen) override;

  virtual ~FileopsWin32() = default;
};
