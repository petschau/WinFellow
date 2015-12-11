/*=========================================================================*/
/* Fellow                                                                  */
/* Host framebuffer driver                                                 */
/* DirectX implementation                                                  */
/* Author: Petter Schau                                                    */
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

/***********************************************************************/
/**
  * @file
  * Graphics device module
  *
  * Framebuffer modes of operation:
  *
  * 1. Using the primary buffer non-clipped, with possible back-buffers.
  *    This applies to fullscreen mode with a framepointer.
  * 2. Using a secondary buffer to render and then applying the blitter
  *    to the primary buffer. The primary buffer can be clipped (window)
  *    or non-clipped (fullscreen). In this mode there are no backbuffers.
  *
  * blitmode is used:
  *  1. When running on the desktop in a window.
  *  2. In fullscreen mode with a primary surface that
  *     can not supply a framebuffer pointer.
  *
  * Windows:
  * Two types of windows:
  *  1. Normal desktop window for desktop operation
  *  2. Full-screen window for fullscreen mode
  *
  * Windows are created and destroyed on emulation start and stop.
  *
  * Buffers:
  * Buffers are created when emulation starts and destroyed
  * when emulation stops. (Recreated when lost also)
  *
  * if blitmode, create single primary buffer
  * and a secondary buffer for actual rendering in system memory.
  * if windowed also add a clipper to the primary buffer
  *
  * else create a primary buffer (with backbuffers)
  ***************************************************************************/

#define INITGUID

#include "portable.h"

#include <windowsx.h>
#include "gui_general.h"
#include <ddraw.h>
#include <dsound.h>

#include "defs.h"
#include "fellow.h"
#include "fmem.h"
#include "sound.h"
#include "graph.h"
#include "draw.h"
#include "kbd.h"
#include "kbddrv.h"
#include "listtree.h"
#include "windrv.h"
#include "gameport.h"
#include "mousedrv.h"
#include "joydrv.h"
#include "sounddrv.h"
#include "wgui.h"
#include "ini.h"

#include "GfxDrvCommon.h"

#ifdef RETRO_PLATFORM
#include "RetroPlatform.h"
#include "config.h"
#endif

/*==========================================================================*/
/* Structs for holding information about a DirectDraw device and mode       */
/*==========================================================================*/

typedef struct
{
  ULO width;
  ULO height;
  ULO depth;
  ULO refresh;
  ULO redpos;
  ULO redsize;
  ULO greenpos;
  ULO greensize;
  ULO bluepos;
  ULO bluesize;
  ULO pitch;
  bool windowed;
} gfx_drv_ddraw_mode;

typedef struct
{
  LPGUID               lpGUID;
  LPSTR                lpDriverDescription;
  LPSTR                lpDriverName;
  LPDIRECTDRAW         lpDD;
  LPDIRECTDRAW2        lpDD2;
  LPDIRECTDRAWSURFACE  lpDDSPrimary;		        /*!< Primary display surface        */
  LPDIRECTDRAWSURFACE  lpDDSBack;               /*!< Current backbuffer for Primary */
  LPDIRECTDRAWSURFACE  lpDDSSecondary;		      /*!< Source surface in blitmode     */
  DDSURFACEDESC        ddsdPrimary;
  DDSURFACEDESC        ddsdBack;
  DDSURFACEDESC        ddsdSecondary;
  LPDIRECTDRAWCLIPPER  lpDDClipper;
  felist               *modes;
  gfx_drv_ddraw_mode   *mode;
  ULO		       buffercount;
  ULO		       maxbuffercount;
  RECT		       hwnd_clientrect_screen;
  RECT		       hwnd_clientrect_win;
  draw_mode            *drawmode;
  bool		       use_blitter;
  bool		       can_stretch_y;
  bool		       no_dd_hardware;
} gfx_drv_ddraw_device;

gfx_drv_ddraw_device *gfx_drv_ddraw_device_current;
felist               *gfx_drv_ddraw_devices;


bool gfx_drv_ddraw_initialized;

/*==========================================================================*/
/* Returns textual error message. Adapted from DX SDK                       */
/*==========================================================================*/

STR *gfxDrvDDrawErrorString(HRESULT hResult)
{
  switch (hResult)
  {
  case DD_OK:				    return "DD_OK";
  case DDERR_ALREADYINITIALIZED:	    return "DDERR_ALREADYINITIALIZED";
  case DDERR_BLTFASTCANTCLIP:		    return "DDERR_BLTFASTCANTCLIP";
  case DDERR_CANNOTATTACHSURFACE:	    return "DDERR_CANNOTATTACHSURFACE";
  case DDERR_CANNOTDETACHSURFACE:	    return "DDERR_CANNOTDETACHSURFACE";
  case DDERR_CANTCREATEDC:		    return "DDERR_CANTCREATEDC";
  case DDERR_CANTDUPLICATE:               return "DDERR_CANTDUPLICATE";
  case DDERR_CANTLOCKSURFACE:		    return "DDERR_CANTLOCKSURFACE";
  case DDERR_CANTPAGELOCK:		    return "DDERR_CANTPAGELOCK";
  case DDERR_CANTPAGEUNLOCK:		    return "DDERR_CANTPAGEUNLOCK";
  case DDERR_CLIPPERISUSINGHWND:	    return "DDERR_CLIPPERISUSINGHWND";
  case DDERR_COLORKEYNOTSET:		    return "DDERR_COLORKEYNOTSET";
  case DDERR_CURRENTLYNOTAVAIL:	    return "DDERR_CURRENTLYNOTAVAIL";
  case DDERR_DCALREADYCREATED:	    return "DDERR_DCALREADYCREATED";
  case DDERR_DIRECTDRAWALREADYCREATED:    return "DDERR_DIRECTDRAWALREADYCREATED";
  case DDERR_EXCEPTION:		    return "DDERR_EXCEPTION";
  case DDERR_EXCLUSIVEMODEALREADYSET:     return "DDERR_EXCLUSIVEMODEALREADYSET";
  case DDERR_GENERIC:			    return "DDERR_GENERIC";
  case DDERR_HEIGHTALIGN:		    return "DDERR_HEIGHTALIGN";
  case DDERR_HWNDALREADYSET:		    return "DDERR_HWNDALREADYSET";
  case DDERR_HWNDSUBCLASSED:		    return "DDERR_HWNDSUBCLASSED";
  case DDERR_IMPLICITLYCREATED:	    return "DDERR_IMPLICITLYCREATED";
  case DDERR_INCOMPATIBLEPRIMARY:	    return "DDERR_INCOMPATIBLEPRIMARY";
  case DDERR_INVALIDCAPS:		    return "DDERR_INVALIDCAPS";
  case DDERR_INVALIDCLIPLIST:		    return "DDERR_INVALIDCLIPLIST";
  case DDERR_INVALIDDIRECTDRAWGUID:       return "DDERR_INVALIDDIRECTDRAWGUID";
  case DDERR_INVALIDMODE:		    return "DDERR_INVALIDMODE";
  case DDERR_INVALIDOBJECT:		    return "DDERR_INVALIDOBJECT";
  case DDERR_INVALIDPARAMS:		    return "DDERR_INVALIDPARAMS";
  case DDERR_INVALIDPIXELFORMAT:	    return "DDERR_INVALIDPIXELFORMAT";
  case DDERR_INVALIDPOSITION:		    return "DDERR_INVALIDPOSITION";
  case DDERR_INVALIDRECT:		    return "DDERR_INVALIDRECT";
  case DDERR_INVALIDSURFACETYPE:	    return "DDERR_INVALIDSURFACETYPE";
  case DDERR_LOCKEDSURFACES:		    return "DDERR_LOCKEDSURFACES";
  case DDERR_NO3D:			    return "DDERR_NO3D";
  case DDERR_NOALPHAHW:		    return "DDERR_NOALPHAHW";
  case DDERR_NOBLTHW:			    return "DDERR_NOBLTHW";
  case DDERR_NOCLIPLIST:		    return "DDERR_NOCLIPLIST";
  case DDERR_NOCLIPPERATTACHED:	    return "DDERR_NOCLIPPERATTACHED";
  case DDERR_NOCOLORCONVHW:		    return "DDERR_NOCOLORCONVHW";
  case DDERR_NOCOLORKEY:		    return "DDERR_NOCOLORKEY";
  case DDERR_NOCOLORKEYHW:		    return "DDERR_NOCOLORKEYHW";
  case DDERR_NOCOOPERATIVELEVELSET:       return "DDERR_NOCOOPERATIVELEVELSET";
  case DDERR_NODC:			    return "DDERR_NODC";
  case DDERR_NODDROPSHW:		    return "DDERR_NODDROPSHW";
  case DDERR_NODIRECTDRAWHW:		    return "DDERR_NODIRECTDRAWHW";
  case DDERR_NODIRECTDRAWSUPPORT:	    return "DDERR_NODIRECTDRAWSUPPORT";
  case DDERR_NOEMULATION:		    return "DDERR_NOEMULATION";
  case DDERR_NOEXCLUSIVEMODE:		    return "DDERR_NOEXCLUSIVEMODE";
  case DDERR_NOFLIPHW:		    return "DDERR_NOFLIPHW";
  case DDERR_NOGDI:			    return "DDERR_NOGDI";
  case DDERR_NOHWND:			    return "DDERR_NOHWND";
  case DDERR_NOMIPMAPHW:		    return "DDERR_NOMIPMAPHW";
  case DDERR_NOMIRRORHW:		    return "DDERR_NOMIRRORHW";
  case DDERR_NOOVERLAYDEST:		    return "DDERR_NOOVERLAYDEST";
  case DDERR_NOOVERLAYHW:		    return "DDERR_NOOVERLAYHW";
  case DDERR_NOPALETTEATTACHED:	    return "DDERR_NOPALETTEATTACHED";
  case DDERR_NOPALETTEHW:		    return "DDERR_NOPALETTEHW";
  case DDERR_NORASTEROPHW:		    return "DDERR_NORASTEROPHW";
  case DDERR_NOROTATIONHW:		    return "DDERR_NOROTATIONHW";
  case DDERR_NOSTRETCHHW:		    return "DDERR_NOSTRETCHHW";
  case DDERR_NOT4BITCOLOR:		    return "DDERR_NOT4BITCOLOR";
  case DDERR_NOT4BITCOLORINDEX:	    return "DDERR_NOT4BITCOLORINDEX";
  case DDERR_NOT8BITCOLOR:		    return "DDERR_NOT8BITCOLOR";
  case DDERR_NOTAOVERLAYSURFACE:	    return "DDERR_NOTAOVERLAYSURFACE";
  case DDERR_NOTEXTUREHW:		    return "DDERR_NOTEXTUREHW";
  case DDERR_NOTFLIPPABLE:		    return "DDERR_NOTFLIPPABLE";
  case DDERR_NOTFOUND:		    return "DDERR_NOTFOUND";
  case DDERR_NOTINITIALIZED:		    return "DDERR_NOTINITIALIZED";
  case DDERR_NOTLOCKED:		    return "DDERR_NOTLOCKED";
  case DDERR_NOTPAGELOCKED:		    return "DDERR_NOTPAGELOCKED";
  case DDERR_NOTPALETTIZED:		    return "DDERR_NOTPALETTIZED";
  case DDERR_NOVSYNCHW:		    return "DDERR_NOVSYNCHW";
  case DDERR_NOZBUFFERHW:		    return "DDERR_NOZBUFFERHW";
  case DDERR_NOZOVERLAYHW:		    return "DDERR_NOZOVERLAYHW";
  case DDERR_OUTOFCAPS:		    return "DDERR_OUTOFCAPS";
  case DDERR_OUTOFMEMORY:		    return "DDERR_OUTOFMEMORY";
  case DDERR_OUTOFVIDEOMEMORY:	    return "DDERR_OUTOFVIDEOMEMORY";
  case DDERR_OVERLAYCANTCLIP:		    return "DDERR_OVERLAYCANTCLIP";
  case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:return "DDERR_OVERLAYCOLORKEYONLYONEACTIVE";
  case DDERR_OVERLAYNOTVISIBLE:	    return "DDERR_OVERLAYNOTVISIBLE";
  case DDERR_PALETTEBUSY:		    return "DDERR_PALETTEBUSY";
  case DDERR_PRIMARYSURFACEALREADYEXISTS: return "DDERR_PRIMARYSURFACEALREADYEXISTS";
  case DDERR_REGIONTOOSMALL:		    return "DDERR_REGIONTOOSMALL";
  case DDERR_SURFACEALREADYATTACHED:      return "DDERR_SURFACEALREADYATTACHED";
  case DDERR_SURFACEALREADYDEPENDENT:     return "DDERR_SURFACEALREADYDEPENDENT";
  case DDERR_SURFACEBUSY:		    return "DDERR_SURFACEBUSY";
  case DDERR_SURFACEISOBSCURED:	    return "DDERR_SURFACEISOBSCURED";
  case DDERR_SURFACELOST:		    return "DDERR_SURFACELOST";
  case DDERR_SURFACENOTATTACHED:          return "DDERR_SURFACENOTATTACHED";
  case DDERR_TOOBIGHEIGHT:		    return "DDERR_TOOBIGHEIGHT";
  case DDERR_TOOBIGSIZE:		    return "DDERR_TOOBIGSIZE";
  case DDERR_TOOBIGWIDTH:		    return "DDERR_TOOBIGWIDTH";
  case DDERR_UNSUPPORTED:		    return "DDERR_UNSUPPORTED";
  case DDERR_UNSUPPORTEDFORMAT:	    return "DDERR_UNSUPPORTEDFORMAT";
  case DDERR_UNSUPPORTEDMASK:		    return "DDERR_UNSUPPORTEDMASK";
  case DDERR_UNSUPPORTEDMODE:		    return "DDERR_UNSUPPORTEDMODE";
  case DDERR_VERTICALBLANKINPROGRESS:     return "DDERR_VERTICALBLANKINPROGRESS";
  case DDERR_WASSTILLDRAWING:		    return "DDERR_WASSTILLDRAWING";
  case DDERR_WRONGMODE:		    return "DDERR_WRONGMODE";
  case DDERR_XALIGN:			    return "DDERR_XALIGN";
  }
  return "Not a DirectDraw Error";
}


