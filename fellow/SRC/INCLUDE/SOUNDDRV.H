#ifndef SOUNDDRV_H
#define SOUNDDRV_H


/*===========================================================================*/
/* Implementing these functions creates a sound driver for Fellow            */
/*===========================================================================*/

extern bool soundDrvStartup(sound_device *devinfo);
extern void soundDrvShutdown(void);
extern bool soundDrvEmulationStart(ULO outputrate,
				   bool bits16, 
				   bool stereo, 
				   ULO *buffersamplecountmax);
extern void soundDrvEmulationStop(void);
extern void soundDrvPlay(WOR *leftbuffer, WOR *rightbuffer, ULO samplecount);
extern void soundDrvPollBufferPosition(void);
extern bool  soundDrvDSoundSetVolume(int);

#endif