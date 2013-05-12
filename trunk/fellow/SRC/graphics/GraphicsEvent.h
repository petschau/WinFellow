#ifndef GRAPHICS_EVENT_H
#define GRAPHICS_EVENT_H

#include "DEFS.H"
#include "BUS.H"

class GraphicsEventQueue;

class GraphicsEvent
{
public:
  GraphicsEventQueue *_queue;
  GraphicsEvent *_next;
  GraphicsEvent *_prev;
  ULO _arriveTime;
  ULO _priority;

  ULO MakeArriveTime(ULO rasterY, ULO cylinder);

  virtual void InitializeEvent(GraphicsEventQueue *queue) = 0;
  virtual void Handler(ULO rasterY, ULO cylinder) = 0;
  GraphicsEvent(void);
};

#endif
