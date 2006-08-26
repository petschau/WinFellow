/*===========================================================================*/
/* Fellow Amiga Emulator                                                     */
/* Host framebuffer driver                                                   */
/* SDL implementation                                                        */
/* Author: Worfje (worfje@gmx.net)                                           */
/*                                                                           */
/* This file is under the GNU Public License (GPL)                           */
/*===========================================================================*/

/*===========================================================================*/
/* (SDL) todo's                                                              */
/*                                                                           */
/* - generate WinFellow icon and place it in the root directory              */
/*===========================================================================*/

#define INITGUID

#include "portable.h"
#include <windowsx.h>
#include "gui_general.h"
#include "SDL.h"
#include "defs.h"
#include "fellow.h"
#include "fmem.h"
#include "sound.h"
#include "graph.h"
#include "draw.h"
#include "gfxdrv.h"
#include "kbd.h"
#include "kbddrvsdl.h"
#include "listtree.h"
#include "windrv.h"
#include "gameport.h"
#include "mousedrvsdl.h"
#include "joydrv.h"
#include "sounddrv.h"
#include "wgui.h"
#include "ini.h"

ini *gfxdrv_sdl_ini; /* GFXDRVSDL copy of initdata */

/*==========================================================================*/
/* Structs for holding information about a SDL device and mode              */
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
} gfxdrv_sdl_mode;

typedef struct {
  SDL_Surface *     draw_surface;
  SDL_Surface *     window_icon;
  BOOLE		          PaletteInitialized;
  felist *          modes;
  gfxdrv_sdl_mode * mode;
  ULO		            buffercount;
  ULO		            maxbuffercount;
  RECT		          clientrect_screen;
  RECT		          hwnd_clientrect_win;
  draw_mode *       drawmode;
  BOOLE		          use_blitter;
  ULO		            vertical_scale;
  ULO               vertical_scale_strategy;
  BOOLE		          can_stretch_y;
  BOOLE		          stretch_warning_shutup;
  BOOLE		          no_sdl_hardware;
} gfxdrv_sdl_device;

gfxdrv_sdl_device * gfxdrv_sdl_device_current;
felist *            gfxdrv_sdl_known_mode_list;

/*==========================================================================*/
/* Window hosting the Amiga display and some generic status information     */
/*==========================================================================*/

BOOLE gfxdrv_sdl_initialized;
volatile BOOLE gfxdrv_sdl_win_active;
volatile BOOLE gfxdrv_sdl_win_active_original;
volatile BOOLE gfxdrv_sdl_win_minimized_original;
volatile BOOLE gfxdrv_sdl_syskey_down;
HWND  gfxdrv_sdl_hwnd;
BOOLE gfxdrv_sdl_displaychange;
BOOLE gfxdrv_sdl_stretch_always;

HANDLE gfxdrv_sdl_app_run;        /* Event indicating running or paused status */

/*==========================================================================*/
/* Master copy of the 8 bit palette                                         */
/*==========================================================================*/

SDL_Color gfxdrv_sdl_palette[256];

/*==========================================================================*/
/* Initialize the run status event                                          */
/*==========================================================================*/

BOOLE gfxDrvSDL_RunEventInitialize(void) 
{
	gfxdrv_sdl_app_run = CreateEvent(NULL, TRUE, FALSE, NULL);
	return (gfxdrv_sdl_app_run != NULL);
}

void gfxDrvSDL_RunEventRelease(void) 
{
	if (gfxdrv_sdl_app_run) 
	{
		CloseHandle(gfxdrv_sdl_app_run);
	}
}

void gfxDrvSDL_RunEventSet(void) 
{
	SetEvent(gfxdrv_sdl_app_run);
}

void gfxDrvSDL_RunEventReset(void) 
{
	ResetEvent(gfxdrv_sdl_app_run);
}

void gfxDrvSDL_RunEventWait(void) 
{
	WaitForSingleObject(gfxdrv_sdl_app_run, INFINITE);
}

void gfxDrvSDL_ChangeInputDeviceStates(SDL_Event* event) 
{
  mouseDrvSDLStateHasChanged(event->active.gain);
  //joyDrvStateHasChanged(active);
  kbdDrvSDLStateHasChanged(event->active.gain);
}

void gfxDrvSDL_EvaluateActiveStatus(void) {
  BOOLE old_active = gfxdrv_sdl_win_active;

  gfxdrv_sdl_win_active = (gfxdrv_sdl_win_active_original &&
                       !gfxdrv_sdl_win_minimized_original &&
		        !gfxdrv_sdl_syskey_down);
  if (gfxdrv_sdl_win_active) {
    //gfxDrvSDL_RunEventSet();
  }
  else {
    //gfxDrvSDL_RunEventReset();
  }
}

/*==========================================================================*/
/* Returns textual error message.                                           */
/*==========================================================================*/

STR *gfxDrvSDL_ErrorString(void) 
{
  return SDL_GetError();
}


/*==========================================================================*/
/* Logs a sensible error message                                            */
/*==========================================================================*/

void gfxDrvSDL_Failure(STR *header) {
  fellowAddLog("gfxdrvSDL: ");
  fellowAddLog(header);
  fellowAddLog(gfxDrvSDL_ErrorString());
  fellowAddLog("\n");
}

