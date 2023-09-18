#pragma once

extern void wavPlay(int16_t *left, int16_t *right, uint32_t sample_count);
extern void wavEmulationStart(sound_rates rate, BOOLE bits16, BOOLE stereo, uint32_t buffersamplecountmax);
extern void wavEmulationStop();
extern void wavStartup();
extern void wavShutdown();
