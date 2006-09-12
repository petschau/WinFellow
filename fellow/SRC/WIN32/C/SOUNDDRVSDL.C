/*===========================================================================*/
/* Fellow Amiga Emulator                                                     */
/* Sound driver based on SDL                                                 */
/* Author: Worfje (worfje@gmx.net)                                           */
/*                                                                           */
/* This file is under the GNU Public License (GPL)                           */
/*===========================================================================*/

#include "SDL.h"
#include "defs.h"
#include "fellow.h"
#include "sound.h"
#include "listtree.h"
#include "sounddrvsdl.h"


/*===========================================================================*/
/* Some structs where we put all our sound specific stuff                    */
/*===========================================================================*/

typedef struct {
  ULO   rate;
  BOOLE bits16;
  BOOLE stereo;
  ULO   buffer_sample_count;
  ULO   buffer_block_align;
} sound_drv_sdl_mode;


typedef struct {
  felist *modes;
  sound_drv_sdl_mode* mode_current;
  UWO *pending_data_left;
  UWO *pending_data_right;
  HANDLE data_available;
  HANDLE can_add_data;
  HANDLE end_emulation;
  ULO pending_data_sample_count;
  DWORD lastreadpos;
} sound_drv_sdl_device;


/*==========================================================================*/
/* currently active SDL device                                              */
/*==========================================================================*/

sound_drv_sdl_device sound_drv_sdl_device_current;
BOOLE waiting_for_termination;

/*==========================================================================*/
/* Forward decleration of functions                                         */
/*==========================================================================*/

void soundDrvSDL_Callback(void *userdata, Uint8 *stream, int len);


/*==========================================================================*/
/* Returns textual error message.                                           */
/*==========================================================================*/

STR *soundDrvSDL_ErrorString() 
{
  return SDL_GetError();
}

/*==========================================================================*/
/* Logs a sensible error message                                            */
/*==========================================================================*/

void soundDrvSDL_Failure(STR *header) 
{
  fellowAddLog("sounddrvSDL: ");
  fellowAddLog(header);
  fellowAddLog(soundDrvSDL_ErrorString());
  fellowAddLog("\n");
}


/*===========================================================================*/
/* Adds a mode to the sound_drv_sdl_device struct                            */
/*===========================================================================*/

void soundDrvSDL_AddMode(sound_drv_sdl_device *sdl_device, BOOLE stereo, BOOLE bits16, ULO rate) 
{
  sound_drv_sdl_mode *sdl_mode = (sound_drv_sdl_mode *) malloc(sizeof(sound_drv_sdl_mode));
  sdl_mode->stereo   = stereo;
  sdl_mode->bits16   = bits16;
  sdl_mode->rate     = rate;
  sdl_mode->buffer_sample_count = 0;
  sdl_device->modes  = listAddLast(sdl_device->modes, listNew(sdl_mode));
}


/*===========================================================================*/
/* Finds a mode in the sound_drv_sdl_device struct                           */
/*===========================================================================*/

sound_drv_sdl_mode *soundDrvSDL_FindMode(sound_drv_sdl_device *sdl_device, BOOLE stereo,	BOOLE bits16, ULO rate) 
{
  felist *fl;

  for (fl = sdl_device->modes; fl != NULL; fl = listNext(fl)) 
  {
    sound_drv_sdl_mode *mode = (sound_drv_sdl_mode *) listNode(fl);
    if ((mode->rate == rate) && (mode->stereo == stereo && mode->bits16 == bits16))
    {
      return mode;
    }
  }
  return NULL;
}


/*===========================================================================*/
/* Release SDL mode information                                              */
/*===========================================================================*/

void soundDrvSDL_ModeInformationRelease(sound_drv_sdl_device *sdl_device) 
{
  listFreeAll(sdl_device->modes, TRUE);
  sdl_device->modes = NULL;
}


/*===========================================================================*/
/* Initialize DirectSound mode information                                   */
/*===========================================================================*/

