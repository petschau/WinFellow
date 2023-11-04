#pragma once

#include <cstdint>
#include <string>

namespace fellow::hardfile
{
  struct HardfileMountListEntry
  {
    unsigned int DeviceIndex;
    int PartitionIndex;
    std::string Name;
    uint32_t NameAddress;

    HardfileMountListEntry(unsigned int deviceIndex, int partitionIndex, const std::string &name);
  };
}