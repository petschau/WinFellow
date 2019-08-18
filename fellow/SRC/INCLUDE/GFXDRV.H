#pragma once

#include <list>

#include "fellow/configuration/Configuration.h"
#include "GfxDrvMappedBufferPointer.h"
#include "GfxDrvColorBitsInformation.h"
#include "MappedChipsetFramebuffer.h"
#include "fellow/application/HostRenderConfiguration.h"
#include "fellow/application/HostRenderRuntimeSettings.h"
#include "ChipsetBufferRuntimeSettings.h"
#include "fellow/application/DisplayMode.h"
#include "fellow/hud/HudPropertyProvider.h"

/*===========================================================================*/
/* Implementing these functions creates a graphics driver for Fellow         */
/*===========================================================================*/

extern void gfxDrvClearCurrentBuffer();

extern void gfxDrvBufferFlip();

extern void gfxDrvSizeChanged(unsigned int width, unsigned int height);
extern void gfxDrvPositionChanged();

extern GfxDrvMappedBufferPointer gfxDrvMapChipsetFramebuffer();
extern void gfxDrvUnmapChipsetFramebuffer();

extern GfxDrvColorBitsInformation gfxDrvGetColorBitsInformation();
extern std::list<DisplayMode> gfxDrvGetDisplayModeList();

extern void gfxDrvDrawHUD(const MappedChipsetFramebuffer &mappedFramebuffer);

extern bool gfxDrvEmulationStart(const HostRenderConfiguration &hostRenderConfiguration, const HostRenderRuntimeSettings& hostRenderRuntimeSettings, const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings, HudPropertyProvider* hudPropertyProvider);
extern ULO gfxDrvEmulationStartPost(const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings);

extern void gfxDrvEmulationStop();

extern void gfxDrvNotifyActiveStatus(bool active);

extern bool gfxDrvStartup(DISPLAYDRIVER displaydriver);
extern void gfxDrvShutdown();

extern bool gfxDrvSaveScreenshot(const bool, const STR *filename);

extern bool gfxDrvValidateRequirements();
