#ifndef GRAPHICS_EVENT_QUEUE_H
#define GRAPHICS_EVENT_QUEUE_H

#include "DEFS.H"
#include "GraphicsEvent.h"

class GraphicsEventQueue
{
  ULO _cycle;
  GraphicsEvent *_events;

public:
  const static ULO GRAPHICS_CYCLE_NONE = 0xffffffff;

  void Clear(void);
  GraphicsEvent* Pop(void);
  void Insert(GraphicsEvent *graphics_event);
  void Remove(GraphicsEvent *graphics_event);

  void Run(ULO untilCycle);

  ULO GetRasterY(void);
  ULO GetRasterX(void);

};

#endif
