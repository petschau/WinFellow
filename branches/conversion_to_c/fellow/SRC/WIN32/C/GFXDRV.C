/*===========================================================================*/
/* Fellow Amiga Emulator                                                     */
/* Host framebuffer driver                                                   */
/* DirectX implementation                                                    */
/* Author: Petter Schau (peschau@online.no)                                  */
/*                                                                           */
/* This file is under the GNU Public License (GPL)                           */
/*===========================================================================*/


/*===========================================================================
  Framebuffer modes of operation:

  1. Using the primary buffer non-clipped, with possible back-buffers.
     This applies to fullscreen mode with a framepointer.
  
  2. Using a secondary buffer to render and then applying the blitter
     to the primary buffer. The primary buffer can be clipped (window)
     or non-clipped (fullscreen). In this mode there are no backbuffers.
    
      
  blitmode is used:
  
    1. When running on the desktop in a window.
    2. In fullscreen mode with a primary surface that
       can not supply a framebuffer pointer.
					   
			     
  Windows:
					       
  Two types of windows: 1. Normal desktop window for desktop operation
			2. Full-screen window for fullscreen mode
						 
						   
  Windows are created and destroyed on emulation start and stop.
						     
						       
  Buffers:
							 
  Buffers are created when emulation starts and destroyed
  when emulation stops. (Recreated when lost also)
							   
  if blitmode, create single primary buffer
  and a secondary buffer for actual rendering in system memory.
  if windowed also add a clipper to the primary buffer
							     
  else create a primary buffer (with backbuffers)
							       
*/  


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
#include "gfxdrv.h"
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

ini *gfxdrv_ini; /* GFXDRV copy of initdata */

/*==========================================================================*/
/* Structs for holding information about a DirectDraw device and mode       */
/*==========================================================================*/

typedef struct {
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
  BOOLE windowed;
} gfx_drv_ddraw_mode;

typedef struct {
  LPGUID               lpGUID;
  LPSTR                lpDriverDescription;
  LPSTR                lpDriverName;
  LPDIRECTDRAW         lpDD;
  LPDIRECTDRAW2        lpDD2;
  LPDIRECTDRAWSURFACE  lpDDSPrimary;		/* Primary display surface        */
  LPDIRECTDRAWSURFACE  lpDDSBack;               /* Current backbuffer for Primary */
  LPDIRECTDRAWSURFACE  lpDDSSecondary;		/* Source surface in blitmode     */
  DDSURFACEDESC        ddsdPrimary;
  DDSURFACEDESC        ddsdBack;
  DDSURFACEDESC        ddsdSecondary;
  LPDIRECTDRAWPALETTE  lpDDPalette;
  LPDIRECTDRAWCLIPPER  lpDDClipper;
  BOOLE		       PaletteInitialized;
  felist               *modes;
  gfx_drv_ddraw_mode   *mode;
  ULO		       buffercount;
  ULO		       maxbuffercount;
  RECT		       hwnd_clientrect_screen;
  RECT		       hwnd_clientrect_win;
  draw_mode            *drawmode;
  BOOLE		       use_blitter;
  ULO		       vertical_scale;
  BOOLE		       can_stretch_y;
  BOOLE		       stretch_warning_shutup;
  BOOLE		       no_dd_hardware;
} gfx_drv_ddraw_device;

gfx_drv_ddraw_device *gfx_drv_ddraw_device_current;
felist               *gfx_drv_ddraw_devices;


/*==========================================================================*/
/* Window hosting the Amiga display and some generic status information     */
/*==========================================================================*/

BOOLE gfx_drv_initialized;
volatile BOOLE gfx_drv_win_active;
volatile BOOLE gfx_drv_win_active_original;
volatile BOOLE gfx_drv_win_minimized_original;
volatile BOOLE gfx_drv_syskey_down;
HWND  gfx_drv_hwnd;
BOOLE gfx_drv_displaychange;
BOOLE gfx_drv_stretch_always;

HANDLE gfx_drv_app_run;        /* Event indicating running or paused status */


/*==========================================================================*/
/* Master copy of the 8 bit palette                                         */
/*==========================================================================*/

PALETTEENTRY gfx_drv_palette[256];


/*==========================================================================*/
/* Initialize the run status event                                          */
/*==========================================================================*/

BOOLE gfxDrvRunEventInitialize(void) {
  gfx_drv_app_run = CreateEvent(NULL, TRUE, FALSE, NULL);
  return (gfx_drv_app_run != NULL);
}

void gfxDrvRunEventRelease(void) {
  if (gfx_drv_app_run) CloseHandle(gfx_drv_app_run);
}

void gfxDrvRunEventSet(void) {
  SetEvent(gfx_drv_app_run);
}

void gfxDrvRunEventReset(void) {
  ResetEvent(gfx_drv_app_run);
}

void gfxDrvRunEventWait(void) {
  WaitForSingleObject(gfx_drv_app_run, INFINITE);
}


/*==========================================================================*/
/* Our own messages                                                         */
/*==========================================================================*/

#define WM_DIACQUIRE  (WM_USER + 0)

void gfxDrvChangeDInputDeviceStates(BOOLE active) {
  mouseDrvStateHasChanged(active);
  joyDrvStateHasChanged(active);
  kbdDrvStateHasChanged(active);
}

void gfxDrvEvaluateActiveStatus(void) {
  BOOLE old_active = gfx_drv_win_active;
  gfx_drv_win_active = (gfx_drv_win_active_original &&
                       !gfx_drv_win_minimized_original &&
		        !gfx_drv_syskey_down);
  if (gfx_drv_win_active) {
    //fellowAddLog("App is currently active ");
//    if (gfx_drv_win_minimized_original) fellowAddLog("minimized ");
//    if (gfx_drv_win_active_original) fellowAddLog("active ");
//    if (gfx_drv_syskey_down) fellowAddLog("syskey_down ");
//    else fellowAddLog("syskey_up ");
//    fellowAddLog("\n");
    gfxDrvRunEventSet();
  }
  else {
//    fellowAddLog("App is currently in-active ");
//    if (gfx_drv_win_minimized_original) fellowAddLog("minimized ");
//    if (gfx_drv_win_active_original) fellowAddLog("active ");
//    if (gfx_drv_syskey_down) fellowAddLog("syskey_down ");
//    else fellowAddLog("syskey_up ");
//    fellowAddLog("\n");
    gfxDrvRunEventReset();
  }
}


/*==========================================================================*/
/* Window procedure for the emulation window                                */
/* Distributes events to mouse and keyboard drivers as well                 */
/*==========================================================================*/

void gfxDrvWindowFindClientRect(gfx_drv_ddraw_device *ddraw_device);
BOOLE gfxDrvDDrawSetPalette(gfx_drv_ddraw_device *ddraw_device);


long FAR PASCAL EmulationWindowProc(HWND hWnd,
				    UINT message, 
                                    WPARAM wParam,
				    LPARAM lParam) {
  RECT emulationRect;

  BOOLE diacquire_sent = FALSE;
  switch (message) {
  case WM_TIMER:
    if (wParam == 1) {
      winDrvHandleInputDevices();
      soundDrvPollBufferPosition();
      return 0;
    }
    break;
  case WM_SYSKEYDOWN:
//    fellowAddLog("WM_SYSKEYDOWN %d\n", (int) wParam);
    {
      int vkey = (int) wParam;
      gfx_drv_syskey_down = (vkey != VK_F10);
      gfxDrvEvaluateActiveStatus();
    }
    break;
  case WM_SYSKEYUP:
//    fellowAddLog("WM_SYSKEYUP\n");
    gfx_drv_syskey_down = FALSE;
    gfxDrvEvaluateActiveStatus();
    switch (wParam) {
      case VK_RETURN:  /* Full screen vs windowed */
//        fellowAddLog("WM_SYSKEYUP/VK_RETURN\n");
        break;
    }
    break;
  case WM_MOVE:
  case WM_SIZE:
    if (gfx_drv_ddraw_device_current->mode->windowed)
      gfxDrvWindowFindClientRect(gfx_drv_ddraw_device_current);
    break;
  case WM_ACTIVATE:
/*      
    fellowAddLog("WM_ACTIVE");

    if (hWnd == gfx_drv_hwnd) fellowAddLog("Our window\n");
    else fellowAddLog("Not our window\n");
    if (HIWORD(wParam)) fellowAddLog("Minimized\n");
    else fellowAddLog("Not minimized\n");
    if (LOWORD(wParam) == WA_ACTIVE) fellowAddLog("WA_ACTIVE\n");
    if (LOWORD(wParam) == WA_CLICKACTIVE) fellowAddLog("WA_CLICKACTIVE\n");
*/
    /* WM_ACTIVATE tells us whether our window is active or not */
    /* It is monitored so that we can know whether we should claim */
    /* the DirectInput devices */

    gfx_drv_win_active_original = (((LOWORD(wParam)) == WA_ACTIVE) ||
		                   ((LOWORD(wParam)) == WA_CLICKACTIVE));
    gfx_drv_win_minimized_original = ((HIWORD(wParam)) != 0);
    gfxDrvChangeDInputDeviceStates(gfx_drv_win_active_original);
    gfxDrvEvaluateActiveStatus();
    return 0; /* We processed this message */
  case WM_ENTERMENULOOP:
  case WM_ENTERSIZEMOVE:
    gfx_drv_win_active_original = FALSE;
    gfxDrvChangeDInputDeviceStates(gfx_drv_win_active_original);
    return 0;
  case WM_EXITMENULOOP:
  case WM_EXITSIZEMOVE:
    gfx_drv_win_active_original = (GetActiveWindow() == hWnd && !IsIconic(hWnd));
    PostMessage(hWnd, WM_DIACQUIRE, 0, 0L);
    return 0;
  case WM_SYSCOMMAND:
//    fellowAddLog("WM_SYSCOMMAND\n");
    if (IsWindow(hWnd)) {
      gfxDrvChangeDInputDeviceStates(gfx_drv_win_active_original);
    }
    switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case SC_SCREENSAVE:
	  return 0;
	case SC_KEYMENU:
//	  fellowAddLog("SC_KEYMENU\n");
	  return 0;
        default:
          return DefWindowProc(hWnd, message, wParam, lParam);
      }
  case WM_ACTIVATEAPP:
//    fellowAddLog("WM_ACTIVATEAPP ");
//    if (hWnd == gfx_drv_hwnd) fellowAddLog("Our window\n");
//    else fellowAddLog("Not our window\n");
    if (wParam) {
      /* Being activated */
    }
    else {
      /* Being de-activated */
      gfx_drv_syskey_down = FALSE;
    }
    return 0;
/*    gfx_drv_win_activateapp_pure = wParam;
      {*/
	/* Windowed operation we pause only if the window is minimized */
	/* In full-screen mode we pause anytime the app is not active */
	/* or when the window is minimized */
/*	BOOLE active_status = ((gfx_drv_ddraw_device_current->mode->windowed &&
	                        !gfx_drv_win_minimized) ||
			       (!gfx_drv_ddraw_device_current->mode->windowed &&
			        gfx_drv_win_activateapp_pure && gfx_drv_win_active));
        if (gfx_drv_app_paused && active_status)
	  fellowAddLog("WM_ACTIVATEAPP made app active, running\n");
	else if (!gfx_drv_app_paused && !active_status)
	  fellowAddLog("WM_ACTIVATEAPP made app inactive, pause\n");
	if (gfx_drv_app_paused != !active_status) {
	  gfx_drv_app_paused = !active_status;
	  if (!diacquire_sent) {
	    PostMessage(gfx_drv_hwnd, WM_DIACQUIRE, 0, 0L);
	    diacquire_sent = TRUE;
	  }
	}
      }
      */
    break;
  case WM_DESTROY:
	  // save emulation window position only if in windowed mode
	  if (gfx_drv_ddraw_device_current->mode->windowed) {
		GetWindowRect(hWnd, &emulationRect);  
		iniSetEmulationWindowPosition(gfxdrv_ini, emulationRect.left, emulationRect.top);
	  }
//      fellowAddLog("WM_DESTROY\n");
      gfxDrvChangeDInputDeviceStates(FALSE);
      return 0;
      break;
    case WM_SHOWWINDOW:
      break; 
    case WM_DISPLAYCHANGE:
      gfx_drv_displaychange = (wParam != gfx_drv_ddraw_device_current->mode->depth) &&
			      gfx_drv_ddraw_device_current->mode->windowed;
      break;
    case WM_PALETTECHANGED:	/* Palette has changed, sometimes we did it */
      /*if (((HWND) wParam) == gfx_drv_hwnd)*/
	break;
      /* else run code in WM_PALETTECHANGED to claim whatever colors are left for us */
    case WM_QUERYNEWPALETTE:	/* We are activated and are allowed to realise our palette */
      gfxDrvDDrawSetPalette(gfx_drv_ddraw_device_current);
      break;
    case WM_DIACQUIRE:        /* Re-evaluate the active status of DI-devices */
//      fellowAddLog("WM_DIACQUIRE\n");
      gfxDrvChangeDInputDeviceStates(gfx_drv_win_active_original);
      return 0;
      break;
    case WM_CLOSE:
//      fellowAddLog("WM_CLOSE\n");
      fellowRequestEmulationStop();
      return 0; /* We handled this message */ 

      
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}



