#ifndef BLIT_H
#define BLIT_H


/*==============================*/
/* IO-register access functions */
/*==============================*/

void wbltcon0_C(ULO data, ULO address);
void wbltcon1_C(ULO data, ULO address);
void wbltafwm_C(ULO data, ULO address);
void wbltalwm_C(ULO data, ULO address);
void wbltcpth_C(ULO data, ULO address);
void wbltcptl_C(ULO data, ULO address);
void wbltbpth_C(ULO data, ULO address);
void wbltbptl_C(ULO data, ULO address);
void wbltapth_C(ULO data, ULO address);
void wbltaptl_C(ULO data, ULO address);
void wbltdpth_C(ULO data, ULO address);
void wbltdptl_C(ULO data, ULO address);
void wbltsize_C(ULO data, ULO address);
void wbltcmod_C(ULO data, ULO address);
void wbltbmod_C(ULO data, ULO address);
void wbltamod_C(ULO data, ULO address);
void wbltdmod_C(ULO data, ULO address);
void wbltcdat_C(ULO data, ULO address);
void wbltbdat_C(ULO data, ULO address);
void wbltadat_C(ULO data, ULO address);

void wbltcon0l_C(ULO data, ULO address);
void wbltsizh_C(ULO data, ULO address);
void wbltsizv_C(ULO data, ULO address);



/*==========================*/
/* Global blitter variables */
/*==========================*/

extern LON blitend;
extern ULO blitterstatus;
extern ULO bltzero;
extern BOOLE blitter_ECS;
extern ULO blit_cycle_length;
extern ULO blit_cycle_free;
extern ULO blitterdmawaiting;
extern BOOLE blit_started;

/*===========================================================================*/
/* Blitter-properties                                                        */
/*===========================================================================*/

extern void blitterSetFast(BOOLE fast);
extern BOOLE blitterGetFast(void);
extern void blitterSetECS(BOOLE ECS);
extern BOOLE blitterGetECS(void);

extern void blitterSetOperationLog(BOOLE operation_log);
extern BOOLE blitterGetOperationLog(void);


/*===========================================================================*/
/* Declare C blitter functions                                               */
/*===========================================================================*/

void blitterEmulationStart(void);
void blitterEmulationStop(void);
void blitterHardReset(void);
void blitterStartup(void);
void blitterShutdown(void);
void blitterLineMode(void);
void blitFinishBlit(void);
void blitterCopy(void);

#endif
