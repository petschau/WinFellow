#ifndef WAV_H
#define WAV_H


extern void wavPlay(WOR *left, WOR *right, ULO sample_count);
extern void wavEmulationStart(sound_rates rate, BOOLE bits16, BOOLE stereo, ULO buffersamplecountmax);
extern void wavEmulationStop(void);
extern void wavStartup(void);
extern void wavShutdown(void);

#endif

