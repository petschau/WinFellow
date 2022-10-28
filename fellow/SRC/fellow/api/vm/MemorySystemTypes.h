#pragma once

#include <cstdint>

namespace fellow::api::vm
{
  typedef void (*EmemCardInitFunc)();
  typedef void (*EmemCardMapFunc)(uint32_t mapping);

  typedef uint16_t (*IoReadFunc)(uint32_t address);
  typedef void (*IoWriteFunc)(uint16_t data, uint32_t address);

  typedef uint8_t (*ReadByteFunc)(uint32_t address);
  typedef uint16_t (*ReadWordFunc)(uint32_t address);
  typedef uint32_t (*ReadLongFunc)(uint32_t address);
  typedef void (*WriteByteFunc)(uint8_t data, uint32_t address);
  typedef void (*WriteWordFunc)(uint16_t data, uint32_t address);
  typedef void (*WriteLongFunc)(uint32_t data, uint32_t address);

  enum class MemoryKind
  {
    ChipRAM,
    ChipRAMMirror,
    FastRAM,
    SlowRAM,
    Autoconfig,
    ChipsetRegisters,
    KickstartROM,
    KickstartROMOverlay,
    ExtendedROM,
    A1000BootstrapROM,
    FellowHardfileROM,
    FellowHardfileDataArea,
    FilesystemRTArea,
    FilesystemROM,
    CIA,
    RTC,
    UnmappedRandom
  };

  struct MemoryBankDescriptor
  {
    MemoryKind Kind;
    unsigned int BankNumber;
    unsigned int BaseBankNumber;
    ReadByteFunc ReadByteFunc;
    ReadWordFunc ReadWordFunc;
    ReadLongFunc ReadLongFunc;
    WriteByteFunc WriteByteFunc;
    WriteWordFunc WriteWordFunc;
    WriteLongFunc WriteLongFunc;
    uint8_t *BasePointer;
    bool IsBasePointerWritable;
  };

  struct MemoryMapDescriptor
  {
    MemoryKind Kind;
    uint32_t StartAddress;
    uint32_t EndAddress;
  };
}
