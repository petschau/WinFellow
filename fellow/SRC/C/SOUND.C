/* @(#) $Id: SOUND.C,v 1.4.2.1 2004-06-08 14:24:58 carfesh Exp $ */
/*=========================================================================*/
/* Fellow Amiga Emulator                                                   */
/*                                                                         */
/* Sound emulation                                                         */
/*                                                                         */
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

#include "portable.h"
#include "renaming.h"

#include "defs.h"
#include "fmem.h"
#include "sound.h"
#include "wav.h"
#include "sounddrv.h"


#define MAX_BUFFER_SAMPLES 65536


/*===========================================================================*/
/* Sound emulation configuration                                             */
/*===========================================================================*/

sound_rates sound_rate;                               /* Current output rate */
BOOLE sound_stereo;                           /* Current mono/stereo setting */
BOOLE sound_16bits;                              /* Current 8/16 bit setting */
sound_emulations sound_emulation;
sound_filters sound_filter;
sound_notifications sound_notification;
BOOLE sound_wav_capture;
BOOLE sound_device_found;


/*===========================================================================*/
/* Buffer data                                                               */
/*===========================================================================*/

ULO sound_current_buffer;
WOR sound_left[2][MAX_BUFFER_SAMPLES],
    sound_right[2][MAX_BUFFER_SAMPLES];         /* Samplebuffer, 16-b.signed */
ULO sound_buffer_length;                      /* Current buffer length in ms */
ULO sound_buffer_sample_count;    /* Current number of samples in the buffer */
ULO sound_buffer_sample_count_max;         /* Maximum capacity of the buffer */


/*===========================================================================*/
/* Information about the sound device                                        */
/*===========================================================================*/

sound_device sound_dev;


/*===========================================================================*/
/* Run-time data                                                             */
/*===========================================================================*/

ULO audiocounter;                   /* Used in 22050/44100 to decide samples */
ULO audioodd;                          /* Used for skipping samples in 22050 */
ULO sound_framecounter;                       /* Count frames, and then play */
ULO sound_scale;


/*===========================================================================*/
/* Audio-registers                                                           */
/*===========================================================================*/

ULO audpt[4];                                          /* Sample-DMA pointer */
ULO audlen[4];                                                     /* Length */
ULO audper[4];                      /* Used directly, NOTE: translated value */ 
ULO audvol[4];             /* Volume, possibly not reloaded by state-machine */
ULO auddat[4];                                 /* Last data word read by DMA */


/*===========================================================================*/
/* Internal variables used by state-machine                                  */
/*===========================================================================*/

ULO audlenw[4];                                            /* Length counter */
ULO audpercounter[4];                                      /* Period counter */
ULO auddatw[4];                    /* Sample currently output, 16-bit signed */
soundStateFunc audstate[4];                 /* Current state for the channel */
ULO audvolw[4];                   /* Current volume, reloaded at some points */
ULO audptw[4];               /* Current dma-pointer, reloaded at some points */ 


/*===========================================================================*/
/* Translation tables                                                        */
/*===========================================================================*/

ULO periodtable[65536];
WOR volumes[256][64];
ULO audioirqmask[4] = {0x0080, 0x0100, 0x0200, 0x0400};
ULO audiodmaconmask[4] = {0x1, 0x2, 0x4, 0x8};


/*===========================================================================*/
/* Property settings                                                         */
/*===========================================================================*/

__inline void soundSetRate(sound_rates rate) {
  sound_rate = rate;
}

__inline sound_rates soundGetRate(void) {
  return sound_rate;
}

__inline ULO soundGetRateReal(void) {
  switch (soundGetRate()) {
    case SOUND_44100:	return 44100;
    case SOUND_31300:	return 31300;
    case SOUND_22050:	return 22050;
    case SOUND_15650:	return 15650;
  }
  return 0;
}

__inline void soundSetStereo(BOOLE stereo) {
  sound_stereo = stereo;
}

__inline BOOLE soundGetStereo(void) {
  return sound_stereo;
}

__inline void soundSet16Bits(BOOLE bits16) {
  sound_16bits = bits16;
}

__inline BOOLE soundGet16Bits(void) {
  return sound_16bits;
}

__inline void soundSetEmulation(sound_emulations emulation) {
  sound_emulation = emulation;
}

sound_emulations soundGetEmulation(void) {
  return sound_emulation;
}

__inline void soundSetFilter(sound_filters filter) {
  sound_filter = filter;
}

__inline sound_filters soundGetFilter(void) {
  return sound_filter;
}

__inline void soundSetNotification(sound_notifications notification) {
  sound_notification = notification;
}

sound_notifications soundGetNotification(void) {
  return sound_notification;
}

