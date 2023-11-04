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

#include "Defs.h"
#include "chipset.h"
#include "MemoryInterface.h"
#include "ComplexInterfaceAdapter.h"
#include "GraphicsPipeline.h"
#include "interrupt.h"
#include "VirtualHost/Core.h"

using namespace CustomChipset;

// ==================
// AUDXPT
// ==================
// $dff0a0,b0,c0,d0

void waudXpth(uint16_t data, uint32_t address)
{
  _core.Sound->SetAudXpth(data, address);
}

void waudXptl(uint16_t data, uint32_t address)
{
  _core.Sound->SetAudXptl(data, address);
}

uint32_t Sound::GetChannelNumber(uint32_t address)
{
  return ((address & 0x70) >> 4) - 2;
}

void Sound::SetAudXpth(uint16_t data, uint32_t address)
{
  uint32_t channel = GetChannelNumber(address);
  _audpt[channel] = chipsetReplaceHighPtr(_audpt[channel], data);
}

void Sound::SetAudXptl(uint16_t data, uint32_t address)
{
  uint32_t channel = GetChannelNumber(address);
  _audpt[channel] = chipsetReplaceLowPtr(_audpt[channel], data);
}

// ==================
// AUDXLEN
// ==================
// $dff0a4,b4,c4,d4

void waudXlen(uint16_t data, uint32_t address)
{
  _core.Sound->SetAudXlen(data, address);
}

void Sound::SetAudXlen(uint16_t data, uint32_t address)
{
  uint32_t channel = GetChannelNumber(address);
  _audlen[channel] = data;
}

// ==================
// AUDXPER
// ==================
// $dff0a6,b6,c6,d6

void waudXper(uint16_t data, uint32_t address)
{
  _core.Sound->SetAudXper(data, address);
}

void Sound::SetAudXper(uint16_t data, uint32_t address)
{
  uint32_t channel = GetChannelNumber(address);
  _audper[channel] = _periodTable[data];
}

// ==================
// AUDXVOL
// ==================
// $dff0a8,b8,c8,d8

void waudXvol(uint16_t data, uint32_t address)
{
  _core.Sound->SetAudXvol(data, address);
}

void Sound::SetAudXvol(uint16_t data, uint32_t address)
{
  uint32_t channel = GetChannelNumber(address);
  // Try to accomodate, replay routines sometimes access volume as a byte register at $dff0X9...
  if (((data & 0xff) == 0) && ((data & 0xff00) != 0))
  {
    data = (data >> 8) & 0xff;
  }

  if ((data & 64) == 64)
  {
    data = 63;
  }

  _audvol[channel] = data & 0x3f;
}

// ==================
// AUDXDAT
// ==================
// $dff0aa,ba,ca,da

void waudXdat(uint16_t data, uint32_t address)
{
  _core.Sound->SetAudXdat(data, address);
}

void Sound::SetAudXdat(uint16_t data, uint32_t address)
{
  uint32_t channel = GetChannelNumber(address);
  _auddat[channel] = data & 0xff;
  _auddatSet[channel] = true;
}

void Sound::ExecuteState(uint32_t channel)
{
  switch (_audstate[channel])
  {
    case 0: State0(channel); break;
    case 1: State1(channel); break;
    case 2: State2(channel); break;
    case 3: State3(channel); break;
    case 4: State4(channel); break;
    case 5: State5(channel); break;
    case 6: State6(channel); break;
    default: break;
  }
}

void Sound::State0(uint32_t channel)
{
  // -------------------
  // Statechange 0 to 1
  // -------------------

  _audlenw[channel] = _audlen[channel];
  _audptw[channel] = _audpt[channel];
  _audpercounter[channel] = 0;
  _audstate[channel] = 1;
}

void Sound::State1(uint32_t channel)
{
  // -------------------
  // Statechange 1 to 5
  // -------------------

  if (_audlenw[channel] != 1)
  {
    _audlenw[channel]--;
  }

  _audstate[channel] = 5;
  wintreq_direct((uint16_t)(_audioIrqMask[channel] | 0x8000), 0xdff09c, true);
}