/*==========================================================================*/
/* Returns textual error message. Adapted from DX SDK                       */
/*==========================================================================*/

STR *gfxDrvDDrawErrorString(HRESULT hResult) {
  switch (hResult) {
  case DD_OK:				return "DD_OK";
  case DDERR_ALREADYINITIALIZED:	       return "DDERR_ALREADYINITIALIZED";
  case DDERR_BLTFASTCANTCLIP:	       return "DDERR_BLTFASTCANTCLIP";
  case DDERR_CANNOTATTACHSURFACE:	       return "DDERR_CANNOTATTACHSURFACE";
  case DDERR_CANNOTDETACHSURFACE:	       return "DDERR_CANNOTDETACHSURFACE";
  case DDERR_CANTCREATEDC:		       return "DDERR_CANTCREATEDC";
  case DDERR_CANTDUPLICATE:                return "DDERR_CANTDUPLICATE";
  case DDERR_CANTLOCKSURFACE:	       return "DDERR_CANTLOCKSURFACE"; 
  case DDERR_CANTPAGELOCK:		       return "DDERR_CANTPAGELOCK"; 
  case DDERR_CANTPAGEUNLOCK:	       return "DDERR_CANTPAGEUNLOCK";
  case DDERR_CLIPPERISUSINGHWND:	       return "DDERR_CLIPPERISUSINGHWND";
  case DDERR_COLORKEYNOTSET:	       return "DDERR_COLORKEYNOTSET";
  case DDERR_CURRENTLYNOTAVAIL:	       return "DDERR_CURRENTLYNOTAVAIL";
  case DDERR_DCALREADYCREATED:	       return "DDERR_DCALREADYCREATED";
  case DDERR_DIRECTDRAWALREADYCREATED:     return "DDERR_DIRECTDRAWALREADYCREATED";
  case DDERR_EXCEPTION:		       return "DDERR_EXCEPTION";
  case DDERR_EXCLUSIVEMODEALREADYSET:      return "DDERR_EXCLUSIVEMODEALREADYSET";
  case DDERR_GENERIC:		       return "DDERR_GENERIC";
  case DDERR_HEIGHTALIGN:		       return "DDERR_HEIGHTALIGN";
  case DDERR_HWNDALREADYSET:	       return "DDERR_HWNDALREADYSET";
  case DDERR_HWNDSUBCLASSED:	       return "DDERR_HWNDSUBCLASSED";
  case DDERR_IMPLICITLYCREATED:	       return "DDERR_IMPLICITLYCREATED";
  case DDERR_INCOMPATIBLEPRIMARY:	       return "DDERR_INCOMPATIBLEPRIMARY";
  case DDERR_INVALIDCAPS:		       return "DDERR_INVALIDCAPS";
  case DDERR_INVALIDCLIPLIST:	       return "DDERR_INVALIDCLIPLIST";
  case DDERR_INVALIDDIRECTDRAWGUID:        return "DDERR_INVALIDDIRECTDRAWGUID";
  case DDERR_INVALIDMODE:		       return "DDERR_INVALIDMODE";
  case DDERR_INVALIDOBJECT:		       return "DDERR_INVALIDOBJECT";
  case DDERR_INVALIDPARAMS:		       return "DDERR_INVALIDPARAMS";
  case DDERR_INVALIDPIXELFORMAT:	       return "DDERR_INVALIDPIXELFORMAT";
  case DDERR_INVALIDPOSITION:	       return "DDERR_INVALIDPOSITION";
  case DDERR_INVALIDRECT:		       return "DDERR_INVALIDRECT";
  case DDERR_INVALIDSURFACETYPE:	       return "DDERR_INVALIDSURFACETYPE";
  case DDERR_LOCKEDSURFACES:	       return "DDERR_LOCKEDSURFACES";
  case DDERR_NO3D:			       return "DDERR_NO3D";
  case DDERR_NOALPHAHW:		       return "DDERR_NOALPHAHW";
  case DDERR_NOBLTHW:		       return "DDERR_NOBLTHW";
  case DDERR_NOCLIPLIST:		       return "DDERR_NOCLIPLIST";
  case DDERR_NOCLIPPERATTACHED:	       return "DDERR_NOCLIPPERATTACHED";
  case DDERR_NOCOLORCONVHW:		       return "DDERR_NOCOLORCONVHW";
  case DDERR_NOCOLORKEY:		       return "DDERR_NOCOLORKEY";
  case DDERR_NOCOLORKEYHW:		       return "DDERR_NOCOLORKEYHW";
  case DDERR_NOCOOPERATIVELEVELSET:        return "DDERR_NOCOOPERATIVELEVELSET";
  case DDERR_NODC:			       return "DDERR_NODC";
  case DDERR_NODDROPSHW:		       return "DDERR_NODDROPSHW";
  case DDERR_NODIRECTDRAWHW:	       return "DDERR_NODIRECTDRAWHW";
  case DDERR_NODIRECTDRAWSUPPORT:	       return "DDERR_NODIRECTDRAWSUPPORT";
  case DDERR_NOEMULATION:		       return "DDERR_NOEMULATION";
  case DDERR_NOEXCLUSIVEMODE:	       return "DDERR_NOEXCLUSIVEMODE";
  case DDERR_NOFLIPHW:		       return "DDERR_NOFLIPHW";
  case DDERR_NOGDI:			       return "DDERR_NOGDI";
  case DDERR_NOHWND:		       return "DDERR_NOHWND";
  case DDERR_NOMIPMAPHW:		       return "DDERR_NOMIPMAPHW";
  case DDERR_NOMIRRORHW:		       return "DDERR_NOMIRRORHW";
  case DDERR_NOOVERLAYDEST:		       return "DDERR_NOOVERLAYDEST";
  case DDERR_NOOVERLAYHW:		       return "DDERR_NOOVERLAYHW";
  case DDERR_NOPALETTEATTACHED:	       return "DDERR_NOPALETTEATTACHED";
  case DDERR_NOPALETTEHW:		       return "DDERR_NOPALETTEHW";
  case DDERR_NORASTEROPHW:		       return "DDERR_NORASTEROPHW";
  case DDERR_NOROTATIONHW:		       return "DDERR_NOROTATIONHW";
  case DDERR_NOSTRETCHHW:		       return "DDERR_NOSTRETCHHW";
  case DDERR_NOT4BITCOLOR:		       return "DDERR_NOT4BITCOLOR";
  case DDERR_NOT4BITCOLORINDEX:	       return "DDERR_NOT4BITCOLORINDEX";
  case DDERR_NOT8BITCOLOR:		       return "DDERR_NOT8BITCOLOR";
  case DDERR_NOTAOVERLAYSURFACE:	       return "DDERR_NOTAOVERLAYSURFACE";
  case DDERR_NOTEXTUREHW:		       return "DDERR_NOTEXTUREHW";
  case DDERR_NOTFLIPPABLE:		       return "DDERR_NOTFLIPPABLE";
  case DDERR_NOTFOUND:		       return "DDERR_NOTFOUND";
  case DDERR_NOTINITIALIZED:	       return "DDERR_NOTINITIALIZED";
  case DDERR_NOTLOCKED:		       return "DDERR_NOTLOCKED";
  case DDERR_NOTPAGELOCKED:		       return "DDERR_NOTPAGELOCKED";
  case DDERR_NOTPALETTIZED:		       return "DDERR_NOTPALETTIZED";
  case DDERR_NOVSYNCHW:		       return "DDERR_NOVSYNCHW";
  case DDERR_NOZBUFFERHW:		       return "DDERR_NOZBUFFERHW";
  case DDERR_NOZOVERLAYHW:		       return "DDERR_NOZOVERLAYHW";
  case DDERR_OUTOFCAPS:		       return "DDERR_OUTOFCAPS";
  case DDERR_OUTOFMEMORY:		       return "DDERR_OUTOFMEMORY";
  case DDERR_OUTOFVIDEOMEMORY:	       return "DDERR_OUTOFVIDEOMEMORY";
  case DDERR_OVERLAYCANTCLIP:	       return "DDERR_OVERLAYCANTCLIP";
  case DDERR_OVERLAYCOLORKEYONLYONEACTIVE: return "DDERR_OVERLAYCOLORKEYONLYONEACTIVE";
  case DDERR_OVERLAYNOTVISIBLE:	       return "DDERR_OVERLAYNOTVISIBLE";
  case DDERR_PALETTEBUSY:		       return "DDERR_PALETTEBUSY";
  case DDERR_PRIMARYSURFACEALREADYEXISTS:  return "DDERR_PRIMARYSURFACEALREADYEXISTS";
  case DDERR_REGIONTOOSMALL:	       return "DDERR_REGIONTOOSMALL";
  case DDERR_SURFACEALREADYATTACHED:       return "DDERR_SURFACEALREADYATTACHED";
  case DDERR_SURFACEALREADYDEPENDENT:      return "DDERR_SURFACEALREADYDEPENDENT";
  case DDERR_SURFACEBUSY:		       return "DDERR_SURFACEBUSY";
  case DDERR_SURFACEISOBSCURED:	       return "DDERR_SURFACEISOBSCURED";
  case DDERR_SURFACELOST:		       return "DDERR_SURFACELOST";
  case DDERR_SURFACENOTATTACHED:           return "DDERR_SURFACENOTATTACHED";
  case DDERR_TOOBIGHEIGHT:		       return "DDERR_TOOBIGHEIGHT";
  case DDERR_TOOBIGSIZE:		       return "DDERR_TOOBIGSIZE";
  case DDERR_TOOBIGWIDTH:		       return "DDERR_TOOBIGWIDTH";
  case DDERR_UNSUPPORTED:		       return "DDERR_UNSUPPORTED";
  case DDERR_UNSUPPORTEDFORMAT:	       return "DDERR_UNSUPPORTEDFORMAT";
  case DDERR_UNSUPPORTEDMASK:	       return "DDERR_UNSUPPORTEDMASK";
  case DDERR_UNSUPPORTEDMODE:	       return "DDERR_UNSUPPORTEDMODE";
  case DDERR_VERTICALBLANKINPROGRESS:      return "DDERR_VERTICALBLANKINPROGRESS";
  case DDERR_WASSTILLDRAWING:	       return "DDERR_WASSTILLDRAWING";
  case DDERR_WRONGMODE:		       return "DDERR_WRONGMODE";
  case DDERR_XALIGN:		       return "DDERR_XALIGN";
  }
  return "Not a DirectDraw Error";
}


