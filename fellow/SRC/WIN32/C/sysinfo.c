/*============================================================================*/
/* Fellow Amiga Emulator - system information retrieval                       */
/*                                                                            */
/* Author: Torsten Enderling (carfesh@gmx.net)                                */
/*                                                                            */
/* Outputs valuable debugging information like hardware details and           */
/* OS specifics into the logfile                                              */
/*                                                                            */
/* Partially based on MSDN Library code examples; reference IDs are           */
/* Q117889 Q124207 Q124305                                                    */
/*                                                                            */
/* This file is under the GNU General Public License (GPL)                    */
/*============================================================================*/
/*============================================================================*/
/* Changelog:                                                                 */
/* ----------                                                                 */
/* 2000/10/24: included draft cpu detection code on assembly level; basically */
/*             this is a port of the MemTest86 routines to VisualC            */
/* 2000/10/21: reads more stuff from the registry, even more experimental     */
/*             regarding to where to find stuff in different windows versions */
/*             and how to enumerate keys in two levels...                     */
/* 2000/10/19: first release, highly experimental                             */
/*============================================================================*/

#include <windows.h>
#include <excpt.h>
#include "defs.h"
#include "fellow.h"

/* Older compilers do not support CPUID and RDTSC properly */
#define cpuid _asm _emit 0x0f _asm _emit 0xa2
#define rdtsc _asm _emit 0Fh  _asm _emit 031h 

struct cpu_ident {
	char type;
	char model;
	char step;
	char fill;
	long thecpuid;
	long capability;
	char vend_id[12];
	unsigned char cache_info[16];
} cpu_id;

#define MYREGBUFFERSIZE 1024

/*=======================*/
/* handle error messages */
/*=======================*/
void
LogErrorMessageFromSystem (void)
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
  if (cMsgLen)
    {
      strcat (szTemp, msgBuf);
      fellowAddLog ("%s\n", szTemp);
    }
}

/*===================================*/
/* Windows Registry access functions */
/*===================================*/

