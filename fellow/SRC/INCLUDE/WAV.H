#pragma once

extern void wavPlay(WOR *left, WOR *right, ULO sample_count);
extern void wavEmulationStart(sound_rates rate, BOOLE bits16, BOOLE stereo, ULO buffersamplecountmax);
extern void wavEmulationStop();
extern void wavStartup();
extern void wavShutdown();
