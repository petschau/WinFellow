/* 
 * UAE - The Un*x Amiga Emulator
 *
 * Win32 interface
 *
 * Copyright 1997 Mathias Ortmann
 */

/* FELLOW OUT START -----------------------
#include "config.h"
#include "sysconfig.h"
#include "sysdeps.h"

#include <windows.h>
#include <stdlib.h>
#include <stdarg.h>
#ifdef _MSC_VER
#include <ddraw.h>
#include <commctrl.h>
#include <commdlg.h>
#else
#include "winstuff.h"
#endif
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>
#include <process.h>
#include "options.h"
#include "posixemu.h"
#include "filesys.h"
   FELLOW OUT END -----------------------*/

/* FELLOW IN START -----------------------*/

#include "uae2fell.h"
#include "filesys.h"
#include <process.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>
#include <time.h>
#include <sys/utime.h>

/* FELLOW IN END   -----------------------*/

/* convert time_t to/from AmigaDOS time */
#define secs_per_day ( 24 * 60 * 60 )
#define diff ( (8 * 365 + 2) * secs_per_day )

void get_time(time_t t, long* days, long* mins, long* ticks)
{
    /* time_t is secs since 1-1-1970 */
    /* days since 1-1-1978 */
    /* mins since midnight */
    /* ticks past minute @ 50Hz */

    t -= diff;
    *days = t / secs_per_day;
    t -= *days * secs_per_day;
    *mins = t / 60;
    t -= *mins * 60;
    *ticks = t * 50;
}

time_t put_time (long days, long mins, long ticks)
{
    time_t t;

    t = ticks / 50;
    t += mins * 60;
    t += days * secs_per_day;
    t += diff;

    return t;
}

/* stdioemu, posixemu, mallocemu, and various file system helper routines */
static DWORD lasterror;

static int isillegal (unsigned char *str)
{
    unsigned char a = *str, b = str[1], c = str[2];

    if (a >= 'a' && a <= 'z')
	a &= ~' ';
    if (b >= 'a' && b <= 'z')
	b &= ~' ';
    if (c >= 'a' && c <= 'z')
	c &= ~' ';

    return( (a == 'A' && b == 'U' && c == 'X') ||
	        (a == 'C' && b == 'O' && c == 'N') ||
	        (a == 'P' && b == 'R' && c == 'N') ||
	        (a == 'N' && b == 'U' && c == 'L') );
}

static int checkspace (char *str, char s, char d)
{
    char *ptr = str;

    while (*ptr && *ptr == s)
	ptr++;

    if (!*ptr || *ptr == '/' || *ptr == '\\') {
	while (str < ptr)
	    *(str++) = d;
	return 0;
    }
    return 1;
}

/* This is sick and incomplete... in the meantime, I have discovered six new illegal file name formats
 * M$ sucks! */
void fname_atow (const char *src, char *dst, int size)
{
    char *lastslash = dst, *strt = dst, *posn = NULL, *temp = NULL;
    int i, j;

    temp = xmalloc( size );

    while (size-- > 0) {
	if (!(*dst = *src++))
	    break;

	if (*dst == '~' || *dst == '|' || *dst == '*' || *dst == '?') {
	    if (size > 2) {
		sprintf (dst, "~%02x", *dst);
		size -= 2;
		dst += 2;
	    }
	} else if (*dst == '/') {
	    if (checkspace (lastslash, ' ', 0xa0) && (dst - lastslash == 3 || (dst - lastslash > 3 && lastslash[3] == '.')) && isillegal (lastslash)) {
		i = dst - lastslash - 3;
		dst++;
		for (j = i + 1; j--; dst--)
		    *dst = dst[-1];
		*(dst++) = 0xa0;
		dst += i;
		size--;
	    } else if (*lastslash == '.' && (dst - lastslash == 1 || (lastslash[1] == '.' && dst - lastslash == 2)) && size) {
		*(dst++) = 0xa0;
		size--;
	    }
	    *dst = '\\';
	    lastslash = dst + 1;
	}
	dst++;
    }

    if (checkspace (lastslash, ' ', 0xa0) && (dst - lastslash == 3 || (dst - lastslash > 3 && lastslash[3] == '.')) && isillegal (lastslash) && size > 1) {
	i = dst - lastslash - 3;
	dst++;
	for (j = i + 1; j--; dst--)
	    *dst = dst[-1];
	*(dst++) = 0xa0;
    } else if (!strcmp (lastslash, ".") || !strcmp (lastslash, ".."))
	strcat (lastslash, "\xa0");

    /* Major kludge, because I can't find the problem... */
    if( ( posn = strstr( strt, "..\xA0\\" ) ) == strt && temp)
    {
        strcpy( temp, "..\\" );
        strcat( temp, strt + 4 );
        strcpy( strt, temp );
    }

    /* Another major kludge, for the MUI installation... */
    if( *strt == ' ' ) /* first char as a space is illegal in Windoze */
    {
        sprintf( temp, "~%02x%s", ' ', strt+1 );
        strcpy( strt, temp );
    }
}

