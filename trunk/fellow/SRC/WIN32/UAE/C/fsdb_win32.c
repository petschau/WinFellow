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

/* FELLOW IN (START)

  This file is somewhat adapted to suit Fellow's way of controlling its modules.
  It originates from the UAE 0.8.15 source code distribution.

  Torsten Enderling (carfesh@gmx.net) 2000

  FELLOW IN (END) */

/* FELLOW OUT (START)-------------------
#include "sysconfig.h"
#include "sysdeps.h"
/* FELLOW OUT (END)------------------- */

/* FELLOW IN (START)---------------- */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
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

#include "fsdb.h"


/* Return nonzero for any name we can't create on the native filesystem.  */
int fsdb_name_invalid (const char *n)
{
    char a = n[0];
    char b = (a == '\0' ? a : n[1]);
    char c = (b == '\0' ? b : n[2]);

    if (a >= 'a' && a <= 'z')
	a -= 32;
    if (b >= 'a' && b <= 'z')
	b -= 32;
    if (c >= 'a' && c <= 'z')
	c -= 32;

    if ((a == 'A' && b == 'U' && c == 'X')
	|| (a == 'C' && b == 'O' && c == 'N')
	|| (a == 'P' && b == 'R' && c == 'N')
	|| (a == 'N' && b == 'U' && c == 'L'))
	return 1;

    if (strchr (n, '\\') != 0)
	return 1;

    if (strcmp (n, FSDB_FILE) == 0)
	return 1;
    if (n[0] != '.')
	return 0;
    if (n[1] == '\0')
	return 1;
    return n[1] == '.' && n[2] == '\0';
}

/* FELLOW IN (START): routine to parse the amiga file mask properly */

uae_u32 filesys_parse_mask(uae_u32 mask)
{
    	/* according to the Amiga Guru Book, mask looks as follows

       bit      | 31-24 | 23-8     | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0  
      ----------+-------+----------+---+---+---+---+---+---+---+---
       function | user  | reserved | H | S | P | A |!R |!W |!E |!D  

       where the flags mean Delete, Execute, Write, Read, Archive,
	       Pure, Script and Hold

       so to parse the mask more easily, the last 4 bits should be xor'ed */

    uae_u32 result;
    result = mask  ^ 0xf;
/*
    write_log("parse_amiga_mask(\"%s\"): ", fname);
    write_log((result & (1<<7)) != 0 ? "H" : "-");
    write_log((result & (1<<6)) != 0 ? "S" : "-");
    write_log((result & (1<<5)) != 0 ? "P" : "-");
    write_log((result & (1<<4)) != 0 ? "A" : "-");
    write_log((result & (1<<3)) != 0 ? "R" : "-");
    write_log((result & (1<<2)) != 0 ? "W" : "-");
    write_log((result & (1<<1)) != 0 ? "E" : "-");
    write_log((result & (1<<0)) != 0 ? "D\n" : "-\n");
*/
	return result;
}

/* FELLOW IN (END): routine to parse the amiga file mask properly */

/* For an a_inode we have newly created based on a filename we found on the
 * native fs, fill in information about this file/directory.  */
void fsdb_fill_file_attrs (a_inode *aino)
{
    int mode = 0;
	DWORD error;

	if((mode = GetFileAttributes(aino->nname)) == 0xFFFFFFFF) return;
	
    aino->dir = (mode & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
    aino->amigaos_mode = (FILE_ATTRIBUTE_ARCHIVE & mode) ? 0 : A_FIBF_ARCHIVE;
	aino->amigaos_mode = filesys_parse_mask(aino->amigaos_mode);
}

int fsdb_set_file_attrs (a_inode *aino, uae_u32 mask)
{
    struct stat statbuf;
    uae_u32 mode=0, tmpmask;

    tmpmask = filesys_parse_mask(mask);
	
    if (stat (aino->nname, &statbuf) == -1)
	return ERROR_OBJECT_NOT_FOUND;
	
    /* Unix dirs behave differently than AmigaOS ones.  */
	/* windows dirs go where no dir has gone before...  */
    if (! aino->dir) {
	
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
    strncat (tmp, suggestion, 240);

    /* @@@ Brian... this may or may not need fixing.  */
    while ((c = strchr (tmp, '\\')) != 0)
	*c = '_';
    while ((c = strchr (tmp, '.')) != 0)
	*c = '_';

    for (;;) {
	int i;
	char *p = build_nname (base->nname, tmp);
	if (access (p, R_OK) < 0 && errno == ENOENT) {
	    printf ("unique name: %s\n", p);
	    return p;
	}
	free (p);

	/* tmpnam isn't reentrant and I don't really want to hack configure
	 * right now to see whether tmpnam_r is available...  */
	for (i = 0; i < 8; i++) {
	    tmp[i] = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"[random () % 63];
	}
    }
}