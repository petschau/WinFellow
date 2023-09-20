/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* Filesys autmount functionality; moved here from WinUAE 0.8.8 filesys.c  */
/* to keep the UAE filesys.c as original as possible                       */ 
/*                                                                         */
/* (w)2004 by Torsten Enderling (carfesh@gmx.net)                          */
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

#include <windows.h>
#include <stdio.h>

#ifdef _FELLOW_DEBUG_CRT_MALLOC
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#ifdef _FELLOW_DEBUG_CRT_MALLOC
#include <crtdbg.h>
#endif

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <direct.h>
#include <io.h>
#include <string.h>
#include <sys/utime.h>

#include "uae2fell.h"
#include "penguin.h"
#include "filesys.h"
#include "autoconf.h"
#include "fsusage.h"

/* references to stuff defined in filesys.c */

extern struct uaedev_mount_info mountinfo;

/*===========================================================================*/
/* automount drives if wanted                                                */
/*===========================================================================*/

void filesys_init(int automount_drives)
{
  UINT errormode = SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );
  char volumename[MAX_PATH]="";
  char volumepath[6];
  char *result = nullptr;

  mountinfo.num_units = 0;
  if ( automount_drives) {
    if (memoryGetKickImageVersion() >= 36)
    {
      DWORD dwDriveMask = GetLogicalDrives();

      for( int drive = 'A'; drive <= 'Z'; sprintf( volumepath, "%c:\\", ++drive ) )
      {
	if( ( dwDriveMask & 1 ) && CheckRM( volumepath ) ) /* Is this drive-letter valid and media in drive? */
	{
	  int drivetype = GetDriveType(volumepath);
	  if( drivetype == DRIVE_REMOTE )
	    strcat( volumepath, "." );
	  else
	    strcat( volumepath, ".." );
	  int readonly = (drivetype == DRIVE_CDROM) ? 1 : 0;
	  int removable = (drivetype == DRIVE_CDROM || drivetype == DRIVE_REMOVABLE) ? 1 : 0;

	  if( get_volume_name( &mountinfo, volumepath, volumename, MAX_PATH, drive, drivetype, 1 ) )
	  {
	    result = add_filesys_unit( &mountinfo, volumename, volumepath, readonly, 0, 0, 0, 0);
	    if( result )
	      write_log( result );
	  }
	} /* if drivemask */
	dwDriveMask >>= 1;
      }
    }
  }
  SetErrorMode( errormode );
}

static int get_volume_name(struct uaedev_mount_info *mtinf, char *volumepath, char *volumename, int size, int drive, int drivetype, int fullcheck)
{
  int result = -1;

  if( GetVolumeInformation( volumepath, volumename, size, nullptr, nullptr, nullptr, nullptr, 0 ) && volumename[0] && valid_volumename( mtinf, volumename, fullcheck ) )
  {
    result = 1;
  }
  else
  {
    result = 2;
    switch( drivetype )
    {
    case DRIVE_FIXED:
      sprintf( volumename, "WinDH_%c", drive );
      break;
    case DRIVE_CDROM:
      sprintf( volumename, "WinCD_%c", volumepath[0] );
      break;
    case DRIVE_REMOVABLE:
      sprintf( volumename, "WinRMV_%c", volumepath[0] );
      break;
    case DRIVE_REMOTE:
      sprintf( volumename, "WinNET_%c", volumepath[0] );
      break;
    case DRIVE_RAMDISK:
      sprintf( volumename, "WinRAM_%c", volumepath[0] );
      break;
    case DRIVE_UNKNOWN:
    case DRIVE_NO_ROOT_DIR:
    default:
      result = 0;
      break;
    }
  }
  return result;
}

BOOLE CheckRM(char *DriveName)
{
  char filename[ MAX_PATH ];
  BOOL result = FALSE;

  sprintf( filename, "%s.", DriveName );
  DWORD dwHold = GetFileAttributes(filename);
  if( dwHold != 0xFFFFFFFF )
    result = TRUE;
  return result;
}

/* This function makes sure the volume-name being requested is not already in use, or any of the following
illegal values: */

int valid_volumename( struct uaedev_mount_info *mountinfo, char *volumename, int fullcheck )
{
  char *illegal_volumenames[7] = { "SYS", "DEVS", "LIBS", "FONTS", "C", "L", "S" };

  int i, result = 1;
  for( i = 0; i < 7; i++ )
  {
    if( strcmp( volumename, illegal_volumenames[i] ) == 0 )
    {
      result = 0;
      break;
    }
  }
  /* if result is still good, we've passed the illegal names check, and must check for duplicates now */
  if( result && fullcheck)
  {
    for( i = 0; i < mountinfo->num_units; i++ )
    {
      if( mountinfo->ui[i].volname && ( strcmp( mountinfo->ui[i].volname, volumename ) == 0 ) )
      {
	result = 0;
	break;
      }
    }
  }
  return result;
}