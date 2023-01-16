#pragma once

#include "fellow/application/HostRenderConfiguration.h"

class Scheduler;
class BitplaneRegisters;

class InterlaceStatus
{
private:
  Scheduler *_scheduler;
  BitplaneRegisters *_bitplaneRegisters;

  bool frame_is_interlaced;
  bool frame_is_long;
  bool enable_deinterlace;
  bool use_interlaced_rendering;
  DisplayScaleStrategy display_scale_strategy;

  bool GetUseInterlacedRendering();
  bool GetFrameIsLong();
  bool DecideUseInterlacedRendering();
  bool GetFrameIsInterlaced();
  void DecideInterlaceStatusForNextFrame();
  void InterlaceStartup();
  void InterlaceEndOfFrame();
  void InterlaceConfigure(bool deinterlace, DisplayScaleStrategy displayScaleStrategy);

public:
  InterlaceStatus(Scheduler *scheduler, BitplaneRegisters *bitplaceRegisters);
};