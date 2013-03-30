/* $Id$ */
/*=========================================================================*/
/* Fellow Amiga Emulator                                                   */
/* Sound driver for Windows                                                */
/* Author: Petter Schau (peschau@online.no)                                */
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

/** @file
 *  Sound driver for Windows
 */

#include <windowsx.h>
#include <dsound.h>

#include "defs.h"
#include "fellow.h"
#include "sound.h"
#include "listtree.h"
#include "windrv.h"
#include "sounddrv.h"
#include "config.h"


/*===========================================================================*/
/*===========================================================================*/
/* Sound specific data                                                       */
/*===========================================================================*/
/*===========================================================================*/

/*===========================================================================*/
/* Some structs where we put all our sound specific stuff                    */
/*===========================================================================*/

typedef struct 
{
  ULO rate;
  bool bits16;
  bool stereo;
  ULO buffer_sample_count;
  ULO buffer_block_align;
} sound_drv_dsound_mode;


typedef struct 
{
  LPDIRECTSOUND lpDS;
  LPDIRECTSOUNDBUFFER lpDSB;  // Primary buffer
  LPDIRECTSOUNDBUFFER lpDSBS; // Secondary buffer
  LPDIRECTSOUNDNOTIFY lpDSN;  // Notificaton object
  felist *modes;
  sound_drv_dsound_mode* mode_current;
  HANDLE notifications[3];
  HANDLE data_available;
  HANDLE can_add_data;
  HANDLE mutex;
  UWO *pending_data_left;
  UWO *pending_data_right;
  ULO pending_data_sample_count;
  HANDLE thread;
  DWORD thread_id;
  bool notification_supported;
  ULO mmtimer;
  ULO mmresolution;
  DWORD lastreadpos;
} sound_drv_dsound_device;


/*==========================================================================*/
/* Currently active dsound device                                           */
/*==========================================================================*/

sound_drv_dsound_device sound_drv_dsound_device_current;


/*==========================================================================*/
/* Returns textual error message. Adapted from DX SDK                       */
/*==========================================================================*/

STR *soundDrvDSoundErrorString(HRESULT hResult)
{
  switch( hResult )
  {
    case DSERR_ALLOCATED:		return "DSERR_ALLOCATED";
    case DSERR_CONTROLUNAVAIL:		return "DSERR_CONTROLUNAVAIL";
    case DSERR_INVALIDPARAM:		return "DSERR_INVALIDPARAM";
    case DSERR_INVALIDCALL:		return "DSERR_INVALIDCALL";
    case DSERR_GENERIC:			return "DSERR_GENERIC";
    case DSERR_PRIOLEVELNEEDED:		return "DSERR_PRIOLEVELNEEDED";
    case DSERR_OUTOFMEMORY:		return "DSERR_OUTOFMEMORY";
    case DSERR_BADFORMAT:		return "DSERR_BADFORMAT";
    case DSERR_UNSUPPORTED:		return "DSERR_UNSUPPORTED";
    case DSERR_NODRIVER:		return "DSERR_NODRIVER";
    case DSERR_ALREADYINITIALIZED:	return "DSERR_ALREADYINITIALIZED";
    case DSERR_NOAGGREGATION:		return "DSERR_NOAGGREGATION";
    case DSERR_BUFFERLOST:		return "DSERR_BUFFERLOST";
    case DSERR_OTHERAPPHASPRIO:		return "DSERR_OTHERAPPHASPRIO";
    case DSERR_UNINITIALIZED:		return "DSERR_UNINITIALIZED";
    default:				return "Unknown DirectSound Error";
  }
  return "Unknown DirectSound Error";
}

/* Forward declaration */
void soundDrvPollBufferPosition(void);

/*==========================================================================*/
/* Multimedia Callback fnc. Used when DirectSound notification unsupported  */
/*==========================================================================*/

void CALLBACK timercb(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
  soundDrvPollBufferPosition();
}


/*==========================================================================*/
/* Logs a sensible error message                                            */
/*==========================================================================*/

void soundDrvDSoundFailure(STR *header, HRESULT err)
{
  fellowAddLog(header);
  fellowAddLog(soundDrvDSoundErrorString(err));
  fellowAddLog("\n");
}


/*===========================================================================*/
/* Release DirectSound                                                       */
/*===========================================================================*/

void soundDrvDSoundRelease(void)
{
  if (sound_drv_dsound_device_current.lpDS != NULL)
  {
    IDirectSound_Release(sound_drv_dsound_device_current.lpDS);
  }
  sound_drv_dsound_device_current.lpDS = NULL;
  for (ULO i = 0; i < 3; i++)
  {
    if (sound_drv_dsound_device_current.notifications[i] != NULL)
    {
      CloseHandle(sound_drv_dsound_device_current.notifications[i]);
    }
  }
  if (sound_drv_dsound_device_current.data_available != NULL)
  {
    CloseHandle(sound_drv_dsound_device_current.data_available);
  }
  if (sound_drv_dsound_device_current.can_add_data != NULL)
  {
    CloseHandle(sound_drv_dsound_device_current.can_add_data);
  }
}

	
/*===========================================================================*/
/* Initialize DirectSound                                                    */
/*===========================================================================*/

bool soundDrvDSoundInitialize(void)
{
  ULO i;

  sound_drv_dsound_device_current.modes = NULL;
  sound_drv_dsound_device_current.lpDS = NULL;
  sound_drv_dsound_device_current.lpDSB = NULL;
  sound_drv_dsound_device_current.lpDSBS = NULL;
  sound_drv_dsound_device_current.lpDSN = NULL;
  for (i = 0; i < 3; i++)
  {
    sound_drv_dsound_device_current.notifications[i] = NULL;
  }
  sound_drv_dsound_device_current.data_available = NULL;
  sound_drv_dsound_device_current.can_add_data = NULL;
  sound_drv_dsound_device_current.thread = NULL;
  HRESULT directSoundCreateResult = DirectSoundCreate(NULL, &sound_drv_dsound_device_current.lpDS, NULL);
  if (directSoundCreateResult != DS_OK)
  {
    soundDrvDSoundFailure("soundDrvDSoundInitialize: DirectSoundCreate - ", directSoundCreateResult);
    return false;
  }
  for (i = 0; i < 3; i++)
  {
    sound_drv_dsound_device_current.notifications[i] = CreateEvent(0, 0, 0, 0);
  }
  sound_drv_dsound_device_current.data_available = CreateEvent(0, 0, 0, 0);
  sound_drv_dsound_device_current.can_add_data = CreateEvent(0, 0, 0, 0);
  return true;
}


