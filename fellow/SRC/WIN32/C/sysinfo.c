/*=========================================================================*/
/* WinFellow                                                               */
/*                                                                         */
/* System information retrieval                                            */
/*                                                                         */
/* Author: Torsten Enderling                                               */
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
#include "sysinfo.h"

#define MYREGBUFFERSIZE 1024

typedef BOOL(WINAPI* LPFN_GLPI)(
  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,
  PDWORD);

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

  sprintf(szTemp, "Error %u: ", dwError);
  cMsgLen =
    FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER
		   | 40, NULL, dwError, MAKELANGID (0, SUBLANG_ENGLISH_US),
		   (LPTSTR) & msgBuf, MYREGBUFFERSIZE, NULL);
  if(cMsgLen) {
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

  lRet = RegQueryValueEx (hKey, ValueName, NULL, &dwType, (LPBYTE) szBuffer, &dwBufLen);

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
    sysinfoEnumHardwareTree(TEXT ("Enum\\PCI"));
    sysinfoEnumHardwareTree(TEXT ("Enum\\ISAPNP"));
  }
}

static const char* sysinfoGetProcessorArchitectureDescription(WORD wProcessorArchitecture)
{
  switch(wProcessorArchitecture)
  {
  case PROCESSOR_ARCHITECTURE_INTEL:
    return "INTEL";
  case PROCESSOR_ARCHITECTURE_MIPS:
    return "MIPS";
  case PROCESSOR_ARCHITECTURE_ALPHA:
    return "ALPHA";
  case PROCESSOR_ARCHITECTURE_PPC:
    return "PPC";
  case PROCESSOR_ARCHITECTURE_SHX:
    return "SHX";
  case PROCESSOR_ARCHITECTURE_ARM:
    return "ARM";
  case PROCESSOR_ARCHITECTURE_IA64:
    return "IA64";
  case PROCESSOR_ARCHITECTURE_ALPHA64:
    return "ALPHA64";
  case PROCESSOR_ARCHITECTURE_MSIL:
    return "MSIL";
  case PROCESSOR_ARCHITECTURE_AMD64:
    return "AMD64";
  case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
    return "IA32_ON_WIN64";
  }
  return "UNKNOWN PROCESSUR ARCHITECTURE";
}

/*================================*/
/* windows system info structures */
/*================================*/

static void sysinfoParseSystemInfo (void)
{
  SYSTEM_INFO SystemInfo;
  GetNativeSystemInfo(&SystemInfo);
  fellowAddTimelessLog("\tlogical processors: \t%d\n", SystemInfo.dwNumberOfProcessors);
  fellowAddTimelessLog("\tarchitecture: \t\t%s\n", sysinfoGetProcessorArchitectureDescription(SystemInfo.wProcessorArchitecture));
  fellowAddTimelessLog("\tlevel: \t\t\t%d\n", SystemInfo.wProcessorLevel);
  fellowAddTimelessLog("\trevision: \t\t%d\n", SystemInfo.wProcessorRevision);
}

// https://docs.microsoft.com/de-de/windows/desktop/api/sysinfoapi/nf-sysinfoapi-getlogicalprocessorinformation
// Helper function to count set bits in the processor mask.
static DWORD sysinfoCountSetBits(ULONG_PTR bitMask)
{
  DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
  DWORD bitSetCount = 0;
  ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;
  DWORD i;

  for (i = 0; i <= LSHIFT; ++i)
  {
    bitSetCount += ((bitMask & bitTest) ? 1 : 0);
    bitTest /= 2;
  }

  return bitSetCount;
}

