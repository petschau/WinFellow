 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Unix file system handler for AmigaDOS
  * Windows related functions
  *
  * Copyright 2000 Torsten Enderling
  */

#ifndef __FILESYS_WIN32_H__
#define __FILESYS_WIN32_H__

#include "uae2fell.h"

/* replace the os-independant calls with calls for according win32 routines */

#define opendir  win32_opendir 
#define readdir  win32_readdir
#define closedir win32_closedir
#define mkdir    win32_mkdir

#define truncate win32_truncate

/* interfaces of the according win32 routines */

extern DIR *win32_opendir (const char *);
extern struct dirent *win32_readdir(DIR *);
extern void win32_closedir(DIR *);
extern int win32_mkdir(const char *, int);

extern int win32_truncate(const char *, long int);

#endif /* #ifndef __FILESYS_WIN32_H__ */