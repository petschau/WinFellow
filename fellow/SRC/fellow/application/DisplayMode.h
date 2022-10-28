#pragma once

#include <string>

struct DisplayMode
{
  int Id;
  unsigned int Width;
  unsigned int Height;
  unsigned int Bits;
  unsigned int Refresh;
  std::string Name;
  bool IsWindowed;

  DisplayMode(int id, unsigned int width, unsigned int height, unsigned int bits, unsigned int refresh, const std::string &name) noexcept;
  DisplayMode(int id, unsigned int width, unsigned int height, const std::string &name) noexcept;
  DisplayMode() noexcept;

  std::string ToString() const;
};