void Sound::State2(uint32_t channel)
{
  if (_audpercounter[channel] >= 0x10000)
  {
    // -------------------
    // Statechange 2 to 3
    // -------------------

    _audpercounter[channel] -= 0x10000;
    _audpercounter[channel] += _audper[channel];
    _audvolw[channel] = _audvol[channel];
    _audstate[channel] = 3;
    _auddatw[channel] = _volumes[(_auddat[channel] & 0xff00) >> 8][_audvolw[channel]];
  }
  else
  {
    // -------------------
    // Statechange 2 to 2
    // -------------------

    _audpercounter[channel] += _audper[channel];
  }
}

void Sound::State3(uint32_t channel)
{
  if (_audpercounter[channel] >= 0x10000)
  {
    // -------------------
    // Statechange 3 to 2
    // -------------------

    _audpercounter[channel] -= 0x10000;
    _audpercounter[channel] += _audper[channel];
    _audvolw[channel] = _audvol[channel];
    _audstate[channel] = 2;
    _auddatw[channel] = _volumes[_auddat[channel] & 0xff][_audvolw[channel]];
    _auddat[channel] = chipmemReadWord(_audptw[channel]);
    _audptw[channel] = chipsetMaskPtr(_audptw[channel] + 2);
    if (_audlenw[channel] != 1)
    {
      _audlenw[channel]--;
    }
    else
    {
      _audlenw[channel] = _audlen[channel];
      _audptw[channel] = _audpt[channel];
      wintreq_direct((uint16_t)(_audioIrqMask[channel] | 0x8000), 0xdff09c, true);
    }
  }
  else
  {
    // -------------------
    // Statechange 3 to 3
    // -------------------

    _audpercounter[channel] += _audper[channel];
  }
}

void Sound::State4(uint32_t channel)
{
}

void Sound::State6(uint32_t channel)
{
}

void Sound::State5(uint32_t channel)
{
  // -------------------
  // Statechange 5 to 2
  // -------------------

  _audvolw[channel] = _audvol[channel];
  _audpercounter[channel] = 0;
  _auddat[channel] = chipmemReadWord(_audptw[channel]);
  _audptw[channel] = chipsetMaskPtr(_audptw[channel] + 2);
  _audstate[channel] = 2;

  if (_audlenw[channel] != 1)
  {
    _audlenw[channel]--;
  }
  else
  {
    _audlenw[channel] = _audlen[channel];
    _audptw[channel] = _audpt[channel];
    wintreq_direct((uint16_t)(_audioIrqMask[channel] | 0x8000), 0xdff09c, true);
  }
}

// ==============================================================================
// This is called by DMACON when DMA is turned off
// ==============================================================================

void Sound::ChannelKill(uint32_t channel)
{
  _auddatw[channel] = 0;
  _audstate[channel] = 0;
}

// ==============================================================================
// This is called by DMACON when DMA is turned on
// ==============================================================================

void Sound::ChannelEnable(uint32_t channel)
{
  State0(channel);
}

// ==============================================================================
// Generate sound
// Called in every end of line
// Will run the audio state machine the needed number of times
// and move the generated samples to a temporary buffer
// ==============================================================================

// ==============================================================================
//  Audio_Lowpass filters the __right and _left memory with a
//  pass1 lowpass filter at 7000 Hz
//  coded by Rainer Sinsch (sinsch@informatik.uni-frankfurt.de)
// ==============================================================================

void Sound::LowPass(uint32_t count, int16_t *bufferLeft, int16_t *bufferRight)
{
  double amplitudeDiv;
  double filterValue;
  switch (GetRate())
  {
    case sound_rates::SOUND_44100:
      amplitudeDiv = _amplitudeDiv45;
      filterValue = _filterValue45;
      break;
    case sound_rates::SOUND_31300:
      amplitudeDiv = _amplitudeDiv33;
      filterValue = _filterValue33;
      break;
    case sound_rates::SOUND_22050:
      amplitudeDiv = _amplitudeDiv22;
      filterValue = _filterValue22;
      break;
    case sound_rates::SOUND_15650:
      amplitudeDiv = _amplitudeDiv15;
      filterValue = _filterValue15;
      break;
  }

  for (uint32_t i = 0; i < count; ++i)
  {
    _lastLeft = filterValue * _lastLeft + (double)bufferLeft[i];
    bufferLeft[i] = (int16_t)(_lastLeft / amplitudeDiv);
    _lastRight = filterValue * _lastRight + (double)bufferRight[i];
    bufferRight[i] = (int16_t)(_lastRight / amplitudeDiv);
  }
}

