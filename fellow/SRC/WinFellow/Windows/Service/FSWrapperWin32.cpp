#include "Windows/Service/FSWrapperWin32.h"
#include "VirtualHost/Core.h"
#include "DEFS.H"
#include "FELLOW.H"
#include "LISTTREE.H"

using namespace Service;

FSWrapperWin32::FSWrapperWin32()
{
}

fs_wrapper_point * FSWrapperWin32::MakePoint(const char *point)
{
  fs_navig_point *navig_point = MakePointInternal(point);
  if (navig_point)
  {
    fs_wrapper_point* result = new fs_wrapper_point();
    result->drive = navig_point->drive;
    result->name = navig_point->name;
    result->relative = navig_point->relative;
    result->size = navig_point->size;
    result->type = MapFileType(navig_point->type);
    result->writeable = navig_point->writeable;
    free(navig_point);
    return result;
  }

  return nullptr;
}

// this function is a very limited stat() workaround implementation to address
// the stat() bug on Windows XP using later platform toolsets; the problem was 
// addressed by Micosoft, but still persists for statically linked projects
// https://connect.microsoft.com/VisualStudio/feedback/details/1600505/stat-not-working-on-windows-xp-using-v14-xp-platform-toolset-vs2015
// https://stackoverflow.com/questions/32452777/visual-c-2015-express-stat-not-working-on-windows-xp
// limitations: only sets a limited set of mode flags and calculates size information
int FSWrapperWin32::Stat(const char* szFilename, struct stat* pStatBuffer)
{
  int result;

#ifdef _DEBUG
  _core.Log->AddLog("fsWrapStat(szFilename=%s, pStatBuffer=0x%08x)\n",
    szFilename, pStatBuffer);
#endif

  result = stat(szFilename, pStatBuffer);

#ifdef _DEBUG
  _core.Log->AddLog(" native result=%d mode=0x%04x nlink=%d size=%lu\n",
    result,
    pStatBuffer->st_mode,
    pStatBuffer->st_nlink,
    pStatBuffer->st_size);
#endif

#if (_MSC_VER >= 1900) // compiler version is Visual Studio 2015 or higher?
  // check OS version, only execute replacement code on Windows XP/OS versions before Vista
  OSVERSIONINFO osvi;// Windows 2000, XP, 2003

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

      _core.Log->AddLog("  fsWrapStat(): GetFileAttributesEx() failed, return code=%d",
        hResult);

      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
        nullptr, hResult, MAKELANGID(0, SUBLANG_ENGLISH_US), (LPTSTR)&szErrorMessage, 0, nullptr);
      if (szErrorMessage != nullptr)
      {
        _core.Log->AddTimelessLog(" (%s)\n", szErrorMessage);
        LocalFree(szErrorMessage);
        szErrorMessage = nullptr;
      }
      else
        _core.Log->AddTimelessLog("\n");
#endif

      * _errno() = ENOENT;
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
    _core.Log->AddLog(" fswrap result=%d mode=0x%04x nlink=%d size=%lu\n",
      result,
      pStatBuffer->st_mode,
      pStatBuffer->st_nlink,
      pStatBuffer->st_size);
#endif
  } // legacy OS

#endif // newer compiler version check

  return result;
}

fs_wrapper_file_types FSWrapperWin32::MapFileType(fs_navig_file_types type)
{
  if (type == fs_navig_file_types::FS_NAVIG_FILE)
  {
    return fs_wrapper_file_types::FS_NAVIG_FILE;
  }
  if (type == fs_navig_file_types::FS_NAVIG_DIR)
  {
    return fs_wrapper_file_types::FS_NAVIG_DIR;
  }
  return fs_wrapper_file_types::FS_NAVIG_OTHER;
}

fs_navig_point* FSWrapperWin32::MakePointInternal(const char* point)
{
  struct stat mystat;
  fs_navig_point* fsnp = nullptr;

  // check file permissions
  if (Stat(point, &mystat) == 0) {
    fsnp = (fs_navig_point*)malloc(sizeof(fs_navig_point));
    strcpy(fsnp->name, point);
    if (mystat.st_mode & _S_IFREG)
      fsnp->type = fs_navig_file_types::FS_NAVIG_FILE;
    else if (mystat.st_mode & _S_IFDIR)
      fsnp->type = fs_navig_file_types::FS_NAVIG_DIR;
    else
      fsnp->type = fs_navig_file_types::FS_NAVIG_OTHER;
    fsnp->writeable = !!(mystat.st_mode & _S_IWRITE);
    if (fsnp->writeable)
    {
      FILE* file_ptr = fopen(point, "a");
      if (file_ptr == nullptr)
      {
        fsnp->writeable = FALSE;
      }
      else
      {
        fclose(file_ptr);
      }
    }
    fsnp->size = mystat.st_size;
    fsnp->drive = 0;
    fsnp->relative = FALSE;
    fsnp->lnode = nullptr;
  }
  else
  {
    char* strError = strerror(errno);
    fellowShowRequester(FELLOW_REQUESTER_TYPE_ERROR, "fsWrapMakePoint(): ERROR getting file information for %s: error code %i (%s)\n", point, errno, strError);
  }
  return fsnp;
}