/*=============================================================================*/
/* Read a string value from the registry; if succesful a pointer to the string */
/* is returned, if failed it returns NULL                                      */
/*=============================================================================*/
static char *
RegistryQueryStringValue (HKEY RootKey, LPCTSTR SubKey, LPCTSTR ValueName)
{
  HKEY hKey;
  TCHAR szBuffer[MYREGBUFFERSIZE];
  DWORD dwBufLen = MYREGBUFFERSIZE;
  DWORD dwType;
  LONG lRet;
  char *result;

  if (RegOpenKeyEx (RootKey, SubKey, 0, KEY_QUERY_VALUE, &hKey) !=
      ERROR_SUCCESS) return NULL;
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
static DWORD *
RegistryQueryDWORDValue (HKEY RootKey, LPCTSTR SubKey, LPCTSTR ValueName)
{
  HKEY hKey;
  DWORD dwBuffer;
  DWORD dwBufLen = sizeof (dwBuffer);
  DWORD dwType;
  LONG lRet;
  DWORD *result;

  if (RegOpenKeyEx (RootKey, SubKey, 0, KEY_QUERY_VALUE, &hKey) !=
      ERROR_SUCCESS) return NULL;
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
static void
EnumHardwareTree (LPCTSTR SubKey)
{
  HKEY hKey, hKey2;
  DWORD dwNoSubkeys, dwNoSubkeys2;
  DWORD CurrentSubKey, CurrentSubKey2;
  TCHAR szSubKeyName[MYREGBUFFERSIZE], szSubKeyName2[MYREGBUFFERSIZE];
  DWORD dwSubKeyNameLen = MYREGBUFFERSIZE, dwSubKeyNameLen2 = MYREGBUFFERSIZE;
  char *szClass, *szDevice, *szDriver;

  /* get handle to specified key tree */
  if (RegOpenKeyEx
      (HKEY_LOCAL_MACHINE, SubKey, 0,
       KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
    {
      LogErrorMessageFromSystem ();
      return;
    }

  /* retrieve information about that key (no. of subkeys) */
  if (RegQueryInfoKey (hKey, NULL, NULL, NULL, &dwNoSubkeys, NULL,
		       NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
    {
      LogErrorMessageFromSystem ();
      return;
    }

  for (CurrentSubKey = 0; CurrentSubKey < dwNoSubkeys; CurrentSubKey++)
    {
      dwSubKeyNameLen = MYREGBUFFERSIZE;
      if (RegEnumKeyEx (hKey,
			CurrentSubKey,
			szSubKeyName,
			&dwSubKeyNameLen,
			NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
	{
	  LogErrorMessageFromSystem ();
	  continue;
	}

      /* now query this subkey for the keys with the real information...
         I hate the registry. */
      if (RegOpenKeyEx
	  (hKey, szSubKeyName, 0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
	   &hKey2) != ERROR_SUCCESS)
	{
	  LogErrorMessageFromSystem ();
	  return;
	}

      if (RegQueryInfoKey (hKey2, NULL, NULL, NULL, &dwNoSubkeys2, NULL,
			   NULL, NULL, NULL, NULL, NULL,
			   NULL) != ERROR_SUCCESS)
	{
	  LogErrorMessageFromSystem ();
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
	      LogErrorMessageFromSystem ();
	      continue;
	    }

	  /* now open the key and read the info */
	  szClass =
	    RegistryQueryStringValue (hKey2, szSubKeyName2, TEXT ("Class"));
	  if (szClass)
	    {
	      if ((stricmp (szClass, "display") == 0) ||
		  (stricmp (szClass, "media") == 0))
		{
		  szDevice = RegistryQueryStringValue (hKey2,
						       szSubKeyName2,
						       TEXT ("DeviceDesc"));
		  if (szDevice)
		    {
		      fellowAddLog ("%s: %s\n", szClass, szDevice);
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

static void
EnumRegistry (void)
{
  OSVERSIONINFO osInfo;
  ZeroMemory (&osInfo, sizeof (OSVERSIONINFO));
  osInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
  if (!GetVersionEx (&osInfo))
    {
      LogErrorMessageFromSystem ();
      return;
    }

  if (osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
      /* this seems to be the right place in Win2k and NT */
      EnumHardwareTree (TEXT ("SYSTEM\\CurrentControlSet\\Enum\\PCI"));
      EnumHardwareTree (TEXT ("SYSTEM\\CurrentControlSet\\Enum\\ISAPNP"));
    }
  else
    {
      /* this one is for Win9x and ME */
      EnumHardwareTree (TEXT ("Enum\\PCI"));
      EnumHardwareTree (TEXT ("Enum\\ISAPNP"));
    }
}

/*================================*/
/* windows system info structures */
/*================================*/

static void
ParseSystemInfo (void)
{
  SYSTEM_INFO SystemInfo;
  GetSystemInfo (&SystemInfo);
  fellowAddLog
    ("Number of processors: %d\n", SystemInfo.dwNumberOfProcessors);
  fellowAddLog ("Processor Type: %d\n", SystemInfo.dwProcessorType);
  fellowAddLog ("Architecture: %d\n", SystemInfo.wProcessorArchitecture);
  /*fellowAddLog("Processor level: %d\n", wProcessorLevel);
     fellowAddLog("Processor revision: %d\n", wProcessorRevision; */
}

static void
ParseOSVersionInfo (void)
{
  OSVERSIONINFO osInfo;
  ZeroMemory (&osInfo, sizeof (OSVERSIONINFO));
  osInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
  if (!GetVersionEx (&osInfo))
    {
      LogErrorMessageFromSystem ();
      return;
    }

  if (osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
      char *tempstr = NULL;
      fellowAddLog ("We seem to be running on NT.");
      tempstr =
	RegistryQueryStringValue
	(HKEY_LOCAL_MACHINE,
	 TEXT
	 ("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"),
	 TEXT ("ProductType")); if (tempstr)
	{
	  fellowAddLog (" It identifies itself as %s.\n", tempstr);
	  free (tempstr);
	}
      else
	fellowAddLog ("\n");
    }
  if (osInfo.dwMajorVersion == 5)
    fellowAddLog ("This should be Windows 2000\n");

  fellowAddLog
    ("OS %d.%d build %d desc %s, platform %d\n",
     osInfo.dwMajorVersion,
     osInfo.dwMinorVersion,
     osInfo.dwBuildNumber,
     (osInfo.szCSDVersion ? osInfo.szCSDVersion : "--"), osInfo.dwPlatformId);}

static void
ParseRegistry (void)
{
  char *tempstr = NULL;
  DWORD *dwTemp = NULL;
  fellowAddLog ("Querying registry for processor information:\n");
  tempstr =
    RegistryQueryStringValue
    (HKEY_LOCAL_MACHINE,
     TEXT
     ("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
     TEXT ("VendorIdentifier"));
  if (tempstr)
    {
      fellowAddLog ("CPU Vendor: %s\n", tempstr);
      free (tempstr);
    }

  tempstr =
    RegistryQueryStringValue
    (HKEY_LOCAL_MACHINE,
     TEXT
     ("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
     TEXT ("Identifier")); if (tempstr)
    {
      fellowAddLog ("CPU Identifier: %s\n", tempstr);
      free (tempstr);
    }

  /* clock speed seems to be only available on NT systems here */
  dwTemp =
    RegistryQueryDWORDValue
    (HKEY_LOCAL_MACHINE,
     TEXT
     ("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"), TEXT ("~MHz"));
  if (dwTemp)
    {
      fellowAddLog ("CPU clock: %d MHz\n", *dwTemp);
      free (dwTemp);
    }
}

static void
ParseMemoryStatus (void)
{
  MEMORYSTATUS MemoryStatus;
  ZeroMemory (&MemoryStatus, sizeof (MemoryStatus));
  MemoryStatus.dwLength = sizeof (MEMORYSTATUS);
  GlobalMemoryStatus (&MemoryStatus);
  fellowAddLog ("Memory information:\n");
  fellowAddLog
    ("Total physical memory: %u MB (%u Bytes)\n",
     MemoryStatus.dwTotalPhys / 1024 / 1024, MemoryStatus.dwTotalPhys);
  fellowAddLog
    ("Free physical memory: %u MB (%u Bytes)\n",
     MemoryStatus.dwAvailPhys / 1024 / 1024, MemoryStatus.dwAvailPhys);
  fellowAddLog ("Memory load (percent): %u\n", MemoryStatus.dwMemoryLoad);
  fellowAddLog
    ("Total size of pagefile: %u MB (%u Bytes)\n",
     MemoryStatus.dwTotalPageFile / 1024 /
     1024, MemoryStatus.dwTotalPageFile);
  fellowAddLog
    ("Free size of pagefile: %u MB (%u Bytes)\n",
     MemoryStatus.dwAvailPageFile / 1024 /
     1024, MemoryStatus.dwAvailPageFile);
  fellowAddLog
    ("Total virtual address space: %u MB (%u Bytes)\n",
     MemoryStatus.dwTotalVirtual / 1024 / 1024, MemoryStatus.dwTotalVirtual);
  fellowAddLog
    ("Free virtual address space: %u MB (%u Bytes)\n",
     MemoryStatus.dwAvailVirtual / 1024 / 1024, MemoryStatus.dwAvailVirtual);
}

static void
fellowVersionInfo (void)
{
  fellowAddLog ("WinFellow alpha v0.4.2 build 1");
#ifdef USE_DX3
  fellowAddLog (" (DX3");
#else
  fellowAddLog (" (DX5");
#endif
#ifdef _DEBUG
  fellowAddLog (" Debug Build)");
#else
  fellowAddLog (" Release Build)");
#endif
  fellowAddLog (" now starting up...\n");
}

/*============================================================================*/
/* gather information about x86 CPUs                                          */
/*                                                                            */
/* based on MemTest86 by Chris Brady (cbrady@sgi.com)                         */
/* http://reality.sgi.com/cbrady/memtest86                                    */
/*                                                                            */
/* lines marked with @@@@@ differ from the original source                    */
/*============================================================================*/
static void cpu_get_features(void)
{
  /* initialization */
  memset(&cpu_id, 0, sizeof(cpu_id));
  
  _asm
  {
	/* Find out the CPU type */

	mov	cpu_id.thecpuid,	-1			;  -1 for no CPUID initially

	/* check if it is 486 or 386. */

	mov		cpu_id.type,	3			; at least 386
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

	mov		cpu_id.type,	4			; at least 486
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
	mov		cpu_id.type,	2			; Use two to identify as Cyrix
	jp		id_done

have_cpuid:

	/* get vendor info */

	xor		eax,			eax			; call CPUID with 0 -> return vendor ID
	cpuid
	mov		cpu_id.thecpuid,eax				; save CPUID level
	mov		dword ptr[cpu_id.vend_id],ebx	; first 4 chars
	mov		dword ptr[cpu_id.vend_id+4],edx	; next 4 chars
	mov		dword ptr[cpu_id.vend_id+8],ecx	; last 4 chars

	or		eax,			eax			; do we have processor info as well?
	je		id_done

	mov		eax,			1			; Use the CPUID instruction to get CPU type
	cpuid
	mov		cl,				al			; save reg for future use
	and		ah,				0x0f		; mask processor family
	mov		cpu_id.type,	ah
	and		al,				0xf0		; mask model
	shr		al,				4
	mov		cpu_id.model,	al
	and		cl,				0x0f		; mask mask revision
	mov		cpu_id.step,	cl
	mov		cpu_id.capability,edx

	mov		dword ptr[cpu_id.cache_info],0
	mov		dword ptr[cpu_id.cache_info+4],0
	mov		dword ptr[cpu_id.cache_info+8],0
	mov		dword ptr[cpu_id.cache_info+12],0

	mov		eax,			dword ptr[cpu_id.vend_id+8]
	cmp		eax,			0x6c65746e	; Is this an Intel CPU?
	jne		not_intel
	mov		eax,			2			; Use the CPUID instruction to get cache info 
	cpuid
	mov		dword ptr[cpu_id.cache_info],eax
	mov		dword ptr[cpu_id.cache_info+4],ebx
	mov		dword ptr[cpu_id.cache_info+8],ecx
	mov		dword ptr[cpu_id.cache_info+12],edx
	jp		id_done

not_intel:
	mov		eax,			dword ptr[cpu_id.vend_id+8]
	cmp		eax,			0x444d4163	; Is this an AMD CPU?
	jne		not_amd

	mov		eax,			0x80000005	; Use the CPUID instruction to get cache info 
	cpuid
	mov		dword ptr[cpu_id.cache_info],ecx
	mov		dword ptr[cpu_id.cache_info+4],edx
	mov		eax,			0x80000006	; Use the CPUID instruction to get cache info
	cpuid
	mov		dword ptr[cpu_id.cache_info+8],ecx

not_amd:
	mov		eax,			dword ptr[cpu_id.vend_id+8]
	cmp		eax,			0x64616374	; Is this a Cyrix CPU?
	jne		id_done

	mov		eax,			cpu_id.thecpuid	; get CPUID level
	cmp		eax,			2			; Is there cache information available ?
	jne		id_done

	mov		eax,			2			; Use the CPUID instruction to get cache info 
	cpuid
	mov		dword ptr[cpu_id.cache_info],edx

id_done:
  }
}

static float cpu_speed(void)
{
  unsigned int  Cycle_Start, Cycle_End, ticks, cycles;  
  unsigned int  result0 = 0, result1 = 0, result2 = 0, SumResults;          
  long          tries=0;        
  LARGE_INTEGER Timer_Start, Timer_End, Counter_Frequency;  
  unsigned int  Priority_Process;
  long          Priority_Thread;

  /* get a precise timer if available */
  if (!QueryPerformanceFrequency(&Counter_Frequency)) return 0;

  /* store thread and process information */
  Priority_Process = GetPriorityClass(GetCurrentProcess());
  Priority_Thread  = GetThreadPriority(GetCurrentThread());

  /* give this thread the highest possible priority to get usable results */
  SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

  /* compare elapsed time on timer with elapsed cycles on the TSR */
  do 
  {
	/* run until the results are exact +/- 1 MHz or up to 10 times */
	tries++;
	result2 = result1; /* shift results */
	result1 = result0;

    /* store starting counter */
    QueryPerformanceCounter(&Timer_Start); 
		
    Timer_End.LowPart = Timer_Start.LowPart;		/* initial time */
	Timer_End.HighPart = Timer_Start.HighPart;

	/* loop until 50 ticks have passed */
    while ((unsigned int)Timer_End.LowPart - (unsigned int)Timer_Start.LowPart < 50) 
	  QueryPerformanceCounter(&Timer_End);
		
   	_asm
    {
		rdtsc				
		mov		Cycle_Start,	eax
   	}

	Timer_Start.LowPart = Timer_End.LowPart;		/* reset initial time */
	Timer_Start.HighPart = Timer_End.HighPart;

    /* loop for 1000 ticks */
    while ((unsigned int)Timer_End.LowPart - (unsigned int)Timer_Start.LowPart < 1000 ) 	
      QueryPerformanceCounter(&Timer_End);
			
	_asm
	{
		rdtsc
        mov		Cycle_End, eax
    }

    cycles = Cycle_End - Cycle_Start;
    ticks = (unsigned int)Timer_End.LowPart - (unsigned int)Timer_Start.LowPart;	

	ticks = ticks * 100000;				
	ticks = ticks / ( Counter_Frequency.LowPart/10 );		
		
	if (ticks%Counter_Frequency.LowPart > Counter_Frequency.LowPart/2)		
	  ticks++;
		
	result0 = cycles/ticks;	/* cycles / ticks  = MHz */
        									
    if (cycles%ticks > ticks/2)
      result0++;
          	
	SumResults = result0 + result1 + result2;	
  } while((tries < 10) && ((abs(3*result0-SumResults) > 3) ||
		  (abs(3*result1-SumResults) > 3) || (abs(3*result2-SumResults) > 3)));
		
  if (SumResults/3 != (SumResults+1)/3)
    SumResults ++;

  // restore the thread priority
  SetPriorityClass(GetCurrentProcess(), Priority_Process);
  SetThreadPriority(GetCurrentThread(), Priority_Thread);

  return(((float)SumResults) / 3);
}

/*============================================*/
/* parse the contents of the cpu_id structure */
/*============================================*/
static void DetectCPU (void)
{
  int i, off=0;

  int l1_cache=0, l2_cache=0;
  float speed;
  char vendor[13];

  cpu_get_features();
 
  fellowAddLog("Processor detection attempt: ");
  strncpy(vendor, cpu_id.vend_id, 12);
  fellowAddLog("%s ", vendor);

  /* If the CPUID instruction is not supported then this is */
  /* a 386, 486 or one of the early Cyrix CPU's */
  if (cpu_id.thecpuid < 1) {
    switch (cpu_id.type) {
	  case 2:
        /* This is a Cyrix CPU without CPUID */

		  /*
			i = getCx86(0xfe);
			i &= 0xf0;
			i >>= 4;
			switch(i) {
			case 0:
			case 1:
				fellowAddLog("Cyrix Cx486");
				break;
			case 2:
				fellowAddLog("Cyrix 5x86");
				break;
			case 3:
				fellowAddLog("Cyrix 6x86");
				break;
			case 4:
				fellowAddLog("Cyrix MediaGX");
				break;
			case 5:
				fellowAddLog("Cyrix 6x86MX");
				break;
			case 6:
				fellowAddLog("Cyrix MII");
				break;
			default:
				fellowAddLog("Cyrix ???");
				break;
			}
			*/
			fellowAddLog("Cytrix - buhbuh...");
			break;

	case 3:
		fellowAddLog("386");
		break;

	case 4:
		fellowAddLog("486");
		l1_cache = 8;
		break;
		}
		return;
	}

	switch(cpu_id.vend_id[0]) {
	/* AMD Processors */
	case 'A':
		switch(cpu_id.type) {
		case 4:
			switch(cpu_id.model) {
			case 3:
				fellowAddLog("AMD 486DX2");
				break;
			case 7:
				fellowAddLog("AMD 486DX2-WB");
				break;
			case 8:
				fellowAddLog("AMD 486DX4");
				break;
			case 9:
				fellowAddLog("AMD 486DX4-WB");
				break;
			case 14:
				fellowAddLog("AMD 5x86-WT");
				break;
			}
			/* Since we can't get CPU speed or cache info return */
			return;
		case 5:
			switch(cpu_id.model) {
			case 0:
			case 1:
			case 2:
			case 3:
				fellowAddLog("AMD K5");
				off = 6;
				break;
			case 6:
			case 7:
				fellowAddLog("AMD K6");
				off = 6;
				l1_cache = cpu_id.cache_info[3];
				l1_cache += cpu_id.cache_info[7];
				break;
			case 8:
				fellowAddLog("AMD K6-2");
				off = 8;
				l1_cache = cpu_id.cache_info[3];
				l1_cache += cpu_id.cache_info[7];
				break;
			case 9:
				fellowAddLog("AMD K6-III");
				off = 10;
				l1_cache = cpu_id.cache_info[3];
				l1_cache += cpu_id.cache_info[7];
				l2_cache = (cpu_id.cache_info[11] << 8);
				l2_cache += cpu_id.cache_info[10];
				break;
			}
			break;
		case 6:
			switch(cpu_id.model) {
			case 1:
			case 2:
			case 4:
				fellowAddLog("AMD Athlon");
				off = 10;
				break;
			case 3:
				fellowAddLog("AMD Duron");
				off = 9;
				break;
			}
			l1_cache = cpu_id.cache_info[3];
			l1_cache += cpu_id.cache_info[7];
			l2_cache = (cpu_id.cache_info[11] << 8);
			l2_cache += cpu_id.cache_info[10];
		}
		break;

	/* Intel Processors */
	case 'G':
		if (cpu_id.type == 4) {
			switch(cpu_id.model) {
			case 0:
			case 1:
				fellowAddLog("Intel 486DX");
				off = 11;
				break;
			case 2:
				fellowAddLog("Intel 486SX");
				off = 11;
				break;
			case 3:
				fellowAddLog("Intel 486DX2");
				off = 12;
				break;
			case 4:
				fellowAddLog("Intel 486SL");
				off = 11;
				break;
			case 5:
				fellowAddLog("Intel 486SX2");
				off = 12;
				break;
			case 7:
				fellowAddLog("Intel 486DX2-WB");
				off = 15;
				break;
			case 8:
				fellowAddLog("Intel 486DX4");
				off = 12;
				break;
			case 9:
				fellowAddLog("Intel 486DX4-WB");
				off = 15;
				break;
			}
			/* Since we can't get CPU speed or cache info return */
			return;
		}

		/* Get the cache info */
		for (i=0; i<16; i++) {
			switch(cpu_id.cache_info[i]) {
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

		switch(cpu_id.type) {
		case 5:
			switch(cpu_id.model) {
			case 0:
			case 1:
			case 2:
			case 3:
			case 7:
				fellowAddLog("Pentium");
				if (l1_cache == 0) {
					l1_cache = 8;
				}
				off = 7;
				break;
			case 4:
			case 8:
				fellowAddLog("Pentium-MMX");
				if (l1_cache == 0) {
					l1_cache = 16;
				}
				off = 11;
				break;
			}
			break;
		case 6:
			switch(cpu_id.model) {
			case 0:
			case 1:
				fellowAddLog("Pentium Pro");
				off = 11;
				break;
			case 3:
				fellowAddLog("Pentium II");
				off = 10;
				break;
			case 5:
				if (l2_cache == 0) {
					fellowAddLog("Celeron");
					off = 7;
				} else {
					fellowAddLog("Pentium II");
					off = 10;
				}
				break;
			case 6:
				if (l2_cache == 128) {
					fellowAddLog("Celeron");
					off = 7;
				} else {
					fellowAddLog("Pentium II");
					off = 10;
				}
				break;
			case 7:
			case 8:
				fellowAddLog("Pentium III");
				off = 11;
				break;
			}
		}
		break;

	/* Cyrix Processors with CPUID */
	case 'C':
		switch(cpu_id.model) {
		case 0:
			fellowAddLog("Cyrix 6x86MX/MII");
			off = 16;
			break;
		case 4:
			fellowAddLog("Cyrix GXm");
			off = 9;
			break;
		}
		return;
		break;

	/* Unknown processor */
	default:
		off = 3;
		/* Make a guess at the family */
		switch(cpu_id.type) {
		case 5:
			fellowAddLog("586");
			return;
		case 6:
			fellowAddLog("686");
			return;
		}
	}
	fellowAddLog("; L1 Cache: %d KB; L2 Cache: %d KB", l1_cache, l2_cache);
	fellowAddLog("\n");

	 __try
     {
	   speed = cpu_speed();
	   fellowAddLog("Calculated CPU speed: %f MHz\n", speed);
     }
     __except(EXCEPTION_EXECUTE_HANDLER)
     {
       fellowAddLog("Can't calculate CPU speed: processor doesn't support the RDTSC instruction.\n");
     }
}

/*===============*/
/* Do the thing. */
/*===============*/
void
fellowLogSysInfo (void)
{
  fellowVersionInfo ();
  ParseRegistry ();
  ParseOSVersionInfo ();
  ParseSystemInfo ();
  ParseMemoryStatus ();
  EnumRegistry ();
  DetectCPU ();
}
