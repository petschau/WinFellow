#ifndef GFXDRV_H


/*===========================================================================*/
/* Implementing these functions creates a graphics driver for Fellow         */
/*===========================================================================*/

extern void gfxDrvSet8BitColor(ULO col, ULO r, ULO g, ULO b);
extern void gfxDrvBufferFlip(void);
extern ULO gfxDrvModeSet(draw_mode *dm, ULO maxbuffercount);
extern UBY *gfxDrvValidateBufferPointer(void);
extern void gfxDrvInvalidateBufferPointer(void);
extern void gfxDrvEmulationStart(draw_mode *dm, ULO maxbuffercount);
extern ULO gfxDrvEmulationStartPost(void);
extern void gfxDrvEmulationStop(void);
extern BOOLE gfxDrvStartup(void);
extern void gfxDrvShutdown(void);

#endif