/*===========================================================================*/
/* Adds a mode to the sound_drv_dsound_device struct                         */
/*===========================================================================*/

void soundDrvAddMode(sound_drv_dsound_device *dsound_device,
		     bool stereo,
		     bool bits16,
		     ULO rate)
{
  sound_drv_dsound_mode *dsound_mode = (sound_drv_dsound_mode *) malloc(sizeof(sound_drv_dsound_mode));
  dsound_mode->stereo = stereo;
  dsound_mode->bits16 = bits16;
  dsound_mode->rate = rate;
  dsound_mode->buffer_sample_count = 0;
  dsound_device->modes = listAddLast(dsound_device->modes, listNew(dsound_mode));
}


/*===========================================================================*/
/* Finds a mode in the sound_drv_dsound_device struct                        */
/*===========================================================================*/

sound_drv_dsound_mode *soundDrvFindMode(sound_drv_dsound_device *dsound_device,
					bool stereo,
					bool bits16,
					ULO rate)
{
  for (felist *fl = dsound_device->modes; fl != NULL; fl = listNext(fl))
  {
    sound_drv_dsound_mode *mode = (sound_drv_dsound_mode *) listNode(fl);
    if (mode->rate == rate && mode->stereo == stereo && mode->bits16 == bits16)
    {
      return mode;
    }
  }
  return NULL;
}


/*===========================================================================*/
/* If DirectSound mode information                                           */
/*===========================================================================*/

void soundDrvDSoundModeInformationRelease(sound_drv_dsound_device *dsound_device)
{
  listFreeAll(dsound_device->modes, TRUE);
  dsound_device->modes = NULL;
}


/*===========================================================================*/
/* Initialize DirectSound mode information                                   */
/*===========================================================================*/

void soundDrvYesNoLog(STR *intro, bool pred)
{
  fellowAddLog(intro);
  fellowAddLog((pred) ? " - Yes\n" : " - No\n");
}

bool soundDrvDSoundModeInformationInitialize(sound_drv_dsound_device *dsound_device)
{
  DSCAPS dscaps;
  bool stereo, mono, bits8, bits16;
  bool secondary_stereo, secondary_mono, secondary_bits8, secondary_bits16;
  bool continuous_rate, emulated_driver, certified_driver;
  ULO minrate, maxrate;
  char s[80];

  memset(&dscaps, 0, sizeof(dscaps));
  dscaps.dwSize = sizeof(dscaps);
  HRESULT getCapsResult = IDirectSound_GetCaps(dsound_device->lpDS, &dscaps);
  if (getCapsResult != DS_OK)
  {
    soundDrvDSoundFailure("soundDrvDSoundModeInformationInitialize: ", getCapsResult);
    return false;
  }
  stereo = !!(dscaps.dwFlags & DSCAPS_PRIMARYSTEREO);
  soundDrvYesNoLog("DSCAPS_PRIMARYSTEREO", stereo);
  mono = !!(dscaps.dwFlags & DSCAPS_PRIMARYMONO);
  soundDrvYesNoLog("DSCAPS_PRIMARYMONO", mono);
  bits16 = !!(dscaps.dwFlags & DSCAPS_PRIMARY16BIT);
  soundDrvYesNoLog("DSCAPS_PRIMARY16BIT", bits16);
  bits8 = !!(dscaps.dwFlags & DSCAPS_PRIMARY8BIT);
  soundDrvYesNoLog("DSCAPS_PRIMARY8BIT", bits8);

  secondary_stereo = !!(dscaps.dwFlags & DSCAPS_SECONDARYSTEREO);
  soundDrvYesNoLog("DSCAPS_SECONDARYSTEREO", secondary_stereo);
  secondary_mono = !!(dscaps.dwFlags & DSCAPS_SECONDARYMONO);
  soundDrvYesNoLog("DSCAPS_SECONDARYMONO", secondary_mono);
  secondary_bits16 = !!(dscaps.dwFlags & DSCAPS_SECONDARY16BIT);
  soundDrvYesNoLog("DSCAPS_SECONDARY16BIT", secondary_bits16);
  secondary_bits8 = !!(dscaps.dwFlags & DSCAPS_SECONDARY8BIT);
  soundDrvYesNoLog("DSCAPS_SECONDARY8BIT", secondary_bits8);

  continuous_rate = !!(dscaps.dwFlags & DSCAPS_CONTINUOUSRATE);
  soundDrvYesNoLog("DSCAPS_CONTINUOUSRATE", continuous_rate);
  emulated_driver = !!(dscaps.dwFlags & DSCAPS_EMULDRIVER);
  soundDrvYesNoLog("DSCAPS_EMULDRIVER", emulated_driver);
  certified_driver = !!(dscaps.dwFlags & DSCAPS_CERTIFIED);
  soundDrvYesNoLog("DSCAPS_CERTIFIED", certified_driver);

  minrate = dscaps.dwMinSecondarySampleRate;
  maxrate = dscaps.dwMaxSecondarySampleRate;
  sprintf(s, "ddscaps.dwMinSecondarySampleRate - %u\n", minrate);
  fellowAddLog(s);
  sprintf(s, "ddscaps.dwMaxSecondarySampleRate - %u\n", maxrate);
  fellowAddLog(s);

  /* Set maximum sample rate to 44100 if it cannot be determined
     from the driver caps; this is supposed to fix problems with
	 ESS SOLO-1 PCI soundcards */

  if (maxrate == 0)
  {
    maxrate = 44100;
    sprintf(s, "ddscaps.dwMaxSecondarySampleRate correction applied - %u\n", maxrate);
    fellowAddLog(s);
  }

  if (stereo)
  {
    if (bits16)
    {
      soundDrvAddMode(dsound_device, stereo, bits16, 15650);
      soundDrvAddMode(dsound_device, stereo, bits16, 22050);
      soundDrvAddMode(dsound_device, stereo, bits16, 31300);
      soundDrvAddMode(dsound_device, stereo, bits16, 44100);
    }
    if (bits8)
    {
      soundDrvAddMode(dsound_device, stereo, !bits8, 15650);
      soundDrvAddMode(dsound_device, stereo, !bits8, 22050);
      soundDrvAddMode(dsound_device, stereo, !bits8, 31300);
      soundDrvAddMode(dsound_device, stereo, !bits8, 44100);
    }
  }
  if (mono)
  {
    if (bits16)
    {
      soundDrvAddMode(dsound_device, !mono, bits16, 15650);
      soundDrvAddMode(dsound_device, !mono, bits16, 22050);
      soundDrvAddMode(dsound_device, !mono, bits16, 31300);
      soundDrvAddMode(dsound_device, !mono, bits16, 44100);
    }
    if (bits8)
    {
      soundDrvAddMode(dsound_device, !mono, !bits8, 15650);
      soundDrvAddMode(dsound_device, !mono, !bits8, 22050);
      soundDrvAddMode(dsound_device, !mono, !bits8, 31300);
      soundDrvAddMode(dsound_device, !mono, !bits8, 44100);
    }
  }

  return true;
}

