#ifndef BLIT_H
#define BLIT_H

/*===========================================================================*/
/* Rename assembly functions to non-underscore form                          */
/*===========================================================================*/

#define blit_min_generic ASMRENAME(blit_min_generic)
#define blit_min_00 ASMRENAME(blit_min_00)
#define blit_min_01 ASMRENAME(blit_min_01)
#define blit_min_02 ASMRENAME(blit_min_02)
#define blit_min_03 ASMRENAME(blit_min_03)
#define blit_min_04 ASMRENAME(blit_min_04)
#define blit_min_05 ASMRENAME(blit_min_05)
#define blit_min_06 ASMRENAME(blit_min_06)
#define blit_min_07 ASMRENAME(blit_min_07)
#define blit_min_08 ASMRENAME(blit_min_08)
#define blit_min_09 ASMRENAME(blit_min_09)
#define blit_min_0a ASMRENAME(blit_min_0a)
#define blit_min_0b ASMRENAME(blit_min_0b)
#define blit_min_0c ASMRENAME(blit_min_0c)
#define blit_min_0d ASMRENAME(blit_min_0d)
#define blit_min_0e ASMRENAME(blit_min_0e)
#define blit_min_0f ASMRENAME(blit_min_0f)
#define blit_min_2a ASMRENAME(blit_min_2a)
#define blit_min_4a ASMRENAME(blit_min_4a)
#define blit_min_ca ASMRENAME(blit_min_ca)
#define blit_min_d8 ASMRENAME(blit_min_d8)
#define blit_min_ea ASMRENAME(blit_min_ea)
#define blit_min_f0 ASMRENAME(blit_min_f0)
#define blit_min_fa ASMRENAME(blit_min_fa)
#define blit_min_fc ASMRENAME(blit_min_fc)
#define blit_min_ff ASMRENAME(blit_min_ff)
#define wbltcon0 ASMRENAME(wbltcon0)
#define wbltcon0l ASMRENAME(wbltcon0l)
#define wbltcon1 ASMRENAME(wbltcon1)
#define wbltafwm ASMRENAME(wbltafwm)
#define wbltalwm ASMRENAME(wbltalwm)
#define wbltcpth ASMRENAME(wbltcpth)
#define wbltcptl ASMRENAME(wbltcptl)
#define wbltbpth ASMRENAME(wbltbpth)
#define wbltbptl ASMRENAME(wbltbptl)
#define wbltapth ASMRENAME(wbltapth)
#define wbltaptl ASMRENAME(wbltaptl)
#define wbltdpth ASMRENAME(wbltdpth)
#define wbltdptl ASMRENAME(wbltdptl)
#define wbltsize ASMRENAME(wbltsize)
#define wbltcmod ASMRENAME(wbltcmod)
#define wbltbmod ASMRENAME(wbltbmod)
#define wbltamod ASMRENAME(wbltamod)
#define wbltdmod ASMRENAME(wbltdmod)
#define wbltsizv ASMRENAME(wbltsizv)
#define wbltsizh ASMRENAME(wbltsizh)
#define wbltcdat ASMRENAME(wbltcdat)
#define wbltbdat ASMRENAME(wbltbdat)
#define wbltadat ASMRENAME(wbltadat)
#define blitterlinemodeoctant0 ASMRENAME(blitterlinemodeoctant0)
#define blitterlinemodeoctant1 ASMRENAME(blitterlinemodeoctant1)
#define blitterlinemodeoctant2 ASMRENAME(blitterlinemodeoctant2)
#define blitterlinemodeoctant3 ASMRENAME(blitterlinemodeoctant3)
#define blitterlinemodeoctant4 ASMRENAME(blitterlinemodeoctant4)
#define blitterlinemodeoctant5 ASMRENAME(blitterlinemodeoctant5)
#define blitterlinemodeoctant6 ASMRENAME(blitterlinemodeoctant6)
#define blitterlinemodeoctant7 ASMRENAME(blitterlinemodeoctant7)


