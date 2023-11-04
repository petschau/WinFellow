#pragma once

#include <string>
#include "Module/Hardfile/HardfileGeometry.h"

namespace Module::Hardfile
{

  struct HardfilePartition
  {
    std::string PreferredName;
    HardfileGeometry Geometry;
  };
}