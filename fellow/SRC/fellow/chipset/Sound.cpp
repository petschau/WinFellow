/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* Sound emulation                                                         */
/*                                                                         */
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

#include "fellow/api/defs.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/memory/Memory.h"
#include "SOUND.H"
#include "WAV.H"
#include "CIA.H"
#include "fellow/chipset/Graphics.h"
#include "fellow/application/SoundDriver.h"
#include "fellow/application/Interrupt.h"

#define MAX_BUFFER_SAMPLES 65536

/*===========================================================================*/
/* Sound emulation configuration                                             */
/*===========================================================================*/

sound_rates sound_rate; /* Current output rate */
bool sound_stereo;      /* Current mono/stereo setting */
bool sound_16bits;      /* Current 8/16 bit setting */
sound_emulations sound_emulation;
sound_filters sound_filter;
sound_notifications sound_notification;
BOOLE sound_wav_capture;
BOOLE sound_device_found;
ULO sound_volume;

/*===========================================================================*/
/* Buffer data                                                               */
/*===========================================================================*/

ULO sound_current_buffer;
WOR sound_left[2][MAX_BUFFER_SAMPLES], sound_right[2][MAX_BUFFER_SAMPLES]; /* Samplebuffer, 16-b.signed */
ULO sound_buffer_length;                                                   /* Current buffer length in ms */
ULO sound_buffer_sample_count;                                             /* Current number of samples in the buffer */
ULO sound_buffer_sample_count_max;                                         /* Maximum capacity of the buffer */

/*===========================================================================*/
/* Information about the sound device                                        */
/*===========================================================================*/

sound_device sound_dev;

/*===========================================================================*/
/* Run-time data                                                             */
/*===========================================================================*/

ULO audiocounter;       /* Used in 22050/44100 to decide samples */
ULO audioodd;           /* Used for skipping samples in 22050 */
ULO sound_framecounter; /* Count frames, and then play */
ULO sound_scale;

double filter_value45 = 0.857270436755215389; // 7000 Hz at 45454 Hz samplingrate
double filter_value33 = 0.809385175167476725; // 7000 Hz at 33100 Hz samplingrate
double filter_value22 = 0.727523105310746957; // 7000 Hz at 22005 Hz samplingrate
double filter_value15 = 0.639362082983339100; // 7000 Hz at 15650 Hz samplingrate
/*
double amplitude_div45 = 3.5000000000;
double amplitude_div33 = 2.8000000000;
double amplitude_div22 = 1.9000000000;
double amplitude_div15 = 1.4000000000;
*/

double amplitude_div45 = 7.035;
double amplitude_div33 = 5.25;
double amplitude_div22 = 3.67;
double amplitude_div15 = 2.773;
double last_right = 0.0000000000;
double last_left = 0.0000000000;

/*===========================================================================*/
/* Audio-registers                                                           */
/*===========================================================================*/

ULO audpt[4];        /* Sample-DMA pointer */
ULO audlen[4];       /* Length */
ULO audper[4];       /* Used directly, NOTE: translated value */
ULO audvol[4];       /* Volume, possibly not reloaded by state-machine */
ULO auddat[4];       /* Last data word set by DMA or CPU */
BOOLE auddat_set[4]; /* Set TRUE whenever auddat is written */

/*===========================================================================*/
/* Internal variables used by state-machine                                  */
/*===========================================================================*/

ULO audlenw[4];             /* Length counter */
ULO audpercounter[4];       /* Period counter */
ULO auddatw[4];             /* Sample currently output, 16-bit signed */
soundStateFunc audstate[4]; /* Current state for the channel */
ULO audvolw[4];             /* Current volume, reloaded at some points */
ULO audptw[4];              /* Current dma-pointer, reloaded at some points */

/*===========================================================================*/
/* Translation tables                                                        */
/*===========================================================================*/

ULO periodtable[65536];
WOR volumes[256][64];
ULO audioirqmask[4] = {0x0080, 0x0100, 0x0200, 0x0400};
ULO audiodmaconmask[4] = {0x1, 0x2, 0x4, 0x8};

/*==============================================================================
Audio IO Registers
==============================================================================*/

/* Extract channel number from the register address */

ULO soundGetChannelNumber(ULO address)
{
  return ((address & 0x70) >> 4) - 2;
}