__inline void soundSetBufferLength(ULO ms) {
  sound_buffer_length = ms;
}

__inline ULO soundGetBufferLength(void) {
  return sound_buffer_length;
}

__inline void soundSetWAVDump(BOOLE wav_capture) {
  sound_wav_capture = wav_capture;
}

__inline BOOLE soundGetWAVDump(void) {
  return sound_wav_capture;
}

__inline void soundSetBufferSampleCount(ULO sample_count) {
  sound_buffer_sample_count = sample_count;
}

__inline ULO soundGetBufferSampleCount(void) {
  return sound_buffer_sample_count;
}

__inline void soundSetBufferSampleCountMax(ULO sample_count_max) {
  sound_buffer_sample_count_max = sample_count_max;
}

__inline ULO soundGetBufferSampleCountMax(void) {
  return sound_buffer_sample_count_max;
}

__inline void soundSetDeviceFound(BOOLE device_found) {
  sound_device_found = device_found;
}

__inline BOOLE soundGetDeviceFound(void) {
  return sound_device_found;
}

__inline void soundSetScale(ULO scale) {
  sound_scale = scale;
}

__inline ULO soundGetScale(void) {
  return sound_scale;
}

__inline void soundSetSampleVolume(UBY sample_in, UBY volume, WOR sample_out) {
  volumes[sample_in][volume] = sample_out;
}

__inline WOR soundGetSampleVolume(BYT sample_in, UBY volume) {
  return volumes[sample_in][volume];
}

__inline void soundSetPeriodValue(ULO period, ULO value) {
  periodtable[period] = value;
}

__inline ULO soundGetPeriodValue(ULO period) {
  return periodtable[period];
}


/*===========================================================================*/
/* Initializes the volume table                                              */
/*===========================================================================*/

void soundVolumeTableInitialize(BOOLE stereo) {
  LON i, s, j;

  if (!stereo)
    s = 1;                                                           /* Mono */
  else
    s = 2;                                                         /* Stereo */

  for (i = -128; i < 128; i++) 
    for (j = 0; j < 64; j++)
      if (j == 0)
	soundSetSampleVolume((UBY) (i & 0xff), (UBY) j, (WOR)  0);
      else
        soundSetSampleVolume((UBY) (i & 0xff), (UBY) j, (WOR) ((i*j*s)/2));
}


/*===========================================================================*/
/* Initializes the period table                                              */
/*===========================================================================*/

void soundPeriodTableInitialize(ULO outputrate) {
  double j;
  LON i, periodvalue;

  if (outputrate < 29000)
    outputrate *= 2;   /* Internally, can not run slower than max Amiga rate */
  soundSetScale((ULO) (((double)(65536.0*2.0*31300.0))/((double) outputrate)));
  soundSetPeriodValue(0, 0x10000);
  for (i = 1; i < 65536; i++) {
    j = 3568200 / i;                                          /* Sample rate */
    periodvalue = (ULO) ((j*65536) / outputrate);
    if (periodvalue > 0x10000)
      periodvalue = 0x10000;
    soundSetPeriodValue(i, periodvalue);
  }
}


/*===========================================================================*/
/* Sets up sound emulation for a specific quality                            */
/*===========================================================================*/

void soundPlaybackInitialize(void) {
  ULO samplewidth = (soundGet16Bits()) ? 2 : 1;
  audiocounter = 0;
  if (soundGetEmulation() > SOUND_NONE) {                      /* Play sound */
    soundPeriodTableInitialize(soundGetRateReal());
    soundVolumeTableInitialize(soundGetStereo());
    soundSetBufferSampleCount(0);
    sound_current_buffer = 0;
    soundSetBufferSampleCountMax(soundGetRateReal() / (1000 / soundGetBufferLength()));
  }
}


/*===========================================================================*/
/* Clear a device struct                                                     */
/*===========================================================================*/

void soundDeviceClear(sound_device *sd) {
  memset(sd, 0, sizeof(sound_device));
}


/*===========================================================================*/
/* Set IO register stubs                                                     */
/*===========================================================================*/

void soundIOHandlersInstall(void) {
  ULO i;

  for (i = 0; i < 4; i++) {
    memorySetIOWriteStub(0xa0 + 16*i, waudXpth);
    memorySetIOWriteStub(0xa2 + 16*i, waudXptl);
    memorySetIOWriteStub(0xa4 + 16*i, waudXlen);
    memorySetIOWriteStub(0xa6 + 16*i, waudXper);
    memorySetIOWriteStub(0xa8 + 16*i, waudXvol);
  }
}


/*===========================================================================*/
/* Clear all sound emulation data                                            */
/*===========================================================================*/

