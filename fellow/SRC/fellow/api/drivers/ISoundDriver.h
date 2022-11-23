#pragma once

#include "fellow/api/defs.h"

//===============================
// Interface for the sound driver
//===============================

struct sound_device_capabilities
{
    BOOLE mono;
    BOOLE stereo;
    BOOLE bits8;
    BOOLE bits16;
    ULO rates_max[2][2]; // Maximum playback rate for [8/16 bits][mono/stereo]
};

class ISoundDriver
{
public:
    virtual void Play(WOR *leftBuffer, WOR *rightBuffer, ULO sampleCount) = 0;
    virtual void PollBufferPosition() = 0;
    virtual bool SetVolume(const int) = 0;

    virtual bool EmulationStart(ULO outputRate, bool bits16, bool stereo, ULO *bufferSampleCountMax) = 0;
    virtual void EmulationStop() = 0;
    virtual bool Startup(sound_device_capabilities *devInfo) = 0;
    virtual void Shutdown() = 0;
};