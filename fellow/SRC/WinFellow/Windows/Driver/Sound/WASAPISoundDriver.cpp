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

/**
 * @file WASAPISoundDriver.cpp
 * @brief Implementation of the Windows Audio Session API (WASAPI) sound driver for WinFellow.
 *
 * This module provides a WASAPI-based sound driver that integrates with the WinFellow sound subsystem via the ISoundDriver interface.
 * It is responsible for initializing the Windows audio stack, managing audio buffers, and streaming emulation audio to the host system.
 *
 * ## Integration with Sound Driver Architecture
 *
 * WASAPISoundDriver is a concrete implementation of ISoundDriver, allowing WinFellow to select WASAPI as the audio backend.
 * The driver exposes methods for initialization, playback, buffer management, and device control, and interacts with the emulation core through these interfaces.
 * Supported sound modes are determined based on the WASAPI mix format and typical emulation rates, and are stored internally for runtime selection.
 *
 * ## WASAPI Driver Operation
 *
 * - **Initialization**: The driver sets up COM in the main thread (apartment-threaded), enumerates audio devices, and activates the default render endpoint.
 * It queries the mix format and configures the audio client for shared-mode streaming with event-driven notifications. Device change notifications are registered
 * to handle hotplug and default device changes. The main thread uses apartment-threaded COM, which allows the RetroPlatform host (which requires single-threaded COM)
 * to operate without conflicts.
 *
 * - **Playback Thread**: A dedicated thread is created for audio streaming. This thread initializes its own COM context (apartment-threaded) and waits for
 * WASAPI event notifications. It acquires the render buffer, fills it with audio data from the ring buffer, and manages format conversion as needed
 * (bit depth, channel count, float/integer). The thread primes the buffer on startup and signals producers when buffer space is available. On thread exit,
 * COM is properly uninitialized, keeping the main thread's COM context separate.
 *
 * - **Buffer Management**: Audio samples from the emulation core are written to a ring buffer. The buffer size is chosen to provide headroom for timing variations. If the
 * emulation sample rate does not match the WASAPI mix rate, linear interpolation is used for resampling.
 *
 * - **Audio Data Flow**: The Play() method receives left and right channel samples from the emulation core and writes them to the ring buffer, with optional resampling. The
 * playback thread consumes samples, converts them to the required format, and passes them to WASAPI. Buffer underruns are logged and silence is output if necessary.
 *
 * - **Device and Volume Control**: The driver supports device changes by reinitializing WASAPI and restarting emulation. Volume can be set via the IAudioEndpointVolume
 * interface if supported. Device notifications ensure the driver responds to changes in the default audio endpoint.
 *
 * - **Shutdown**: On destruction or emulation stop, the driver releases all WASAPI resources, stops playback, cleans up synchronization objects and buffers,
 * and uninitializes COM in both the main and playback threads.
 *
 * ## Design Notes
 *
 * - Uses separate COM contexts: main thread for initialization/device enumeration, playback thread for audio streaming.
 * - Both threads use apartment-threaded COM (COINIT_APARTMENTTHREADED) for compatibility with the RetroPlatform host.
 * - Handles format conversion and resampling between emulation output and WASAPI mix format.
 * - Employs mutexes and events for thread safety between main and playback threads.
 * - Supports device hotplug and volume control where available.
 * - Provides logging for diagnostics.
 *
 * The WASAPISoundDriver is selected and managed by the WinFellow sound subsystem, providing audio output compatible with modern Windows systems
 * while allowing RetroPlatform to operate without conflicts.
 */

