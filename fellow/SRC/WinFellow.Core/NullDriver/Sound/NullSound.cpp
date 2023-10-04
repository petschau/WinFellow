#include "NullSound.h"

void NullSound::Play(int16_t *leftBuffer, int16_t *rightBuffer, uint32_t sampleCount)
{
}

void NullSound::PollBufferPosition()
{
}

bool NullSound::SetCurrentSoundDeviceVolume(int volume)
{
  return true;
}

void NullSound::HardReset()
{
}

bool NullSound::EmulationStart(SoundDriverRuntimeConfiguration runtimeConfiguration)
{
  return true;
}

void NullSound::EmulationStop()
{
}

bool NullSound::IsInitialized()
{
  return true;
}
