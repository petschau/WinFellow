#pragma once

#include "fellow/api/defs.h"

namespace fellow::api
{
  class IFileops
  {
  public:
    virtual bool GetFellowLogfileName(char *szPath) = 0;
    virtual bool GetGenericFileName(char *szPath, const char *szSubDir, const char *filename) = 0;
    virtual bool GetDefaultConfigFileName(char *szPath) = 0;
    virtual bool ResolveVariables(const char *szPath, char *szNewPath) = 0;
    virtual bool GetWinFellowPresetPath(char *, const size_t) = 0;
    virtual BOOLE GetScreenshotFileName(char *szFilename) = 0;
    virtual char *GetTemporaryFilename() = 0;
    virtual bool GetWinFellowInstallationPath(char *strBuffer, const size_t lBufferSize) = 0;
    virtual bool GetKickstartByCRC32(const char *strSearchPath, const ULO lCRC32, char *strDestFilename, const ULO strDestLen) = 0;
  };
}