/*
==================
AUDXPT
==================
$dff0a0,b0,c0,d0
*/

void waudXpth(UWO data, ULO address)
{
  ULO ch = soundGetChannelNumber(address);
  audpt[ch] = chipsetReplaceHighPtr(audpt[ch], data);
}

void waudXptl(UWO data, ULO address)
{
  ULO ch = soundGetChannelNumber(address);
  audpt[ch] = chipsetReplaceLowPtr(audpt[ch], data);
}

/*
==================
AUDXLEN
==================
$dff0a4,b4,c4,d4
*/

void waudXlen(UWO data, ULO address)
{
  ULO ch = soundGetChannelNumber(address);
  audlen[ch] = data;
}

/*
==================
AUDXPER
==================
$dff0a6,b6,c6,d6
*/

void waudXper(UWO data, ULO address)
{
  ULO ch = soundGetChannelNumber(address);
  audper[ch] = periodtable[data];
}

/*
==================
AUDXVOL
==================
$dff0a8,b8,c8,d8

*/

void waudXvol(UWO data, ULO address)
{
  ULO ch = soundGetChannelNumber(address);
  /*Replay routines sometimes access volume as a byte register at $dff0X9...*/
  if (((data & 0xff) == 0) && ((data & 0xff00) != 0)) data = (data >> 8) & 0xff;
  if ((data & 64) == 64) data = 63;
  audvol[ch] = data & 0x3f;
}

/*
==================
AUDXDAT
==================
$dff0aa,ba,ca,da

Not used right now.
*/

void waudXdat(UWO data, ULO address)
{
  ULO ch = soundGetChannelNumber(address);
  auddat[ch] = data & 0xff;
  auddat_set[ch] = TRUE;
}

/*==============================================================================
Audio state machine
==============================================================================*/

void soundState0(ULO ch);
void soundState1(ULO ch);
void soundState2(ULO ch);
void soundState3(ULO ch);
void soundState4(ULO ch);
void soundState5(ULO ch);
void soundState6(ULO ch);

/*==============================================================================
State 0
==============================================================================*/

void soundState0(ULO ch)
{
  /*-------------------
  Statechange 0 to 1
  -------------------*/

  audlenw[ch] = audlen[ch];
  audptw[ch] = audpt[ch];
  audpercounter[ch] = 0;
  audstate[ch] = soundState1;
}

/*==============================================================================
State 1
==============================================================================*/

void soundState1(ULO ch)
{
  /*-------------------
  Statechange 1 to 5
  -------------------*/

  if (audlenw[ch] != 1) audlenw[ch]--;
  audstate[ch] = soundState5;
  wintreq_direct((UWO)(audioirqmask[ch] | 0x8000), 0xdff09c, true);
}

/*==============================================================================
State 2
==============================================================================*/

void soundState2(ULO ch)
{
  if (audpercounter[ch] >= 0x10000)
  {

    /*-------------------
    Statechange 2 to 3
    -------------------*/

    audpercounter[ch] -= 0x10000;
    audpercounter[ch] += audper[ch];
    audvolw[ch] = audvol[ch];
    audstate[ch] = soundState3;
    auddatw[ch] = volumes[(auddat[ch] & 0xff00) >> 8][audvolw[ch]];
  }
  else
  {
    /*-------------------
    Statechange 2 to 2
    -------------------*/

    audpercounter[ch] += audper[ch];
  }
}

/*==============================================================================
State 3
==============================================================================*/

void soundState3(ULO ch)
{
  if (audpercounter[ch] >= 0x10000)
  {
    /*-------------------
    Statechange 3 to 2
    -------------------*/

    audpercounter[ch] -= 0x10000;
    audpercounter[ch] += audper[ch];
    audvolw[ch] = audvol[ch];
    audstate[ch] = soundState2;
    auddatw[ch] = volumes[auddat[ch] & 0xff][audvolw[ch]];
    auddat[ch] = chipmemReadWord(audptw[ch]);
    audptw[ch] = chipsetMaskPtr(audptw[ch] + 2);
    if (audlenw[ch] != 1)
      audlenw[ch]--;
    else
    {
      audlenw[ch] = audlen[ch];
      audptw[ch] = audpt[ch];
      wintreq_direct((UWO)(audioirqmask[ch] | 0x8000), 0xdff09c, true);
    }
  }
  else
  {
    /*-------------------
    Statechange 3 to 3
    -------------------*/

    audpercounter[ch] += audper[ch];
  }
}

