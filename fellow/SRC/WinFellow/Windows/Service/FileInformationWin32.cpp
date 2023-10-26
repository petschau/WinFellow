#include "Windows/Service/FileInformationWin32.h"
#include "VirtualHost/Core.h"
#include "DEFS.H"
#include "FELLOW.H"

using namespace Service;

// this function is a very limited stat() workaround implementation to address
// the stat() bug on Windows XP using later platform toolsets; the problem was
// addressed by Micosoft, but still persists for statically linked projects
// https://connect.microsoft.com/VisualStudio/feedback/details/1600505/stat-not-working-on-windows-xp-using-v14-xp-platform-toolset-vs2015
// https://stackoverflow.com/questions/32452777/visual-c-2015-express-stat-not-working-on-windows-xp
// limitations: only sets a limited set of mode flags and calculates size information
int FileInformationWin32::Stat(const char *szFilename, struct stat *pStatBuffer)
{
  int result;

#ifdef _DEBUG
  _core.Log->AddLog("FileInformationWin32::Stat(szFilename=%s, pStatBuffer=0x%08x)\n", szFilename, pStatBuffer);
#endif

  result = stat(szFilename, pStatBuffer);

#ifdef _DEBUG
  _core.Log->AddLog(" native result=%d mode=0x%04x nlink=%d size=%lu\n", result, pStatBuffer->st_mode, pStatBuffer->st_nlink, pStatBuffer->st_size);
#endif

#if (_MSC_VER >= 1900) // compiler version is Visual Studio 2015 or higher?
  // check OS version, only execute replacement code on Windows XP/OS versions before Vista
  OSVERSIONINFO osvi; // Windows 2000, XP, 2003

  ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

  GetVersionEx(&osvi);

  BOOL bIsLegacyOS = osvi.dwMajorVersion == 5;

  if (bIsLegacyOS)
  {
    WIN32_FILE_ATTRIBUTE_DATA hFileAttributeData;
    // mark files as readable by default; set flags for user, group and other
    unsigned short mode = _S_IREAD | (_S_IREAD >> 3) | (_S_IREAD >> 6);

    memset(pStatBuffer, 0, sizeof(struct stat));
    pStatBuffer->st_nlink = 1;

    if (!GetFileAttributesEx(szFilename, GetFileExInfoStandard, &hFileAttributeData))
    {

#ifdef _DEBUG
      LPTSTR szErrorMessage = nullptr;
      DWORD hResult = GetLastError();

      _core.Log->AddLog("  FileInformationWin32::Stat(): GetFileAttributesEx() failed, return code=%d", hResult);

      FormatMessage(
          FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
          nullptr,
          hResult,
          MAKELANGID(0, SUBLANG_ENGLISH_US),
          (LPTSTR)&szErrorMessage,
          0,
          nullptr);
      if (szErrorMessage != nullptr)
      {
        _core.Log->AddTimelessLog(" (%s)\n", szErrorMessage);
        LocalFree(szErrorMessage);
        szErrorMessage = nullptr;
      }
      else
        _core.Log->AddTimelessLog("\n");
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
    _core.Log->AddLog(" FileInformationWin32::Stat() result=%d mode=0x%04x nlink=%d size=%lu\n", result, pStatBuffer->st_mode, pStatBuffer->st_nlink, pStatBuffer->st_size);
#endif
  } // legacy OS

#endif // newer compiler version check

  return result;
}

FileProperties *FileInformationWin32::GetFileProperties(const char *filename)
{
  struct stat mystat;
  FileProperties *fileProperties = nullptr;

  // check file permissions
  if (Stat(filename, &mystat) == 0)
  {
    fileProperties = new FileProperties();
    fileProperties->Name = filename;
    if (mystat.st_mode & _S_IFREG)
      fileProperties->Type = FileType::File;
    else if (mystat.st_mode & _S_IFDIR)
      fileProperties->Type = FileType::Directory;
    else
      fileProperties->Type = FileType::Directory;
    fileProperties->IsWritable = !!(mystat.st_mode & _S_IWRITE);
    if (fileProperties->IsWritable)
    {
      FILE *file_ptr = fopen(filename, "a");
      if (file_ptr == nullptr)
      {
        fileProperties->IsWritable = false;
      }
      else
      {
        fclose(file_ptr);
      }
    }
    fileProperties->Size = mystat.st_size;
  }
  else
  {
    char *strError = strerror(errno);
    fellowShowRequester(
        FELLOW_REQUESTER_TYPE_ERROR, "FileInformationWin32::GetFileProperties(): ERROR getting file information for %s: error code %i (%s)\n", filename, errno, strError);
  }
  return fileProperties;
}