/*==========================================================================*/
/* Logs a sensible error message                                            */
/*==========================================================================*/

void gfxDrvDDrawFailure(STR *header, HRESULT err) {
  fellowAddLog("gfxdrv: ");
  fellowAddLog(header);
  fellowAddLog(gfxDrvDDrawErrorString(err));
  fellowAddLog("\n");
}

/*==========================================================================*/
/* Create window classes for Amiga display                                  */
/* Called on emulator startup                                               */
/*==========================================================================*/

BOOLE gfxDrvWindowClassInitialize(void) {
  WNDCLASS wc1;
  
  memset(&wc1, 0, sizeof(wc1));
  wc1.style = CS_HREDRAW | CS_VREDRAW | CS_BYTEALIGNCLIENT;
  wc1.lpfnWndProc = EmulationWindowProc;
  wc1.cbClsExtra = 0;
  wc1.cbWndExtra = 0;
  wc1.hInstance = win_drv_hInstance;
  wc1.hIcon = LoadIcon(win_drv_hInstance, MAKEINTRESOURCE(IDI_ICON_WINFELLOW));
  wc1.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc1.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
  wc1.lpszClassName = "FellowWindowClass";
  wc1.lpszMenuName = "Fellow";
  return (RegisterClass(&wc1) != 0);
}


/*==========================================================================*/
/* Get information about the current client rectangle                       */
/*==========================================================================*/

void gfxDrvWindowFindClientRect(gfx_drv_ddraw_device *ddraw_device) {
  GetClientRect(gfx_drv_hwnd, &ddraw_device->hwnd_clientrect_win);
  memcpy(&ddraw_device->hwnd_clientrect_screen, &ddraw_device->hwnd_clientrect_win, sizeof(RECT));
  ClientToScreen(gfx_drv_hwnd, (LPPOINT) &ddraw_device->hwnd_clientrect_screen);
  ClientToScreen(gfx_drv_hwnd, (LPPOINT) &ddraw_device->hwnd_clientrect_screen + 1);
}


/*==========================================================================*/
/* Show window hosting the amiga display                                    */
/* Called on every emulation startup                                        */
/*==========================================================================*/

void gfxDrvWindowShow(gfx_drv_ddraw_device *ddraw_device) {
  fellowAddLog("gfxdrv: gfxDrvWindowShow()\n");
  if (!ddraw_device->mode->windowed) {
    ShowWindow(gfx_drv_hwnd, SW_SHOWMAXIMIZED);
    UpdateWindow(gfx_drv_hwnd);
  }
  else {
    RECT rc1;
    
    SetRect(&rc1, iniGetEmulationWindowXPos(gfxdrv_ini), iniGetEmulationWindowYPos(gfxdrv_ini), 
		ddraw_device->drawmode->width + iniGetEmulationWindowXPos(gfxdrv_ini), 
		ddraw_device->drawmode->height + iniGetEmulationWindowYPos(gfxdrv_ini));
//    SetRect(&rc1, 0, 0, ddraw_device->drawmode->width, ddraw_device->drawmode->height);
    AdjustWindowRectEx(&rc1,
      GetWindowStyle(gfx_drv_hwnd),
      GetMenu(gfx_drv_hwnd) != NULL,
      GetWindowExStyle(gfx_drv_hwnd));
/*
    MoveWindow(gfx_drv_hwnd,
      0, 
      0, 
      rc1.right - rc1.left,
      rc1.bottom - rc1.top,
      FALSE);*/
	MoveWindow(gfx_drv_hwnd,
      iniGetEmulationWindowXPos(gfxdrv_ini),
      iniGetEmulationWindowYPos(gfxdrv_ini), 
      rc1.right - rc1.left,
      rc1.bottom - rc1.top,
      FALSE);
    ShowWindow(gfx_drv_hwnd, SW_SHOWNORMAL);
    UpdateWindow(gfx_drv_hwnd);
    gfxDrvWindowFindClientRect(ddraw_device);
  }
}


/*==========================================================================*/
/* Hide window hosting the amiga display                                    */
/* Called on emulation stop                                                 */
/*==========================================================================*/

void gfxDrvWindowHide(gfx_drv_ddraw_device *ddraw_device) {
  if (!ddraw_device->mode->windowed)
    ShowWindow(gfx_drv_hwnd, SW_SHOWMINIMIZED);
}


/*==========================================================================*/
/* Create a window to host the Amiga display                                */
/* Called on emulation start                                                */
/* Returns TRUE if succeeded                                                */
/*==========================================================================*/

BOOLE gfxDrvWindowInitialize(gfx_drv_ddraw_device *ddraw_device) {
  if (ddraw_device->mode->windowed) {
    gfx_drv_hwnd = CreateWindowEx(0,
      "FellowWindowClass",
      FELLOWVERSION,
      WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | ((gfx_drv_stretch_always) ? (WS_MAXIMIZEBOX | WS_SIZEBOX) : 0),
      CW_USEDEFAULT,
      SW_SHOW,
      ddraw_device->drawmode->width,
      ddraw_device->drawmode->height,
      NULL,
      NULL,
      win_drv_hInstance,
      NULL);
  }
  else {
    gfx_drv_hwnd = CreateWindowEx(WS_EX_TOPMOST,
      "FellowWindowClass",
      FELLOWVERSION,
      WS_POPUP,
      0,
      0,
      GetSystemMetrics(SM_CXSCREEN),
      GetSystemMetrics(SM_CYSCREEN),
      NULL,
      NULL,
      win_drv_hInstance,
      NULL);
  }
  fellowAddLog("gfxdrv: Window created\n");
  return (gfx_drv_hwnd != NULL);
}


/*==========================================================================*/
/* Destroy window hosting the amiga display                                 */
/* Called on emulation stop                                                 */
/*==========================================================================*/

void gfxDrvWindowRelease(gfx_drv_ddraw_device *ddraw_device) {
  if (gfx_drv_hwnd != NULL) {
    DestroyWindow(gfx_drv_hwnd);
    gfx_drv_hwnd = NULL;
  }
}


/*==========================================================================*/
/* Select the correct drawing surface for a draw operation                  */
/*==========================================================================*/

void gfxDrvDDrawDrawTargetSurfaceSelect(gfx_drv_ddraw_device *ddraw_device,
				        LPDIRECTDRAWSURFACE *lpDDS,
				        LPDDSURFACEDESC *lpDDSD) {
  if (ddraw_device->use_blitter) {
    *lpDDS = ddraw_device->lpDDSSecondary;
    *lpDDSD = &(ddraw_device->ddsdSecondary);
  }
  else {
    *lpDDS = (ddraw_device->buffercount == 1) ? ddraw_device->lpDDSPrimary :
                                                ddraw_device->lpDDSBack;
    *lpDDSD = (ddraw_device->buffercount == 1) ? &(ddraw_device->ddsdPrimary) :
                                                 &(ddraw_device->ddsdBack);
  }
}

