/* @(#) $Id: UAE2FELL.H,v 1.9 2004-05-27 19:56:56 carfesh Exp $ */
/*=========================================================================*/
/* Fellow Amiga Emulator                                                   */
/*                                                                         */
/* UAE to Fellow bridge header file                                        */
/*                                                                         */
/* In the case where similar functions exist, macros are defined which     */
/* expand to the Fellow equivalent.                                        */
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

#ifndef UAE2FELLH
#define UAE2FELLH

#include "defs.h"
#include "cpu.h"
#include "fhfile.h"
#include "fmem.h"
#include "fellow.h"


/*------------------------*/
/* Base types             */
/*------------------------*/

#define ULONG unsigned int
#define uae_u32 unsigned int
#define uae_s32 int
#define uae_u16 unsigned short
#define uae_s16 short
#define uae_u8 unsigned char
#define uae_s8 signed char
#define uaecptr unsigned int

#define __DOS__

/*------------------------*/
/* Misc types, funcs      */
/*------------------------*/

#define mode_t unsigned int
#define my_mkdir(P, M) (mkdir(P))
#define REGPARAM2
#define REGPARAM
#define uae_quit fellowNastyExit

#define version 043
#define UAEMAJOR  ((version / 100) % 10)
#define UAEMINOR  ((version / 10) % 10)
#define UAESUBREV ((version / 1) % 10)

/*================*/
/* UAE's Hardfile */
/*================*/

extern char uaehf0[256];
extern ULO hardfile_size;

/*------------------------*/
/* Access m68k registers  */
/*------------------------*/

#define m68k_dreg(REGS, NR) (da_regs[0][NR])
#define m68k_areg(REGS, NR) (da_regs[1][NR])
#define m68k_setpc(I) (cpuSetPC(I))
extern void m68k_do_rts(void);

/*------------------------*/
/* Memory interfacing     */
/*------------------------*/

#define get_long(ADR) (fetl(ADR))
#define get_word(ADR) (fetw(ADR))
#define get_byte(ADR) (char)(fetb(ADR))

#define put_long(ADR, DATA) (wril(DATA, ADR))
#define put_word(ADR, DATA) (wriw(DATA, ADR))
#define put_byte(ADR, DATA) (wrib(DATA, ADR))

#define expamem_write(OFFSET, DATA) (memoryEmemSet(OFFSET,DATA))
#define expamem memory_emem
#define expamem_init_clear memoryEmemClear
#define allocated_chipmem memory_chipsize
extern uae_u8 filesysory[65536];
extern void expamem_init_filesys (void);
extern void expamem_map_filesys (ULO mapping);

/*===========================================*/
/* These access memory with a direct pointer */
/*===========================================*/

extern ULO do_get_mem_byte(uae_u8 *address);
extern ULO do_get_mem_word(uae_u16 *address);
extern ULO do_get_mem_long(uae_u32 *address);
extern void do_put_mem_long(uae_u32 *address, uae_u32 data);
extern void do_put_mem_word(uae_u16 *address, uae_u16 data);
extern void do_put_mem_byte(uae_u8 *address, uae_u8 data);
extern int valid_address(uaecptr adr, uae_u32 size);
#define get_real_address(ADR) (memoryAddressToPtr(ADR))


/*========================*/
/* Logging                */
/*========================*/

extern char warning_buffer[256];


/*===================*/
/* UAE malloc define */
/*===================*/

#define xmalloc malloc
#define xcalloc calloc

#ifndef _FELLOW_DEBUG_CRT_MALLOC
#define my_strdup _strdup
#endif

/*===================*/
/* UAE random define */
/*===================*/

#define random rand

/*==========*/
/* Prefetch */
/*==========*/

#ifdef PREFETCH
#define fill_prefetch_0() cpuPrefetchFill(pc)
#else
#define fill_prefetch_0()
#endif


/*================================*/
/* Taken from sysdeps.h on WinUAE */
/*================================*/

#include <windows.h>
#define O_NDELAY 0

/*#define DONT_HAVE_POSIX*/ /* I want Mathias' posixemu_ functions! */

extern void write_log( const char *, ... );
extern void gui_message( const char *, ...);