static int hextol (char a)
{
    if (a >= '0' && a <= '9')
	return a - '0';
    if (a >= 'a' && a <= 'f')
	return a - 'a' + 10;
    if (a >= 'A' && a <= 'F')
	return a - 'A' + 10;
    return 2;
}

/* Win32 file name restrictions suck... */
void fname_wtoa (unsigned char *ptr)
{
    unsigned char *lastslash = ptr;

    while (*ptr) {
	if (*ptr == '~') {
	    *ptr = hextol (ptr[1]) * 16 + hextol (ptr[2]);
	    strcpy (ptr + 1, ptr + 3);
	} else if (*ptr == '\\') {
	    if (checkspace (lastslash, ' ', 0xa0) && ptr - lastslash > 3 && lastslash[3] == 0xa0 && isillegal (lastslash)) {
		ptr--;
		strcpy (lastslash + 3, lastslash + 4);
	    }
	    *ptr = '/';
	    lastslash = ptr + 1;
	}
	ptr++;
    }

    if (checkspace (lastslash, ' ', 0xa0) && ptr - lastslash > 3 && lastslash[3] == 0xa0 && isillegal (lastslash))
	strcpy (lastslash + 3, lastslash + 4);
}

/* pthread Win32 emulation */
void sem_init (HANDLE * event, int manual_reset, int initial_state)
{
    *event = CreateEvent (NULL, manual_reset, initial_state, NULL);
}

void sem_wait (HANDLE * event)
{
    WaitForSingleObject (*event, INFINITE);
}

void sem_post (HANDLE * event)
{
    SetEvent (*event);
}

int sem_trywait (HANDLE * event)
{
    return WaitForSingleObject (*event, 0) == WAIT_OBJECT_0;
}

#ifdef __GNUC__
/* Mega-klduge to prevent problems with Watcom's habit of passing
 * arguments in registers... */
static HANDLE thread_sem;
static void *(*thread_startfunc) (void *);
#ifndef __GNUC__
static void * __stdcall thread_starter (void *arg)
#else
static DWORD thread_starter( void *arg )
#endif
{
    DWORD result;
    void *(*func) (void *) = thread_startfunc;
    SetEvent( thread_sem );
    result = (DWORD)(*func)(arg);
    return result;
}

/* this creates high priority threads by default to speed up the file system (kludge, will be
 * replaced by a set_penguin_priority() routine soon) */
int start_penguin (void *(*f) (void *), void *arg, DWORD * foo)
{
    int result = 0;
    static int have_event = 0;
    HANDLE hThread;

    if (! have_event) {
	thread_sem = CreateEvent (NULL, 0, 0, NULL);
	have_event = 1;
    }
    thread_startfunc = f;
    hThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) thread_starter, arg, 0, foo);
    if( hThread )
    {
        SetThreadPriority (hThread, THREAD_PRIORITY_HIGHEST);
        WaitForSingleObject (thread_sem, INFINITE);
        result = 1;
    }
    return result;
}
#else

typedef unsigned (__stdcall *BEGINTHREADEX_FUNCPTR)(void *);

int start_penguin (void *(*f)(void *), void *arg, DWORD * foo)
{
    HANDLE hThread;
    int result = 1;

    hThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE)f, arg, 0, foo);
//    hThread = _beginthreadex( NULL, 0, (BEGINTHREADEX_FUNCPTR)f, arg, 0, foo );
    if( hThread )
        SetThreadPriority (hThread, THREAD_PRIORITY_HIGHEST);
    else
        result = 0;
    return result;
}
#endif

#ifndef HAVE_TRUNCATE
int posixemu_truncate (const char *name, long int len)
{
    HANDLE hFile;
    BOOL bResult = FALSE;
    int result = -1;
	char buf[1024];

	fname_atow(name,buf,sizeof buf);

    if( ( hFile = CreateFile( buf, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL ) ) != INVALID_HANDLE_VALUE )
    {
        if( SetFilePointer( hFile, len, NULL, FILE_BEGIN ) == (DWORD)len )
        {
            if( SetEndOfFile( hFile ) == TRUE )
                result = 0;
        }
        else
        {
            write_log( "SetFilePointer() failure for %s to posn %d\n", buf, len );
        }
        CloseHandle( hFile );
    }
    else
    {
        write_log( "CreateFile() failed to open %s\n", buf );
    }
    if( result == -1 )
        lasterror = GetLastError();
    return result;
}
#endif