/*==============================================================================
State 4 and State 6, no operation
==============================================================================*/

void soundState4(ULO ch)
{
}

void soundState6(ULO ch)
{
}

/*==============================================================================
State 5
==============================================================================*/

void soundState5(ULO ch)
{
  /*-------------------
  Statechange 5 to 2
  -------------------*/

  audvolw[ch] = audvol[ch];
  audpercounter[ch] = 0;
  auddat[ch] = chipmemReadWord(audptw[ch]);
  audptw[ch] = chipsetMaskPtr(audptw[ch] + 2);
  audstate[ch] = soundState2;
  if (audlenw[ch] != 1)
    audlenw[ch]--;
  else
  {
    audlenw[ch] = audlen[ch];
    audptw[ch] = audpt[ch];
    wintreq_direct((UWO)(audioirqmask[ch] | 0x8000), 0xdff09c, true);
  }
}

/*==============================================================================
This is called by DMACON when DMA is turned off
==============================================================================*/

void soundChannelKill(ULO ch)
{
  auddatw[ch] = 0;
  audstate[ch] = soundState0;
}

/*==============================================================================
This is called by DMACON when DMA is turned on
==============================================================================*/

void soundChannelEnable(ULO ch)
{
  soundState0(ch);
}

/*==============================================================================
Generate sound
Called in every end of line
Will run the audio state machine the needed number of times
and move the generated samples to a temporary buffer
==============================================================================*/

/*==============================================================================
; Audio_Lowpass filters the _sound_right and sound_left memory with a
; pass1 lowpass filter at 7000 Hz
; coded by Rainer Sinsch (sinsch@informatik.uni-frankfurt.de)
;==============================================================================*/

void soundLowPass(ULO count, WOR *buffer_left, WOR *buffer_right)
{
  double amplitude_div;
  double filter_value;
  switch (soundGetRate())
  {
    case sound_rates::SOUND_44100:
      amplitude_div = amplitude_div45;
      filter_value = filter_value45;
      break;
    case sound_rates::SOUND_31300:
      amplitude_div = amplitude_div33;
      filter_value = filter_value33;
      break;
    case sound_rates::SOUND_22050:
      amplitude_div = amplitude_div22;
      filter_value = filter_value22;
      break;
    case sound_rates::SOUND_15650:
      amplitude_div = amplitude_div15;
      filter_value = filter_value15;
      break;
  }

  for (ULO i = 0; i < count; ++i)
  {
    last_left = filter_value * last_left + (double)buffer_left[i];
    buffer_left[i] = (WOR)(last_left / amplitude_div);
    last_right = filter_value * last_right + (double)buffer_right[i];
    buffer_right[i] = (WOR)(last_right / amplitude_div);
  }
}

ULO soundChannelUpdate(ULO ch, WOR *buffer_left, WOR *buffer_right, ULO count, BOOLE halfscale, BOOLE odd)
{
  ULO samples_added = 0;

  if (dmacon & audiodmaconmask[ch])
  {
    for (ULO i = 0; i < count; ++i)
    {
      audstate[ch](ch);
      if ((!halfscale) || (halfscale && !odd))
      {
        if (ch == 0 || ch == 3)
        {
          buffer_left[samples_added++] += (WOR)auddatw[ch];
        }
        else
        {
          buffer_right[samples_added++] += (WOR)auddatw[ch];
        }
      }
      odd = !odd;
    }
  }
  else
  {
    if (!interruptIsRequested(audioirqmask[ch]) && auddat_set[ch])
    {
      auddat_set[ch] = FALSE;
      wintreq_direct((UWO)(audioirqmask[ch] | 0x8000), 0xdff09c, true);
    }
    if (!halfscale)
    {
      samples_added = count;
    }
    else
    {
      for (ULO i = 0; i < count; ++i)
      {
        if (!odd)
        {
          samples_added++;
        }
        odd = !odd;
      }
    }
  }
  return samples_added;
}