/*==========================================================================*/
/* Logs a sensible error message                                            */
/*==========================================================================*/

void gfxDrvDDrawFailure(STR *header, HRESULT err)
{
  fellowAddLog("gfxdrv: ");
  fellowAddLog(header);
  fellowAddLog(gfxDrvDDrawErrorString(err));
  fellowAddLog("\n");
}

/*==========================================================================*/
/* Get information about the current client rectangle                       */
/*==========================================================================*/

void gfxDrvDDrawFindWindowClientRect(gfx_drv_ddraw_device *ddraw_device)
{
  GetClientRect(gfxDrvCommon->GetHWND(), &ddraw_device->hwnd_clientrect_win);

#ifdef RETRO_PLATFORM
  if (RetroPlatformGetMode())
  {
    ULO lDisplayScale = RetroPlatformGetDisplayScale();

    ddraw_device->hwnd_clientrect_win.left   *= lDisplayScale;
    ddraw_device->hwnd_clientrect_win.top    *= lDisplayScale;
    ddraw_device->hwnd_clientrect_win.right  *= lDisplayScale;
    ddraw_device->hwnd_clientrect_win.bottom *= lDisplayScale;
   }
#endif

  memcpy(&ddraw_device->hwnd_clientrect_screen, &ddraw_device->hwnd_clientrect_win, sizeof(RECT));
  ClientToScreen(gfxDrvCommon->GetHWND(), (LPPOINT)&ddraw_device->hwnd_clientrect_screen);
  ClientToScreen(gfxDrvCommon->GetHWND(), (LPPOINT)&ddraw_device->hwnd_clientrect_screen + 1);
}

/*==========================================================================*/
/* Select the correct drawing surface for a draw operation                  */
/*==========================================================================*/

void gfxDrvDDrawDrawTargetSurfaceSelect(gfx_drv_ddraw_device *ddraw_device,
  LPDIRECTDRAWSURFACE *lpDDS,
  LPDDSURFACEDESC *lpDDSD)
{
  if (ddraw_device->use_blitter)
  {
    *lpDDS = ddraw_device->lpDDSSecondary;
    *lpDDSD = &(ddraw_device->ddsdSecondary);
  }
  else
  {
    *lpDDS = (ddraw_device->buffercount == 1) ? ddraw_device->lpDDSPrimary :
      ddraw_device->lpDDSBack;
    *lpDDSD = (ddraw_device->buffercount == 1) ? &(ddraw_device->ddsdPrimary) :
      &(ddraw_device->ddsdBack);
  }
}

/*==========================================================================*/
/* Select the correct target surface for a blit operation                   */
/*==========================================================================*/

void gfxDrvDDrawBlitTargetSurfaceSelect(gfx_drv_ddraw_device *ddraw_device, LPDIRECTDRAWSURFACE *lpDDS)
{
  *lpDDS = (ddraw_device->buffercount == 1) ? ddraw_device->lpDDSPrimary :
    ddraw_device->lpDDSBack;
}

/*==========================================================================*/
/* Release clipper                                                          */
/*==========================================================================*/

void gfxDrvDDrawClipperRelease(gfx_drv_ddraw_device *ddraw_device)
{
  if (ddraw_device->lpDDClipper != NULL)
  {
    IDirectDrawClipper_Release(ddraw_device->lpDDClipper);
    ddraw_device->lpDDClipper = NULL;
  }
}


/*==========================================================================*/
/* Initialize clipper                                                       */
/*==========================================================================*/

bool gfxDrvDDrawClipperInitialize(gfx_drv_ddraw_device *ddraw_device)
{
  HRESULT err = IDirectDraw2_CreateClipper(ddraw_device->lpDD2, 0, &ddraw_device->lpDDClipper, NULL);

  if (err != DD_OK)
  {
    gfxDrvDDrawFailure("gfxDrvDDrawClipperInitialize: CreateClipper() ", err);
  }
  else
  {
    err = IDirectDrawClipper_SetHWnd(ddraw_device->lpDDClipper, 0, gfxDrvCommon->GetHWND());
    if (err != DD_OK)
    {
      gfxDrvDDrawFailure("gfxDrvDDrawClipperInitialize: SetHWnd() ", err);
    }
    else
    {
      err = IDirectDrawSurface_SetClipper(ddraw_device->lpDDSPrimary, ddraw_device->lpDDClipper);
      if (err != DD_OK)
      {
        gfxDrvDDrawFailure("gfxDrvDDrawClipperInitialize: SetClipper() ", err);
      }
    }
  }
  if (err != DD_OK)
  {
    gfxDrvDDrawClipperRelease(ddraw_device);
  }
  return (err == DD_OK);
}


/*==========================================================================*/
/* Callback used to collect information about available DirectDraw devices  */
/* Called on emulator startup                                               */
/*==========================================================================*/

BOOL WINAPI gfxDrvDDrawDeviceEnumerate(GUID FAR *lpGUID,
  LPSTR lpDriverDescription,
  LPSTR lpDriverName,
  LPVOID lpContext)
{
  gfx_drv_ddraw_device *tmpdev;

  winDrvSetThreadName(-1, "gfxDrvDDrawDeviceEnumerate()");

  tmpdev = (gfx_drv_ddraw_device *)malloc(sizeof(gfx_drv_ddraw_device));
  memset(tmpdev, 0, sizeof(gfx_drv_ddraw_device));
  if (lpGUID == NULL)
  {
    tmpdev->lpGUID = NULL;
    gfx_drv_ddraw_device_current = tmpdev;
  }
  else
  {
    tmpdev->lpGUID = (LPGUID)malloc(sizeof(GUID) + 16);
    memcpy(tmpdev->lpGUID, lpGUID, sizeof(GUID));
  }
  tmpdev->lpDriverDescription = (LPSTR)malloc(strlen(lpDriverDescription) + 1);
  strcpy(tmpdev->lpDriverDescription, lpDriverDescription);
  tmpdev->lpDriverName = (LPSTR)malloc(strlen(lpDriverName) + 1);
  strcpy(tmpdev->lpDriverName, lpDriverName);
  gfx_drv_ddraw_devices = listAddLast(gfx_drv_ddraw_devices, listNew(tmpdev));
  return DDENUMRET_OK;
}


