#pragma once

class IFpsProvider
{
public:
  virtual unsigned int GetLast50Fps() const = 0;
};