DIR {
    WIN32_FIND_DATA finddata;
    HANDLE hDir;
    int getnext;
};

DIR *opendir(const char *path)
{
	char buf[1024];
	DIR *dir;

	if (!(dir = (DIR *)GlobalAlloc(GPTR,sizeof(DIR))))
	{
		lasterror = GetLastError();
		return 0;
	}

	fname_atow(path,buf,sizeof buf-4);
	strcat(buf,"\\*");

	if ((dir->hDir = FindFirstFile(buf,&dir->finddata)) == INVALID_HANDLE_VALUE)
	{
		lasterror = GetLastError();
		GlobalFree(dir);
		return 0;
	}

	return dir;
}

struct dirent *readdir(DIR *dir)
{
	if (dir->getnext)
	{
		if (!FindNextFile(dir->hDir,&dir->finddata))
		{
			lasterror = GetLastError();
			return 0;
		}
	}
	dir->getnext = TRUE;

	fname_wtoa(dir->finddata.cFileName);
	return (struct dirent *)dir->finddata.cFileName;
}

void closedir(DIR *dir)
{
	FindClose(dir->hDir);
	GlobalFree(dir);
}

// Translate lasterror to valid AmigaDOS error code
// The mapping is probably not 100% correct yet
long dos_errno(void)
{
	int i;
	static DWORD errtbl[][2] = {
	{  ERROR_FILE_NOT_FOUND,         ERROR_OBJECT_NOT_AROUND    },  /* 2 */
    {  ERROR_PATH_NOT_FOUND,         ERROR_OBJECT_NOT_AROUND    },  /* 3 */
	{  ERROR_SHARING_VIOLATION,		 ERROR_OBJECT_IN_USE    },
    {  ERROR_ACCESS_DENIED,          ERROR_WRITE_PROTECTED    },  /* 5 */
    {  ERROR_ARENA_TRASHED,          ERROR_NO_FREE_STORE    },  /* 7 */
    {  ERROR_NOT_ENOUGH_MEMORY,      ERROR_NO_FREE_STORE    },  /* 8 */
    {  ERROR_INVALID_BLOCK,          ERROR_SEEK_ERROR    },  /* 9 */
    {  ERROR_INVALID_DRIVE,          ERROR_DIR_NOT_FOUND    },  /* 15 */
    {  ERROR_CURRENT_DIRECTORY,      ERROR_DISK_WRITE_PROTECTED    },  /* 16 */
    {  ERROR_NO_MORE_FILES,          ERROR_NO_MORE_ENTRIES    },  /* 18 */
    {  ERROR_SHARING_VIOLATION,      ERROR_OBJECT_IN_USE },            /* 32 */
    {  ERROR_LOCK_VIOLATION,         ERROR_DISK_WRITE_PROTECTED    },  /* 33 */
    {  ERROR_BAD_NETPATH,            ERROR_DIR_NOT_FOUND    },  /* 53 */
    {  ERROR_NETWORK_ACCESS_DENIED,  ERROR_DISK_WRITE_PROTECTED    },  /* 65 */
    {  ERROR_BAD_NET_NAME,           ERROR_DIR_NOT_FOUND    },  /* 67 */
    {  ERROR_FILE_EXISTS,            ERROR_OBJECT_EXISTS    },  /* 80 */
    {  ERROR_CANNOT_MAKE,            ERROR_DISK_WRITE_PROTECTED    },  /* 82 */
    {  ERROR_FAIL_I24,               ERROR_WRITE_PROTECTED    },  /* 83 */
    {  ERROR_DRIVE_LOCKED,           ERROR_OBJECT_IN_USE    },  /* 108 */
    {  ERROR_DISK_FULL,              ERROR_DISK_IS_FULL    },  /* 112 */
    {  ERROR_NEGATIVE_SEEK,          ERROR_SEEK_ERROR    },  /* 131 */
    {  ERROR_SEEK_ON_DEVICE,         ERROR_SEEK_ERROR    },  /* 132 */
    {  ERROR_DIR_NOT_EMPTY,          ERROR_DIRECTORY_NOT_EMPTY    },  /* 145 */
    {  ERROR_ALREADY_EXISTS,         ERROR_OBJECT_EXISTS    },  /* 183 */
    {  ERROR_FILENAME_EXCED_RANGE,   ERROR_OBJECT_NOT_AROUND    },  /* 206 */
    {  ERROR_NOT_ENOUGH_QUOTA,       ERROR_DISK_IS_FULL    },  /* 1816 */
	{  ERROR_DIRECTORY,              ERROR_OBJECT_WRONG_TYPE    } };

	for (i = sizeof(errtbl)/sizeof(errtbl[0]); i--; )
	{
		if (errtbl[i][0] == lasterror) return errtbl[i][1];
	}
	return 236;
}

