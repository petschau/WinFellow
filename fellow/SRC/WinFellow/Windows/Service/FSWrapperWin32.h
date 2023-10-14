#pragma once

#include "Service/IFSWrapper.h"
#include "DEFS.H"
#include "LISTTREE.H"

#define FS_WRAP_MAX_PATH_LENGTH       256

typedef enum {
  FS_NAVIG_FILE,
  FS_NAVIG_DIR,
  FS_NAVIG_OTHER
} fs_navig_file_types;

typedef struct {
  uint8_t drive;
  char name[FS_WRAP_MAX_PATH_LENGTH];
  BOOLE relative;
  BOOLE writeable;
  uint32_t size;
  fs_navig_file_types type;
  felist* lnode;
} fs_navig_point;

class FSWrapperWin32 : public Service::IFSWrapper
{
private:
  Service::fs_wrapper_file_types MapFileType(fs_navig_file_types type);
  fs_navig_point* MakePointInternal(const char* point);

public:
  Service::fs_wrapper_point *MakePoint(const char *point) override;
  int Stat(const char* szFilename, struct stat* pStatBuffer) override;
  FSWrapperWin32();
};
