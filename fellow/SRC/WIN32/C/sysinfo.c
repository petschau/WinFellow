/* @(#) $Id: sysinfo.c,v 1.11.2.7 2005-01-01 00:28:11 worfje Exp $ */
/*=========================================================================*/
/* Fellow Amiga Emulator                                                   */
/*                                                                         */
/* System information retrieval                                            */
/*                                                                         */
/* Author: Torsten Enderling (carfesh@gmx.net)                             */
/*                                                                         */
/* Outputs valuable debugging information like hardware details and        */
/* OS specifics into the logfile                                           */
/*                                                                         */
/* Partially based on MSDN Library code examples; reference IDs are        */
/* Q117889 Q124207 Q124305                                                 */
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

#include <windows.h>
#include <excpt.h>
#include "defs.h"
#include "fellow.h"
#include "mmx.h"
#include "sse.h"

/* Older compilers do not support CPUID and RDTSC properly */
#define cpuid _asm _emit 0x0f _asm _emit 0xa2
#define rdtsc _asm _emit 0x0f _asm _emit 0x31

struct sysinfo_cpu_ident {
  char type;
  char model;
  char step;
  char fill;
  long thecpuid;
  long capability;
  char vend_id[12];
  unsigned char cache_info[16];
} sysinfo_cpu_id;

#define MYREGBUFFERSIZE 1024

/*=======================*/
/* handle error messages */
/*=======================*/
void sysinfoLogErrorMessageFromSystem (void) 
{
  CHAR szTemp[MYREGBUFFERSIZE * 2];
  DWORD cMsgLen;
  DWORD dwError;
  CHAR *msgBuf;

  dwError = GetLastError ();

  sprintf (szTemp, "Error %d: ", dwError);
  cMsgLen =
    FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER
		   | 40, NULL, dwError, MAKELANGID (0, SUBLANG_ENGLISH_US),
		   (LPTSTR) & msgBuf, MYREGBUFFERSIZE, NULL);
  if (cMsgLen) {
      strcat (szTemp, msgBuf);
      fellowAddTimelessLog ("%s\n", szTemp);
  }
}

/*===================================*/
/* Windows Registry access functions */
/*===================================*/

