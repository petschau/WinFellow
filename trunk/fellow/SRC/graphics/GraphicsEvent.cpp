#include "GraphicsEventQueue.h"

#ifdef GRAPH2

GraphicsEvent::GraphicsEvent(void)
  : _cycle(GraphicsEventQueue::GRAPHICS_CYCLE_NONE),
    _next(0),
    _prev(0),
    _priority(0)
{
}

#endif