void soundFrequencyHandler()
{
  WOR *buffer_left = (WOR *)sound_left + sound_buffer_sample_count;
  WOR *buffer_right = (WOR *)sound_right + sound_buffer_sample_count;
  ULO count = 0;
  ULO samples_added;
  BOOLE halfscale = (soundGetRate() == sound_rates::SOUND_22050 || soundGetRate() == sound_rates::SOUND_15650);
  if (soundGetRate() == sound_rates::SOUND_44100 || soundGetRate() == sound_rates::SOUND_22050)
  {
    while (audiocounter <= 0x40000)
    {
      count++;
      audiocounter += sound_scale;
    }
  }
  else
    count = 2;
  audiocounter -= 0x40000;
  for (ULO i = 0; i < count; ++i)
    buffer_left[i] = buffer_right[i] = 0;
  for (ULO i = 0; i < 4; ++i)
    samples_added = soundChannelUpdate(i, buffer_left, buffer_right, count, halfscale, audioodd);
  if (halfscale && count & 1) audioodd = !audioodd;

  if (sound_filter != sound_filters::SOUND_FILTER_NEVER)
  {
    if (sound_filter == sound_filters::SOUND_FILTER_ALWAYS || ciaIsSoundFilterEnabled()) soundLowPass(samples_added, buffer_left, buffer_right);
  }
  sound_buffer_sample_count += samples_added;
}

/*===========================================================================*/
/* Property settings                                                         */
/*===========================================================================*/

void soundSetRate(sound_rates rate)
{
  sound_rate = rate;
}

sound_rates soundGetRate()
{
  return sound_rate;
}

ULO soundGetRateReal()
{
  switch (soundGetRate())
  {
    case sound_rates::SOUND_44100: return 44100;
    case sound_rates::SOUND_31300: return 31300;
    case sound_rates::SOUND_22050: return 22050;
    case sound_rates::SOUND_15650: return 15650;
  }
  return 0;
}

void soundSetStereo(bool stereo)
{
  sound_stereo = stereo;
}

bool soundGetStereo()
{
  return sound_stereo;
}

void soundSet16Bits(bool bits16)
{
  sound_16bits = bits16;
}

bool soundGet16Bits()
{
  return sound_16bits;
}

void soundSetEmulation(sound_emulations emulation)
{
  sound_emulation = emulation;
}

sound_emulations soundGetEmulation()
{
  return sound_emulation;
}

void soundSetFilter(sound_filters filter)
{
  sound_filter = filter;
}

sound_filters soundGetFilter()
{
  return sound_filter;
}

void soundSetNotification(sound_notifications notification)
{
  sound_notification = notification;
}

sound_notifications soundGetNotification()
{
  return sound_notification;
}

void soundSetBufferLength(ULO ms)
{
  sound_buffer_length = ms;
}

ULO soundGetBufferLength()
{
  return sound_buffer_length;
}

ULO soundGetVolume()
{
  return sound_volume;
}

void soundSetVolume(const ULO volume)
{
  sound_volume = volume;
}

void soundSetWAVDump(BOOLE wav_capture)
{
  sound_wav_capture = wav_capture;
}

BOOLE soundGetWAVDump()
{
  return sound_wav_capture;
}

void soundSetBufferSampleCount(ULO sample_count)
{
  sound_buffer_sample_count = sample_count;
}

ULO soundGetBufferSampleCount()
{
  return sound_buffer_sample_count;
}

void soundSetBufferSampleCountMax(ULO sample_count_max)
{
  sound_buffer_sample_count_max = sample_count_max;
}

ULO soundGetBufferSampleCountMax()
{
  return sound_buffer_sample_count_max;
}

void soundSetDeviceFound(BOOLE device_found)
{
  sound_device_found = device_found;
}

BOOLE soundGetDeviceFound()
{
  return sound_device_found;
}

void soundSetScale(ULO scale)
{
  sound_scale = scale;
}

ULO soundGetScale()
{
  return sound_scale;
}

void soundSetSampleVolume(UBY sample_in, UBY volume, WOR sample_out)
{
  volumes[sample_in][volume] = sample_out;
}

WOR soundGetSampleVolume(BYT sample_in, UBY volume)
{
  return volumes[sample_in][volume];
}

