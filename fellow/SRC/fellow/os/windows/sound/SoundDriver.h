#pragma once

#include "fellow/api/defs.h"
#include "fellow/api/drivers/ISoundDriver.h"

struct sound_drv_dsound_mode
{
  ULO rate;
  bool bits16;
  bool stereo;
  ULO buffer_sample_count;
  ULO buffer_block_align;
};

struct sound_drv_dsound_device
{
  LPDIRECTSOUND lpDS;
  LPDIRECTSOUNDBUFFER lpDSB;  // Primary buffer
  LPDIRECTSOUNDBUFFER lpDSBS; // Secondary buffer
  LPDIRECTSOUNDNOTIFY lpDSN;  // Notificaton object
  felist *modes;
  sound_drv_dsound_mode *mode_current;
  HANDLE notifications[3];
  HANDLE data_available;
  HANDLE can_add_data;
  HANDLE mutex;
  UWO *pending_data_left;
  UWO *pending_data_right;
  ULO pending_data_sample_count;
  HANDLE thread;
  DWORD thread_id;
  bool notification_supported;
  ULO mmtimer;
  ULO mmresolution;
  DWORD lastreadpos;
};

class SoundDriver : ISoundDriver
{
private:
    static void CALLBACK timercb(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
  void HandleTimerCallback();
    const char *DSoundErrorString(HRESULT hResult);
    void DSoundFailure(const char *header, HRESULT err);
    void DSoundRelease();
    bool DSoundInitialize();
    void AddMode(sound_drv_dsound_device *dsound_device, bool stereo, bool bits16, ULO rate);
    sound_drv_dsound_mode *FindMode(sound_drv_dsound_device *dsound_device, bool stereo, bool bits16, ULO rate);
    void DSoundModeInformationRelease(sound_drv_dsound_device *dsound_device);
    void YesNoLog(const char *intro, bool pred);
    bool DSoundModeInformationInitialize(sound_drv_dsound_device *dsound_device);
    bool DSoundSetVolume(sound_drv_dsound_device *dsound_device, const int volume);
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
    void AcquireSoundMutex(sound_drv_dsound_device *dsound_device);
    void ReleaseSoundMutex(sound_drv_dsound_device *dsound_device);
    bool WaitForData(sound_drv_dsound_device *dsound_device, ULO next_buffer_no, bool &need_to_restart_playback);
    bool ProcessEndOfBuffer(sound_drv_dsound_device *dsound_device, ULO current_buffer_no, ULO next_buffer_no);
    static DWORD WINAPI ThreadProc(void *in);
    DWORD HandleThreadProc();

public:
    void Play(WOR *leftBuffer, WOR *rightBuffer, ULO sampleCount) override;
    void PollBufferPosition() override;
    bool SetVolume(const int) override;
    void SetCurrentSoundDeviceVolume(int volume) override;

    void HardReset();
    bool EmulationStart(ULO outputRate, bool bits16, bool stereo, ULO *bufferSampleCountMax) override;
    void EmulationStop() override;
    bool Startup(sound_device_capabilities *devInfo) override;
    void Shutdown() override;
};