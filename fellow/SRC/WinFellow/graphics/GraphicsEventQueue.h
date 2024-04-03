#pragma once

#include "Defs.h"
#include "GraphicsEvent.h"
#include "VirtualHost/Core.h"

class GraphicsEventQueue
{
private:
  GraphicsEvent *_events;

  void RunQueue(uint32_t untilTime);

public:
  const static uint32_t GRAPHICS_ARRIVE_TIME_NONE = 0xffffffff;

  static uint32_t GetCylindersPerLine()
  {
    return _core.CurrentFrameParameters->LongLineChipCycles.Offset * 2;
  }

  void Clear();
  GraphicsEvent *Pop();
  void Insert(GraphicsEvent *graphics_event);
  void Remove(GraphicsEvent *graphics_event);

  void Run(uint32_t untilTime);
};
