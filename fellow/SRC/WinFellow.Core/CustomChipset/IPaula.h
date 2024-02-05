#pragma once

class IPaula
{
public:
  virtual void InitializeInterruptEvent() = 0;

  virtual ~IPaula() = default;
};