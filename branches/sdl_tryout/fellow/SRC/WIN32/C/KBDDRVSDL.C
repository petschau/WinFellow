/*===========================================================================*/
/* Fellow Amiga Emulator                                                     */
/* Keyboard driver for Windows                                               */
/* Author: Worfje (worfje@gmx.net)                                           */
/*                                                                           */
/* This file is under the GNU Public License (GPL)                           */
/*===========================================================================*/

/*===========================================================================*/
/* (SDL) todo's                                                              */
/*                                                                           */
/* - writting of mapping file to disk                                        */
/* - reading of joystick replacement mapping from disk                       */
/* - fix 'home/end' shortcut (next resolution)                               */
/* - fix 'home/end' shortcut (scrolling, have to mail Petter)                */
/* - fix 'home/end' shortcut (insert/swap floppy's)                          */
/* - fix 'home/end' shortcut (turbo speed vs. synced to PAL)                 */
/*===========================================================================*/

#include "defs.h"
#include "SDL.h"
#include "keycodes.h"
#include "kbd.h"
#include "kbddrvsdl.h"
#include "mousedrvsdl.h"
#include "gameport.h"
#include "windrv.h"
#include "kbdparser.h"
#include "fellow.h"
#include "gfxdrvsdl.h"
#include "sound.h"

#define SDL_MAPPING_FILENAME  "mapping.key"

/*===========================================================================*/
/* Keyboard specific data                                                    */
/*===========================================================================*/

BOOLE			 kbd_drv_sdl_active;
BOOLE      kbd_drv_sdl_in_use;
BOOLE			 kbd_drv_sdl_keys[SDLK_LAST];       // contains boolean values (pressed/not pressed) for actual keystroke
BOOLE			 kbd_drv_sdl_prevkeys[SDLK_LAST];   // contains boolean values (pressed/not pressed) for past keystroke
BOOLE      kbd_drv_sdl_init_ok;
BOOLE      kbd_drv_sdl_rewrite_mapping_file;


/*===========================================================================*/
/* Map SDL keycode to amiga keycode                                          */
/*===========================================================================*/

UBY sdl_to_amiga_keycode[SDLK_LAST];


/*===========================================================================*/
/* Keyboard handler state data                                               */
/*===========================================================================*/

// the order of kbd_drv_sdl_joykey_directions enum influence the order of the replacement_keys array in kbdparser.c

typedef enum {
  JOYKEY_LEFT      = 0,
  JOYKEY_RIGHT     = 1,
  JOYKEY_UP        = 2,
  JOYKEY_DOWN      = 3,
  JOYKEY_FIRE0     = 4,
  JOYKEY_FIRE1     = 5,
  JOYKEY_AUTOFIRE0 = 6,
  JOYKEY_AUTOFIRE1 = 7
} kbd_drv_sdl_joykey_directions;

SDLKey      kbd_drv_sdl_joykey[2][KBDDRVSDL_MAX_JOYKEY_VALUE];	// keys for joykeys 0 and 1 
BOOLE		    kbd_drv_sdl_joykey_enabled[2][2];	        // for each port, the enabled joykeys 
kbd_event	  kbd_drv_sdl_joykey_event[2][2][KBDDRVSDL_MAX_JOYKEY_VALUE];	// event ID for each joykey [port][pressed][direction] 

/*==========================================================================*/
/* Logs a sensible error message                                            */
/*==========================================================================*/

void kbdDrvSDLInputFailure(STR *header) {
  fellowAddLog(header);
  fellowAddLog(SDL_GetError());
  fellowAddLog("\n");
}


/*===========================================================================*/
/* Unacquire SDL keyboard device                                             */
/*===========================================================================*/

void kbdDrvSDLInputUnacquire(void) 
{
  // will be unacquired when performing gfxDrvSDL_Shutdown (SDL_Quit)
}


/*===========================================================================*/
/* Acquire SDL keyboard device                                               */
/*===========================================================================*/

void kbdDrvSDLInputAcquire(void) 
{
  // has been acquired by gfxDrvSDL_Startup (SDL_Init)
}


/*===========================================================================*/
/* Release SDL for keyboard                                                  */
/*===========================================================================*/

void kbdDrvSDLInputRelease(void) 
{
  // will be released when performing gfxDrvSDL_Shutdown (SDL_Quit)
}


/*===========================================================================*/
/* Initialize SDL for keyboard                                               */
/*===========================================================================*/

