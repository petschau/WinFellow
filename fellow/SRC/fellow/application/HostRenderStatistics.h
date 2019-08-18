#pragma once

#include "fellow/application/IFpsProvider.h"

class HostRenderStatistics : public IFpsProvider
{
private:
  unsigned int _first_frame_timestamp;
  unsigned int _last_frame_timestamp;
  unsigned int _last_50_timestamp;
  unsigned int _last_frame_ms;
  unsigned int _last_50_ms;
  unsigned int _frame_count;

public:
  void FpsStatsClear();
  void FpsStatsTimestamp();

  unsigned int GetLast50Fps() const override;
};
