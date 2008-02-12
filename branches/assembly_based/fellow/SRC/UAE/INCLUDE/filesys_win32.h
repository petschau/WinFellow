/* @(#) $Id: filesys_win32.h,v 1.4 2004-06-08 14:10:32 carfesh Exp $ */
/*=========================================================================*/
/* UAE - The Un*x Amiga Emulator                                           */
/*                                                                         */
/* Unix file system handler for AmigaDOS                                   */
/* Windows related functions                                               */
/*                                                                         */
/* (w)2004 by Torsten Enderling <carfesh@gmx.net>                          */
/*                                                                         */
/* Copyright (C) 1991, 1992, 1996 Free Software Foundation, Inc.           */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2, or (at your option)     */
/* any later version.                                                      */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software Foundation, */
/* Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.          */
/*=========================================================================*/

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