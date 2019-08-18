#include "fellow/service/FSWrapper.h"
#include "fellow/api/Services.h"
#include "fellow/os/windows/io/FSWrapStat.h"

using namespace std;
using namespace fellow::api;

namespace fellow::service
{
  //===========================================================================
  // Fills in file or directory attributes in fs_object_info structure
  //
  // Depends on OS specific stat wrapper fsWrapStat()
  //
  // Return the fs_object_info object, or nullptr on error
  //===========================================================================

  fs_wrapper_object_info *FSWrapper::GetFSObjectInfo(const string &pathToObject)
  {
    struct stat mystat;

    // check file permissions
    if (fsWrapStat(pathToObject, &mystat) != 0)
    {
      const STR *strError = strerror(errno);
      Service->Log.AddLog("GetFSObjectInfo(): ERROR getting file information for %s: error code %i (%s)\n", pathToObject.c_str(), errno, strError);
      return nullptr;
    }

    fs_wrapper_object_info *fsnp = new fs_wrapper_object_info();

    fsnp->name = pathToObject;
    fsnp->size = mystat.st_size;
    fsnp->type = GetFSObjectType(mystat.st_mode);
    fsnp->writeable = GetFSObjectIsWriteable(mystat.st_mode, pathToObject);

    return fsnp;
  }

  fs_wrapper_object_types FSWrapper::GetFSObjectType(unsigned short st_mode)
  {
    if (st_mode & _S_IFREG) return fs_wrapper_object_types::FILE;
    if (st_mode & _S_IFDIR) return fs_wrapper_object_types::DIRECTORY;

    return fs_wrapper_object_types::OTHER;
  }

  bool FSWrapper::GetFSObjectIsWriteable(unsigned short st_mode, const string &pathToObject)
  {
    bool isWriteable = st_mode & _S_IWRITE;
    if (isWriteable)
    {
      FILE *file_ptr = fopen(pathToObject.c_str(), "a");
      if (file_ptr == nullptr)
      {
        isWriteable = false;
      }
      else
      {
        fclose(file_ptr);
      }
    }

    return isWriteable;
  }

}
