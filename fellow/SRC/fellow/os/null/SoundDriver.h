#pragma once

#include "fellow/api/defs.h"
#include "fellow/api/drivers/ISoundDriver.h"

class SoundDriver : public ISoundDriver
{
public:
    void Play(WOR *leftBuffer, WOR *rightBuffer, ULO sampleCount) override;
    void PollBufferPosition() override;
    bool SetVolume(const int) override;

    bool EmulationStart(ULO outputRate, bool bits16, bool stereo, ULO *bufferSampleCountMax) override;
    void EmulationStop() override;
    bool Startup(sound_device_capabilities *devInfo) override;
    void Shutdown() override;
};
