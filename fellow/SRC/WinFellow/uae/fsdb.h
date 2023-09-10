 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Library of functions to make emulated filesystem as independent as
  * possible of the host filesystem's capabilities.
  *
  * Copyright 1999 Bernd Schmidt
  */

/* FELLOW IN (START)-----------------

  This file has been adapted for use in WinFellow.
  It originates from the UAE 0.8.22 source code distribution.

  Torsten Enderling (carfesh@gmx.net) 2004

  @(#) $Id: fsdb.h,v 1.7 2008-02-17 12:57:02 peschau Exp $

   FELLOW IN (END)------------------- */

/* FELLOW IN (START)----------------- */

#include "uae2fell.h"
/* FELLOW IN (END)------------------- */

#ifndef FSDB_FILE
#define FSDB_FILE "_UAEFSDB.___"
#endif

#ifndef FSDB_DIR_SEPARATOR
/* FELLOW CHANGE: #define FSDB_DIR_SEPARATOR '/' */
#define FSDB_DIR_SEPARATOR '\\' /* windows dir separator */
#endif

/* AmigaOS errors */
#define ERROR_NO_FREE_STORE		103
#define ERROR_OBJECT_IN_USE		202
#define ERROR_OBJECT_EXISTS		203
#define ERROR_DIR_NOT_FOUND		204
#define ERROR_OBJECT_NOT_AROUND		205
#define ERROR_ACTION_NOT_KNOWN		209
#define ERROR_INVALID_LOCK		211
#define ERROR_OBJECT_WRONG_TYPE		212
#define ERROR_DISK_WRITE_PROTECTED	214
#define ERROR_DIRECTORY_NOT_EMPTY	216
#define ERROR_DEVICE_NOT_MOUNTED	218
#define ERROR_SEEK_ERROR		219
#define ERROR_DISK_IS_FULL		221
#define ERROR_DELETE_PROTECTED		222
#define ERROR_WRITE_PROTECTED		223
#define ERROR_READ_PROTECTED		224
#define ERROR_NO_MORE_ENTRIES		232
#define ERROR_NOT_IMPLEMENTED		236

#define A_FIBF_SCRIPT  (1<<6)
#define A_FIBF_PURE    (1<<5)
#define A_FIBF_ARCHIVE (1<<4)
#define A_FIBF_READ    (1<<3)
#define A_FIBF_WRITE   (1<<2)
#define A_FIBF_EXECUTE (1<<1)
#define A_FIBF_DELETE  (1<<0)

/* AmigaOS "keys" */
typedef struct a_inode_struct {
    /* Circular list of recycleable a_inodes.  */
    struct a_inode_struct *next, *prev;
    /* This a_inode's relatives in the directory structure.  */
    struct a_inode_struct *parent;
    struct a_inode_struct *child, *sibling;
    /* AmigaOS name, and host OS name.  The host OS name is a full path, the
     * AmigaOS name is relative to the parent.  */
    char *aname;
    char *nname;
    /* AmigaOS file comment, or NULL if file has none.  */
    char *comment;
    /* AmigaOS protection bits.  */
    int amigaos_mode;
    /* Unique number for identification.  */
    uae_u32 uniq;
	/* For a directory that is being ExNext()ed, the number of child ainos
       which must be kept locked in core.  */
    unsigned long locked_children;
    /* How many ExNext()s are going on in this directory?  */
    unsigned long exnext_count;
    /* AmigaOS locking bits.  */
    int shlock;
	long db_offset;
    unsigned int dir:1;
    unsigned int elock:1;
    /* Nonzero if this came from an entry in our database.  */
    unsigned int has_dbentry:1;
    /* Nonzero if this will need an entry in our database.  */
    unsigned int needs_dbentry:1;
    /* This a_inode possibly needs writing back to the database.  */
    unsigned int dirty:1;
    /* If nonzero, this represents a deleted file; the corresponding
     * entry in the database must be cleared.  */
    unsigned int deleted:1;
} a_inode;

extern char *nname_begin (char *);
/* FELLOW CHANGE (START): extern char *build_nname (const char *d, const char *n); */
#ifndef _FELLOW_DEBUG_CRT_MALLOC
extern char *build_nname (const char *d, const char *n); 
#else
/* added for malloc() debugging purposes */
extern char *my_strcpat(char *p, const char* d, const char *n);
#define build_nname(d,n) my_strcpat((char *)xmalloc(strlen (d) + strlen (n) + 2), d, n)
#endif
/* FELLOW CHANGE (END) */
extern char *build_aname (const char *d, const char *n);

/* Filesystem-independent functions.  */
extern void fsdb_clean_dir (a_inode *);
extern char *fsdb_search_dir (const char *dirname, char *rel);
extern void fsdb_dir_writeback (a_inode *);
extern int fsdb_used_as_nname (a_inode *base, const char *);
extern a_inode *fsdb_lookup_aino_aname (a_inode *base, const char *);
extern a_inode *fsdb_lookup_aino_nname (a_inode *base, const char *);

/* FELLOW CHANGE: STATIC_INLINE int same_aname (const char *an1, const char *an2) */
static int same_aname (const char *an1, const char *an2)
{
    return strcasecmp (an1, an2) == 0;
}

/* Filesystem-dependent functions.  */
extern int fsdb_name_invalid (const char *n);
extern void fsdb_fill_file_attrs (a_inode *);
extern int fsdb_set_file_attrs (a_inode *, int);
extern int fsdb_mode_representable_p (const a_inode *);
extern char *fsdb_create_unique_nname (a_inode *base, const char *);
