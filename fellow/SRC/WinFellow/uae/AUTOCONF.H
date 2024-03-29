/*
 * UAE - The Un*x Amiga Emulator
 *
 * Autoconfig device support
 *
 * (c) 1996 Ed Hanway
 */

/* FELLOW IN (START)-----------------

  This file has been adapted for use in WinFellow.
  It originates from the UAE 0.8.22 source code distribution.

  Torsten Enderling (carfesh@gmx.net) 2004

   FELLOW IN (END)------------------- */

typedef uae_u32 (*TrapFunction)();

extern int lasttrap;
extern void do_emultrap(int nr);

extern uae_u32 addr(int);
extern void db(uae_u8);
extern void dw(uae_u16);
extern void dl(uae_u32);
extern uae_u32 ds(const char *);
extern void calltrap(uae_u32);
extern void org(uae_u32);
extern uae_u32 here();
extern int deftrap2(TrapFunction func, int mode, const char *str);
extern int deftrap(TrapFunction);
extern void align(int);
extern uae_u32 CallLib(uaecptr base, uae_s16 offset);
extern void call_calltrap(int nr) REGPARAM;

extern volatile int uae_int_requested;
extern void set_uae_int_flag();

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

extern int nr_units(struct uaedev_mount_info *mountinfo);
extern int is_hardfile(struct uaedev_mount_info *mountinfo, int unit_no);
extern const char *set_filesys_unit(struct uaedev_mount_info *mountinfo, int, char *volname, char *rootdir, int readonly, int secs, int surfaces, int reserved, int blocksize);
extern const char *add_filesys_unit(struct uaedev_mount_info *mountinfo, char *volname, char *rootdir, int readonly, int secs, int surfaces, int reserved, int blocksize);
extern const char *get_filesys_unit(
    struct uaedev_mount_info *mountinfo,
    int nr,
    char **volname,
    char **rootdir,
    int *readonly,
    int *secspertrack,
    int *surfaces,
    int *reserved,
    int *cylinders,
    int *size,
    int *blocksize);
extern int kill_filesys_unit(struct uaedev_mount_info *mountinfo, int);
extern int move_filesys_unit(struct uaedev_mount_info *mountinfo, int nr, int to);
extern int sprintf_filesys_unit(struct uaedev_mount_info *mountinfo, char *buffer, int num);
extern void write_filesys_config(struct uaedev_mount_info *mountinfo, const char *unexpanded, const char *defaultpath, FILE *f);

extern struct uaedev_mount_info *alloc_mountinfo();
extern struct uaedev_mount_info *dup_mountinfo(struct uaedev_mount_info *);
extern void free_mountinfo(struct uaedev_mount_info *);

extern void filesys_reset();
extern void filesys_prepare_reset();
extern void filesys_start_threads();

extern void filesys_install();
extern void filesys_install_code();
extern void filesys_store_devinfo(uae_u8 *);
extern void hardfile_install();
extern void emulib_install();
extern void expansion_init();
extern void expansion_cleanup();

#define TRAPFLAG_NO_REGSAVE 1
#define TRAPFLAG_NO_RETVAL 2
#define TRAPFLAG_EXTRA_STACK 4
#define TRAPFLAG_DORET 8

extern uaecptr libemu_InstallFunction(TrapFunction, uaecptr, int, const char *);
extern uaecptr libemu_InstallFunctionFlags(TrapFunction, uaecptr, int, int, const char *);

#define RTAREA_BASE 0xF00000

/* these appear to be missing and are here to suppress compiler warnings */

extern void rtarea_setup();
extern void rtarea_init();

/* FELLOW IN (END)------------------- */
