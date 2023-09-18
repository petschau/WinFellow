#pragma once

/*===========================================================================*/
/* Implementing these functions creates a sound driver for Fellow            */
/*===========================================================================*/

extern bool soundDrvStartup(sound_device *devinfo);
extern void soundDrvShutdown();
extern bool soundDrvEmulationStart(uint32_t outputrate,
				   bool bits16, 
				   bool stereo, 
				   uint32_t *buffersamplecountmax);
extern void soundDrvEmulationStop();
extern void soundDrvPlay(int16_t *leftbuffer, int16_t *rightbuffer, uint32_t samplecount);
extern void soundDrvPollBufferPosition();
extern bool soundDrvDSoundSetCurrentSoundDeviceVolume(const int);