/** Configure volume of secondary DirectSound buffer of current sound device.
 * 
 *  Loudness is perceived in a logarithmic manner; the calculation attempts
 *  to utilize the upper half of the available spectrum quadratically, so
 *  that the perceived volume when moving the slider along feels more natural
 *  the numbers may still need some fine-tuning
 *  @param[in] volume the target volume in the range of 0 to 100 (100 being 
               full volume)
 *  @return TRUE is successful, FALSE otherwise.
 */
static bool soundDrvDSoundSetVolume(sound_drv_dsound_device *dsound_device, const int volume) {
  HRESULT hResult;
  LONG vol;

	if (volume <= 100 && volume > 0)
    vol = (LONG) -((50-(volume/2))*(50-(volume/2)));
  else if (volume == 0)
    vol = DSBVOLUME_MIN;
  else
    vol = DSBVOLUME_MAX;

#ifdef _DEBUG
  fellowAddLog("soundDrvDSoundSetVolume: volume %d scaled to %d, setting volume...\n",
    volume, vol);
#endif

	hResult = IDirectSoundBuffer_SetVolume (dsound_device->lpDSBS, vol);
	if (FAILED (hResult))
		soundDrvDSoundFailure("soundDrvDSoundSetVolume(): SetVolume() failed: ", hResult);

  return (hResult == DS_OK);
}

bool soundDrvDSoundSetCurrentSoundDeviceVolume(const int volume) {
  return soundDrvDSoundSetVolume(&sound_drv_dsound_device_current, volume);
}

/*===========================================================================*/
/* Set sound cooperative level                                               */
/* Ref: Microsoft Press, Inside DirectX, page 210                            */
/*===========================================================================*/

bool soundDrvDSoundSetCooperativeLevel(sound_drv_dsound_device *dsound_device)
{
  /* We need the HWND of the amiga emulation window, which means sound is    */
  /* initialized after the gfx stuff */

  HRESULT setCooperativeLevelResult = IDirectSound_SetCooperativeLevel(dsound_device->lpDS,
								       gfx_drv_hwnd,
								       DSSCL_PRIORITY);
  if (setCooperativeLevelResult != DS_OK)
  {
    soundDrvDSoundFailure("soundDrvDSoundSetCooperativeLevel", setCooperativeLevelResult);
  }
  return (setCooperativeLevelResult == DS_OK);
}


/*===========================================================================*/
/* Release the primary buffer                                                */
/*===========================================================================*/

void soundDrvDSoundPrimaryBufferRelease(sound_drv_dsound_device *dsound_device)
{
  if (dsound_device->lpDSB != NULL)
  {
    IDirectSoundBuffer_Play(dsound_device->lpDSB, 0, 0, 0);
    IDirectSoundBuffer_Release(dsound_device->lpDSB);
    dsound_device->lpDSB = NULL;
  }
}


/*===========================================================================*/
/* Get a primary buffer                                                      */
/* We always make a primary buffer, in the same format as the sound data     */
/* we are going to receive from the sound emulation routines.                */
/* If it fails, we don't play sound.                                         */
/* Ref: Microsoft Press, Inside DirectX, page 212                            */
/*===========================================================================*/

bool soundDrvDSoundPrimaryBufferInitialize(sound_drv_dsound_device *dsound_device)
{
  DSBUFFERDESC dsbdesc;
  WAVEFORMATEX wfm;

  /* Create sound buffer */
  
  memset(&dsbdesc, 0, sizeof(dsbdesc));
  dsbdesc.dwSize = sizeof(dsbdesc);
  dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
  dsbdesc.dwBufferBytes = 0;
  dsbdesc.lpwfxFormat = NULL;

  memset(&wfm, 0, sizeof(wfm));
  wfm.wFormatTag = WAVE_FORMAT_PCM;
  wfm.nChannels = (dsound_device->mode_current->stereo) ? 2 : 1;
  wfm.nSamplesPerSec = dsound_device->mode_current->rate;
  wfm.wBitsPerSample = (dsound_device->mode_current->bits16) ? 16 : 8;
  wfm.nBlockAlign = (wfm.wBitsPerSample / 8) * wfm.nChannels;
  wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign;
  dsound_device->mode_current->buffer_block_align = wfm.nBlockAlign;

  HRESULT createSoundBufferResult = IDirectSound_CreateSoundBuffer(dsound_device->lpDS,
								   &dsbdesc,
								   &dsound_device->lpDSB,
								   NULL);
  if (createSoundBufferResult != DS_OK)
  {
    soundDrvDSoundFailure("soundDrvDSoundPrimaryBufferInitialize(): CreateSoundBuffer(), ", createSoundBufferResult);
    return false;
  }

  /* Set format of primary buffer */

  HRESULT setFormatResult = IDirectSoundBuffer_SetFormat(dsound_device->lpDSB, &wfm);
  if (setFormatResult != DS_OK)
  {
    soundDrvDSoundFailure("soundDrvDSoundPrimaryBufferInitialize(): SetFormat(), ", setFormatResult);
    soundDrvDSoundPrimaryBufferRelease(dsound_device);
    return false;
  }

  return true;
}