uint32_t Sound::ChannelUpdate(uint32_t channel, int16_t *bufferLeft, int16_t *bufferRight, uint32_t count, bool halfscale, bool odd)
{
  uint32_t samplesAdded = 0;
  uint32_t i;

  if (dmacon & _audioDmaconMask[channel])
  {
    for (i = 0; i < count; ++i)
    {
      ExecuteState(channel);
      if ((!halfscale) || (halfscale && !odd))
      {
        if (channel == 0 || channel == 3)
        {
          bufferLeft[samplesAdded++] += (int16_t)_auddatw[channel];
        }
        else
        {
          bufferRight[samplesAdded++] += (int16_t)_auddatw[channel];
        }
      }
      odd = !odd;
    }
  }
  else
  {
    if (!interruptIsRequested(_audioIrqMask[channel]) && _auddatSet[channel])
    {
      _auddatSet[channel] = false;
      wintreq_direct((uint16_t)(_audioIrqMask[channel] | 0x8000), 0xdff09c, true);
    }
    if (!halfscale)
    {
      samplesAdded = count;
    }
    else
    {
      for (i = 0; i < count; ++i)
      {
        if (!odd)
        {
          samplesAdded++;
        }
        odd = !odd;
      }
    }
  }
  return samplesAdded;
}

void Sound::FrequencyHandler()
{
  int16_t *bufferLeft = (int16_t *)_left + _bufferSampleCount;
  int16_t *bufferRight = (int16_t *)_right + _bufferSampleCount;
  uint32_t count = 0;
  uint32_t samplesAdded;
  bool halfscale = (GetRate() == sound_rates::SOUND_22050 || GetRate() == sound_rates::SOUND_15650);

  if (GetRate() == sound_rates::SOUND_44100 || GetRate() == sound_rates::SOUND_22050)
  {
    while (audiocounter <= 0x40000)
    {
      count++;
      audiocounter += _scale;
    }
  }
  else
  {
    count = 2;
  }

  audiocounter -= 0x40000;
  for (uint32_t i = 0; i < count; ++i)
  {
    bufferLeft[i] = bufferRight[i] = 0;
  }

  for (uint32_t i = 0; i < 4; ++i)
  {
    samplesAdded = ChannelUpdate(i, bufferLeft, bufferRight, count, halfscale, audioodd);
  }

  if (halfscale && count & 1)
  {
    audioodd = !audioodd;
  }

  if (_filter != SOUND_FILTER_NEVER)
  {
    if (_filter == SOUND_FILTER_ALWAYS || ciaIsSoundFilterEnabled())
    {
      LowPass(samplesAdded, bufferLeft, bufferRight);
    }
  }
  _bufferSampleCount += samplesAdded;
}

void Sound::SetRate(sound_rates rate)
{
  _rate = rate;
}

sound_rates Sound::GetRate()
{
  return _rate;
}

uint32_t Sound::GetRateReal()
{
  switch (GetRate())
  {
    case sound_rates::SOUND_44100: return 44100;
    case sound_rates::SOUND_31300: return 31300;
    case sound_rates::SOUND_22050: return 22050;
    case sound_rates::SOUND_15650: return 15650;
  }
  return 0;
}

void Sound::SetIsStereo(bool stereo)
{
  _isStereo = stereo;
}

bool Sound::GetIsStereo()
{
  return _isStereo;
}

void Sound::SetIs16Bits(bool bits16)
{
  _is16Bits = bits16;
}

bool Sound::GetIs16Bits()
{
  return _is16Bits;
}

void Sound::SetEmulation(sound_emulations emulation)
{
  _emulation = emulation;
}

sound_emulations Sound::GetEmulation()
{
  return _emulation;
}

void Sound::SetFilter(sound_filters filter)
{
  _filter = filter;
}

