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

#define INITGUID

#include <windowsx.h>

#include "GfxDrvCommon.h"
#include "fellow/api/defs.h"
#include "DirectSoundDriver.h"
#include "windrv.h"
#include "fellow/api/Services.h"
#include "VirtualHost/Core.h"

using namespace fellow::api;
using namespace CustomChipset;

const char* DirectSoundDriver::DSoundErrorString(HRESULT hResult)
{
  switch (hResult)
  {
  case DSERR_ALLOCATED: return "DSERR_ALLOCATED";
  case DSERR_CONTROLUNAVAIL: return "DSERR_CONTROLUNAVAIL";
  case DSERR_INVALIDPARAM: return "DSERR_INVALIDPARAM";
  case DSERR_INVALIDCALL: return "DSERR_INVALIDCALL";
  case DSERR_GENERIC: return "DSERR_GENERIC";
  case DSERR_PRIOLEVELNEEDED: return "DSERR_PRIOLEVELNEEDED";
  case DSERR_OUTOFMEMORY: return "DSERR_OUTOFMEMORY";
  case DSERR_BADFORMAT: return "DSERR_BADFORMAT";
  case DSERR_UNSUPPORTED: return "DSERR_UNSUPPORTED";
  case DSERR_NODRIVER: return "DSERR_NODRIVER";
  case DSERR_ALREADYINITIALIZED: return "DSERR_ALREADYINITIALIZED";
  case DSERR_NOAGGREGATION: return "DSERR_NOAGGREGATION";
  case DSERR_BUFFERLOST: return "DSERR_BUFFERLOST";
  case DSERR_OTHERAPPHASPRIO: return "DSERR_OTHERAPPHASPRIO";
  case DSERR_UNINITIALIZED: return "DSERR_UNINITIALIZED";
  }

  return "Unknown DirectSound Error";
}

//========================================================================
// Multimedia Callback fnc. Used when DirectSound notification unsupported
//========================================================================

void CALLBACK DirectSoundDriver::timercb(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
  ((DirectSoundDriver*)dwUser)->HandleTimerCallback();
}

void DirectSoundDriver::HandleTimerCallback()
{
  PollBufferPosition();
}

void DirectSoundDriver::DSoundFailure(const char* header, HRESULT errorCode)
{
  _core.Log->AddLog(header);
  _core.Log->AddLog(DSoundErrorString(errorCode));
  _core.Log->AddLog("\n");
}

void DirectSoundDriver::DSoundRelease()
{
  if (_lpDS != nullptr)
  {
    IDirectSound_Release(_lpDS);
    _lpDS = nullptr;
  }

  for (unsigned int i = 0; i < 3; i++)
  {
    if (_notifications[i] != nullptr)
    {
      CloseHandle(_notifications[i]);
      _notifications[i] = nullptr;
    }
  }

  if (_dataAvailable != nullptr)
  {
    CloseHandle(_dataAvailable);
    _dataAvailable = nullptr;
  }

  if (_canAddData != nullptr)
  {
    CloseHandle(_canAddData);
    _canAddData = nullptr;
  }
}

bool DirectSoundDriver::DSoundInitialize()
{
  _lpDS = nullptr;
  _lpDSB = nullptr;
  _lpDSBS = nullptr;
  _lpDSN = nullptr;

  for (unsigned int i = 0; i < 3; i++)
  {
    _notifications[i] = nullptr;
  }

  _dataAvailable = nullptr;
  _canAddData = nullptr;
  _thread = nullptr;

  HRESULT directSoundCreateResult = DirectSoundCreate(nullptr, &_lpDS, nullptr);
  if (directSoundCreateResult != DS_OK)
  {
    DSoundFailure("DirectSoundDriver::DSoundInitialize(): DirectSoundCreate - ", directSoundCreateResult);
    return false;
  }

  for (unsigned int i = 0; i < 3; i++)
  {
    _notifications[i] = CreateEvent(nullptr, 0, 0, nullptr);
  }

  _dataAvailable = CreateEvent(nullptr, 0, 0, nullptr);
  _canAddData = CreateEvent(nullptr, 0, 0, nullptr);

  return true;
}

void DirectSoundDriver::AddMode(bool isStereo, bool is16Bits, uint32_t rate)
{
  auto mode = new DirectSoundMode();
  mode->Rate = rate;
  mode->Is16Bits = is16Bits;
  mode->IsStereo = isStereo;
  mode->BufferSampleCount = 0;
  mode->BufferBlockAlign = 0;
  _modes.emplace_back(mode);
}

