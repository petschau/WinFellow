 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Autoconfig device support
  *
  * (c) 1996 Ed Hanway
  */

typedef uae_u32 (*TrapFunction) (void);

extern int lasttrap;
extern void do_emultrap (int nr);

extern uae_u32 addr (int);
extern void db (uae_u8);
extern void dw (uae_u16);
extern void dl (uae_u32);
extern uae_u32 ds (char *);
extern void calltrap (uae_u32);
extern void org (uae_u32);
extern uae_u32 here (void);
extern int deftrap2 (TrapFunction func, int mode, const char *str);
extern int deftrap (TrapFunction);
extern void align (int);
extern uae_u32 CallLib (uaecptr base, uae_s16 offset);
extern void call_calltrap (int nr) REGPARAM;

extern volatile int uae_int_requested;
extern void set_uae_int_flag (void);

#define RTS 0x4e75
#define RTE 0x4e73

extern uaecptr EXPANSION_explibname, EXPANSION_doslibname, EXPANSION_uaeversion;
extern uaecptr EXPANSION_explibbase, EXPANSION_uaedevname, EXPANSION_haveV36;
extern uaecptr EXPANSION_bootcode, EXPANSION_nullfunc;

extern uaecptr ROM_filesys_resname, ROM_filesys_resid;
extern uaecptr ROM_filesys_diagentry;
extern uaecptr ROM_hardfile_resname, ROM_hardfile_resid;
extern uaecptr ROM_hardfile_init;
extern uaecptr filesys_initcode;

struct uaedev_mount_info;
extern int nr_units (struct uaedev_mount_info *mountinfo);
extern int is_hardfile (struct uaedev_mount_info *mountinfo, int unit_no);
extern char *set_filesys_unit (struct uaedev_mount_info *mountinfo, int,
			       char *volname, char *rootdir, int readonly,
			       int secs, int surfaces, int reserved,
			       int blocksize, int removable);
extern char *add_filesys_unit (struct uaedev_mount_info *mountinfo,
			       char *volname, char *rootdir, int readonly,
			       int secs, int surfaces, int reserved,
			       int blocksize, int removable);
extern char *get_filesys_unit (struct uaedev_mount_info *mountinfo, int nr,
			       char **volname, char **rootdir, int *readonly,
			       int *secspertrack, int *surfaces, int *reserved,
			       int *cylinders, int *size, int *blocksize);
extern int kill_filesys_unit (struct uaedev_mount_info *mountinfo, int);
extern int move_filesys_unit (struct uaedev_mount_info *mountinfo, int nr, int to);
extern int sprintf_filesys_unit (struct uaedev_mount_info *mountinfo, char *buffer, int num);
extern void write_filesys_config (struct uaedev_mount_info *mountinfo, const char *unexpanded,
				  const char *defaultpath, FILE *f);

extern struct uaedev_mount_info *alloc_mountinfo (void);
extern struct uaedev_mount_info *dup_mountinfo (struct uaedev_mount_info *);
extern void free_mountinfo (struct uaedev_mount_info *);

extern void filesys_reset (void);
extern void filesys_prepare_reset (void);
extern void filesys_start_threads (void);

extern void reset_uaedevices (void);
extern int get_new_device (char **devname, uaecptr *devname_amiga);

extern void filesys_install (void);
extern void filesys_install_code (void);
extern void filesys_store_devinfo (uae_u8 *);
extern void hardfile_install (void);
extern void emulib_install (void);
extern void expansion_init (void);

#define TRAPFLAG_NO_REGSAVE 1
#define TRAPFLAG_NO_RETVAL 2
#define TRAPFLAG_EXTRA_STACK 4
#define TRAPFLAG_DORET 8

extern uaecptr libemu_InstallFunction (TrapFunction, uaecptr, int, const char *);
extern uaecptr libemu_InstallFunctionFlags (TrapFunction, uaecptr, int, int, const char *);

/* FELLOW IN: -------------------------------*/

/* Rename assembly functions */

#define rtarea_bgetASM ASMRENAME(rtarea_bgetASM)
#define rtarea_wgetASM ASMRENAME(rtarea_wgetASM)
#define rtarea_lgetASM ASMRENAME(rtarea_lgetASM)
#define rtarea_bputASM ASMRENAME(rtarea_bputASM)
#define rtarea_lputASM ASMRENAME(rtarea_lputASM)
#define filesys_bgetASM ASMRENAME(filesys_bgetASM)
#define filesys_wgetASM ASMRENAME(filesys_wgetASM)
#define filesys_lgetASM ASMRENAME(filesys_lgetASM)
#define filesys_bputASM ASMRENAME(filesys_bputASM)
#define filesys_wputASM ASMRENAME(filesys_wputASM)
#define filesys_lputASM ASMRENAME(filesys_lputASM)

/* Declare assembly functions */

extern ULO rtarea_bgetASM(ULO addr);
extern ULO rtarea_wgetASM(ULO addr);
extern ULO rtarea_lgetASM(ULO addr);
extern void rtarea_bputASM(ULO value, ULO addr);
extern void rtarea_lputASM(ULO value, ULO addr);
extern ULO filesys_bgetASM(ULO addr);
extern ULO filesys_wgetASM(ULO addr);
extern ULO filesys_lgetASM(ULO addr);
extern void filesys_bputASM(ULO value, ULO addr);
extern void filesys_wputASM(ULO value, ULO addr);
extern void filesys_lputASM(ULO value, ULO addr);



/* FELLOW OUT: ------------------------------*/

