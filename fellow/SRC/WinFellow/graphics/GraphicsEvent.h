#pragma once

#include "DEFS.H"
#include "BUS.H"

class GraphicsEventQueue;

class GraphicsEvent
{
public:
  GraphicsEventQueue *_queue;
  GraphicsEvent *_next;
  GraphicsEvent *_prev;
  uint32_t _arriveTime;
  uint32_t _priority;

  uint32_t MakeArriveTime(uint32_t rasterY, uint32_t cylinder);

  virtual void InitializeEvent(GraphicsEventQueue *queue) = 0;
  virtual void Handler(uint32_t rasterY, uint32_t cylinder) = 0;
  GraphicsEvent();
};