const DirectSoundMode* DirectSoundDriver::FindMode(bool isStereo, bool is16Bits, uint32_t rate) const
{
  for (const auto* mode : _modes)
  {
    if (mode->Rate == rate && mode->Is16Bits == is16Bits && mode->IsStereo == isStereo)
    {
      return mode;
    }
  }

  return nullptr;
}

void DirectSoundDriver::DSoundModeInformationRelease()
{
  for (const auto* mode : _modes)
  {
    delete mode;
  }
  _modes.clear();
}

void DirectSoundDriver::YesNoLog(const char* description, bool predicate)
{
  _core.Log->AddLog(description);
  _core.Log->AddLog(predicate ? " - Yes\n" : " - No\n");
}

bool DirectSoundDriver::DSoundModeInformationInitialize()
{
  DSCAPS dscaps = {};
  dscaps.dwSize = sizeof(dscaps);
  HRESULT getCapsResult = IDirectSound_GetCaps(_lpDS, &dscaps);
  if (getCapsResult != DS_OK)
  {
    DSoundFailure("DirectSoundDriver::DSoundModeInformationInitialize(): ", getCapsResult);
    return false;
  }

  bool supportsStereo = !!(dscaps.dwFlags & DSCAPS_PRIMARYSTEREO);
  YesNoLog("DSCAPS_PRIMARYSTEREO", supportsStereo);
  bool supportsMono = !!(dscaps.dwFlags & DSCAPS_PRIMARYMONO);
  YesNoLog("DSCAPS_PRIMARYMONO", supportsMono);
  bool supports16Bits = !!(dscaps.dwFlags & DSCAPS_PRIMARY16BIT);
  YesNoLog("DSCAPS_PRIMARY16BIT", supports16Bits);
  bool supports8Bits = !!(dscaps.dwFlags & DSCAPS_PRIMARY8BIT);
  YesNoLog("DSCAPS_PRIMARY8BIT", supports8Bits);

  bool secondaryStereo = !!(dscaps.dwFlags & DSCAPS_SECONDARYSTEREO);
  YesNoLog("DSCAPS_SECONDARYSTEREO", secondaryStereo);
  bool secondaryMono = !!(dscaps.dwFlags & DSCAPS_SECONDARYMONO);
  YesNoLog("DSCAPS_SECONDARYMONO", secondaryMono);
  bool secondary16Bits = !!(dscaps.dwFlags & DSCAPS_SECONDARY16BIT);
  YesNoLog("DSCAPS_SECONDARY16BIT", secondary16Bits);
  bool secondary8Bits = !!(dscaps.dwFlags & DSCAPS_SECONDARY8BIT);
  YesNoLog("DSCAPS_SECONDARY8BIT", secondary8Bits);

  bool supportsContinuousRate = !!(dscaps.dwFlags & DSCAPS_CONTINUOUSRATE);
  YesNoLog("DSCAPS_CONTINUOUSRATE", supportsContinuousRate);
  bool isEmulatedDriver = !!(dscaps.dwFlags & DSCAPS_EMULDRIVER);
  YesNoLog("DSCAPS_EMULDRIVER", isEmulatedDriver);
  bool isCertifiedDriver = !!(dscaps.dwFlags & DSCAPS_CERTIFIED);
  YesNoLog("DSCAPS_CERTIFIED", isCertifiedDriver);

  uint32_t minrate = dscaps.dwMinSecondarySampleRate;
  uint32_t maxrate = dscaps.dwMaxSecondarySampleRate;
  _core.Log->AddLog("ddscaps.dwMinSecondarySampleRate - %u\n", minrate);
  _core.Log->AddLog("ddscaps.dwMaxSecondarySampleRate - %u\n", maxrate);

  if (supportsStereo)
  {
    if (supports16Bits)
    {
      AddMode(true, true, 15650);
      AddMode(true, true, 22050);
      AddMode(true, true, 31300);
      AddMode(true, true, 44100);
    }
    if (supports8Bits)
    {
      AddMode(true, false, 15650);
      AddMode(true, false, 22050);
      AddMode(true, false, 31300);
      AddMode(true, false, 44100);
    }
  }
  if (supportsMono)
  {
    if (supports16Bits)
    {
      AddMode(false, true, 15650);
      AddMode(false, true, 22050);
      AddMode(false, true, 31300);
      AddMode(false, true, 44100);
    }
    if (supports8Bits)
    {
      AddMode(false, false, 15650);
      AddMode(false, false, 22050);
      AddMode(false, false, 31300);
      AddMode(false, false, 44100);
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
bool DirectSoundDriver::DSoundSetVolume(const int volume)
{
  HRESULT hResult;
  LONG vol;

  if (volume <= 100 && volume > 0)
    vol = (LONG)-((50 - (volume / 2)) * (50 - (volume / 2)));
  else if (volume == 0)
    vol = DSBVOLUME_MIN;
  else
    vol = DSBVOLUME_MAX;

#ifdef _DEBUG
  _core.Log->AddLog("DirectSoundDriver::DSoundSetVolume(): volume %d scaled to %d, setting volume...\n", volume, vol);
#endif

  hResult = IDirectSoundBuffer_SetVolume(_lpDSBS, vol);
  if (FAILED(hResult)) DSoundFailure("DirectSoundDriver::DSoundSetVolume(): SetVolume() failed: ", hResult);

  return (hResult == DS_OK);
}

bool DirectSoundDriver::SetCurrentSoundDeviceVolume(int volume)
{
  return DSoundSetVolume(volume);
}

bool DirectSoundDriver::DSoundSetCooperativeLevel()
{
  // We need the HWND of the amiga emulation window, which means sound must be initialized after the gfx stuff

  HWND hwnd = gfxDrvCommon->GetHWND();
  HRESULT setCooperativeLevelResult = IDirectSound_SetCooperativeLevel(_lpDS, hwnd, DSSCL_PRIORITY);
  if (setCooperativeLevelResult != DS_OK)
  {
    DSoundFailure("DirectSoundDriver::DSoundSetCooperativeLevel()", setCooperativeLevelResult);
  }

  return (setCooperativeLevelResult == DS_OK);
}

void DirectSoundDriver::DSoundPrimaryBufferRelease()
{
  if (_lpDSB != nullptr)
  {
    IDirectSoundBuffer_Play(_lpDSB, 0, 0, 0);
    IDirectSoundBuffer_Release(_lpDSB);
    _lpDSB = nullptr;
  }
}

//======================================================================
// Get a primary buffer
// We always make a primary buffer, in the same format as the sound data
// we are going to receive from the sound emulation routines.
// If it fails, we don't play sound.
//======================================================================

bool DirectSoundDriver::DSoundPrimaryBufferInitialize()
{
  DSBUFFERDESC dsbdesc{};
  WAVEFORMATEX wfm{};

  dsbdesc.dwSize = sizeof(dsbdesc);
  dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
  dsbdesc.dwBufferBytes = 0;
  dsbdesc.lpwfxFormat = nullptr;

  wfm.wFormatTag = WAVE_FORMAT_PCM;
  wfm.nChannels = (_modeCurrent.IsStereo) ? 2 : 1;
  wfm.nSamplesPerSec = _modeCurrent.Rate;
  wfm.wBitsPerSample = (_modeCurrent.Is16Bits) ? 16 : 8;
  wfm.nBlockAlign = (wfm.wBitsPerSample / 8) * wfm.nChannels;
  wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign;
  _modeCurrent.BufferBlockAlign = wfm.nBlockAlign;

  HRESULT createSoundBufferResult = IDirectSound_CreateSoundBuffer(_lpDS, &dsbdesc, &_lpDSB, NULL);
  if (createSoundBufferResult != DS_OK)
  {
    DSoundFailure("DirectSoundDriver::DSoundPrimaryBufferInitialize(): CreateSoundBuffer(), ", createSoundBufferResult);
    return false;
  }

  HRESULT setFormatResult = IDirectSoundBuffer_SetFormat(_lpDSB, &wfm);
  if (setFormatResult != DS_OK)
  {
    DSoundFailure("DirectSoundDriver::DSoundPrimaryBufferInitialize(): SetFormat(), ", setFormatResult);
    DSoundPrimaryBufferRelease();
    return false;
  }

  return true;
}

void DirectSoundDriver::DSoundSecondaryBufferRelease()
{
  if (_lpDSBS != nullptr)
  {
    IDirectSoundBuffer_Play(_lpDSBS, 0, 0, 0);
    IDirectSoundBuffer_Release(_lpDSBS);
    _lpDSBS = nullptr;
  }

  if (_lpDSN != nullptr)
  {
    IDirectSoundNotify_Release(_lpDSN);
    _lpDSN = nullptr;
  }

  if (!_notificationSupported)
  {
    timeKillEvent(_mmTimer);
    timeEndPeriod(_mmResolution);
  }
}

bool DirectSoundDriver::CreateSecondaryBuffer()
{
  WAVEFORMATEX wfm{};
  DSBUFFERDESC dsbdesc{};

  wfm.wFormatTag = WAVE_FORMAT_PCM;
  wfm.nChannels = (_modeCurrent.IsStereo) ? 2 : 1;
  wfm.nSamplesPerSec = _modeCurrent.Rate;
  wfm.wBitsPerSample = (_modeCurrent.Is16Bits) ? 16 : 8;
  wfm.nBlockAlign = (wfm.wBitsPerSample / 8) * wfm.nChannels;
  wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign;

  dsbdesc.dwSize = sizeof(dsbdesc);
  dsbdesc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLVOLUME;
  dsbdesc.dwBufferBytes = _modeCurrent.BufferSampleCount * wfm.nBlockAlign * 2;
  dsbdesc.lpwfxFormat = &wfm;

  HRESULT createSoundBufferResult = IDirectSound_CreateSoundBuffer(_lpDS, &dsbdesc, &_lpDSBS, NULL);
  if (createSoundBufferResult != DS_OK)
  {
    DSoundFailure("DirectSoundDriver::CreateSecondaryBuffer: CreateSoundBuffer(), ", createSoundBufferResult);
    return false;
  }

  return true;
}

bool DirectSoundDriver::ClearSecondaryBuffer()
{
  char* lpAudio;
  DWORD dwBytes;

  HRESULT lock1Result = IDirectSoundBuffer_Lock(
    _lpDSBS,
    0,
    0, // Ignored because we pass DSBLOCK_ENTIREBUFFER
    (LPVOID*)&lpAudio,
    &dwBytes,
    NULL,
    NULL,
    DSBLOCK_ENTIREBUFFER);

  if (lock1Result != DS_OK)
  {
    DSoundFailure("DirectSoundDriver::ClearSecondaryBuffer: Lock(), ", lock1Result);

    if (lock1Result == DSERR_BUFFERLOST)
    {
      // Buffer lost. Try to restore buffer once. Multiple restores need more intelligence
      HRESULT restoreResult = IDirectSoundBuffer_Restore(_lpDSBS);
      if (restoreResult != DS_OK)
      {
        DSoundFailure("DirectSoundDriver::ClearSecondaryBuffer: Restore(), ", restoreResult);
        return false;
      }

      HRESULT lock2Result = IDirectSoundBuffer_Lock(
        _lpDSBS,
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
        DSoundFailure("DirectSoundDriver::ClearSecondaryBuffer: Lock(), ", lock2Result);
        return false;
      }
    }
  }

  for (DWORD i = 0; i < dwBytes; i++)
  {
    lpAudio[i] = 0;
  }

  HRESULT unlockResult = IDirectSoundBuffer_Unlock(_lpDSBS, lpAudio, dwBytes, NULL, 0);
  if (unlockResult != DS_OK)
  {
    // Here we give up
    DSoundFailure("DirectSoundDriver::ClearSecondaryBuffer: Unlock(), ", unlockResult);
    return false;
  }

  return true;
}

bool DirectSoundDriver::InitializeSecondaryBufferNotification()
{
  DSBCAPS dsbcaps = {};
  dsbcaps.dwSize = sizeof(dsbcaps);

  HRESULT getCapsResult = IDirectSoundBuffer_GetCaps(_lpDSBS, &dsbcaps);
  if (getCapsResult != DS_OK)
  {
    DSoundFailure("DirectSoundDriver::InitializeSecondaryBufferNotification(): GetCaps(), ", getCapsResult);
    return false;
  }

  _notificationSupported = _runtimeConfiguration.NotificationMode == sound_notifications::SOUND_DSOUND_NOTIFICATION && !!(dsbcaps.dwFlags & DSBCAPS_CTRLPOSITIONNOTIFY);

  if (_notificationSupported)
  {
    DSBPOSITIONNOTIFY rgdscbpn[2];

    // Notification supported AND selected
    // Get notification interface

    HRESULT queryInterfaceResult = IDirectSoundBuffer_QueryInterface(_lpDSBS, IID_IDirectSoundNotify, (LPVOID * FAR) & _lpDSN);
    if (queryInterfaceResult != DS_OK)
    {
      DSoundFailure("DirectSoundDriver::InitializeSecondaryBufferNotification(): QueryInterface(IID_IDirectSoundNotify), ", queryInterfaceResult);
      return false;
    }

    // Attach notification objects to buffer

    rgdscbpn[0].dwOffset = _modeCurrent.BufferBlockAlign * (_modeCurrent.BufferSampleCount - 1);
    rgdscbpn[0].hEventNotify = _notifications[0];
    rgdscbpn[1].dwOffset = _modeCurrent.BufferBlockAlign * (_modeCurrent.BufferSampleCount * 2 - 1);
    rgdscbpn[1].hEventNotify = _notifications[1];

    HRESULT setNotificationPositionsResult = IDirectSoundNotify_SetNotificationPositions(_lpDSN, 2, rgdscbpn);
    if (setNotificationPositionsResult != DS_OK)
    {
      DSoundFailure("DirectSoundDriver::InitializeSecondaryBufferNotification(): SetNotificationPositions(), ", setNotificationPositionsResult);
      return false;
    }
  }
  else
  {
    // Notification not supported, fallback to timer

    char s[80];
    TIMECAPS timecaps;

    MMRESULT timeGetDevCapsResult = timeGetDevCaps(&timecaps, sizeof(TIMECAPS));
    if (timeGetDevCapsResult != TIMERR_NOERROR)
    {
      _core.Log->AddLog("DirectSoundDriver::InitializeSecondaryBufferNotification(): timeGetDevCaps() failed\n");
      return false;
    }

    sprintf(s, "timeGetDevCaps: min: %u, max %u\n", timecaps.wPeriodMin, timecaps.wPeriodMax);
    _core.Log->AddLog(s);

    _mmResolution = timecaps.wPeriodMin;

    MMRESULT timeBeginPeriodResult = timeBeginPeriod(_mmResolution);
    if (timeBeginPeriodResult != TIMERR_NOERROR)
    {
      _core.Log->AddLog("DirectSoundDriver::InitializeSecondaryBufferNotification(): timeBeginPeriod() failed\n");
      return false;
    }

    MMRESULT timeSetEventResult = timeSetEvent(1, 0, DirectSoundDriver::timercb, (DWORD_PTR)this, (UINT)TIME_PERIODIC);
    if (timeSetEventResult == 0)
    {
      _core.Log->AddLog("DirectSoundDriver::InitializeSecondaryBufferNotification(): timeSetEvent() failed\n");
      return false;
    }
    _mmTimer = timeSetEventResult;
  }
  return true;
}

bool DirectSoundDriver::DSoundSecondaryBufferInitialize()
{
  if (!CreateSecondaryBuffer())
  {
    return false;
  }

  if (!ClearSecondaryBuffer())
  {
    DSoundSecondaryBufferRelease();
    return false;
  }

  if (!InitializeSecondaryBufferNotification())
  {
    DSoundSecondaryBufferRelease();
    return false;
  }

  DSoundSetVolume(_runtimeConfiguration.Volume);

  return true;
}

void DirectSoundDriver::DSoundPlaybackStop()
{
  DSoundSecondaryBufferRelease();
  DSoundPrimaryBufferRelease();
}

bool DirectSoundDriver::DSoundPlaybackInitialize()
{
  _lastReadPosition = 0;
  bool result = DSoundPrimaryBufferInitialize();
  if (result)
  {
    result = DSoundSecondaryBufferInitialize();
  }

  if (!result)
  {
    _core.Log->AddLog("Sound, secondary failed\n");
    DSoundPrimaryBufferRelease();
  }

  if (result)
  {
    HRESULT primaryPlayResult = IDirectSoundBuffer_Play(_lpDSB, 0, 0, DSBPLAY_LOOPING);
    if (primaryPlayResult != DS_OK)
    {
      DSoundFailure("DirectSoundDriver::DSoundPlaybackInitialize(): Primary->Play(), ", primaryPlayResult);
    }

    HRESULT secondaryPlayResult = IDirectSoundBuffer_Play(_lpDSBS, 0, 0, DSBPLAY_LOOPING);
    if (secondaryPlayResult != DS_OK)
    {
      DSoundFailure("DirectSoundDriver::DSoundPlaybackInitialize(): Secondary->Play(), ", secondaryPlayResult);
    }
  }

  return result;
}

void DirectSoundDriver::Copy16BitsStereo(uint16_t* audioBuffer, uint16_t* left, uint16_t* right, uint32_t sampleCount)
{
  for (unsigned int i = 0; i < sampleCount; i++)
  {
    *audioBuffer++ = *left++;
    *audioBuffer++ = *right++;
  }
}

void DirectSoundDriver::Copy16BitsMono(uint16_t* audioBuffer, uint16_t* left, uint16_t* right, uint32_t sampleCount)
{
  for (unsigned int i = 0; i < sampleCount; i++)
  {
    *audioBuffer++ = (*left++ + *right++);
  }
}

void DirectSoundDriver::Copy8BitsStereo(uint8_t* audioBuffer, uint16_t* left, uint16_t* right, uint32_t sampleCount)
{
  for (unsigned int i = 0; i < sampleCount; i++)
  {
    *audioBuffer++ = ((*left++) >> 8) + 128;
    *audioBuffer++ = ((*right++) >> 8) + 128;
  }
}

void DirectSoundDriver::Copy8BitsMono(uint8_t* audioBuffer, uint16_t* left, uint16_t* right, uint32_t sampleCount)
{
  for (unsigned int i = 0; i < sampleCount; i++)
  {
    *audioBuffer++ = (((*left++) + (*right++)) >> 8) + 128;
  }
}

bool DirectSoundDriver::DSoundCopyToBuffer(uint16_t* left, uint16_t* right, uint32_t sampleCount, uint32_t bufferHalf)
{
  LPVOID lpvAudio;
  DWORD dwBytes;
  DWORD size = _modeCurrent.BufferSampleCount * _modeCurrent.BufferBlockAlign;
  DWORD startOffset = bufferHalf * size;

  HRESULT lockResult = IDirectSoundBuffer_Lock(_lpDSBS, startOffset, size, &lpvAudio, &dwBytes, NULL, NULL, 0);

  if (lockResult != DS_OK)
  {
    DSoundFailure("DirectSoundDriver::DSoundCopyToBuffer(): Lock(), ", lockResult);
    return false;
  }

  if (lockResult == DSERR_BUFFERLOST)
  {
    // Buffer lost. Try to restore buffer once. Multiple restores need more intelligence
    HRESULT restoreResult = IDirectSoundBuffer_Restore(_lpDSBS);
    if (restoreResult != DS_OK)
    {
      DSoundFailure("DirectSoundDriver::DSoundCopyToBuffer(): Restore(), ", restoreResult);
      return false;
    }
    HRESULT lock2Result = IDirectSoundBuffer_Lock(_lpDSBS, startOffset, size, &lpvAudio, &dwBytes, NULL, NULL, 0);
    if (lock2Result != DS_OK)
    {
      DSoundFailure("DirectSoundDriver::DSoundCopyToBuffer(): Lock() after Restore , ", lock2Result);
      return false;
    }
  }

  if (_runtimeConfiguration.IsStereo)
  {
    if (_runtimeConfiguration.Is16Bits)
    {
      Copy16BitsStereo((uint16_t*)lpvAudio, left, right, sampleCount);
    }
    else
    {
      Copy8BitsStereo((uint8_t*)lpvAudio, left, right, sampleCount);
    }
  }
  else
  {
    if (_runtimeConfiguration.Is16Bits)
    {
      Copy16BitsMono((uint16_t*)lpvAudio, left, right, sampleCount);
    }
    else
    {
      Copy8BitsMono((uint8_t*)lpvAudio, left, right, sampleCount);
    }
  }

  HRESULT unlockResult = IDirectSoundBuffer_Unlock(_lpDSBS, lpvAudio, dwBytes, NULL, 0);
  if (unlockResult != DS_OK)
  {
    DSoundFailure("DirectSoundDriver::DSoundCopyToBuffer(): Unlock(), ", unlockResult);
    return false;
  }

  return true;
}

void DirectSoundDriver::Play(int16_t* left, int16_t* right, uint32_t sampleCount)
{
  WaitForSingleObject(_canAddData, INFINITE);
  _pendingDataLeft = (uint16_t*)left;
  _pendingDataRight = (uint16_t*)right;
  _pendingDataSampleCount = sampleCount;
  ResetEvent(_canAddData);
  SetEvent(_dataAvailable);
}

void DirectSoundDriver::AcquireSoundMutex()
{
  WaitForSingleObject(_mutex, INFINITE);
}

void DirectSoundDriver::ReleaseSoundMutex()
{
  ReleaseMutex(_mutex);
}

bool DirectSoundDriver::WaitForData(uint32_t nextBufferNo, bool& needToRestartPlayback)
{
  HANDLE multiEvents[3];
  bool terminateWait = false;
  bool terminateThread = false;
  uint32_t numberOfEventsInWait = 3;

  // No-wait test for data_available, return TRUE if data is available
  if (WaitForSingleObject(_dataAvailable, 0) == WAIT_OBJECT_0)
  {
    return true;
  }

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

  while (!terminateWait)
  {
    multiEvents[0] = _dataAvailable;
    multiEvents[1] = _notifications[2];
    if (numberOfEventsInWait == 3)
    {
      multiEvents[2] = _notifications[nextBufferNo];
    }
    DWORD evt = WaitForMultipleObjects(numberOfEventsInWait, (void* const*)multiEvents, FALSE, INFINITE);
    switch (evt)
    {
    case WAIT_OBJECT_0:
      // Data is now available
      // Restart playing if we have stopped it
      needToRestartPlayback = (numberOfEventsInWait == 2);
      terminateWait = true;
      break;
    case WAIT_OBJECT_0 + 1:
      // End of thread requested, return FALSE
      terminateWait = true;
      terminateThread = true;
      break;
    case WAIT_OBJECT_0 + 2:
      // End of next buffer reached, stop playback, wait more
      HRESULT playResult = IDirectSoundBuffer_Play(_lpDSBS, 0, 0, 0);
      if (playResult != DS_OK)
      {
        DSoundFailure("DirectSoundDriver::WaitForData(): Play(), ", playResult);
      }
      numberOfEventsInWait = 2;
      break;
    }
  }
  return !terminateThread;
}

void DirectSoundDriver::PollBufferPosition()
{
  AcquireSoundMutex();
  if (_runtimeConfiguration.PlaybackMode == sound_emulations::SOUND_PLAY && !_notificationSupported)
  {
    DWORD readpos, writepos;
    DWORD halfway = (_modeCurrent.BufferSampleCount * _modeCurrent.BufferBlockAlign);

    if (_lpDSBS != nullptr)
    {
      HRESULT getCurrentPositionResult = IDirectSoundBuffer_GetCurrentPosition(_lpDSBS, &readpos, &writepos);
      if (getCurrentPositionResult != DS_OK)
      {
        DSoundFailure("DirectSoundDriver::PollBufferPosition(): GetCurrentPosition(), ", getCurrentPositionResult);
      }

      if ((readpos >= halfway) && (_lastReadPosition < halfway))
      {
        SetEvent(_notifications[0]);
      }
      else if ((readpos < halfway) && (_lastReadPosition >= halfway))
      {
        SetEvent(_notifications[1]);
      }

      _lastReadPosition = readpos;
    }
  }

  ReleaseSoundMutex();
}

bool DirectSoundDriver::ProcessEndOfBuffer(uint32_t currentBufferNo, uint32_t nextBufferNo)
{
  bool terminateThread = false;
  bool needToRestartPlayback = false;
  if (WaitForData(nextBufferNo, needToRestartPlayback))
  {
    DSoundCopyToBuffer(_pendingDataLeft, _pendingDataRight, _pendingDataSampleCount, currentBufferNo);
    if (needToRestartPlayback)
    {
      HRESULT playResult = IDirectSoundBuffer_Play(_lpDSBS, 0, 0, DSBPLAY_LOOPING);

      if (playResult != DS_OK)
      {
        DSoundFailure("DirectSoundDriver::ProcessEndOfBuffer(): Play(), ", playResult);
      }

      if (playResult == DSERR_BUFFERLOST)
      {
        /* Temporary fix for restoring buffer once. Multiple restore needs alot more */
        HRESULT restoreResult = IDirectSoundBuffer_Restore(_lpDSBS);
        if (restoreResult != DS_OK)
        {
          DSoundFailure("DirectSoundDriver::ProcessEndOfBuffer(): Restore(), ", restoreResult);
        }
        else
        {
          HRESULT play2Result = IDirectSoundBuffer_Play(_lpDSBS, 0, 0, DSBPLAY_LOOPING);
          if (play2Result != DS_OK)
          {
            DSoundFailure("DirectSoundDriver::ProcessEndOfBuffer(): Play() after restore, ", play2Result);
          }
        }
      }
    }

    // If the next buffer also triggered during WaitForData()
    // it will end multiple object wait immediately, since we don't reset it
    ResetEvent(_dataAvailable);
    SetEvent(_canAddData);
    ResetEvent(_notifications[currentBufferNo]);
  }
  else
  {
    terminateThread = true;
  }

  return terminateThread;
}

DWORD WINAPI DirectSoundDriver::ThreadProc(void* in)
{
  return ((DirectSoundDriver*)in)->HandleThreadProc();
}

DWORD DirectSoundDriver::HandleThreadProc()
{
  bool terminateThread = false;

  winDrvSetThreadName(-1, "DirectSoundDriver::ThreadProc()");

  while (!terminateThread)
  {
    // Thread is activated by 3 different events:
    // 0 - Play position is at end of buffer 0
    // 1 - Play position is at end of buffer 1
    // 2 - Thread must terminate (triggered in EmulationStop())
    DWORD dwEvt = WaitForMultipleObjects(3, (void* const*)(_notifications), FALSE, INFINITE);

    switch (dwEvt)
    {
    case WAIT_OBJECT_0 + 0: /* End of first buffer */
      // Wait for data_available event to become signaled
      // or FALSE is returned if (2) becomes signaled (end thread)
      terminateThread = ProcessEndOfBuffer(0, 1);
      break;
    case WAIT_OBJECT_0 + 1: /* End of first buffer */
      // Wait for data_available event to become signaled
      // or FALSE is returned if (2) becomes signaled (end thread)
      terminateThread = ProcessEndOfBuffer(1, 0);
      break;
    case WAIT_OBJECT_0 + 2: /* Emulation is ending */
    default: terminateThread = true; break;
    }
  }

  return 0;
}

void DirectSoundDriver::HardReset()
{
}

bool DirectSoundDriver::EmulationStart(SoundDriverRuntimeConfiguration runtimeConfiguration)
{
  _runtimeConfiguration = runtimeConfiguration;

  AcquireSoundMutex();

  // Set all events to their initial state

  for (unsigned int i = 0; i < 3; i++)
  {
    ResetEvent(_notifications[i]);
  }

  ResetEvent(_dataAvailable);
  SetEvent(_canAddData);

  // Check if the driver can support the requested sound quality

  auto currentMode = FindMode(_runtimeConfiguration.IsStereo, _runtimeConfiguration.Is16Bits, _runtimeConfiguration.ActualSampleRate);
  bool result = currentMode != nullptr;
  if (result)
  {
    _modeCurrent = *currentMode;
    _modeCurrent.BufferSampleCount = _runtimeConfiguration.MaximumBufferSampleCount;
  }

  // Record the number of samples in our buffer (ie. one half of the size)

  if (result)
  {
    result = DSoundSetCooperativeLevel();
  }

  // Create the needed buffer(s)

  if (result)
  {
    result = DSoundPlaybackInitialize();
  }

  // Start playback thread

  if (result)
  {
    _thread = CreateThread(
      nullptr,     // Security attr
      0,           // Stack Size
      ThreadProc,  // Thread procedure
      this,        // Thread parameter
      0,           // Creation flags
      &_threadId); // ThreadId

    result = _thread != nullptr;
  }

  // In case of failure, we undo any stuff we've done so far

  if (!result)
  {
    _core.Log->AddLog("Failed to start sound\n");
    DSoundPlaybackStop();
  }

  ReleaseSoundMutex();
  return result;
}

void DirectSoundDriver::EmulationStop()
{
  AcquireSoundMutex();
  SetEvent(_notifications[2]);
  WaitForSingleObject(_thread, INFINITE);
  CloseHandle(_thread);
  _thread = nullptr;
  DSoundPlaybackStop();
  ReleaseSoundMutex();
}

bool DirectSoundDriver::IsInitialized()
{
  return _isInitialized;
}

DirectSoundDriver::DirectSoundDriver() : ISoundDriver()
{
  _isInitialized = DSoundInitialize();
  if (!_isInitialized)
  {
    return;
  }

  _isInitialized = DSoundModeInformationInitialize();
  if (!_isInitialized)
  {
    DSoundRelease();
    return;
  }

  _mutex = CreateMutex(nullptr, 0, nullptr);
  _isInitialized = _mutex != nullptr;
  if (!_isInitialized)
  {
    DSoundModeInformationRelease();
    DSoundRelease();
  }
}

DirectSoundDriver::~DirectSoundDriver()
{
  DSoundModeInformationRelease();
  DSoundRelease();

  if (_mutex != nullptr)
  {
    CloseHandle(_mutex);
    _mutex = nullptr;
  }
}
