/*===========================================================================*/
/* Fellow Amiga Emulator                                                     */
/* SDL Mouse driver for Windows                                              */
/* Author: Worfje (worfje@gmx.net)                                           */
/*                                                                           */
/* This file is under the GNU Public License (GPL)                           */
/*===========================================================================*/

/*===========================================================================*/
/* (SDL) todo's                                                              */
/*                                                                           */
/* - currently nothing                                                       */
/*===========================================================================*/

#include "defs.h"
#include <windows.h>
#include "SDL.h"
#include "gameport.h"
#include "fellow.h"
#include "mousedrvsdl.h"
#include "windrv.h"
#include "gfxdrvsdl.h"
#include <initguid.h>

/*===========================================================================*/
/* Mouse specific data                                                       */
/*===========================================================================*/

BOOLE			    mouse_drv_sdl_focus;
BOOLE			    mouse_drv_sdl_active;
BOOLE			    mouse_drv_sdl_in_use;
BOOLE			    mouse_drv_sdl_init_ok;
static BOOLE	mouse_drv_sdl_left_button;
static BOOLE	mouse_drv_sdl_middle_button;
static BOOLE	mouse_drv_sdl_right_button;


/*==========================================================================*/
/* Logs a sensible error message                                            */
/*==========================================================================*/

void mouseDrvSDLInputFailure(STR *header) 
{
  fellowAddLog(header);
  fellowAddLog(SDL_GetError());
  fellowAddLog("\n");
}

/*===========================================================================*/
/* Acquire SDL mouse device                                                  */
/*===========================================================================*/

void mouseDrvSDLInputAcquire(void) 
{
  SDL_GrabMode result_grab;
  int          result_show;

  fellowAddLog("mouseDrvSDLInputAcquire()\n");

  if (mouse_drv_sdl_in_use) 
  {
    // hide the cursor
    result_show = SDL_ShowCursor(SDL_DISABLE);
    
    // grab mouse and keyboard input
    result_grab = SDL_WM_GrabInput(SDL_GRAB_ON);

    // check outcome
    if ((result_grab != SDL_GRAB_ON) || (result_show != SDL_DISABLE))
    {
        mouseDrvSDLInputFailure("mouseDrvSDLInputAcquire(): ");
    }
  }
  else 
  {
    // mouse driver is not active, probably no active window

    // show the cursor
    result_show = SDL_ShowCursor(SDL_ENABLE);
    
    // unacquire the mouse and keyboard input
    result_grab = SDL_WM_GrabInput(SDL_GRAB_OFF);

    // check outcome
    if ((result_grab != SDL_GRAB_OFF) || (result_show != SDL_ENABLE))
    {
        mouseDrvSDLInputFailure("mouseDrvSDLInputAcquire(): ");
    }
  }
}


/*===========================================================================*/
/* release SDL for mouse                                                     */
/*===========================================================================*/

void mouseDrvSDLInputRelease(void) 
{
  // nothing to release when using SDL
  // will be released when quiting SDL
}


/*===========================================================================*/
/* initialize SDL for mouse                                                  */
/*===========================================================================*/

BOOLE mouseDrvSDLInputInitialize(void) 
{
  fellowAddLog("mouseDrvSDLInputInitialize()\n");

  // mouse driver is initilized by SDL Video module
  mouse_drv_sdl_init_ok = gfxdrv_sdl_initialized;
  return mouse_drv_sdl_init_ok;
}


/*===========================================================================*/
/* mouse grab status has changed                                             */
/*===========================================================================*/

void mouseDrvSDLStateHasChanged(BOOLE window_active) 
{
  mouse_drv_sdl_active = window_active;

  if (mouse_drv_sdl_active && mouse_drv_sdl_focus) 
  {
    mouse_drv_sdl_in_use = TRUE;
  }
  else 
  {
    mouse_drv_sdl_in_use = FALSE;
  }
  mouseDrvSDLInputAcquire();
}


