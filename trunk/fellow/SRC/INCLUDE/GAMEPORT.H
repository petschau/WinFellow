#ifndef GAMEPORT_H
#define GAMEPORT_H

/* ---------------- CHANGE LOG ----------------- 
Tuesday, September 19, 2000: nova
- added gameport_autofire arrays
*/

/*===========================================================================*/
/* From assembly                                                             */
/*===========================================================================*/

#define rjoy0dat ASMRENAME(rjoy0dat)
#define rjoy1dat ASMRENAME(rjoy1dat)
#define rpot0dat ASMRENAME(rpot0dat)
#define rpot1dat ASMRENAME(rpot1dat)
#define rpotgor ASMRENAME(rpotgor)
#define wjoytest ASMRENAME(wjoytest)

extern ULO rjoy0dat(ULO address);
extern ULO rjoy1dat(ULO address);
extern ULO rpot0dat(ULO address);
extern ULO rpot1dat(ULO address);
extern ULO rpotgor(ULO address);
extern void wjoytest(ULO data, ULO address);


/*===========================================================================*/
/* Joystickdata                                                              */
/* TRUE or FALSE                                                             */
/*===========================================================================*/

extern BOOLE gameport_left[2];
extern BOOLE gameport_up[2];
extern BOOLE gameport_right[2];
extern BOOLE gameport_down[2];
extern BOOLE gameport_fire0[2];
extern BOOLE gameport_fire1[2];
extern BOOLE gameport_autofire0[2];
extern BOOLE gameport_autofire1[2];


/*===========================================================================*/
/* Types of input devices to a gameport                                      */
/*===========================================================================*/

typedef enum {
  GP_NONE,
  GP_JOYKEY0,
  GP_JOYKEY1,
  GP_ANALOG0,
  GP_ANALOG1,
  GP_MOUSE0,
  GP_MOUSE1
} gameport_inputs;

extern gameport_inputs gameport_input[2];


/*===========================================================================*/
/* Functions                                                                 */
/*===========================================================================*/

extern void gameportMouseHandler(gameport_inputs mousedev,
								 LON x,
								 LON y,
								 BOOLE button1,
								 BOOLE button2,
								 BOOLE button3);
extern void gameportJoystickHandler(gameport_inputs joydev,
									BOOLE left,
									BOOLE up,
									BOOLE right,
									BOOLE down,
									BOOLE button1,
									BOOLE button2);
extern void gameportSetInput(ULO index, gameport_inputs gameportinput);


/*===========================================================================*/
/* Fellow standard control functions                                         */
/*===========================================================================*/

extern void gameportEOFHandler(void);
extern void gameportHardReset(void);
extern void gameportEmulationStart(void);
extern void gameportEmulationStop(void);
extern void gameportStartup(void);
extern void gameportShutdown(void);


#endif