void soundIORegistersClear(void) {
  ULO i;

  /* MSVC 5 and 6 generates an internal compiler error in release mode if audstate
     is initialized inside the loop....... Convincing.... PS */

  audstate[0] = soundState0;
  audstate[1] = soundState0;
  audstate[2] = soundState0;
  audstate[3] = soundState0;
  
  for (i = 0; i < 4; i++) {
    audpt[i] = 0;
    audptw[i] = 0;
    audlen[i] = 2;
    audper[i] = 0;
    audvol[i] = 0;
    audpercounter[i] = 0;
    auddat[i] = 0;
    auddatw[i] = 0;
    audlenw[i] = 2;
    audvolw[i] = 0;
  }
  sound_current_buffer = 0;
}


/*===========================================================================*/
/* Called on end of line                                                     */
/*===========================================================================*/

void soundEndOfLine(void) {
  if (soundGetEmulation() != SOUND_NONE) {
    switch (soundGetRate()) {
      case SOUND_44100:
	soundFrequencyHandler44100();
	break;
      case SOUND_31300:
	soundFrequencyHandler31300();
	break;
      case SOUND_22050:
	soundFrequencyHandler22050();
	break;
      case SOUND_15650:
	soundFrequencyHandler15650();
      break;
    }
    if ((soundGetBufferSampleCount() - (sound_current_buffer*MAX_BUFFER_SAMPLES)) >=
        soundGetBufferSampleCountMax()) {
      if (soundGetEmulation() == SOUND_PLAY) {
	soundDrvPlay(sound_left[sound_current_buffer],
		     sound_right[sound_current_buffer],
		     soundGetBufferSampleCountMax());
      }
      if (soundGetWAVDump())
	wavPlay(sound_left[sound_current_buffer],
	        sound_right[sound_current_buffer],
		soundGetBufferSampleCountMax());
      sound_current_buffer++;
      if (sound_current_buffer > 1) sound_current_buffer = 0;
      soundSetBufferSampleCount(0 + MAX_BUFFER_SAMPLES*sound_current_buffer);
    }
  }
}


/*===========================================================================*/
/* Called on emulation start and stop                                        */
/*===========================================================================*/

void soundEmulationStart(void) {
  soundIOHandlersInstall();
  audioodd = 0;
  soundPlaybackInitialize();
  if (soundGetEmulation() != SOUND_NONE && soundGetEmulation() != SOUND_EMULATE)
  {
    /* Allow sound driver to override buffer length */
    ULO buffer_length = soundGetBufferSampleCountMax();
    if (!soundDrvEmulationStart(soundGetRateReal(),
			        soundGet16Bits(),
			        soundGetStereo(),
			        &buffer_length))
      soundSetEmulation(SOUND_EMULATE); /* Driver failed, slient emulation */
    if (buffer_length != soundGetBufferSampleCountMax())
      soundSetBufferSampleCountMax(buffer_length);
  }
  if (soundGetWAVDump() && (soundGetEmulation() != SOUND_NONE))
    wavEmulationStart(soundGetRate(),
		      soundGet16Bits(),
		      soundGetStereo(),
		      soundGetBufferSampleCountMax());
}

void soundEmulationStop(void) {
  if (soundGetEmulation() != SOUND_NONE && soundGetEmulation() != SOUND_EMULATE)
    soundDrvEmulationStop();
  if (soundGetWAVDump() && (soundGetEmulation() != SOUND_NONE))
    wavEmulationStop();
}


/*===========================================================================*/
/* Called every time we do a hard-reset                                      */
/*===========================================================================*/

void soundHardReset(void) {
  soundIORegistersClear();
}


/*===========================================================================*/
/* Called once on emulator startup                                           */
/*===========================================================================*/

BOOLE soundStartup(void) {
  soundSetEmulation(SOUND_NONE);
  soundSetFilter(SOUND_FILTER_ORIGINAL);
  soundSetRate(SOUND_15650);
  soundSetStereo(FALSE);
  soundSet16Bits(FALSE);
  soundSetNotification(SOUND_MMTIMER_NOTIFICATION);
  soundSetWAVDump(FALSE);
  soundSetBufferLength(40);
  soundIORegistersClear();
  soundDeviceClear(&sound_dev);
  soundSetDeviceFound(soundDrvStartup(&sound_dev));
  wavStartup();
  if (!soundGetDeviceFound())
    if (soundGetEmulation() == SOUND_PLAY)
      soundSetEmulation(SOUND_NONE);
  return soundGetDeviceFound();
}


/*===========================================================================*/
/* Called once on emulator shutdown                                          */
/*===========================================================================*/

void soundShutdown(void) {
  soundDrvShutdown();
}