#include "WASAPISoundDriver.h"
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <endpointvolume.h>
#include "VirtualHost/Core.h"
#include <functiondiscoverykeys_devpkey.h>
#include <mmreg.h>
#include <ksmedia.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

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
  // Initialize COM in the main thread (apartment-threaded) for device enumeration.
  // The playback thread will have its own COM initialization.
  if (!_comInitializedInMainThread)
  {
    hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
    {
      _comInitializedInMainThread = true;
      _core.Log->AddLog("WASAPISoundDriver: COM initialized in main thread with COINIT_APARTMENTTHREADED.\n");
    }
    else
    {
      _core.Log->AddLog("WASAPISoundDriver: Failed to initialize COM in main thread (0x%08lx)\n", hr);
      // Continue anyway; might already be initialized
    }
  }

  // Create the device enumerator if not already created
  if (!_deviceEnumerator)
  {
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void **)&_deviceEnumerator);
    if (FAILED(hr))
    {
      _core.Log->AddLog("WASAPISoundDriver: Failed to create IMMDeviceEnumerator (0x%08lx)\n", hr);
      ReleaseWASAPI();
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
    ReleaseWASAPI();
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
    ReleaseWASAPI();
    return false;
  }

  hr = _audioClient->GetMixFormat(&_pwfx);
  if (FAILED(hr))
  {
    _core.Log->AddLog("WASAPISoundDriver: GetMixFormat failed (0x%08lx)\n", hr);
    ReleaseWASAPI();
    return false;
  }

  _core.Log->AddLog(
      "WASAPISoundDriver: wFormatTag=%u, nChannels=%u, wBitsPerSample=%u, nBlockAlign=%u, nSamplesPerSec=%u\n",
      _pwfx->wFormatTag,
      _pwfx->nChannels,
      _pwfx->wBitsPerSample,
      _pwfx->nBlockAlign,
      _pwfx->nSamplesPerSec);

  // Initialize audio client with a temporary buffer duration; will be reconfigured in EmulationStart
  REFERENCE_TIME hnsBufferDuration = 1000000; // Default 100ms, will be updated in EmulationStart
  hr = _audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, hnsBufferDuration, 0, _pwfx, nullptr);
  if (FAILED(hr))
  {
    _core.Log->AddLog("WASAPISoundDriver: AudioClient initialization failed (0x%08lx)\n", hr);
    ReleaseWASAPI();
    return false;
  }

  hr = _audioClient->GetService(__uuidof(IAudioRenderClient), (void **)&_renderClient);
  if (FAILED(hr))
  {
    _core.Log->AddLog("WASAPISoundDriver: Failed to get IAudioRenderClient (0x%08lx)\n", hr);
    ReleaseWASAPI();
    return false;
  }

  _eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (!_eventHandle)
  {
    _core.Log->AddLog("WASAPISoundDriver: Failed to create event handle.\n");
    ReleaseWASAPI();
    return false;
  }

  hr = _audioClient->SetEventHandle(_eventHandle);
  if (FAILED(hr))
  {
    _core.Log->AddLog("WASAPISoundDriver: SetEventHandle failed (0x%08lx)\n", hr);
    ReleaseWASAPI();
    return false;
  }

  _mutex = CreateMutex(nullptr, FALSE, nullptr);
  if (!_mutex)
  {
    _core.Log->AddLog("WASAPISoundDriver: Failed to create mutex.\n");
    ReleaseWASAPI();
    return false;
  }

  // Event to notify producer that space is available in ring buffer
  _canAddData = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (!_canAddData)
  {
    _core.Log->AddLog("WASAPISoundDriver: Failed to create canAddData event.\n");
    // not fatal, continue without it
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
  if (_canAddData)
  {
    CloseHandle(_canAddData);
    _canAddData = nullptr;
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

  // Uninitialize COM in main thread if it was initialized by this driver
  if (_comInitializedInMainThread)
  {
    CoUninitialize();
    _comInitializedInMainThread = false;
    _core.Log->AddLog("WASAPISoundDriver: COM uninitialized in main thread.\n");
  }

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
 *
 * @param[in] in Pointer to the WASAPISoundDriver instance.
 * @return Thread exit code.
 */
DWORD WINAPI WASAPISoundDriver::ThreadProc(void *in)
{
  return ((WASAPISoundDriver *)in)->HandleThreadProc();
}

/**
 * @brief Playback thread procedure. Handles buffer events and audio streaming.
 *
 * This thread waits for WASAPI event notifications, obtains the render buffer
 * and calls FillBuffer() to copy audio data from the internal ring buffer
 * into the device buffer. It also primes the buffer on startup to avoid
 * initial underruns and signals producers when there is space available.
 *
 * @return Thread exit code (0 on normal termination).
 */
DWORD WASAPISoundDriver::HandleThreadProc()
{
  // Initialize COM for this thread with single-threaded apartment model
  HRESULT comInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (FAILED(comInit))
  {
    _core.Log->AddLog("WASAPISoundDriver: Failed to initialize COM in playback thread (0x%08lx)\n", comInit);
    return 1;
  }

  _core.Log->AddLog("WASAPISoundDriver: COM initialized in playback thread with COINIT_APARTMENTTHREADED.\n");

  UINT32 bufferFrameCount = 0;
  _audioClient->GetBufferSize(&bufferFrameCount);

  // Prime the buffer: fill it once before starting playback to avoid initial underrun
  {
    HRESULT hr;
    BYTE *pData = nullptr;
    UINT32 numFramesPadding = 0;
    _audioClient->GetCurrentPadding(&numFramesPadding);
    UINT32 numFramesAvailable = bufferFrameCount - numFramesPadding;
    if (numFramesAvailable > 0)
    {
      hr = _renderClient->GetBuffer(numFramesAvailable, &pData);
      if (SUCCEEDED(hr))
      {
        AcquireSoundMutex();
        FillBuffer(pData, numFramesAvailable);
        ReleaseSoundMutex();
        _renderClient->ReleaseBuffer(numFramesAvailable, 0);
      }
    }
  }

  if (FAILED(_audioClient->Start()))
  {
    _core.Log->AddLog("WASAPISoundDriver: Failed to start audio client.\n");
  }

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

      // signal producers that space may be available now
      if (_canAddData) SetEvent(_canAddData);

      _renderClient->ReleaseBuffer(numFramesAvailable, 0);
    }
    else
    {
      _core.Log->AddLog("WASAPISoundDriver: GetBuffer failed (0x%08lx)\n", hr);
    }
  }
  _audioClient->Stop();
  // signal producers to wake if waiting
  if (_canAddData) SetEvent(_canAddData);
  _core.Log->AddLog("WASAPISoundDriver: Playback thread stopped.\n");

  // Uninitialize COM for this thread
  CoUninitialize();
  _core.Log->AddLog("WASAPISoundDriver: COM uninitialized in playback thread.\n");

  return 0;
}