int w32fopendel(char *name, char *mode, int delflag)
{
	HANDLE hFile;

	if ((hFile = CreateFile(name,
		mode[1] == '+' ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ,	// ouch :)
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		delflag ? FILE_ATTRIBUTE_NORMAL|FILE_FLAG_DELETE_ON_CLOSE : FILE_ATTRIBUTE_NORMAL,
		NULL)) == INVALID_HANDLE_VALUE)
	{
		lasterror = GetLastError();
		hFile = 0;
	}

	return (int)hFile;                      /* return handle */
}
	
DWORD getattr(char *name, LPFILETIME lpft, size_t *size)
{
	HANDLE hFind;
	WIN32_FIND_DATA fd;

	if ((hFind = FindFirstFile(name,&fd)) == INVALID_HANDLE_VALUE)
	{
		lasterror = GetLastError();

		fd.dwFileAttributes = GetFileAttributes(name);

		return fd.dwFileAttributes;
	}

	FindClose(hFind);

	if (lpft) *lpft = fd.ftLastWriteTime;
	if (size) *size = fd.nFileSizeLow;

	return fd.dwFileAttributes;
}

int posixemu_access(const char *name, int mode)
{
	DWORD attr;

	if ((attr = getattr((char *) name,NULL,NULL)) == (DWORD)~0) return -1;

	if (attr & FILE_ATTRIBUTE_READONLY && (mode & W_OK)) {
		lasterror = ERROR_ACCESS_DENIED;
		return -1;
	}
	else return 0;
}

int posixemu_open(const char *name, int oflag, int dummy)
{
	DWORD fileaccess;
	DWORD filecreate;
	char buf[1024];

	HANDLE hFile;

    switch (oflag & (O_RDONLY | O_WRONLY | O_RDWR))
	{
        case O_RDONLY:
            fileaccess = GENERIC_READ;
            break;
        case O_WRONLY:
            fileaccess = GENERIC_WRITE;
            break;
        case O_RDWR:
            fileaccess = GENERIC_READ | GENERIC_WRITE;
            break;
        default:
            return -1;
    }

    switch (oflag & (O_CREAT | O_TRUNC))
	{
        case O_CREAT:
            filecreate = OPEN_ALWAYS;
            break;
        case O_TRUNC:
            filecreate = TRUNCATE_EXISTING;
            break;
        case O_CREAT | O_TRUNC:
            filecreate = CREATE_ALWAYS;
            break;
        default:
            filecreate = OPEN_EXISTING;
            break;
    }

	fname_atow(name,buf,sizeof buf);

	if ((hFile = CreateFile(buf,
		fileaccess,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		filecreate,
		FILE_ATTRIBUTE_NORMAL,
		NULL)) == INVALID_HANDLE_VALUE) lasterror = GetLastError();

    return (int)hFile;
}

void posixemu_close(int hFile)
{
	CloseHandle((HANDLE)hFile);
}

int posixemu_read(int hFile, char *ptr, int size)
{
	DWORD actual;

	if (!ReadFile((HANDLE)hFile,ptr,size,&actual,NULL)) lasterror = GetLastError();

	return actual;
}

int posixemu_write(int hFile, const char *ptr, int size)
{
	DWORD actual;

	if (!WriteFile((HANDLE)hFile,ptr,size,&actual,NULL)) lasterror = GetLastError();

	return actual;
}

int posixemu_seek(int hFile, int pos, int type)
{
	int result;

	switch (type)
	{
		case SEEK_SET:
			type = FILE_BEGIN;
			break;
		case SEEK_CUR:
			type = FILE_CURRENT;
			break;
		case SEEK_END:
			type = FILE_END;
	}

	if ((result = SetFilePointer((HANDLE)hFile,pos,NULL,type)) == ~0) lasterror = GetLastError();

	return result;
}

