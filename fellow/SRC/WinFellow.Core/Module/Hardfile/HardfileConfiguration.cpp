#include "Module/Hardfile/HardfileConfiguration.h"

namespace Module::Hardfile
{
  bool HardfileConfiguration::operator==(const HardfileConfiguration &other) const
  {
    return Filename == other.Filename && Geometry == other.Geometry && Readonly == other.Readonly;
  }

  void HardfileConfiguration::Clear()
  {
    Filename = "";
    Readonly = false;
    Geometry.Clear();
    Partitions.clear();
  }
}