/*==========================================================================*/
/* Show window hosting the amiga display                                    */
/* Called on every emulation startup                                        */
/*==========================================================================*/

void gfxDrvSDL_WindowShow(gfxdrv_sdl_device *sdl_device) {
  fellowAddLog("gfxdrvSDL: gfxDrvSDL_WindowShow()\n");
  if (!sdl_device->mode->windowed) {
    //ShowWindow(gfx_drv_hwnd, SW_SHOWMAXIMIZED);
    //UpdateWindow(gfx_drv_hwnd);
  }
  else {
	  /*
    RECT rc1;
    
    SetRect(&rc1, iniGetEmulationWindowXPos(gfxdrv_ini), iniGetEmulationWindowYPos(gfxdrv_ini), 
		ddraw_device->drawmode->width + iniGetEmulationWindowXPos(gfxdrv_ini), 
		ddraw_device->drawmode->height + iniGetEmulationWindowYPos(gfxdrv_ini));
//    SetRect(&rc1, 0, 0, ddraw_device->drawmode->width, ddraw_device->drawmode->height);
    AdjustWindowRectEx(&rc1,
      GetWindowStyle(gfx_drv_hwnd),
      GetMenu(gfx_drv_hwnd) != NULL,
      GetWindowExStyle(gfx_drv_hwnd));

	MoveWindow(gfx_drv_hwnd,
      iniGetEmulationWindowXPos(gfxdrv_ini),
      iniGetEmulationWindowYPos(gfxdrv_ini), 
      rc1.right - rc1.left,
      rc1.bottom - rc1.top,
      FALSE);
    ShowWindow(gfx_drv_hwnd, SW_SHOWNORMAL);
    UpdateWindow(gfx_drv_hwnd);
    gfxDrvWindowFindClientRect(ddraw_device);
	*/
  }
}


/*==========================================================================*/
/* Hide window hosting the amiga display                                    */
/* Called on emulation stop                                                 */
/*==========================================================================*/

void gfxDrvSDL_WindowHide(gfxdrv_sdl_device *sdl_device) 
{
  if (!sdl_device->mode->windowed)
  {
    SDL_WM_IconifyWindow();
  }
}


/*==========================================================================*/
/* looks for the given graphical mode within the existing list              */
/*==========================================================================*/

