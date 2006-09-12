#ifndef SOUNDDRVSDL_H
#define SOUNDDRVSDL_H


/*===========================================================================*/
/* Implementing these functions creates a sound driver for Fellow            */
/*===========================================================================*/

extern BOOLE soundDrvSDL_Startup(sound_device *devinfo);
extern void  soundDrvSDL_Shutdown(void);
extern BOOLE soundDrvSDL_EmulationStart(ULO outputrate, BOOLE bits16, BOOLE stereo, ULO *buffersamplecountmax);
extern void  soundDrvSDL_EmulationStop(void);
extern void  soundDrvSDL_Play(WOR *leftbuffer, WOR *rightbuffer, ULO samplecount);
extern void  soundDrvSDL_PollBufferPosition(void);

#endif