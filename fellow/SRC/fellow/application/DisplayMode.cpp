#include "fellow/application/DisplayMode.h"
#include <sstream>

using namespace std;

DisplayMode::DisplayMode(int id, unsigned int width, unsigned int height, unsigned int bits, unsigned int refresh, const std::string &name) noexcept
  : Id(id), Width(width), Height(height), Bits(bits), Refresh(refresh), Name(name), IsWindowed(false)
{
}

DisplayMode::DisplayMode(int id, unsigned int width, unsigned int height, const std::string &name) noexcept
  : Id(id), Width(width), Height(height), Bits(0), Refresh(0), Name(name), IsWindowed(true)
{
}

DisplayMode::DisplayMode() noexcept : Id(0), Width(0), Height(0), Bits(0), Refresh(0), IsWindowed(false)
{
}

string DisplayMode::ToString() const
{
  ostringstream oss;

  if (IsWindowed)
  {
    oss << Width << ", " << Height << " window (internal id " << Id << " - " << Name << ")";
  }
  else
  {
    oss << Width << ", " << Height << " " << Bits << " bits " << Refresh << " Hz fullscreen (internal id " << Id << " - " << Name << ")";
  }

  return oss.str();
}
