/* @(#) $Id: RetroPlatform.h,v 1.2 2012-12-11 17:52:17 carfesh Exp $ */
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

typedef enum {
  RETRO_PLATFORM_NO_ACTION,
  RETRO_PLATFORM_START_EMULATION,
  RETRO_PLATFORM_QUIT_EMULATOR,
  RETRO_PLATFORM_CONFIGURATION,
  RETRO_PLATFORM_OPEN_CONFIGURATION,
  RETRO_PLATFORM_SAVE_CONFIGURATION,
  RETRO_PLATFORM_SAVE_CONFIGURATION_AS,
  RETRO_PLATFORM_LOAD_HISTORY0,
  RETRO_PLATFORM_LOAD_HISTORY1,
  RETRO_PLATFORM_LOAD_HISTORY2,
  RETRO_PLATFORM_LOAD_HISTORY3,
  RETRO_PLATFORM_DEBUGGER_START,
  RETRO_PLATFORM_ABOUT,
  RETRO_PLATFORM_LOAD_STATE,
  RETRO_PLATFORM_SAVE_STATE
} RetroPlatformActions;

BOOLE RetroPlatformGetMode(void);
void  RetroPlatformSetAction(const RetroPlatformActions);
void  RetroPlatformSetEscapeKey(const char *);
void  RetroPlatformSetEscapeHoldTime(const char *);
void  RetroPlatformSetHostID(const char *);
void  RetroPlatformSetMode(BOOLE);
void  RetroPlatformSetScreenMode(const char *);
void  RetroPlatformStartup(void);

BOOLE RetroPlatformEnter(void);

#endif

#endif