BOOLE kbdDrvSDLInputInitialize(void) 
{
  // keyboard driver is initilized by gfxDrvSDL_Startup (SDL_Init)
  
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

  kbd_drv_sdl_init_ok = gfxdrv_sdl_initialized;
  return kbd_drv_sdl_init_ok;
}


/*===========================================================================*/
/* keyboard grab status has changed                                          */
/*===========================================================================*/

void kbdDrvSDLStateHasChanged(BOOL window_active) 
{
  kbd_drv_sdl_active = window_active;

  if (kbd_drv_sdl_active) 
  {
    kbdDrvSDLInputAcquire();
  }
  else 
  {
    kbdDrvSDLInputUnacquire();
  }
}



/*===========================================================================*/
/* Keyboard keypress emulator control event checker                          */
/* Returns TRUE if the key generated an event                                */
/* which means it must not be passed to the emulator                         */
/*===========================================================================*/

BOOLE kbdDrvSDLEventChecker(SDL_KeyboardEvent* key_event) 
{
  ULO eol_evpos;
  ULO eof_evpos;
  ULO port;
  ULO setting;
  BOOLE home_is_down;
  BOOLE end_is_down;
  SDLKey current_key;
    
  eol_evpos = kbd_state.eventsEOL.inpos;
  eof_evpos = kbd_state.eventsEOF.inpos;
  
  current_key  = key_event->keysym.sym;
  home_is_down = kbd_drv_sdl_keys[SDLK_HOME];
  end_is_down  = kbd_drv_sdl_keys[SDLK_END];

  for (;;) 
  {

  // pagedown pushed in combination with home or end
  if ((kbd_drv_sdl_keys[SDLK_PAGEDOWN]) && (!(kbd_drv_sdl_prevkeys[SDLK_PAGEDOWN])))
	{
	  if (home_is_down)
	  {
      // switch to next resulution 
		  kbdEventEOFAdd(EVENT_RESOLUTION_NEXT);
      break;
	  } 
    else if(end_is_down)
    {
      // switch to next scaling for y direction
		  kbdEventEOFAdd(EVENT_SCALEY_NEXT);
      break;
    }
	}

  // pageup pushed in combination with home or end
	if((kbd_drv_sdl_keys[SDLK_PAGEUP]) && (!(kbd_drv_sdl_prevkeys[SDLK_PAGEUP])))
	{
	  if (home_is_down)
	  {
	  	kbdEventEOFAdd(EVENT_RESOLUTION_PREV);
      break;
	  } 
    else if(end_is_down)
    {
		  kbdEventEOFAdd(EVENT_SCALEY_PREV);
      break;
    }
	}
	if((kbd_drv_sdl_keys[SDLK_F11]) && (!(kbd_drv_sdl_prevkeys[SDLK_F11])))
  {
	  kbdEventEOFAdd(EVENT_EXIT);
    break;
  }

	if((kbd_drv_sdl_keys[SDLK_F12]) && (!(kbd_drv_sdl_prevkeys[SDLK_F12])))
	{
    mouseDrvSDLToggleFocus();
	  //joyDrvToggleFocus();
	  break;
	}

  if((kbd_drv_sdl_keys[SDLK_SYSREQ]) && (!(kbd_drv_sdl_prevkeys[SDLK_SYSREQ])))
	{
    // toggle for running emulation at full speed or synchronized
    soundToggleSynchronization();
    break;
	}
/*	
	if (home_pressed)
	{
	  if( released( A_F1 )) kbdEventEOFAdd( EVENT_INSERT_DF0 );
	  if( released( A_F2 )) kbdEventEOFAdd( EVENT_INSERT_DF1 );
	  if( released( A_F3 )) kbdEventEOFAdd( EVENT_INSERT_DF2 );
	  if( released( A_F4 )) kbdEventEOFAdd( EVENT_INSERT_DF3 );
	}
	else if( end_is_down )
	{
	  if( released( A_F1 )) kbdEventEOFAdd( EVENT_EJECT_DF0 );
	  if( released( A_F2 )) kbdEventEOFAdd( EVENT_EJECT_DF1 );
	  if( released( A_F3 )) kbdEventEOFAdd( EVENT_EJECT_DF2 );
	  if( released( A_F4 )) kbdEventEOFAdd( EVENT_EJECT_DF3 );
	}
	/*
	if( released( A_F11 ) && kbd_drv_home_pressed )
	kbdEventEOFAdd( EVENT_BMP_DUMP );
	*/
	
  // performing scrolling of emulation window
	if (home_is_down)
	{
	  if (kbd_drv_sdl_keys[SDLK_KP2])
    {
		  kbdEventEOFAdd(EVENT_SCROLL_DOWN);
      break;
    }
	
    if (kbd_drv_sdl_keys[SDLK_KP4])
    {
		  kbdEventEOFAdd(EVENT_SCROLL_LEFT);
      break;
    }
	  
	  if (kbd_drv_sdl_keys[SDLK_KP6])
    {
		  kbdEventEOFAdd(EVENT_SCROLL_RIGHT);
      break;
    }

	  if (kbd_drv_sdl_keys[SDLK_KP8])
    {
		  kbdEventEOFAdd(EVENT_SCROLL_UP);
      break;
    }
	}
	
	// check joysticks replacement by keyboard
	for (port = 0; port < 2; port++)
  {
	  for(setting = 0; setting < 2; setting++)
    {
		  if (kbd_drv_sdl_joykey_enabled[port][setting])
		  {
		    // Here the gameport is set up for input from the specified set of joykeys 
		    // Check each key for change
		    kbd_drv_sdl_joykey_directions direction;

        // check all joystick replacement keys
		    for (direction = 0; direction < KBDDRVSDL_MAX_JOYKEY_VALUE; direction++)
        {
			    if (current_key == kbd_drv_sdl_joykey[setting][direction])
          {
            // is the key pressed or released (transition)
			      if ((kbd_drv_sdl_keys[current_key]) && (!(kbd_drv_sdl_prevkeys[current_key])) ||
              (!(kbd_drv_sdl_keys[current_key])) && (kbd_drv_sdl_prevkeys[current_key]))
            {
  				    kbdEventEOLAdd(kbd_drv_sdl_joykey_event[port][(kbd_drv_sdl_keys[current_key]) && (!(kbd_drv_sdl_prevkeys[current_key]))][direction]);
              break;
            }
          }
        }
		  }
    }
  }
  break;  
  }
  return (eol_evpos != kbd_state.eventsEOL.inpos) || (eof_evpos != kbd_state.eventsEOF.inpos);
}