sound_filters Sound::GetFilter()
{
  return _filter;
}

void Sound::SetNotification(sound_notifications notification)
{
  _notification = notification;
}

sound_notifications Sound::GetNotification()
{
  return _notification;
}

void Sound::SetBufferLength(uint32_t ms)
{
  _bufferLength = ms;
}

uint32_t Sound::GetBufferLength()
{
  return _bufferLength;
}

int Sound::GetVolume()
{
  return _volume;
}

void Sound::SetVolume(const int volume)
{
  _volume = volume;
}

void Sound::SetWAVDump(bool wavCapture)
{
  _wavCapture = wavCapture;
}

bool Sound::GetWAVDump()
{
  return _wavCapture;
}

void Sound::SetBufferSampleCount(uint32_t sampleCount)
{
  _bufferSampleCount = sampleCount;
}

uint32_t Sound::GetBufferSampleCount()
{
  return _bufferSampleCount;
}

void Sound::SetBufferSampleCountMax(uint32_t sampleCountMax)
{
  _bufferSampleCountMax = sampleCountMax;
}

uint32_t Sound::GetBufferSampleCountMax()
{
  return _bufferSampleCountMax;
}

void Sound::SetDeviceFound(bool deviceFound)
{
  _deviceFound = deviceFound;
}

bool Sound::GetDeviceFound()
{
  return _deviceFound;
}

void Sound::SetScale(uint32_t scale)
{
  _scale = scale;
}

uint32_t Sound::GetScale()
{
  return _scale;
}

void Sound::SetSampleVolume(uint8_t sampleIn, uint8_t volume, int16_t sampleOut)
{
  _volumes[sampleIn][volume] = sampleOut;
}

int16_t Sound::GetSampleVolume(int8_t sampleIn, uint8_t volume)
{
  return _volumes[sampleIn][volume];
}

void Sound::SetPeriodValue(uint32_t period, uint32_t value)
{
  _periodTable[period] = value;
}

uint32_t Sound::GetPeriodValue(uint32_t period)
{
  return _periodTable[period];
}

void Sound::VolumeTableInitialize(bool isStereo)
{
  int32_t s = isStereo ? 2 : 1;

  for (int32_t i = -128; i < 128; i++)
  {
    for (int32_t j = 0; j < 64; j++)
    {
      if (j == 0)
      {
        SetSampleVolume((uint8_t)(i & 0xff), (uint8_t)j, (int16_t)0);
      }
      else
      {
        SetSampleVolume((uint8_t)(i & 0xff), (uint8_t)j, (int16_t)((i * j * s)));
      }
    }
  }
}

void Sound::PeriodTableInitialize(uint32_t outputRate)
{
  if (outputRate < 29000)
  {
    outputRate *= 2; // Internally, can not run slower than max Amiga rate
  }

  SetScale((uint32_t)(((double)(65536.0 * 2.0 * 31200.0)) / ((double)outputRate)));

  SetPeriodValue(0, 0x10000);
  for (int32_t i = 1; i < 65536; i++)
  {
    double j = 3546895 / i; // Sample rate
    int32_t periodvalue = (uint32_t)((j * 65536) / outputRate);
    if (periodvalue > 0x10000)
    {
      periodvalue = 0x10000;
    }
    SetPeriodValue(i, periodvalue);
  }
}

void Sound::PlaybackInitialize()
{
  audiocounter = 0;
  if (GetEmulation() != SOUND_NONE)
  { // Play sound
    PeriodTableInitialize(GetRateReal());
    VolumeTableInitialize(GetIsStereo());
    SetBufferSampleCount(0);
    _currentBuffer = 0;
    SetBufferSampleCountMax(static_cast<uint32_t>(static_cast<float>(GetRateReal()) / (1000.0f / static_cast<float>(GetBufferLength()))));
  }
}

void Sound::IOHandlersInstall()
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

