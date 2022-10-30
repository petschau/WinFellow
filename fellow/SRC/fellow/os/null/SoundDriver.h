#pragma once

#include "fellow/api/defs.h"

class SoundDriver : ISoundDriver
{
public:
    void Play(WOR *leftBuffer, WOR *rightBuffer, ULO sampleCount) override;
    void PollBufferPosition() override;
    bool SetVolume(const int) override;

    bool EmulationStart(ULO outputRate, bool bits16, bool stereo, ULO *bufferSampleCountMax) override;
    void EmulationStop() override;
    bool Startup(sound_device *devInfo) override;
    void Shutdown() override;
};