/*===========================================================================*/
/* Declare assembly functions                                                */
/*===========================================================================*/


/*==============================*/
/* Used to check code alignment */
/*==============================*/

extern void blitstart(void);


/*===============*/
/* Minterm logic */
/*===============*/

typedef ULO (*blit_min_func)(ULO, ULO, ULO);

extern ULO blit_min_generic(ULO, ULO, ULO);
extern ULO blit_min_00(ULO, ULO, ULO);
extern ULO blit_min_01(ULO, ULO, ULO);
extern ULO blit_min_02(ULO, ULO, ULO);
extern ULO blit_min_03(ULO, ULO, ULO);
extern ULO blit_min_04(ULO, ULO, ULO);
extern ULO blit_min_05(ULO, ULO, ULO);
extern ULO blit_min_06(ULO, ULO, ULO);
extern ULO blit_min_07(ULO, ULO, ULO);
extern ULO blit_min_08(ULO, ULO, ULO);
extern ULO blit_min_09(ULO, ULO, ULO);
extern ULO blit_min_0a(ULO, ULO, ULO);
extern ULO blit_min_0b(ULO, ULO, ULO);
extern ULO blit_min_0c(ULO, ULO, ULO);
extern ULO blit_min_0d(ULO, ULO, ULO);
extern ULO blit_min_0e(ULO, ULO, ULO);
extern ULO blit_min_0f(ULO, ULO, ULO);
extern ULO blit_min_2a(ULO, ULO, ULO);
extern ULO blit_min_4a(ULO, ULO, ULO);
extern ULO blit_min_ca(ULO, ULO, ULO);
extern ULO blit_min_d8(ULO, ULO, ULO);
extern ULO blit_min_ea(ULO, ULO, ULO);
extern ULO blit_min_f0(ULO, ULO, ULO);
extern ULO blit_min_fa(ULO, ULO, ULO);
extern ULO blit_min_fc(ULO, ULO, ULO);
extern ULO blit_min_ff(ULO, ULO, ULO);


/*===============*/
/* Blitter modes */
/*===============*/

typedef void (*blitmodefunc)(void);

extern void blitterlinemodeoctant0(void);
extern void blitterlinemodeoctant1(void);
extern void blitterlinemodeoctant2(void);
extern void blitterlinemodeoctant3(void);
extern void blitterlinemodeoctant4(void);
extern void blitterlinemodeoctant5(void);
extern void blitterlinemodeoctant6(void);
extern void blitterlinemodeoctant7(void);


/*==============================*/
/* IO-register access functions */
/*==============================*/

extern void wbltcon0(ULO data, ULO address);
extern void wbltcon0l(ULO data, ULO address);
extern void wbltcon1(ULO data, ULO address);
extern void wbltafwm(ULO data, ULO address);
extern void wbltalwm(ULO data, ULO address);
extern void wbltcpth(ULO data, ULO address);
extern void wbltcptl(ULO data, ULO address);
extern void wbltbpth(ULO data, ULO address);
extern void wbltbptl(ULO data, ULO address);
extern void wbltapth(ULO data, ULO address);
extern void wbltaptl(ULO data, ULO address);
extern void wbltdpth(ULO data, ULO address);
extern void wbltdptl(ULO data, ULO address);
extern void wbltsize(ULO data, ULO address);
extern void wbltsizh(ULO data, ULO address);
extern void wbltsizv(ULO data, ULO address);
extern void wbltcmod(ULO data, ULO address);
extern void wbltbmod(ULO data, ULO address);
extern void wbltamod(ULO data, ULO address);
extern void wbltdmod(ULO data, ULO address);
extern void wbltcdat(ULO data, ULO address);
extern void wbltbdat(ULO data, ULO address);
extern void wbltadat(ULO data, ULO address);

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

extern ULO blitend;
extern ULO blitterstatus;
extern ULO bltzero;


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

#endif