void Sound::IORegistersClear()
{
  _audstate[0] = 0;
  _audstate[1] = 0;
  _audstate[2] = 0;
  _audstate[3] = 0;

  for (int i = 0; i < 4; i++)
  {
    _audpt[i] = 0;
    _audptw[i] = 0;
    _audlen[i] = 2;
    _audper[i] = 0;
    _audvol[i] = 0;
    _audpercounter[i] = 0;
    _auddat[i] = 0;
    _auddatSet[i] = false;
    _auddatw[i] = 0;
    _audlenw[i] = 2;
    _audvolw[i] = 0;
  }
  _currentBuffer = 0;
}

void Sound::CopyBufferOverrunToCurrentBuffer(uint32_t availableSamples, uint32_t previousBuffer)
{
  uint32_t pos = 0;
  for (uint32_t i = GetBufferSampleCountMax(); i < availableSamples; i++)
  {
    _left[_currentBuffer][pos] = _left[previousBuffer][i];
    _right[_currentBuffer][pos] = _right[previousBuffer][i];
    pos++;
  }
  SetBufferSampleCount(pos + MAX_BUFFER_SAMPLES * _currentBuffer);
}

void Sound::EndOfLine()
{
  if (GetEmulation() == SOUND_NONE)
  {
    return;
  }

  FrequencyHandler();
  uint32_t availableSamples = GetBufferSampleCount() - _currentBuffer * MAX_BUFFER_SAMPLES;

  if (availableSamples < GetBufferSampleCountMax())
  {
    return;
  }

  if (GetEmulation() == SOUND_PLAY)
  {
    _core.Drivers.SoundDriver->Play(_left[_currentBuffer], _right[_currentBuffer], GetBufferSampleCountMax());
  }

  if (GetWAVDump())
  {
    _wavFileWriter.Play(_left[_currentBuffer], _right[_currentBuffer], GetBufferSampleCountMax());
  }

  int previousBuffer = _currentBuffer;
  _currentBuffer++;
  if (_currentBuffer > 1)
  {
    _currentBuffer = 0;
  }
  SetBufferSampleCount(0 + MAX_BUFFER_SAMPLES * _currentBuffer);

  if (availableSamples > GetBufferSampleCountMax())
  {
    CopyBufferOverrunToCurrentBuffer(availableSamples, previousBuffer);
  }
}

void Sound::EmulationStart()
{
  IOHandlersInstall();
  audioodd = 0;
  PlaybackInitialize();
  if (GetEmulation() == SOUND_PLAY)
  {
    auto soundDriverStarted = _core.Drivers.SoundDriver->EmulationStart(SoundDriverRuntimeConfiguration{
        GetEmulation(),
        GetRate(),
        GetFilter(),
        GetNotification(),
        GetIsStereo(),
        GetIs16Bits(),
        GetVolume(),
        GetRateReal(),
        GetBufferSampleCountMax(),
    });

    if (!soundDriverStarted)
    {
      SetEmulation(SOUND_EMULATE);
    }
  }
  if (GetWAVDump() && GetEmulation() != SOUND_NONE)
  {
    _wavFileWriter.EmulationStart(GetRate(), GetIs16Bits(), GetIsStereo(), GetRateReal());
  }
}

void Sound::EmulationStop()
{
  if (GetEmulation() == SOUND_PLAY)
  {
    _core.Drivers.SoundDriver->EmulationStop();
  }

  if (GetWAVDump() && GetEmulation() != SOUND_NONE)
  {
    _wavFileWriter.EmulationStop();
  }
}

void Sound::HardReset()
{
  IORegistersClear();
}

void Sound::Startup()
{
  SetEmulation(SOUND_EMULATE);
  SetFilter(SOUND_FILTER_ORIGINAL);
  SetRate(sound_rates::SOUND_15650);
  SetIsStereo(false);
  SetIs16Bits(false);
  SetNotification(SOUND_MMTIMER_NOTIFICATION);
  SetWAVDump(false);
  SetBufferLength(40);
  IORegistersClear();
  SetDeviceFound(_core.Drivers.SoundDriver->IsInitialized());
  _wavFileWriter.Startup();

  if (GetEmulation() == SOUND_PLAY && !GetDeviceFound())
  {
    SetEmulation(SOUND_EMULATE);
  }
}

void Sound::Shutdown()
{
  _wavFileWriter.Shutdown();
}

Sound::Sound()
{
}

Sound::~Sound()
{
}
