/* @(#) $Id: RetroPlatform.h,v 1.15 2012-12-30 13:05:08 carfesh Exp $ */
/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* This file contains RetroPlatform specific functionality to register as  */
/* guest and interact with the host.                                       */
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

#ifndef RETROPLATFORM_H
#define RETROPLATFORM_H

#ifdef RETRO_PLATFORM

#include "gfxdrv.h"

extern void  RetroPlatformEmulationStart(void);
extern void  RetroPlatformEmulationStop(void);
extern void  RetroPlatformEndOfFrame(void);
extern void  RetroPlatformEnter(void);
extern const STR *RetroPlatformGetActionName(const RetroPlatformActions);
extern ULO   RetroPlatformGetEscapeKey(void);
extern BOOLE RetroPlatformGetMode(void);
extern BOOLE RetroPlatformGetMouseCaptureRequestedByHost(void);
extern HWND  RetroPlatformGetParentWindowHandle(void);
extern void  RetroPlatformSendActivate(const BOOLE, const LPARAM);
extern void  RetroPlatformSendClose(void);
extern void  RetroPlatformSendMouseCapture(const BOOLE);
extern void  RetroPlatformSendScreenMode(HWND);
extern void  RetroPlatformSetAction(const RetroPlatformActions);
extern void  RetroPlatformSetEscapeKey(const char *);
extern void  RetroPlatformSetEscapeKeyHoldTime(const char *);
extern void  RetroPlatformSetEscapeKeyTargetHoldTime(const BOOLE);
extern void  RetroPlatformSetHostID(const char *);
extern void  RetroPlatformSetMode(const BOOLE);
extern void  RetroPlatformSetScreenMode(const char *);
extern void  RetroPlatformSetWindowInstance(HINSTANCE);
extern void  RetroPlatformShutdown(void);
extern void  RetroPlatformStartup(void);

#endif

#endif