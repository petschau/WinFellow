#ifndef SOUND_H
#define SOUND_H


/*===========================================================================*/
/* Exports from assembly                                                     */
/*===========================================================================*/

/* Renaming */

#define soundStart ASMRENAME(soundStart)
#define waudXpth ASMRENAME(waudXpth)
#define waudXptl ASMRENAME(waudXptl)
#define waudXlen ASMRENAME(waudXlen)
#define waudXper ASMRENAME(waudXper)
#define waudXvol ASMRENAME(waudXvol)
#define soundFrequencyHandler44100 ASMRENAME(soundFrequencyHandler44100)
#define soundFrequencyHandler31300 ASMRENAME(soundFrequencyHandler31300)
#define soundFrequencyHandler22050 ASMRENAME(soundFrequencyHandler22050)
#define soundFrequencyHandler15650 ASMRENAME(soundFrequencyHandler15650)
#define soundState0 ASMRENAME(soundState0)
#define soundChannelKill ASMRENAME(soundChannelKill)

/* Declare assembly functions */

extern void soundStart(void); /* Code alignment check label */
extern void waudXpth(ULO data, ULO address);
extern void waudXptl(ULO data, ULO address);
extern void waudXlen(ULO data, ULO address);
extern void waudXvol(ULO data, ULO address);
extern void waudXper(ULO data, ULO address);
extern void soundFrequencyHandler44100(void);
extern void soundFrequencyHandler31300(void);
extern void soundFrequencyHandler22050(void);
extern void soundFrequencyHandler15650(void);
extern void soundState0(ULO channelno);
extern void soundChannelKill(ULO channelno);


/*===========================*/
/* Symbols for configuration */
/*===========================*/

typedef enum {SOUND_15650, SOUND_22050, SOUND_31300, SOUND_44100} sound_rates;
typedef enum {SOUND_NONE, SOUND_PLAY, SOUND_EMULATE} sound_emulations;
typedef enum {SOUND_FILTER_ORIGINAL, SOUND_FILTER_ALWAYS, SOUND_FILTER_NEVER} sound_filters;


/*==========================*/
/* Sound device information */
/*==========================*/

typedef struct {
  BOOLE mono;
  BOOLE stereo;
  BOOLE bits8;
  BOOLE bits16;
  ULO rates_max[2][2]; /* Maximum playback rate for [8/16 bits][mono/stereo] */
} sound_device;


extern sound_emulations sound_emulation;
extern sound_rates sound_rate;
extern BOOLE sound_stereo;
extern BOOLE sound_16bits;
extern BOOLE sound_wav_capture;
extern ULO sound_buffer_length;
extern sound_filters sound_filter;


/*==================================*/
/* Property access                  */
/*==================================*/

extern void soundSetRate(sound_rates rate);
extern sound_rates soundGetRate(void);
extern ULO soundGetRateReal(void);
extern void soundSetStereo(BOOLE stereo);
extern BOOLE soundGetStereo(void);
extern void soundSet16Bits(BOOLE bits16);
extern BOOLE soundGet16Bits(void);
extern void soundSetEmulation(sound_emulations emulation);
extern sound_emulations soundGetEmulation(void);
extern void soundSetFilter(sound_filters filter);
extern sound_filters soundGetFilter(void);
extern void soundSetBufferLength(ULO ms);
extern ULO soundGetBufferLength(void);
extern void soundSetWAVDump(BOOLE wav_capture);
extern BOOLE soundGetWAVDump(void);




/*==================================*/
/* Standard Fellow Module Functions */
/*==================================*/

typedef void (*soundStateFunc)(ULO);

extern void soundEndOfLine(void);
extern void soundEndOfFrame(void);
extern void soundHardReset(void);
extern void soundEmulationStart(void);
extern void soundEmulationStop(void);
extern BOOLE soundStartup(void);
extern void soundShutdown(void);

#endif