int posixemu_stat(const char *name, struct stat *statbuf)
{
	char buf[1024];
	DWORD attr;
	FILETIME ft, lft;

	fname_atow(name,buf,sizeof buf);

	if ((attr = getattr(buf,&ft,(size_t*)&statbuf->st_size)) == (DWORD)~0)
	{
		lasterror = GetLastError();
		return -1;
	}
	else
	{
        statbuf->st_mode = (attr & FILE_ATTRIBUTE_READONLY) ? FILEFLAG_READ: FILEFLAG_READ | FILEFLAG_WRITE;
		if (attr & FILE_ATTRIBUTE_ARCHIVE) statbuf->st_mode |= FILEFLAG_ARCHIVE;
		if (attr & FILE_ATTRIBUTE_DIRECTORY) statbuf->st_mode |= FILEFLAG_DIR;
		FileTimeToLocalFileTime(&ft,&lft);
		statbuf->st_mtime = (*(__int64 *)&lft-((__int64)(369*365+89)*(__int64)(24*60*60)*(__int64)10000000))/(__int64)10000000;
	}

	return 0;
}

int posixemu_mkdir(const char *name, int mode)
{
	char buf[1024];

	fname_atow(name,buf,sizeof buf);

	if (CreateDirectory(buf,NULL)) return 0;

	lasterror = GetLastError();

	return -1;
}

int posixemu_unlink(const char *name)
{
	char buf[1024];

	fname_atow(name,buf,sizeof buf);
	if (DeleteFile(buf)) return 0;

	lasterror = GetLastError();

	return -1;
}

int posixemu_rmdir(const char *name)
{
	char buf[1024];

	fname_atow(name,buf,sizeof buf);
	if (RemoveDirectory(buf)) return 0;

	lasterror = GetLastError();

	return -1;
}

int posixemu_rename(const char *name1, const char *name2)
{
	char buf1[1024];
	char buf2[1024];

	fname_atow(name1,buf1,sizeof buf1);
	fname_atow(name2,buf2,sizeof buf2);
	if (MoveFile(buf1,buf2)) return 0;

	lasterror = GetLastError();

	return -1;
}

int posixemu_chmod(const char *name, int mode)
{
	char buf[1024];
	DWORD attr = FILE_ATTRIBUTE_NORMAL;

	fname_atow(name,buf,sizeof buf);

	if (mode & 1) attr |= FILE_ATTRIBUTE_READONLY;
	if (mode & 0x10) attr |= FILE_ATTRIBUTE_ARCHIVE;

	if (SetFileAttributes(buf,attr)) return 1;

	lasterror = GetLastError();

	return -1;
}

void posixemu_tmpnam(char *name)
{
	char buf[MAX_PATH];

	GetTempPath(MAX_PATH,buf);
	GetTempFileName(buf,"uae",0,name);
}

void tmToSystemTime( struct tm *tmtime, LPSYSTEMTIME systime )
{
    if( tmtime == NULL )
    {
        GetSystemTime( systime );
    }
    else
    {
        systime->wDay       = tmtime->tm_mday;
        systime->wDayOfWeek = tmtime->tm_wday;
        systime->wMonth     = tmtime->tm_mon + 1;
        systime->wYear      = tmtime->tm_year + 1900;
        systime->wHour      = tmtime->tm_hour;
        systime->wMinute    = tmtime->tm_min;
        systime->wSecond    = tmtime->tm_sec;
        systime->wMilliseconds = 0;
    }
}

static int setfiletime(const char *name, unsigned int days, int minute, int tick)
{
	FILETIME LocalFileTime, FileTime;
	HANDLE hFile;
	int success;
	char buf[1024];

	fname_atow(name,buf,sizeof buf);

	if ((hFile = CreateFile(buf, GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, NULL)) == INVALID_HANDLE_VALUE)
	{
		lasterror = GetLastError();
		return 0;
	}

	*(__int64 *)&LocalFileTime = (((__int64)(377*365+91+days)*(__int64)1440+(__int64)minute)*(__int64)(60*50)+(__int64)tick)*(__int64)200000;
	
	if (!LocalFileTimeToFileTime(&LocalFileTime,&FileTime)) FileTime = LocalFileTime;

	if (!(success = SetFileTime(hFile,&FileTime,&FileTime,&FileTime))) lasterror = GetLastError();
	CloseHandle(hFile);

	return success;
}

int posixemu_utime( const char *name, struct utimbuf *time )
{
    int result = -1;
    long days, mins, ticks;

    get_time( time->actime, &days, &mins, &ticks );

    if( setfiletime( name, days, mins, ticks ) )
        result = 0;

	return result;
}