/*==========================================================================*/
/* Dump information about available DirectDraw devices to log               */
/*==========================================================================*/

void gfxDrvDDrawDeviceInformationDump()
{
  felist *l;
  STR s[120];

  sprintf(s, "gfxdrv: DirectDraw devices found: %u\n", listCount(gfx_drv_ddraw_devices));
  fellowAddLog(s);
  for (l = gfx_drv_ddraw_devices; l != NULL; l = listNext(l))
  {
    gfx_drv_ddraw_device *tmpdev = (gfx_drv_ddraw_device *)listNode(l);
    sprintf(s, "gfxdrv: DirectDraw Driver Description: %s\n", tmpdev->lpDriverDescription);
    fellowAddLog(s);
    sprintf(s, "gfxdrv: DirectDraw Driver Name       : %s\n\n", tmpdev->lpDriverName);
    fellowAddLog(s);
  }
}


/*==========================================================================*/
/* Creates a list of available DirectDraw devices                           */
/* Called on emulator startup                                               */
/*==========================================================================*/

bool gfxDrvDDrawDeviceInformationInitialize()
{
  gfx_drv_ddraw_devices = NULL;
  gfx_drv_ddraw_device_current = NULL;
  HRESULT err = DirectDrawEnumerate(gfxDrvDDrawDeviceEnumerate, NULL);
  if (err != DD_OK)
  {
    gfxDrvDDrawFailure("gfxDrvDDrawDeviceInformationInitialize(), DirectDrawEnumerate(): ", err);
  }
  if (gfx_drv_ddraw_device_current == NULL)
  {
    gfx_drv_ddraw_device_current = (gfx_drv_ddraw_device *)listNode(gfx_drv_ddraw_devices);
  }
  gfxDrvDDrawDeviceInformationDump();
  return (listCount(gfx_drv_ddraw_devices) > 0);
}


/*==========================================================================*/
/* Releases the list of available DirectDraw devices                        */
/* Called on emulator shutdown                                              */
/*==========================================================================*/

void gfxDrvDDrawDeviceInformationRelease()
{
  felist *l;

  for (l = gfx_drv_ddraw_devices; l != NULL; l = listNext(l))
  {
    gfx_drv_ddraw_device *tmpdev = (gfx_drv_ddraw_device *)listNode(l);
    if (tmpdev->lpGUID != NULL)
    {
      free(tmpdev->lpGUID);
    }
    free(tmpdev->lpDriverDescription);
    free(tmpdev->lpDriverName);
  }
  listFreeAll(gfx_drv_ddraw_devices, TRUE);
  gfx_drv_ddraw_device_current = NULL;
  gfx_drv_ddraw_devices = NULL;
}


/*==========================================================================*/
/* Creates the DirectDraw1 object for the given DirectDraw device           */
/* Called on emulator startup                                               */
/*==========================================================================*/

bool gfxDrvDDraw1ObjectInitialize(gfx_drv_ddraw_device *ddraw_device)
{
  ddraw_device->lpDD = NULL;
  HRESULT err = DirectDrawCreate(ddraw_device->lpGUID, &(ddraw_device->lpDD), NULL);
  if (err != DD_OK)
  {
    gfxDrvDDrawFailure("gfxDrvDDraw1ObjectInitialize(): ", err);
    return false;
  }
  return true;
}


/*==========================================================================*/
/* Creates the DirectDraw2 object for the given DirectDraw device           */
/* Requires that a DirectDraw1 object has been initialized                  */
/* Called on emulator startup                                               */
/*==========================================================================*/

bool gfxDrvDDraw2ObjectInitialize(gfx_drv_ddraw_device *ddraw_device)
{
  ddraw_device->lpDD2 = NULL;
  HRESULT err = IDirectDraw_QueryInterface(ddraw_device->lpDD,
    IID_IDirectDraw2,
    (LPVOID *)&(ddraw_device->lpDD2));
  if (err != DD_OK)
  {
    gfxDrvDDrawFailure("gfxDrvDDraw2ObjectInitialize(): ", err);
    return false;
  }
  else
  {
    DDCAPS caps;
    HRESULT res;
    memset(&caps, 0, sizeof(caps));
    caps.dwSize = sizeof(caps);
    res = IDirectDraw_GetCaps(ddraw_device->lpDD2, &caps, NULL);
    if (res != DD_OK)
    {
      gfxDrvDDrawFailure("GetCaps()", res);
    }
    else
    {
      ddraw_device->can_stretch_y = (caps.dwFXCaps & DDFXCAPS_BLTARITHSTRETCHY) ||
        (caps.dwFXCaps & DDFXCAPS_BLTARITHSTRETCHYN) ||
        (caps.dwFXCaps & DDFXCAPS_BLTSTRETCHY) ||
        (caps.dwFXCaps & DDFXCAPS_BLTSHRINKYN);
      if (!ddraw_device->can_stretch_y)
      {
        fellowAddLog("gfxdrv: WARNING: No hardware stretch\n");
      }
      ddraw_device->no_dd_hardware = !!(caps.dwCaps & DDCAPS_NOHARDWARE);
      if (ddraw_device->no_dd_hardware)
      {
        fellowAddLog("gfxdrv: WARNING: No DirectDraw hardware\n");
      }
    }
  }
  return true;
}


/*==========================================================================*/
/* Releases the DirectDraw1 object for the given DirectDraw device          */
/* Called on emulator startup                                               */
/*==========================================================================*/

bool gfxDrvDDraw1ObjectRelease(gfx_drv_ddraw_device *ddraw_device)
{
  HRESULT err = DD_OK;

  if (ddraw_device->lpDD != NULL)
  {
    err = IDirectDraw_Release(ddraw_device->lpDD);
    if (err != DD_OK)
    {
      gfxDrvDDrawFailure("gfxDrvDDraw1ObjectRelease(): ", err);
    }
    ddraw_device->lpDD = NULL;
  }
  return (err == DD_OK);
}


/*==========================================================================*/
/* Releases the DirectDraw2 object for the given DirectDraw device          */
/* Called on emulator shutdown                                              */
/*==========================================================================*/

bool gfxDrvDDraw2ObjectRelease(gfx_drv_ddraw_device *ddraw_device)
{
  HRESULT err = DD_OK;

  if (ddraw_device->lpDD2 != NULL)
  {
    err = IDirectDraw2_Release(ddraw_device->lpDD2);
    if (err != DD_OK)
    {
      gfxDrvDDrawFailure("gfxDrvDDraw2ObjectRelease(): ", err);
    }
    ddraw_device->lpDD2 = NULL;
  }
  return (err == DD_OK);
}


/*==========================================================================*/
/* Creates the DirectDraw object for the selected DirectDraw device         */
/* Called on emulator startup                                               */
/* Returns success or not                                                   */
/*==========================================================================*/

bool gfxDrvDDrawObjectInitialize(gfx_drv_ddraw_device *ddraw_device)
{
  bool result = gfxDrvDDraw1ObjectInitialize(ddraw_device);

  if (result)
  {
    result = gfxDrvDDraw2ObjectInitialize(ddraw_device);
    gfxDrvDDraw1ObjectRelease(ddraw_device);
  }
  return result;
}


/*==========================================================================*/
/* Returns mode information about described mode                            */
/*==========================================================================*/

gfx_drv_ddraw_mode *gfxDrvDDrawModeFind(gfx_drv_ddraw_device *ddraw_device,
  ULO width,
  ULO height,
  ULO depth)
{
  felist *l;

  for (l = ddraw_device->modes; l != NULL; l = listNext(l))
  {
    gfx_drv_ddraw_mode *tmpmode = (gfx_drv_ddraw_mode *)listNode(l);
    if (tmpmode->width == width &&
      tmpmode->height == height &&
      tmpmode->depth == depth)
    {
      return tmpmode;
    }
  }
  return NULL;
}


/*==========================================================================*/
/* Describes all the found modes to the draw module                         */
/* Called on emulator startup                                               */
/*==========================================================================*/

void gfxDrvDDrawModeInformationRegister(gfx_drv_ddraw_device *ddraw_device)
{
  felist *l;
  ULO id;

  id = 0;
  for (l = ddraw_device->modes; l != NULL; l = listNext(l))
  {
    gfx_drv_ddraw_mode *ddmode = (gfx_drv_ddraw_mode *)listNode(l);
    draw_mode *mode = (draw_mode *)malloc(sizeof(draw_mode));

    mode->width = ddmode->width;
    mode->height = ddmode->height;
    mode->bits = ddmode->depth;
    mode->windowed = ddmode->windowed;
    mode->refresh = ddmode->refresh;
    mode->redpos = ddmode->redpos;
    mode->redsize = ddmode->redsize;
    mode->greenpos = ddmode->greenpos;
    mode->greensize = ddmode->greensize;
    mode->bluepos = ddmode->bluepos;
    mode->bluesize = ddmode->bluesize;
    mode->pitch = ddmode->pitch;
    mode->id = id;
    if (!ddmode->windowed)
    {
      char hz[16];
      if (ddmode->refresh != 0)
      {
        sprintf(hz, "%uHZ", ddmode->refresh);
      }
      else
      {
        hz[0] = 0;
      }
      sprintf(mode->name, "%uWx%uHx%uBPPx%s", ddmode->width, ddmode->height, ddmode->depth, hz);
    }
    else
    {
      sprintf(mode->name, "%uWx%uHxWindow", ddmode->width, ddmode->height);
    }
    drawModeAdd(mode);
    id++;
  }
}