/*===========================================================================*/
/* Release the secondary buffer                                              */
/*===========================================================================*/

void soundDrvDSoundSecondaryBufferRelease(sound_drv_dsound_device *dsound_device)
{
  if (dsound_device->lpDSBS != NULL)
  {
    IDirectSoundBuffer_Play(dsound_device->lpDSBS, 0, 0, 0);
    IDirectSoundBuffer_Release(dsound_device->lpDSBS);
    dsound_device->lpDSBS = NULL;
  }
  if (dsound_device->lpDSN != NULL)
  {
    IDirectSoundNotify_Release(dsound_device->lpDSN);
    dsound_device->lpDSN = NULL;
  }
  if (!dsound_device->notification_supported)
  {
    timeKillEvent(dsound_device->mmtimer); 
    timeEndPeriod(dsound_device->mmresolution);
  }
}


bool soundDrvCreateSecondaryBuffer(sound_drv_dsound_device *dsound_device)
{
  WAVEFORMATEX wfm;
  DSBUFFERDESC dsbdesc;

  memset(&wfm, 0, sizeof(wfm));
  wfm.wFormatTag = WAVE_FORMAT_PCM;
  wfm.nChannels = (dsound_device->mode_current->stereo) ? 2 : 1;
  wfm.nSamplesPerSec = dsound_device->mode_current->rate;
  wfm.wBitsPerSample = (dsound_device->mode_current->bits16) ? 16 : 8;
  wfm.nBlockAlign = (wfm.wBitsPerSample / 8) * wfm.nChannels;
  wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign;

  memset(&dsbdesc, 0, sizeof(dsbdesc));
  dsbdesc.dwSize = sizeof(dsbdesc);
  dsbdesc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY |
		    DSBCAPS_GETCURRENTPOSITION2 |
		    DSBCAPS_GLOBALFOCUS |
        DSBCAPS_CTRLVOLUME;
  dsbdesc.dwBufferBytes = dsound_device->mode_current->buffer_sample_count*wfm.nBlockAlign*2;
  dsbdesc.lpwfxFormat = &wfm;

  HRESULT createSoundBufferResult = IDirectSound_CreateSoundBuffer(dsound_device->lpDS,
								   &dsbdesc,
								   &dsound_device->lpDSBS,
								   NULL);
  if (createSoundBufferResult != DS_OK)
  {
    soundDrvDSoundFailure("soundDrvCreateSecondaryBuffer: CreateSoundBuffer(), ", createSoundBufferResult);
    return false;
  }
  return true;
}

bool soundDrvClearSecondaryBuffer(sound_drv_dsound_device *dsound_device)
{
  char* lpAudio;
  DWORD dwBytes;

  HRESULT lock1Result = IDirectSoundBuffer_Lock(dsound_device->lpDSBS,
						0,
						0, // Ignored because we pass DSBLOCK_ENTIREBUFFER
						(LPVOID*) &lpAudio,
						&dwBytes,
						NULL,
						NULL,
						DSBLOCK_ENTIREBUFFER);

  if (lock1Result != DS_OK)
  {
    soundDrvDSoundFailure("soundDrvClearSecondaryBuffer: Lock(), ", lock1Result);

    if (lock1Result == DSERR_BUFFERLOST)
    {
      /* Buffer lost. Try to restore buffer once. Multiple restores need more intelligence */
      HRESULT restoreResult = IDirectSoundBuffer_Restore(dsound_device->lpDSBS);
      if (restoreResult != DS_OK)
      {
	soundDrvDSoundFailure("soundDrvClearSecondaryBuffer: Restore(), ", restoreResult);
	return false;
      }

      HRESULT lock2Result = IDirectSoundBuffer_Lock(dsound_device->lpDSBS,
						    0,
						    0, // Ignored because we pass DSBLOCK_ENTIREBUFFER
						    (LPVOID*)&lpAudio,
						    &dwBytes,
						    NULL,
						    NULL,
						    DSBLOCK_ENTIREBUFFER);
      if (lock2Result != DS_OK)
      {
	// Here we give up
	soundDrvDSoundFailure("soundDrvClearSecondaryBuffer: Lock(), ", lock2Result);
	return false;
      }
    }
  }

  for (DWORD i = 0; i < dwBytes; i++)
  {
    lpAudio[i] = 0;
  }

  HRESULT unlockResult = IDirectSoundBuffer_Unlock(dsound_device->lpDSBS, lpAudio, dwBytes, NULL, 0);
  if (unlockResult != DS_OK)
  {
    // Here we give up
    soundDrvDSoundFailure("soundDrvClearSecondaryBuffer: Unlock(), ", unlockResult);
    return false;
  }

  return true;
}

