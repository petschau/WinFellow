#pragma once

#include <dsound.h>
#include <list>
#include "Driver/ISoundDriver.h"
#include "DirectSoundMode.h"

class DirectSoundDriver : public ISoundDriver
{
private:
  SoundDriverRuntimeConfiguration _runtimeConfiguration;

  LPDIRECTSOUND _lpDS;
  LPDIRECTSOUNDBUFFER _lpDSB;  // Primary buffer
  LPDIRECTSOUNDBUFFER _lpDSBS; // Secondary buffer
  LPDIRECTSOUNDNOTIFY _lpDSN;  // Notificaton object

  std::list<DirectSoundMode *> _modes;
  DirectSoundMode _modeCurrent;

  HANDLE _notifications[3];
  HANDLE _dataAvailable;
  HANDLE _canAddData;
  HANDLE _mutex;
  uint16_t *_pendingDataLeft;
  uint16_t *_pendingDataRight;
  uint32_t _pendingDataSampleCount;
  HANDLE _thread;
  DWORD _threadId;
  bool _notificationSupported;
  uint32_t _mmTimer;
  uint32_t _mmResolution;
  DWORD _lastReadPosition;

  bool _isInitialized = false;

  static void CALLBACK timercb(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
  void HandleTimerCallback();
  const char *DSoundErrorString(HRESULT hResult);
  void DSoundFailure(const char *header, HRESULT errorCode);
  void DSoundRelease();
  bool DSoundInitialize();

  bool DSoundModeInformationInitialize();
  void DSoundModeInformationRelease();
  void AddMode(bool isStereo, bool is16Bits, uint32_t rate);
  const DirectSoundMode *FindMode(bool isStereo, bool is16Bits, uint32_t rate) const;

  void YesNoLog(const char *description, bool predicate);
  bool DSoundSetVolume(const int volume);
  bool DSoundSetCooperativeLevel();
  void DSoundPrimaryBufferRelease();
  bool DSoundPrimaryBufferInitialize();
  void DSoundSecondaryBufferRelease();
  bool CreateSecondaryBuffer();
  bool ClearSecondaryBuffer();
  bool InitializeSecondaryBufferNotification();
  bool DSoundSecondaryBufferInitialize();
  void DSoundPlaybackStop();
  bool DSoundPlaybackInitialize();
  void Copy16BitsStereo(uint16_t *audioBuffer, uint16_t *left, uint16_t *right, uint32_t sampleCount);
  void Copy16BitsMono(uint16_t *audioBuffer, uint16_t *left, uint16_t *right, uint32_t sampleCount);
  void Copy8BitsStereo(uint8_t *audioBuffer, uint16_t *left, uint16_t *right, uint32_t sampleCount);
  void Copy8BitsMono(uint8_t *audioBuffer, uint16_t *left, uint16_t *right, uint32_t sampleCount);
  bool DSoundCopyToBuffer(uint16_t *left, uint16_t *right, uint32_t sampleCount, uint32_t bufferHalf);
  void AcquireSoundMutex();
  void ReleaseSoundMutex();
  bool WaitForData(uint32_t nextBufferNo, bool &needToRestartPlayback);
  bool ProcessEndOfBuffer(uint32_t currentBufferNo, uint32_t nextBufferNo);
  static DWORD WINAPI ThreadProc(void *in);
  DWORD HandleThreadProc();

public:
  void Play(int16_t *leftBuffer, int16_t *rightBuffer, uint32_t sampleCount) override;
  void PollBufferPosition() override;
  bool SetCurrentSoundDeviceVolume(int volume) override;

  void HardReset();
  bool EmulationStart(SoundDriverRuntimeConfiguration runtimeConfiguration) override;
  void EmulationStop() override;

  bool IsInitialized() override;

  DirectSoundDriver();
  virtual ~DirectSoundDriver();
};