/*===========================================================================*/
/* Handle one specific keycode change                                        */
/*===========================================================================*/
    
void kbdDrvSDLKeypress(SDL_KeyboardEvent* key_event) 
{
	SDLKey sdl_key;
  UBY    amiga_key;
  BOOLE  key_pressed;
  BOOLE  key_prev_pressed;

  sdl_key   = key_event->keysym.sym;
  amiga_key = sdl_to_amiga_keycode[sdl_key]; 
	
  // update key pressed book keeping
  key_pressed               = (key_event->state == SDL_PRESSED);
	key_prev_pressed          = kbd_drv_sdl_prevkeys[sdl_key];
	kbd_drv_sdl_keys[sdl_key] = key_pressed;

  // we only handle key state transitions

  // check if key state changed from pressed to released
	if ((!key_pressed) && key_prev_pressed) 
  {
		// check if a key press must be passed to the application
    // and not to the emulation itself
		if (!kbdDrvSDLEventChecker(key_event))
		{
			kbdKeyAdd(amiga_key | 0x80);
		}
	}
  // check if key state changed from released to pressed
	else if (key_pressed && (!key_prev_pressed)) 
  {
    // check if a key press must be passed to the application
    // and not to the emulation itself
    if (!kbdDrvSDLEventChecker(key_event))
		{
			kbdKeyAdd(amiga_key);
		}
	}
	kbd_drv_sdl_prevkeys[sdl_key] = key_pressed;
}


/*===========================================================================*/
/* Keyboard keypress handler                                                 */
/*===========================================================================*/

void kbdDrvSDLKeypressHandler(SDL_Event* event)
{
		
  if(kbd_drv_sdl_active)
  {
	  kbdDrvSDLKeypress(&(event->key));
  }
}    
    
/*===========================================================================*/
/* Return string describing the given symbolic key                           */
/*===========================================================================*/

STR *kbdDrvSDLKeyString(SDLKey key) {
  if (key >= SDLK_LAST)
  {
    return "unknown";
  }
  return SDL_GetKeyName(key);
}


/*===========================================================================*/
/* Get joystick replacement for a given joystick and direction               */
/*===========================================================================*/