static int sysinfoParseProcessorInformation()
{
  LPFN_GLPI glpi;
  BOOL done = FALSE;
  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
  DWORD returnLength = 0;
  DWORD logicalProcessorCount = 0;
  DWORD numaNodeCount = 0;
  DWORD processorCoreCount = 0;
  DWORD processorL1CacheCount = 0;
  DWORD processorL2CacheCount = 0;
  DWORD processorL3CacheCount = 0;
  DWORD processorPackageCount = 0;
  DWORD byteOffset = 0;
  PCACHE_DESCRIPTOR Cache;

  glpi = (LPFN_GLPI)GetProcAddress(
    GetModuleHandle(TEXT("kernel32")),
    "GetLogicalProcessorInformation");
  if (NULL == glpi)
  {
    fellowAddTimelessLog("\n\tGetLogicalProcessorInformation is not supported.\n");
    return (1);
  }

  while (!done)
  {
    DWORD rc = glpi(buffer, &returnLength);

    if (FALSE == rc)
    {
      if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
      {
        if (buffer)
          free(buffer);

        buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(
          returnLength);

        if (NULL == buffer)
        {
          fellowAddTimelessLog("\n\tError: Allocation failure\n");
          return (2);
        }
      }
      else
      {
        fellowAddTimelessLog(("\n\tError %d\n"), GetLastError());
        return (3);
      }
    }
    else
    {
      done = TRUE;
    }
  }

  ptr = buffer;

  while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength)
  {
    switch (ptr->Relationship)
    {
    case RelationNumaNode:
      // Non-NUMA systems report a single record of this type.
      numaNodeCount++;
      break;

    case RelationProcessorCore:
      processorCoreCount++;

      // A hyperthreaded core supplies more than one logical processor.
      logicalProcessorCount += sysinfoCountSetBits(ptr->ProcessorMask);
      break;

    case RelationCache:
      // Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache. 
      Cache = &ptr->Cache;
      if (Cache->Level == 1)
      {
        processorL1CacheCount++;
      }
      else if (Cache->Level == 2)
      {
        processorL2CacheCount++;
      }
      else if (Cache->Level == 3)
      {
        processorL3CacheCount++;
      }
      break;

    case RelationProcessorPackage:
      // Logical processors share a physical package.
      processorPackageCount++;
      break;

    default:
      fellowAddTimelessLog("\n\tError: Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value.\n");
      break;
    }
    byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
    ptr++;
  }

  fellowAddTimelessLog("\tnumber of NUMA nodes: \t\t\t%d\n", numaNodeCount);
  fellowAddTimelessLog("\tnumber of physical processor packages: \t%d\n", processorPackageCount);
  fellowAddTimelessLog("\tnumber of processor cores: \t\t%d\n", processorCoreCount);
  fellowAddTimelessLog("\tnumber of logical processors: \t\t%d\n", logicalProcessorCount);
  fellowAddTimelessLog("\tnumber of processor L1/L2/L3 caches: \t%d/%d/%d\n",
    processorL1CacheCount,
    processorL2CacheCount,
    processorL3CacheCount);
  fellowAddTimelessLog("\n");

  free(buffer);

  return 0;
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
      if((osInfo.dwMajorVersion == 4) && (osInfo.dwMinorVersion == 0)) {
	      fellowAddTimelessLog("\toperating system: \tWindows 95 ");
	      if (osInfo.szCSDVersion[1] == 'C' || osInfo.szCSDVersion[1] == 'B' ) {
          fellowAddTimelessLog("OSR2\n");
        } else {
          fellowAddTimelessLog("\n");
	      }
      }

      if((osInfo.dwMajorVersion == 4) && (osInfo.dwMinorVersion == 10)) {
	fellowAddTimelessLog("\toperating system: \tWindows 98 ");
	if ( osInfo.szCSDVersion[1] == 'A' ) {
          fellowAddTimelessLog("SE\n" );
	} else {
    	  fellowAddTimelessLog("\n");
	}
      } 

      if((osInfo.dwMajorVersion == 4) && (osInfo.dwMinorVersion == 90)) {
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
      case 6:
        switch (osInfo.dwMinorVersion) {
        case 0:
          fellowAddTimelessLog("\toperating system: \tWindows Vista\n");
          break;
        case 1:
          fellowAddTimelessLog("\toperating system: \tWindows 7\n");
          break;
        case 2:
          fellowAddTimelessLog("\toperating system: \tWindows 8\n");
          break;
        case 3:
          fellowAddTimelessLog("\toperating system: \tWindows 8.1\n");
          break;
        }
        break;
	  case 10:
      switch (osInfo.dwMinorVersion) {
        case 0:
          STR strOS[24] = "Windows 10";
          switch (osInfo.dwBuildNumber)
          {
            case 10240:
              sprintf(strOS, "Windows 10 version 1507");
              break;
            case 10586:
              sprintf(strOS, "Windows 10 version 1511");
              break;
            case 14393:
              sprintf(strOS, "Windows 10 version 1607");
              break;
            case 15063:
              sprintf(strOS, "Windows 10 version 1703");
              break;
            case 16299:
              sprintf(strOS, "Windows 10 version 1709");
              break;
            case 17134:
              sprintf(strOS, "Windows 10 version 1803");
              break;
            case 17763:
              sprintf(strOS, "Windows 10 version 1809");
              break;
            case 18362:
              sprintf(strOS, "Windows 10 version 1903");
              break;
          }
          fellowAddTimelessLog("\toperating system : \t%s\n", strOS);
          break;
        }
        break;
      default:
        fellowAddTimelessLog("\toperating system: \tunknown platform Win32 NT\n");
      }
      break;
    default:	
      fellowAddTimelessLog("\toperating system: \tunknown\n");	
  }

  fellowAddTimelessLog("\tparameters: \t\tOS %d.%d build %d, %s\n", 
    osInfo.dwMajorVersion, osInfo.dwMinorVersion, osInfo.dwBuildNumber,
    strcmp(osInfo.szCSDVersion, "") != 0 ? osInfo.szCSDVersion : "no servicepack");

  switch (osInfo.wProductType)
  {
    case VER_NT_WORKSTATION:
      fellowAddTimelessLog("\tproduct type: \t\tworkstation\n");
      break;
    case VER_NT_SERVER:
      fellowAddTimelessLog("\tproduct type: \t\tserver\n");
      break;
    case VER_NT_DOMAIN_CONTROLLER:
      fellowAddTimelessLog("\tproduct type: \t\tdomain controller\n");
      break;
    default:
      fellowAddTimelessLog("\tproduct type: \t\tunknown product type\n");
      break;
  }
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
  MEMORYSTATUSEX MemoryStatusEx;

  ZeroMemory(&MemoryStatusEx, sizeof (MemoryStatusEx));
  MemoryStatusEx.dwLength = sizeof (MEMORYSTATUSEX);
  GlobalMemoryStatusEx(&MemoryStatusEx);

  fellowAddTimelessLog("\ttotal physical memory: \t\t%I64d MB\n", MemoryStatusEx.ullTotalPhys / 1024 / 1024);
  fellowAddTimelessLog("\tfree physical memory: \t\t%I64d MB\n", MemoryStatusEx.ullAvailPhys / 1024 / 1024);
  fellowAddTimelessLog("\tmemory in use: \t\t\t%u%%\n", MemoryStatusEx.dwMemoryLoad);
  fellowAddTimelessLog("\ttotal size of pagefile: \t%I64d MB\n", MemoryStatusEx.ullTotalPageFile / 1024 / 1024);
  fellowAddTimelessLog("\tfree size of pagefile: \t\t%I64d MB\n", MemoryStatusEx.ullAvailPageFile / 1024 / 1024);
  fellowAddTimelessLog("\ttotal virtual address space: \t%I64d MB\n", MemoryStatusEx.ullTotalVirtual / 1024 / 1024);
  fellowAddTimelessLog("\tfree virtual address space: \t%I64d MB\n", MemoryStatusEx.ullAvailVirtual / 1024 / 1024);
}

static void sysinfoVersionInfo (void) {
  char *versionstring = fellowGetVersionString();
  fellowAddTimelessLog(versionstring);
  free(versionstring);

  #ifdef _DEBUG
    fellowAddTimelessLog(" (debug build)\n");
  #else
    fellowAddTimelessLog(" (release build)\n");
  #endif
}

/*===============*/
/* Do the thing. */
/*===============*/
void sysinfoLogSysInfo(void)
{
  sysinfoVersionInfo();
  fellowAddTimelessLog("\nsystem information:\n\n");
  sysinfoParseRegistry();
  fellowAddTimelessLog("\n");
  sysinfoParseOSVersionInfo();
  fellowAddTimelessLog("\n");
  sysinfoParseSystemInfo();
  fellowAddTimelessLog("\n");
  sysinfoParseProcessorInformation();
  sysinfoParseMemoryStatus();
  fellowAddTimelessLog("\n");
  sysinfoEnumRegistry();
  fellowAddTimelessLog("\n\ndebug information:\n\n");
}

