#ifndef SOUNDDRV_H
#define SOUNDDRV_H


/*===========================================================================*/
/* Implementing these functions creates a sound driver for Fellow            */
/*===========================================================================*/

extern bool soundDrvStartup(sound_device *devinfo);
extern void soundDrvShutdown(void);
extern bool soundDrvEmulationStart(uint32_t outputrate,
				   bool bits16, 
				   bool stereo, 
				   uint32_t *buffersamplecountmax);
extern void soundDrvEmulationStop(void);
extern void soundDrvPlay(int16_t *leftbuffer, int16_t *rightbuffer, uint32_t samplecount);
extern void soundDrvPollBufferPosition(void);
extern bool soundDrvDSoundSetCurrentSoundDeviceVolume(const int);

#endif