ULO kbdDrvSDLJoystickReplacementGet(kbd_event event) 
{
  switch (event) 
  {
    case EVENT_JOY0_UP_ACTIVE:
      return kbd_drv_sdl_joykey[0][JOYKEY_UP];
    case EVENT_JOY0_DOWN_ACTIVE:
      return kbd_drv_sdl_joykey[0][JOYKEY_DOWN];
    case EVENT_JOY0_LEFT_ACTIVE:
      return kbd_drv_sdl_joykey[0][JOYKEY_LEFT];
    case EVENT_JOY0_RIGHT_ACTIVE:
      return kbd_drv_sdl_joykey[0][JOYKEY_RIGHT];
    case EVENT_JOY0_FIRE0_ACTIVE:
      return kbd_drv_sdl_joykey[0][JOYKEY_FIRE0];
    case EVENT_JOY0_FIRE1_ACTIVE:
      return kbd_drv_sdl_joykey[0][JOYKEY_FIRE1];
    case EVENT_JOY0_AUTOFIRE0_ACTIVE:
      return kbd_drv_sdl_joykey[0][JOYKEY_AUTOFIRE0];
    case EVENT_JOY0_AUTOFIRE1_ACTIVE:
      return kbd_drv_sdl_joykey[0][JOYKEY_AUTOFIRE1];
    case EVENT_JOY1_UP_ACTIVE:
      return kbd_drv_sdl_joykey[1][JOYKEY_UP];
    case EVENT_JOY1_DOWN_ACTIVE:
      return kbd_drv_sdl_joykey[1][JOYKEY_DOWN];
    case EVENT_JOY1_LEFT_ACTIVE:
      return kbd_drv_sdl_joykey[1][JOYKEY_LEFT];
    case EVENT_JOY1_RIGHT_ACTIVE:
      return kbd_drv_sdl_joykey[1][JOYKEY_RIGHT];
    case EVENT_JOY1_FIRE0_ACTIVE:
      return kbd_drv_sdl_joykey[1][JOYKEY_FIRE0];
    case EVENT_JOY1_FIRE1_ACTIVE:
      return kbd_drv_sdl_joykey[1][JOYKEY_FIRE1];
    case EVENT_JOY1_AUTOFIRE0_ACTIVE:
      return kbd_drv_sdl_joykey[1][JOYKEY_AUTOFIRE0];
    case EVENT_JOY1_AUTOFIRE1_ACTIVE:
      return kbd_drv_sdl_joykey[1][JOYKEY_AUTOFIRE1];
  }
  return PC_NONE;
}


/*===========================================================================*/
/* Hard Reset                                                                */
/*===========================================================================*/

void kbdDrvSDLHardReset(void) 
{
  kbdDrvSDLEmulationStart();
}


/*===========================================================================*/
/* Emulation Starting                                                        */
/*===========================================================================*/

void kbdDrvSDLEmulationStart(void) 
{
  ULO port;
  
  for (port = 0; port < 2; port++) 
  {
    kbd_drv_sdl_joykey_enabled[port][0] = (gameport_input[port] == GP_JOYKEY0);
    kbd_drv_sdl_joykey_enabled[port][1] = (gameport_input[port] == GP_JOYKEY1);
  }
  
  memset( kbd_drv_sdl_prevkeys, 0, sizeof( kbd_drv_sdl_prevkeys ));
  memset( kbd_drv_sdl_keys, 0, sizeof( kbd_drv_sdl_keys ));
  
  kbdDrvSDLStateHasChanged((SDL_GetAppState() & SDL_APPINPUTFOCUS) == SDL_APPINPUTFOCUS);

  kbdDrvSDLInputInitialize();
}


/*===========================================================================*/
/* Emulation Stopping                                                        */
/*===========================================================================*/

void kbdDrvSDLEmulationStop(void) 
{
}

/*===========================================================================*/
/* Emulation Startup                                                         */
/*===========================================================================*/

