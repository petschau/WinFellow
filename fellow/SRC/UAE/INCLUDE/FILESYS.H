 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Unix file system handler for AmigaDOS
  *
  * Copyright 1997 Bernd Schmidt
  */

/* FELLOW IN (START)-----------------

  This file has been adapted for use in WinFellow.
  It originates from the UAE 0.8.22 source code distribution.

  Torsten Enderling (carfesh@gmx.net) 2004

   FELLOW IN (END)------------------- */

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
#include "uae2fell.h"

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
    uae_thread_id tid;
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

extern struct uaedev_mount_info mountinfo;

/* FELLOW IN (END): moved from filesys.c because other modules need it */