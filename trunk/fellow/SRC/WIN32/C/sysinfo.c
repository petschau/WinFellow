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
/* 2000/10/23: some NT systems have an additional hardware enum path          */
/* 2000/10/21: reads more stuff from the registry, even more experimental     */
/*             regarding to where to find stuff in different windows versions */
/*             and how to enumerate keys in two levels...                     */
/* 2000/10/19: first release, highly experimental                             */
/*============================================================================*/

#include <windows.h>
#include <winbase.h>
#include <ddraw.h>

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

  dwError = GetLastError(); 

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
    RegQueryValueEx (hKey, ValueName, NULL, &dwType, (LPBYTE) &dwBuffer,
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
  char *szClass, *szDevice;

  /* get handle to specified key tree */
  if (RegOpenKeyEx
      (HKEY_LOCAL_MACHINE, SubKey, 0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
       &hKey) != ERROR_SUCCESS)
    {
      LogErrorMessageFromSystem();
      return;
    }

  /* retrieve information about that key (no. of subkeys) */
  if (RegQueryInfoKey (hKey, NULL, NULL, NULL, &dwNoSubkeys, NULL,
		       NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
    {
      LogErrorMessageFromSystem();
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
	  LogErrorMessageFromSystem();
	  continue;
	}

      /* now query this subkey for the keys with the real information...
         I hate the registry. */
      if (RegOpenKeyEx (hKey, szSubKeyName, 0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
			&hKey2) != ERROR_SUCCESS)
	{
	  LogErrorMessageFromSystem();
	  return;
	}

      if (RegQueryInfoKey (hKey2, NULL, NULL, NULL, &dwNoSubkeys2, NULL,
			   NULL, NULL, NULL, NULL, NULL,
			   NULL) != ERROR_SUCCESS)
	{
	  LogErrorMessageFromSystem();
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
	      LogErrorMessageFromSystem();
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
      LogErrorMessageFromSystem();
      return;
    }

  if (osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
	  /* this seems to be the right place in Win2k and NT */
      EnumHardwareTree (TEXT ("SYSTEM\\CurrentControlSet\\Enum\\PCI"));
      EnumHardwareTree (TEXT ("SYSTEM\\CurrentControlSet\\Enum\\ISAPNP"));
	  /* I'm not sure where exactly that one is required; I needed it */
	  /* on WinNT 4 SP 5:                                             */
	  EnumHardwareTree (TEXT ("SYSTEM\\CurrentControlSet\\Enum\\Root"));
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
  fellowAddLog("Processor revision: %d\n", wProcessorRevision;*/
}

static void
ParseOSVersionInfo (void)
{
  OSVERSIONINFO osInfo;
  ZeroMemory (&osInfo, sizeof (OSVERSIONINFO));
  osInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
  if (!GetVersionEx (&osInfo))
    {
      LogErrorMessageFromSystem();
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
  if(osInfo.dwMajorVersion == 5)
	    fellowAddLog("This should be Windows 2000\n");

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
     ("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
     TEXT ("~MHz")); 
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
}