bool soundDrvInitializeSecondaryBufferNotification(sound_drv_dsound_device *dsound_device)
{
  DSBCAPS dsbcaps;
  DSBPOSITIONNOTIFY rgdscbpn[2];

  memset(&dsbcaps, 0, sizeof(dsbcaps));
  dsbcaps.dwSize = sizeof(dsbcaps);

  HRESULT getCapsResult = IDirectSoundBuffer_GetCaps(dsound_device->lpDSBS, &dsbcaps);
  if (getCapsResult != DS_OK)
  {
    soundDrvDSoundFailure("soundDrvInitializeSecondaryBufferNotification: GetCaps(), ", getCapsResult);
    return false;
  }

  dsound_device->notification_supported = (!!(dsbcaps.dwFlags & DSBCAPS_CTRLPOSITIONNOTIFY)) 
					  && (soundGetNotification() == SOUND_DSOUND_NOTIFICATION);

  if (dsound_device->notification_supported)
  {
    /* Notification supported AND selected */

    /* Get notification interface */
  
    HRESULT queryInterfaceResult = IDirectSoundBuffer_QueryInterface(dsound_device->lpDSBS,
								     IID_IDirectSoundNotify,
								     (LPVOID * FAR) &dsound_device->lpDSN);
    if (queryInterfaceResult != DS_OK)
    {
      soundDrvDSoundFailure("soundDrvInitializeSecondaryBufferNotification(): QueryInterface(IID_IDirectSoundNotify), ", 
			    queryInterfaceResult);
      return false;
    }

    /* Attach notification objects to buffer */
  
    rgdscbpn[0].dwOffset = dsound_device->mode_current->buffer_block_align*
                           (dsound_device->mode_current->buffer_sample_count - 1);
    rgdscbpn[0].hEventNotify = dsound_device->notifications[0];
    rgdscbpn[1].dwOffset = dsound_device->mode_current->buffer_block_align*
                           (dsound_device->mode_current->buffer_sample_count*2 - 1);
    rgdscbpn[1].hEventNotify = dsound_device->notifications[1];

    HRESULT setNotificationPositionsResult = IDirectSoundNotify_SetNotificationPositions(dsound_device->lpDSN, 2, rgdscbpn);
    if (setNotificationPositionsResult != DS_OK)
    {
      soundDrvDSoundFailure("soundDrvInitializeSecondaryBufferNotification(): SetNotificationPositions(), ", setNotificationPositionsResult);
      return false;
    }
  }
  else
  {
    /* Notification not supported, fallback to timer */

    char s[80];
    TIMECAPS timecaps;

    MMRESULT timeGetDevCapsResult = timeGetDevCaps(&timecaps, sizeof(TIMECAPS));
    if (timeGetDevCapsResult != TIMERR_NOERROR)
    {
      fellowAddLog("soundDrvInitializeSecondaryBufferNotification(): timeGetDevCaps() failed\n");
      return false;
    }

    sprintf(s, "timeGetDevCaps: min: %u, max %u\n", timecaps.wPeriodMin, timecaps.wPeriodMax);
    fellowAddLog(s);

    dsound_device->mmresolution = timecaps.wPeriodMin;

    MMRESULT timeBeginPeriodResult = timeBeginPeriod(dsound_device->mmresolution);
    if (timeBeginPeriodResult != TIMERR_NOERROR)
    {
      fellowAddLog("soundDrvInitializeSecondaryBufferNotification(): timeBeginPeriod() failed\n");
      return false;
    }

    MMRESULT timeSetEventResult = timeSetEvent(1, 0, timercb, (DWORD_PTR)0, (UINT) TIME_PERIODIC);
    if (timeSetEventResult == 0)
    {
      fellowAddLog("soundDrvInitializeSecondaryBufferNotification(): timeSetEvent() failed\n");
      return false;
    }
    dsound_device->mmtimer = timeSetEventResult;
  }
  return true;
}

/*===========================================================================*/
/* Get a secondary buffer                                                    */
/* Ref: Microsoft Press, Inside DirectX, page 212, 229-232                   */
/*===========================================================================*/

bool soundDrvDSoundSecondaryBufferInitialize(sound_drv_dsound_device *dsound_device)
{
  if (!soundDrvCreateSecondaryBuffer(dsound_device))
  {
    return false;
  }

  if (!soundDrvClearSecondaryBuffer(dsound_device))
  {
    soundDrvDSoundSecondaryBufferRelease(dsound_device);
    return false;
  }

  if (!soundDrvInitializeSecondaryBufferNotification(dsound_device))
  {
    soundDrvDSoundSecondaryBufferRelease(dsound_device);
    return false;
  }

  soundDrvDSoundSetVolume(dsound_device, soundGetVolume());

  return true;
}


/*===========================================================================*/
/* Stop playback                                                             */
/*===========================================================================*/

void soundDrvDSoundPlaybackStop(sound_drv_dsound_device *dsound_device)
{
  soundDrvDSoundSecondaryBufferRelease(dsound_device);
  soundDrvDSoundPrimaryBufferRelease(dsound_device);
}


/*===========================================================================*/
/* Initialize playback                                                      */
/*===========================================================================*/

bool soundDrvDSoundPlaybackInitialize(sound_drv_dsound_device *dsound_device)
{
  dsound_device->lastreadpos = 0;
  bool result = soundDrvDSoundPrimaryBufferInitialize(dsound_device);
  if (result)
  {
    result = soundDrvDSoundSecondaryBufferInitialize(dsound_device);
  }
  if (!result)
  {
    fellowAddLog("Sound, secondary failed\n");
    soundDrvDSoundPrimaryBufferRelease(dsound_device);
  }
  if (result)
  {
    HRESULT primaryPlayResult = IDirectSoundBuffer_Play(dsound_device->lpDSB, 0, 0, DSBPLAY_LOOPING);
    if (primaryPlayResult != DS_OK)
    {
      soundDrvDSoundFailure("soundDrvDSoundPlaybackInitialize: Primary->Play(), ", primaryPlayResult);
    }
    HRESULT secondaryPlayResult = IDirectSoundBuffer_Play(dsound_device->lpDSBS, 0, 0, DSBPLAY_LOOPING);
    if (secondaryPlayResult != DS_OK)
    {
      soundDrvDSoundFailure("soundDrvDSoundPlaybackInitialize: Secondary->Play(), ", secondaryPlayResult);
    }
  }
  return result;
}


