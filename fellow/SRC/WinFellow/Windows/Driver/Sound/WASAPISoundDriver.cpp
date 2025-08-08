/*=========================================================================*/
/* WinFellow                                                               */
/*                                                                         */
/* WASAPI sound driver                                                     */
/*                                                                         */
/* Author: Torsten Enderling                                               */
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
 *  WASAPI sound driver
 *
 *  Windows Audio Session API (WASAPI) sound driver implementation for WinFellow.
 *  Handles initialization, playback, and buffer management for audio output using WASAPI.
 *  Integrates with the Amiga emulation core, providing format conversion, ring buffering,
 *  and event-driven playback to ensure smooth and accurate audio output.
 */

#include "WASAPISoundDriver.h"
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <endpointvolume.h>
#include <comdef.h>
#include <thread>
#include <chrono>
#include "VirtualHost/Core.h"
#include <functiondiscoverykeys_devpkey.h>
#include <mmreg.h>
#include <ksmedia.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

#define WASAPI_FILLBUFFER_TESTMODE 0 // 1 = sine wave test mode, 0 = normal operation

/**
 * @brief Initializes the supported sound modes based on the WASAPI mix format.
 *        Adds typical emulation modes if they match the device's mix format.
 * @return true if at least one mode was added, false otherwise.
 */
bool WASAPISoundDriver::InitializeModeInformation()
{
  if (!_pwfx) return false;

  bool isStereo = (_pwfx->nChannels == 2);
  uint8_t bitsPerSample = _pwfx->wBitsPerSample;

  _core.Log->AddLog("WASAPISoundDriver: Supported WASAPI mix format: %u Hz, %s, %u bits per sample\n", _pwfx->nSamplesPerSec, isStereo ? "stereo" : "mono", bitsPerSample);

  const uint32_t rates[] = {15650, 22050, 31300, 44100};

  for (uint32_t rate : rates)
  {
    if (rate == _pwfx->nSamplesPerSec)
    {
      AddMode(isStereo, bitsPerSample, rate);
      _core.Log->AddLog("WASAPISoundDriver: Added mode: %u Hz, %s, %u bits per sample\n", rate, isStereo ? "stereo" : "mono", bitsPerSample);
    }
  }

  // Always add the mix format as a fallback if not already present
  if (FindMode(isStereo, bitsPerSample, _pwfx->nSamplesPerSec) == nullptr)
  {
    AddMode(isStereo, bitsPerSample, _pwfx->nSamplesPerSec);
    _core.Log->AddLog("WASAPISoundDriver: Added fallback mode: %u Hz, %s, %u bits per sample\n", _pwfx->nSamplesPerSec, isStereo ? "stereo" : "mono", bitsPerSample);
  }

  if (_modes.empty()) _core.Log->AddLog("WASAPISoundDriver: No supported sound modes found!\n");

  return !_modes.empty();
}

/**
 * @brief Constructor. Initializes WASAPI and supported sound modes.
 */
WASAPISoundDriver::WASAPISoundDriver() : ISoundDriver()
{
  _ringBufferSize = 0;
  _isInitialized = InitializeWASAPI();
  if (_isInitialized)
  {
    if (!InitializeModeInformation())
    {
      _core.Log->AddLog("WASAPISoundDriver: Mode information initialization failed!\n");
      _isInitialized = false;
    }
    else
    {
      _core.Log->AddLog("WASAPISoundDriver: Initialization successful.\n");
    }
  }
  else
  {
    _core.Log->AddLog("WASAPISoundDriver: Initialization failed!\n");
  }
}

/**
 * @brief Destructor. Releases all WASAPI resources.
 */
WASAPISoundDriver::~WASAPISoundDriver()
{
  EmulationStop();
  ReleaseWASAPI();
  _core.Log->AddLog("WASAPISoundDriver: Resources released.\n");
}

/**
 * @brief Initializes the WASAPI audio client and related resources.
 * @return true if initialization succeeded, false otherwise.
 */
