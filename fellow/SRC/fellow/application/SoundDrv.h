#pragma once

/*===========================================================================*/
/* Implementing these functions creates a sound driver for Fellow            */
/*===========================================================================*/

extern bool soundDrvStartup(sound_device *devinfo);
extern void soundDrvShutdown();
extern bool soundDrvEmulationStart(ULO outputrate, bool bits16, bool stereo, ULO *buffersamplecountmax);
extern void soundDrvEmulationStop();
extern void soundDrvPlay(WOR *leftbuffer, WOR *rightbuffer, ULO samplecount);
extern void soundDrvPollBufferPosition();
extern bool soundDrvDSoundSetCurrentSoundDeviceVolume(const int);
