#pragma once

#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include <list>
#include <vector>
#include "Driver/ISoundDriver.h"
#include "WASAPISoundMode.h"

class WASAPISoundDriver : public ISoundDriver
{
private:
  SoundDriverRuntimeConfiguration _runtimeConfiguration;

  IMMDevice *_device = nullptr;
  IAudioClient *_audioClient = nullptr;
  IAudioRenderClient *_renderClient = nullptr;
  HANDLE _eventHandle = nullptr;
  HANDLE _thread = nullptr;
  HANDLE _mutex = nullptr;
  HANDLE _canAddData = nullptr; // event signaled by audio thread when space available
  WAVEFORMATEX *_pwfx = nullptr;
  std::list<WASAPISoundMode *> _modes;
  WASAPISoundMode _modeCurrent;
  bool _isInitialized = false;
  bool _running = false;
  bool _comInitializedInMainThread = false; // Track if COM was initialized in main thread

  // Ring buffer for audio data
  std::vector<int16_t> _ringBufferLeft, _ringBufferRight;
  size_t _ringReadPos = 0, _ringWritePos = 0, _ringBufferSize = 0;

  // Resampling state
  bool _needResample = false;     // True if emulator sample rate != WASAPI mix rate
  double _resampleRatio = 1.0;   // mixRate / emulatorRate
  double _resampleSrcPos = 0.0;  // fractional source position progress between Play() calls
  int16_t _lastInputLeft = 0;    // last sample from previous Play() to aid interpolation
  int16_t _lastInputRight = 0;

  class DeviceNotificationClient : public IMMNotificationClient
  {
    LONG _refCount;
    WASAPISoundDriver *_driver;

  public:
    DeviceNotificationClient(WASAPISoundDriver *driver) : _refCount(1), _driver(driver)
    {
    }
    ULONG STDMETHODCALLTYPE AddRef() override
    {
      return InterlockedIncrement(&_refCount);
    }
    ULONG STDMETHODCALLTYPE Release() override
    {
      ULONG ulRef = InterlockedDecrement(&_refCount);
      if (0 == ulRef) delete this;
      return ulRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface) override
    {
      if (IID_IUnknown == riid || __uuidof(IMMNotificationClient) == riid)
      {
        *ppvInterface = static_cast<IMMNotificationClient *>(this);
        AddRef();
        return S_OK;
      }
      *ppvInterface = nullptr;
      return E_NOINTERFACE;
    }
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR) override
    {
      if (flow == eRender && role == eConsole)
      {
        _driver->OnDefaultDeviceChanged();
      }
      return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override
    {
      return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override
    {
      return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) override
    {
      return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override
    {
      return S_OK;
    }
  };

  IMMDeviceEnumerator *_deviceEnumerator = nullptr;
  DeviceNotificationClient *_notificationClient = nullptr;

  static DWORD WINAPI ThreadProc(void *in);
  DWORD HandleThreadProc();
  bool InitializeWASAPI();
  void ReleaseWASAPI();
  void AddMode(bool isStereo, uint8_t bitsPerSample, uint32_t rate);
  const WASAPISoundMode *FindMode(bool isStereo, uint8_t bitsPerSample, uint32_t rate) const;
  void FillBuffer(BYTE *pData, uint32_t frames);
  void AcquireSoundMutex();
  void ReleaseSoundMutex();
  bool InitializeModeInformation();

public:
  WASAPISoundDriver();
  virtual ~WASAPISoundDriver();

  void Play(int16_t *left, int16_t *right, uint32_t sampleCount) override;
  void PollBufferPosition() override;
  bool SetCurrentSoundDeviceVolume(int volume) override;
  bool CanAcceptSamples(uint32_t sampleCount) override;

  void HardReset();
  bool EmulationStart(SoundDriverRuntimeConfiguration runtimeConfiguration) override;
  void EmulationStop() override;
  bool IsInitialized() override;
  void OnDefaultDeviceChanged();
};