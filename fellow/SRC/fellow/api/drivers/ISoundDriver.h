#pragma once

#include "fellow/api/defs.h"

class ISoundDriver
{
public:
    virtual void Play(WOR *leftBuffer, WOR *rightBuffer, ULO sampleCount) = 0;
    virtual void PollBufferPosition() = 0;
    virtual bool SetCurrentSoundDeviceVolume(int volume) = 0;

    virtual bool EmulationStart(ULO outputRate, bool bits16, bool stereo, ULO *bufferSampleCountMax) = 0;
    virtual void EmulationStop() = 0;

    virtual bool IsInitialized() = 0;

    ISoundDriver() = default;
    ~ISoundDriver() = default;
};
