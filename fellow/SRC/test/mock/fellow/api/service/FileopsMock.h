#pragma once

#include "fellow/api/service/IFileops.h"

namespace test::mock::fellow::api
{
  class FileopsMock : public ::fellow::api::IFileops
  {
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
  };
}
