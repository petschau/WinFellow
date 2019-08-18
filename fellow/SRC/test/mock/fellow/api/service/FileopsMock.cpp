#include "test/mock/fellow/api/service/FileopsMock.h"

namespace test::mock::fellow::api
{
  bool FileopsMock::GetFellowLogfileName(char *szPath)
  {
    szPath[0] = '\n';
    return true;
  }

  bool FileopsMock::GetGenericFileName(char *szPath, const char *szSubDir, const char *filename)
  {
    szPath[0] = '\n';
    return true;
  }

  bool FileopsMock::GetDefaultConfigFileName(char *szPath)
  {
    szPath[0] = '\n';
    return true;
  }

  bool FileopsMock::ResolveVariables(const char *szPath, char *szNewPath)
  {
    szNewPath[0] = '\n';
    return true;
  }

  bool FileopsMock::GetWinFellowPresetPath(char *strBuffer, const size_t lBufferSize)
  {
    strBuffer[0] = '\n';
    return true;
  }

  BOOLE FileopsMock::GetScreenshotFileName(char *szFilename)
  {
    szFilename[0] = '\n';
    return TRUE;
  }

  char *FileopsMock::GetTemporaryFilename()
  {
    return nullptr;
  }

  bool FileopsMock::GetWinFellowInstallationPath(char *strBuffer, const size_t lBufferSize)
  {
    strBuffer[0] = '\n';
    return true;
  }

  bool FileopsMock::GetKickstartByCRC32(const char *strSearchPath, const ULO lCRC32, char *strDestFilename, const ULO strDestLen)
  {
    strDestFilename[0] = '\n';
    return true;
  }
}
