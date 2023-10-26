// Simple caps plug-in support on Win32, should fit most users
//
// You may want to link with the libray instead, if your product fully complies
// with the license.
// If using the library directly without the plug-in, define CAPS_USER, and include CapsLib.h
// functions are the same with the plug-in, but their name start with "CAPS", not "Caps"
// CAPSInit does not have any parameters, CapsInit gets the library name
//
// Comment out stdafx.h if your project does not use the MSVC precompiled headers
//
// www.caps-project.org

//#include "stdafx.h"
#include "Windows.h"

#include "ComType.h"
#include "CapsAPI.h"

#include "windows.h"
#include "CapsPlug.h"

//#define CAPS_USER
//#include "CapsLib.h"

HMODULE capi = NULL;

CapsProc cpr[] = {
    "CAPSInit",
    NULL,
    "CAPSExit",
    NULL,
    "CAPSAddImage",
    NULL,
    "CAPSRemImage",
    NULL,
    "CAPSLockImage",
    NULL,
    "CAPSUnlockImage",
    NULL,
    "CAPSLoadImage",
    NULL,
    "CAPSGetImageInfo",
    NULL,
    "CAPSLockTrack",
    NULL,
    "CAPSUnlockTrack",
    NULL,
    "CAPSUnlockAllTracks",
    NULL,
    "CAPSGetPlatformName",
    NULL,
    "CAPSLockImageMemory",
    NULL,
    NULL,
    NULL};

// start caps image support
SDWORD CapsInit(LPCTSTR lib)
{
  if (capi) return imgeOk;

  capi = LoadLibrary(lib);
  if (!capi) return imgeUnsupported;

  for (int prc = 0; cpr[prc].name; prc++)
    cpr[prc].proc = GetProcAddress(capi, cpr[prc].name);

  SDWORD res = cpr[0].proc ? CAPSHOOKN(cpr[0].proc)() : imgeUnsupported;

  return res;
}

// stop caps image support
SDWORD CapsExit()
{
  SDWORD res = cpr[1].proc ? CAPSHOOKN(cpr[1].proc)() : imgeUnsupported;

  if (capi)
  {
    FreeLibrary(capi);
    capi = NULL;
  }

  for (int prc = 0; cpr[prc].name; prc++)
    cpr[prc].proc = NULL;

  return res;
}

// add image container
SDWORD CapsAddImage()
{
  SDWORD res = cpr[2].proc ? CAPSHOOKN(cpr[2].proc)() : -1;

  return res;
}

// delete image container
SDWORD CapsRemImage(SDWORD id)
{
  SDWORD res = cpr[3].proc ? CAPSHOOKN(cpr[3].proc)(id) : -1;

  return res;
}

// lock image
SDWORD CapsLockImage(SDWORD id, PCHAR name)
{
  SDWORD res = cpr[4].proc ? CAPSHOOKN(cpr[4].proc)(id, name) : imgeUnsupported;

  return res;
}

// unlock image
SDWORD CapsUnlockImage(SDWORD id)
{
  SDWORD res = cpr[5].proc ? CAPSHOOKN(cpr[5].proc)(id) : imgeUnsupported;

  return res;
}

// load and decode complete image
SDWORD CapsLoadImage(SDWORD id, UDWORD flag)
{
  SDWORD res = cpr[6].proc ? CAPSHOOKN(cpr[6].proc)(id, flag) : imgeUnsupported;

  return res;
}

// get image information
SDWORD CapsGetImageInfo(PCAPSIMAGEINFO pi, SDWORD id)
{
  SDWORD res = cpr[7].proc ? CAPSHOOKN(cpr[7].proc)(pi, id) : imgeUnsupported;

  return res;
}

// load and decode track, or return with the cache
SDWORD CapsLockTrack(PCAPSTRACKINFO pi, SDWORD id, UDWORD cylinder, UDWORD head, UDWORD flag)
{
  SDWORD res = cpr[8].proc ? CAPSHOOKN(cpr[8].proc)(pi, id, cylinder, head, flag) : imgeUnsupported;

  return res;
}

// remove track from cache
SDWORD CapsUnlockTrack(SDWORD id, UDWORD cylinder, UDWORD head)
{
  SDWORD res = cpr[9].proc ? CAPSHOOKN(cpr[9].proc)(id, cylinder, head) : imgeUnsupported;

  return res;
}

// remove all tracks from cache
SDWORD CapsUnlockAllTracks(SDWORD id)
{
  SDWORD res = cpr[10].proc ? CAPSHOOKN(cpr[10].proc)(id) : imgeUnsupported;

  return res;
}

// get platform name
PCHAR CapsGetPlatformName(UDWORD pid)
{
  PCHAR res = cpr[11].proc ? CAPSHOOKS(cpr[11].proc)(pid) : NULL;

  return res;
}

// lock memory mapped image
SDWORD CapsLockImageMemory(SDWORD id, PUBYTE buffer, UDWORD length, UDWORD flag)
{
  SDWORD res = cpr[12].proc ? CAPSHOOKN(cpr[12].proc)(id, buffer, length, flag) : imgeUnsupported;

  return res;
}
