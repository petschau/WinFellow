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
/* 2000/10/19: first release, highly experimental                             */
/*============================================================================*/

#include <windows.h>
#include <winbase.h>


/*=============================================================================*/
/* Read a string value from the registry; if succesful a pointer to the string */
/* is returned, if failed it returns NULL                                      */
/*=============================================================================*/
static char *RegistryQueryStringValue(HKEY RootKey, LPCTSTR SubKey, LPCTSTR ValueName)
{
  HKEY hKey;
  TCHAR szBuffer[255];
  DWORD dwBufLen = 255;
  DWORD dwType;
  LONG lRet;
  char *result;

  if(RegOpenKeyEx(RootKey, SubKey, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS) return NULL;
  lRet = RegQueryValueEx(hKey, ValueName, NULL,	&dwType, (LPBYTE) szBuffer, &dwBufLen);
  RegCloseKey(hKey);

  if (lRet != ERROR_SUCCESS) return NULL;
  if (dwType != REG_SZ) return NULL;

  result = (char *) malloc(strlen(szBuffer)+1);
  strcpy(result, szBuffer);

  return result;
}

static void ParseSystemInfo(void)
{
  SYSTEM_INFO SystemInfo;

  GetSystemInfo(&SystemInfo);

  fellowAddLog("Number of processors: %d\n",
  SystemInfo.dwNumberOfProcessors);
  fellowAddLog("Processor Type: %d\n", SystemInfo.dwProcessorType);
  fellowAddLog("Architecture: %d\n", SystemInfo.wProcessorArchitecture);
  //fellowAddLog("Processor level: %d\n", wProcessorLevel);
  //fellowAddLog("Processor revision: %d\n", wProcessorRevision;
}

static void ParseOSVersionInfo(void)
{
  OSVERSIONINFO osInfo;

  ZeroMemory(&osInfo, sizeof (OSVERSIONINFO));
  osInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

  if (!GetVersionEx(&osInfo))
  {
    fellowAddLog("JoyDrv -- GetVersionEx fail -- [%d]\n", GetLastError());
	return;
  }

#ifdef USE_DX5
  if ((osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
    (osInfo.dwMajorVersion == 4) && (osInfo.dwMinorVersion == 0))
#else
  if (osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
#endif
  {
    char *tempstr = NULL;
	fellowAddLog("We seem to be running on NT.");
	tempstr = RegistryQueryStringValue(
	  HKEY_LOCAL_MACHINE,
	  TEXT("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"),
	  TEXT("ProductType")
	);
	if(tempstr)
	{
	  fellowAddLog(" It identifies itself as %s.\n", tempstr);
	  free(tempstr);
	}
	else
	  fellowAddLog("\n");
  }

  fellowAddLog("OS %d.%d build %d desc %s, platform %d\n",
    osInfo.dwMajorVersion,
	osInfo.dwMinorVersion,
	osInfo.dwBuildNumber,
	(osInfo.szCSDVersion ? osInfo.szCSDVersion : "--"),
	 osInfo.dwPlatformId);
}

static void ParseRegistry(void)
{
  char *tempstr = NULL;

  fellowAddLog("Querying registry for processor information:\n");
  tempstr = RegistryQueryStringValue(
    HKEY_LOCAL_MACHINE, 
    TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
    TEXT("VendorIdentifier")
	);
  if(tempstr)
  {
    fellowAddLog("CPU Vendor: %s\n", tempstr);
	free(tempstr);
  }

  tempstr = RegistryQueryStringValue(
    HKEY_LOCAL_MACHINE, 
    TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
    TEXT("Identifier")
	);
  if(tempstr)
  {
	  fellowAddLog("CPU Identifier: %s\n", tempstr);
	  free(tempstr);
  }
}

static void ParseMemoryStatus(void)
{
  MEMORYSTATUS MemoryStatus;

  ZeroMemory(&MemoryStatus, sizeof(MemoryStatus));
  MemoryStatus.dwLength = sizeof(MEMORYSTATUS);

  GlobalMemoryStatus(&MemoryStatus);

  fellowAddLog("Memory information:\n");

  fellowAddLog("Total physical memory: %u MB (%u Bytes)\n",
    MemoryStatus.dwTotalPhys / 1024 / 1024,
    MemoryStatus.dwTotalPhys);
  fellowAddLog("Free physical memory: %u MB (%u Bytes)\n", 
    MemoryStatus.dwAvailPhys / 1024 / 1024,
	MemoryStatus.dwAvailPhys);
  fellowAddLog("Memory load (percent): %u\n", MemoryStatus.dwMemoryLoad);
  fellowAddLog("Total size of pagefile: %u MB (%u Bytes)\n",
    MemoryStatus.dwTotalPageFile / 1024 / 1024,
	MemoryStatus.dwTotalPageFile);
  fellowAddLog("Free size of pagefile: %u MB (%u Bytes)\n", 
    MemoryStatus.dwAvailPageFile / 1024 / 1024,
	MemoryStatus.dwAvailPageFile);
  fellowAddLog("Total virtual address space: %u MB (%u Bytes)\n",
    MemoryStatus.dwTotalVirtual / 1024 / 1024,
	MemoryStatus.dwTotalVirtual);
  fellowAddLog("Free virtual address space: %u MB (%u Bytes)\n",
    MemoryStatus.dwAvailVirtual / 1024 / 1024,
	MemoryStatus.dwAvailVirtual);
}

static void fellowVersionInfo(void)
{	 
  fellowAddLog("WinFellow alpha v0.4.2 build 1");
#ifdef USE_DX3
  fellowAddLog(" (DX3");
#else
  fellowAddLog(" (DX5");
#endif
#ifdef _DEBUG
  fellowAddLog(" Debug Build)");
#else
  fellowAddLog(" Release Build)");
#endif
  fellowAddLog(" now starting up...\n");
}

/*===============*/
/* Do the thing. */
/*===============*/
void fellowLogSysInfo(void)
{
  fellowVersionInfo();
  ParseRegistry();
  ParseOSVersionInfo();
  ParseSystemInfo();
  ParseMemoryStatus(); 
}
