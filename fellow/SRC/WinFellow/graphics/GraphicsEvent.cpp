#include "Defs.h"

#include "GraphicsEventQueue.h"

uint32_t GraphicsEvent::MakeArriveTime(uint32_t rasterY, uint32_t cylinder)
{
  return rasterY * GraphicsEventQueue::GetCylindersPerLine() + cylinder;
}

GraphicsEvent::GraphicsEvent() : _arriveTime(GraphicsEventQueue::GRAPHICS_ARRIVE_TIME_NONE), _next(nullptr), _prev(nullptr), _priority(0)
{
}
