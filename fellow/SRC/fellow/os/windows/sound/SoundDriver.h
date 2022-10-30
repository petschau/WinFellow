#pragma once

#include "fellow/api/defs.h"

class SoundDriver : ISoundDriver
{
private:
    const char *DSoundErrorString(HRESULT hResult);
    void DSoundFailure(const char *header, HRESULT err);
    void DSoundRelease();
    bool DSoundInitialize();
    void AddMode(sound_drv_dsound_device *dsound_device, bool stereo, bool bits16, ULO rate);
    sound_drv_dsound_mode *FindMode(sound_drv_dsound_device *dsound_device, bool stereo, bool bits16, ULO rate);
    void DSoundModeInformationRelease(sound_drv_dsound_device *dsound_device);
    void YesNoLog(const char *intro, bool pred);
    bool DSoundModeInformationInitialize(sound_drv_dsound_device *dsound_device);
    static bool DSoundSetVolume(sound_drv_dsound_device *dsound_device, const int volume);
    bool DSoundSetCurrentSoundDeviceVolume(const int volume);
    bool DSoundSetCooperativeLevel(sound_drv_dsound_device *dsound_device);
    void DSoundPrimaryBufferRelease(sound_drv_dsound_device *dsound_device);
    bool DSoundPrimaryBufferInitialize(sound_drv_dsound_device *dsound_device);
    void DSoundSecondaryBufferRelease(sound_drv_dsound_device *dsound_device);
    bool CreateSecondaryBuffer(sound_drv_dsound_device *dsound_device);
    bool ClearSecondaryBuffer(sound_drv_dsound_device *dsound_device);
    bool InitializeSecondaryBufferNotification(sound_drv_dsound_device *dsound_device);
    bool DSoundSecondaryBufferInitialize(sound_drv_dsound_device *dsound_device);
    void DSoundPlaybackStop(sound_drv_dsound_device *dsound_device);
    bool DSoundPlaybackInitialize(sound_drv_dsound_device *dsound_device);
    void Copy16BitsStereo(UWO *audio_buffer, UWO *left, UWO *right, ULO sample_count);
    void Copy16BitsMono(UWO *audio_buffer, UWO *left, UWO *right, ULO sample_count);
    void Copy8BitsStereo(UBY *audio_buffer, UWO *left, UWO *right, ULO sample_count);
    void Copy8BitsMono(UBY *audio_buffer, UWO *left, UWO *right, ULO sample_count);
    bool DSoundCopyToBuffer(sound_drv_dsound_device *dsound_device, UWO *left, UWO *right, ULO sample_count, ULO buffer_half);
    void Play(WOR *left, WOR *right, ULO sample_count);
    void AcquireMutex(sound_drv_dsound_device *dsound_device);
    void ReleaseMutex(sound_drv_dsound_device *dsound_device);
    bool WaitForData(sound_drv_dsound_device *dsound_device, ULO next_buffer_no, bool &need_to_restart_playback);
    void PollBufferPosition();
    bool ProcessEndOfBuffer(sound_drv_dsound_device *dsound_device, ULO current_buffer_no, ULO next_buffer_no);
    DWORD WINAPI ThreadProc(void *in);
    void HardReset();
    bool EmulationStart(ULO rate, bool bits16, bool stereo, ULO *sample_count_max);
    void EmulationStop();
    bool Startup(sound_device *devinfo);
    void Shutdown();

public:
    void Play(WOR *leftBuffer, WOR *rightBuffer, ULO sampleCount) override;
    void PollBufferPosition() override;
    bool SetVolume(const int) override;

    bool EmulationStart(ULO outputRate, bool bits16, bool stereo, ULO *bufferSampleCountMax) override;
    void EmulationStop() override;
    bool Startup(sound_device *devInfo) override;
    void Shutdown() override;
};