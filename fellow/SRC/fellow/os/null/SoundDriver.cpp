#include "SoundDriver.h"

void SoundDriver::Play(WOR *leftBuffer, WOR *rightBuffer, ULO sampleCount)
{
}

void SoundDriver::PollBufferPosition()
{
}

bool SoundDriver::SetVolume(const int)
{
  return true;
}

bool SoundDriver::EmulationStart(ULO outputRate, bool bits16, bool stereo, ULO *bufferSampleCountMax)
{
  return true;
}

void SoundDriver::EmulationStop()
{
}

bool SoundDriver::Startup(sound_device_capabilities *devInfo)
{
  return true;
}

void SoundDriver::Shutdown()
{
}
