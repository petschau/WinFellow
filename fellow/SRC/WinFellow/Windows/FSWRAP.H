#pragma once

#define FS_WRAP_MAX_PATH_LENGTH       256
#define FS_WRAP_PATH_SEPARATOR_STR    "\\"
#define FS_WRAP_PATH_SEPARATOR_CHAR   '\\'

#include "fsnavig.h"


extern void fsWrapFullPath(char *dst, char *src);

extern BOOLE fsWrapHasDrives();
extern BOOLE *fsWrapGetDriveMap();
extern BOOLE fsWrapSetCWD(fs_navig_point *fs_point);
fs_navig_point *fsWrapGetCWD();
extern BOOLE fsWrapOpenDir(fs_navig_point *fs_point);
extern fs_navig_point *fsWrapReadDir();
extern void fsWrapCloseDir();
extern fs_navig_point *fsWrapMakePoint(const char *point);
extern int fsWrapStat(const char *szFilename, struct stat *pStatBuffer);
