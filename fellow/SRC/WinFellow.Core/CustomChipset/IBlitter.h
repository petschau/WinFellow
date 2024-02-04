#pragma once

class IBlitter
{
public:
  virtual void InitializeBlitterEvent() = 0;

  ~IBlitter() = default;
};
