 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Unix file system handler for AmigaDOS
  *
  * Copyright 1997 Bernd Schmidt
  */

#define A_FIBF_SCRIPT  (1<<6)
#define A_FIBF_PURE    (1<<5)
#define A_FIBF_ARCHIVE (1<<4)
#define A_FIBF_READ    (1<<3)
#define A_FIBF_WRITE   (1<<2)
#define A_FIBF_EXECUTE (1<<1)
#define A_FIBF_DELETE  (1<<0)

#ifndef _WIN32
#define MAKE_CASE_INSENSITIVE
#endif

/* errors - stolen from dos/dos.h */
#if 1
/* Errors from IoErr(), etc. */
#define ERROR_NO_FREE_STORE		  103
#define ERROR_TASK_TABLE_FULL		  105
#define ERROR_BAD_TEMPLATE		  114
#define ERROR_BAD_NUMBER		  115
#define ERROR_REQUIRED_ARG_MISSING	  116
#define ERROR_KEY_NEEDS_ARG		  117
#define ERROR_TOO_MANY_ARGS		  118
#define ERROR_UNMATCHED_QUOTES		  119
#define ERROR_LINE_TOO_LONG		  120
#define ERROR_FILE_NOT_OBJECT		  121
#define ERROR_INVALID_RESIDENT_LIBRARY	  122
#define ERROR_NO_DEFAULT_DIR		  201
#define ERROR_OBJECT_IN_USE		  202
#define ERROR_OBJECT_EXISTS		  203
#define ERROR_DIR_NOT_FOUND		  204
#define ERROR_OBJECT_NOT_FOUND		  205
#define ERROR_BAD_STREAM_NAME		  206
#define ERROR_OBJECT_TOO_LARGE		  207
#define ERROR_ACTION_NOT_KNOWN		  209
#define ERROR_INVALID_COMPONENT_NAME	  210
#define ERROR_INVALID_LOCK		  211
#define ERROR_OBJECT_WRONG_TYPE		  212
#define ERROR_DISK_NOT_VALIDATED	  213
#define ERROR_DISK_WRITE_PROTECTED	  214
#define ERROR_RENAME_ACROSS_DEVICES	  215
#define ERROR_DIRECTORY_NOT_EMPTY	  216
#define ERROR_TOO_MANY_LEVELS		  217
#define ERROR_DEVICE_NOT_MOUNTED	  218
#define ERROR_SEEK_ERROR		  219
#define ERROR_COMMENT_TOO_BIG		  220
#define ERROR_DISK_IS_FULL			  221
#define ERROR_DELETE_PROTECTED		  222
#define ERROR_WRITE_PROTECTED		  223
#define ERROR_READ_PROTECTED		  224
#define ERROR_NOT_A_DOS_DISK		  225
#define ERROR_NO_DISK			  226
#define ERROR_NO_MORE_ENTRIES		  232
/* added for 1.4 */
#define ERROR_IS_SOFT_LINK		  233
#define ERROR_OBJECT_LINKED		  234
#define ERROR_BAD_HUNK			  235
#define ERROR_NOT_IMPLEMENTED		  236
#define ERROR_RECORD_NOT_LOCKED		  240
#define ERROR_LOCK_COLLISION		  241
#define ERROR_LOCK_TIMEOUT		  242
#define ERROR_UNLOCK_ERROR		  243

#else

#define ERROR_NO_FREE_STORE	        103
#define ERROR_OBJECT_IN_USE	        202
#define ERROR_OBJECT_EXISTS	        203
#define ERROR_DIR_NOT_FOUND	        204
#define ERROR_OBJECT_NOT_FOUND	    205
#define ERROR_ACTION_NOT_KNOWN	    209
#define ERROR_INVALID_LOCK          211
#define ERROR_OBJECT_WRONG_TYPE	    212
#define ERROR_DISK_WRITE_PROTECTED  214
#define ERROR_DIRECTORY_NOT_EMPTY   216
#define ERROR_DEVICE_NOT_MOUNTED    218
#define ERROR_SEEK_ERROR            219
#define ERROR_DISK_IS_FULL          221
#define ERROR_DELETE_PROTECTED      222
#define ERROR_WRITE_PROTECTED       223
#define ERROR_READ_PROTECTED        224
#define ERROR_NO_MORE_ENTRIES       232
#define ERROR_NOT_IMPLEMENTED       236

#endif

struct hardfiledata {
    unsigned long size;
    int nrcyls;
    int secspertrack;
    int surfaces;
    int reservedblocks;
    int blocksize;
    FILE *fd;
};

struct uaedev_mount_info;

extern struct hardfiledata *get_hardfile_data (int nr);


/* FELLOW IN (START): moved from filesys.c because other modules need it */

#include "penguin.h"

typedef struct {
    char *devname; /* device name, e.g. UAE0: */
    uaecptr devname_amiga;
    uaecptr startup;
    char *volname; /* volume name, e.g. CDROM, WORK, etc. */
    char *rootdir; /* root unix directory */
    int readonly; /* disallow write access? */
    int devno;
    
    struct hardfiledata hf;

    /* Threading stuff */
    smp_comm_pipe *unit_pipe, *back_pipe;
    penguin_id tid;
    struct _unit *volatile self;
    /* Reset handling */
    uae_sem_t reset_sync_sem;
    int reset_state;
} UnitInfo;

#define MAX_UNITS 20

struct uaedev_mount_info {
    int num_units;
    UnitInfo ui[MAX_UNITS];
};
/* FELLOW IN (END): moved from filesys.c because other modules need it */