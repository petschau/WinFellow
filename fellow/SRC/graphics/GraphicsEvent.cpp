#ifdef GRAPH2

#include "GraphicsEventQueue.h"

ULO GraphicsEvent::MakeArriveTime(ULO rasterY, ULO cylinder)
{
  return rasterY*GraphicsEventQueue::GRAPHICS_CYLINDERS_PER_LINE + cylinder;
}

GraphicsEvent::GraphicsEvent(void)
  : _arriveTime(GraphicsEventQueue::GRAPHICS_ARRIVE_TIME_NONE),
    _next(0),
    _prev(0),
    _priority(0)
{
}

#endif