void soundSetPeriodValue(ULO period, ULO value)
{
  periodtable[period] = value;
}

ULO soundGetPeriodValue(ULO period)
{
  return periodtable[period];
}

/*===========================================================================*/
/* Initializes the volume table                                              */
/*===========================================================================*/

void soundVolumeTableInitialize(BOOLE stereo)
{
  LON s;

  if (!stereo)
    s = 1; /* Mono */
  else
    s = 2; /* Stereo */

  for (LON i = -128; i < 128; i++)
    for (LON j = 0; j < 64; j++)
      if (j == 0)
        soundSetSampleVolume((UBY)(i & 0xff), (UBY)j, (WOR)0);
      else
        soundSetSampleVolume((UBY)(i & 0xff), (UBY)j, (WOR)((i * j * s)));
}

/*===========================================================================*/
/* Initializes the period table                                              */
/*===========================================================================*/

void soundPeriodTableInitialize(ULO outputrate)
{
  if (outputrate < 29000) outputrate *= 2; /* Internally, can not run slower than max Amiga rate */
  soundSetScale((ULO)(((double)(65536.0 * 2.0 * 31200.0)) / ((double)outputrate)));

  soundSetPeriodValue(0, 0x10000);
  for (LON i = 1; i < 65536; i++)
  {
    double j = 3546895 / i; /* Sample rate */
    LON periodvalue = (ULO)((j * 65536) / outputrate);
    if (periodvalue > 0x10000) periodvalue = 0x10000;
    soundSetPeriodValue(i, periodvalue);
  }
}

/*===========================================================================*/
/* Sets up sound emulation for a specific quality                            */
/*===========================================================================*/

void soundPlaybackInitialize()
{
  audiocounter = 0;
  if (soundGetEmulation() > sound_emulations::SOUND_NONE)
  { /* Play sound */
    soundPeriodTableInitialize(soundGetRateReal());
    soundVolumeTableInitialize(soundGetStereo());
    soundSetBufferSampleCount(0);
    sound_current_buffer = 0;
    soundSetBufferSampleCountMax(static_cast<ULO>(static_cast<float>(soundGetRateReal()) / (1000.0f / static_cast<float>(soundGetBufferLength()))));
  }
}

/*===========================================================================*/
/* Clear a device struct                                                     */
/*===========================================================================*/

void soundDeviceClear(sound_device *sd)
{
  memset(sd, 0, sizeof(sound_device));
}

/*===========================================================================*/
/* Set IO register stubs                                                     */
/*===========================================================================*/

void soundIOHandlersInstall()
{
  for (int i = 0; i < 4; i++)
  {
    memorySetIoWriteStub(0xa0 + 16 * i, waudXpth);
    memorySetIoWriteStub(0xa2 + 16 * i, waudXptl);
    memorySetIoWriteStub(0xa4 + 16 * i, waudXlen);
    memorySetIoWriteStub(0xa6 + 16 * i, waudXper);
    memorySetIoWriteStub(0xa8 + 16 * i, waudXvol);
    memorySetIoWriteStub(0xaa + 16 * i, waudXdat);
  }
}

/*===========================================================================*/
/* Clear all sound emulation data                                            */
/*===========================================================================*/

void soundIORegistersClear()
{
  audstate[0] = soundState0;
  audstate[1] = soundState0;
  audstate[2] = soundState0;
  audstate[3] = soundState0;

  for (int i = 0; i < 4; i++)
  {
    audpt[i] = 0;
    audptw[i] = 0;
    audlen[i] = 2;
    audper[i] = 0;
    audvol[i] = 0;
    audpercounter[i] = 0;
    auddat[i] = 0;
    auddat_set[i] = FALSE;
    auddatw[i] = 0;
    audlenw[i] = 2;
    audvolw[i] = 0;
  }
  sound_current_buffer = 0;
}

void soundCopyBufferOverrunToCurrentBuffer(ULO available_samples, ULO previous_buffer)
{
  ULO pos = 0;
  for (ULO i = soundGetBufferSampleCountMax(); i < available_samples; i++)
  {
    sound_left[sound_current_buffer][pos] = sound_left[previous_buffer][i];
    sound_right[sound_current_buffer][pos] = sound_right[previous_buffer][i];
    pos++;
  }
  soundSetBufferSampleCount(pos + MAX_BUFFER_SAMPLES * sound_current_buffer);
}

