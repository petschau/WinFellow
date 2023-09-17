#ifndef WAV_H
#define WAV_H


extern void wavPlay(WOR *left, WOR *right, uint32_t sample_count);
extern void wavEmulationStart(sound_rates rate, BOOLE bits16, BOOLE stereo, uint32_t buffersamplecountmax);
extern void wavEmulationStop(void);
extern void wavStartup(void);
extern void wavShutdown(void);

#endif

