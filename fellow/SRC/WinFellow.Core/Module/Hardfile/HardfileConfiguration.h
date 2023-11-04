#pragma once

#include <string>
#include <vector>

#include "Module/Hardfile/HardfilePartition.h"

namespace Module::Hardfile
{
  struct HardfileConfiguration
  {
    std::string Filename;
    bool Readonly = false;
    HardfileGeometry Geometry;

    std::vector<HardfilePartition> Partitions;

    bool operator==(const HardfileConfiguration &other) const;
    void Clear();
  };
}