/*==========================================================================*/
/* Examines RGB masks for the mode information initializer                  */
/* Called on emulator startup                                               */
/*==========================================================================*/

ULO gfxDrvRGBMaskPos(ULO mask)
{
  ULO i;

  for (i = 0; i < 32; i++)
  {
    if (mask & (1 << i))
    {
      return i;
    }
  }
  return 0;
}

ULO gfxDrvRGBMaskSize(ULO mask)
{
  ULO i, sz;

  sz = 0;
  for (i = 0; i < 32; i++)
  {
    if (mask & (1 << i))
    {
      sz++;
    }
  }
  return sz;
}

/*==========================================================================*/
/* Dump information about available modes                                   */
/*==========================================================================*/

void gfxDrvDDrawModeInformationDump(gfx_drv_ddraw_device *ddraw_device)
{
  felist *l;
  STR s[120];

  sprintf(s, "gfxdrv: DirectDraw modes found: %u\n", listCount(ddraw_device->modes));
  fellowAddLog(s);
  for (l = ddraw_device->modes; l != NULL; l = listNext(l))
  {
    gfx_drv_ddraw_mode *tmpmode = (gfx_drv_ddraw_mode *)listNode(l);
    if (!tmpmode->windowed)
    {
      sprintf(s,
        "gfxdrv: Mode Description: %uWx%uHx%uBPPx%uHZ (%u,%u,%u,%u,%u,%u)\n",
        tmpmode->width,
        tmpmode->height,
        tmpmode->depth,
        tmpmode->refresh,
        tmpmode->redpos,
        tmpmode->redsize,
        tmpmode->greenpos,
        tmpmode->greensize,
        tmpmode->bluepos,
        tmpmode->bluesize);
    }
    else
    {
      sprintf(s, "gfxdrv: Mode Description: %uWx%uHxWindow\n", tmpmode->width, tmpmode->height);
    }
    fellowAddLog(s);
  }
}

/*==========================================================================*/
/* Makes a new mode information struct and fills in the provided params     */
/* Called on emulator startup                                               */
/*==========================================================================*/

gfx_drv_ddraw_mode *gfxDrvDDrawModeNew(ULO width,
  ULO height,
  ULO depth,
  ULO refresh,
  ULO redpos,
  ULO redsize,
  ULO greenpos,
  ULO greensize,
  ULO bluepos,
  ULO bluesize,
  bool windowed)
{
  gfx_drv_ddraw_mode *tmpmode = (gfx_drv_ddraw_mode *)malloc(sizeof(gfx_drv_ddraw_mode));
  tmpmode->width = width;
  tmpmode->height = height;
  tmpmode->depth = depth;
  tmpmode->refresh = refresh;
  tmpmode->redpos = redpos;
  tmpmode->redsize = redsize;
  tmpmode->greenpos = greenpos;
  tmpmode->greensize = greensize;
  tmpmode->bluepos = bluepos;
  tmpmode->bluesize = bluesize;
  tmpmode->windowed = windowed;
  return tmpmode;
}


/*==========================================================================*/
/* Callback used when creating the list of available modes                  */
/* Called on emulator startup                                               */
/*==========================================================================*/

HRESULT WINAPI gfxDrvDDrawModeEnumerate(LPDDSURFACEDESC lpDDSurfaceDesc,
  LPVOID lpContext)
{
  winDrvSetThreadName(-1, "gfxDrvDDrawModeEnumerate()");
  if (((lpDDSurfaceDesc->ddpfPixelFormat.dwFlags == DDPF_RGB) &&
    ((lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount == 16) ||
      (lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount == 24) ||
      (lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount == 32))))
  {
    if (lpDDSurfaceDesc->dwRefreshRate > 1 && lpDDSurfaceDesc->dwRefreshRate < 50)
    {
      fellowAddLog("gfxDrvDDrawModeEnumerate(): ignoring mode %ux%u, %u bit, %u Hz\n",
        lpDDSurfaceDesc->dwWidth,
        lpDDSurfaceDesc->dwHeight,
        lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount,
        lpDDSurfaceDesc->dwRefreshRate);

      return DDENUMRET_OK;
    }

    gfx_drv_ddraw_mode *tmpmode;
    gfx_drv_ddraw_device *ddraw_device;

    ddraw_device = (gfx_drv_ddraw_device *)lpContext;
    tmpmode = gfxDrvDDrawModeNew(lpDDSurfaceDesc->dwWidth,
      lpDDSurfaceDesc->dwHeight,
      lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount,
      lpDDSurfaceDesc->dwRefreshRate,
      (lpDDSurfaceDesc->ddpfPixelFormat.dwFlags == DDPF_RGB) ? gfxDrvRGBMaskPos(lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask) : 0,
      (lpDDSurfaceDesc->ddpfPixelFormat.dwFlags == DDPF_RGB) ? gfxDrvRGBMaskSize(lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask) : 0,
      (lpDDSurfaceDesc->ddpfPixelFormat.dwFlags == DDPF_RGB) ? gfxDrvRGBMaskPos(lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask) : 0,
      (lpDDSurfaceDesc->ddpfPixelFormat.dwFlags == DDPF_RGB) ? gfxDrvRGBMaskSize(lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask) : 0,
      (lpDDSurfaceDesc->ddpfPixelFormat.dwFlags == DDPF_RGB) ? gfxDrvRGBMaskPos(lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask) : 0,
      (lpDDSurfaceDesc->ddpfPixelFormat.dwFlags == DDPF_RGB) ? gfxDrvRGBMaskSize(lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask) : 0,
      FALSE);
    ddraw_device->modes = listAddLast(ddraw_device->modes, listNew(tmpmode));
  }
  return DDENUMRET_OK;
}


/*==========================================================================*/
/* Creates a list of available modes on a given DirectDraw device           */
/* Called on emulator startup                                               */
/*==========================================================================*/

