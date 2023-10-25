#pragma once

#include <cstdint>

namespace Service
{
  constexpr auto FILEOPS_MAX_FILE_PATH = 260;

  class IFileops
  {
  public:
    virtual bool GetFellowLogfileName(char *) = 0;
    virtual bool GetGenericFileName(char *, const char *, const char *) = 0;
    virtual bool GetDefaultConfigFileName(char *) = 0;
    virtual bool ResolveVariables(const char *, char *) = 0;
    virtual bool GetWinFellowPresetPath(char *, const uint32_t) = 0;
    virtual bool GetScreenshotFileName(char *) = 0;
    virtual char *GetTemporaryFilename() = 0;
    virtual bool GetWinFellowInstallationPath(char *, const uint32_t) = 0;
    virtual bool GetKickstartByCRC32(const char *, const uint32_t, char *, const uint32_t) = 0;

    IFileops() = default;
    virtual ~IFileops() = default;
  };
}