/*=============================================================================*/
/* Read a string value from the registry; if succesful a pointer to the string */
/* is returned, if failed it returns NULL                                      */
/*=============================================================================*/
static char *sysinfoRegistryQueryStringValue (HKEY RootKey, LPCTSTR SubKey, LPCTSTR ValueName) {
  HKEY hKey;
  TCHAR szBuffer[MYREGBUFFERSIZE];
  DWORD dwBufLen = MYREGBUFFERSIZE;
  DWORD dwType;
  LONG lRet;
  char *result;

  if (RegOpenKeyEx (RootKey, SubKey, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS) 
	return NULL;
  lRet =
    RegQueryValueEx (hKey, ValueName, NULL, &dwType, (LPBYTE) szBuffer,
		     &dwBufLen);
  RegCloseKey (hKey);

  if (lRet != ERROR_SUCCESS)
    return NULL;
  if (dwType != REG_SZ)
    return NULL;

  result = (char *) malloc (strlen (szBuffer) + 1);
  strcpy (result, szBuffer);

  return result;
}

/*=============================================================================*/
/* Read a DWORD value from the registry; if succesful a pointer to the DWORD   */
/* is returned, if failed it returns NULL                                      */
/*=============================================================================*/
static DWORD *sysinfoRegistryQueryDWORDValue (HKEY RootKey, LPCTSTR SubKey, LPCTSTR ValueName) {
  HKEY hKey;
  DWORD dwBuffer;
  DWORD dwBufLen = sizeof (dwBuffer);
  DWORD dwType;
  LONG lRet;
  DWORD *result;

  if (RegOpenKeyEx (RootKey, SubKey, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS) 
	return NULL;
  lRet =
    RegQueryValueEx (hKey, ValueName, NULL, &dwType, (LPBYTE) & dwBuffer,
		     &dwBufLen);
  RegCloseKey (hKey);

  if (lRet != ERROR_SUCCESS)
    return NULL;
  if (dwType != REG_DWORD)
    return NULL;

  result = (DWORD *) malloc (sizeof (dwBuffer));
  *result = dwBuffer;

  return result;
}

/*============================================================*/
/* walk through a given registry hardware enumeration tree    */
/* warning: this may cause serious damage to your health.. ;) */
/*============================================================*/
static void sysinfoEnumHardwareTree(LPCTSTR SubKey) {
  HKEY hKey, hKey2;
  DWORD dwNoSubkeys, dwNoSubkeys2;
  DWORD CurrentSubKey, CurrentSubKey2;
  TCHAR szSubKeyName[MYREGBUFFERSIZE], szSubKeyName2[MYREGBUFFERSIZE];
  DWORD dwSubKeyNameLen = MYREGBUFFERSIZE, dwSubKeyNameLen2 = MYREGBUFFERSIZE;
  char *szClass, *szDevice;

  /* get handle to specified key tree */
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SubKey, 0,
       KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS) {
      /* LogErrorMessageFromSystem (); */
      return;
  }

  /* retrieve information about that key (no. of subkeys) */
  if (RegQueryInfoKey (hKey, NULL, NULL, NULL, &dwNoSubkeys, NULL,
		       NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
      /* LogErrorMessageFromSystem (); */
      return;
  }

  for (CurrentSubKey = 0; CurrentSubKey < dwNoSubkeys; CurrentSubKey++) {
      dwSubKeyNameLen = MYREGBUFFERSIZE;
      if (RegEnumKeyEx (hKey,
			CurrentSubKey,
			szSubKeyName,
			&dwSubKeyNameLen,
			NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
	    /* sysinfoLogErrorMessageFromSystem (); */
	    continue;
	  }

      /* now query this subkey for the keys with the real information...
         I hate the registry. */
      if (RegOpenKeyEx
	  (hKey, szSubKeyName, 0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
	   &hKey2) != ERROR_SUCCESS)
	{
	  /* sysinfoLogErrorMessageFromSystem (); */
	  return;
	}

      if (RegQueryInfoKey (hKey2, NULL, NULL, NULL, &dwNoSubkeys2, NULL,
			   NULL, NULL, NULL, NULL, NULL,
			   NULL) != ERROR_SUCCESS)
	{
	  /* sysinfoLogErrorMessageFromSystem (); */
	  return;
	}

      for (CurrentSubKey2 = 0; CurrentSubKey2 < dwNoSubkeys2;
	   CurrentSubKey2++)
	{
	  dwSubKeyNameLen2 = MYREGBUFFERSIZE;
	  if (RegEnumKeyEx (hKey2,
			    CurrentSubKey2,
			    szSubKeyName2,
			    &dwSubKeyNameLen2,
			    NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
	    {
	      /* LogErrorMessageFromSystem (); */
	      continue;
	    }

	  /* now open the key and read the info */
	  szClass = sysinfoRegistryQueryStringValue (hKey2, szSubKeyName2, TEXT ("Class"));
	  if (szClass)
	    {
	      if ((stricmp (szClass, "display") == 0) ||
		      (stricmp (szClass, "media"  ) == 0) ||
			  (stricmp (szClass, "unknown") == 0))
		{
		  szDevice = sysinfoRegistryQueryStringValue (hKey2, szSubKeyName2, TEXT ("DeviceDesc"));
		  if (szDevice) {
		      fellowAddTimelessLog("\t%s: %s\n", strlwr(szClass), szDevice);
		      free (szDevice);
		  }
		}
	      free (szClass);
	    }
	}
      RegCloseKey (hKey2);
    }
  RegCloseKey (hKey);
}

static void sysinfoEnumRegistry (void) {
  OSVERSIONINFO osInfo;

  ZeroMemory(&osInfo, sizeof (OSVERSIONINFO));
  osInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
  if (!GetVersionEx (&osInfo))
    {
      sysinfoLogErrorMessageFromSystem ();
      return;
    }

  if (osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
      /* this seems to be the right place in Win2k and NT */
      sysinfoEnumHardwareTree(TEXT ("SYSTEM\\CurrentControlSet\\Enum\\PCI"));
      sysinfoEnumHardwareTree(TEXT ("SYSTEM\\CurrentControlSet\\Enum\\ISAPNP"));
	  sysinfoEnumHardwareTree(TEXT ("SYSTEM\\CurrentControlSet\\Enum\\Root"));
    }
  else
    {
      /* this one is for Win9x and ME */
      sysinfoEnumHardwareTree (TEXT ("Enum\\PCI"));
      sysinfoEnumHardwareTree (TEXT ("Enum\\ISAPNP"));
    }
}

/*================================*/
/* windows system info structures */
/*================================*/

static void sysinfoParseSystemInfo (void)
{
  SYSTEM_INFO SystemInfo;
  GetSystemInfo(&SystemInfo);
  fellowAddTimelessLog("\tnumber of processors: \t%d\n", SystemInfo.dwNumberOfProcessors);
  fellowAddTimelessLog("\tprocessor type: \t%d\n", SystemInfo.dwProcessorType);
  fellowAddTimelessLog("\tarchitecture: \t\t%d\n", SystemInfo.wProcessorArchitecture);
  // fellowAddTimelessLog("Processor level: %d\n", wProcessorLevel);
  // fellowAddTimelessLog("Processor revision: %d\n", wProcessorRevision; 
}

static void sysinfoParseOSVersionInfo(void) {
	OSVERSIONINFOEX osInfo;
	BOOL osVersionInfoEx;

	ZeroMemory(&osInfo, sizeof(OSVERSIONINFOEX));
	osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if(!(osVersionInfoEx = GetVersionEx((OSVERSIONINFO *) &osInfo))) {
      
		// OSVERSIONINFOEX didn't work, we try OSVERSIONINFO.
		osInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
		if (!GetVersionEx((OSVERSIONINFO *) &osInfo)) {
			sysinfoLogErrorMessageFromSystem();
			return;  
		}
	}

	switch (osInfo.dwPlatformId) {
	case VER_PLATFORM_WIN32s:
		fellowAddTimelessLog("\toperating system: \tWindows %d.%d\n", osInfo.dwMajorVersion, osInfo.dwMinorVersion);
		break;
	case VER_PLATFORM_WIN32_WINDOWS:

		if ((osInfo.dwMajorVersion == 4) && (osInfo.dwMinorVersion == 0)) {
			fellowAddTimelessLog("\toperating system: \tWindows 95 ");
			if (osInfo.szCSDVersion[1] == 'C' || osInfo.szCSDVersion[1] == 'B' ) {
				fellowAddTimelessLog("OSR2\n");
			} else {
				fellowAddTimelessLog("\n");
			}
		}

		if ((osInfo.dwMajorVersion == 4) && (osInfo.dwMinorVersion == 10)) {
			fellowAddTimelessLog("\toperating system: \tWindows 98 ");
			if ( osInfo.szCSDVersion[1] == 'A' ) {
                fellowAddTimelessLog("SE\n" );
			} else {
				fellowAddTimelessLog("\n");
			}
		} 

		if ((osInfo.dwMajorVersion == 4) && (osInfo.dwMinorVersion == 90)) {
			fellowAddTimelessLog("\toperating system: \tWindows ME\n");
		}
		break;
	case VER_PLATFORM_WIN32_NT: 
		switch (osInfo.dwMajorVersion) {
		case 0:
		case 1:
		case 2:
		case 3:
			fellowAddTimelessLog("\toperating system: \tWindows NT 3\n");
			break;
		case 4:
			fellowAddTimelessLog("\toperating system: \tWindows NT 4\n");
			break;
		case 5:
			switch (osInfo.dwMinorVersion) {
			case 0:
				fellowAddTimelessLog("\toperating system: \tWindows 2000\n");
				break;
			case 1:
				fellowAddTimelessLog("\toperating system: \tWindows XP\n");
				break;
			default:
				fellowAddTimelessLog("\toperating system: \tunknown platform Win32 NT\n");
			}
			break;
		default:
			fellowAddTimelessLog("\toperating system: \tunknown platform Win32 NT\n");
		}
		break;
	default:
		fellowAddTimelessLog("\toperating system: \tunknown\n");
	}

  fellowAddTimelessLog("\tparameters: \t\tOS %d.%d build %d, %s\n", osInfo.dwMajorVersion,
	  osInfo.dwMinorVersion, osInfo.dwBuildNumber,
	  (osInfo.szCSDVersion ? osInfo.szCSDVersion : "--"));
}

static void sysinfoParseRegistry(void) {
  char *tempstr = NULL;
  DWORD *dwTemp = NULL;

  tempstr = sysinfoRegistryQueryStringValue(HKEY_LOCAL_MACHINE, TEXT
     ("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
     TEXT ("VendorIdentifier"));
  if (tempstr) {
    fellowAddTimelessLog("\tCPU vendor: \t\t%s\n", tempstr);
    free(tempstr);
  }

  tempstr = sysinfoRegistryQueryStringValue(HKEY_LOCAL_MACHINE, TEXT
     ("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
     TEXT ("ProcessorNameString"));
  if (tempstr) {
    fellowAddTimelessLog("\tCPU type: \t\t%s\n", tempstr);
    free(tempstr);
  }

  tempstr = sysinfoRegistryQueryStringValue(HKEY_LOCAL_MACHINE, TEXT
     ("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
     TEXT ("Identifier")); 
  if (tempstr) {
      fellowAddTimelessLog("\tCPU identifier: \t%s\n", tempstr);
      free (tempstr);
  }

  /* clock speed seems to be only available on NT systems here */
  dwTemp = sysinfoRegistryQueryDWORDValue(HKEY_LOCAL_MACHINE, TEXT
     ("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"), TEXT ("~MHz"));
  if (dwTemp) {
      fellowAddTimelessLog("\tCPU clock: \t\t%d MHz\n", *dwTemp);
      free(dwTemp);
  }
}

static void sysinfoParseMemoryStatus (void) {
  MEMORYSTATUS MemoryStatus;

  ZeroMemory(&MemoryStatus, sizeof (MemoryStatus));
  MemoryStatus.dwLength = sizeof (MEMORYSTATUS);
  GlobalMemoryStatus(&MemoryStatus);

  fellowAddTimelessLog("\ttotal physical memory: \t%u MB\n", MemoryStatus.dwTotalPhys / 1024 / 1024);
  fellowAddTimelessLog("\tfree physical memory: \t%u MB\n", MemoryStatus.dwAvailPhys / 1024 / 1024);
  fellowAddTimelessLog("\tmemory in use: \t\t%u%%\n", MemoryStatus.dwMemoryLoad);
  fellowAddTimelessLog("\ttotal size of pagefile: \t%u MB\n", MemoryStatus.dwTotalPageFile / 1024 / 1024);
  fellowAddTimelessLog("\tfree size of pagefile: \t\t%u MB\n", MemoryStatus.dwAvailPageFile / 1024 / 1024);
  fellowAddTimelessLog("\ttotal virtual address space: \t%u MB\n", MemoryStatus.dwTotalVirtual / 1024 / 1024);
  fellowAddTimelessLog("\tfree virtual address space: \t%u MB\n", MemoryStatus.dwAvailVirtual / 1024 / 1024);
}

static void sysinfoVersionInfo (void) {
    char *versionstring = fellowGetVersionString();
	fellowAddTimelessLog(versionstring);
    free(versionstring);

	#ifdef USE_DX3
		fellowAddTimelessLog(" (DirectX 3");
	#else
		fellowAddTimelessLog(" (DirectX 5");
	#endif

	#ifdef _DEBUG
		fellowAddTimelessLog(" debug build)\n");
	#else
		fellowAddTimelessLog(" release build)\n");
	#endif
}

/*============================================================================*/
/* gather information about x86 CPUs                                          */
/*                                                                            */
/* based on MemTest86 by Chris Brady (cbrady@sgi.com)                         */
/* http://www.memtest86.com                                                   */
/*                                                                            */
/* lines marked with @@@@@ differ from the original source                    */
/* original code from head.s                                                  */
/*============================================================================*/
static void sysinfoCPUGetFeatures(void) 
{
  /* initialization */
  memset(&sysinfo_cpu_id, 0, sizeof(sysinfo_cpu_id));

  _asm
  {
    /* *INDENT-OFF* */
	/* Find out the CPU type */

	mov	sysinfo_cpu_id.thecpuid,	-1			;  -1 for no CPUID initially

	/* check if it is 486 or 386. */

	mov		sysinfo_cpu_id.type,	3			; at least 386
	pushfd								; push EFLAGS								@@@@@
	pop     eax			    			; get EFLAGS

	mov		ecx,			eax			; save original EFLAGS
	xor		eax,			0x40000		; flip AC bit in EFLAGS
	push	eax							; copy to EFLAGS
	popfd								; set EFLAGS								@@@@@
	pushfd								; get new EFLAGS							@@@@@
	pop		eax							; put it in eax
	xor		eax,			ecx			; change in flags
	and		eax,			0x40000		; check if AC bit changed
	je		id_done

	mov		sysinfo_cpu_id.type,	4			; at least 486
	mov		eax,			ecx
	xor		eax,			0x200000	; check ID flag
	push	eax
	popfd								; if we are on a straight 486DX, SX, or		@@@@@
	pushfd								; 487SX we can't change it					@@@@@
	pop		eax
	xor		eax,			ecx
	push	ecx							; restore original EFLAGS
	popfd								;											@@@@@
	and		eax,			0x200000
	jne		have_cpuid

	/* Test for Cyrix CPU types */

	xor		ax,				ax			; clear ax
	sahf								; clear flags
	mov		ax,				5
	mov		bx,				2
	div		bl							; do operation that does not change flags
	lahf								; get flags
	cmp		ah,				2			; check for change in flags
	jne		id_done						; if not Cyrix
	mov		sysinfo_cpu_id.type,	2			; Use two to identify as Cyrix
	jp		id_done

have_cpuid:

	/* get vendor info */

	xor		eax,			eax			; call CPUID with 0 -> return vendor ID
	cpuid
	mov		sysinfo_cpu_id.thecpuid,eax				; save CPUID level
	mov		dword ptr[sysinfo_cpu_id.vend_id],ebx	; first 4 chars
	mov		dword ptr[sysinfo_cpu_id.vend_id+4],edx	; next 4 chars
	mov		dword ptr[sysinfo_cpu_id.vend_id+8],ecx	; last 4 chars

	or		eax,			eax			; do we have processor info as well?
	je		id_done

	mov		eax,			1			; Use the CPUID instruction to get CPU type
	cpuid
	mov		cl,				al			; save reg for future use
	and		ah,				0x0f		; mask processor family
	mov		sysinfo_cpu_id.type,	ah
	and		al,				0xf0		; mask model
	shr		al,				4
	mov		sysinfo_cpu_id.model,	al
	and		cl,				0x0f		; mask mask revision
	mov		sysinfo_cpu_id.step,	cl
	mov		sysinfo_cpu_id.capability,edx

	mov		dword ptr[sysinfo_cpu_id.cache_info],0
	mov		dword ptr[sysinfo_cpu_id.cache_info+4],0
	mov		dword ptr[sysinfo_cpu_id.cache_info+8],0
	mov		dword ptr[sysinfo_cpu_id.cache_info+12],0

	mov		eax,			dword ptr[sysinfo_cpu_id.vend_id+8]
	cmp		eax,			0x6c65746e	; Is this an Intel CPU?
	jne		not_intel
	mov		eax,			2			; Use the CPUID instruction to get cache info 
	cpuid
	mov		dword ptr[sysinfo_cpu_id.cache_info],eax
	mov		dword ptr[sysinfo_cpu_id.cache_info+4],ebx
	mov		dword ptr[sysinfo_cpu_id.cache_info+8],ecx
	mov		dword ptr[sysinfo_cpu_id.cache_info+12],edx
	jp		id_done

not_intel:
	mov		eax,			dword ptr[sysinfo_cpu_id.vend_id+8]
	cmp		eax,			0x444d4163	; Is this an AMD CPU?
	jne		not_amd

	mov		eax,			0x80000005	; Use the CPUID instruction to get cache info 
	cpuid
	mov		dword ptr[sysinfo_cpu_id.cache_info],ecx
	mov		dword ptr[sysinfo_cpu_id.cache_info+4],edx
	mov		eax,			0x80000006	; Use the CPUID instruction to get cache info
	cpuid
	mov		dword ptr[sysinfo_cpu_id.cache_info+8],ecx

not_amd:
	mov		eax,			dword ptr[sysinfo_cpu_id.vend_id+8]
	cmp		eax,			0x64616374	; Is this a Cyrix CPU?
	jne		id_done

	mov		eax,			sysinfo_cpu_id.thecpuid	; get CPUID level
	cmp		eax,			2			; Is there cache information available ?
	jne		id_done

	mov		eax,			2			; Use the CPUID instruction to get cache info 
	cpuid
	mov		dword ptr[sysinfo_cpu_id.cache_info],edx

id_done:
	/* *INDENT-ON* */
  }
}

static float sysinfoCPUSpeed(void)
{
  unsigned int Cycle_Start, Cycle_End, ticks, cycles;
  unsigned int result0 = 0, result1 = 0, result2 = 0, SumResults;
  long tries = 0;
  LARGE_INTEGER Timer_Start, Timer_End, Counter_Frequency;
  unsigned int Priority_Process;
  long Priority_Thread;

  /* get a precise timer if available */
  if (!QueryPerformanceFrequency (&Counter_Frequency))
    return 0;

  /* store thread and process information */
  Priority_Process = GetPriorityClass (GetCurrentProcess ());
  Priority_Thread = GetThreadPriority (GetCurrentThread ());

  /* give this thread the highest possible priority to get usable results */
  SetPriorityClass (GetCurrentProcess (), REALTIME_PRIORITY_CLASS);
  SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_TIME_CRITICAL);

  /* compare elapsed time on timer with elapsed cycles on the TSR */
  do
    {
      /* run until the results are exact +/- 1 MHz or up to 10 times */
      tries++;
      result2 = result1;	/* shift results */
      result1 = result0;

      /* store starting counter */
      QueryPerformanceCounter (&Timer_Start);

      Timer_End.LowPart = Timer_Start.LowPart;	/* initial time */
      Timer_End.HighPart = Timer_Start.HighPart;

      /* loop until 50 ticks have passed */
      while ((unsigned int) Timer_End.LowPart -
	     (unsigned int) Timer_Start.LowPart < 50)
	QueryPerformanceCounter (&Timer_End);

      _asm
      {
		/* *INDENT-OFF* */
		rdtsc				
		mov		Cycle_Start,	eax
		/* *INDENT-ON* */
      }

      Timer_Start.LowPart = Timer_End.LowPart;	/* reset initial time */
      Timer_Start.HighPart = Timer_End.HighPart;

      /* loop for 1000 ticks */
      while ((unsigned int) Timer_End.LowPart -
	     (unsigned int) Timer_Start.LowPart < 1000)
	QueryPerformanceCounter (&Timer_End);

      _asm
      {
		/* *INDENT-OFF* */
		rdtsc
        mov		Cycle_End, eax
		/* *INDENT-ON* */
      }

      cycles = Cycle_End - Cycle_Start;
      ticks =
	(unsigned int) Timer_End.LowPart - (unsigned int) Timer_Start.LowPart;

      ticks = ticks * 100000;
      ticks = ticks / (Counter_Frequency.LowPart / 10);

      if (ticks % Counter_Frequency.LowPart > Counter_Frequency.LowPart / 2)
	ticks++;

      result0 = cycles / ticks;	/* cycles / ticks  = MHz */

      if (cycles % ticks > ticks / 2)
	result0++;

      SumResults = result0 + result1 + result2;
    }
  while ((tries < 10) && ((abs (3 * result0 - SumResults) > 3) ||
			  (abs (3 * result1 - SumResults) > 3)
			  || (abs (3 * result2 - SumResults) > 3)));

  if (SumResults / 3 != (SumResults + 1) / 3)
    SumResults++;

  // restore the thread priority
  SetPriorityClass (GetCurrentProcess (), Priority_Process);
  SetThreadPriority (GetCurrentThread (), Priority_Thread);

  return (((float) SumResults) / 3);
}

/*====================================================*/
/* parse the contents of the sysinfo_cpu_id structure */
/* original function is cpu_type from init.c          */
/*====================================================*/
static void sysinfoDetectCPU (void) {
  int i, off = 0;

  int l1_cache = 0, l2_cache = 0;
  float speed;
  char vendor[13];
  BOOL exception;

  sysinfoCPUGetFeatures();

  strncpy (vendor, sysinfo_cpu_id.vend_id, 12);
  fellowAddTimelessLog("\tCPU vendor: \t%s\n", vendor);
  fellowAddTimelessLog("\tCPU type: \t");

  /* If the CPUID instruction is not supported then this is */
  /* a 386, 486 or one of the early Cyrix CPU's */
  if (sysinfo_cpu_id.thecpuid < 1)
    {
      switch (sysinfo_cpu_id.type)
	{
	case 2:
	  /* This is a Cyrix CPU without CPUID */

	  /*
	     i = getCx86(0xfe);
	     i &= 0xf0;
	     i >>= 4;
	     switch(i) {
	     case 0:
	     case 1:
	     fellowAddTimelessLog("Cyrix Cx486");
	     break;
	     case 2:
	     fellowAddTimelessLog("Cyrix 5x86");
	     break;
	     case 3:
	     fellowAddTimelessLog("Cyrix 6x86");
	     break;
	     case 4:
	     fellowAddTimelessLog("Cyrix MediaGX");
	     break;
	     case 5:
	     fellowAddTimelessLog("Cyrix 6x86MX");
	     break;
	     case 6:
	     fellowAddTimelessLog("Cyrix MII");
	     break;
	     default:
	     fellowAddTimelessLog("Cyrix ???");
	     break;
	     }
	   */
	  fellowAddTimelessLog ("Cyrix - buhbuh...");
	  break;

	case 3:
	  fellowAddTimelessLog("386");
	  break;

	case 4:
	  fellowAddTimelessLog("486");
	  l1_cache = 8;
	  break;
	}
      return;
    }

  switch (sysinfo_cpu_id.vend_id[0])
    {
      /* AMD Processors */
    case 'A':
      switch (sysinfo_cpu_id.type)
	{
	case 4:
	  switch (sysinfo_cpu_id.model)
	    {
	    case 3:
	      fellowAddTimelessLog("AMD 486DX2");
	      break;
	    case 7:
	      fellowAddTimelessLog("AMD 486DX2-WB");
	      break;
	    case 8:
	      fellowAddTimelessLog("AMD 486DX4");
	      break;
	    case 9:
	      fellowAddTimelessLog("AMD 486DX4-WB");
	      break;
	    case 14:
	      fellowAddTimelessLog("AMD 5x86-WT");
	      break;
	    }
	  /* Since we can't get CPU speed or cache info return */
	  return;
	case 5:
	  switch (sysinfo_cpu_id.model)
	    {
	    case 0:
	    case 1:
	    case 2:
	    case 3:
	      fellowAddTimelessLog("AMD K5");
	      off = 6;
	      break;
	    case 6:
	    case 7:
	      fellowAddTimelessLog("AMD K6");
	      off = 6;
	      l1_cache = sysinfo_cpu_id.cache_info[3];
	      l1_cache += sysinfo_cpu_id.cache_info[7];
	      break;
	    case 8:
	      fellowAddTimelessLog("AMD K6-2");
	      off = 8;
	      l1_cache = sysinfo_cpu_id.cache_info[3];
	      l1_cache += sysinfo_cpu_id.cache_info[7];
	      break;
	    case 9:
	      fellowAddTimelessLog("AMD K6-III");
	      off = 10;
	      l1_cache = sysinfo_cpu_id.cache_info[3];
	      l1_cache += sysinfo_cpu_id.cache_info[7];
	      l2_cache = (sysinfo_cpu_id.cache_info[11] << 8);
	      l2_cache += sysinfo_cpu_id.cache_info[10];
	      break;
	    }
	  break;
	case 6:
	  switch (sysinfo_cpu_id.model)
	    {
	    case 1:
	    case 2:
	    case 4:
	      fellowAddTimelessLog("AMD Athlon");
	      off = 10;
	      break;
	    case 3:
	      fellowAddTimelessLog("AMD Duron");
	      off = 9;
	      break;
	    }
	  l1_cache = sysinfo_cpu_id.cache_info[3];
	  l1_cache += sysinfo_cpu_id.cache_info[7];
	  l2_cache = (sysinfo_cpu_id.cache_info[11] << 8);
	  l2_cache += sysinfo_cpu_id.cache_info[10];
	}
      break;

      /* Intel Processors */
    case 'G':
      if (sysinfo_cpu_id.type == 4)
	{
	  switch (sysinfo_cpu_id.model)
	    {
	    case 0:
	    case 1:
	      fellowAddTimelessLog("Intel 486DX");
	      off = 11;
	      break;
	    case 2:
	      fellowAddTimelessLog("Intel 486SX");
	      off = 11;
	      break;
	    case 3:
	      fellowAddTimelessLog("Intel 486DX2");
	      off = 12;
	      break;
	    case 4:
	      fellowAddTimelessLog("Intel 486SL");
	      off = 11;
	      break;
	    case 5:
	      fellowAddTimelessLog("Intel 486SX2");
	      off = 12;
	      break;
	    case 7:
	      fellowAddTimelessLog("Intel 486DX2-WB");
	      off = 15;
	      break;
	    case 8:
	      fellowAddTimelessLog("Intel 486DX4");
	      off = 12;
	      break;
	    case 9:
	      fellowAddTimelessLog("Intel 486DX4-WB");
	      off = 15;
	      break;
	    }
	  /* Since we can't get CPU speed or cache info return */
	  return;
	}

      /* Get the cache info */
      for (i = 0; i < 16; i++) {
	  switch (sysinfo_cpu_id.cache_info[i])
	    {
	    case 0x6:
	      l1_cache += 8;
	      break;
	    case 0x8:
	      l1_cache += 16;
	      break;
	    case 0xa:
	      l1_cache += 8;
	      break;
	    case 0xc:
	      l1_cache += 16;
	      break;
	    case 0x40:
	      l2_cache = 0;
	      break;
	    case 0x41:
	      l2_cache = 128;
	      break;
	    case 0x42:
	    case 0x82:
	      l2_cache = 256;
	      break;
	    case 0x43:
	      l2_cache = 512;
	      break;
	    case 0x44:
	    case 0x84:
	      l2_cache = 1024;
	      break;
	    case 0x45:
	    case 0x85:
	      l2_cache = 2048;
	      break;
	    }
	}

      switch (sysinfo_cpu_id.type)
	{
	case 5:
	  switch (sysinfo_cpu_id.model)
	    {
	    case 0:
	    case 1:
	    case 2:
	    case 3:
	    case 7:
	      fellowAddTimelessLog("Pentium");
	      if (l1_cache == 0)
		{
		  l1_cache = 8;
		}
	      off = 7;
	      break;
	    case 4:
	    case 8:
	      fellowAddTimelessLog("Pentium-MMX");
	      if (l1_cache == 0)
		{
		  l1_cache = 16;
		}
	      off = 11;
	      break;
	    }
	  break;
	case 6:
	  switch (sysinfo_cpu_id.model)
	    {
	    case 0:
	    case 1:
	      fellowAddTimelessLog("Pentium Pro");
	      off = 11;
	      break;
	    case 3:
	      fellowAddTimelessLog("Pentium II");
	      off = 10;
	      break;
	    case 5:
	      if (l2_cache == 0)
		{
		  fellowAddTimelessLog("Celeron");
		  off = 7;
		}
	      else
		{
		  fellowAddTimelessLog("Pentium II");
		  off = 10;
		}
	      break;
	    case 6:
	      if (l2_cache == 128)
		{
		  fellowAddTimelessLog("Celeron");
		  off = 7;
		}
	      else
		{
		  fellowAddTimelessLog("Pentium II");
		  off = 10;
		}
	      break;
	    case 7:
	    case 8:
	      fellowAddTimelessLog("Pentium III");
	      off = 11;
	      break;
	    }
	}
      break;

      /* Cyrix Processors with CPUID */
    case 'C':
      switch (sysinfo_cpu_id.model)
	{
	case 0:
	  fellowAddTimelessLog("Cyrix 6x86MX/MII");
	  off = 16;
	  break;
	case 4:
	  fellowAddTimelessLog("Cyrix GXm");
	  off = 9;
	  break;
	}
      return;
      break;

      /* Unknown processor */
    default:
      off = 3;
      /* Make a guess at the family */
      switch (sysinfo_cpu_id.type)
	{
	case 5:
	  fellowAddTimelessLog("586");
	  return;
	case 6:
	  fellowAddTimelessLog("686");
	  return;
	}
    }

  fellowAddTimelessLog("\n");
  fellowAddTimelessLog("\tlevel 1 cache: \t%d KB\n", l1_cache);
  fellowAddTimelessLog("\tlevel 2 cache: \t%d KB\n", l2_cache);

  __try
  {
    speed = sysinfoCPUSpeed();
	exception = FALSE;
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
	  exception = TRUE;
  }

  if (exception == FALSE) {
	fellowAddTimelessLog("\tCPU clock: \t%3.0f MHz\n", speed);
  }
  
  if (sysinfoDetectMMX() == 1) {
	fellowAddTimelessLog("\tCPU supports: \tMMX instruction set\n");
  }
  if (sysinfoDetectSSE() == 1) {
	fellowAddTimelessLog("\tCPU supports: \tSSE instruction set\n");
  }

  /*
  if (detect3DNow() == 1) {
	fellowAddTimelessLog("CPU supports 3DNow! instruction set\n");
  }

  if (detect3DNow2() == 1) {
	fellowAddTimele("CPU supports 3DNow2! instruction set\n");
  }
  */

}

/*===============*/
/* Do the thing. */
/*===============*/
void sysinfoLogSysInfo(void)
{
  sysinfoVersionInfo();
  fellowAddTimelessLog("\ninformation retrieved from registry:\n\n");
  sysinfoParseRegistry();
  fellowAddTimelessLog("\n");
  sysinfoParseOSVersionInfo();
  fellowAddTimelessLog("\n");
  sysinfoParseSystemInfo();
  fellowAddTimelessLog("\n");
  sysinfoParseMemoryStatus();
  fellowAddTimelessLog("\n");
  sysinfoEnumRegistry();
  fellowAddTimelessLog("\n\ninformation retrieved from hardware detection:\n\n");
  sysinfoDetectCPU();
  fellowAddTimelessLog("\n\ndebug information:\n\n");
}

BOOL sysinfoDetectMMX(void)
{	
	BOOL mmxFound;

	mmxFound = 0;
	_asm
	{
		pushad
		pushfd
		pop	eax
		mov     ebx, eax
		xor     eax, 00200000h
		push    eax
		popfd
		pushfd
		pop     eax
		cmp     eax, ebx
		jz      no_mmx
		mov     eax, 1
		cpuid
		test    edx, 0800000h
		jz      no_mmx
		popad
		mov     eax, 1
		jmp     outt
no_mmx:	
		popad
		xor     eax, eax
outt:		
		mov mmxFound, eax
	}
	return mmxFound;
}

BOOL sysinfoDetectSSE(void)
{
	BOOL sseFound;

	sseFound = 0;

	_asm 
	{
		pushad
		pushfd
		pop	eax
		mov     ebx, eax
		xor     eax, 00200000h
		push    eax
		popfd
		pushfd
		pop     eax
		cmp     eax, ebx
		jz      no_sse
		mov     eax, 1
		cpuid
		test    edx, 02000000h
		jz      no_sse
		popad
		mov     eax, 1
		jmp     outt
no_sse:	
		popad
		xor     eax, eax
outt:		
		mov sseFound, eax;
	}
	return sseFound;
}