BOOLE soundDrvSDL_ModeInformationInitialize(sound_drv_sdl_device *sdl_device) 
{
  // 16 bit stereo modes
	soundDrvSDL_AddMode(sdl_device, TRUE, TRUE, 15650);
	soundDrvSDL_AddMode(sdl_device, TRUE, TRUE, 22050);
	soundDrvSDL_AddMode(sdl_device, TRUE, TRUE, 31300);
	soundDrvSDL_AddMode(sdl_device, TRUE, TRUE, 44100);
  //soundDrvSDL_AddMode(sdl_device, TRUE, TRUE, 48000);

  // 8 bit stereo modes
	soundDrvSDL_AddMode(sdl_device, TRUE, FALSE, 15650);
	soundDrvSDL_AddMode(sdl_device, TRUE, FALSE, 22050);
	soundDrvSDL_AddMode(sdl_device, TRUE, FALSE, 31300);
	soundDrvSDL_AddMode(sdl_device, TRUE, FALSE, 44100);
  //soundDrvSDL_AddMode(sdl_device, TRUE, FALSE, 48000);

  // 16 bit mono modes
	soundDrvSDL_AddMode(sdl_device, FALSE, TRUE, 15650);
	soundDrvSDL_AddMode(sdl_device, FALSE, TRUE, 22050);
	soundDrvSDL_AddMode(sdl_device, FALSE, TRUE, 31300);
	soundDrvSDL_AddMode(sdl_device, FALSE, TRUE, 44100);
  //soundDrvSDL_AddMode(sdl_device, FALSE, TRUE, 48000);

  // 8 bit mono modes
  soundDrvSDL_AddMode(sdl_device, FALSE, FALSE, 15650);
	soundDrvSDL_AddMode(sdl_device, FALSE, FALSE, 22050);
	soundDrvSDL_AddMode(sdl_device, FALSE, FALSE, 31300);
	soundDrvSDL_AddMode(sdl_device, FALSE, FALSE, 44100);
  //soundDrvSDL_AddMode(sdl_device, FALSE, FALSE, 48000);

  return TRUE;
}


/*===========================================================================*/
/* Initialize playback                                                      */
/*===========================================================================*/

BOOLE soundDrvSDL_PlaybackInitialize(sound_drv_sdl_device *sdl_device) 
{
  BOOLE result;
  SDL_AudioSpec desired;
  SDL_AudioSpec obtained;

  result = TRUE;

  desired.freq     = sdl_device->mode_current->rate;
  desired.format   = (sdl_device->mode_current->bits16) ? AUDIO_S16SYS : AUDIO_U8;
  desired.channels = (sdl_device->mode_current->stereo) ? 2 : 1;
  desired.samples  = sdl_device->mode_current->buffer_sample_count;
  desired.callback = soundDrvSDL_Callback;

  // open the SDL audio device
  if (SDL_OpenAudio(&desired, &obtained) < 0)
  {
    soundDrvSDL_Failure("sounddrvSDL: initialize failed, ");
    result = FALSE;
  }

  // check if received matches request
  if ( !( 
    (obtained.freq     == desired.freq) &&
    (obtained.format   == desired.format) &&
    (obtained.channels == desired.channels) &&
    (obtained.samples  == desired.samples)
    ) )
  {
    soundDrvSDL_Failure("sounddrvSDL: hardware did not support requested mode, ");
    result = FALSE;
  }

  sdl_device->data_available = CreateEvent(0, 0, 0, 0);
  sdl_device->can_add_data   = CreateEvent(0, 0, 0, 0);
  sdl_device->end_emulation  = CreateEvent(0, 0, 0, 0);
  
  return result;
}


/*===========================================================================*/
/* Copy data to a buffer                                                     */
/*===========================================================================*/

void soundDrvSDL_Copy16BitsStereo(UWO *audio_buffer, UWO *left, UWO *right, ULO sample_count) 
{
  ULO i;

  for (i = 0; i < sample_count; i++) 
  {
    *audio_buffer++ = *left++;
    *audio_buffer++ = *right++;
  }
}

void soundDrvSDL_Copy16BitsMono(UWO *audio_buffer, UWO *left, UWO *right, ULO sample_count) 
{
  ULO i;

  for (i = 0; i < sample_count; i++)
  {
    *audio_buffer++ = ((*left++) + (*right++));
  }
}

void soundDrvSDL_Copy8BitsStereo(UBY *audio_buffer, UWO *left, UWO *right, ULO sample_count) 
{
  ULO i;

  for (i = 0; i < sample_count; i++) 
  {
    *audio_buffer++ = ((*left++)>>8) + 128;
    *audio_buffer++ = ((*right++)>>8) + 128;
  }
}

void soundDrvSDL_Copy8BitsMono(UBY *audio_buffer, UWO *left, UWO *right, ULO sample_count) 
{
  ULO i;

  for (i = 0; i < sample_count; i++)
  {
    *audio_buffer++ = (((*left++) + (*right++))>>8) + 128;
  }
}

void soundDrvSDL_CopyToBuffer(UWO *buffer, WOR *left, WOR *right, ULO sample_count) 
{
  if (soundGetStereo()) 
  {
    if (soundGet16Bits()) 
    {
      soundDrvSDL_Copy16BitsStereo(buffer, left, right, sample_count);
    }
    else 
    {
      soundDrvSDL_Copy8BitsStereo((UBY *) buffer, left, right, sample_count);
    }
  }
  else 
  {
    if (soundGet16Bits()) 
    {
      soundDrvSDL_Copy16BitsMono(buffer, left, right, sample_count);
    }
    else 
    {
      soundDrvSDL_Copy8BitsMono((UBY *) buffer, left, right, sample_count);
    }
  }
}

