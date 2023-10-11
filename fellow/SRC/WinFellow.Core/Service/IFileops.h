#pragma once

#include <cstdint>

namespace Service
{
  constexpr auto FILEOPS_MAX_FILE_PATH = 260;

  class IFileops
  {
  public:

    virtual bool fileopsGetFellowLogfileName(char*) = 0;
    virtual bool fileopsGetGenericFileName(char*, const char*, const char*) = 0;
    virtual bool fileopsGetDefaultConfigFileName(char*) = 0;
    virtual bool fileopsResolveVariables(const char*, char*) = 0;
    virtual bool fileopsGetWinFellowPresetPath(char*, const uint32_t) = 0;
    virtual bool fileopsGetScreenshotFileName(char*) = 0;
    virtual char* fileopsGetTemporaryFilename() = 0;
    virtual bool fileopsGetWinFellowInstallationPath(char*, const uint32_t) = 0;
    virtual bool fileopsGetKickstartByCRC32(const char*, const uint32_t, char*, const uint32_t) = 0;

    IFileops() = default;
    virtual ~IFileops() = default;
  };
}
