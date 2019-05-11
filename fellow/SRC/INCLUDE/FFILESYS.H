#ifndef FFILESYS_H
#define FFILESYS_H

#define FFILESYS_MAX_DEVICES 20
#define FFILESYS_MAX_VOLUMENAME 64

typedef enum {
  FFILESYS_NONE = 0,
  FFILESYS_INSERTED = 1
} ffilesys_status;

typedef struct {
  STR		  volumename[FFILESYS_MAX_VOLUMENAME];
  STR		  rootpath[CFG_FILENAME_LENGTH];
  BOOLE		  readonly;
  ffilesys_status status;
} ffilesys_dev;


/* Configuring the filesys */

extern void ffilesysSetEnabled(bool enabled);
extern bool ffilesysGetEnabled();
extern void ffilesysSetFilesys(ffilesys_dev filesys, ULO index);
extern BOOLE ffilesysCompareFilesys(ffilesys_dev filesys, ULO index);
extern BOOLE ffilesysRemoveFilesys(ULO index);
extern void ffilesysClear(void);
extern void ffilesysSetAutomountDrives(BOOLE automount_drives);
extern BOOLE ffilesysGetAutomountDrives(void);


/* Starting and stopping the filesys */

extern void ffilesysHardReset(void);
extern void ffilesysEmulationStart(void);
extern void ffilesysEmulationStop(void);
extern void ffilesysStartup(void);
extern void ffilesysShutdown(void);

#endif
