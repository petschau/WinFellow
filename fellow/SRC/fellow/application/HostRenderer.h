#pragma once

#include "fellow/api/defs.h"

#include "GRAPH.H"

#include "MappedChipsetFramebuffer.h"
#include "GfxDrvColorBitsInformation.h"
#include "fellow/chipset/ChipsetCallbacks.h"

#include "fellow/configuration/Configuration.h"
#include <map>
#include <list>

#include "fellow/application/HostRenderConfiguration.h"
#include "fellow/application/HostRenderRuntimeSettings.h"
#include "ChipsetBufferRuntimeSettings.h"
#include "fellow/application/HostRenderStatistics.h"
#include "fellow/hud/HudPropertyProvider.h"

class HostRenderer : public IColorChangeEventHandler
{
private:
  HostRenderConfiguration _hostRenderConfiguration;
  HostRenderRuntimeSettings _hostRenderRuntimeSettings;
  ChipsetBufferRuntimeSettings _chipsetBufferRuntimeSettings;
  HostRenderStatistics _hostRenderStatistics;
  HudPropertyProvider _hudPropertyProvider;
  std::list<DisplayMode> _displayModes;

  unsigned int _clear_buffers;
  unsigned int _buffer_show;  // Number of the currently shown buffer
  unsigned int _buffer_draw;  // Number of the current drawing buffer
  unsigned int _buffer_count; // Number of available framebuffers

  unsigned int _framecounter;

  int _frameSkipCounter;

  // Move this to a separate class
  // Maps all 12-bit colors to the current host color format
  uint64_t _hostColorMap[4096];

  // Holds the host color for each color register, including a half-brite version
  uint64_t _hostColor[64];

  void InitializeHostColorMap(const GfxDrvColorBitsInformation &colorBitsInformation);
  void InitializeMaxClip();

  void DrawFrameInHostLineExact(const MappedChipsetFramebuffer &mappedChipsetFramebuffer);

  MappedChipsetFramebuffer MapChipsetFramebuffer();
  void UnmapChipsetFramebuffer();

  void FlipBuffer();
  void DrawHUD(const MappedChipsetFramebuffer &mappedChipsetFramebuffer);

  void CalculateChipsetBufferSize();
  void CalculateOutputClipPixels();

  unsigned int GetAutomaticInternalScaleFactor() const;

  void SetDisplayModeForConfiguration();
  void SetFullscreenMode(const DimensionsUInt &size, DisplayColorDepth colorDepth, unsigned int refreshRate);
  void SetWindowedMode(const DimensionsUInt &size);

  const DisplayMode* GetFirstDisplayMode() const;
  const DisplayMode* FindDisplayMode(unsigned int width, unsigned int height, unsigned int bits, unsigned int refresh, bool allowAnyRefresh) const;
  void ClearDisplayModeList();

  void InitializeFrameSkipCounter();
  void AdvanceFrameSkipCounter();

  void InitializeFramecounter();
  void AdvanceFramecounter();

public:
  void Configure(const HostRenderConfiguration &hostRenderConfiguration);

  const RectShresi &GetChipsetBufferMaxClip() const;

  bool ShouldSkipFrame() const;
  unsigned int GetFramecounter() const;
  unsigned int GetDrawBufferIndex() const;

  const uint64_t *GetHostColors() const;
  uint64_t GetHostColorForColor12Bit(uint16_t color12Bit) const;

  void ColorChangedHandler(unsigned int colorIndex, UWO color12Bit, UWO halfbriteColor12Bit) override;

  unsigned int GetActiveBufferColorBits() const;

  void SetChipsetBufferMaxClip(const RectShresi &chipsetBufferMaxClip);
  void SetHostOutputClip(const RectShresi &chipsetBufferOutputClip);

  const std::list<DisplayMode> &GetDisplayModes() const;

  unsigned int GetChipsetBufferScaleFactor() const;
  ULO GetOutputScaleFactor() const;

  void SetDisplayScale(DISPLAYSCALE displayscale);
  DISPLAYSCALE GetDisplayScale() const;

  void SetDisplayScaleStrategy(DISPLAYSCALE_STRATEGY displayscalestrategy);
  DISPLAYSCALE_STRATEGY GetDisplayScaleStrategy() const;

  void SetDisplayDriver(DISPLAYDRIVER displaydriver);

  void SetFrameskipRatio(unsigned int frameskipratio);
  ULO GetBufferCount() const;

  void DrawModeFunctionTablesInitialize(unsigned int activeBufferColorBits);
  void SetClearAllBuffersFlag();

  void UpdateDrawFunctions();
  void ReinitializeRendering();

  bool RestartGraphicsDriver(DISPLAYDRIVER displaydriver);

  // Fellow runtime events
  void EndOfFrame();

  // Fellow offline events
  void HardReset();
  bool EmulationStart();
  bool EmulationStartPost();
  void EmulationStop();
  bool Startup();
  void Shutdown();

  HostRenderer();
};

extern HostRenderer Draw;