/*==========================================================================*/
/* Select the correct target surface for a blit operation                   */ 
/*==========================================================================*/

void gfxDrvDDrawBlitTargetSurfaceSelect(gfx_drv_ddraw_device *ddraw_device,
				        LPDIRECTDRAWSURFACE *lpDDS) {
  *lpDDS = (ddraw_device->buffercount == 1) ? ddraw_device->lpDDSPrimary :
                                              ddraw_device->lpDDSBack;
}

/*==========================================================================*/
/* Release clipper                                                          */
/*==========================================================================*/

void gfxDrvDDrawClipperRelease(gfx_drv_ddraw_device *ddraw_device) {
  if (ddraw_device->lpDDClipper != NULL) {
    IDirectDrawClipper_Release(ddraw_device->lpDDClipper);
    ddraw_device->lpDDClipper = NULL;
  }
}


/*==========================================================================*/
/* Initialize clipper                                                       */
/*==========================================================================*/

BOOLE gfxDrvDDrawClipperInitialize(gfx_drv_ddraw_device *ddraw_device) {
  HRESULT err;
  
  if ((err = IDirectDraw2_CreateClipper(ddraw_device->lpDD2,
    0,
    &ddraw_device->lpDDClipper,
    NULL)) != DD_OK)
    gfxDrvDDrawFailure("gfxDrvDDrawClipperInitialize: CreateClipper() ", err);
  else {
    if ((err = IDirectDrawClipper_SetHWnd(ddraw_device->lpDDClipper,
      0,
      gfx_drv_hwnd)) != DD_OK)
      gfxDrvDDrawFailure("gfxDrvDDrawClipperInitialize: SetHWnd() ", err);
    else {
      if ((err = IDirectDrawSurface_SetClipper(ddraw_device->lpDDSPrimary,
	ddraw_device->lpDDClipper)) != DD_OK)
	gfxDrvDDrawFailure("gfxDrvDDrawClipperInitialize: SetClipper() ", err);
    }
  }
  if (err != DD_OK)
    gfxDrvDDrawClipperRelease(ddraw_device);
  return (err == DD_OK);
}


/*==========================================================================*/
/* Activate the palette                                                     */
/*==========================================================================*/

BOOLE gfxDrvDDrawSetPalette(gfx_drv_ddraw_device *ddraw_device) {
  HRESULT err = DD_OK;
  
  if (ddraw_device->lpDDSPrimary != NULL) {
    err = IDirectDrawSurface_SetPalette(ddraw_device->lpDDSPrimary,
                                        ddraw_device->lpDDPalette);
    if (err != DD_OK)
      gfxDrvDDrawFailure("gfxDrvDDrawSurfacesInitialize(): SetPalette() ", err);
  }
  return (err == DD_OK);
}


/*==========================================================================*/
/* Releases the palette for the 8-bit display modes.                        */
/* Called on emulator shutdown                                              */
/*==========================================================================*/

void gfxDrvDDrawPaletteRelease(gfx_drv_ddraw_device *ddraw_device) {
  if (ddraw_device->lpDDPalette != NULL)
    IDirectDrawPalette_Release(ddraw_device->lpDDPalette);
  ddraw_device->lpDDPalette = NULL;
}


/*==========================================================================*/
/* Creates a palette for the 8-bit display modes.                           */
/* Called on emulator startup                                               */
/*==========================================================================*/

BOOLE gfxDrvDDrawPaletteInitialize(gfx_drv_ddraw_device *ddraw_device) {
  HRESULT err;
  
  if (!ddraw_device->PaletteInitialized) {
    if ((err = IDirectDraw2_CreatePalette(ddraw_device->lpDD2,
                                          DDPCAPS_8BIT, 
                                          gfx_drv_palette,
                                          &(ddraw_device->lpDDPalette),
                                          NULL)) != DD_OK)
      gfxDrvDDrawFailure("gfxDrvDDrawPaletteInitialize(): ", err);
    ddraw_device->PaletteInitialized = (err == DD_OK);
  }
  return ddraw_device->PaletteInitialized;
}


/*==========================================================================*/
/* Callback used to collect information about available DirectDraw devices  */
/* Called on emulator startup                                               */
/*==========================================================================*/

BOOL WINAPI gfxDrvDDrawDeviceEnumerate(GUID FAR *lpGUID,
				       LPSTR lpDriverDescription,
				       LPSTR lpDriverName,
				       LPVOID lpContext) {
  gfx_drv_ddraw_device *tmpdev;

  winDrvSetThreadName(-1, "WinFellow gfxDrvDDrawDeviceEnumerate()");
  
  tmpdev = (gfx_drv_ddraw_device *) malloc(sizeof(gfx_drv_ddraw_device));
  memset(tmpdev, 0, sizeof(gfx_drv_ddraw_device));
  if (lpGUID == NULL) {
    tmpdev->lpGUID = NULL;
    gfx_drv_ddraw_device_current = tmpdev;
  }
  else {
    tmpdev->lpGUID = (LPGUID) malloc(sizeof(GUID) + 16);
    memcpy(tmpdev->lpGUID, lpGUID, sizeof(GUID));
  }
  tmpdev->lpDriverDescription = (LPSTR) malloc(strlen(lpDriverDescription) + 1);
  strcpy(tmpdev->lpDriverDescription, lpDriverDescription);
  tmpdev->lpDriverName = (LPSTR) malloc(strlen(lpDriverName) + 1);
  strcpy(tmpdev->lpDriverName, lpDriverName);
  gfx_drv_ddraw_devices = listAddLast(gfx_drv_ddraw_devices, listNew(tmpdev));
  return DDENUMRET_OK;
}


/*==========================================================================*/
/* Dump information about available DirectDraw devices to log               */
/*==========================================================================*/

void gfxDrvDDrawDeviceInformationDump(void) {
  felist *l;
  STR s[120];
  
  sprintf(s,
    "gfxdrv; DirectDraw devices found: %d\n",
    listCount(gfx_drv_ddraw_devices));
  fellowAddLog(s);
  for (l = gfx_drv_ddraw_devices; l != NULL; l = listNext(l)) {
    gfx_drv_ddraw_device *tmpdev = (gfx_drv_ddraw_device *) listNode(l);
    sprintf(s,
      "gfxdrv: DirectDraw Driver Description: %s\n",
      tmpdev->lpDriverDescription);
    fellowAddLog(s);
    sprintf(s,
      "gfxdrv: DirectDraw Driver Name       : %s\n\n",
      tmpdev->lpDriverName);
    fellowAddLog(s);
  }
}


/*==========================================================================*/
/* Creates a list of available DirectDraw devices                           */
/* Called on emulator startup                                               */
/*==========================================================================*/

BOOLE gfxDrvDDrawDeviceInformationInitialize(void) {
  HRESULT err;
  
  gfx_drv_ddraw_devices = NULL;
  gfx_drv_ddraw_device_current = NULL;
  if ((err = DirectDrawEnumerate(gfxDrvDDrawDeviceEnumerate, NULL)) != DD_OK)
    gfxDrvDDrawFailure("gfxDrvDDrawDeviceInformationInitialize(), DirectDrawEnumerate(): ", err);
  if (gfx_drv_ddraw_device_current == NULL)
    gfx_drv_ddraw_device_current = (gfx_drv_ddraw_device *) listNode(gfx_drv_ddraw_devices);
  gfxDrvDDrawDeviceInformationDump();
  return (listCount(gfx_drv_ddraw_devices) > 0);
}


/*==========================================================================*/
/* Releases the list of available DirectDraw devices                        */
/* Called on emulator shutdown                                              */
/*==========================================================================*/