/*===========================================================================*/
/* Copy data to a buffer                                                     */
/*===========================================================================*/

void soundDrvCopy16BitsStereo(UWO *audio_buffer,
			      UWO *left, 
			      UWO *right, 
			      ULO sample_count)
{
  for (ULO i = 0; i < sample_count; i++)
  {
    *audio_buffer++ = *left++;
    *audio_buffer++ = *right++;
  }
}

void soundDrvCopy16BitsMono(UWO *audio_buffer,
			    UWO *left, 
			    UWO *right, 
			    ULO sample_count)
{
  for (ULO i = 0; i < sample_count; i++)
  {
    *audio_buffer++ = (*left++ + *right++);
  }
}

void soundDrvCopy8BitsStereo(UBY *audio_buffer,
			     UWO *left, 
			     UWO *right, 
			     ULO sample_count)
{
  for (ULO i = 0; i < sample_count; i++)
  {
    *audio_buffer++ = ((*left++)>>8) + 128;
    *audio_buffer++ = ((*right++)>>8) + 128;
  }
}

void soundDrvCopy8BitsMono(UBY *audio_buffer,
			   UWO *left, 
			   UWO *right, 
			   ULO sample_count)
{
  for (ULO i = 0; i < sample_count; i++)
  {
    *audio_buffer++ = (((*left++) + (*right++))>>8) + 128;
  }
}

bool soundDrvDSoundCopyToBuffer(sound_drv_dsound_device *dsound_device,
				UWO *left,
				UWO *right,
				ULO sample_count,
				ULO buffer_half)
{
  LPVOID lpvAudio;
  DWORD dwBytes;
  DWORD size = dsound_device->mode_current->buffer_sample_count*
	       dsound_device->mode_current->buffer_block_align; 
  DWORD start_offset = buffer_half*size;

  HRESULT lockResult = IDirectSoundBuffer_Lock(dsound_device->lpDSBS,
					       start_offset,
					       size,
					       &lpvAudio,
					       &dwBytes,
					       NULL,
					       NULL,
					       0);

  if (lockResult != DS_OK)
  {
    soundDrvDSoundFailure("soundDrvDSoundCopyToBuffer: Lock(), ", lockResult);
    return false;
  }

  if (lockResult == DSERR_BUFFERLOST)
  {
    /* Buffer lost. Try to restore buffer once. Multiple restores need more intelligence */
    HRESULT restoreResult = IDirectSoundBuffer_Restore(dsound_device->lpDSBS);
    if (restoreResult != DS_OK)
    {
      soundDrvDSoundFailure("soundDrvDSoundCopyToBuffer: Restore(), ", restoreResult);
      return false;
    }
    HRESULT lock2Result = IDirectSoundBuffer_Lock(dsound_device->lpDSBS,
						  start_offset,
						  size,
						  &lpvAudio,
						  &dwBytes,
						  NULL,
						  NULL,
						  0);
    if (lock2Result != DS_OK)
    {
      soundDrvDSoundFailure("soundDrvDSoundCopyToBuffer: Lock() after Restore , ", lock2Result);
      return false;
    }
  }

  if (soundGetStereo())
  {
    if (soundGet16Bits())
    {
      soundDrvCopy16BitsStereo((UWO*)lpvAudio, left, right, sample_count);
    }
    else
    {
      soundDrvCopy8BitsStereo((UBY*)lpvAudio, left, right, sample_count);
    }
  }
  else
  {
    if (soundGet16Bits())
    {
      soundDrvCopy16BitsMono((UWO*)lpvAudio, left, right, sample_count);
    }
    else
    {
      soundDrvCopy8BitsMono((UBY*)lpvAudio, left, right, sample_count);
    }
  }
  HRESULT unlockResult = IDirectSoundBuffer_Unlock(dsound_device->lpDSBS, lpvAudio, dwBytes, NULL, 0);
  if (unlockResult != DS_OK)
  {
    soundDrvDSoundFailure("soundDrvDSoundCopyToBuffer: Unlock(), ", unlockResult);
    return false;
  }
  return true;
}


/*===========================================================================*/
/* Play a buffer                                                             */
/* This is also where the emulator receives synchronization,                 */
/* since waiting for the sound-buffer to be                                  */
/* ready slows the emulator down to its original 50hz PAL speed.             */
/*===========================================================================*/

void soundDrvPlay(WOR *left, WOR *right, ULO sample_count)
{
  sound_drv_dsound_device *dsound_device = &sound_drv_dsound_device_current;
  WaitForSingleObject(dsound_device->can_add_data, INFINITE);
  dsound_device->pending_data_left = (UWO*)left;
  dsound_device->pending_data_right = (UWO*)right;
  dsound_device->pending_data_sample_count = sample_count;
  ResetEvent(dsound_device->can_add_data);
  SetEvent(dsound_device->data_available);
}


/*===========================================================================*/
/* We need a mutex to control stopping and starting the driver during a      */
/* display mode change.                                                      */
/*===========================================================================*/

void soundDrvAcquireMutex(sound_drv_dsound_device *dsound_device)
{
  WaitForSingleObject(dsound_device->mutex, INFINITE);
}

void soundDrvReleaseMutex(sound_drv_dsound_device *dsound_device)
{
  ReleaseMutex(dsound_device->mutex);
}


/*===========================================================================*/
/* Used by playback thread to wait for data                                  */
/* There is one caveat here, that the app is about to end emulation, in that */
/* case the data will never come, so we need to keep an eye on the end event */
/* as well. In case of termination, we return FALSE                          */
/* We use this one only when notification is supported                       */
/*===========================================================================*/