bool gfxDrvDDrawModeInformationInitialize(gfx_drv_ddraw_device *ddraw_device)
{
  bool result;

  ddraw_device->modes = NULL;
  HRESULT err = IDirectDraw2_EnumDisplayModes(ddraw_device->lpDD2,
    DDEDM_REFRESHRATES,
    NULL,
    (LPVOID)ddraw_device,
    gfxDrvDDrawModeEnumerate);
  if (err != DD_OK)
  {
    gfxDrvDDrawFailure("gfxDrvDDrawModeInformationInitialize(): ", err);
    result = false;
  }
  else
  {
    result = listCount(ddraw_device->modes) != 0;

    if (!result)
    {
      fellowAddLog("gfxdrv: no valid draw modes found, retry while ignoring refresh rates...\n");
      err = IDirectDraw2_EnumDisplayModes(ddraw_device->lpDD2,
        NULL,
        NULL,
        (LPVOID)ddraw_device,
        gfxDrvDDrawModeEnumerate);

      result = listCount(ddraw_device->modes) != 0;
    }

    if (result)
    {
      #define GFXWIDTH_NORMAL 640
      #define GFXWIDTH_OVERSCAN 752
      #define GFXWIDTH_MAXOVERSCAN 768

      #define GFXHEIGHT_NTSC 400
      #define GFXHEIGHT_PAL 512
      #define GFXHEIGHT_OVERSCAN 576

      // 1X
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(GFXWIDTH_NORMAL, GFXHEIGHT_NTSC, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(GFXWIDTH_NORMAL, GFXHEIGHT_PAL, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(GFXWIDTH_NORMAL, GFXHEIGHT_OVERSCAN, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(GFXWIDTH_OVERSCAN, GFXHEIGHT_NTSC, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(GFXWIDTH_OVERSCAN, GFXHEIGHT_PAL, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(GFXWIDTH_OVERSCAN, GFXHEIGHT_OVERSCAN, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(GFXWIDTH_MAXOVERSCAN, GFXHEIGHT_OVERSCAN, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));

      // 2X
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(2 * GFXWIDTH_NORMAL, 2 * GFXHEIGHT_NTSC, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(2 * GFXWIDTH_NORMAL, 2 * GFXHEIGHT_PAL, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(2 * GFXWIDTH_NORMAL, 2 * GFXHEIGHT_OVERSCAN, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(2 * GFXWIDTH_OVERSCAN, 2 * GFXHEIGHT_NTSC, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(2 * GFXWIDTH_OVERSCAN, 2 * GFXHEIGHT_PAL, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(2 * GFXWIDTH_OVERSCAN, 2 * GFXHEIGHT_OVERSCAN, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(2 * GFXWIDTH_MAXOVERSCAN, 2 * GFXHEIGHT_OVERSCAN, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));

      // 3X
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(3 * GFXWIDTH_NORMAL, 3 * GFXHEIGHT_NTSC, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(3 * GFXWIDTH_NORMAL, 3 * GFXHEIGHT_PAL, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(3 * GFXWIDTH_NORMAL, 3 * GFXHEIGHT_OVERSCAN, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(3 * GFXWIDTH_OVERSCAN, 3 * GFXHEIGHT_NTSC, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(3 * GFXWIDTH_OVERSCAN, 3 * GFXHEIGHT_PAL, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(3 * GFXWIDTH_OVERSCAN, 3 * GFXHEIGHT_OVERSCAN, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(3 * GFXWIDTH_MAXOVERSCAN, 3 * GFXHEIGHT_OVERSCAN, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));

      // 4X
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(4 * GFXWIDTH_NORMAL, 4 * GFXHEIGHT_NTSC, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(4 * GFXWIDTH_NORMAL, 4 * GFXHEIGHT_PAL, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(4 * GFXWIDTH_NORMAL, 4 * GFXHEIGHT_OVERSCAN, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(4 * GFXWIDTH_OVERSCAN, 4 * GFXHEIGHT_NTSC, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(4 * GFXWIDTH_OVERSCAN, 4 * GFXHEIGHT_PAL, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(4 * GFXWIDTH_OVERSCAN, 4 * GFXHEIGHT_OVERSCAN, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(4 * GFXWIDTH_MAXOVERSCAN, 4 * GFXHEIGHT_OVERSCAN, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));

      gfxDrvDDrawModeInformationRegister(ddraw_device);
    }
  }
  gfxDrvDDrawModeInformationDump(ddraw_device);
  return result;
}


/*==========================================================================*/
/* Releases the list of available modes                                     */
/* Called on emulator shutdown                                              */
/*==========================================================================*/

void gfxDrvDDrawModeInformationRelease(gfx_drv_ddraw_device *ddraw_device)
{
  listFreeAll(ddraw_device->modes, TRUE);
  ddraw_device->modes = NULL;
}


/*==========================================================================*/
/* Set cooperative level                                                    */
/*==========================================================================*/

bool gfxDrvDDrawSetCooperativeLevelNormal(gfx_drv_ddraw_device *ddraw_device)
{
  HRESULT err = IDirectDraw2_SetCooperativeLevel(ddraw_device->lpDD2, gfxDrvCommon->GetHWND(), DDSCL_NORMAL);
  if (err != DD_OK)
  {
    gfxDrvDDrawFailure("gfxDrvDDrawSetCooperativeLevelNormal(): ", err);
  }
  return (err == DD_OK);
}


bool gfxDrvDDrawSetCooperativeLevelExclusive(gfx_drv_ddraw_device *ddraw_device)
{
  HRESULT err = IDirectDraw2_SetCooperativeLevel(ddraw_device->lpDD2, gfxDrvCommon->GetHWND(), DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE);
  if (err != DD_OK)
  {
    gfxDrvDDrawFailure("gfxDrvDDrawSetCooperativeLevelExclusive(): ", err);
  }
  return (err == DD_OK);
}


bool gfxDrvDDrawSetCooperativeLevel(gfx_drv_ddraw_device *ddraw_device)
{
  if (ddraw_device->mode->windowed)
  {
    return gfxDrvDDrawSetCooperativeLevelNormal(ddraw_device);
  }
  return gfxDrvDDrawSetCooperativeLevelExclusive(ddraw_device);
}


/*==========================================================================*/
/* Clear surface with blitter                                               */
/*==========================================================================*/

void gfxDrvDDrawSurfaceClear(LPDIRECTDRAWSURFACE surface)
{
  DDBLTFX ddbltfx;

  memset(&ddbltfx, 0, sizeof(ddbltfx));
  ddbltfx.dwSize = sizeof(ddbltfx);
  ddbltfx.dwFillColor = 0;
  HRESULT err = IDirectDrawSurface_Blt(surface,
    NULL,
    NULL,
    NULL,
    DDBLT_COLORFILL | DDBLT_WAIT,
    &ddbltfx);
  if (err != DD_OK)
  {
    gfxDrvDDrawFailure("gfxDrvDDrawSurfaceClear(): ", err);
  }
  fellowAddLog("gfxdrv: Clearing surface\n");
}


/*==========================================================================*/
/* Restore a surface, do some additional cleaning up.                       */
/*==========================================================================*/

HRESULT gfxDrvDDrawSurfaceRestore(gfx_drv_ddraw_device *ddraw_device, LPDIRECTDRAWSURFACE surface)
{
  if (IDirectDrawSurface_IsLost(surface) != DDERR_SURFACELOST)
  {
    return DD_OK;
  }
  HRESULT err = IDirectDrawSurface_Restore(surface);
  if (err == DD_OK)
  {
    gfxDrvDDrawSurfaceClear(surface);
    if ((surface == ddraw_device->lpDDSPrimary) && (ddraw_device->buffercount > 1))
    {
      gfxDrvDDrawSurfaceClear(ddraw_device->lpDDSBack);
      if (ddraw_device->buffercount == 3)
      {
        err = IDirectDrawSurface_Flip(surface, NULL, DDFLIP_WAIT);
        if (err != DD_OK)
        {
          gfxDrvDDrawFailure("gfxDrvDDrawSurfaceRestore(), Flip(): ", err);
        }
      }
      else
      {
        gfxDrvDDrawSurfaceClear(ddraw_device->lpDDSBack);
      }
    }
    graphLineDescClear();
  }
  return err;
}

/*==========================================================================*/
/* Blit secondary buffer to the primary surface                             */
/*==========================================================================*/

void gfxDrvDDrawSurfaceBlit(gfx_drv_ddraw_device *ddraw_device)
{
  HRESULT err;
  RECT srcwin;
  RECT dstwin;
  LPDIRECTDRAWSURFACE lpDDSDestination;
  DDBLTFX bltfx;

  memset(&dstwin, 0, sizeof(RECT));

  memset(&bltfx, 0, sizeof(DDBLTFX));
  bltfx.dwSize = sizeof(DDBLTFX);

  /* When using the blitter to show our buffer, */
  /* source surface is always the secondary off-screen buffer */

  /* Srcwin is used when we do vertical scaling */
  /* Prevent horizontal scaling of the offscreen buffer */
  if (ddraw_device->mode->windowed)
  {
#ifdef RETRO_PLATFORM
    if (!RetroPlatformGetMode())
#endif
    {
      srcwin.left = 0;
      srcwin.right = ddraw_device->mode->width;
      srcwin.top = 0;
      srcwin.bottom = ddraw_device->mode->height;
    }
#ifdef RETRO_PLATFORM
    else
    {
      srcwin.left   = RetroPlatformGetClippingOffsetLeftAdjusted();
      srcwin.right  = RetroPlatformGetScreenWidthAdjusted() + RetroPlatformGetClippingOffsetLeftAdjusted();
      srcwin.top    = RetroPlatformGetClippingOffsetTopAdjusted();
      srcwin.bottom = RetroPlatformGetScreenHeightAdjusted() + RetroPlatformGetClippingOffsetTopAdjusted();
    }
#endif
  }
  else
  {
    srcwin.left = draw_hoffset;
    srcwin.right = draw_hoffset + draw_width_amiga_real;
    srcwin.top = draw_voffset;
    srcwin.bottom = draw_voffset + draw_height_amiga_real;
  }

  /* Destination is always the primary or one the backbuffers attached to it */
  gfxDrvDDrawBlitTargetSurfaceSelect(ddraw_device, &lpDDSDestination);
  /* Destination window, in windowed mode, use the clientrect */
  if (!ddraw_device->mode->windowed)
  {
    /* In full-screen mode, blit centered to the screen */
    dstwin.left = draw_hoffset;
    dstwin.top = draw_voffset;
    dstwin.right = draw_hoffset + draw_width_amiga_real;
    dstwin.bottom = draw_voffset + draw_height_amiga_real;
  }

  RECT *srcrect = NULL;

#ifdef RETRO_PLATFORM
  if (RetroPlatformGetMode())
    srcrect = &srcwin;
#endif

  /* This can fail when a surface is lost */
  err = IDirectDrawSurface_Blt(lpDDSDestination,
    (ddraw_device->mode->windowed) ? &ddraw_device->hwnd_clientrect_screen : &dstwin, ddraw_device->lpDDSSecondary,
    srcrect, DDBLT_ASYNC, &bltfx);
  if (err != DD_OK)
  {
    gfxDrvDDrawFailure("gfxDrvDDrawSurfaceBlit(): 0x%x.\n", err);
    if (err == DDERR_SURFACELOST)
    {
      /* Reclaim surface, if that also fails, pass over the blit and hope it */
      /* gets better later */
      err = gfxDrvDDrawSurfaceRestore(ddraw_device, ddraw_device->lpDDSPrimary);
      if (err != DD_OK)
      {
        /* Here we are in deep trouble, we can not provide a buffer pointer */
        gfxDrvDDrawFailure("gfxDrvDDrawSurfaceBlit(): (Restore primary surface failed) ", err);
        return;
      }
      err = gfxDrvDDrawSurfaceRestore(ddraw_device, ddraw_device->lpDDSSecondary);
      if (err != DD_OK)
      {
        /* Here we are in deep trouble, we can not provide a buffer pointer */
        gfxDrvDDrawFailure("gfxDrvDDrawSurfaceBlit(): (Restore secondary surface failed) ", err);
        return;
      }
      /* Restore was successful, do the blit */
      err = IDirectDrawSurface_Blt(lpDDSDestination,
        (ddraw_device->mode->windowed) ? &ddraw_device->hwnd_clientrect_screen : &dstwin, ddraw_device->lpDDSSecondary,
        srcrect, DDBLT_ASYNC, &bltfx);
      if (err != DD_OK)
      {
        /* failed second time, pass over */
        gfxDrvDDrawFailure("gfxDrvDDrawSurfaceBlit(): (Blit failed after restore) ", err);
      }
    }
  }
}


/*==========================================================================*/
/* Release the surfaces                                                     */
/*==========================================================================*/

void gfxDrvDDrawSurfacesRelease(gfx_drv_ddraw_device *ddraw_device)
{
  HRESULT err;

  if (ddraw_device->lpDDSPrimary != NULL)
  {
    err = IDirectDrawSurface_Release(ddraw_device->lpDDSPrimary);
    if (err != DD_OK)
    {
      gfxDrvDDrawFailure("gfxDrvDDrawSurfacesRelease(): ", err);
    }
    ddraw_device->lpDDSPrimary = NULL;
    if (ddraw_device->mode->windowed)
    {
      gfxDrvDDrawClipperRelease(ddraw_device);
    }
  }
  ddraw_device->buffercount = 0;
  if (ddraw_device->lpDDSSecondary != NULL)
  {
    err = IDirectDrawSurface_Release(ddraw_device->lpDDSSecondary);
    if (err != DD_OK)
    {
      gfxDrvDDrawFailure("gfxDrvDDrawSurfacesRelease(): ", err);
    }
    ddraw_device->lpDDSSecondary = NULL;
  }
}

/*==========================================================================*/
/* Create a second offscreen buffer                                         */
/* A second buffer is needed when the blitter is used for displaying data   */
/* It is acceptable for this buffer to be a software buffer, but normally   */
/* it will drain performance, anywhere from 1/2 the framerate down to a     */
/* near full stop depending on the card and driver.                         */
/*==========================================================================*/

char *gfxDrvDDrawVideomemLocationStr(ULO pass)
{
  switch (pass)
  {
  case 0: return "local videomemory (on card)";
  case 1: return "non-local videomemory (AGP shared mem)";
  case 2: return "system memory";
  }
  return "unknown memory";
}

ULO gfxDrvDDrawVideomemLocationFlags(ULO pass)
{
  switch (pass)
  {
  case 0: return DDSCAPS_VIDEOMEMORY; /* Local videomemory (default oncard) */
  case 1: return DDSCAPS_NONLOCALVIDMEM | DDSCAPS_VIDEOMEMORY; /* Shared videomemory */
  case 2: return DDSCAPS_SYSTEMMEMORY; /* Plain memory */
  }
  return 0;
}

bool gfxDrvDDrawCreateSecondaryOffscreenSurface(gfx_drv_ddraw_device *ddraw_device)
{
  ULO pass;
  bool buffer_allocated;
  bool result = true;
  HRESULT err;

  pass = 0;
  buffer_allocated = false;
  while ((pass < 3) && !buffer_allocated)
  {
    ddraw_device->ddsdSecondary.dwSize = sizeof(ddraw_device->ddsdSecondary);
    ddraw_device->ddsdSecondary.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddraw_device->ddsdSecondary.ddsCaps.dwCaps = gfxDrvDDrawVideomemLocationFlags(pass)
      | DDSCAPS_OFFSCREENPLAIN;

#ifdef RETRO_PLATFORM
    if (!RetroPlatformGetMode())
    {
#endif
      ddraw_device->ddsdSecondary.dwHeight = ddraw_device->drawmode->height;
      ddraw_device->ddsdSecondary.dwWidth = ddraw_device->drawmode->width;
#ifdef RETRO_PLATFORM
    }
    else {
      // increase buffer size to accomodate different display scaling factors
      ddraw_device->ddsdSecondary.dwHeight = RETRO_PLATFORM_MAX_PAL_LORES_HEIGHT * RetroPlatformGetDisplayScale() * 2;
      ddraw_device->ddsdSecondary.dwWidth  = RETRO_PLATFORM_MAX_PAL_LORES_WIDTH  * RetroPlatformGetDisplayScale() * 2;
    }
#endif
    err = IDirectDraw2_CreateSurface(ddraw_device->lpDD2,
      &(ddraw_device->ddsdSecondary),
      &(ddraw_device->lpDDSSecondary),
      NULL);
    if (err != DD_OK)
    {
      gfxDrvDDrawFailure("gfxDrvDDrawCreateSecondaryOffscreenSurface() 0x%x\n", err);
      fellowAddLog("gfxdrv: Failed to allocate second offscreen surface in %s\n",
        gfxDrvDDrawVideomemLocationStr(pass));
      result = false;
    }
    else
    {
      buffer_allocated = true;
      fellowAddLog("gfxdrv: Allocated second offscreen surface in %s (%d, %d)\n",
        gfxDrvDDrawVideomemLocationStr(pass),
        ddraw_device->drawmode->width,
        ddraw_device->drawmode->height);
      gfxDrvDDrawSurfaceClear(ddraw_device->lpDDSSecondary);
      result = true;
    }
    pass++;
  }
  return result && buffer_allocated;
}

/*==========================================================================*/
/* Create the surfaces                                                      */
/* Called before emulation start                                            */
/*==========================================================================*/

ULO gfxDrvDDrawSurfacesInitialize(gfx_drv_ddraw_device *ddraw_device)
{
  HRESULT err;
  ULO buffer_count_want;
  DDSCAPS ddscaps;
  bool success = true;

  if (ddraw_device->use_blitter)
  {
    success = gfxDrvDDrawCreateSecondaryOffscreenSurface(ddraw_device);
  }
  if (!success)
  {
    return 0;
  }

  /* Want x backbuffers first, then reduce it until it succeeds */
  buffer_count_want = (ddraw_device->use_blitter) ? 1 : ddraw_device->maxbuffercount;
  success = false;
  while (buffer_count_want > 0 && !success)
  {
    success = true;
    if (ddraw_device->lpDDSPrimary != NULL)
    {
      gfxDrvDDrawSurfacesRelease(ddraw_device);
    }
    ddraw_device->ddsdPrimary.dwSize = sizeof(ddraw_device->ddsdPrimary);
    ddraw_device->ddsdPrimary.dwFlags = DDSD_CAPS;
    ddraw_device->ddsdPrimary.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    if (buffer_count_want > 1)
    {
      ddraw_device->ddsdPrimary.dwFlags |= DDSD_BACKBUFFERCOUNT;
      ddraw_device->ddsdPrimary.ddsCaps.dwCaps |= DDSCAPS_FLIP |
        DDSCAPS_COMPLEX;
      ddraw_device->ddsdPrimary.dwBackBufferCount = buffer_count_want - 1;
      ddraw_device->ddsdBack.dwSize = sizeof(ddraw_device->ddsdBack);
    }
    err = IDirectDraw2_CreateSurface(ddraw_device->lpDD2,
      &(ddraw_device->ddsdPrimary),
      &(ddraw_device->lpDDSPrimary),
      NULL);
    if (err != DD_OK)
    {
      gfxDrvDDrawFailure("gfxDrvDDrawSurfacesInitialize(): ", err);
      fellowAddLog("gfxdrv: Failed to allocate primary surface with %d backbuffers\n", buffer_count_want - 1);
      success = false;
    }
    else
    { /* Here we have got a buffer, clear it */
      fellowAddLog("gfxdrv: Allocated primary surface with %d backbuffers\n", buffer_count_want - 1);
      if (buffer_count_want > 1)
      {
        ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
        err = IDirectDrawSurface_GetAttachedSurface(ddraw_device->lpDDSPrimary,
          &ddscaps,
          &(ddraw_device->lpDDSBack));
        if (err != DD_OK)
        {
          gfxDrvDDrawFailure("gfxDrvDDrawSurfacesInitialize(): (GetAttachedSurface()) ", err);
          success = false;
        }
        else
        { /* Have a backbuffer, seems to work fine */
          gfxDrvDDrawSurfaceClear(ddraw_device->lpDDSPrimary);
          gfxDrvDDrawSurfaceClear(ddraw_device->lpDDSBack);
          if (buffer_count_want > 2)
          {
            err = IDirectDrawSurface_Flip(ddraw_device->lpDDSPrimary,
              NULL,
              DDFLIP_WAIT);
            if (err != DD_OK)
            {
              gfxDrvDDrawFailure("gfxDrvDDrawSurfacesInitialize(), Flip(): ", err);
              success = false;
            }
            else
            {
              gfxDrvDDrawSurfaceClear(ddraw_device->lpDDSBack);
            }
          }
        }
      }
      else
      { /* No backbuffer, don't clear if we are in a window */
        if (!ddraw_device->mode->windowed)
        {
          gfxDrvDDrawSurfaceClear(ddraw_device->lpDDSPrimary);
        }
      }
    }
    if (!success)
    {
      buffer_count_want--;
    }
  }

  /* Here we have created a primary buffer, possibly with backbuffers */

  if (success)
  {
    DDPIXELFORMAT ddpf;
    memset(&ddpf, 0, sizeof(ddpf));
    ddpf.dwSize = sizeof(ddpf);
    err = IDirectDrawSurface_GetPixelFormat(ddraw_device->lpDDSPrimary, &ddpf);
    if (err != DD_OK)
    {
      /* We failed to find the pixel format */
      gfxDrvDDrawFailure("gfxDrvDDrawSurfacesInitialize(): GetPixelFormat() ", err);
      gfxDrvDDrawSurfacesRelease(ddraw_device);
      success = FALSE;
    }
    else
    {
      /* Examine pixelformat */
      if (((ddpf.dwFlags == DDPF_RGB) &&
        ((ddpf.dwRGBBitCount == 16) ||
          (ddpf.dwRGBBitCount == 24) ||
          (ddpf.dwRGBBitCount == 32))) ||
        (ddpf.dwFlags == (DDPF_PALETTEINDEXED8 | DDPF_RGB)))
      {
        ddraw_device->drawmode->bits = ddpf.dwRGBBitCount;
        ddraw_device->drawmode->redpos = (ddpf.dwFlags == DDPF_RGB) ? gfxDrvRGBMaskPos(ddpf.dwRBitMask) : 0;
        ddraw_device->drawmode->redsize = (ddpf.dwFlags == DDPF_RGB) ? gfxDrvRGBMaskSize(ddpf.dwRBitMask) : 0;
        ddraw_device->drawmode->greenpos = (ddpf.dwFlags == DDPF_RGB) ? gfxDrvRGBMaskPos(ddpf.dwGBitMask) : 0;
        ddraw_device->drawmode->greensize = (ddpf.dwFlags == DDPF_RGB) ? gfxDrvRGBMaskSize(ddpf.dwGBitMask) : 0;
        ddraw_device->drawmode->bluepos = (ddpf.dwFlags == DDPF_RGB) ? gfxDrvRGBMaskPos(ddpf.dwBBitMask) : 0;
        ddraw_device->drawmode->bluesize = (ddpf.dwFlags == DDPF_RGB) ? gfxDrvRGBMaskSize(ddpf.dwBBitMask) : 0;

        /* Set clipper */

        if (success && (ddraw_device->mode->windowed))
        {
          if (!gfxDrvDDrawClipperInitialize(ddraw_device))
          {
            gfxDrvDDrawSurfacesRelease(ddraw_device);
            success = false;
          }
        }
      }
      else
      { /* Unsupported pixel format..... */
        fellowAddLog("gfxdrv: gfxDrvDDrawSurfacesInitialized(): Window mode - unsupported Pixelformat\n");
        gfxDrvDDrawSurfacesRelease(ddraw_device);
        success = false;
      }
    }
  }
  ddraw_device->buffercount = (success) ? buffer_count_want : 0;
  return ddraw_device->buffercount;
}

/*==========================================================================*/
/* Lock the current drawing surface                                         */
/*==========================================================================*/

UBY *gfxDrvDDrawSurfaceLock(gfx_drv_ddraw_device *ddraw_device, ULO *pitch)
{
  LPDIRECTDRAWSURFACE lpDDS;
  LPDDSURFACEDESC lpDDSD;

  gfxDrvDDrawDrawTargetSurfaceSelect(ddraw_device, &lpDDS, &lpDDSD);
  HRESULT err = IDirectDrawSurface_Lock(lpDDS, NULL, lpDDSD, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);
  /* We sometimes have a pointer to a backbuffer, but only primary surfaces can
  be restored, it implicitly recreates backbuffers as well */
  if (err != DD_OK)
  {
    gfxDrvDDrawFailure("gfxDrvDDrawSurfaceLock(): ", err);
    if (err == DDERR_SURFACELOST)
    {
      /* Here we have lost the surface, restore it */
      /* Unlike when blitting, we only use 1 surface here */
      err = gfxDrvDDrawSurfaceRestore(ddraw_device,
        (ddraw_device->mode->windowed) ? lpDDS : ddraw_device->lpDDSPrimary);
      if (err != DD_OK)
      {
        /* Here we are in deep trouble, we can not provide a buffer pointer */
        gfxDrvDDrawFailure("gfxDrvDDrawSurfaceLock(): (Failed to restore surface 1) ", err);
        return NULL;
      }
      else
      {
        /* Try again to lock the surface */
        err = IDirectDrawSurface_Lock(lpDDS, NULL, lpDDSD, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);
        if (err != DD_OK)
        {
          /* Here we are in deep trouble, we can not provide a buffer pointer */
          gfxDrvDDrawFailure("gfxDrvDDrawSurfaceLock(): (Lock failed after restore) ", err);
          return NULL;
        }
      }
    }
    else
    {
      /* Here we are in deep trouble, we can not provide a buffer pointer */
      /* The error is something else than lost surface */
      fellowAddLog("gfxdrv: gfxDrvDDrawSurfaceLock(): (Mega problem 3)\n");
      return NULL;
    }
  }
  *pitch = lpDDSD->lPitch;
  return (UBY*)lpDDSD->lpSurface;
}


/*==========================================================================*/
/* Unlock the primary surface                                               */
/*==========================================================================*/

void gfxDrvDDrawSurfaceUnlock(gfx_drv_ddraw_device *ddraw_device)
{
  LPDIRECTDRAWSURFACE lpDDS;
  LPDDSURFACEDESC lpDDSD;

  gfxDrvDDrawDrawTargetSurfaceSelect(ddraw_device, &lpDDS, &lpDDSD);
  HRESULT err = IDirectDrawSurface_Unlock(lpDDS, lpDDSD->lpSurface);
  if (err != DD_OK)
  {
    gfxDrvDDrawFailure("gfxDrvSurfaceUnlock(): ", err);
  }
}


/*==========================================================================*/
/* Flip to next buffer                                                      */
/*==========================================================================*/

void gfxDrvDDrawFlip()
{
  if (gfx_drv_ddraw_device_current->use_blitter)     /* Blit secondary buffer to primary */
  {
    gfxDrvDDrawSurfaceBlit(gfx_drv_ddraw_device_current);
  }
  if (gfx_drv_ddraw_device_current->buffercount > 1)    /* Flip buffer if there are several */
  {
    HRESULT err = IDirectDrawSurface_Flip(gfx_drv_ddraw_device_current->lpDDSPrimary, NULL, DDFLIP_WAIT);
    if (err != DD_OK)
    {
      gfxDrvDDrawFailure("gfxDrvDDrawFlip(): ", err);
    }
  }
}


/*==========================================================================*/
/* Set specified mode                                                       */
/* In windowed mode, we don't set a mode, but instead queries the pixel-    */
/* format from the surface (gfxDrvDDrawSurfacesInitialize())                */
/* Called before emulation start                                            */
/*==========================================================================*/

unsigned int gfxDrvDDrawSetMode(gfx_drv_ddraw_device *ddraw_device)
{
  bool result = true;
  unsigned int buffers = 0;

  if (gfxDrvDDrawSetCooperativeLevel(ddraw_device))
  {
    ddraw_device->use_blitter =
      ddraw_device->mode->windowed
      || ddraw_device->no_dd_hardware;

    if (!ddraw_device->mode->windowed)
    {
      gfx_drv_ddraw_mode *mode;
      HRESULT err;
      DDSURFACEDESC myDDSDesc;

      mode = (gfx_drv_ddraw_mode *)listNode(listIndex(ddraw_device->modes, ddraw_device->drawmode->id));
      memset(&myDDSDesc, 0, sizeof(myDDSDesc));
      myDDSDesc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
      err = IDirectDraw2_SetDisplayMode(ddraw_device->lpDD2, mode->width, mode->height, mode->depth, mode->refresh, 0);
      if (err != DD_OK)
      {
        gfxDrvDDrawFailure("gfxDrvDDrawSetMode(): ", err);
        result = false;
      }
    }
    if (result)
    {
      buffers = gfxDrvDDrawSurfacesInitialize(gfx_drv_ddraw_device_current);
      if (buffers == 0)
      {
        gfxDrvDDrawSetCooperativeLevelNormal(gfx_drv_ddraw_device_current);
      }
    }
  }
  return buffers;
}


/*==========================================================================*/
/* Open the default DirectDraw device, and record information about what is */
/* available.                                                               */
/* Called on emulator startup                                               */
/*==========================================================================*/

bool gfxDrvDDrawInitialize()
{
  bool result = gfxDrvDDrawDeviceInformationInitialize();

  if (result)
  {
    result = gfxDrvDDrawObjectInitialize(gfx_drv_ddraw_device_current);
    if (result)
    {
      result = gfxDrvDDrawModeInformationInitialize(gfx_drv_ddraw_device_current);
      if (!result)
      {
        gfxDrvDDraw2ObjectRelease(gfx_drv_ddraw_device_current);
        gfxDrvDDrawDeviceInformationRelease();
      }
    }
    else
    {
      gfxDrvDDrawDeviceInformationRelease();
    }
  }
  return result;
}


/*==========================================================================*/
/* Release any resources allocated by gfxDrvDDrawInitialize                 */
/* Called on emulator shutdown                                              */
/*==========================================================================*/

void gfxDrvDDrawRelease()
{
  gfxDrvDDrawModeInformationRelease(gfx_drv_ddraw_device_current);
  gfxDrvDDraw2ObjectRelease(gfx_drv_ddraw_device_current);
  gfxDrvDDrawDeviceInformationRelease();
}

/*==========================================================================*/
/* Functions below are the actual "graphics driver API"                     */
/*==========================================================================*/


void gfxDrvDDrawClearCurrentBuffer()
{
  LPDIRECTDRAWSURFACE lpDDS;
  LPDDSURFACEDESC lpDDSD;

  gfxDrvDDrawDrawTargetSurfaceSelect(gfx_drv_ddraw_device_current, &lpDDS, &lpDDSD);
  gfxDrvDDrawSurfaceClear(lpDDS);
}

/*==========================================================================*/
/* Locks surface and puts a valid framebuffer pointer in framebuffer        */
/*==========================================================================*/

UBY *gfxDrvDDrawValidateBufferPointer()
{
  ULO pitch;
  UBY *buffer = gfxDrvDDrawSurfaceLock(gfx_drv_ddraw_device_current, &pitch);
  if (buffer != NULL)
  {
    gfx_drv_ddraw_device_current->drawmode->pitch = pitch;

    /* Align pointer returned to drawing routines */

    if (gfx_drv_ddraw_device_current->drawmode->bits == 32)
    {
      if ((PTR_TO_INT(buffer) & 0x7) != 0)
      {
        buffer = (UBY *)(PTR_TO_INT(buffer) & ~PTR_TO_INT_MASK_TYPE(7)) + 8;
      }
    }
    if (gfx_drv_ddraw_device_current->drawmode->bits == 15 || gfx_drv_ddraw_device_current->drawmode->bits == 16)
    {
      if ((PTR_TO_INT(buffer) & 0x3) != 0)
      {
        buffer = (UBY *)(PTR_TO_INT(buffer) & ~PTR_TO_INT_MASK_TYPE(3)) + 4;
      }
    }
  }
  return buffer;
}


/*==========================================================================*/
/* Unlocks surface                                                          */
/*==========================================================================*/

void gfxDrvDDrawInvalidateBufferPointer()
{
  gfxDrvDDrawSurfaceUnlock(gfx_drv_ddraw_device_current);
}

void gfxDrvDDrawSizeChanged()
{
  gfxDrvDDrawFindWindowClientRect(gfx_drv_ddraw_device_current);
}

/*==========================================================================*/
/* Set the specified mode                                                   */
/* Returns the number of available buffers, zero is error                   */
/*==========================================================================*/

void gfxDrvDDrawSetMode(draw_mode *dm)
{
  gfx_drv_ddraw_device_current->drawmode = dm;
  gfx_drv_ddraw_device_current->mode = (gfx_drv_ddraw_mode *)listNode(listIndex(gfx_drv_ddraw_device_current->modes, dm->id));
}

/************************************************************************/
/**
  * Emulation is starting.
  *
  * Called on emulation start.
  * Subtlety: In exclusive mode, the window that is attached to the device
  * appears to become activated, even if it is not shown at the time.
  * The WM_ACTIVATE message triggers DirectInput acquisition, which means
  * that the DirectInput object needs to have been created at that time.
  * Unfortunately, the window must be created as well in order to attach DI
  * objects to it. So we create window, create DI objects in between and then
  * do the rest of the gfx init.
  * That is why gfxDrvEmulationStart is split in two.
  ***************************************************************************/

bool gfxDrvDDrawEmulationStart(ULO maxbuffercount)
{
  gfx_drv_ddraw_device_current->maxbuffercount = maxbuffercount;
  return true;
}


/************************************************************************/
/**
* Emulation is starting, post
***************************************************************************/

unsigned int gfxDrvDDrawEmulationStartPost()
{
  ULO buffers = gfxDrvDDrawSetMode(gfx_drv_ddraw_device_current);

  if (buffers == 0)
  {
    fellowAddLog("gfxdrv: gfxDrvDDrawEmulationStartPost(): Zero buffers, gfxDrvDDrawSetMode() failed\n");
  }

  return buffers;
}


/*==========================================================================*/
/* Emulation is stopping, clean up current videomode                        */
/*==========================================================================*/

void gfxDrvDDrawEmulationStop()
{
  gfxDrvDDrawSurfacesRelease(gfx_drv_ddraw_device_current);
  gfxDrvDDrawSetCooperativeLevelNormal(gfx_drv_ddraw_device_current);
}

#ifdef RETRO_PLATFORM

// TODO: Move it to GfxDrvCommon when mode lists are moved there

void gfxDrvDDrawRegisterRetroPlatformScreenMode(const bool bStartup) {
  ULO lHeight, lWidth;
  STR strMode[3] = "";

  if (RetroPlatformGetScanlines())
    cfgSetDisplayScaleStrategy(gfxDrvCommon->rp_startup_config, DISPLAYSCALE_STRATEGY_SCANLINES);
  else
    cfgSetDisplayScaleStrategy(gfxDrvCommon->rp_startup_config, DISPLAYSCALE_STRATEGY_SOLID);

  if (bStartup) {
    RetroPlatformSetScreenHeight(cfgGetScreenHeight(gfxDrvCommon->rp_startup_config));
    RetroPlatformSetScreenWidth(cfgGetScreenWidth(gfxDrvCommon->rp_startup_config));
  }

  lHeight = RetroPlatformGetScreenHeightAdjusted();
  lWidth = RetroPlatformGetScreenWidthAdjusted();

  cfgSetScreenHeight(gfxDrvCommon->rp_startup_config, lHeight);
  cfgSetScreenWidth(gfxDrvCommon->rp_startup_config, lWidth);

  fellowAddLog("gfxdrv: operating in RetroPlatform %s mode, insert resolution %ux%u into list of valid screen resolutions...\n",
    strMode, lWidth, lHeight);

  listAddLast(gfx_drv_ddraw_device_current->modes, listNew(gfxDrvDDrawModeNew(lWidth, lHeight, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
  gfxDrvDDrawModeInformationRegister(gfx_drv_ddraw_device_current);

  drawSetMode(cfgGetScreenWidth(gfxDrvCommon->rp_startup_config),
    cfgGetScreenHeight(gfxDrvCommon->rp_startup_config),
    cfgGetScreenColorBits(gfxDrvCommon->rp_startup_config),
    cfgGetScreenRefresh(gfxDrvCommon->rp_startup_config),
    cfgGetScreenWindowed(gfxDrvCommon->rp_startup_config));
}

#endif
/*==========================================================================*/
/* Collects information about the DirectX capabilities of this computer     */
/* After this, a DDraw object exists.                                       */
/* Called on emulator startup                                               */
/*==========================================================================*/

bool gfxDrvDDrawStartup()
{
  graph_buffer_lost = FALSE;
  gfx_drv_ddraw_initialized = gfxDrvDDrawInitialize();

#ifdef RETRO_PLATFORM
  if (RetroPlatformGetMode() && gfx_drv_ddraw_initialized) {
    gfxDrvDDrawRegisterRetroPlatformScreenMode(true);
  }
#endif

  return gfx_drv_ddraw_initialized;
}


/*==========================================================================*/
/* Free DDraw resources                                                     */
/* Called on emulator shutdown                                              */
/*==========================================================================*/

void gfxDrvDDrawShutdown()
{
  if (gfx_drv_ddraw_initialized)
  {
    gfxDrvDDrawRelease();
    gfx_drv_ddraw_initialized = false;
  }
}

static bool gfxDrvTakeScreenShotFromDCArea(HDC hDC, DWORD x, DWORD y, DWORD width, DWORD height, ULO lDisplayScale, DWORD bits, const STR *filename)
{
  BITMAPFILEHEADER bfh;
  BITMAPINFOHEADER bih;
  HDC memDC = NULL;
  HGDIOBJ bitmap = NULL;
  HGDIOBJ oldbit = NULL;
  FILE *file = NULL;
  void *data = NULL;
  int bpp, datasize;
  bool bSuccess = FALSE;

  if (!hDC) return 0;
  if (bits <= 0) return 0;
  if (width <= 0) return 0;
  if (height <= 0) return 0;

  bpp = bits / 8;
  if (bpp < 2) bpp = 2;
  if (bpp > 3) bpp = 3;

  datasize = width * bpp * height;
  if (width * bpp % 4)
  {
    datasize += height * (4 - width * bpp % 4);
  }

  memset((void*)&bfh, 0, sizeof(bfh));

  bfh.bfType = 'B' + ('M' << 8);
  bfh.bfSize = sizeof(bfh) + sizeof(bih) + datasize;
  bfh.bfOffBits = sizeof(bfh) + sizeof(bih);

  memset((void*)&bih, 0, sizeof(bih));
  bih.biSize = sizeof(bih);
  bih.biWidth = width;
  bih.biHeight = height;
  bih.biPlanes = 1;
  bih.biBitCount = bpp * 8;
  bih.biCompression = BI_RGB;

  bitmap = CreateDIBSection(NULL, (BITMAPINFO *)&bih,
    DIB_RGB_COLORS, &data, NULL, 0);

  if (!bitmap) goto cleanup;
  if (!data) goto cleanup;

  memDC = CreateCompatibleDC(hDC);
  if (!memDC) goto cleanup;

  oldbit = SelectObject(memDC, bitmap);
  if (oldbit <= 0) goto cleanup;

  bSuccess = StretchBlt(memDC, 0, 0, width, height, hDC, x, y, width / lDisplayScale, height / lDisplayScale, SRCCOPY) ? true : false;
  if (!bSuccess) goto cleanup;

  file = fopen(filename, "wb");
  if (!file) goto cleanup;

  fwrite((void*)&bfh, sizeof(bfh), 1, file);
  fwrite((void*)&bih, sizeof(bih), 1, file);
  fwrite((void*)data, 1, datasize, file);

  bSuccess = TRUE;

cleanup:
  if (oldbit > 0) SelectObject(memDC, oldbit);
  if (memDC) DeleteDC(memDC);
  if (file) fclose(file);
  if (bitmap) DeleteObject(bitmap);

  fellowAddLog("gfxDrvTakeScreenShotFromDC(hDC=0x%x, width=%d, height=%d, bits=%d, filename='%s' %s.\n",
    hDC, width, height, bits, filename, bSuccess ? "successful" : "failed");

  return bSuccess;
}

static bool gfxDrvTakeScreenShotFromDirectDrawSurfaceArea(LPDIRECTDRAWSURFACE surface,
  DWORD x, DWORD y, DWORD width, DWORD height, ULO lDisplayScale, const STR *filename) {
  DDSURFACEDESC ddsd;
  HRESULT hResult = DD_OK;
  HDC surfDC = NULL;
  bool bSuccess = FALSE;

  if (!surface) return FALSE;

  ZeroMemory(&ddsd, sizeof(ddsd));
  ddsd.dwSize = sizeof(ddsd);
  hResult = surface->GetSurfaceDesc(&ddsd);
  if (FAILED(hResult)) return FALSE;

  hResult = surface->GetDC(&surfDC);
  if (FAILED(hResult)) return FALSE;

  bSuccess = gfxDrvTakeScreenShotFromDCArea(surfDC, x, y, width, height, lDisplayScale, ddsd.ddpfPixelFormat.dwRGBBitCount, filename);

  if (surfDC) surface->ReleaseDC(surfDC);

  return bSuccess;
}

bool gfxDrvTakeScreenShot(const bool bTakeFilteredScreenshot, const STR *filename) {
  bool bResult;
  DWORD width = 0, height = 0, x = 0, y = 0;
  ULO lDisplayScale = RetroPlatformGetDisplayScale();

  if (bTakeFilteredScreenshot) {
    width = RetroPlatformGetScreenWidthAdjusted();
    height = RetroPlatformGetScreenHeightAdjusted();
    x = RetroPlatformGetClippingOffsetLeftAdjusted();
    y = RetroPlatformGetClippingOffsetTopAdjusted();
    bResult = gfxDrvTakeScreenShotFromDirectDrawSurfaceArea(gfx_drv_ddraw_device_current->lpDDSSecondary, x, y, width, height, lDisplayScale, filename);
  }
  else {
    //width and height in RP mode are sized for maximum scale factor
    // use harcoded RetroPlatform max PAL dimensions from WinUAE
    width = RETRO_PLATFORM_MAX_PAL_LORES_WIDTH * 2;
    height = RETRO_PLATFORM_MAX_PAL_LORES_HEIGHT * 2;
    bResult = gfxDrvTakeScreenShotFromDirectDrawSurfaceArea(gfx_drv_ddraw_device_current->lpDDSSecondary, x, y, width, height, 1, filename);
  }

  fellowAddLog("gfxDrvTakeScreenShot(filtered=%d, filename='%s') %s.\n", bTakeFilteredScreenshot, filename,
    bResult ? "successful" : "failed");

  return bResult;
}