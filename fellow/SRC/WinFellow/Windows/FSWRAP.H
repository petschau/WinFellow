#ifndef FSWRAP_H
#define FSWRAP_H

#define FS_WRAP_MAX_PATH_LENGTH       256
#define FS_WRAP_PATH_SEPARATOR_STR    "\\"
#define FS_WRAP_PATH_SEPARATOR_CHAR   '\\'

#include "fsnavig.h"


extern void fsWrapFullPath(STR *dst, STR *src);

extern BOOLE fsWrapHasDrives(void);
extern BOOLE *fsWrapGetDriveMap(void);
extern BOOLE fsWrapSetCWD(fs_navig_point *fs_point);
fs_navig_point *fsWrapGetCWD(void);
extern BOOLE fsWrapOpenDir(fs_navig_point *fs_point);
extern fs_navig_point *fsWrapReadDir(void);
extern void fsWrapCloseDir(void);
extern fs_navig_point *fsWrapMakePoint(const STR *point);
extern int fsWrapStat(const char *szFilename, struct stat *pStatBuffer);

#endif