/**
 * @brief Passes audio data from the emulation core to the driver for playback.
 *
 * Copies the provided left and right channel samples into the driver's ring
 * buffer. If resampling is required the data is linearly interpolated to the
 * mix sample rate. This function is thread-safe (it acquires the internal
 * mutex while writing to the ring buffer).
 *
 * @param[in] left Pointer to left channel samples.
 * @param[in] right Pointer to right channel samples.
 * @param[in] sampleCount Number of samples per channel provided in the arrays.
 */
void WASAPISoundDriver::Play(int16_t *left, int16_t *right, uint32_t sampleCount)
{
  if (!left || !right || sampleCount == 0) return;

  AcquireSoundMutex();

  // If resampling is not required, copy samples directly (but do not overflow ring buffer)
  if (!_needResample)
  {
    for (uint32_t i = 0; i < sampleCount; ++i)
    {
      size_t next = (_ringWritePos + 1) % _ringBufferSize;
      // If buffer full, stop writing to avoid overwrite. This can happen if emulator runs faster than consumer.
      if (next == _ringReadPos)
      {
        // buffer full, wait briefly for space to become available
        Sleep(1);
        next = (_ringWritePos + 1) % _ringBufferSize;
        // check again, if still full, just drop the sample
        if (next == _ringReadPos)
        {
          break;
        }
      }
      _ringBufferLeft[_ringWritePos] = left[i];
      _ringBufferRight[_ringWritePos] = right[i];
      _ringWritePos = next;
    }
  }
  else
  {
    // Linear resampling: source = emulator samples, target = mix samples
    // We step through source samples and produce floor(sampleCount * ratio) samples into ring
    // Use _resampleSrcPos to carry fractional position across calls

    // Save last input sample for interpolation continuity
    int16_t prevL = _lastInputLeft;
    int16_t prevR = _lastInputRight;
    if (sampleCount > 0)
    {
      prevL = left[0];
      prevR = right[0];
    }

    for (uint32_t si = 0; si < sampleCount; ++si)
    {
      // push current source sample for interpolation usage
      int16_t srcL = left[si];
      int16_t srcR = right[si];

      // while we need to produce output samples that map to this source interval
      while (_resampleSrcPos <= 1.0)
      {
        // fractional interpolation between prev and src
        double t = _resampleSrcPos;
        int16_t outL = static_cast<int16_t>((1.0 - t) * prevL + t * srcL);
        int16_t outR = static_cast<int16_t>((1.0 - t) * prevR + t * srcR);

        size_t next = (_ringWritePos + 1) % _ringBufferSize;
        if (next == _ringReadPos)
        {
          // buffer full, stop producing
          break;
        }
        _ringBufferLeft[_ringWritePos] = outL;
        _ringBufferRight[_ringWritePos] = outR;
        _ringWritePos = next;

        _resampleSrcPos += (1.0 / _resampleRatio);
      }

      // advance to next source sample interval
      _resampleSrcPos -= 1.0; // bring into [0,1] for the next source interval
      prevL = srcL;
      prevR = srcR;
    }

    // store last input sample for next call continuity
    _lastInputLeft = left[sampleCount - 1];
    _lastInputRight = right[sampleCount - 1];
  }

  ReleaseSoundMutex();

  // Signal that new data is available
  SetEvent(_canAddData);
}