/*===========================================================================*/
/* Called on end of line                                                     */
/*===========================================================================*/

void soundEndOfLine()
{
  if (soundGetEmulation() != sound_emulations::SOUND_NONE)
  {
    soundFrequencyHandler();
    ULO available_samples = soundGetBufferSampleCount() - sound_current_buffer * MAX_BUFFER_SAMPLES;
    if (available_samples >= soundGetBufferSampleCountMax())
    {
      if (soundGetEmulation() == sound_emulations::SOUND_PLAY)
      {
        soundDrvPlay(sound_left[sound_current_buffer], sound_right[sound_current_buffer], soundGetBufferSampleCountMax());
      }
      if (soundGetWAVDump())
      {
        wavPlay(sound_left[sound_current_buffer], sound_right[sound_current_buffer], soundGetBufferSampleCountMax());
      }
      int previous_buffer = sound_current_buffer;
      sound_current_buffer++;
      if (sound_current_buffer > 1)
      {
        sound_current_buffer = 0;
      }
      soundSetBufferSampleCount(0 + MAX_BUFFER_SAMPLES * sound_current_buffer);

      if (available_samples > soundGetBufferSampleCountMax())
      {
        soundCopyBufferOverrunToCurrentBuffer(available_samples, previous_buffer);
      }
    }
  }
}

/*===========================================================================*/
/* Called on emulation start and stop                                        */
/*===========================================================================*/

void soundEmulationStart()
{
  soundIOHandlersInstall();
  audioodd = 0;
  soundPlaybackInitialize();
  if (soundGetEmulation() != sound_emulations::SOUND_NONE && soundGetEmulation() != sound_emulations::SOUND_EMULATE)
  {
    /* Allow sound driver to override buffer length */
    ULO buffer_length = soundGetBufferSampleCountMax();
    if (!soundDrvEmulationStart(soundGetRateReal(), soundGet16Bits(), soundGetStereo(), &buffer_length))
    {
      soundSetEmulation(sound_emulations::SOUND_EMULATE); /* Driver failed, slient emulation */
    }
    if (buffer_length != soundGetBufferSampleCountMax())
    {
      soundSetBufferSampleCountMax(buffer_length);
    }
  }
  if (soundGetWAVDump() && (soundGetEmulation() != sound_emulations::SOUND_NONE))
  {
    wavEmulationStart(soundGetRate(), soundGet16Bits(), soundGetStereo(), soundGetBufferSampleCountMax());
  }
}

void soundEmulationStop()
{
  if (soundGetEmulation() != sound_emulations::SOUND_NONE && soundGetEmulation() != sound_emulations::SOUND_EMULATE) soundDrvEmulationStop();
  if (soundGetWAVDump() && (soundGetEmulation() != sound_emulations::SOUND_NONE)) wavEmulationStop();
}

/*===========================================================================*/
/* Called every time we do a hard-reset                                      */
/*===========================================================================*/

void soundHardReset()
{
  soundIORegistersClear();
}

/*===========================================================================*/
/* Called once on emulator startup                                           */
/*===========================================================================*/

BOOLE soundStartup()
{
  soundSetEmulation(sound_emulations::SOUND_NONE);
  soundSetFilter(sound_filters::SOUND_FILTER_ORIGINAL);
  soundSetRate(sound_rates::SOUND_15650);
  soundSetStereo(FALSE);
  soundSet16Bits(FALSE);
  soundSetNotification(sound_notifications::SOUND_MMTIMER_NOTIFICATION);
  soundSetWAVDump(FALSE);
  soundSetBufferLength(40);
  soundIORegistersClear();
  soundDeviceClear(&sound_dev);
  soundSetDeviceFound(soundDrvStartup(&sound_dev));
  wavStartup();
  if (!soundGetDeviceFound())
    if (soundGetEmulation() == sound_emulations::SOUND_PLAY) soundSetEmulation(sound_emulations::SOUND_NONE);
  return soundGetDeviceFound();
}

/*===========================================================================*/
/* Called once on emulator shutdown                                          */
/*===========================================================================*/

void soundShutdown()
{
  soundDrvShutdown();
}