bool soundDrvWaitForData(sound_drv_dsound_device *dsound_device,
			  ULO next_buffer_no,
			  bool &need_to_restart_playback)
{
  HANDLE multi_events[3];
  DWORD evt;
  bool terminate_wait = false;
  bool terminate_thread = false;
  ULO wait_for_x_events = 3;

  // No-wait test for data_available, return TRUE if data is available
  if (WaitForSingleObject(dsound_device->data_available, 0) == WAIT_OBJECT_0)
  {
    return true;
  }
  //fellowAddLog("Data was not available\n");
  // Here, data is not available from the sound emulation
  // And we have played past the end of the buffer we want to copy data to
  // We can continue to play until the end of the next buffer, but then we
  // must stop playback, since there is no more data to play
  // So we set up a multiple object wait which trigger on the following
  // conditions:
  // 0 - Data becomes available, return TRUE, playback is not stopped
  // 1 - End of the thread is signaled, return FALSE to indicate termination
  // 2 - The end of the next buffer is reached, stop playback, has no data,yet
  //     Redo the wait, this time without the event for end of the next buffer

  // What if we are lagging behind already?

  while (!terminate_wait)
  {
    multi_events[0] = dsound_device->data_available;
    multi_events[1] = dsound_device->notifications[2];
    if (wait_for_x_events == 3)
    {
      multi_events[2] = dsound_device->notifications[next_buffer_no];
    }
    evt = WaitForMultipleObjects(wait_for_x_events,
			         (void*const*) multi_events,
			         FALSE,
			         INFINITE);
    switch (evt)
    {
      case WAIT_OBJECT_0:
        // Data is now available
	// Restart playing if we have stopped it
        need_to_restart_playback = (wait_for_x_events == 2);
	terminate_wait = true;
	break;
      case WAIT_OBJECT_0 + 1:
	// End of thread requested, return FALSE
	terminate_wait = true;
	terminate_thread = true;
	break;
      case WAIT_OBJECT_0 + 2:
	// End of next buffer reached, stop playback, wait more
        HRESULT playResult = IDirectSoundBuffer_Play(dsound_device->lpDSBS, 0, 0, 0);
	if (playResult != DS_OK)
	{
	  soundDrvDSoundFailure("soundDrvWaitForData: Play(), ", playResult);
	}
	wait_for_x_events = 2;
	break;
    }
  }
  return !terminate_thread;
}


/*===========================================================================*/
/* Emulates notification when notification is not supported.                 */
/* I am not sure if this is a stupid thing to do, or if there really are more*/
/* efficient ways to control a "polled" buffer.                              */
/* Somebody give that guy at Microsoft who likes polling so much a beating...*/  
/* Called from the message loop in response to a timer.                      */
/*===========================================================================*/

void soundDrvPollBufferPosition(void)
{
  sound_drv_dsound_device *dsound_device = &sound_drv_dsound_device_current;
  soundDrvAcquireMutex(dsound_device);
  if ((soundGetEmulation() == SOUND_PLAY) && !dsound_device->notification_supported)
  {
    DWORD readpos, writepos;
    DWORD halfway = (dsound_device->mode_current->buffer_sample_count*
  		     dsound_device->mode_current->buffer_block_align);

    if (dsound_device->lpDSBS != NULL)
    {
      HRESULT getCurrentPositionResult = IDirectSoundBuffer_GetCurrentPosition(dsound_device->lpDSBS,
									       &readpos,
									       &writepos);
      if (getCurrentPositionResult != DS_OK)
      {
	soundDrvDSoundFailure("soundDrvPollBufferPosition: GetCurrentPosition(), ", getCurrentPositionResult);
      }
      if ((readpos >= halfway) && (dsound_device->lastreadpos < halfway))
      {
        SetEvent(dsound_device->notifications[0]);
      }
      else if ((readpos < halfway) && (dsound_device->lastreadpos >= halfway))
      {
        SetEvent(dsound_device->notifications[1]);
      }
      dsound_device->lastreadpos = readpos;
    }
  }
  soundDrvReleaseMutex(dsound_device);
}

/*===========================================================================*/
/* Process end of buffer event                                               */
/*===========================================================================*/

bool soundDrvProcessEndOfBuffer(sound_drv_dsound_device *dsound_device,
				ULO current_buffer_no,
				ULO next_buffer_no)
{
  bool terminate_thread = false;
  bool need_to_restart_playback = false;
  if (soundDrvWaitForData(dsound_device, next_buffer_no, need_to_restart_playback))
  {
    soundDrvDSoundCopyToBuffer(dsound_device,
 			       dsound_device->pending_data_left,
			       dsound_device->pending_data_right,
			       dsound_device->pending_data_sample_count,
			       current_buffer_no);
    if (need_to_restart_playback)
    {
      HRESULT playResult = IDirectSoundBuffer_Play(dsound_device->lpDSBS, 0, 0, DSBPLAY_LOOPING);

      if (playResult != DS_OK)
      {
	soundDrvDSoundFailure("soundDrvProcessEndOfBuffer: Play(), ", playResult);
      }

      if (playResult == DSERR_BUFFERLOST)
      {
	/* Temporary fix for restoring buffer once. Multiple restore needs alot more */
        HRESULT restoreResult = IDirectSoundBuffer_Restore(dsound_device->lpDSBS);
	if (restoreResult != DS_OK)
	{
	  soundDrvDSoundFailure("soundDrvProcessEndOfBuffer: Restore(), ", restoreResult);
	}
	else
	{
	  HRESULT play2Result = IDirectSoundBuffer_Play(dsound_device->lpDSBS, 0, 0, DSBPLAY_LOOPING);
	  if (play2Result != DS_OK)
	  {
	    soundDrvDSoundFailure("soundDrvProcessEndOfBuffer: Play() after restore, ", play2Result);
	  }
	}
      }
    }

    // If the next buffer also triggered during soundDrvWaitForData()
    // it will end multiple object wait immediately, since we don't reset it
    ResetEvent(dsound_device->data_available);
    SetEvent(dsound_device->can_add_data);
    ResetEvent(dsound_device->notifications[current_buffer_no]);
  }
  else
  {
    terminate_thread = true;
  }
  return terminate_thread;
}

