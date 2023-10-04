#pragma once
#include <cstdint>
#include "Driver/SoundDriverRuntimeConfiguration.h"

class ISoundDriver
{
public:
  virtual void Play(int16_t *leftBuffer, int16_t *rightBuffer, uint32_t sampleCount) = 0;
  virtual void PollBufferPosition() = 0;
  virtual bool SetCurrentSoundDeviceVolume(int volume) = 0;

  virtual bool EmulationStart(SoundDriverRuntimeConfiguration runtimeConfiguration) = 0;
  virtual void EmulationStop() = 0;

  virtual bool IsInitialized() = 0;

  ISoundDriver() = default;
  virtual ~ISoundDriver() = default;
};