/*===========================================================================*/
/* Play a buffer                                                             */
/* This is also where the emulator receives synchronization,                 */
/* since waiting for the sound-buffer to be                                  */
/* ready slows the emulator down to its original 50hz PAL speed.             */
/*===========================================================================*/

void soundDrvSDL_Play(WOR *left, WOR *right, ULO sample_count) 
{
  sound_drv_sdl_device *sdl_device = &sound_drv_sdl_device_current;

  WaitForSingleObject(sdl_device->can_add_data, INFINITE);
  sdl_device->pending_data_left = left;
  sdl_device->pending_data_right = right;
  sdl_device->pending_data_sample_count = sample_count;
  ResetEvent(sdl_device->can_add_data);
  SetEvent(sdl_device->data_available);

  // to improve performance on 10 ms buffer length 
  Sleep(1);
}

void soundDrvSDL_Callback(void *userdata, Uint8 *stream, int len)
{
  ULO i;
  HANDLE multi_events[2];
  DWORD ret_event;

  multi_events[0] = sound_drv_sdl_device_current.data_available;
  multi_events[1] = sound_drv_sdl_device_current.end_emulation;

  if (waiting_for_termination == FALSE)
  {
    ret_event = WaitForMultipleObjects(2, (void*const*) multi_events, FALSE, INFINITE);

    // check if we were signaled to terminate
    switch (ret_event)
    {
      case WAIT_OBJECT_0:

        soundDrvSDL_CopyToBuffer(
          (UWO *) stream, 
          sound_drv_sdl_device_current.pending_data_left,
          sound_drv_sdl_device_current.pending_data_right,
          sound_drv_sdl_device_current.pending_data_sample_count);
  
        // the sound thread has finished copying the sound buffer to its stream
        ResetEvent(sound_drv_sdl_device_current.data_available);
        SetEvent(sound_drv_sdl_device_current.can_add_data);

        // to improve performance on 10 ms buffer length 
        Sleep(1);
        break;

      case WAIT_OBJECT_0 + 1:

        // we are signaled to terminate by emulation thread
        waiting_for_termination = TRUE;
        break;
    }
  }
}


/*===========================================================================*/
/* Hard Reset                                                                */
/*===========================================================================*/

void soundDrvSDL_HardReset(void) 
{
}


/*===========================================================================*/
/* Emulation Starting                                                        */
/*===========================================================================*/

BOOLE soundDrvSDL_EmulationStart(ULO rate, BOOLE bits16, BOOLE stereo, ULO *sample_count_max) 
{
  sound_drv_sdl_device *sdl_device = &sound_drv_sdl_device_current;
  BOOLE result;
  ULO i;

  // check if the driver can support the requested sound quality 
  sdl_device->mode_current = soundDrvSDL_FindMode(sdl_device, stereo, bits16, rate);
  result = (sdl_device->mode_current != NULL);

  // record the number of samples in our buffer (i.e. one half of the size) 
  if (result) 
  {
    sdl_device->mode_current->buffer_sample_count = *sample_count_max;
    
    result = soundDrvSDL_PlaybackInitialize(sdl_device);
  }

  // set all events to their initial state 
  ResetEvent(sdl_device->data_available);
  SetEvent(sdl_device->can_add_data);
  ResetEvent(sdl_device->end_emulation);

  // set global variable which is set when emulation is ending
  waiting_for_termination = FALSE;

  if (result) 
  {
    // start playback
    SDL_PauseAudio(0);
  }

  // in case of failure, we undo any stuff we've done so far 
  if (!result) 
  {
    soundDrvSDL_Failure("sounddrvSDL: failed to start sound\n");
    SDL_CloseAudio();
  }

  return result;
}


/*===========================================================================*/
/* Emulation Stopping                                                        */
/*===========================================================================*/

void soundDrvSDL_EmulationStop(void) 
{
  // signal the callback to skip waiting on the 'data is available' event
  SetEvent(sound_drv_sdl_device_current.end_emulation);

  // terminate the audio driver 
  SDL_CloseAudio();
  
  if (sound_drv_sdl_device_current.data_available != NULL)
  {
    CloseHandle(sound_drv_sdl_device_current.data_available);
  }

  if (sound_drv_sdl_device_current.can_add_data != NULL)
  {
    CloseHandle(sound_drv_sdl_device_current.can_add_data);
  }

  //SetEvent(sound_drv_sdl_device_current.end_emulation);
}


/*===========================================================================*/
/* Emulation Startup                                                         */
/*===========================================================================*/

BOOLE soundDrvSDL_Startup(sound_device *devinfo) 
{
  return soundDrvSDL_ModeInformationInitialize(&sound_drv_sdl_device_current);
}


/*===========================================================================*/
/* Emulation Shutdown                                                        */
/*===========================================================================*/

void soundDrvSDL_Shutdown(void) 
{
  soundDrvSDL_ModeInformationRelease(&sound_drv_sdl_device_current);
}
