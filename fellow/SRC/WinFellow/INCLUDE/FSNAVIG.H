#pragma once

#include "fswrap.h"
#include "ListTree.h"

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
  felist *lnode;
} fs_navig_point;

extern BOOLE fsNavigHasDrives(void);
extern BOOLE *fsNavigGetDriveMap(void);
extern void fsNavigSetCWDInstallDir(void);
extern void fsNavigSetCWDStartupDir(void);
extern void fsNavigStartup(char *argv[]);
extern void fsNavigShutdown(void);
extern felist *fsNavigGetDirList(fs_navig_point *fs_point);
extern fs_navig_point *fsNavigGetCWD(void);
extern void fsNavigSetCWD(fs_navig_point *fs_point);
