#pragma once

#include "Service/IFileops.h"

class FileopsWin32 : public Service::IFileops
{
private:
  bool fileopsGetWinFellowExecutablePath(char* strBuffer, const uint32_t lBufferSize);
  bool fileopsDirectoryExists(const char* strPath);

public:
  bool fileopsGetFellowLogfileName(char*) override;
  bool fileopsGetGenericFileName(char*, const char*, const char*) override;
  bool fileopsGetDefaultConfigFileName(char*) override;
  bool fileopsResolveVariables(const char*, char*) override;
  bool fileopsGetWinFellowPresetPath(char*, const uint32_t) override;
  bool fileopsGetScreenshotFileName(char*) override;
  char* fileopsGetTemporaryFilename() override;
  bool fileopsGetWinFellowInstallationPath(char*, const uint32_t) override;
  bool fileopsGetKickstartByCRC32(const char*, const uint32_t, char*, const uint32_t) override;

  FileopsWin32() = default;
  virtual ~FileopsWin32() = default;
};
