/*=========================================================================*/
/* WinFellow                                                               */
/* Ini file for Windows                                                    */
/* Author: Worfje (worfje@gmx.net)                                         */
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
#include "fellow/api/defs.h"
#include "versioninfo.h"
#include <windows.h>
#include <direct.h>
#include "fellow/application/Ini.h"
#include "fellow/application/HostRenderer.h"
#include "fellow/api/Services.h"

using namespace fellow::api;

#define INI_FILENAME "WinFellow.ini"

/*============================================================================*/
/* The actual iniManager instance                                             */
/*============================================================================*/

iniManager ini_manager;
char ini_filename[MAX_PATH];
char ini_default_config_filename[MAX_PATH];