bool WASAPISoundDriver::InitializeWASAPI()
{
  HRESULT hr;
  CoInitializeEx(nullptr, COINIT_MULTITHREADED);

  // Create the device enumerator if not already created
  if (!_deviceEnumerator)
  {
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void **)&_deviceEnumerator);
    if (FAILED(hr))
    {
      _core.Log->AddLog("WASAPISoundDriver: Failed to create IMMDeviceEnumerator (0x%08lx)\n", hr);
      return false;
    }
  }

  // Register for device notifications
  if (!_notificationClient)
  {
    _notificationClient = new DeviceNotificationClient(this);
    _deviceEnumerator->RegisterEndpointNotificationCallback(_notificationClient);
  }

  // Use the class member _deviceEnumerator instead of a local pEnumerator
  hr = _deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &_device);
  if (FAILED(hr))
  {
    _core.Log->AddLog("WASAPISoundDriver: No default audio endpoint found (0x%08lx)\n", hr);
    return false;
  }

  // Log the friendly name of the selected audio device
  {
    IPropertyStore *pProps = nullptr;
    hr = _device->OpenPropertyStore(STGM_READ, &pProps);
    if (SUCCEEDED(hr) && pProps)
    {
      PROPVARIANT varName;
      PropVariantInit(&varName);
      hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
      if (SUCCEEDED(hr))
      {
        _core.Log->AddLog("WASAPISoundDriver: Using audio device: %ws\n", varName.pwszVal);
        PropVariantClear(&varName);
      }
      else
      {
        _core.Log->AddLog("WASAPISoundDriver: Could not get device friendly name (0x%08lx)\n", hr);
      }
      pProps->Release();
    }
    else
    {
      _core.Log->AddLog("WASAPISoundDriver: Could not open property store for device (0x%08lx)\n", hr);
    }
  }

  hr = _device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void **)&_audioClient);
  if (FAILED(hr))
  {
    _core.Log->AddLog("WASAPISoundDriver: Failed to activate IAudioClient (0x%08lx)\n", hr);
    return false;
  }

  hr = _audioClient->GetMixFormat(&_pwfx);
  if (FAILED(hr))
  {
    _core.Log->AddLog("WASAPISoundDriver: GetMixFormat failed (0x%08lx)\n", hr);
    return false;
  }

  _core.Log->AddLog(
      "WASAPISoundDriver: wFormatTag=%u, nChannels=%u, wBitsPerSample=%u, nBlockAlign=%u, nSamplesPerSec=%u\n",
      _pwfx->wFormatTag,
      _pwfx->nChannels,
      _pwfx->wBitsPerSample,
      _pwfx->nBlockAlign,
      _pwfx->nSamplesPerSec);

  REFERENCE_TIME hnsBufferDuration = 1000000; // 100ms
  hr = _audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, hnsBufferDuration, 0, _pwfx, nullptr);
  if (FAILED(hr))
  {
    _core.Log->AddLog("WASAPISoundDriver: AudioClient initialization failed (0x%08lx)\n", hr);
    return false;
  }

  hr = _audioClient->GetService(__uuidof(IAudioRenderClient), (void **)&_renderClient);
  if (FAILED(hr))
  {
    _core.Log->AddLog("WASAPISoundDriver: Failed to get IAudioRenderClient (0x%08lx)\n", hr);
    return false;
  }

  _eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (!_eventHandle)
  {
    _core.Log->AddLog("WASAPISoundDriver: Failed to create event handle.\n");
    return false;
  }

  hr = _audioClient->SetEventHandle(_eventHandle);
  if (FAILED(hr))
  {
    _core.Log->AddLog("WASAPISoundDriver: SetEventHandle failed (0x%08lx)\n", hr);
    return false;
  }

  _mutex = CreateMutex(nullptr, FALSE, nullptr);
  if (!_mutex)
  {
    _core.Log->AddLog("WASAPISoundDriver: Failed to create mutex.\n");
    return false;
  }

  _core.Log->AddLog("WASAPISoundDriver: Initialization complete.\n");
  _isInitialized = true;
  return true;
}

/**
 * @brief Releases all WASAPI and related resources.
 */