#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <winbase.h>
#include <io.h>
#include <direct.h>

#define O_BINARY _O_BINARY
#define O_WRONLY _O_WRONLY
#define O_RDONLY _O_RDONLY
#define O_RDWR   _O_RDWR
#define O_CREAT  _O_CREAT
#define O_TRUNC  _O_TRUNC
#define strcasecmp _stricmp
#define REGPARAM
#define REGPARAM2
#define __inline__ __inline
#define __volatile__ volatile
#define __asm__(a) ;
#define off_t  int
/*#define utimbuf _utimbuf*/
#define fdopen _fdopen
#define DIR struct DIR
extern DIR* opendir(char *);
#define dirent direct
struct direct
{
    char d_name[1];
};
struct dirent *readdir (DIR * dir);
extern void closedir(DIR *);
#define W_OK 0x2
#define R_OK 0x4


#define FILEFLAG_DIR     0x1
#define FILEFLAG_ARCHIVE 0x2
#define FILEFLAG_WRITE   0x4
#define FILEFLAG_READ    0x8
#define FILEFLAG_EXECUTE 0x10
#define FILEFLAG_SCRIPT  0x20
#define FILEFLAG_PURE    0x40
#undef S_ISDIR
#undef S_IWUSR
#undef S_IRUSR
#undef S_IXUSR
#define S_ISDIR(a) (a&FILEFLAG_DIR)
#define S_ISARC(a) (a&FILEFLAG_ARCHIVE)
#define S_IWUSR FILEFLAG_WRITE
#define S_IRUSR FILEFLAG_READ
#define S_IXUSR FILEFLAG_EXECUTE

#undef L_tmpname
#define L_tmpname MAX_PATH /* For posixemu_tmpnam() */

#define HAVE_GETTICKCOUNT

#ifdef DONT_HAVE_POSIX
#define access posixemu_access
extern int posixemu_access (const char *, int);
#define open posixemu_open
#define creat(a,b) posixemu_open(a,O_RDWR|O_CREAT,0)
extern int posixemu_open (const char *, int, int);
#define close posixemu_close
extern void posixemu_close (int);
#define read posixemu_read
extern int posixemu_read (int, char *, int);
#define write posixemu_write
extern int posixemu_write (int, const char *, int);
#undef lseek
#define lseek posixemu_seek
extern int posixemu_seek (int, int, int);
#define stat(a,b) posixemu_stat ((a), (b))
extern int posixemu_stat (const char *, struct stat *);
#define mkdir posixemu_mkdir
extern int posixemu_mkdir (const char *, int);
#define rmdir posixemu_rmdir
extern int posixemu_rmdir (const char *);
#define unlink posixemu_unlink
extern int posixemu_unlink (const char *);
#define truncate posixemu_truncate
extern int posixemu_truncate (const char *, long int);
#define rename posixemu_rename
extern int posixemu_rename (const char *, const char *);
#define chmod posixemu_chmod
extern int posixemu_chmod (const char *, int);
#define tmpnam posixemu_tmpnam
extern void posixemu_tmpnam (char *);
#define utime posixemu_utime
struct utimbuf;
extern int posixemu_utime (const char *, struct utimbuf *);
#define opendir posixemu_opendir
extern DIR * posixemu_opendir (const char *);
#define readdir posixemu_readdir
extern struct dirent* readdir (DIR *);
#define closedir posixemu_closedir
extern void closedir (DIR *);

#if defined( SUPPORT_PENGUINS ) || defined( UAE_FILESYS_THREADS )
/* We need posixemu prototypes for the semaphore stuff */
typedef HANDLE  uae_sem_t;
typedef DWORD   penguin_id;
void sem_init( uae_sem_t*, int, int );
void sem_wait( uae_sem_t* );
void sem_post( uae_sem_t* );
int sem_trywait( uae_sem_t* );
int start_penguin( void *(*f)(void *), void *, penguin_id* );
#endif

/* This isn't the best place for this, but it fits reasonably well. The logic
 * is that you probably don't have POSIX errnos if you don't have the above
 * functions. */
extern long dos_errno (void);

#include "posixemu.h"

#endif

#endif










