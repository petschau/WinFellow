#pragma once

//===============================
// Interface for the sound driver
//===============================

class ISoundDriver
{
public:
    virtual void Play(WOR *leftBuffer, WOR *rightBuffer, ULO sampleCount) = 0;
    virtual void PollBufferPosition() = 0;
    virtual bool SetVolume(const int) = 0;

    virtual bool EmulationStart(ULO outputRate, bool bits16, bool stereo, ULO *bufferSampleCountMax) = 0;
    virtual void EmulationStop() = 0;
    virtual bool Startup(sound_device *devInfo) = 0;
    virtual void Shutdown() = 0;
};