#ifndef GRAPHICS_EVENT_H
#define GRAPHICS_EVENT_H

#include "DEFS.H"

class GraphicsEventQueue;

class GraphicsEvent
{
public:
  GraphicsEventQueue *_queue;
  GraphicsEvent *_next;
  GraphicsEvent *_prev;
  ULO _cycle;
  ULO _priority;

  virtual void InitializeEvent(GraphicsEventQueue *queue) = 0;
  virtual void Handler(ULO rasterY, ULO rasterX) = 0;
  GraphicsEvent(void);
};

#endif
