#pragma once

#include "Service/IFileops.h"

class FileopsWin32 : public Service::IFileops
{
private:
  bool fileopsGetWinFellowExecutablePath(char* strBuffer, const uint32_t lBufferSize);
  bool fileopsDirectoryExists(const char* strPath);

public:
  bool GetFellowLogfileName(char*) override;
  bool GetGenericFileName(char*, const char*, const char*) override;
  bool GetDefaultConfigFileName(char*) override;
  bool ResolveVariables(const char*, char*) override;
  bool GetWinFellowPresetPath(char*, const uint32_t) override;
  bool GetScreenshotFileName(char*) override;
  char* GetTemporaryFilename() override;
  bool GetWinFellowInstallationPath(char*, const uint32_t) override;
  bool GetKickstartByCRC32(const char*, const uint32_t, char*, const uint32_t) override;

  FileopsWin32() = default;
  virtual ~FileopsWin32() = default;
};
