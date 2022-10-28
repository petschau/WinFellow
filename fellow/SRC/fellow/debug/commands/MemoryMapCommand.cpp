#include "fellow/debug/commands/MemoryMapCommand.h"
#include "fellow/api/VM.h"

using namespace std;
using namespace fellow::api;
using namespace fellow::api::vm;

MemoryMapResult MemoryMapCommand::Execute(const MemoryMapParameters &parameters)
{
  return MemoryMapResult{.Bits = VM->Memory.GetAddressSpace32Bits() ? 32U : 24U, .Entries = GetMemoryMapEntries()};
}

string MemoryMapCommand::GetMemoryKindDescription(MemoryKind kind) const
{
  switch (kind)
  {
    case MemoryKind::A1000BootstrapROM: return "Amiga 1000 bootstrap ROM";
    case MemoryKind::Autoconfig: return "Autoconfig";
    case MemoryKind::ChipRAM: return "Chip RAM";
    case MemoryKind::ChipRAMMirror: return "Chip RAM mirror";
    case MemoryKind::ChipsetRegisters: return "Chipset registers";
    case MemoryKind::CIA: return "CIA";
    case MemoryKind::ExtendedROM: return "Extended ROM";
    case MemoryKind::FastRAM: return "Fast RAM";
    case MemoryKind::FellowHardfileROM: return "Fellow hardfile device ROM";
    case MemoryKind::FellowHardfileDataArea: return "Fellow hardfile device data area";
    case MemoryKind::FilesystemRTArea: return "Filesystem RTarea";
    case MemoryKind::FilesystemROM: return "Filesystem ROM";
    case MemoryKind::KickstartROM: return "Kickstart ROM";
    case MemoryKind::KickstartROMOverlay: return "Kickstart ROM overlay";
    case MemoryKind::SlowRAM: return "Slow RAM";
    case MemoryKind::RTC: return "Real time clock";
    case MemoryKind::UnmappedRandom: return "Unmapped random";
  }
  return "Unknown memory kind";
}

vector<MemoryMapEntry> MemoryMapCommand::GetMemoryMapEntries()
{
  vector<MemoryMapEntry> entries;
  const auto &descriptors = VM->Memory.GetMemoryMapDescriptors();

  for (const auto &descriptor : descriptors)
  {
    entries.emplace_back(MemoryMapEntry{.StartAddress = descriptor.StartAddress, .EndAddress = descriptor.EndAddress, .Description = GetMemoryKindDescription(descriptor.Kind)});
  }

  return entries;
}