void WASAPISoundDriver::ReleaseWASAPI()
{
  if (_audioClient)
  {
    _audioClient->Stop();
  }
  if (_thread)
  {
    CloseHandle(_thread);
    _thread = nullptr;
  }
  if (_eventHandle)
  {
    CloseHandle(_eventHandle);
    _eventHandle = nullptr;
  }
  if (_mutex)
  {
    CloseHandle(_mutex);
    _mutex = nullptr;
  }
  if (_renderClient)
  {
    _renderClient->Release();
    _renderClient = nullptr;
  }
  if (_audioClient)
  {
    _audioClient->Release();
    _audioClient = nullptr;
  }
  if (_device)
  {
    _device->Release();
    _device = nullptr;
  }
  if (_pwfx)
  {
    CoTaskMemFree(_pwfx);
    _pwfx = nullptr;
  }

  // Unregister for device change notifications
  if (_deviceEnumerator && _notificationClient)
  {
    _deviceEnumerator->UnregisterEndpointNotificationCallback(_notificationClient);
    _notificationClient->Release();
    _notificationClient = nullptr;
  }
  if (_deviceEnumerator)
  {
    _deviceEnumerator->Release();
    _deviceEnumerator = nullptr;
  }

  // Free all WASAPISoundMode objects
  for (auto mode : _modes)
    delete mode;
  _modes.clear();

  CoUninitialize();
  _isInitialized = false;
}

/**
 * @brief Finds a matching sound mode.
 * @param isStereo True for stereo, false for mono.
 * @param bitsPerSample Bits per sample (8, 16, 32).
 * @param rate Sample rate in Hz.
 * @return Pointer to the mode or nullptr if not found.
 */
const WASAPISoundMode *WASAPISoundDriver::FindMode(bool isStereo, uint8_t bitsPerSample, uint32_t rate) const
{
  for (const auto *mode : _modes)
  {
    if (mode->Rate == rate && mode->BitsPerSample == bitsPerSample && mode->IsStereo == isStereo)
    {
      return mode;
    }
  }
  return nullptr;
}

/**
 * @brief Acquires the sound mutex for thread safety.
 */
void WASAPISoundDriver::AcquireSoundMutex()
{
  WaitForSingleObject(_mutex, INFINITE);
}

/**
 * @brief Releases the sound mutex.
 */
void WASAPISoundDriver::ReleaseSoundMutex()
{
  ReleaseMutex(_mutex);
}

/**
 * @brief Fills the WASAPI buffer with audio data from the ring buffer, converting to the target mix format if needed.
 *        Performs sample rate, bit depth, and mono/stereo conversion as required.
 *        If insufficient data is available, fills the remainder with silence.
 * @param pData Pointer to the buffer to fill.
 * @param frames Number of audio frames to fill.
 */
void WASAPISoundDriver::FillBuffer(BYTE *pData, uint32_t frames)
{
  // Only proceed if ring buffer is initialized
  if (_ringBufferSize == 0 || _ringBufferLeft.empty() || _ringBufferRight.empty())
  {
    memset(pData, 0, frames * _pwfx->nBlockAlign);
    return;
  }

  uint32_t framesFilled = 0;

  if (_pwfx->wBitsPerSample == 32)
  {
    bool isFloat =
        (_pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT || _pwfx->wFormatTag == 3 ||
         (_pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE && ((WAVEFORMATEXTENSIBLE *)_pwfx)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT));
    if (isFloat)
    {
      float *out = reinterpret_cast<float *>(pData);
      for (; framesFilled < frames; ++framesFilled)
      {
        if (_ringReadPos != _ringWritePos)
        {
          float l = _ringBufferLeft[_ringReadPos] / 32768.0f;
          float r = _ringBufferRight[_ringReadPos] / 32768.0f;
          if (_pwfx->nChannels == 2)
          {
            *out++ = l;
            *out++ = r;
          }
          else
          {
            *out++ = l;
          }
          _ringReadPos = (_ringReadPos + 1) % _ringBufferSize;
        }
        else
        {
          // Buffer underrun detected
          _core.Log->AddLog("WASAPISoundDriver: Buffer underrun detected! Outputting silence.\n");
          break;
        }
      }
      // Zero remaining frames if not enough data
      if (framesFilled < frames)
      {
        memset(out, 0, (frames - framesFilled) * _pwfx->nBlockAlign);
      }
    }
    else
    {
      int32_t *out = reinterpret_cast<int32_t *>(pData);
      for (; framesFilled < frames; ++framesFilled)
      {
        if (_ringReadPos != _ringWritePos)
        {
          int32_t l = _ringBufferLeft[_ringReadPos] << 16;
          int32_t r = _ringBufferRight[_ringReadPos] << 16;
          if (_pwfx->nChannels == 2)
          {
            *out++ = l;
            *out++ = r;
          }
          else
          {
            *out++ = l;
          }
          _ringReadPos = (_ringReadPos + 1) % _ringBufferSize;
        }
        else
        {
          // Buffer underrun detected
          _core.Log->AddLog("WASAPISoundDriver: Buffer underrun detected! Outputting silence.\n");
          break;
        }
      }
      if (framesFilled < frames)
      {
        memset(out, 0, (frames - framesFilled) * _pwfx->nBlockAlign);
      }
    }
  }
  else if (_pwfx->wBitsPerSample == 16)
  {
    int16_t *out = reinterpret_cast<int16_t *>(pData);
    for (; framesFilled < frames; ++framesFilled)
    {
      if (_ringReadPos != _ringWritePos)
      {
        int16_t l = _ringBufferLeft[_ringReadPos];
        int16_t r = _ringBufferRight[_ringReadPos];
        if (_pwfx->nChannels == 2)
        {
          *out++ = l;
          *out++ = r;
        }
        else
        {
          *out++ = l;
        }
        _ringReadPos = (_ringReadPos + 1) % _ringBufferSize;
      }
      else
      {
        // Buffer underrun detected
        _core.Log->AddLog("WASAPISoundDriver: Buffer underrun detected! Outputting silence.\n");
        break;
      }
    }
    if (framesFilled < frames)
    {
      memset(out, 0, (frames - framesFilled) * _pwfx->nBlockAlign);
    }
  }
  else
  {
    memset(pData, 0, frames * _pwfx->nBlockAlign);
  }
}