void gfxDrvDDrawDeviceInformationRelease(void) {
  felist *l;
  
  for (l = gfx_drv_ddraw_devices; l != NULL; l = listNext(l)) {
    gfx_drv_ddraw_device *tmpdev = (gfx_drv_ddraw_device *) listNode(l);
    if (tmpdev->lpGUID != NULL)
      free(tmpdev->lpGUID);
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

BOOLE gfxDrvDDraw1ObjectInitialize(gfx_drv_ddraw_device *ddraw_device) {
  HRESULT err;
  
  ddraw_device->lpDD = NULL;
  if ((err = DirectDrawCreate(ddraw_device->lpGUID,
    &(ddraw_device->lpDD),
    NULL)) != DD_OK) {
    gfxDrvDDrawFailure("gfxDrvDDraw1ObjectInitialize(): ", err);
    return FALSE;
  }
  return TRUE;
}


/*==========================================================================*/
/* Creates the DirectDraw2 object for the given DirectDraw device           */
/* Requires that a DirectDraw1 object has been initialized                  */
/* Called on emulator startup                                               */
/*==========================================================================*/

BOOLE gfxDrvDDraw2ObjectInitialize(gfx_drv_ddraw_device *ddraw_device) {
  HRESULT err;
  
  ddraw_device->lpDD2 = NULL;
  if ((err = IDirectDraw_QueryInterface(ddraw_device->lpDD,
    &IID_IDirectDraw2,
    (LPVOID *) &(ddraw_device->lpDD2))) != DD_OK) {
    gfxDrvDDrawFailure("gfxDrvDDraw2ObjectInitialize(): ", err);
    return FALSE;
  }
  else
  {
    DDCAPS caps;
    HRESULT res;
    memset(&caps, 0, sizeof(caps));
    caps.dwSize = sizeof(caps);
    res = IDirectDraw_GetCaps(ddraw_device->lpDD2, &caps, NULL);
    if (res != DD_OK) gfxDrvDDrawFailure("GetCaps()", res);
    else
    {
      ddraw_device->can_stretch_y = (caps.dwFXCaps & DDFXCAPS_BLTARITHSTRETCHY) ||
				    (caps.dwFXCaps & DDFXCAPS_BLTARITHSTRETCHYN) ||
				    (caps.dwFXCaps & DDFXCAPS_BLTSTRETCHY) ||
				    (caps.dwFXCaps & DDFXCAPS_BLTSHRINKYN);
      if (!ddraw_device->can_stretch_y) fellowAddLog("gfxdrv: WARNING: No hardware stretch\n");
      ddraw_device->no_dd_hardware = !!(caps.dwCaps & DDCAPS_NOHARDWARE);
      if (ddraw_device->no_dd_hardware) fellowAddLog("gfxdrv: WARNING: No DirectDraw hardware\n");
    }
  }
  return TRUE;
}


/*==========================================================================*/
/* Releases the DirectDraw1 object for the given DirectDraw device          */
/* Called on emulator startup                                               */
/*==========================================================================*/

BOOLE gfxDrvDDraw1ObjectRelease(gfx_drv_ddraw_device *ddraw_device) {
  HRESULT err = DD_OK;
  
  if (ddraw_device->lpDD != NULL)
  {
    if ((err = IDirectDraw_Release(ddraw_device->lpDD)) != DD_OK)
      gfxDrvDDrawFailure("gfxDrvDDraw1ObjectRelease(): ", err);
    ddraw_device->lpDD = NULL;
  }
  return (err == DD_OK);
}


/*==========================================================================*/
/* Releases the DirectDraw2 object for the given DirectDraw device          */
/* Called on emulator shutdown                                              */
/*==========================================================================*/

BOOLE gfxDrvDDraw2ObjectRelease(gfx_drv_ddraw_device *ddraw_device) {
  HRESULT err = DD_OK;
  
  if (ddraw_device->lpDD2 != NULL)
  {
    if ((err = IDirectDraw2_Release(ddraw_device->lpDD2)) != DD_OK)
      gfxDrvDDrawFailure("gfxDrvDDraw2ObjectRelease(): ", err);
    ddraw_device->lpDD2 = NULL;
  }
  return (err == DD_OK);
}


/*==========================================================================*/
/* Creates the DirectDraw object for the selected DirectDraw device         */
/* Called on emulator startup                                               */
/* Returns success or not                                                   */
/*==========================================================================*/

BOOLE gfxDrvDDrawObjectInitialize(gfx_drv_ddraw_device *ddraw_device) {
  BOOLE result;
  
  if ((result = gfxDrvDDraw1ObjectInitialize(ddraw_device))) {
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
					ULO depth) {
  felist *l;
  
  for (l = ddraw_device->modes; l != NULL; l = listNext(l)) {
    gfx_drv_ddraw_mode *tmpmode = (gfx_drv_ddraw_mode *) listNode(l);
    if (tmpmode->width == width &&
      tmpmode->height == height &&
      tmpmode->depth == depth)
      return tmpmode;
  }
  return NULL;
}


/*==========================================================================*/
/* Describes all the found modes to the draw module                         */
/* Called on emulator startup                                               */
/*==========================================================================*/

void gfxDrvDDrawModeInformationRegister(gfx_drv_ddraw_device *ddraw_device) {
  felist *l;
  ULO id;
  
  id = 0;
  for (l = ddraw_device->modes; l != NULL; l = listNext(l)) {
    gfx_drv_ddraw_mode *ddmode = listNode(l);
    draw_mode *mode = (draw_mode *) malloc(sizeof(draw_mode));
    
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
    if (!ddmode->windowed) {
      char hz[16];
      if (ddmode->refresh != 0) sprintf(hz, "%dHZ", ddmode->refresh);
      else hz[0] = 0;
      sprintf(mode->name,
      "%dWx%dHx%dBPPx%s",
      ddmode->width,
      ddmode->height,
      ddmode->depth,
      hz);
    }
    else {
      sprintf(mode->name,
      "%dWx%dHxWindow",
      ddmode->width,
      ddmode->height);
    }
    drawModeAdd(mode);
    id++;
  }
}


/*==========================================================================*/
/* Examines RGB masks for the mode information initializer                  */
/* Called on emulator startup                                               */
/*==========================================================================*/

ULO gfxDrvRGBMaskPos(ULO mask) {
  ULO i;
  
  for (i = 0; i < 32; i++)
    if (mask & (1<<i))
      return i;
    return 0;
}

ULO gfxDrvRGBMaskSize(ULO mask) {
  ULO i, sz;
  
  sz = 0;
  for (i = 0; i < 32; i++)
    if (mask & (1<<i))
      sz++;
    return sz;
}

/*==========================================================================*/
/* Dump information about available modes                                   */
/*==========================================================================*/

void gfxDrvDDrawModeInformationDump(gfx_drv_ddraw_device *ddraw_device) {
  felist *l;
  STR s[120];
  
  sprintf(s,
    "gfxdrv: DirectDraw modes found: %d\n",
    listCount(ddraw_device->modes));
  fellowAddLog(s);
  for (l = ddraw_device->modes; l != NULL; l = listNext(l)) {
    gfx_drv_ddraw_mode *tmpmode = (gfx_drv_ddraw_mode *) listNode(l);
    if (!tmpmode->windowed)
      sprintf(s,
      "gfxdrv: Mode Description: %dWx%dHx%dBPPx%dHZ (%d,%d,%d,%d,%d,%d)\n",
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
    else
      sprintf(s,
      "gfxdrv: Mode Description: %dWx%dHxWindow\n",
      tmpmode->width,
      tmpmode->height);
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
				       BOOLE windowed) {
  gfx_drv_ddraw_mode *tmpmode = (gfx_drv_ddraw_mode *) malloc(sizeof(gfx_drv_ddraw_mode));
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

BOOL WINAPI gfxDrvDDrawModeEnumerate(LPDDSURFACEDESC lpDDSurfaceDesc,
				     LPVOID lpContext) {
  winDrvSetThreadName(-1, "WinFellow gfxDrvDDrawModeEnumerate()");
  if (((lpDDSurfaceDesc->ddpfPixelFormat.dwFlags == DDPF_RGB) &&
    ((lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount == 16) ||
    (lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount == 24) ||
    (lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount == 32))) ||
    (lpDDSurfaceDesc->ddpfPixelFormat.dwFlags == (DDPF_PALETTEINDEXED8 | DDPF_RGB))) {
    gfx_drv_ddraw_mode *tmpmode;
    gfx_drv_ddraw_device *ddraw_device;
    
    ddraw_device = (gfx_drv_ddraw_device *) lpContext;
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

BOOL gfxDrvDDrawModeInformationInitialize(gfx_drv_ddraw_device *ddraw_device) {
  HRESULT err;
  BOOLE result;
  
  ddraw_device->modes = NULL;
  if ((err = IDirectDraw2_EnumDisplayModes(ddraw_device->lpDD2,
    DDEDM_REFRESHRATES,
    NULL,
    (LPVOID) ddraw_device,
    gfxDrvDDrawModeEnumerate)) != DD_OK) {
    gfxDrvDDrawFailure("gfxDrvDDrawModeInformationInitialize(): ", err);
    result = FALSE;
  }
  else {
    if ((result = (listCount(ddraw_device->modes) != 0))) {
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(320, 200, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(320, 256, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(320, 288, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(320, 400, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(320, 512, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(320, 576, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(376, 200, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(376, 256, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(376, 288, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(376, 400, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(376, 512, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(376, 576, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(640, 200, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(640, 256, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(640, 288, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(640, 400, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(640, 512, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(640, 576, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(752, 200, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(752, 256, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(752, 288, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(752, 400, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(752, 512, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
      listAddLast(ddraw_device->modes, listNew(gfxDrvDDrawModeNew(752, 576, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
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

void gfxDrvDDrawModeInformationRelease(gfx_drv_ddraw_device *ddraw_device) {
  listFreeAll(ddraw_device->modes, TRUE);
  ddraw_device->modes = NULL;
}


/*==========================================================================*/
/* Set cooperative level                                                    */
/*==========================================================================*/

BOOL gfxDrvDDrawSetCooperativeLevelNormal(gfx_drv_ddraw_device *ddraw_device) {
  HRESULT err;
  
  if ((err = IDirectDraw2_SetCooperativeLevel(ddraw_device->lpDD2,
    gfx_drv_hwnd,
    DDSCL_NORMAL)) != DD_OK)
    gfxDrvDDrawFailure("gfxDrvDDrawSetCooperativeLevelNormal(): ", err);
  return (err == DD_OK);
}


BOOL gfxDrvDDrawSetCooperativeLevelExclusive(gfx_drv_ddraw_device *ddraw_device) {
  HRESULT err;
  
  if ((err = IDirectDraw2_SetCooperativeLevel(ddraw_device->lpDD2,
    gfx_drv_hwnd,
    DDSCL_FULLSCREEN |
    DDSCL_EXCLUSIVE)) != DD_OK)
    gfxDrvDDrawFailure("gfxDrvDDrawSetCooperativeLevelExclusive(): ", err);
  return (err == DD_OK);
}


BOOL gfxDrvDDrawSetCooperativeLevel(gfx_drv_ddraw_device *ddraw_device) {
  if (ddraw_device->mode->windowed)
    return gfxDrvDDrawSetCooperativeLevelNormal(ddraw_device);
  return gfxDrvDDrawSetCooperativeLevelExclusive(ddraw_device);
}


/*==========================================================================*/
/* Clear surface with blitter                                               */
/*==========================================================================*/

void gfxDrvDDrawSurfaceClear(LPDIRECTDRAWSURFACE surface) {
  HRESULT err;
  DDBLTFX ddbltfx;
  
  memset(&ddbltfx, 0, sizeof(ddbltfx));
  ddbltfx.dwSize = sizeof(ddbltfx);
  ddbltfx.dwFillColor = 0;
  if ((err = IDirectDrawSurface_Blt(surface,
                                    NULL,
                                    NULL,
                                    NULL,
                                    DDBLT_COLORFILL | DDBLT_WAIT,
                                    &ddbltfx)) != DD_OK)
    gfxDrvDDrawFailure("gfxDrvDDrawSurfaceClear(): ", err);
  fellowAddLog("gfxdrv: Clearing surface\n");
}


/*==========================================================================*/
/* Restore a surface, do some additional cleaning up.                       */
/*==========================================================================*/

HRESULT gfxDrvDDrawSurfaceRestore(gfx_drv_ddraw_device *ddraw_device,
				  LPDIRECTDRAWSURFACE surface) {
  HRESULT err;
  if (IDirectDrawSurface_IsLost(surface) != DDERR_SURFACELOST) return DD_OK;

  if ((err = IDirectDrawSurface_Restore(surface)) == DD_OK) {
    gfxDrvDDrawSurfaceClear(surface);
    if ((surface == ddraw_device->lpDDSPrimary) &&
        (ddraw_device->buffercount > 1)) {
      gfxDrvDDrawSurfaceClear(ddraw_device->lpDDSBack);
      if (ddraw_device->buffercount == 3)
        if ((err = IDirectDrawSurface_Flip(surface,
                                           NULL,
                                           DDFLIP_WAIT)) != DD_OK)
          gfxDrvDDrawFailure("gfxDrvDDrawSurfaceRestore(), Flip(): ", err);
	else
	  gfxDrvDDrawSurfaceClear(ddraw_device->lpDDSBack);
    }
    graphLineDescClearSkips();
  }
  return err;
}
    
/*==========================================================================*/
/* Blit secondary buffer to the primary surface                             */
/*==========================================================================*/

void gfxDrvDDrawSurfaceBlit(gfx_drv_ddraw_device *ddraw_device) {
  HRESULT err;
  RECT srcwin;
  RECT dstwin;
  LPDIRECTDRAWSURFACE lpDDSDestination;
  DDBLTFX bltfx;

  memset(&bltfx, 0, sizeof(DDBLTFX));
  bltfx.dwSize = sizeof(DDBLTFX);

  /* When using the blitter to show our buffer, */
  /* source surface is always the secondary off-screen buffer */

  /* Srcwin is used when we do vertical scaling */
  /* Prevent horisontal scaling of the offscreen buffer */
  if (ddraw_device->mode->windowed && !gfx_drv_stretch_always)
  {
    srcwin.left = 0;
    srcwin.right = ddraw_device->mode->width;
    srcwin.top = 0;
    srcwin.bottom = (ddraw_device->mode->height >> (ddraw_device->vertical_scale - 1));
  }
  else
  {
    srcwin.left = draw_hoffset;
    srcwin.right = draw_hoffset + draw_width_amiga_real;
    srcwin.top = draw_voffset;
    srcwin.bottom = draw_voffset + (draw_height_amiga_real >> (ddraw_device->vertical_scale - 1));
  }
  /* Destination is always the primary or one the backbuffers attached to it */
  gfxDrvDDrawBlitTargetSurfaceSelect(ddraw_device, &lpDDSDestination);
  /* Destination window, in windowed mode, use the clientrect */
  if (!ddraw_device->mode->windowed) {
    /* In full-screen mode, blit centered to the screen */
    if (!gfx_drv_stretch_always)
    {
      dstwin.left = draw_hoffset;
      dstwin.top = draw_voffset;
      dstwin.right = draw_hoffset + draw_width_amiga_real;
      dstwin.bottom = draw_voffset + draw_height_amiga_real;
    }
    else
    {
      dstwin.left = 0;
      dstwin.top = 0;
      dstwin.right = ddraw_device->mode->width;
      dstwin.bottom = ddraw_device->mode->height;
    }
  }

  /* This can fail when a surface is lost */
  if ((err = IDirectDrawSurface_Blt(lpDDSDestination,
                                    (ddraw_device->mode->windowed) ?
				      &ddraw_device->hwnd_clientrect_screen :
		                      &dstwin,
                                    ddraw_device->lpDDSSecondary,
                                    ((ddraw_device->vertical_scale == 1) && !gfx_drv_stretch_always) ? NULL : &srcwin,
                                    DDBLT_ASYNC,
                                    &bltfx)) != DD_OK) {
    gfxDrvDDrawFailure("gfxDrvDDrawSurfaceBlit(): ", err);
    if (err == DDERR_SURFACELOST) {
      /* Reclaim surface, if that also fails, pass over the blit and hope it */
      /* gets better later */
      if ((err = gfxDrvDDrawSurfaceRestore(ddraw_device, ddraw_device->lpDDSPrimary)) != DD_OK) {
	/* Here we are in deep trouble, we can not provide a buffer pointer */
	gfxDrvDDrawFailure("gfxDrvDDrawSurfaceBlit(): (Restore primary surface failed) ", err);
	return;
      }
      if ((err = gfxDrvDDrawSurfaceRestore(ddraw_device, ddraw_device->lpDDSSecondary)) != DD_OK) {
	/* Here we are in deep trouble, we can not provide a buffer pointer */
	gfxDrvDDrawFailure("gfxDrvDDrawSurfaceBlit(): (Restore secondary surface failed) ", err);
	return;
      }
      /* Restore was successful, do the blit */
      if ((err = IDirectDrawSurface_Blt(lpDDSDestination,
                                        (ddraw_device->mode->windowed) ?
				          &ddraw_device->hwnd_clientrect_screen :
		                          &dstwin,
                                        ddraw_device->lpDDSSecondary,
                                        (ddraw_device->vertical_scale == 1) ? NULL : &srcwin,
                                        DDBLT_ASYNC,
                                        &bltfx)) != DD_OK) {
	/* Failed second time, pass over */
        gfxDrvDDrawFailure("gfxDrvDDrawSurfaceBlit(): (Blit failed after restore) ", err);
      }
    }
  }
}


/*==========================================================================*/
/* Release the surfaces                                                     */
/*==========================================================================*/

void gfxDrvDDrawSurfacesRelease(gfx_drv_ddraw_device *ddraw_device) {
  HRESULT err;
  
  if (ddraw_device->lpDDSPrimary != NULL) {
    if ((err = IDirectDrawSurface_Release(ddraw_device->lpDDSPrimary)) != DD_OK) {
      gfxDrvDDrawFailure("gfxDrvDDrawSurfacesRelease(): ", err);
    }
    ddraw_device->lpDDSPrimary = NULL;
    if (ddraw_device->mode->windowed)
      gfxDrvDDrawClipperRelease(ddraw_device);
  }
  ddraw_device->buffercount = 0;
  if (ddraw_device->lpDDSSecondary != NULL) {
    if ((err = IDirectDrawSurface_Release(ddraw_device->lpDDSSecondary)) != DD_OK)
      gfxDrvDDrawFailure("gfxDrvDDrawSurfacesRelease(): ", err);
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

char *gfxDrvDDrawVideomemLocationStr(ULO pass) {
  switch (pass) {
    case 0: return "local videomemory (on card)";
    case 1: return "non-local videomemory (AGP shared mem)";
    case 2: return "system memory";
  }
  return "unknown memory";
}

ULO gfxDrvDDrawVideomemLocationFlags(ULO pass) {
  switch (pass) {
    case 0: return DDSCAPS_VIDEOMEMORY; /* Local videomemory (default oncard) */
    case 1: return DDSCAPS_NONLOCALVIDMEM | DDSCAPS_VIDEOMEMORY; /* Shared videomemory */
    case 2: return DDSCAPS_SYSTEMMEMORY; /* Plain memory */
  }
  return 0;
}

BOOLE
gfxDrvDDrawCreateSecondaryOffscreenSurface(gfx_drv_ddraw_device *ddraw_device) {
  ULO pass;
  BOOLE buffer_allocated;
  BOOLE result = TRUE;
  HRESULT err;

  pass = 0;
  buffer_allocated = FALSE;
  while ((pass < 3) && !buffer_allocated) {
    ddraw_device->ddsdSecondary.dwSize = sizeof(ddraw_device->ddsdSecondary);
    ddraw_device->ddsdSecondary.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddraw_device->ddsdSecondary.ddsCaps.dwCaps = gfxDrvDDrawVideomemLocationFlags(pass)
						 | DDSCAPS_OFFSCREENPLAIN;
    ddraw_device->ddsdSecondary.dwHeight = ddraw_device->drawmode->height;
    ddraw_device->ddsdSecondary.dwWidth = ddraw_device->drawmode->width;
    if ((err = IDirectDraw2_CreateSurface(ddraw_device->lpDD2,
                                          &(ddraw_device->ddsdSecondary),
                                          &(ddraw_device->lpDDSSecondary),
                                          NULL)) != DD_OK) {
      gfxDrvDDrawFailure("gfxDrvDDrawCreateSecondaryOffscreenSurface() ", err);
      fellowAddLog("gfxdrv: Failed to allocate second offscreen surface in %s\n",
		   gfxDrvDDrawVideomemLocationStr(pass));
      result = FALSE;
    }
    else
    {
      buffer_allocated = TRUE;
      fellowAddLog("gfxdrv: Allocated second offscreen surface in %s (%d, %d)\n",
		   gfxDrvDDrawVideomemLocationStr(pass),
		   ddraw_device->drawmode->width, 
		   ddraw_device->drawmode->height);
      gfxDrvDDrawSurfaceClear(ddraw_device->lpDDSSecondary);
      result = TRUE;
    }
    pass++;
  }
  return result && buffer_allocated;
}

/*==========================================================================*/
/* Create the surfaces                                                      */
/* Called before emulation start                                            */
/*==========================================================================*/

ULO gfxDrvDDrawSurfacesInitialize(gfx_drv_ddraw_device *ddraw_device) {
  HRESULT err;
  ULO buffer_count_want;
  DDSCAPS ddscaps;
  BOOLE success = TRUE;

  if (ddraw_device->use_blitter)
     success = gfxDrvDDrawCreateSecondaryOffscreenSurface(ddraw_device);
  if (!success) return 0;

  /* Want x backbuffers first, then reduce it until it succeeds */
  buffer_count_want = (ddraw_device->use_blitter) ? 1 : ddraw_device->maxbuffercount;
  success = FALSE;
  while (buffer_count_want > 0 && !success) {
    success = TRUE;
    if (ddraw_device->lpDDSPrimary != NULL)
      gfxDrvDDrawSurfacesRelease(ddraw_device);
    ddraw_device->ddsdPrimary.dwSize = sizeof(ddraw_device->ddsdPrimary);
    ddraw_device->ddsdPrimary.dwFlags = DDSD_CAPS;
    ddraw_device->ddsdPrimary.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    if (buffer_count_want > 1) {
      ddraw_device->ddsdPrimary.dwFlags |= DDSD_BACKBUFFERCOUNT;
      ddraw_device->ddsdPrimary.ddsCaps.dwCaps |= DDSCAPS_FLIP |
						  DDSCAPS_COMPLEX;
      ddraw_device->ddsdPrimary.dwBackBufferCount = buffer_count_want - 1;
      ddraw_device->ddsdBack.dwSize = sizeof(ddraw_device->ddsdBack);
    }
    if ((err = IDirectDraw2_CreateSurface(ddraw_device->lpDD2,
                                          &(ddraw_device->ddsdPrimary),
                                          &(ddraw_device->lpDDSPrimary),
                                          NULL)) != DD_OK) {
      gfxDrvDDrawFailure("gfxDrvDDrawSurfacesInitialize(): ", err);
      fellowAddLog("gfxdrv: Failed to allocate primary surface with %d backbuffers\n", buffer_count_want - 1);
      success = FALSE;
    }
    else { /* Here we have got a buffer, clear it */
      fellowAddLog("gfxdrv: Allocated primary surface with %d backbuffers\n", buffer_count_want - 1);
      if (buffer_count_want > 1) {
	ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
	if ((err = IDirectDrawSurface_GetAttachedSurface(ddraw_device->lpDDSPrimary,
	                                                 &ddscaps,
	                                                 &(ddraw_device->lpDDSBack))) != DD_OK) {
	  gfxDrvDDrawFailure("gfxDrvDDrawSurfacesInitialize(): (GetAttachedSurface()) ", err);
          success = FALSE;
	}
	else { /* Have a backbuffer, seems to work fine */
          gfxDrvDDrawSurfaceClear(ddraw_device->lpDDSPrimary);
          gfxDrvDDrawSurfaceClear(ddraw_device->lpDDSBack);
	  if (buffer_count_want > 2)
            if ((err = IDirectDrawSurface_Flip(ddraw_device->lpDDSPrimary,
                                               NULL,
                                               DDFLIP_WAIT)) != DD_OK) {
	      gfxDrvDDrawFailure("gfxDrvDDrawSurfacesInitialize(), Flip(): ", err);
              success = FALSE;
	    }
	    else
	      gfxDrvDDrawSurfaceClear(ddraw_device->lpDDSBack);
	}
      }
      else { /* No backbuffer, don't clear if we are in a window */
	if (!ddraw_device->mode->windowed)
	  gfxDrvDDrawSurfaceClear(ddraw_device->lpDDSPrimary);
      }
    }
    if (!success) buffer_count_want--;
  }
  
  /* Here we have created a primary buffer, possibly with backbuffers */
  
  if (success) {
    DDPIXELFORMAT ddpf;
    memset(&ddpf, 0, sizeof(ddpf));
    ddpf.dwSize = sizeof(ddpf);
    if ((err = IDirectDrawSurface_GetPixelFormat(ddraw_device->lpDDSPrimary,
                                                 &ddpf)) != DD_OK) {
      /* We failed to find the pixel format */
      gfxDrvDDrawFailure("gfxDrvDDrawSurfacesInitialize(): GetPixelFormat() ", err);
      gfxDrvDDrawSurfacesRelease(ddraw_device);
      success = FALSE;
    }
    else {
      /* Examine pixelformat */
      if (((ddpf.dwFlags == DDPF_RGB) &&
	((ddpf.dwRGBBitCount == 16) ||
	(ddpf.dwRGBBitCount == 24) ||
	(ddpf.dwRGBBitCount == 32))) ||
	(ddpf.dwFlags == (DDPF_PALETTEINDEXED8 | DDPF_RGB))) {
	ddraw_device->drawmode->bits = ddpf.dwRGBBitCount;
        ddraw_device->drawmode->redpos = (ddpf.dwFlags == DDPF_RGB) ? gfxDrvRGBMaskPos(ddpf.dwRBitMask) : 0;
	ddraw_device->drawmode->redsize = (ddpf.dwFlags == DDPF_RGB) ? gfxDrvRGBMaskSize(ddpf.dwRBitMask) : 0;
        ddraw_device->drawmode->greenpos = (ddpf.dwFlags == DDPF_RGB) ? gfxDrvRGBMaskPos(ddpf.dwGBitMask) : 0;
	ddraw_device->drawmode->greensize = (ddpf.dwFlags == DDPF_RGB) ? gfxDrvRGBMaskSize(ddpf.dwGBitMask) : 0;
        ddraw_device->drawmode->bluepos = (ddpf.dwFlags == DDPF_RGB) ? gfxDrvRGBMaskPos(ddpf.dwBBitMask) : 0;
	ddraw_device->drawmode->bluesize = (ddpf.dwFlags == DDPF_RGB) ? gfxDrvRGBMaskSize(ddpf.dwBBitMask) : 0;
	
	/* Set palette and clipper */
	
	if (ddraw_device->drawmode->bits == 8) {
	  if (!gfxDrvDDrawSetPalette(ddraw_device)) {
	    gfxDrvDDrawSurfacesRelease(ddraw_device);
	    success = FALSE;
	  }
	}
	if (success && (ddraw_device->mode->windowed)) {
	  if (!gfxDrvDDrawClipperInitialize(ddraw_device)) {
	    gfxDrvDDrawSurfacesRelease(ddraw_device);
	    success = FALSE;
	  }
	}
      }
      else { /* Unsupported pixel format..... */
	fellowAddLog("gfxdrv: gfxDrvDDrawSurfacesInitialized(): Window mode - unsupported Pixelformat\n");
	gfxDrvDDrawSurfacesRelease(ddraw_device);
	success = FALSE;
      }
    }
  }
  ddraw_device->buffercount = (success) ? buffer_count_want : 0;
  return ddraw_device->buffercount;
}

/*==========================================================================*/
/* Lock the current drawing surface                                         */
/*==========================================================================*/

UBY *gfxDrvDDrawSurfaceLock(gfx_drv_ddraw_device *ddraw_device, ULO *pitch) {
  HRESULT err;
  LPDIRECTDRAWSURFACE lpDDS;
  LPDDSURFACEDESC lpDDSD;
  
  gfxDrvDDrawDrawTargetSurfaceSelect(ddraw_device, &lpDDS, &lpDDSD);
  err = IDirectDrawSurface_Lock(lpDDS,
				NULL,
				lpDDSD,
				DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT,
				NULL);
  /* We sometimes have a pointer to a backbuffer, but only primary surfaces can
     be restored, it implicitly recreates backbuffers as well */
  if (err != DD_OK)
  {
    gfxDrvDDrawFailure("gfxDrvDDrawSurfaceLock(): ", err);
    if (err == DDERR_SURFACELOST) {
      /* Here we have lost the surface, restore it */
      /* Unlike when blitting, we only use 1 surface here */
      if ((err = gfxDrvDDrawSurfaceRestore(ddraw_device, (ddraw_device->mode->windowed || (ddraw_device->vertical_scale > 1)) ? lpDDS : ddraw_device->lpDDSPrimary)) != DD_OK) {
	/* Here we are in deep trouble, we can not provide a buffer pointer */
	gfxDrvDDrawFailure("gfxDrvDDrawSurfaceLock(): (Failed to restore surface 1) ", err);
	return NULL;
      }
      else { /* Try again to lock the surface */
	if ((err = IDirectDrawSurface_Lock(lpDDS,
	                                   NULL,
	                                   lpDDSD,
	                                   DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT,
	                                   NULL)) != DD_OK) {
	  /* Here we are in deep trouble, we can not provide a buffer pointer */
	  gfxDrvDDrawFailure("gfxDrvDDrawSurfaceLock(): (Lock failed after restore) ", err);
	  return NULL;
	}
      }      
    }
    else {
      /* Here we are in deep trouble, we can not provide a buffer pointer */
      /* The error is something else than lost surface */
      fellowAddLog("gfxdrv: gfxDrvDDrawSurfaceLock(): (Mega problem 3)\n");	
      return NULL;
    }
  }
  *pitch = lpDDSD->lPitch;  
  return lpDDSD->lpSurface;
}


/*==========================================================================*/
/* Unlock the primary surface                                               */
/*==========================================================================*/

void gfxDrvDDrawSurfaceUnlock(gfx_drv_ddraw_device *ddraw_device) {
  HRESULT err;
  LPDIRECTDRAWSURFACE lpDDS;
  LPDDSURFACEDESC lpDDSD;
  
  gfxDrvDDrawDrawTargetSurfaceSelect(ddraw_device, &lpDDS, &lpDDSD);
  if ((err = IDirectDrawSurface_Unlock(lpDDS,
				       lpDDSD->lpSurface)) != DD_OK)
    gfxDrvDDrawFailure("gfxDrvSurfaceUnlock(): ", err);
}


/*==========================================================================*/
/* Flip to next buffer                                                      */
/*==========================================================================*/

void gfxDrvDDrawFlip(gfx_drv_ddraw_device *ddraw_device) {
  HRESULT err;
  
  if (ddraw_device->use_blitter)     /* Blit secondary buffer to primary */
    gfxDrvDDrawSurfaceBlit(ddraw_device);
  if (ddraw_device->buffercount > 1)    /* Flip buffer if there are several */
    if ((err = IDirectDrawSurface_Flip(ddraw_device->lpDDSPrimary,
      NULL,
      DDFLIP_WAIT)) != DD_OK)
      gfxDrvDDrawFailure("gfxDrvDDrawFlip(): ", err);
}


/*==========================================================================*/
/* Set specified mode                                                       */
/* In windowed mode, we don't set a mode, but instead queries the pixel-    */
/* format from the surface (gfxDrvDDrawSurfacesInitialize())                */
/* Called before emulation start                                            */
/*==========================================================================*/

ULO gfxDrvDDrawSetMode(gfx_drv_ddraw_device *ddraw_device) {
  BOOLE result = TRUE;
  ULO buffers = 0;
  
  if (gfxDrvDDrawSetCooperativeLevel(ddraw_device)) {
    ddraw_device->use_blitter = ((ddraw_device->mode->windowed) ||
			         (ddraw_device->vertical_scale > 1) ||
				 (ddraw_device->no_dd_hardware) ||
				 (gfx_drv_stretch_always));
    if (!ddraw_device->mode->windowed) {
      gfx_drv_ddraw_mode *mode;
      HRESULT err;
      DDSURFACEDESC myDDSDesc;
      
      mode = (gfx_drv_ddraw_mode *) listNode(listIndex(ddraw_device->modes, ddraw_device->drawmode->id));
      memset(&myDDSDesc, 0, sizeof(myDDSDesc));
      myDDSDesc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
      if ((err = ddraw_device->lpDD2->lpVtbl->SetDisplayMode(ddraw_device->lpDD2,
	                                                     mode->width,
	                                                     mode->height,
	                                                     mode->depth,
	                                                     mode->refresh,
	                                                     0)) != DD_OK) {
	gfxDrvDDrawFailure("gfxDrvDDrawSetMode(): ", err);
	result = FALSE;
      }
    }
    if (result) {
      gfxDrvDDrawPaletteInitialize(gfx_drv_ddraw_device_current);
      if ((buffers = gfxDrvDDrawSurfacesInitialize(gfx_drv_ddraw_device_current)) == 0)
	gfxDrvDDrawSetCooperativeLevelNormal(gfx_drv_ddraw_device_current);
    }
  }
  return buffers;
}


/*==========================================================================*/
/* Open the default DirectDraw device, and record information about what is */
/* available.                                                               */
/* Called on emulator startup                                               */
/*==========================================================================*/

BOOLE gfxDrvDDrawInitialize(void) {
  BOOLE result;
  
  if ((result = gfxDrvDDrawDeviceInformationInitialize())) {
    if ((result = gfxDrvDDrawObjectInitialize(gfx_drv_ddraw_device_current))) {
      if (!(result = gfxDrvDDrawModeInformationInitialize(gfx_drv_ddraw_device_current))) {
	gfxDrvDDraw2ObjectRelease(gfx_drv_ddraw_device_current);
	gfxDrvDDrawDeviceInformationRelease();
      }
    }
    else
      gfxDrvDDrawDeviceInformationRelease();
  }
  return result;
}


/*==========================================================================*/
/* Release any resources allocated by gfxDrvDDrawInitialize                 */
/* Called on emulator shutdown                                              */
/*==========================================================================*/

void gfxDrvDDrawRelease(void) {
  gfxDrvDDrawModeInformationRelease(gfx_drv_ddraw_device_current);
  gfxDrvDDrawPaletteRelease(gfx_drv_ddraw_device_current);
  gfxDrvDDraw2ObjectRelease(gfx_drv_ddraw_device_current);
  gfxDrvDDrawDeviceInformationRelease();
}

/*==========================================================================*/
/* Flag to enable stretch of source image to any screen resolution          */
/*==========================================================================*/

void gfxDrvSetStretchAlways(BOOLE stretch_always)
{
  gfx_drv_stretch_always = stretch_always;
}

/*==========================================================================*/
/* Functions below are the actual "graphics driver API"                     */
/*==========================================================================*/


/*==========================================================================*/
/* Set an 8 bit palette entry                                               */
/*==========================================================================*/

void gfxDrvSet8BitColor(ULO col, ULO r, ULO g, ULO b) {
  gfx_drv_palette[col].peRed = (UBY) r;
  gfx_drv_palette[col].peGreen = (UBY) g;
  gfx_drv_palette[col].peBlue = (UBY) b;
  gfx_drv_palette[col].peFlags = PC_RESERVED;
}


/*==========================================================================*/
/* Locks surface and puts a valid framebuffer pointer in framebuffer        */
/*==========================================================================*/

UBY *gfxDrvValidateBufferPointer(void) {
  UBY *buffer;
  ULO pitch, bufferULO;
  
  gfxDrvRunEventWait();
  if (gfx_drv_displaychange) {
    fellowAddLog("gfxdrv: Displaymode change\n");
    soundEmulationStop();
    gameportEmulationStop();
    kbdEmulationStop();
    drawEmulationStop();
    drawEmulationStart();
    gameportEmulationStart();
    kbdEmulationStart();
    soundEmulationStart();
    drawEmulationStartPost();
  }
  buffer = gfxDrvDDrawSurfaceLock(gfx_drv_ddraw_device_current, &pitch);
  if (buffer != NULL) {
    gfx_drv_ddraw_device_current->drawmode->pitch = pitch;
    
    /* Align pointer returned to drawing routines */
    
    bufferULO = (ULO) buffer;
    if (gfx_drv_ddraw_device_current->drawmode->bits == 32) {
      if ((bufferULO & 0x7) != 0)
	buffer = (UBY *) (bufferULO & 0xfffffff8) + 8;
    }
    if (gfx_drv_ddraw_device_current->drawmode->bits == 15 || gfx_drv_ddraw_device_current->drawmode->bits == 16) {
      if ((bufferULO & 0x3) != 0)
	buffer = (UBY *) (bufferULO & 0xfffffffc) + 4;
    }
  }
  return buffer;
}


/*==========================================================================*/
/* Unlocks surface                                                          */
/*==========================================================================*/

void gfxDrvInvalidateBufferPointer(void) {
  gfxDrvDDrawSurfaceUnlock(gfx_drv_ddraw_device_current);
}


/*==========================================================================*/
/* Flip the current buffer                                                  */
/*==========================================================================*/

void gfxDrvBufferFlip(void) {
  gfxDrvDDrawFlip(gfx_drv_ddraw_device_current);
}


/*==========================================================================*/
/* Set the specified mode                                                   */
/* Returns the number of available buffers, zero is error                   */
/*==========================================================================*/

void gfxDrvSetMode(draw_mode *dm, ULO vertical_scale) {
  gfx_drv_ddraw_device_current->drawmode = dm;  
  gfx_drv_ddraw_device_current->vertical_scale = vertical_scale;
  gfx_drv_ddraw_device_current->mode = (gfx_drv_ddraw_mode *) listNode(listIndex(gfx_drv_ddraw_device_current->modes, dm->id));
}


/*==========================================================================*/
/* Emulation is starting                                                    */
/* Called on emulation start                                                */
/* Subtlety: In exclusive mode, the window that is attached to the device   */
/* appears to become activated, even if it is not shown at the time.        */
/* The WM_ACTIVATE message triggers DirectInput acquisition, which means    */
/* that the DirectInput object needs to have been created at that time.     */
/* Unfortunately, the window must be created as well in order to attach DI  */
/* objects to it. So we create window, create DI objects in between and then*/
/* do the rest of the gfx init.                                             */
/* That is why the gfxDrvEmulationStart is split in two.                    */
/*==========================================================================*/

BOOLE gfxDrvEmulationStart(ULO maxbuffercount) {
  gfxDrvRunEventReset();                    /* At this point, app is paused */
  gfx_drv_ddraw_device_current->maxbuffercount = maxbuffercount;
  gfx_drv_win_active = FALSE;
  gfx_drv_win_active_original = FALSE;
  gfx_drv_win_minimized_original = FALSE;
  gfx_drv_syskey_down = FALSE;
  gfx_drv_displaychange = FALSE;
  if ((gfx_drv_ddraw_device_current->vertical_scale != 1) &&
      (!gfx_drv_ddraw_device_current->can_stretch_y))
    if (!gfx_drv_ddraw_device_current->stretch_warning_shutup)
    {
      gfx_drv_ddraw_device_current->stretch_warning_shutup = TRUE;
      wguiRequester("Double Lines were selected.",
		    "Hardware assisted scaling is not supported by this computer",
		    "Use scanlines instead if performance is poor");
    }
  if (!gfxDrvWindowInitialize(gfx_drv_ddraw_device_current)) {
    fellowAddLog("gfxdrv: gfxDrvEmulationStart(): Failed to create window\n");
    return FALSE;
  }
  return TRUE;
}

ULO gfxDrvEmulationStartPost(void) {
  ULO buffers;

  if ((buffers = gfxDrvDDrawSetMode(gfx_drv_ddraw_device_current)) == 0)
    fellowAddLog("gfxdrv: gfxDrvEmulationStart(): Zero buffers, gfxDrvDDSetMode() failed\n");
  if (gfx_drv_hwnd != NULL)
    gfxDrvWindowShow(gfx_drv_ddraw_device_current);
  return buffers;
}


/*==========================================================================*/
/* Emulation is stopping, clean up current videomode                        */
/*==========================================================================*/

void gfxDrvEmulationStop(void) {
  gfxDrvDDrawSurfacesRelease(gfx_drv_ddraw_device_current);
  gfxDrvDDrawSetCooperativeLevelNormal(gfx_drv_ddraw_device_current);
  gfxDrvWindowRelease(gfx_drv_ddraw_device_current);
}


/*==========================================================================*/
/* End of frame handler                                                     */
/*==========================================================================*/

void gfxDrvEndOfFrame(void) {
}


/*==========================================================================*/
/* Collects information about the DirectX capabilities of this computer     */
/* After this, a DDraw object exists.                                       */
/* Called on emulator startup                                               */
/*==========================================================================*/

BOOLE gfxDrvStartup(void) {
  gfxDrvSetStretchAlways(FALSE);
  gfxdrv_ini = iniManagerGetCurrentInitdata(&ini_manager);
  gfx_drv_initialized = FALSE;
  gfx_drv_app_run = NULL;
  if (gfxDrvRunEventInitialize())
    if (gfxDrvWindowClassInitialize())
      gfx_drv_initialized = gfxDrvDDrawInitialize();
  if (!gfx_drv_initialized) gfxDrvRunEventRelease();
  return gfx_drv_initialized;
}


/*==========================================================================*/
/* Free DDraw resources                                                     */
/* Called on emulator shutdown                                              */
/*==========================================================================*/

void gfxDrvShutdown(void) {
  if (gfx_drv_initialized) {
    gfxDrvDDrawRelease();
    gfxDrvWindowRelease(gfx_drv_ddraw_device_current);
  }
  gfxDrvRunEventRelease();
}
