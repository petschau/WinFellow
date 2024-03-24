#include <cassert>
#include "SchedulerQueue.h"

void SchedulerQueue::Clear()
{
  _events = nullptr;
}

MasterTimestamp SchedulerQueue::GetNextEventCycle() const
{
  return _events->cycle;
}

void SchedulerQueue::RemoveEvent(SchedulerEvent *ev)
{
  bool found = false;

  for (SchedulerEvent *tmp = _events; tmp != nullptr; tmp = tmp->next)
  {
    if (tmp == ev)
    {
      found = true;
      break;
    }
  }

  if (!found)
  {
    return;
  }

  if (ev->prev == nullptr)
  {
    _events = ev->next;
  }
  else
  {
    ev->prev->next = ev->next;
  }

  if (ev->next != nullptr)
  {
    ev->next->prev = ev->prev;
  }

  ev->prev = ev->next = nullptr;
}

// Works also when the queue is empty
void SchedulerQueue::InsertEventWithNullCheck(SchedulerEvent *ev)
{
  if (_events == nullptr)
  {
    assert(ev->cycle.IsEnabledAndValid());

    ev->prev = ev->next = nullptr;
    _events = ev;
  }
  else
  {
    InsertEvent(ev);
  }
}

void SchedulerQueue::InsertEvent(SchedulerEvent *ev)
{
  SchedulerEvent *tmp_prev = nullptr;

  for (SchedulerEvent *tmp = _events; tmp != nullptr; tmp = tmp->next)
  {
    if (ev->cycle < tmp->cycle)
    {
      ev->next = tmp;
      ev->prev = tmp_prev;
      tmp->prev = ev;

      if (tmp_prev == nullptr)
      {
        _events = ev; /* In front */
      }
      else
      {
        tmp_prev->next = ev;
      }

      return;
    }
    tmp_prev = tmp;
  }

  tmp_prev->next = ev; /* At end */
  ev->prev = tmp_prev;
  ev->next = nullptr;
}

SchedulerEvent *SchedulerQueue::PeekNextEvent() const
{
  return _events;
}

SchedulerEvent *SchedulerQueue::PopEvent()
{
  SchedulerEvent *tmp = _events;
  _events = tmp->next;
  _events->prev = nullptr;

  return tmp;
}

void SchedulerQueue::RebaseEvents(MasterTimeOffset offset)
{
  for (SchedulerEvent *ev = _events; ev != nullptr; ev = ev->next)
  {
    ev->cycle -= offset;

    assert(ev->cycle.IsEnabledAndValid());
  }
}

SchedulerQueue::SchedulerQueue() : _events(nullptr)
{
}