/*===========================================================================*/
/* mouse toggle focus                                                        */
/*===========================================================================*/

void mouseDrvSDLToggleFocus(void) 
{
  mouse_drv_sdl_focus = !mouse_drv_sdl_focus;
  mouseDrvSDLStateHasChanged(mouse_drv_sdl_active);
}


/*===========================================================================*/
/* Mouse movement handler                                                    */
/*===========================================================================*/

void mouseDrvSDLMovementHandler(SDL_Event * event) 
{
  LON x_rel      = 0;
  LON y_rel      = 0;
  LON mouse_num  = 0;

  if (mouse_drv_sdl_in_use) 
  {
    if (event->type == SDL_MOUSEMOTION)
    {
      x_rel = event->motion.xrel;
      y_rel = event->motion.yrel;
    }

    if (event->type == SDL_MOUSEBUTTONDOWN)
    {
      switch(event->button.button)
      {
      case SDL_BUTTON_LEFT:
        mouse_drv_sdl_left_button   = TRUE;
        break;
      case SDL_BUTTON_MIDDLE:
        mouse_drv_sdl_middle_button = TRUE;
        break;
      case SDL_BUTTON_RIGHT:
        mouse_drv_sdl_right_button  = TRUE;
        break;
      }
    }

    if (event->type == SDL_MOUSEBUTTONUP)
    {
      switch(event->button.button)
      {
      case SDL_BUTTON_LEFT:
        mouse_drv_sdl_left_button   = FALSE;
        break;
      case SDL_BUTTON_MIDDLE:
        mouse_drv_sdl_middle_button = FALSE;
        break;
      case SDL_BUTTON_RIGHT:
        mouse_drv_sdl_right_button  = FALSE;
        break;
      }
    }

    // currently only one mouse is supported in SDL(?) and therefor in Fellow
    gameportMouseHandler(GP_MOUSE0, x_rel, y_rel, mouse_drv_sdl_left_button, mouse_drv_sdl_middle_button, mouse_drv_sdl_right_button);
  }
}


/*===========================================================================*/
/* Hard Reset                                                                */
/*===========================================================================*/

void mouseDrvSDLHardReset(void) 
{
  fellowAddLog("mouseDrvSDLHardReset\n");
}


/*===========================================================================*/
/* Emulation Starting                                                        */
/*===========================================================================*/

BOOLE mouseDrvSDLEmulationStart(void) 
{
  fellowAddLog("mouseDrvSDLEmulationStart\n");

  // acquire the mouse (when having keyboard mouse and F12 is not yet pushed)
  mouseDrvSDLStateHasChanged((SDL_GetAppState() & SDL_APPINPUTFOCUS) == SDL_APPINPUTFOCUS);
  return mouseDrvSDLInputInitialize();
}


/*===========================================================================*/
/* Emulation Stopping                                                        */
/*===========================================================================*/

void mouseDrvSDLEmulationStop(void) {
  fellowAddLog("mouseDrvSDLEmulationStop\n");
  mouseDrvSDLInputRelease();
}


/*===========================================================================*/
/* Emulation Startup                                                         */
/*===========================================================================*/

void mouseDrvSDLStartup(void) 
{
  fellowAddLog("mouseDrvSDLStartup\n");

  mouse_drv_sdl_active        = FALSE;
  mouse_drv_sdl_focus         = TRUE;
  mouse_drv_sdl_in_use        = FALSE;
  mouse_drv_sdl_init_ok       = TRUE;
  mouse_drv_sdl_left_button   = FALSE;
  mouse_drv_sdl_middle_button = FALSE;
  mouse_drv_sdl_right_button  = FALSE;
}


/*===========================================================================*/
/* Emulation Shutdown                                                        */
/*===========================================================================*/

void mouseDrvSDLShutdown(void) 
{
  fellowAddLog("mouseDrvSDLShutdown\n");
}

