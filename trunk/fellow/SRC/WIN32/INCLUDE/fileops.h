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

BOOLE fileopsGetFellowLogfileName(char *);
BOOLE fileopsGetGenericFileName(char *, const char *, const char *);
BOOLE fileopsGetDefaultConfigFileName(char *);
BOOLE fileopsResolveVariables(const char *, char *);

#endif // FILEOPS_H