void kbdDrvSDLStartup(void) 
{
  ULO port, setting;
  ULO i;
  
  kbd_drv_sdl_joykey_event[0][0][JOYKEY_UP]     = EVENT_JOY0_UP_INACTIVE;
  kbd_drv_sdl_joykey_event[0][1][JOYKEY_UP]     = EVENT_JOY0_UP_ACTIVE;
  kbd_drv_sdl_joykey_event[1][0][JOYKEY_UP]     = EVENT_JOY1_UP_INACTIVE;
  kbd_drv_sdl_joykey_event[1][1][JOYKEY_UP]     = EVENT_JOY1_UP_ACTIVE;
  kbd_drv_sdl_joykey_event[0][0][JOYKEY_DOWN]   = EVENT_JOY0_DOWN_INACTIVE;
  kbd_drv_sdl_joykey_event[0][1][JOYKEY_DOWN]   = EVENT_JOY0_DOWN_ACTIVE;
  kbd_drv_sdl_joykey_event[1][0][JOYKEY_DOWN]   = EVENT_JOY1_DOWN_INACTIVE;
  kbd_drv_sdl_joykey_event[1][1][JOYKEY_DOWN]   = EVENT_JOY1_DOWN_ACTIVE;
  kbd_drv_sdl_joykey_event[0][0][JOYKEY_LEFT]   = EVENT_JOY0_LEFT_INACTIVE;
  kbd_drv_sdl_joykey_event[0][1][JOYKEY_LEFT]   = EVENT_JOY0_LEFT_ACTIVE;
  kbd_drv_sdl_joykey_event[1][0][JOYKEY_LEFT]   = EVENT_JOY1_LEFT_INACTIVE;
  kbd_drv_sdl_joykey_event[1][1][JOYKEY_LEFT]   = EVENT_JOY1_LEFT_ACTIVE;
  kbd_drv_sdl_joykey_event[0][0][JOYKEY_RIGHT]  = EVENT_JOY0_RIGHT_INACTIVE;
  kbd_drv_sdl_joykey_event[0][1][JOYKEY_RIGHT]  = EVENT_JOY0_RIGHT_ACTIVE;
  kbd_drv_sdl_joykey_event[1][0][JOYKEY_RIGHT]  = EVENT_JOY1_RIGHT_INACTIVE;
  kbd_drv_sdl_joykey_event[1][1][JOYKEY_RIGHT]  = EVENT_JOY1_RIGHT_ACTIVE;
  kbd_drv_sdl_joykey_event[0][0][JOYKEY_FIRE0]  = EVENT_JOY0_FIRE0_INACTIVE;
  kbd_drv_sdl_joykey_event[0][1][JOYKEY_FIRE0]  = EVENT_JOY0_FIRE0_ACTIVE;
  kbd_drv_sdl_joykey_event[1][0][JOYKEY_FIRE0]  = EVENT_JOY1_FIRE0_INACTIVE;
  kbd_drv_sdl_joykey_event[1][1][JOYKEY_FIRE0]  = EVENT_JOY1_FIRE0_ACTIVE;
  kbd_drv_sdl_joykey_event[0][0][JOYKEY_FIRE1]  = EVENT_JOY0_FIRE1_INACTIVE;
  kbd_drv_sdl_joykey_event[0][1][JOYKEY_FIRE1]  = EVENT_JOY0_FIRE1_ACTIVE;
  kbd_drv_sdl_joykey_event[1][0][JOYKEY_FIRE1]  = EVENT_JOY1_FIRE1_INACTIVE;
  kbd_drv_sdl_joykey_event[1][1][JOYKEY_FIRE1]  = EVENT_JOY1_FIRE1_ACTIVE;
  kbd_drv_sdl_joykey_event[0][0][JOYKEY_AUTOFIRE0] = EVENT_JOY0_AUTOFIRE0_INACTIVE;
  kbd_drv_sdl_joykey_event[0][1][JOYKEY_AUTOFIRE0] = EVENT_JOY0_AUTOFIRE0_ACTIVE;
  kbd_drv_sdl_joykey_event[1][0][JOYKEY_AUTOFIRE0] = EVENT_JOY1_AUTOFIRE0_INACTIVE;
  kbd_drv_sdl_joykey_event[1][1][JOYKEY_AUTOFIRE0] = EVENT_JOY1_AUTOFIRE0_ACTIVE;
  kbd_drv_sdl_joykey_event[0][0][JOYKEY_AUTOFIRE1] = EVENT_JOY0_AUTOFIRE1_INACTIVE;
  kbd_drv_sdl_joykey_event[0][1][JOYKEY_AUTOFIRE1] = EVENT_JOY0_AUTOFIRE1_ACTIVE;
  kbd_drv_sdl_joykey_event[1][0][JOYKEY_AUTOFIRE1] = EVENT_JOY1_AUTOFIRE1_INACTIVE;
  kbd_drv_sdl_joykey_event[1][1][JOYKEY_AUTOFIRE1] = EVENT_JOY1_AUTOFIRE1_ACTIVE;
  
  kbd_drv_sdl_joykey[0][JOYKEY_UP]        = SDLK_UP;
  kbd_drv_sdl_joykey[0][JOYKEY_DOWN]      = SDLK_DOWN;
  kbd_drv_sdl_joykey[0][JOYKEY_LEFT]      = SDLK_LEFT;
  kbd_drv_sdl_joykey[0][JOYKEY_RIGHT]     = SDLK_RIGHT;
  kbd_drv_sdl_joykey[0][JOYKEY_FIRE0]     = SDLK_RCTRL;
  kbd_drv_sdl_joykey[0][JOYKEY_FIRE1]     = SDLK_MENU;
  kbd_drv_sdl_joykey[0][JOYKEY_AUTOFIRE0] = SDLK_o;
  kbd_drv_sdl_joykey[0][JOYKEY_AUTOFIRE1] = SDLK_p;
  kbd_drv_sdl_joykey[1][JOYKEY_UP]        = SDLK_r;
  kbd_drv_sdl_joykey[1][JOYKEY_DOWN]      = SDLK_f;
  kbd_drv_sdl_joykey[1][JOYKEY_LEFT]      = SDLK_d;
  kbd_drv_sdl_joykey[1][JOYKEY_RIGHT]     = SDLK_g;
  kbd_drv_sdl_joykey[1][JOYKEY_FIRE0]     = SDLK_LCTRL;
  kbd_drv_sdl_joykey[1][JOYKEY_FIRE1]     = SDLK_MENU;
  kbd_drv_sdl_joykey[1][JOYKEY_AUTOFIRE0] = SDLK_a;
  kbd_drv_sdl_joykey[1][JOYKEY_AUTOFIRE1] = SDLK_s;
  
  for (port = 0; port < 2; port++)
    for (setting = 0; setting < 2; setting++)
      kbd_drv_sdl_joykey_enabled[port][setting] = FALSE;
    
  for( i = 0; i < SDLK_LAST; i++ )
    sdl_to_amiga_keycode[i] = A_NONE;

  // first row of keys on standard US keyboard

  sdl_to_amiga_keycode[ SDLK_ESCAPE          ] = A_ESCAPE; 
  sdl_to_amiga_keycode[ SDLK_F1              ] = A_F1;
  sdl_to_amiga_keycode[ SDLK_F2              ] = A_F2;
  sdl_to_amiga_keycode[ SDLK_F3              ] = A_F3;
  sdl_to_amiga_keycode[ SDLK_F4              ] = A_F4;
  sdl_to_amiga_keycode[ SDLK_F5              ] = A_F5;
  sdl_to_amiga_keycode[ SDLK_F6              ] = A_F6;
  sdl_to_amiga_keycode[ SDLK_F7              ] = A_F7;
  sdl_to_amiga_keycode[ SDLK_F8              ] = A_F8;
  sdl_to_amiga_keycode[ SDLK_F9              ] = A_F9;
  sdl_to_amiga_keycode[ SDLK_F10             ] = A_F10;
  sdl_to_amiga_keycode[ SDLK_F11             ] = A_NONE;
  sdl_to_amiga_keycode[ SDLK_F12             ] = A_NONE;
  sdl_to_amiga_keycode[ SDLK_SYSREQ          ] = A_NONE;
  sdl_to_amiga_keycode[ SDLK_SCROLLOCK       ] = A_NONE;
  sdl_to_amiga_keycode[ SDLK_BREAK           ] = A_NONE;
  
  // second row

  sdl_to_amiga_keycode[ SDLK_BACKQUOTE       ] = A_GRAVE;  
  sdl_to_amiga_keycode[ SDLK_1               ] = A_1;
  sdl_to_amiga_keycode[ SDLK_2               ] = A_2;
  sdl_to_amiga_keycode[ SDLK_3               ] = A_3;
  sdl_to_amiga_keycode[ SDLK_4               ] = A_4;
  sdl_to_amiga_keycode[ SDLK_5               ] = A_5;
  sdl_to_amiga_keycode[ SDLK_6               ] = A_6;
  sdl_to_amiga_keycode[ SDLK_7               ] = A_7;
  sdl_to_amiga_keycode[ SDLK_8               ] = A_8;
  sdl_to_amiga_keycode[ SDLK_9               ] = A_9;
  sdl_to_amiga_keycode[ SDLK_0               ] = A_0;
  sdl_to_amiga_keycode[ SDLK_MINUS           ] = A_MINUS;
  sdl_to_amiga_keycode[ SDLK_EQUALS          ] = A_EQUALS;
  sdl_to_amiga_keycode[ SDLK_BACKSPACE       ] = A_BACKSPACE;
  sdl_to_amiga_keycode[ SDLK_INSERT          ] = A_NONE;
  sdl_to_amiga_keycode[ SDLK_HOME            ] = A_NONE;
  sdl_to_amiga_keycode[ SDLK_PAGEUP          ] = A_NONE;
  sdl_to_amiga_keycode[ SDLK_NUMLOCK         ] = A_NONE;
  sdl_to_amiga_keycode[ SDLK_KP_DIVIDE       ] = A_NUMPAD_DIVIDE;
  sdl_to_amiga_keycode[ SDLK_KP_MULTIPLY     ] = A_NUMPAD_MULTIPLY;
  sdl_to_amiga_keycode[ SDLK_KP_MINUS        ] = A_NUMPAD_MINUS;

  // third row 

  sdl_to_amiga_keycode[ SDLK_TAB             ] = A_TAB;  
  sdl_to_amiga_keycode[ SDLK_q               ] = A_Q;
  sdl_to_amiga_keycode[ SDLK_w               ] = A_W;
  sdl_to_amiga_keycode[ SDLK_e               ] = A_E;
  sdl_to_amiga_keycode[ SDLK_r               ] = A_R;
  sdl_to_amiga_keycode[ SDLK_t               ] = A_T;
  sdl_to_amiga_keycode[ SDLK_y               ] = A_Y;
  sdl_to_amiga_keycode[ SDLK_u               ] = A_U;
  sdl_to_amiga_keycode[ SDLK_i               ] = A_I;
  sdl_to_amiga_keycode[ SDLK_o               ] = A_O;
  sdl_to_amiga_keycode[ SDLK_p               ] = A_P;
  sdl_to_amiga_keycode[ SDLK_LEFTBRACKET     ] = A_LEFT_BRACKET;
  sdl_to_amiga_keycode[ SDLK_RIGHTBRACKET    ] = A_RIGHT_BRACKET;
  sdl_to_amiga_keycode[ SDLK_RETURN          ] = A_RETURN;
  sdl_to_amiga_keycode[ SDLK_DELETE          ] = A_DELETE;
  sdl_to_amiga_keycode[ SDLK_END             ] = A_NONE;
  sdl_to_amiga_keycode[ SDLK_PAGEDOWN        ] = A_NONE;
  sdl_to_amiga_keycode[ SDLK_KP7             ] = A_NUMPAD_7;
  sdl_to_amiga_keycode[ SDLK_KP8             ] = A_NUMPAD_8;
  sdl_to_amiga_keycode[ SDLK_KP9             ] = A_NUMPAD_9;
  sdl_to_amiga_keycode[ SDLK_KP_PLUS         ] = A_NUMPAD_PLUS;
  
  // fourth row

  sdl_to_amiga_keycode[ SDLK_CAPSLOCK        ] = A_CAPS_LOCK; 
  sdl_to_amiga_keycode[ SDLK_a               ] = A_A;
  sdl_to_amiga_keycode[ SDLK_s               ] = A_S;
  sdl_to_amiga_keycode[ SDLK_d               ] = A_D;
  sdl_to_amiga_keycode[ SDLK_f               ] = A_F;
  sdl_to_amiga_keycode[ SDLK_g               ] = A_G;
  sdl_to_amiga_keycode[ SDLK_h               ] = A_H;
  sdl_to_amiga_keycode[ SDLK_j               ] = A_J;
  sdl_to_amiga_keycode[ SDLK_k               ] = A_K;
  sdl_to_amiga_keycode[ SDLK_l               ] = A_L;
  sdl_to_amiga_keycode[ SDLK_SEMICOLON       ] = A_SEMICOLON;
  sdl_to_amiga_keycode[ SDLK_QUOTE           ] = A_APOSTROPHE;
  sdl_to_amiga_keycode[ SDLK_BACKSLASH       ] = A_BACKSLASH;
  sdl_to_amiga_keycode[ SDLK_KP4             ] = A_NUMPAD_4;
  sdl_to_amiga_keycode[ SDLK_KP5             ] = A_NUMPAD_5;
  sdl_to_amiga_keycode[ SDLK_KP6             ] = A_NUMPAD_6;
  
  // fifth row 

  sdl_to_amiga_keycode[ SDLK_LSHIFT          ] = A_LEFT_SHIFT; 
  sdl_to_amiga_keycode[ SDLK_z               ] = A_Z;
  sdl_to_amiga_keycode[ SDLK_x               ] = A_X;
  sdl_to_amiga_keycode[ SDLK_c               ] = A_C;
  sdl_to_amiga_keycode[ SDLK_v               ] = A_V;
  sdl_to_amiga_keycode[ SDLK_b               ] = A_B;
  sdl_to_amiga_keycode[ SDLK_n               ] = A_N;
  sdl_to_amiga_keycode[ SDLK_m               ] = A_M;
  sdl_to_amiga_keycode[ SDLK_COMMA           ] = A_COMMA;
  sdl_to_amiga_keycode[ SDLK_PERIOD          ] = A_PERIOD;
  sdl_to_amiga_keycode[ SDLK_SLASH           ] = A_SLASH;
  sdl_to_amiga_keycode[ SDLK_RSHIFT          ] = A_RIGHT_SHIFT;
  sdl_to_amiga_keycode[ SDLK_UP              ] = A_UP;
  sdl_to_amiga_keycode[ SDLK_KP1             ] = A_NUMPAD_1;
  sdl_to_amiga_keycode[ SDLK_KP2             ] = A_NUMPAD_2;
  sdl_to_amiga_keycode[ SDLK_KP3             ] = A_NUMPAD_3;
  sdl_to_amiga_keycode[ SDLK_KP_ENTER        ] = A_NUMPAD_ENTER;
  
  // sixth row 

  sdl_to_amiga_keycode[ SDLK_LCTRL           ] = A_CTRL; 
  sdl_to_amiga_keycode[ SDLK_LSUPER          ] = A_LEFT_AMIGA;
  sdl_to_amiga_keycode[ SDLK_LALT            ] = A_LEFT_ALT;
  sdl_to_amiga_keycode[ SDLK_SPACE           ] = A_SPACE;
  sdl_to_amiga_keycode[ SDLK_RALT            ] = A_RIGHT_ALT;
  sdl_to_amiga_keycode[ SDLK_RSUPER          ] = A_RIGHT_AMIGA;
  sdl_to_amiga_keycode[ SDLK_MENU            ] = A_NONE;
  sdl_to_amiga_keycode[ SDLK_RCTRL           ] = A_CTRL;
  sdl_to_amiga_keycode[ SDLK_LEFT            ] = A_LEFT;
  sdl_to_amiga_keycode[ SDLK_DOWN            ] = A_DOWN;
  sdl_to_amiga_keycode[ SDLK_RIGHT           ] = A_RIGHT;
  sdl_to_amiga_keycode[ SDLK_KP0             ] = A_NUMPAD_0;
  sdl_to_amiga_keycode[ SDLK_KP_PERIOD       ] = A_NUMPAD_DOT;

  // missing in above map A_BACKSLASH2, A_LESS_THAN and A_HELP

  kbd_drv_sdl_rewrite_mapping_file = prsReadFile( SDL_MAPPING_FILENAME, sdl_to_amiga_keycode, kbd_drv_sdl_joykey );

  // set replacement keys for joystick
  for (port = 0; port < 2; port++)
  {
    for (setting = 0; setting < KBDDRVSDL_MAX_JOYKEY_VALUE; setting++) {

			//kbd_drv_sdl_joykey[port][setting] = sdl_to_amiga_keycode[kbd_drv_sdl_joykey[port][setting]];

#ifdef _DEBUG
			fellowAddLog( "keyplacement port %d direction %d: %s\n", port, setting, kbdDrvSDLKeyString( kbd_drv_sdl_joykey[port][setting] ));
#endif

		}
  }

  kbd_drv_sdl_active = FALSE;
}

/*===========================================================================*/
/* Emulation Shutdown                                                        */
/*===========================================================================*/

void kbdDrvSDLShutdown(void) 
{
  if( kbd_drv_sdl_rewrite_mapping_file )
  {
    prsWriteFile( SDL_MAPPING_FILENAME, sdl_to_amiga_keycode, kbd_drv_sdl_joykey );
  }
}