/**
 * @brief Static thread entry point for WASAPI playback.
 * @param in Pointer to the WASAPISoundDriver instance.
 * @return Thread exit code.
 */
DWORD WINAPI WASAPISoundDriver::ThreadProc(void *in)
{
  return ((WASAPISoundDriver *)in)->HandleThreadProc();
}

/**
 * @brief Playback thread procedure. Handles buffer events and audio streaming.
 * @return Thread exit code.
 */
DWORD WASAPISoundDriver::HandleThreadProc()
{
  _audioClient->Start();
  UINT32 bufferFrameCount = 0;
  _audioClient->GetBufferSize(&bufferFrameCount);

  _core.Log->AddLog("WASAPISoundDriver: Playback thread started.\n");

  while (_running)
  {
    WaitForSingleObject(_eventHandle, INFINITE);

    UINT32 numFramesPadding = 0;
    _audioClient->GetCurrentPadding(&numFramesPadding);
    UINT32 numFramesAvailable = bufferFrameCount - numFramesPadding;

    if (numFramesAvailable == 0) continue;

    BYTE *pData = nullptr;
    HRESULT hr = _renderClient->GetBuffer(numFramesAvailable, &pData);
    if (SUCCEEDED(hr))
    {
      AcquireSoundMutex();
      FillBuffer(pData, numFramesAvailable);
      ReleaseSoundMutex();
#ifdef _DEBUG
      // _core.Log->AddLog("WASAPISoundDriver: Released %u frames to WASAPI\n", numFramesAvailable);
#endif
      _renderClient->ReleaseBuffer(numFramesAvailable, 0);
    }
    else
    {
      _core.Log->AddLog("WASAPISoundDriver: GetBuffer failed (0x%08lx)\n", hr);
    }
  }
  _audioClient->Stop();
  _core.Log->AddLog("WASAPISoundDriver: Playback thread stopped.\n");
  return 0;
}

/**
 * @brief Passes audio data from the emulation core to the driver for playback.
 *        Copies the provided left and right channel samples into the ring buffer.
 *        Also logs format conversion details for debugging.
 * @param left Pointer to left channel data.
 * @param right Pointer to right channel data.
 * @param sampleCount Number of samples per channel.
 */
