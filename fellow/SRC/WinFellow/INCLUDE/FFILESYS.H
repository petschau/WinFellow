#pragma once

#include <string>

#define FFILESYS_MAX_DEVICES 20
#define FFILESYS_MAX_VOLUMENAME 64

typedef enum
{
  FFILESYS_NONE = 0,
  FFILESYS_INSERTED = 1
} ffilesys_status;

typedef struct
{
  char volumename[FFILESYS_MAX_VOLUMENAME];
  char rootpath[CFG_FILENAME_LENGTH];
  BOOLE readonly;
  ffilesys_status status;
} ffilesys_dev;

/* Configuring the filesys */

extern void ffilesysSetEnabled(bool enabled);
extern bool ffilesysGetEnabled();
extern void ffilesysSetFilesys(ffilesys_dev filesys, uint32_t index);
extern BOOLE ffilesysCompareFilesys(ffilesys_dev filesys, uint32_t index);
extern BOOLE ffilesysRemoveFilesys(uint32_t index);
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
