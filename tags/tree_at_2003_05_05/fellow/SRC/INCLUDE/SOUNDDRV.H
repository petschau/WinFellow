#ifndef SOUNDDRV_H
#define SOUNDDRV_H


/*===========================================================================*/
/* Implementing these functions creates a sound driver for Fellow            */
/*===========================================================================*/

extern BOOLE soundDrvStartup(sound_device *devinfo);
extern void soundDrvShutdown(void);
extern BOOLE soundDrvEmulationStart(ULO outputrate,
				    BOOLE bits16, 
				    BOOLE stereo, 
				    ULO *buffersamplecountmax);
extern void soundDrvEmulationStop(void);
extern void soundDrvPlay(WOR *leftbuffer, WOR *rightbuffer, ULO samplecount);

#endif