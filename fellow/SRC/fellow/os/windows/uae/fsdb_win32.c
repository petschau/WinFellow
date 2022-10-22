 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Library of functions to make emulated filesystem as independent as
  * possible of the host filesystem's capabilities.
  * This is the Win32 version.
  *
  * Copyright 1997 Mathias Ortmann
  * Copyright 1999 Bernd Schmidt
  */

/* FELLOW IN (START)-----------------

  This file has been adapted for use in WinFellow.
  It originates from the UAE 0.8.22 source code distribution.

  Torsten Enderling (carfesh@gmx.net) 2004

  @(#) $Id: fsdb_win32.c,v 1.11 2008-02-17 12:57:12 peschau Exp $

   FELLOW IN (END)------------------- */

/* FELLOW OUT (START)-------------------
#include "sysconfig.h"
#include "sysdeps.h"
/* FELLOW OUT (END)------------------- */
#include "fsdb.h"
#include <windows.h>
/* FELLOW IN (START)---------------- */
#include <stdio.h>
#ifdef WIN32
#include "filesys_win32.h"
#endif

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


/* FELLOW IN (END)---------------- */

/* these are deadly (but I think allowed on the Amiga): */
#define NUM_EVILCHARS 7
char evilchars[NUM_EVILCHARS] = { '\\', '*', '?', '\"', '<', '>', '|' };

/* Return nonzero for any name we can't create on the native filesystem.  */
int fsdb_name_invalid (const char *n)
{
    int i;
    char a = n[0];
    char b = (a == '\0' ? a : n[1]);
    char c = (b == '\0' ? b : n[2]);
    char d = (c == '\0' ? c : n[3]);
    size_t l = strlen (n);

    if (a >= 'a' && a <= 'z')
        a -= 32;
    if (b >= 'a' && b <= 'z')
        b -= 32;
    if (c >= 'a' && c <= 'z')
        c -= 32;

    /* reserved dos devices */
    if ((a == 'A' && b == 'U' && c == 'X' && l == 3) /* AUX  */
	|| (a == 'C' && b == 'O' && c == 'N' && l == 3) /* CON  */
	|| (a == 'P' && b == 'R' && c == 'N' && l == 3) /* PRN  */
	|| (a == 'N' && b == 'U' && c == 'L' && l == 3) /* NUL  */
	|| (a == 'L' && b == 'P' && c == 'T'  && (d >= '0' && d <= '9') && l == 4)  /* LPT# */
	|| (a == 'C' && b == 'O' && c == 'M'  && (d >= '0' && d <= '9') && l == 4)) /* COM# */
	return 1;

    /* spaces and periods at the beginning or the end are a no-no */
    if(n[0] == '.' || n[0] == ' ')
        return 1;
    l = strlen(n) - 1;
    if(n[l] == '.' || n[l] == ' ')
        return 1;

    /* these characters are *never* allowed */
    for (i=0; i < NUM_EVILCHARS; i++) {
        if (strchr (n, evilchars[i]) != 0)
            return 1;
    }

    /* the reserved fsdb filename */
    if (strcmp (n, FSDB_FILE) == 0)
        return 1;
    return 0; /* the filename passed all checks, now it should be ok */
}

uae_u32 filesys_parse_mask(uae_u32 mask)
{
    return mask ^ 0xf;
}

/* For an a_inode we have newly created based on a filename we found on the
 * native fs, fill in information about this file/directory.  */
void fsdb_fill_file_attrs (a_inode *aino)
{
    int mode;

	if((mode = GetFileAttributes(aino->nname)) == 0xFFFFFFFF) return;
	
    aino->dir = (mode & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
    /* @@@@@ FELLOW UNSURE (START) */
	aino->amigaos_mode = (FILE_ATTRIBUTE_ARCHIVE & mode) ? 0 : A_FIBF_ARCHIVE;
	aino->amigaos_mode |= 0xf; /* set rwed by default */
	/* @@@@@ FELLOW UNSURE (END) */
	aino->amigaos_mode = filesys_parse_mask(aino->amigaos_mode);
}

int fsdb_set_file_attrs (a_inode *aino, int mask)
{
    struct stat statbuf;
    uae_u32 mode=0, tmpmask;

    tmpmask = filesys_parse_mask(mask);
	
    if (stat (aino->nname, &statbuf) == -1)
	return ERROR_OBJECT_NOT_AROUND;
	
    /* Unix dirs behave differently than AmigaOS ones.  */
	/* windows dirs go where no dir has gone before...  */
    if (! aino->dir) {
	/* @@@@@ FELLOW UNSURE */
	if (tmpmask & A_FIBF_ARCHIVE)
	    mode |= FILE_ATTRIBUTE_ARCHIVE;
	else
	    mode &= ~FILE_ATTRIBUTE_ARCHIVE;

	SetFileAttributes(aino->nname, mode);
    }

    aino->amigaos_mode = mask;
    aino->dirty = 1;
    return 0;
}

/* Return nonzero if we can represent the amigaos_mode of AINO within the
 * native FS.  Return zero if that is not possible.  */
int fsdb_mode_representable_p (const a_inode *aino)
{
	/* @@@@@ FELLOW UNSURE */
    if (aino->dir)
	return aino->amigaos_mode == 0;
    return (aino->amigaos_mode & (
		A_FIBF_DELETE | 
		A_FIBF_SCRIPT | 
		A_FIBF_PURE | 
		A_FIBF_EXECUTE |
		A_FIBF_READ |
		A_FIBF_WRITE)) == 0;
}

char *fsdb_create_unique_nname (a_inode *base, const char *suggestion)
{
    char *c;
    char tmp[256] = "__uae___";
    int i;

	strncat (tmp, suggestion, 240);
	
    /* replace the evil ones... */
    for (i=0; i < NUM_EVILCHARS; i++)
        while ((c = strchr (tmp, evilchars[i])) != 0)
            *c = '_';
	
    while ((c = strchr (tmp, '.')) != 0)
        *c = '_';
	while ((c = strchr (tmp, ' ')) != 0)
        *c = '_';

    for (;;) {
	char *p = build_nname (base->nname, tmp);
	if (access (p, R_OK) < 0 && errno == ENOENT) {
	    write_log ("unique name: %s\n", p);
	    return p;
	}
	free (p);

	/* tmpnam isn't reentrant and I don't really want to hack configure
	 * right now to see whether tmpnam_r is available...  */
	for (i = 0; i < 8; i++) {
	    tmp[i+8] = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"[rand () % 63];
	}
    }
}