/**
 * @brief Initializes the driver state and prepares for audio emulation.
 *
 * This includes setting up the ring buffer, starting the playback thread,
 * and configuring resampling if the emulator sample rate differs from the
 * WASAPI mix rate.
 *
 * @param[in] runtimeConfiguration The desired runtime configuration for the sound driver.
 * @return true if the emulation was successfully started, false otherwise.
 */
bool WASAPISoundDriver::EmulationStart(SoundDriverRuntimeConfiguration runtimeConfiguration)
{
  // Do not force mix format; honor emulator requested rate but setup resampling if needed
  _runtimeConfiguration = runtimeConfiguration;

  uint32_t mixRate = _pwfx ? _pwfx->nSamplesPerSec : runtimeConfiguration.ActualSampleRate;
  bool mixStereo = _pwfx ? (_pwfx->nChannels == 2) : runtimeConfiguration.IsStereo;
  bool mix16 = _pwfx ? (_pwfx->wBitsPerSample == 16) : runtimeConfiguration.Is16Bits;

  // Determine if resampling is required
  _needResample = (runtimeConfiguration.ActualSampleRate != mixRate);
  if (_needResample)
  {
    _resampleRatio = static_cast<double>(mixRate) / static_cast<double>(runtimeConfiguration.ActualSampleRate);
    _resampleSrcPos = 0.0;
    _core.Log->AddLog("WASAPISoundDriver: Resampling enabled: emulator %u Hz -> mix %u Hz, ratio=%f\n", runtimeConfiguration.ActualSampleRate, mixRate, _resampleRatio);
  }
  else
  {
    _resampleRatio = 1.0;
    _resampleSrcPos = 0.0;
  }

  // Keep the rest of the startup as before but pick a mode matching the mix format if possible
  auto currentMode = FindMode(mixStereo, mix16 ? 16 : 32, mixRate);
  if (!currentMode)
  {
    _core.Log->AddLog("WASAPISoundDriver: No suitable mode found for mix format. Aborting start.\n");
    return false;
  }

  _modeCurrent = *currentMode;
  _modeCurrent.BufferSampleCount = _runtimeConfiguration.MaximumBufferSampleCount;

  // Log the configured buffer settings
  _core.Log->AddLog("WASAPISoundDriver: Configured buffer settings: %u samples at %u Hz = %.2f ms\n",
      _runtimeConfiguration.MaximumBufferSampleCount,
      runtimeConfiguration.ActualSampleRate,
      (static_cast<double>(_runtimeConfiguration.MaximumBufferSampleCount) / static_cast<double>(runtimeConfiguration.ActualSampleRate)) * 1000.0);

  // Grow ring buffer to hold more headroom: 4x emulation buffer to tolerate jitter
  _ringBufferSize = static_cast<size_t>(4u * _modeCurrent.BufferSampleCount);
  if (_ringBufferSize < 1024) _ringBufferSize = 1024;
  _ringBufferLeft.assign(_ringBufferSize, 0);
  _ringBufferRight.assign(_ringBufferSize, 0);
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
 *
 * @return true if initialized, false otherwise.
 */
bool WASAPISoundDriver::IsInitialized()
{
  return _isInitialized;
}

/**
 * @brief Adds a supported sound mode to the list of available modes.
 *
 * @param[in] isStereo True for stereo, false for mono.
 * @param[in] bitsPerSample Bits per sample (8,16,32).
 * @param[in] rate Sample rate in Hz.
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

/**
 * @brief Handle device change by reinitializing WASAPI and restarting emulation.
 */
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

/**
 * @brief Check whether the ring buffer can accept a number of samples.
 *
 * @param[in] sampleCount Number of samples to test for.
 * @return true if the ring buffer has space for sampleCount samples; false otherwise.
 */
bool WASAPISoundDriver::CanAcceptSamples(uint32_t sampleCount)
{
  // Calculate available space in ring buffer
  size_t used = (_ringWritePos >= _ringReadPos) ? (_ringWritePos - _ringReadPos) : (_ringBufferSize - _ringReadPos + _ringWritePos);
  return (_ringBufferSize - used) >= sampleCount;
}

/**
 * @brief Poll hook called by higher-level code to update buffer state.
 *
 * For WASAPI this is mostly a no-op because playback is event-driven; the
 * function exists to preserve the ISoundDriver interface and may be used to
 * wake producers in future changes.
 */
void WASAPISoundDriver::PollBufferPosition()
{
  // For WASAPI we rely on event-driven notifications; this poll can be used to
  // wake producers waiting on _canAddData. Keep it lightweight.
  AcquireSoundMutex();
  // No internal state to update here in current implementation; keep for API compatibility
  ReleaseSoundMutex();
}

/**
 * @brief Sets the device master volume if supported.
 *
 * Attempts to use the endpoint volume interface to adjust the master volume.
 * If the operation is unsupported, a log entry is written and false is returned.
 *
 * @param[in] volume0..100 desired master volume.
 * @return true on success, false otherwise.
 */
bool WASAPISoundDriver::SetCurrentSoundDeviceVolume(int volume)
{
  // Try to set device volume via endpoint volume if available. If not possible, return true as a no-op.
  if (!_device) return false;

  IAudioEndpointVolume *endpointVolume = nullptr;
  HRESULT hr = _device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void **)&endpointVolume);
  if (SUCCEEDED(hr) && endpointVolume)
  {
    // Volume expected in0..100
    float fLevel = (volume <= 0) ? 0.0f : (volume >= 100) ? 1.0f : (static_cast<float>(volume) / 100.0f);
    // Convert to scalar level (simple linear mapping)
    hr = endpointVolume->SetMasterVolumeLevelScalar(fLevel, nullptr);
    endpointVolume->Release();
    return SUCCEEDED(hr);
  }

  // Fallback: not supported, but not fatal
  _core.Log->AddLog("WASAPISoundDriver: SetCurrentSoundDeviceVolume not supported on this device\n");
  return false;
}

/**
 * @brief Reset internal sound state and clear buffers.
 *
 * Clears the ring buffers and resets resampling state. This does not affect
 * WASAPI device state; it is intended to be called on emulator hard resets.
 */
void WASAPISoundDriver::HardReset()
{
  AcquireSoundMutex();
  // Clear ring buffer
  std::fill(_ringBufferLeft.begin(), _ringBufferLeft.end(), 0);
  std::fill(_ringBufferRight.begin(), _ringBufferRight.end(), 0);
  _ringReadPos = _ringWritePos = 0;
  _resampleSrcPos = 0.0;
  _lastInputLeft = _lastInputRight = 0;
  ReleaseSoundMutex();
}
