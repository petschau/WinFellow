#pragma once

#include <cstdio>

extern void copperNotifyDMAEnableChanged(bool new_dma_enable_state);
extern void copperEventHandler();
extern void copperEndOfFrame();
extern void copperHardReset();
extern void copperEmulationStart();
extern void copperEmulationStop();
extern void copperStartup();
extern void copperShutdown();
