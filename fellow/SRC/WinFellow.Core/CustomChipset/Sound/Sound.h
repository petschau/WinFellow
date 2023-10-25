#pragma once

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

#include <cstdint>
#include "CustomChipset/Sound/SoundConfiguration.h"
#include "WavFileWriter.h"

constexpr uint32_t MAX_BUFFER_SAMPLES = 65536;

class Sound
{
private:
  WavFileWriter _wavFileWriter;

  CustomChipset::sound_emulations _emulation;
  CustomChipset::sound_rates _rate;
  bool _isStereo;
  bool _is16Bits;
  bool _wavCapture;
  CustomChipset::sound_filters _filter;
  CustomChipset::sound_notifications _notification;
  int _volume;
  bool _deviceFound;

  /*===========================================================================*/
  /* Buffer data                                                               */
  /*===========================================================================*/

  uint32_t _currentBuffer;
  int16_t _left[2][MAX_BUFFER_SAMPLES], _right[2][MAX_BUFFER_SAMPLES]; /* Samplebuffer, 16-b.signed */
  uint32_t _bufferLength;                                              /* Current buffer length in ms */
  uint32_t _bufferSampleCount;                                         /* Current number of samples in the buffer */
  uint32_t _bufferSampleCountMax;                                      /* Maximum capacity of the buffer */

  /*===========================================================================*/
  /* Run-time data                                                             */
  /*===========================================================================*/

  uint32_t audiocounter;  /* Used in 22050/44100 to decide samples */
  uint32_t audioodd;      /* Used for skipping samples in 22050 */
  uint32_t _framecounter; /* Count frames, and then play */
  uint32_t _scale;

  double _filterValue45 = 0.857270436755215389; // 7000 Hz at 45454 Hz samplingrate
  double _filterValue33 = 0.809385175167476725; // 7000 Hz at 33100 Hz samplingrate
  double _filterValue22 = 0.727523105310746957; // 7000 Hz at 22005 Hz samplingrate
  double _filterValue15 = 0.639362082983339100; // 7000 Hz at 15650 Hz samplingrate

  double _amplitudeDiv45 = 7.035;
  double _amplitudeDiv33 = 5.25;
  double _amplitudeDiv22 = 3.67;
  double _amplitudeDiv15 = 2.773;
  double _lastRight = 0.0000000000;
  double _lastLeft = 0.0000000000;

  /*===========================================================================*/
  /* Audio-registers                                                           */
  /*===========================================================================*/

  uint32_t _audpt[4];  /* Sample-DMA pointer */
  uint32_t _audlen[4]; /* Length */
  uint32_t _audper[4]; /* Used directly, NOTE: translated value */
  uint32_t _audvol[4]; /* Volume, possibly not reloaded by state-machine */
  uint32_t _auddat[4]; /* Last data word set by DMA or CPU */
  bool _auddatSet[4];  /* Set TRUE whenever auddat is written */

  /*===========================================================================*/
  /* Internal variables used by state-machine                                  */
  /*===========================================================================*/

  uint32_t _audlenw[4];       /* Length counter */
  uint32_t _audpercounter[4]; /* Period counter */
  uint32_t _auddatw[4];       /* Sample currently output, 16-bit signed */
  uint32_t _audstate[4];      /* Current state for the channel */
  uint32_t _audvolw[4];       /* Current volume, reloaded at some points */
  uint32_t _audptw[4];        /* Current dma-pointer, reloaded at some points */

  /*===========================================================================*/
  /* Translation tables                                                        */
  /*===========================================================================*/

  uint32_t _periodTable[65536];
  int16_t _volumes[256][64];
  uint32_t _audioIrqMask[4] = {0x0080, 0x0100, 0x0200, 0x0400};
  uint32_t _audioDmaconMask[4] = {0x1, 0x2, 0x4, 0x8};

  uint32_t GetChannelNumber(uint32_t address);

  void ExecuteState(uint32_t channel);
  void State0(uint32_t channel);
  void State1(uint32_t channel);
  void State2(uint32_t channel);
  void State3(uint32_t channel);
  void State4(uint32_t channel);
  void State5(uint32_t channel);
  void State6(uint32_t channel);

  void LowPass(uint32_t count, int16_t *bufferLeft, int16_t *bufferRight);
  uint32_t ChannelUpdate(uint32_t channel, int16_t *bufferLeft, int16_t *bufferRight, uint32_t count, bool halfscale, bool odd);
  void FrequencyHandler();

  CustomChipset::sound_rates GetRate();
  uint32_t GetRateReal();
  uint32_t GetBufferLength();
  int GetVolume();

  void SetBufferSampleCount(uint32_t sampleCount);
  uint32_t GetBufferSampleCount();
  void SetBufferSampleCountMax(uint32_t sampleCountMax);
  uint32_t GetBufferSampleCountMax();
  void SetDeviceFound(bool deviceFound);
  bool GetDeviceFound();
  void SetScale(uint32_t scale);
  uint32_t GetScale();

  void SetSampleVolume(uint8_t sampleIn, uint8_t volume, int16_t sampleOut);
  int16_t GetSampleVolume(int8_t sampleIn, uint8_t volume);
  void SetPeriodValue(uint32_t period, uint32_t value);
  uint32_t GetPeriodValue(uint32_t period);

  void VolumeTableInitialize(bool isStereo);
  void PeriodTableInitialize(uint32_t outputRate);
  void PlaybackInitialize();

  void IOHandlersInstall();
  void IORegistersClear();
  void CopyBufferOverrunToCurrentBuffer(uint32_t availableSamples, uint32_t previousBuffer);

public:
  void SetEmulation(CustomChipset::sound_emulations emulation);
  CustomChipset::sound_emulations GetEmulation();
  void SetIsStereo(bool isStereo);
  bool GetIsStereo();
  void SetIs16Bits(bool is16Bits);
  bool GetIs16Bits();
  void SetFilter(CustomChipset::sound_filters filter);
  CustomChipset::sound_filters GetFilter();
  void SetWAVDump(bool wavCapture);
  bool GetWAVDump();
  void SetNotification(CustomChipset::sound_notifications notification);
  CustomChipset::sound_notifications GetNotification();
  void SetRate(CustomChipset::sound_rates rate);
  void SetBufferLength(uint32_t ms);

  void ChannelKill(uint32_t channel);
  void ChannelEnable(uint32_t channel);

  void SetAudXpth(uint16_t data, uint32_t address);
  void SetAudXptl(uint16_t data, uint32_t address);
  void SetAudXlen(uint16_t data, uint32_t address);
  void SetAudXper(uint16_t data, uint32_t address);
  void SetAudXvol(uint16_t data, uint32_t address);
  void SetAudXdat(uint16_t data, uint32_t address);

  void SetVolume(int volume);

  void EndOfLine();
  void HardReset();
  void EmulationStart();
  void EmulationStop();
  void Startup();
  void Shutdown();

  Sound();
  ~Sound();
};
