#include "HardfileMountListEntry.h"

namespace fellow::hardfile
{
  HardfileMountListEntry::HardfileMountListEntry(unsigned int deviceIndex, int partitionIndex, const std::string &name)
    : DeviceIndex(deviceIndex), PartitionIndex(partitionIndex), Name(name), NameAddress(0)
  {
  }
}