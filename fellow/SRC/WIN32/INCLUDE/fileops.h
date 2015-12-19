/* @(#) $Id: fileops.h,v 1.6 2012-12-07 14:05:43 carfesh Exp $             */
/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* Filesystem operations                                                   */
/*                                                                         */
/* Author: Torsten Enderling (carfesh@gmx.net)                             */
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

#ifndef FILEOPS_H
#define FILEOPS_H

extern BOOLE fileopsGetFellowLogfileName(char *);
extern BOOLE fileopsGetGenericFileName(char *, const char *, const char *);
extern BOOLE fileopsGetDefaultConfigFileName(char *);
extern BOOLE fileopsResolveVariables(const char *, char *);
extern BOOLE fileopsGetWinFellowPresetPath(char *, const DWORD);
extern BOOLE fileopsGetScreenshotFileName(char *);
extern char *fileopsGetTemporaryFilename(void);
extern bool fileopsGetWinFellowInstallationPath(char *, const DWORD);
extern bool fileopsGetKickstartByCRC32(const char *, const ULO, char *, const ULO);

#endif // FILEOPS_H