/*===========================================================================*/
/* Thread, from which we control playback much in the same way as under DOS  */
/* Basically, we have two notification objects attached to each end of the   */
/* playback buffer, as well as a stop playback event, signaled in the        */
/* soundDrvEmulationStop function.                                           */
/* When awake, we have two options:                                          */
/* If the event sound_drv_data_available is signaled, data is available,     */
/* and we copy it into the playback buffer, reset the event and sleeps again.*/
/* If data is not available, we must pause playback and wait for that event  */
/* to be signaled. Then we copy data and start playback and goes to sleep.   */
/* The data available event is set by the main emulation thread when data is */
/* ready, in soundDrvPlay()                                                  */        
/*===========================================================================*/

DWORD WINAPI soundDrvThreadProc(void *in)
{
  bool terminate_thread = false;
  sound_drv_dsound_device *dsound_device = (sound_drv_dsound_device *) in;

  winDrvSetThreadName(-1, "soundDrvThreadProc()");

  while (!terminate_thread) {
    // Thread is activated by 3 different events:
    // 0 - Play position is at end of buffer 0
    // 1 - Play position is at end of buffer 1
    // 2 - Thread must terminate (triggered in soundDrvEmulationStop())
    DWORD dwEvt = WaitForMultipleObjects(3,
					 (void*const*) (dsound_device->notifications),
					 FALSE,
					 INFINITE);
    
    switch (dwEvt)
    {
      case WAIT_OBJECT_0 + 0: /* End of first buffer */
	// Wait for data_available event to become signaled
	// or FALSE is returned if (2) becomes signaled (end thread)
	terminate_thread = soundDrvProcessEndOfBuffer(dsound_device, 0, 1);
	break;
      case WAIT_OBJECT_0 + 1: /* End of first buffer */
	// Wait for data_available event to become signaled
	// or FALSE is returned if (2) becomes signaled (end thread)
	terminate_thread = soundDrvProcessEndOfBuffer(dsound_device, 1, 0);
	break;
      case WAIT_OBJECT_0 + 2: /* Emulation is ending */
      default:
	terminate_thread = true;
	break;
    }
  }
  return 0;
}


/*===========================================================================*/
/* Hard Reset                                                                */
/*===========================================================================*/

void soundDrvHardReset(void)
{
}


/*===========================================================================*/
/* Emulation Starting                                                        */
/*===========================================================================*/

bool soundDrvEmulationStart(ULO rate,
			    bool bits16,
			    bool stereo,
			    ULO *sample_count_max)
{
  sound_drv_dsound_device *dsound_device = &sound_drv_dsound_device_current;

  soundDrvAcquireMutex(dsound_device);

  /* Set all events to their initial state */
  
  for (ULO i = 0; i < 3; i++)
  {
    ResetEvent(dsound_device->notifications[i]);
  }
  ResetEvent(dsound_device->data_available);
  SetEvent(dsound_device->can_add_data);

  /* Check if the driver can support the requested sound quality */
  
  dsound_device->mode_current = soundDrvFindMode(dsound_device,
						 stereo,
						 bits16,
						 rate);
  bool result = dsound_device->mode_current != NULL;

  /* Record the number of samples in our buffer (ie. one half of the size) */

  if (result)
  {
    dsound_device->mode_current->buffer_sample_count = *sample_count_max;
    result = soundDrvDSoundSetCooperativeLevel(dsound_device);
  }

  /* Create the needed buffer(s) */
  
  if (result)
  {
    result = soundDrvDSoundPlaybackInitialize(dsound_device);
  }

  /* Start playback thread */

  if (result)
  {
    dsound_device->thread = CreateThread(NULL,                      // Security attr
			                 0,                         // Stack Size
				         soundDrvThreadProc,        // Thread procedure
			                 dsound_device,             // Thread parameter
			                 0,                         // Creation flags
			                 &dsound_device->thread_id);// ThreadId
    result = (dsound_device->thread != NULL);
  }

  /* In case of failure, we undo any stuff we've done so far */
  
  if (!result)
  {
    fellowAddLog("Failed to start sound\n");
    soundDrvDSoundPlaybackStop(dsound_device);
  }

  soundDrvReleaseMutex(dsound_device);
  return result;
}


/*===========================================================================*/
/* Emulation Stopping                                                        */
/*===========================================================================*/

void soundDrvEmulationStop(void)
{
  soundDrvAcquireMutex(&sound_drv_dsound_device_current);
  SetEvent(sound_drv_dsound_device_current.notifications[2]);
  WaitForSingleObject(sound_drv_dsound_device_current.thread, INFINITE);
  CloseHandle(sound_drv_dsound_device_current.thread);
  sound_drv_dsound_device_current.thread = NULL;
  soundDrvDSoundPlaybackStop(&sound_drv_dsound_device_current);
  soundDrvReleaseMutex(&sound_drv_dsound_device_current);
}


/*===========================================================================*/
/* Emulation Startup                                                         */
/*===========================================================================*/

bool soundDrvStartup(sound_device *devinfo)
{
  bool result = soundDrvDSoundInitialize();       /* Create a direct sound object */
  if (result)
  {
    result = soundDrvDSoundModeInformationInitialize(&sound_drv_dsound_device_current);
  }
  if (!result)
  {
    soundDrvDSoundRelease();
  }
  else
  {
    sound_drv_dsound_device_current.mutex = CreateMutex(NULL, 0, NULL);
  }

  return result;
}


/*===========================================================================*/
/* Emulation Shutdown                                                        */
/*===========================================================================*/

void soundDrvShutdown(void)
{
  soundDrvDSoundModeInformationRelease(&sound_drv_dsound_device_current);
  soundDrvDSoundRelease();
  if (sound_drv_dsound_device_current.mutex != NULL)
  {
    CloseHandle(sound_drv_dsound_device_current.mutex);
  }
}
