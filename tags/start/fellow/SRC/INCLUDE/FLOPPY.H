#ifndef FLOPPY_H
#define FLOPPY_H


/* Assembly function renaming */

#define rdskbytr ASMRENAME(rdskbytr)
#define wdskpth ASMRENAME(wdskpth)
#define wdskptl ASMRENAME(wdskptl)
#define wdsklen ASMRENAME(wdsklen)
#define wdsksync ASMRENAME(wdsksync)

ULO rdskbytr(ULO);
void wdskpth(ULO, ULO);
void wdskptl(ULO, ULO);
void wdsklen(ULO, ULO);
void wdsksync(ULO, ULO);


#define FLOPPY_TRACKS 164

/* Info about a track */

typedef struct {
  ULO sync;       /* Sync word */
  ULO length;     /* Track length */
  BOOLE valid;    /* Data found in data is valid (ie. loaded from disk) */
  ULO fileoffset; /* Track starts at this offset in image file */
  UBY *buffer;    /* Pointer to MFM data for track */
} floppytrackinfostruct;

/* Info about a drive */

typedef struct {
  FILE *F;                /* Open file for disk image */
  ULO tracks;             /* Number of tracks on drive */
  BOOLE zipped;           /* This image is zipped */
  ULO compress_serno;     /* Tmp file serial number */
  BOOLE sel;              /* Drive selected when TRUE */
  ULO track;              /* Current head position */
  BOOLE writeprot;        /* Drive is writeprotected */
  BOOLE dir;              /* Direction for step */
  BOOLE motor;            /* Motor on or off */
  BOOLE side;             /* Which side is active */
  BOOLE step;             /* Step line */
  BOOLE enabled;          /* Drive enabled */
  BOOLE changed;          /* Disk has been changed */
  BOOLE idmode;           /* Drive is in ID-mode */
  BOOLE inserted;         /* Disk is inserted */
  ULO insertedframe;      /* Will not be detected until some time after this */
  ULO idcount;            /* Number of times ID has been read */
  ULO lasttrack;          /* Last track accessed on drive */
  ULO lasttrackcount;     /* Number of consecutive reads of the same track */
  BOOLE cached;           /* This image is fully cached in memory */
  UBY *cache;             /* Memory allocated to hold data on this drive */
  floppytrackinfostruct trackinfo[FLOPPY_TRACKS]; /* Info about each track */
  ULO imagestatus;        /* Status of drive (kind of image inserted) */
  ULO imageerror;         /* What kind of error if status reports error */
  STR imagename[CFG_FILENAME_LENGTH];  /* Image name presented to user */
  STR imagenamereal[CFG_FILENAME_LENGTH];/* Image name used internally */
} floppyinfostruct;
  
/* Info about a started disk transfer */

typedef struct {
  UBY *src;              /* Source data ptr */
  UBY *dst;              /* Destination ptr */
  ULO offset;            /* Current index into source data */
  ULO wordsleft;         /* Words left to transfer */
  ULO drive;             /* Drive from which data is transfered */
  ULO wait;              /* Lines to wait before starting transfer */ 
  ULO tracklength;       /* Length of track */
} floppyDMAinfostruct;

/* Status symbols */

#define FLOPPY_STATUS_NORMAL_OK   0
#define FLOPPY_STATUS_EXTENDED_OK 1
#define FLOPPY_STATUS_ERROR       2
#define FLOPPY_STATUS_NONE        3

#define FLOPPY_ERROR_EXISTS_NOT           0
#define FLOPPY_ERROR_COMPRESS             1
#define FLOPPY_ERROR_COMPRESS_NOTMP       2
#define FLOPPY_ERROR_COMPRESS_TMPFILEOPEN 3
#define FLOPPY_ERROR_FILE                 4
#define FLOPPY_ERROR_SIZE                 5

/* Config */

#define FLOPPY_WAIT_INITIAL 10

extern floppyinfostruct floppy[4];
extern floppyDMAinfostruct floppy_DMA;
extern BOOLE floppy_fast;
extern BOOLE floppy_DMA_started;
extern ULO diskdmaen;   /* Counts the number of writes to dsklen */

extern void floppyCacheAllocate(void);
extern void floppyCacheFree(void);
extern void floppyDMAStart(void);
extern void floppyStartup(void);
extern void floppyShutdown(void);
extern LON floppySelectedGet(void);
extern void floppySelectedSet(ULO selbits);
extern BOOLE floppyIsTrack0(ULO drive);
extern BOOLE floppyIsWriteProtected(ULO drive);
extern BOOLE floppyIsReady(ULO drive);
extern BOOLE floppyIsChanged(ULO drive);
extern void floppyMotorSet(ULO drive, BOOLE mtr);
extern void floppySideSet(BOOLE s);
extern void floppyDirSet(BOOLE dr);
extern void floppyStepSet(BOOLE stp);

extern void copy_bootblock(void);

/* Configuration */

extern void floppySetDiskImage(ULO drive, STR *diskname);
extern void floppySetEnabled(ULO drive, BOOLE enabled);
extern void floppySetReadOnly(ULO drive, BOOLE readonly);
extern void floppySetFastDMA(BOOLE fastDMA);

/* Module control */

extern void floppyStartup(void);
extern void floppyShutdown(void);


#endif
