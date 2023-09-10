/* @(#) $Id: sysinfo.h,v 1.4 2008-02-23 20:45:11 carfesh Exp $ */
/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* System information retrieval                                            */
/*                                                                         */
/* Author: Torsten Enderling (carfesh@gmx.net)                             */
/*                                                                         */
/* Outputs valuable debugging information like hardware details and        */
/* OS specifics into the logfile                                           */
/*                                                                         */
/* Partially based on MSDN Library code examples; reference IDs are        */
/* Q117889 Q124207 Q124305                                                 */
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

#ifndef _SYSINFO_H_
#define _SYSINFO_H_

void sysinfoLogSysInfo(void);

#endif