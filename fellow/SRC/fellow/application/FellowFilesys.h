#pragma once

#include <string>

constexpr auto FFILESYS_MAX_DEVICES = 20;
constexpr auto FFILESYS_MAX_VOLUMENAME = 64;

typedef enum
{
  FFILESYS_NONE = 0,
  FFILESYS_INSERTED = 1
} ffilesys_status;

struct ffilesys_dev
{
  STR volumename[FFILESYS_MAX_VOLUMENAME];
  STR rootpath[CFG_FILENAME_LENGTH];
  bool readonly;
  ffilesys_status status;
};

/* Configuring the filesys */

extern void ffilesysSetEnabled(bool enabled);
extern bool ffilesysGetEnabled();
extern void ffilesysSetFilesys(ffilesys_dev filesys, ULO index);
extern BOOLE ffilesysCompareFilesys(ffilesys_dev filesys, ULO index);
extern bool ffilesysRemoveFilesys(ULO index);
extern void ffilesysClear();
extern void ffilesysSetAutomountDrives(BOOLE automount_drives);
extern BOOLE ffilesysGetAutomountDrives();
extern void ffilesysSetDeviceNamePrefix(const std::string &prefix);
extern const std::string &ffilesysGetDeviceNamePrefix();

/* Starting and stopping the filesys */

extern void ffilesysHardReset();
extern void ffilesysEmulationStart();
extern void ffilesysEmulationStop();
extern void ffilesysStartup();
extern void ffilesysShutdown();
