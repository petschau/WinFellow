#include "fellow/application/HostRenderStatistics.h"
#include "TIMER.H"

void HostRenderStatistics::FpsStatsClear()
{
  _last_50_ms = 0;
  _last_frame_ms = 0;
  _frame_count = 0;
}

void HostRenderStatistics::FpsStatsTimestamp()
{
  const unsigned int timestamp = timerGetTimeMs(); /* Get current time */

  if (_frame_count == 0)
  {
    _first_frame_timestamp = timerGetTimeMs();
    _last_frame_timestamp = _first_frame_timestamp;
    _last_50_timestamp = _first_frame_timestamp;
  }
  else
  {
    /* Time for last frame */
    _last_frame_ms = timestamp - _last_frame_timestamp;
    _last_frame_timestamp = timestamp;

    /* Update stats for last 50 frames, 1 Amiga 500 PAL second */
    if ((_frame_count % 50) == 0)
    {
      _last_50_ms = timestamp - _last_50_timestamp;
      _last_50_timestamp = timestamp;
    }
  }
  _frame_count++;
}

unsigned int HostRenderStatistics::GetLast50Fps() const
{
  if (_last_50_ms == 0)
  {
    return 0;
  }
  return 50000 / _last_50_ms;
}