void WASAPISoundDriver::Play(int16_t *left, int16_t *right, uint32_t sampleCount)
{
  AcquireSoundMutex();
  _pendingDataLeft = (uint16_t *)left;
  _pendingDataRight = (uint16_t *)right;
  _pendingDataSampleCount = sampleCount;

#ifdef _DEBUG
  // Format mismatch detection between source buffer and WASAPI mix format
  // Assumption: int16_t* means 16 bits per sample, left+right != nullptr means stereo
  uint8_t expectedBits = _pwfx ? _pwfx->wBitsPerSample : 0;
  uint8_t expectedChannels = _pwfx ? _pwfx->nChannels : 0;
  uint8_t actualBits = 16;
  uint8_t actualChannels = (left && right) ? 2 : 1;

  bool mismatch = false;
  std::string mismatchMsg;

  // Compare bits per sample
  if (expectedBits != actualBits)
  {
    mismatch = true;
    mismatchMsg += "BitsPerSample: expected " + std::to_string(expectedBits) + ", got " + std::to_string(actualBits) + ". ";
  }
  // Compare channel count
  if (expectedChannels != actualChannels)
  {
    mismatch = true;
    mismatchMsg += "Channels: expected " + std::to_string(expectedChannels) + ", got " + std::to_string(actualChannels) + ". ";
  }

  // Log only if a mismatch is detected
  if (mismatch)
  {
    // Only log if the format is not handled by FillBuffer
    if ((expectedBits != 16 && expectedBits != 32) || (expectedChannels != 1 && expectedChannels != 2))
    {
      _core.Log->AddLog("WASAPISoundDriver: UNHANDLED FORMAT MISMATCH! %s\n", mismatchMsg.c_str());
    }
  }
#endif

  // Copy samples into the ring buffer for WASAPI consumption
  for (uint32_t i = 0; i < sampleCount; ++i)
  {
    _ringBufferLeft[_ringWritePos] = left[i];
    _ringBufferRight[_ringWritePos] = right[i];
    _ringWritePos = (_ringWritePos + 1) % _ringBufferSize;
  }

#ifdef _DEBUG
  if (left && right)
  {
    _core.Log->AddLog(
        "WASAPISoundDriver::Play: L=%d %d %d %d %d %d %d %d | R=%d %d %d %d %d %d %d %d (Samples: %u)\n",
        left[0],
        left[1],
        left[2],
        left[3],
        left[4],
        left[5],
        left[6],
        left[7],
        right[0],
        right[1],
        right[2],
        right[3],
        right[4],
        right[5],
        right[6],
        right[7],
        sampleCount);
  }

  _core.Log->AddLog("WASAPISoundDriver: Play called, sampleCount=%u\n", sampleCount);
#endif
  ReleaseSoundMutex();
}

/**
 * @brief Not required for WASAPI event-driven playback.
 */
void WASAPISoundDriver::PollBufferPosition()
{
  // Not required for WASAPI event loopback
}

/**
 * @brief Sets the output device volume.
 * @param volume Volume level (0-100).
 * @return true if successful, false otherwise.
 */
bool WASAPISoundDriver::SetCurrentSoundDeviceVolume(int volume)
{
  // volume: 0-100
  if (!_device) return false;
  IAudioEndpointVolume *endpointVolume = nullptr;
  HRESULT hr = _device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void **)&endpointVolume);
  if (FAILED(hr) || !endpointVolume)
  {
    _core.Log->AddLog("WASAPISoundDriver: Failed to activate IAudioEndpointVolume (0x%08lx)\n", hr);
    return false;
  }
  float vol = (float)volume / 100.0f;
  hr = endpointVolume->SetMasterVolumeLevelScalar(vol, nullptr);
  endpointVolume->Release();
  if (FAILED(hr))
  {
    _core.Log->AddLog("WASAPISoundDriver: SetMasterVolumeLevelScalar failed (0x%08lx)\n", hr);
    return false;
  }
  _core.Log->AddLog("WASAPISoundDriver: Volume set to %d%%.\n", volume);
  return true;
}

/**
 * @brief Resets the driver state.
 */
void WASAPISoundDriver::HardReset()
{
  _core.Log->AddLog("WASAPISoundDriver: HardReset called.\n");
}

/**
 * @brief Starts audio emulation with the given configuration.
 *        Initializes the ring buffer and starts the playback thread.
 * @param runtimeConfiguration The runtime configuration for sound.
 * @return true if successful, false otherwise.
 */