gfxdrv_sdl_mode *gfxDrvSDL_modeFind(gfxdrv_sdl_device *sdl_device, ULO width, ULO height, ULO depth) 
{
	felist *l;
  
	for (l = sdl_device->modes; l != NULL; l = listNext(l)) 
	{
		gfxdrv_sdl_mode * tmpmode = (gfxdrv_sdl_mode *) listNode(l);
		if ((tmpmode->width == width) && (tmpmode->height == height) && (tmpmode->depth == depth))
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

void gfxDrvSDL_ModeInformationRegister(gfxdrv_sdl_device *sdl_device) {
	felist * l;
	ULO id;
  
	id = 0;
	for (l = sdl_device->modes; l != NULL; l = listNext(l)) 
	{
		gfxdrv_sdl_mode * sdl_mode = listNode(l);
		draw_mode * mode = (draw_mode *) malloc(sizeof(draw_mode));
    
		mode->width = sdl_mode->width;
		mode->height = sdl_mode->height;
		mode->bits = sdl_mode->depth;
		mode->windowed = sdl_mode->windowed;
		mode->refresh = sdl_mode->refresh;
		mode->redpos = sdl_mode->redpos;
		mode->redsize = sdl_mode->redsize;
		mode->greenpos = sdl_mode->greenpos;
		mode->greensize = sdl_mode->greensize;
		mode->bluepos = sdl_mode->bluepos;
		mode->bluesize = sdl_mode->bluesize;
		mode->pitch = 1;
		mode->id = id;

		if (!sdl_mode->windowed) {
			char hz[16];

			if (sdl_mode->refresh != 0) 
			{
				sprintf(hz, "%dHZ", sdl_mode->refresh);
			}
			else 
			{	
				hz[0] = 0;
			}
			sprintf(mode->name, "%dWx%dHx%dBPPx%s", sdl_mode->width, sdl_mode->height, sdl_mode->depth, hz);
		}
		else 
		{
			sprintf(mode->name, "%dWx%dHxWindow", sdl_mode->width, sdl_mode->height);
		}
		drawModeAdd(mode);
		id++;
	}
}


/*==========================================================================*/
/* Examines RGB masks for the mode information initializer                  */
/* Called on emulator startup                                               */
/*==========================================================================*/

ULO gfxDrvSDL_RGBMaskPos(ULO mask) {
  ULO i;
  
  for (i = 0; i < 32; i++)
    if (mask & (1<<i))
      return i;
    return 0;
}

ULO gfxDrvSDL_RGBMaskSize(ULO mask) {
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

void gfxDrvSDL_ModeInformationDump(gfxdrv_sdl_device *sdl_device) 
{
	felist * l;
	STR s[120];
  
	sprintf(s, "gfxdrvSDL: SDL modes found: %d\n", listCount(sdl_device->modes));
	fellowAddLog(s);
	for (l = sdl_device->modes; l != NULL; l = listNext(l)) 
	{
		gfxdrv_sdl_mode * tmpmode = (gfxdrv_sdl_mode *) listNode(l);
		if (!tmpmode->windowed)
		{
			sprintf(s, "gfxdrvsdl: Mode Description: %dWx%dHx%dBPPx%dHZ (%d,%d,%d,%d,%d,%d)\n", tmpmode->width, tmpmode->height, tmpmode->depth, tmpmode->refresh, tmpmode->redpos, tmpmode->redsize, tmpmode->greenpos, tmpmode->greensize, tmpmode->bluepos, tmpmode->bluesize);
		}
		else
		{
			sprintf(s, "gfxdrvsdl: Mode Description: %dWx%dHxWindow\n", tmpmode->width, tmpmode->height);
		}
		fellowAddLog(s);
	}
}

/*==========================================================================*/
/* Makes a new mode information struct and fills in the provided params     */
/* Called on emulator startup                                               */
/*==========================================================================*/

gfxdrv_sdl_mode * gfxDrvSDL_ModeNew(ULO width, ULO height, ULO depth, ULO refresh, ULO redpos, ULO redsize, ULO greenpos, ULO greensize, ULO bluepos, ULO bluesize, BOOLE windowed) 
{
  gfxdrv_sdl_mode * tmpmode = (gfxdrv_sdl_mode *) malloc(sizeof(gfxdrv_sdl_mode));
  
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

void gfxDrvSDL_BuildKnownModesList(void)
{
	ULO colordepth = 8;
	ULO width[4] = {320, 376, 640, 752};
	ULO i;

	gfxdrv_sdl_known_mode_list = NULL;

	// fullscreen modes
	for (i = 0; i < 4; i++)
	{
		gfxdrv_sdl_known_mode_list = listAddLast(gfxdrv_sdl_known_mode_list, listNew(gfxDrvSDL_ModeNew(640, 480, colordepth, 0, 0, 0, 0, 0, 0, 0, FALSE)));
		gfxdrv_sdl_known_mode_list = listAddLast(gfxdrv_sdl_known_mode_list, listNew(gfxDrvSDL_ModeNew(800, 600, colordepth, 0, 0, 0, 0, 0, 0, 0, FALSE)));
		gfxdrv_sdl_known_mode_list = listAddLast(gfxdrv_sdl_known_mode_list, listNew(gfxDrvSDL_ModeNew(1024, 768, colordepth, 0, 0, 0, 0, 0, 0, 0, FALSE)));
    gfxdrv_sdl_known_mode_list = listAddLast(gfxdrv_sdl_known_mode_list, listNew(gfxDrvSDL_ModeNew(1152, 864, colordepth, 0, 0, 0, 0, 0, 0, 0, FALSE)));
		gfxdrv_sdl_known_mode_list = listAddLast(gfxdrv_sdl_known_mode_list, listNew(gfxDrvSDL_ModeNew(1280, 1024, colordepth, 0, 0, 0, 0, 0, 0, 0, FALSE)));
		gfxdrv_sdl_known_mode_list = listAddLast(gfxdrv_sdl_known_mode_list, listNew(gfxDrvSDL_ModeNew(1600, 1200, colordepth, 0, 0, 0, 0, 0, 0, 0, FALSE)));
    gfxdrv_sdl_known_mode_list = listAddLast(gfxdrv_sdl_known_mode_list, listNew(gfxDrvSDL_ModeNew(1856, 1392, colordepth, 0, 0, 0, 0, 0, 0, 0, FALSE)));
    gfxdrv_sdl_known_mode_list = listAddLast(gfxdrv_sdl_known_mode_list, listNew(gfxDrvSDL_ModeNew(1900, 1440, colordepth, 0, 0, 0, 0, 0, 0, 0, FALSE)));
    gfxdrv_sdl_known_mode_list = listAddLast(gfxdrv_sdl_known_mode_list, listNew(gfxDrvSDL_ModeNew(2048, 1536, colordepth, 0, 0, 0, 0, 0, 0, 0, FALSE)));

		colordepth += 8;
	}

	// windowed modes (for current set color depth for the desktop)
	for (i = 0; i < 4; i++)
	{
		gfxdrv_sdl_known_mode_list = listAddLast(gfxdrv_sdl_known_mode_list, listNew(gfxDrvSDL_ModeNew(width[i], 200, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
		gfxdrv_sdl_known_mode_list = listAddLast(gfxdrv_sdl_known_mode_list, listNew(gfxDrvSDL_ModeNew(width[i], 256, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
		gfxdrv_sdl_known_mode_list = listAddLast(gfxdrv_sdl_known_mode_list, listNew(gfxDrvSDL_ModeNew(width[i], 288, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
		gfxdrv_sdl_known_mode_list = listAddLast(gfxdrv_sdl_known_mode_list, listNew(gfxDrvSDL_ModeNew(width[i], 400, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
		gfxdrv_sdl_known_mode_list = listAddLast(gfxdrv_sdl_known_mode_list, listNew(gfxDrvSDL_ModeNew(width[i], 512, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
		gfxdrv_sdl_known_mode_list = listAddLast(gfxdrv_sdl_known_mode_list, listNew(gfxDrvSDL_ModeNew(width[i], 576, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
	}		
  gfxdrv_sdl_known_mode_list = listAddLast(gfxdrv_sdl_known_mode_list, listNew(gfxDrvSDL_ModeNew(800, 600, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
  gfxdrv_sdl_known_mode_list = listAddLast(gfxdrv_sdl_known_mode_list, listNew(gfxDrvSDL_ModeNew(1024, 768, 0, 0, 0, 0, 0, 0, 0, 0, TRUE)));
}

void gfxDrvSDL_KnownModesListRelease(void)
{
  listFreeAll(gfxdrv_sdl_known_mode_list, TRUE);
  gfxdrv_sdl_known_mode_list = NULL;
}

/*==========================================================================*/
/* Creates a list of available modes on a given DirectDraw device           */
/* Called on emulator startup                                               */
/*==========================================================================*/

void gfxDrvSDL_ModeInformationInitialize(gfxdrv_sdl_device * sdl_device) 
{
	BOOLE result;
	SDL_Rect ** modes;
	gfxdrv_sdl_mode * mode;
	ULO suggested_depth;
	ULO i, j;
	felist * l;
	ULO colordepth;
	const SDL_VideoInfo * video_info;
  
	sdl_device->modes = NULL;
    
  // retrieve color depth of desktop
  video_info = SDL_GetVideoInfo();

	// check fullscreen modes
	modes = SDL_ListModes(NULL, SDL_FULLSCREEN);

	if (modes == (SDL_Rect **) 0)
	{
		// no fullscreen modes available
	} 
	else if (modes == (SDL_Rect **) -1)
	{
		// all fullscreen modes available (lets check the popular sizes)
		for (l = gfxdrv_sdl_known_mode_list; l != NULL; l = listNext(l)) 
		{
			mode = (gfxdrv_sdl_mode *) listNode(l);
			// make sure the proposed mode is a fullscreen mode
			if (mode->windowed == FALSE)
			{
				suggested_depth = SDL_VideoModeOK(mode->width, mode->height, mode->depth, SDL_FULLSCREEN);
				if (suggested_depth == mode->depth)
				{
					// mode exists at requested color depth
					sdl_device->modes = listAddLast(sdl_device->modes, listNew(gfxDrvSDL_ModeNew(mode->width, mode->height, mode->depth, 0, 0, 0, 0, 0, 0, 0, FALSE)));
				}
			}
		}
	} 
	else
	{
		// some fullscreen modes available 
		for(i = 0; modes[i] != NULL; i++)
		{
			colordepth = 8;
			for (j = 0; j < 4; j++)
			{
				suggested_depth = SDL_VideoModeOK(modes[i]->w, modes[i]->h, colordepth, SDL_FULLSCREEN);
				if (suggested_depth == colordepth)
				{
					// mode exists at requested color depth
					sdl_device->modes = listAddLast(sdl_device->modes, listNew(gfxDrvSDL_ModeNew(modes[i]->w, modes[i]->h, colordepth, 0, 0, 0, 0, 0, 0, 0, FALSE)));
				}
				colordepth += 8;
			}
		}
	}

	// check windowed modes
	modes = SDL_ListModes(NULL, 0);

	if (modes == (SDL_Rect **) 0)
	{
		// no windowed modes available
	} 
	else if (modes == (SDL_Rect **) -1)
	{
		// all windowed modes available (lets check the known sizes)
		for (l = gfxdrv_sdl_known_mode_list; l != NULL; l = listNext(l)) 
		{
			mode = (gfxdrv_sdl_mode *) listNode(l);
			// make sure the proposed mode is a windowed mode
			if (mode->windowed == TRUE)
			{
				// we are bound to desktop color depth
				suggested_depth = SDL_VideoModeOK(mode->width, mode->height, video_info->vfmt->BitsPerPixel, 0);
				if (suggested_depth == video_info->vfmt->BitsPerPixel)
				{
					// mode exists at suggested color depth
					listAddLast(sdl_device->modes, listNew(gfxDrvSDL_ModeNew(mode->width, mode->height, video_info->vfmt->BitsPerPixel, 0, 0, 0, 0, 0, 0, 0, TRUE)));
				}
			}
		}
	} 
	else
	{
		// some windowed modes available 
		for(i = 0; modes[i] != NULL; i++)
		{
			// we are bound to desktop color depth 
			suggested_depth = SDL_VideoModeOK(modes[i]->w, modes[i]->h, video_info->vfmt->BitsPerPixel, 0);
			if (suggested_depth == video_info->vfmt->BitsPerPixel)
			{
				// mode exists at requested color depth
				listAddLast(sdl_device->modes, listNew(gfxDrvSDL_ModeNew(modes[i]->w, modes[i]->h, video_info->vfmt->BitsPerPixel, 0, 0, 0, 0, 0, 0, 0, TRUE)));
			}
		}
	}

	gfxDrvSDL_ModeInformationRegister(sdl_device);
	gfxDrvSDL_ModeInformationDump(sdl_device);
}


/*==========================================================================*/
/* Releases the list of available modes                                     */
/* Called on emulator shutdown                                              */
/*==========================================================================*/

void gfxDrvSDL_ModeInformationRelease() {
  listFreeAll(gfxdrv_sdl_device_current->modes, TRUE);
  listFreeAll(gfxdrv_sdl_known_mode_list, TRUE);
  gfxdrv_sdl_device_current->modes = NULL;
  gfxdrv_sdl_known_mode_list = NULL;
  free(gfxdrv_sdl_device_current);
}


/*==========================================================================*/
/* clear surface (to entirely black color)                                  */
/*==========================================================================*/

void gfxDrvSDL_SurfaceClear(SDL_Surface * surface) 
{
	if (SDL_FillRect(surface, NULL, 0) == -1)
	{
		gfxDrvSDL_Failure("gfxDrvSDL_SurfaceClear(): ");
	}
}

    
/*==========================================================================*/
/* blit secondary buffer to the primary surface                             */
/*==========================================================================*/

void gfxDrvSDL_SurfaceBlit(gfxdrv_sdl_device *sdl_device) 
{
	SDL_Rect srcrect;
	SDL_Rect dstrect;
	int result;

	// when using the blitter to show our buffer
	// source surface is always the secondary off-screen buffer 

	// srcrect must be corrected for exact vertical scaling 
	if (sdl_device->mode->windowed == TRUE)
	{
		// windowed
		srcrect.x = 0;
		srcrect.y = 0;
		srcrect.w = sdl_device->mode->width;
		srcrect.h = sdl_device->mode->height >> (sdl_device->vertical_scale - 1);

		dstrect.x = 0;
		dstrect.y = 0;
		dstrect.w = sdl_device->mode->width;
		dstrect.h = sdl_device->mode->height;
	}
	else
	{
		// fullscreen
		srcrect.x = draw_hoffset;
		srcrect.w = draw_width_amiga_real;
		srcrect.y = draw_voffset;
		srcrect.h = draw_height_amiga_real >> (sdl_device->vertical_scale - 1);

		dstrect.x = draw_hoffset;
		dstrect.y = draw_voffset;
		dstrect.w = draw_width_amiga_real;
		dstrect.h = draw_height_amiga_real;
	}

	// this can fail when a surface is lost 
//	if (result = (SDL_BlitSurface(sdl_device->sec_surface, &srcrect, sdl_device->pri_surface, &dstrect)) != 0)
	result = -3;
	{
		// blit failed 
		gfxDrvSDL_Failure("gfxDrvSDL_SurfaceBlit(): ");

		switch (result)
		{
		case -1:
			// what should we do?
			break;
		case -2:
			// one or more surfaces were within video memory (HW surface) and got lost
			// we should lock it and unlock it to restore the surfaces(?)
			break;
		default:
			gfxDrvSDL_Failure("gfxDrvSDL_SurfaceBlit(): (unknown result of SDL_BlitSurface) ");
			break;
		}
	}
}


/*==========================================================================*/
/* Release the surfaces                                                     */
/*==========================================================================*/

void gfxDrvSDL_SurfacesRelease(gfxdrv_sdl_device *sdl_device) 
{
  
	if (sdl_device->draw_surface != NULL) 
	{
		SDL_FreeSurface(sdl_device->draw_surface);
		sdl_device->draw_surface = NULL;
	}
//	if (sdl_device->sec_surface != NULL) 
	{
//		SDL_FreeSurface(sdl_device->sec_surface);
//		sdl_device->sec_surface = NULL;
	}
	sdl_device->buffercount = 0;
}

void gfxDrvSDL_CreateSecondarySurface(gfxdrv_sdl_device *sdl_device) 
{
//    sdl_device->sec_surface = SDL_CreateRGBSurface(SDL_HWSURFACE, sdl_device->pri_surface->w, sdl_device->pri_surface->h, sdl_device->pri_surface->format->BitsPerPixel, sdl_device->pri_surface->format->Rmask, sdl_device->pri_surface->format->Gmask, sdl_device->pri_surface->format->Bmask, sdl_device->pri_surface->format->Amask);
//	buffer_allocated = TRUE;
//	if ((sdl_device->sec_surface->flags & SDL_HWSURFACE) == SDL_HWSURFACE)
	{
		// surface allocated in HW (video memory)
//		fellowAddLog("gfxdrv: allocated second surface in %s (%dx%dx%d)\n", "video memory", sdl_device->sec_surface->w, sdl_device->sec_surface->h, sdl_device->sec_surface->format->BitsPerPixel);
    }
//	else
	{
		// surface allocated in SW (system memory)
//		fellowAddLog("gfxdrv: allocated second surface in %s (%dx%dx%d)\n", "system memory", sdl_device->sec_surface->w, sdl_device->sec_surface->h, sdl_device->sec_surface->format->BitsPerPixel);
	}
//	gfxDrvSDL_SurfaceClear(sdl_device->sec_surface);
}

/*==========================================================================*/
/* create the surfaces                                                      */
/* called before emulation start                                            */
/*==========================================================================*/

ULO gfxDrvSDL_SurfacesInitialize(gfxdrv_sdl_device *sdl_device) 
{
	return 0;
}

/*==========================================================================*/
/* Lock the current drawing surface                                         */
/*==========================================================================*/

UBY *gfxDrvSDL_SurfaceLock(gfxdrv_sdl_device *sdl_device) 
{
	if ( SDL_MUSTLOCK(sdl_device->draw_surface) ) 
	{
        if ( SDL_LockSurface(sdl_device->draw_surface) < 0 )
		{
            return NULL;
		}
    }
	return sdl_device->draw_surface->pixels;
}


/*==========================================================================*/
/* Unlock the primary surface                                               */
/*==========================================================================*/

void gfxDrvSDL_SurfaceUnlock(gfxdrv_sdl_device *sdl_device) 
{
  	if ( SDL_MUSTLOCK(sdl_device->draw_surface) ) 
	{
       SDL_UnlockSurface(sdl_device->draw_surface);
	}
}


/*==========================================================================*/
/* Flip to next buffer                                                      */
/*==========================================================================*/

void gfxDrvSDL_Flip(gfxdrv_sdl_device *sdl_device) 
{
	/*
  HRESULT err;
  
  if (ddraw_device->use_blitter)     // Blit secondary buffer to primary 
    gfxDrvDDrawSurfaceBlit(ddraw_device);
  if (ddraw_device->buffercount > 1)    // Flip buffer if there are several 
    if ((err = IDirectDrawSurface_Flip(ddraw_device->lpDDSPrimary,
      NULL,
      DDFLIP_WAIT)) != DD_OK)
      gfxDrvDDrawFailure("gfxDrvDDrawFlip(): ", err);
	*/
	SDL_Flip(sdl_device->draw_surface);
	//SDL_UpdateRect(sdl_device->draw_surface,0,0,0,0);
}


/*==========================================================================*/
/* Set specified mode                                                       */
/* In windowed mode, we don't set a mode, but instead queries the pixel-    */
/* format from the surface (gfxDrvDDrawSurfacesInitialize())                */
/* Called before emulation start                                            */
/*==========================================================================*/

ULO gfxDrvSDL_SetModeInternal(gfxdrv_sdl_device *sdl_device) 
{
	/*
	BOOLE result = TRUE;
	ULO buffers = 0;

	if (gfxDrvDDrawSetCooperativeLevel(ddraw_device)) 
	{
		ddraw_device->use_blitter = 
			((ddraw_device->mode->windowed) || ((ddraw_device->vertical_scale > 1) && (ddraw_device->vertical_scale_strategy == 1))|| (ddraw_device->no_dd_hardware) || (gfx_drv_stretch_always));
		if (!ddraw_device->mode->windowed) 
		{
			gfxdrv_sdl_mode *mode;
			HRESULT err;
			DDSURFACEDESC myDDSDesc;

			mode = (gfxdrv_sdl_mode *) listNode(listIndex(ddraw_device->modes, ddraw_device->drawmode->id));
			memset(&myDDSDesc, 0, sizeof(myDDSDesc));
			myDDSDesc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
			if ((err = ddraw_device->lpDD2->lpVtbl->SetDisplayMode(ddraw_device->lpDD2, mode->width, mode->height, mode->depth,	mode->refresh, 0)) != DD_OK) 
			{
				gfxDrvDDrawFailure("gfxDrvDDrawSetMode(): ", err);
				result = FALSE;
			}
		}
		if (result) {
			gfxDrvDDrawPaletteInitialize(gfxdrv_sdl_device_current);
			if ((buffers = gfxDrvDDrawSurfacesInitialize(gfxdrv_sdl_device_current)) == 0)
			{
				gfxDrvDDrawSetCooperativeLevelNormal(gfxdrv_sdl_device_current);
			}
		}
	}
	return buffers;
	*/
	return 0;
}


/*==========================================================================*/
/* Functions below are the actual "graphics driver API"                     */
/*==========================================================================*/


/*==========================================================================*/
/* Set an 8 bit palette entry                                               */
/*==========================================================================*/

void gfxDrvSDL_Set8BitColor(ULO col, ULO r, ULO g, ULO b) 
{
	gfxdrv_sdl_palette[col].r      = (UBY) r;
	gfxdrv_sdl_palette[col].g      = (UBY) g;
	gfxdrv_sdl_palette[col].b      = (UBY) b;
	gfxdrv_sdl_palette[col].unused = 0;
}


/*==========================================================================*/
/* Locks surface and puts a valid framebuffer pointer in framebuffer        */
/*==========================================================================*/

UBY *gfxDrvSDL_ValidateBufferPointer(void) 
{
	UBY * buffer;
	ULO bufferULO;
	
	//gfxDrvSDL_RunEventWait();
	if (gfxdrv_sdl_displaychange) 
	{
		fellowAddLog("gfxdrvSDL: displaymode change\n");
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

	buffer = gfxDrvSDL_SurfaceLock(gfxdrv_sdl_device_current);
	if (buffer != NULL) 
	{
		// align pointer returned to drawing routines
		bufferULO = (ULO) buffer;  
		if (gfxdrv_sdl_device_current->drawmode->bits == 32) 
		{
			if ((bufferULO & 0x7) != 0)
			{
				buffer = (UBY *) (bufferULO & 0xfffffff8) + 8;
			}
		}
		if ((gfxdrv_sdl_device_current->drawmode->bits == 15) || (gfxdrv_sdl_device_current->drawmode->bits == 16)) 
		{
			if ((bufferULO & 0x3) != 0)
			{
				buffer = (UBY *) (bufferULO & 0xfffffffc) + 4;
			}
		}
	}
	return buffer;
}


/*==========================================================================*/
/* Unlocks surface                                                          */
/*==========================================================================*/

void gfxDrvSDL_InvalidateBufferPointer(void) {
  gfxDrvSDL_SurfaceUnlock(gfxdrv_sdl_device_current);
}


/*==========================================================================*/
/* Flip the current buffer                                                  */
/*==========================================================================*/

void gfxDrvSDL_BufferFlip(void) {
  gfxDrvSDL_Flip(gfxdrv_sdl_device_current);
}


/*==========================================================================*/
/* Set the specified mode                                                   */
/* Returns the number of available buffers, zero is error                   */
/*==========================================================================*/

void gfxDrvSDL_SetMode(draw_mode *dm, ULO vertical_scale, ULO vertical_scale_strategy) {
  gfxdrv_sdl_device_current->drawmode = dm;  
  gfxdrv_sdl_device_current->vertical_scale = vertical_scale;
  gfxdrv_sdl_device_current->vertical_scale_strategy = vertical_scale_strategy;
  gfxdrv_sdl_device_current->mode = (gfxdrv_sdl_mode *) listNode(listIndex(gfxdrv_sdl_device_current->modes, dm->id));
}


/*==========================================================================*/
/* Emulation is starting                                                    */
/* Called on emulation start                                                */
/*==========================================================================*/

BOOLE gfxDrvSDL_EmulationStart(ULO maxbuffercount) 
{
  ULO applied_depth;
	const SDL_VideoInfo * video_info;
  ULO suggested_depth;

	// we pause the application
	//gfxDrvSDL_RunEventReset();                    

	gfxdrv_sdl_device_current->maxbuffercount = maxbuffercount;
	gfxdrv_sdl_win_active = FALSE;
	gfxdrv_sdl_win_active_original = FALSE;
	gfxdrv_sdl_win_minimized_original = FALSE;
	gfxdrv_sdl_syskey_down = FALSE;
	gfxdrv_sdl_displaychange = FALSE;
	
  if (gfxdrv_sdl_initialized == FALSE)
  {
  	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0)
  	{
	    gfxDrvSDL_Failure("unable to (re)init SDL");
		  return FALSE;
    }
  }

  // windowed
	if (gfxdrv_sdl_device_current->mode->windowed)
	{
    // retrieve color depth of desktop
    video_info = SDL_GetVideoInfo();

		// when (use_blitter == 1) only a SW surface can be requested
		suggested_depth = SDL_VideoModeOK(
			gfxdrv_sdl_device_current->drawmode->width, 
			gfxdrv_sdl_device_current->drawmode->height, 
			video_info->vfmt->BitsPerPixel, 
			SDL_SWSURFACE 
		);

    if (suggested_depth != video_info->vfmt->BitsPerPixel)
    {
      gfxDrvSDL_Failure("different color depth applied than requested");
    }
		
    // set the icon for the hosting window (bmp not yet available)
    //SDL_WM_SetIcon(SDL_LoadBMP("winfellow_icon.bmp"), NULL);

		// set video mode and show window
		gfxdrv_sdl_device_current->draw_surface = SDL_SetVideoMode(
			gfxdrv_sdl_device_current->drawmode->width, 
			gfxdrv_sdl_device_current->drawmode->height, 
			video_info->vfmt->BitsPerPixel, 
			SDL_SWSURFACE
		);

	  if ( gfxdrv_sdl_device_current->draw_surface == NULL ) 
		{
			gfxDrvSDL_Failure("unable to set selected video mode");
			return FALSE;
		} 

    // set name of the hosting window
    SDL_WM_SetCaption(FELLOWVERSION, NULL);

		// retrieve pixel format (used for color translation in draw module)
		gfxdrv_sdl_device_current->drawmode->redpos    = gfxDrvSDL_RGBMaskPos(gfxdrv_sdl_device_current->draw_surface->format->Rmask);
		gfxdrv_sdl_device_current->drawmode->redsize   = gfxDrvSDL_RGBMaskSize(gfxdrv_sdl_device_current->draw_surface->format->Rmask);
		gfxdrv_sdl_device_current->drawmode->greenpos  = gfxDrvSDL_RGBMaskPos(gfxdrv_sdl_device_current->draw_surface->format->Gmask);
		gfxdrv_sdl_device_current->drawmode->greensize = gfxDrvSDL_RGBMaskSize(gfxdrv_sdl_device_current->draw_surface->format->Gmask);
		gfxdrv_sdl_device_current->drawmode->bluepos   = gfxDrvSDL_RGBMaskPos(gfxdrv_sdl_device_current->draw_surface->format->Bmask);
		gfxdrv_sdl_device_current->drawmode->bluesize  = gfxDrvSDL_RGBMaskSize(gfxdrv_sdl_device_current->draw_surface->format->Bmask);
    }
	else
	{
		// fullscreen
    applied_depth = SDL_VideoModeOK(
			gfxdrv_sdl_device_current->drawmode->width, 
			gfxdrv_sdl_device_current->drawmode->height, 
			gfxdrv_sdl_device_current->drawmode->bits, 
 			//SDL_HWSURFACE | SDL_FULLSCREEN | SDL_DOUBLEBUF
			SDL_HWSURFACE | SDL_FULLSCREEN
		);
		
		// suggested depth should not be zero
		if (applied_depth == 0)
		{
			gfxDrvSDL_Failure("returned color depth was zero, video mode not available");
      return FALSE;
		}	

    // set the icon for the hosting window (bmp not yet available)
    //SDL_WM_SetIcon(SDL_LoadBMP("winfellow_icon.bmp"), NULL);

		// set video mode and show window
		gfxdrv_sdl_device_current->draw_surface = SDL_SetVideoMode(
			gfxdrv_sdl_device_current->drawmode->width, 
			gfxdrv_sdl_device_current->drawmode->height, 
			gfxdrv_sdl_device_current->drawmode->bits, 
			//SDL_HWSURFACE | SDL_FULLSCREEN | SDL_DOUBLEBUF
      SDL_HWSURFACE | SDL_FULLSCREEN
		);

	  if ( gfxdrv_sdl_device_current->draw_surface == NULL ) 
		{
			gfxDrvSDL_Failure("unable to set selected video mode");
			return FALSE;
		} 

    // set name of the hosting window
    SDL_WM_SetCaption(FELLOWVERSION, NULL);

		// retrieve pixel format (used for color translation in draw module)
		gfxdrv_sdl_device_current->drawmode->redpos    = gfxDrvSDL_RGBMaskPos(gfxdrv_sdl_device_current->draw_surface->format->Rmask);
		gfxdrv_sdl_device_current->drawmode->redsize   = gfxDrvSDL_RGBMaskSize(gfxdrv_sdl_device_current->draw_surface->format->Rmask);
		gfxdrv_sdl_device_current->drawmode->greenpos  = gfxDrvSDL_RGBMaskPos(gfxdrv_sdl_device_current->draw_surface->format->Gmask);
		gfxdrv_sdl_device_current->drawmode->greensize = gfxDrvSDL_RGBMaskSize(gfxdrv_sdl_device_current->draw_surface->format->Gmask);
		gfxdrv_sdl_device_current->drawmode->bluepos   = gfxDrvSDL_RGBMaskPos(gfxdrv_sdl_device_current->draw_surface->format->Bmask);
		gfxdrv_sdl_device_current->drawmode->bluesize  = gfxDrvSDL_RGBMaskSize(gfxdrv_sdl_device_current->draw_surface->format->Bmask);
	}

	// recover pitch (maybe not needed here)
	gfxdrv_sdl_device_current->drawmode->pitch = gfxdrv_sdl_device_current->draw_surface->pitch;

	return TRUE;
}


/*==========================================================================*/
/* Emulation is stopping, clean up current videomode                        */
/*==========================================================================*/

void gfxDrvSDL_EmulationStop(void) 
{
  // we quit SDL to free the surface and hosting window 
  SDL_Quit();
  gfxdrv_sdl_initialized = FALSE;
}


/*==========================================================================*/
/* End of frame handler                                                     */
/*==========================================================================*/

void gfxDrvSDL_EndOfFrame(void) {
}


/*==========================================================================*/
/* Collects information about the DirectX capabilities of this computer     */
/* After this, a DDraw object exists.                                       */
/* Called on emulator startup                                               */
/*==========================================================================*/

BOOLE gfxDrvSDL_Startup(void) 
{
  char videodriver_name[32];

  gfxdrv_sdl_ini = iniManagerGetCurrentInitdata(&ini_manager);
	gfxdrv_sdl_initialized = FALSE;
	gfxdrv_sdl_app_run = NULL;
	graph_buffer_lost = FALSE;

	// initilize event, making it possible to pause emulation
	if (gfxDrvSDL_RunEventInitialize() == FALSE)
	{
		fellowAddLog("gfxdrvSDL: event could not be intialized\n");
		return FALSE;
	}

 	// initialize SDL
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0)
	{
	  gfxDrvSDL_Failure("unable to init SDL");
		return FALSE;
	}
	gfxdrv_sdl_initialized = TRUE;

  // log some info
  SDL_VideoDriverName(videodriver_name, sizeof(videodriver_name));
  fellowAddLog("gfxdrvSDL: using %s as 2D graphics layer\n", videodriver_name);
  
	// allocate memory for our current SDL device
	gfxdrv_sdl_device_current = (gfxdrv_sdl_device *) malloc(sizeof(gfxdrv_sdl_device));
	memset(gfxdrv_sdl_device_current, 0, sizeof(gfxdrv_sdl_device));

	// build a list of known modes (needed in case of all modes available)
	gfxDrvSDL_BuildKnownModesList();

	// build a list of possible modes 
	gfxDrvSDL_ModeInformationInitialize(gfxdrv_sdl_device_current);

	return TRUE;
}


/*==========================================================================*/
/* Free DDraw resources                                                     */
/* Called on emulator shutdown                                              */
/*==========================================================================*/

void gfxDrvSDL_Shutdown(void) 
{
	gfxDrvSDL_ModeInformationRelease(gfxdrv_sdl_device_current);

	if (gfxdrv_sdl_initialized) {
		SDL_Quit();
	}
}

void gfxDrvSDL_Debugging(void)
{

}