bool WASAPISoundDriver::EmulationStart(SoundDriverRuntimeConfiguration runtimeConfiguration)
{
  // Force WASAPI mix format
  runtimeConfiguration.ActualSampleRate = _pwfx->nSamplesPerSec;
  runtimeConfiguration.IsStereo = (_pwfx->nChannels == 2);
  runtimeConfiguration.Is16Bits = (_pwfx->wBitsPerSample == 16);

  _runtimeConfiguration = runtimeConfiguration;

  _core.Log->AddLog(
      "WASAPISoundDriver: EmulationStart (forced WASAPI mix format): %u Hz, %s, %s\n",
      runtimeConfiguration.ActualSampleRate,
      runtimeConfiguration.IsStereo ? "stereo" : "mono",
      runtimeConfiguration.Is16Bits ? "16bit" : "32bit");

  // Find mode matching the mix format
  auto currentMode = FindMode(runtimeConfiguration.IsStereo, runtimeConfiguration.Is16Bits ? 16 : 32, runtimeConfiguration.ActualSampleRate);

  if (!currentMode)
  {
    _core.Log->AddLog("WASAPISoundDriver: No suitable mode found for WASAPI mix format. Available modes:\n");
    for (const auto *mode : _modes)
    {
      _core.Log->AddLog(
          "WASAPISoundDriver: Available mode: %u Hz, %s, %s\n",
          mode->Rate,
          mode->IsStereo ? "stereo" : "mono",
          mode->BitsPerSample == 32 ? "32bit" : (mode->BitsPerSample == 16 ? "16bit" : "8bit"));
    }
    return false;
  }

  _core.Log->AddLog(
      "WASAPISoundDriver: Using mode: %u Hz, %s, %s\n",
      currentMode->Rate,
      currentMode->IsStereo ? "stereo" : "mono",
      currentMode->BitsPerSample == 32 ? "32bit" : (currentMode->BitsPerSample == 16 ? "16bit" : "8bit"));

  _modeCurrent = *currentMode;
  _modeCurrent.BufferSampleCount = _runtimeConfiguration.MaximumBufferSampleCount;

  _ringBufferSize = 8 * _modeCurrent.BufferSampleCount; // e.g., 8x emulation buffer
  _ringBufferLeft.resize(_ringBufferSize);
  _ringBufferRight.resize(_ringBufferSize);
  _ringReadPos = _ringWritePos = 0;

  _running = true;
  _thread = CreateThread(nullptr, 0, ThreadProc, this, 0, nullptr);
  if (_thread)
  {
    _core.Log->AddLog("WASAPISoundDriver: EmulationStart successful.\n");
    SetCurrentSoundDeviceVolume(_runtimeConfiguration.Volume);
    return true;
  }
  else
  {
    _core.Log->AddLog("WASAPISoundDriver: EmulationStart failed (could not create thread).\n");
    return false;
  }
}

/**
 * @brief Stops audio emulation and playback thread.
 */
void WASAPISoundDriver::EmulationStop()
{
  _running = false;
  if (_thread)
  {
    SetEvent(_eventHandle);
    WaitForSingleObject(_thread, INFINITE);
    CloseHandle(_thread);
    _thread = nullptr;
    _core.Log->AddLog("WASAPISoundDriver: EmulationStop, thread stopped.\n");
  }
}

/**
 * @brief Returns whether the driver is initialized.
 * @return true if initialized, false otherwise.
 */
bool WASAPISoundDriver::IsInitialized()
{
  return _isInitialized;
}

/**
 * @brief Adds a supported sound mode to the list of available modes.
 * @param isStereo True for stereo, false for mono.
 * @param bitsPerSample Bits per sample (8, 16, 32).
 * @param rate Sample rate in Hz.
 */
void WASAPISoundDriver::AddMode(bool isStereo, uint8_t bitsPerSample, uint32_t rate)
{
  auto mode = new WASAPISoundMode();
  mode->Rate = rate;
  mode->BitsPerSample = bitsPerSample;
  mode->IsStereo = isStereo;
  mode->BufferSampleCount = 0;
  mode->BufferBlockAlign = 0;
  _modes.emplace_back(mode);
}

void WASAPISoundDriver::OnDefaultDeviceChanged()
{
  _core.Log->AddLog("WASAPISoundDriver: Default playback device changed, reinitializing WASAPI.\n");
  // Stop emulation and release WASAPI resources
  EmulationStop();
  ReleaseWASAPI();
  // Re-initialize WASAPI and restart emulation if needed
  if (InitializeWASAPI() && InitializeModeInformation())
  {
    _core.Log->AddLog("WASAPISoundDriver: WASAPI reinitialized after device change.\n");
    // Restart emulation if it was running
    EmulationStart(_runtimeConfiguration);
  }
  else
  {
    _core.Log->AddLog("WASAPISoundDriver: Failed to reinitialize WASAPI after device change.\n");
  }
}
