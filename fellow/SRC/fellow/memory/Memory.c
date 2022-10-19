/*=========================================================================*/
/* Fellow                                                                  */
/* Virtual Memory System                                                   */
/*                                                                         */
/* Authors: Petter Schau, Torsten Enderling                                */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2, or (at your option)     */
/* any later version.                                                      */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software Foundation, */
/* Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.          */
/*=========================================================================*/

#ifdef _FELLOW_DEBUG_CRT_MALLOC
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include "fellow/api/defs.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/application/HostRenderer.h"
#include "fellow/cpu/CpuModule.h"
#include "fellow/api/modules/IHardfileHandler.h"
#include "fellow/memory/Memory.h"
#include "fellow/application/WGui.h"
#include "fellow/chipset/rtc.h"
#include "zlib.h" // crc32 function
#include "fellow/api/Services.h"

#ifdef WIN32
#include <tchar.h>
#endif

using namespace std;
using namespace fellow::api::vm;
using namespace fellow::api::modules;
using namespace fellow::api;

/*============================================================================*/
/* Holds configuration for memory                                             */
/*============================================================================*/

ULO memory_chipsize;
ULO memory_fastsize;
ULO memory_slowsize;
bool memory_useautoconfig;
bool memory_addressSpace32Bit;
STR memory_kickimage[CFG_FILENAME_LENGTH];
STR memory_kickimage_ext[CFG_FILENAME_LENGTH];
STR memory_key[256];
bool memory_a1000_wcs = false;              ///< emulate the Amiga 1000 WCS (writable control store)
UBY *memory_a1000_bootstrap = nullptr;      ///< hold A1000 bootstrap ROM, if used
bool memory_a1000_bootstrap_mapped = false; ///< true while A1000 bootstrap ROM mapped to KS area

/*============================================================================*/
/* Holds actual memory                                                        */
/*============================================================================*/

UBY memory_chip[0x200000 + 32];
UBY memory_slow[0x1c0000 + 32];
UBY memory_kick[0x080000 + 32];
UBY *memory_kick_ext = nullptr;
UBY *memory_fast = nullptr;
ULO memory_fast_baseaddress;
ULO memory_fastallocatedsize;
UBY *memory_slow_base;

/*============================================================================*/
/* Autoconfig data                                                            */
/*============================================================================*/

#define EMEM_MAXARDS 4
UBY memory_emem[0x10000];
EmemCardInitFunc memory_ememard_initfunc[EMEM_MAXARDS];
EmemCardMapFunc memory_ememard_mapfunc[EMEM_MAXARDS];
ULO memory_ememardcount;           /* Number of cards */
ULO memory_ememards_finishedcount; /* Current card */

/*============================================================================*/
/* Device memory data                                                         */
/*============================================================================*/

#define MEMORY_DMEM_OFFSET 0xf40000

UBY memory_dmem[65536];
ULO memory_dmemcounter;

/*============================================================================*/
/* Additional Kickstart data                                                  */
/*============================================================================*/

ULO memory_initial_PC;
ULO memory_initial_SP;
BOOLE memory_kickimage_none;
ULO memory_kickimage_size;
ULO memory_kickimage_version;
STR memory_kickimage_versionstr[80];
ULO memory_kickimage_basebank;
ULO memory_kickimage_ext_size = 0;
ULO memory_kickimage_ext_basebank = 0;
const STR *memory_kickimage_versionstrings[14] = {
    "Kickstart, version information unavailable",
    "Kickstart Pre-V1.0",
    "Kickstart V1.0",
    "Kickstart V1.1 (NTSC)",
    "Kickstart V1.1 (PAL)",
    "Kickstart V1.2",
    "Kickstart V1.3",
    "Kickstart V1.3",
    "Kickstart V2.0",
    "Kickstart V2.04",
    "Kickstart V2.1",
    "Kickstart V3.0",
    "Kickstart V3.1",
    "Kickstart Post-V3.1"};
void memoryKickSettingsClear();

/*============================================================================*/
/* Illegal read / write fault information                                     */
/*============================================================================*/

BOOLE memory_fault_read; /* TRUE - read / FALSE - write */
ULO memory_fault_address;

void memoryKickA1000BootstrapSetMapped(const bool);

void memoryWriteByteToPointer(UBY data, UBY *address)
{
  address[0] = data;
}

void memoryWriteWordToPointer(UWO data, UBY *address)
{
  address[0] = (UBY)(data >> 8);
  address[1] = (UBY)data;
}

void memoryWriteLongToPointer(ULO data, UBY *address)
{
  address[0] = (UBY)(data >> 24);
  address[1] = (UBY)(data >> 16);
  address[2] = (UBY)(data >> 8);
  address[3] = (UBY)data;
}

UBY memoryGetRandomByte()
{
  return (UBY)(rand() % 256);
}

UWO memoryGetRandomWord()
{
  return (UWO)(((rand() % 256) << 8) | (rand() % 256));
}

ULO memoryGetRandomLong()
{
  return (ULO)(((rand() % 256) << 24) | ((rand() % 256) << 16) | ((rand() % 256) << 8) | (rand() % 256));
}

// ----------------------------
// Chip read register functions
// ----------------------------

// To simulate noise, return 0 and -1 every second time.
// Why? Bugged demos test write-only registers for various bit-values
// and to break out of loops, both 0 and 1 values must be returned.

UWO rdefault(ULO address)
{
  return memoryGetRandomWord();
}

void wdefault(UWO data, ULO address)
{
}

/*============================================================================*/
/* Table of register read/write functions                                     */
/*============================================================================*/

IoReadFunc memory_iobank_read[257];   // Account for long writes to the last
IoWriteFunc memory_iobank_write[257]; // word

/*============================================================================*/
/* Memory mapping tables                                                      */
/*============================================================================*/

ReadByteFunc memory_bank_readbyte[65536];
ReadWordFunc memory_bank_readword[65536];
ReadLongFunc memory_bank_readlong[65536];
WriteByteFunc memory_bank_writebyte[65536];
WriteWordFunc memory_bank_writeword[65536];
WriteLongFunc memory_bank_writelong[65536];
UBY *memory_bank_pointer[65536]; /* Used by the filesystem */
bool memory_bank_pointer_can_write[65536];
MemoryKind memory_bank_kind[65536];

vector<MemoryMapDescriptor> memoryGetMemoryMapDescriptors()
{
  const unsigned int stopbank = memoryGetAddressSpace32Bit() ? 65536 : 256;
  vector<MemoryMapDescriptor> descriptors;

  MemoryKind kind = memory_bank_kind[0];
  ULO startAddress = 0;
  ULO endAddress = 65535;

  for (unsigned int bank = 1; bank < stopbank; bank++)
  {
    if (memory_bank_kind[bank] != kind)
    {
      descriptors.emplace_back(MemoryMapDescriptor{.Kind = kind, .StartAddress = startAddress, .EndAddress = endAddress});
      kind = memory_bank_kind[bank];
      startAddress = bank * 65536;
      endAddress = startAddress + 65535;
    }
    else
    {
      endAddress += 65536;
    }
  }

  descriptors.emplace_back(MemoryMapDescriptor{.Kind = kind, .StartAddress = startAddress, .EndAddress = endAddress});

  return descriptors;
}

bool memoryIsValidPointerAddress(ULO address, ULO size)
{
  return ((memory_bank_pointer[(address & 0xffffff) >> 16] != nullptr) && (memory_bank_pointer[((address + size) & 0xffffff) >> 16] != nullptr));
}

//============================================================================
// Memory bank mapping functions
//============================================================================

//============================================================================
// Set read and write stubs for a bank, as well as a direct pointer to
// its memory. NULL pointer is passed when the memory must always be
// read through the stubs, like in a bank of regsters where writing
// or reading the value generates side-effects.
// Datadirect is TRUE when data accesses can be made through a pointer
//============================================================================

void memoryBankSet(const MemoryBankDescriptor &bankDescriptor)
{
  unsigned int bankMirrorStride = memoryGetAddressSpace32Bit() ? 65536 : 256;
  unsigned int bankNumber = bankDescriptor.BankNumber & 0xffff;
  unsigned int baseBankNumber = bankDescriptor.BaseBankNumber & 0xffff;

  for (unsigned int i = bankNumber; i < 65536; i += bankMirrorStride)
  {
    memory_bank_kind[i] = bankDescriptor.Kind;
    memory_bank_readbyte[i] = bankDescriptor.ReadByteFunc;
    memory_bank_readword[i] = bankDescriptor.ReadWordFunc;
    memory_bank_readlong[i] = bankDescriptor.ReadLongFunc;
    memory_bank_writebyte[i] = bankDescriptor.WriteByteFunc;
    memory_bank_writeword[i] = bankDescriptor.WriteWordFunc;
    memory_bank_writelong[i] = bankDescriptor.WriteLongFunc;
    memory_bank_pointer_can_write[i] = bankDescriptor.IsBasePointerWritable;

    if (bankDescriptor.BasePointer != nullptr)
    {
      memory_bank_pointer[i] = bankDescriptor.BasePointer - (baseBankNumber * 65536);
    }
    else
    {
      memory_bank_pointer[i] = nullptr;
    }

    baseBankNumber += bankMirrorStride;
  }
}

/*============================================================================*/
/* Clear one bank data to safe "do-nothing" values                            */
/*============================================================================*/

/* Unmapped memory interface */
// Some memory tests use CLR to write and read present memory (Last Ninja 2), so 0 can not be returned, ever.

static UBY memory_previous_unmapped_byte = 0;
UBY memoryUnmappedReadByte(ULO address)
{
  UBY val;
  do
  {
    val = memoryGetRandomByte();
  } while (val == 0 || val == memory_previous_unmapped_byte);

  memory_previous_unmapped_byte = val;
  return val;
}

static UWO memory_previous_unmapped_word = 0;
UWO memoryUnmappedReadWord(ULO address)
{
  UWO val;
  do
  {
    val = memoryGetRandomWord();
  } while (val == 0 || val == memory_previous_unmapped_word);

  memory_previous_unmapped_word = val;
  return val;
}

static ULO memory_previous_unmapped_long = 0;
ULO memoryUnmappedReadLong(ULO address)
{
  ULO val;
  do
  {
    val = memoryGetRandomLong();
  } while (val == 0 || val == memory_previous_unmapped_long);

  memory_previous_unmapped_long = val;
  return val;
}

void memoryUnmappedWriteByte(UBY data, ULO address)
{
  // NOP
}

void memoryUnmappedWriteWord(UWO data, ULO address)
{
  // NOP
}

void memoryUnmappedWriteLong(ULO data, ULO address)
{
  // NOP
}

void memoryBankClear(ULO bank)
{
  memoryBankSet(MemoryBankDescriptor{
      .Kind = MemoryKind::UnmappedRandom,
      .BankNumber = bank,
      .BaseBankNumber = bank,
      .ReadByteFunc = memoryUnmappedReadByte,
      .ReadWordFunc = memoryUnmappedReadWord,
      .ReadLongFunc = memoryUnmappedReadLong,
      .WriteByteFunc = memoryUnmappedWriteByte,
      .WriteWordFunc = memoryUnmappedWriteWord,
      .WriteLongFunc = memoryUnmappedWriteLong,
      .BasePointer = nullptr,
      .IsBasePointerWritable = false});
}

//============================================================================
// Clear all bank data to safe "do-nothing" values
//============================================================================

void memoryBankClearAll()
{
  const ULO hilim = (memoryGetAddressSpace32Bit()) ? 65536 : 256;
  for (ULO bank = 0; bank < hilim; bank++)
  {
    memoryBankClear(bank);
  }
}

/*============================================================================*/
/* Expansion cards autoconfig                                                 */
/*============================================================================*/

/*============================================================================*/
/* Clear the expansion config bank                                            */
/*============================================================================*/

void memoryEmemClear()
{
  memset(memory_emem, 0xff, 65536);
}

//============================================================================
// Add card to table
//============================================================================

void memoryEmemCardAdd(EmemCardInitFunc cardinit, EmemCardMapFunc cardmap)
{
  if (memory_ememardcount < EMEM_MAXARDS)
  {
    memory_ememard_initfunc[memory_ememardcount] = cardinit;
    memory_ememard_mapfunc[memory_ememardcount] = cardmap;
    memory_ememardcount++;
  }
}

/*============================================================================*/
/* Advance the card pointer                                                   */
/*============================================================================*/

void memoryEmemCardNext()
{
  memory_ememards_finishedcount++;
}

/*============================================================================*/
/* Init the current card                                                      */
/*============================================================================*/

void memoryEmemCardInit()
{
  memoryEmemClear();
  if (memory_ememards_finishedcount != memory_ememardcount) memory_ememard_initfunc[memory_ememards_finishedcount]();
}

/*============================================================================*/
/* Map this card                                                              */
/* Mapping is bank number set by AmigaOS                                      */
/*============================================================================*/

void memoryEmemCardMap(ULO mapping)
{
  if (memory_ememards_finishedcount == memory_ememardcount)
    memoryEmemClear();
  else
    memory_ememard_mapfunc[memory_ememards_finishedcount](mapping);
}

/*============================================================================*/
/* Reset card setup                                                           */
/*============================================================================*/

void memoryEmemCardsReset()
{
  memory_ememards_finishedcount = 0;
  memoryEmemCardInit();
}

/*============================================================================*/
/* Clear the card table                                                       */
/*============================================================================*/

void memoryEmemCardsRemove()
{
  memory_ememardcount = memory_ememards_finishedcount = 0;
}

/*============================================================================*/
/* Set a byte in autoconfig space, for initfunc routines                      */
/* so they can make their configuration visible                               */
/*============================================================================*/

void memoryEmemSet(ULO index, ULO value)
{
  index &= 0xffff;
  switch (index)
  {
    case 0:
    case 2:
    case 0x40:
    case 0x42:
      memory_emem[index] = (UBY)(value & 0xf0);
      memory_emem[index + 2] = (UBY)((value & 0xf) << 4);
      break;
    default:
      memory_emem[index] = (UBY)(~(value & 0xf0));
      memory_emem[index + 2] = (UBY)(~((value & 0xf) << 4));
      break;
  }
}

/*============================================================================*/
/* Copy data into emem space                                                  */
/*============================================================================*/

void memoryEmemMirror(ULO emem_offset, UBY *src, ULO size)
{
  memcpy(memory_emem + emem_offset, src, size);
}

/*============================================================================*/
/* Read/Write stubs for autoconfig memory                                     */
/*============================================================================*/

UBY memoryEmemReadByte(ULO address)
{
  UBY *p = memory_emem + (address & 0xffff);
  return memoryReadByteFromPointer(p);
}

UWO memoryEmemReadWord(ULO address)
{
  UBY *p = memory_emem + (address & 0xffff);
  return memoryReadWordFromPointer(p);
}

ULO memoryEmemReadLong(ULO address)
{
  UBY *p = memory_emem + (address & 0xffff);
  return memoryReadLongFromPointer(p);
}

void memoryEmemWriteByte(UBY data, ULO address)
{
  static ULO mapping;

  switch (address & 0xffff)
  {
    case 0x30:
    case 0x32: mapping = data = 0;
    case 0x48:
      mapping = (mapping & 0xff) | (((ULO)data) << 8);
      memoryEmemCardMap(mapping);
      memoryEmemCardNext();
      memoryEmemCardInit();
      break;
    case 0x4a: mapping = (mapping & 0xff00) | ((ULO)data); break;
    case 0x4c:
      memoryEmemCardNext();
      memoryEmemCardInit();
      break;
  }
}

void memoryEmemWriteWord(UWO data, ULO address)
{
}

void memoryEmemWriteLong(ULO data, ULO address)
{
}

//===========================================================================
// Map the autoconfig memory bank into memory
//===========================================================================

void memoryEmemMap()
{
  constexpr ULO bank = 0xe8;

  if (memoryGetKickImageBaseBank() >= 0xf8)
  {
    memoryBankSet(MemoryBankDescriptor{
        .Kind = MemoryKind::Autoconfig,
        .BankNumber = bank,
        .BaseBankNumber = bank,
        .ReadByteFunc = memoryEmemReadByte,
        .ReadWordFunc = memoryEmemReadWord,
        .ReadLongFunc = memoryEmemReadLong,
        .WriteByteFunc = memoryEmemWriteByte,
        .WriteWordFunc = memoryEmemWriteWord,
        .WriteLongFunc = memoryEmemWriteLong,
        .BasePointer = nullptr,
        .IsBasePointerWritable = false});
  }
}

//===================
// End of autoconfig
//===================

/*============================================================================*/
/* dmem is the data area used by the hardfile device to communicate info      */
/* about itself with the Amiga                                                */
/*============================================================================*/

/*============================================================================*/
/* Functions to set data in dmem by the native device drivers                 */
/*============================================================================*/

UBY memoryDmemReadByte(ULO address)
{
  UBY *p = memory_dmem + (address & 0xffff);
  return memoryReadByteFromPointer(p);
}

UWO memoryDmemReadWord(ULO address)
{
  UBY *p = memory_dmem + (address & 0xffff);
  return memoryReadWordFromPointer(p);
}

ULO memoryDmemReadLong(ULO address)
{
  UBY *p = memory_dmem + (address & 0xffff);
  return memoryReadLongFromPointer(p);
}

void memoryDmemWriteByte(UBY data, ULO address)
{
  // NOP
}

void memoryDmemWriteWord(UWO data, ULO address)
{
  // NOP
}

/*============================================================================*/
/* Writing a long to $f40000 runs a native function                           */
/*============================================================================*/

void memoryDmemWriteLong(ULO data, ULO address)
{
  if ((address & 0xffffff) == 0xf40000)
  {
    HardfileHandler->Do(data);
  }
}

void memoryDmemClear()
{
  memset(memory_dmem, 0, 4096);
}

void memoryDmemSetCounter(ULO val)
{
  memory_dmemcounter = val;
}

ULO memoryDmemGetCounterWithoutOffset()
{
  return memory_dmemcounter;
}

ULO memoryDmemGetCounter()
{
  return memory_dmemcounter + MEMORY_DMEM_OFFSET;
}

void memoryDmemSetString(const STR *st)
{
  strcpy((STR *)(memory_dmem + memory_dmemcounter), st);
  memory_dmemcounter += (ULO)strlen(st) + 1;
  if (memory_dmemcounter & 1) memory_dmemcounter++;
}

void memoryDmemSetByte(UBY data)
{
  memory_dmem[memory_dmemcounter++] = data;
}

void memoryDmemSetWord(UWO data)
{
  memoryWriteWordToPointer(data, memory_dmem + memory_dmemcounter);
  memory_dmemcounter += 2;
}

void memoryDmemSetLong(ULO data)
{
  memoryWriteLongToPointer(data, memory_dmem + memory_dmemcounter);
  memory_dmemcounter += 4;
}

void memoryDmemSetLongNoCounter(ULO data, ULO offset)
{
  memoryWriteLongToPointer(data, memory_dmem + offset);
}

void memoryDmemMap()
{
  constexpr ULO bank = 0xf4;
  ;

  if (memory_useautoconfig && (memory_kickimage_basebank >= 0xf8))
  {
    memoryBankSet(MemoryBankDescriptor{
        .Kind = MemoryKind::FellowHardfileDataArea,
        .BankNumber = bank,
        .BaseBankNumber = bank,
        .ReadByteFunc = memoryDmemReadByte,
        .ReadWordFunc = memoryDmemReadWord,
        .ReadLongFunc = memoryDmemReadLong,
        .WriteByteFunc = memoryDmemWriteByte,
        .WriteWordFunc = memoryDmemWriteWord,
        .WriteLongFunc = memoryDmemWriteLong,
        .BasePointer = memory_dmem,
        .IsBasePointerWritable = false});
  }
}

/*============================================================================*/
/* Converts an address to a direct pointer to memory. Used by hardfile device */
/*============================================================================*/

UBY *memoryAddressToPtr(ULO address)
{
  UBY *result = memory_bank_pointer[address >> 16];

  if (result != nullptr) result += address;
  return result;
}

/*============================================================================*/
/* Chip memory handling                                                       */
/*============================================================================*/

UBY memoryChipReadByte(ULO address)
{
  UBY *p = memory_chip + chipsetMaskAddress(address);
  return memoryReadByteFromPointer(p);
}

UWO memoryChipReadWord(ULO address)
{
  UBY *p = memory_chip + chipsetMaskAddress(address);
  return memoryReadWordFromPointer(p);
}

ULO memoryChipReadLong(ULO address)
{
  UBY *p = memory_chip + chipsetMaskAddress(address);
  return memoryReadLongFromPointer(p);
}

void memoryChipWriteByte(UBY data, ULO address)
{
  UBY *p = memory_chip + chipsetMaskAddress(address);
  memoryWriteByteToPointer(data, p);
}

void memoryChipWriteWord(UWO data, ULO address)
{
  UBY *p = memory_chip + chipsetMaskAddress(address);
  memoryWriteWordToPointer(data, p);
}

void memoryChipWriteLong(ULO data, ULO address)
{
  UBY *p = memory_chip + chipsetMaskAddress(address);
  memoryWriteLongToPointer(data, p);
}

void memoryChipClear()
{
  memset(memory_chip, 0, memoryGetChipSize());
}

UBY memoryOverlayReadByte(ULO address)
{
  UBY *p = memory_kick + (address & 0xffffff);
  return memoryReadByteFromPointer(p);
}

UWO memoryOverlayReadWord(ULO address)
{
  UBY *p = memory_kick + (address & 0xffffff);
  return memoryReadWordFromPointer(p);
}

ULO memoryOverlayReadLong(ULO address)
{
  UBY *p = memory_kick + (address & 0xffffff);
  return memoryReadLongFromPointer(p);
}

void memoryOverlayWriteByte(UBY data, ULO address)
{
  // NOP
}

void memoryOverlayWriteWord(UWO data, ULO address)
{
  // NOP
}

void memoryOverlayWriteLong(ULO data, ULO address)
{
  // NOP
}

ULO memoryChipGetLastBank()
{
  ULO lastbank = memoryGetChipSize() >> 16;

  if (chipsetGetECS())
  {
    return (lastbank <= 32) ? lastbank : 32;
  }

  // OCS
  return (lastbank <= 8) ? lastbank : 8;
}

void memoryChipMap(const bool overlay)
{
  // Build first, "real" chipmem area
  if (overlay)
  {
    // 256k ROMs are already mirrored once in the memory_kick area
    // Map entire 512k ROM area to $0
    for (ULO bank = 0; bank < 8; bank++)
    {
      memoryBankSet(MemoryBankDescriptor{
          .Kind = MemoryKind::KickstartROMOverlay,
          .BankNumber = bank,
          .BaseBankNumber = 0,
          .ReadByteFunc = memoryOverlayReadByte,
          .ReadWordFunc = memoryOverlayReadWord,
          .ReadLongFunc = memoryOverlayReadLong,
          .WriteByteFunc = memoryOverlayWriteByte,
          .WriteWordFunc = memoryOverlayWriteWord,
          .WriteLongFunc = memoryOverlayWriteLong,
          .BasePointer = memory_kick,
          .IsBasePointerWritable = false});
    }
  }

  // Map 512k to 2MB of chip memory, possibly skipping the overlay area
  const ULO firstbank = overlay ? 8 : 0;
  const ULO lastbank = memoryChipGetLastBank();
  for (ULO bank = firstbank; bank < lastbank; bank++)
  {
    memoryBankSet(MemoryBankDescriptor{
        .Kind = MemoryKind::ChipRAM,
        .BankNumber = bank,
        .BaseBankNumber = 0,
        .ReadByteFunc = memoryChipReadByte,
        .ReadWordFunc = memoryChipReadWord,
        .ReadLongFunc = memoryChipReadLong,
        .WriteByteFunc = memoryChipWriteByte,
        .WriteWordFunc = memoryChipWriteWord,
        .WriteLongFunc = memoryChipWriteLong,
        .BasePointer = memory_chip,
        .IsBasePointerWritable = true});
  }

  // In the case of 256k chip memory and not overlaying, clear the second 256k map
  // as this area could have been mapped for the ROM overlay
  if (lastbank < 8 && !overlay)
  {
    for (ULO bank = lastbank; bank < 8; ++bank)
    {
      memoryBankClear(bank);
    }
  }

  if (!chipsetGetECS())
  {
    // OCS: Make 3 more copies of the chipram at $80000, $100000 and $180000
    for (ULO i = 1; i < 4; ++i)
    {
      const ULO bank_start = 8 * i;
      const ULO bank_end = bank_start + lastbank;
      for (ULO bank = bank_start; bank < bank_end; bank++)
      {
        memoryBankSet(MemoryBankDescriptor{
            .Kind = MemoryKind::ChipRAMMirror,
            .BankNumber = bank,
            .BaseBankNumber = bank_start,
            .ReadByteFunc = memoryChipReadByte,
            .ReadWordFunc = memoryChipReadWord,
            .ReadLongFunc = memoryChipReadLong,
            .WriteByteFunc = memoryChipWriteByte,
            .WriteWordFunc = memoryChipWriteWord,
            .WriteLongFunc = memoryChipWriteLong,
            .BasePointer = memory_chip,
            .IsBasePointerWritable = true});
      }
    }
  }
}

/*============================================================================*/
/* Fast memory handling                                                       */
/*============================================================================*/

UBY memoryFastReadByte(ULO address)
{
  UBY *p = memory_fast + ((address & 0xffffff) - memory_fast_baseaddress);
  return memoryReadByteFromPointer(p);
}

UWO memoryFastReadWord(ULO address)
{
  UBY *p = memory_fast + ((address & 0xffffff) - memory_fast_baseaddress);
  return memoryReadWordFromPointer(p);
}

ULO memoryFastReadLong(ULO address)
{
  UBY *p = memory_fast + ((address & 0xffffff) - memory_fast_baseaddress);
  return memoryReadLongFromPointer(p);
}

void memoryFastWriteByte(UBY data, ULO address)
{
  UBY *p = memory_fast + ((address & 0xffffff) - memory_fast_baseaddress);
  memoryWriteByteToPointer(data, p);
}

void memoryFastWriteWord(UWO data, ULO address)
{
  UBY *p = memory_fast + ((address & 0xffffff) - memory_fast_baseaddress);
  memoryWriteWordToPointer(data, p);
}

void memoryFastWriteLong(ULO data, ULO address)
{
  UBY *p = memory_fast + ((address & 0xffffff) - memory_fast_baseaddress);
  memoryWriteLongToPointer(data, p);
}

/*============================================================================*/
/* Set up autoconfig values for fastmem card                                  */
/*============================================================================*/

void memoryFastCardInit()
{
  if (memoryGetFastSize() == 0x100000)
    memoryEmemSet(0, 0xe5);
  else if (memoryGetFastSize() == 0x200000)
    memoryEmemSet(0, 0xe6);
  else if (memoryGetFastSize() == 0x400000)
    memoryEmemSet(0, 0xe7);
  else if (memoryGetFastSize() == 0x800000)
    memoryEmemSet(0, 0xe0);
  memoryEmemSet(8, 128);
  memoryEmemSet(4, 1);
  memoryEmemSet(0x10, 2011 >> 8);
  memoryEmemSet(0x14, 2011 & 0xf);
  memoryEmemSet(0x18, 0);
  memoryEmemSet(0x1c, 0);
  memoryEmemSet(0x20, 0);
  memoryEmemSet(0x24, 1);
  memoryEmemSet(0x28, 0);
  memoryEmemSet(0x2c, 0);
  memoryEmemSet(0x40, 0);
}

/*============================================================================*/
/* Allocate memory for the fast card memory                                   */
/*============================================================================*/

void memoryFastClear()
{
  if (memory_fast != nullptr) memset(memory_fast, 0, memoryGetFastSize());
}

void memoryFastFree()
{
  if (memory_fast != nullptr)
  {
    free(memory_fast);
    memory_fast = nullptr;
    memory_fast_baseaddress = 0;
    memorySetFastAllocatedSize(0);
  }
}

void memoryFastAllocate()
{
  if (memoryGetFastSize() != memoryGetFastAllocatedSize())
  {
    memoryFastFree();
    memory_fast = (UBY *)malloc(memoryGetFastSize());
    if (memory_fast == nullptr)
      memorySetFastSize(0);
    else
      memoryFastClear();
    memorySetFastAllocatedSize((memory_fast == nullptr) ? 0 : memoryGetFastSize());
  }
}

//============================================================================
// Map fastcard.
//============================================================================

ULO memoryFastGetLastBank()
{
  if (memoryGetFastSize() > 0x800000)
  {
    return 0xa00000 >> 16;
  }

  return (memory_fast_baseaddress + memoryGetFastSize()) >> 16;
}

void memoryFastCardMap(ULO mapping)
{
  memory_fast_baseaddress = (mapping >> 8) << 16;
  const ULO lastbank = memoryFastGetLastBank();

  for (ULO bank = memory_fast_baseaddress >> 16; bank < lastbank; bank++)
  {
    memoryBankSet(MemoryBankDescriptor{
        .Kind = MemoryKind::FastRAM,
        .BankNumber = bank,
        .BaseBankNumber = memory_fast_baseaddress >> 16,
        .ReadByteFunc = memoryFastReadByte,
        .ReadWordFunc = memoryFastReadWord,
        .ReadLongFunc = memoryFastReadLong,
        .WriteByteFunc = memoryFastWriteByte,
        .WriteWordFunc = memoryFastWriteWord,
        .WriteLongFunc = memoryFastWriteLong,
        .BasePointer = memory_fast,
        .IsBasePointerWritable = true});
  }

  memset(memory_fast, 0, memoryGetFastSize());
}

void memoryFastCardAdd()
{
  if (memoryGetFastSize() != 0) memoryEmemCardAdd(memoryFastCardInit, memoryFastCardMap);
}

/*============================================================================*/
/* Slow memory handling                                                       */
/*============================================================================*/

UBY memorySlowReadByte(ULO address)
{
  UBY *p = memory_slow_base + ((address & 0xffffff) - 0xc00000);
  return memoryReadByteFromPointer(p);
}

UWO memorySlowReadWord(ULO address)
{
  UBY *p = memory_slow_base + ((address & 0xffffff) - 0xc00000);
  return memoryReadWordFromPointer(p);
}

ULO memorySlowReadLong(ULO address)
{
  UBY *p = memory_slow_base + ((address & 0xffffff) - 0xc00000);
  return memoryReadLongFromPointer(p);
}

void memorySlowWriteByte(UBY data, ULO address)
{
  UBY *p = memory_slow_base + ((address & 0xffffff) - 0xc00000);
  memoryWriteByteToPointer(data, p);
}

void memorySlowWriteWord(UWO data, ULO address)
{
  UBY *p = memory_slow_base + ((address & 0xffffff) - 0xc00000);
  memoryWriteWordToPointer(data, p);
}

void memorySlowWriteLong(ULO data, ULO address)
{
  UBY *p = memory_slow_base + ((address & 0xffffff) - 0xc00000);
  memoryWriteLongToPointer(data, p);
}

bool memorySlowMapAsChip()
{
  return chipsetGetECS() && memoryGetChipSize() == 0x80000 && memoryGetSlowSize() == 0x80000;
}

void memorySlowClear()
{
  memset(memory_slow, 0, memoryGetSlowSize());
  if (memorySlowMapAsChip())
  {
    memset(memory_chip + 0x80000, 0, memoryGetSlowSize());
  }
}

ULO memorySlowGetLastBank()
{
  if (memoryGetSlowSize() > 0x1c0000)
  {
    return 0xdc0000 >> 16;
  }

  return (0xc00000 + memoryGetSlowSize()) >> 16;
}

void memorySlowMap()
{
  constexpr ULO startbank = 0xc0;
  const ULO lastbank = memorySlowGetLastBank();

  // Special config on ECS with 512k chip + 512k slow, chips see slow mem at $80000
  memory_slow_base = (memorySlowMapAsChip()) ? (memory_chip + 0x80000) : memory_slow;

  for (ULO bank = startbank; bank < lastbank; bank++)
  {
    memoryBankSet(MemoryBankDescriptor{
        .Kind = MemoryKind::SlowRAM,
        .BankNumber = bank,
        .BaseBankNumber = startbank,
        .ReadByteFunc = memorySlowReadByte,
        .ReadWordFunc = memorySlowReadWord,
        .ReadLongFunc = memorySlowReadLong,
        .WriteByteFunc = memorySlowWriteByte,
        .WriteWordFunc = memorySlowWriteWord,
        .WriteLongFunc = memorySlowWriteLong,
        .BasePointer = memory_slow_base,
        .IsBasePointerWritable = true});
  }
}

/*============================================================================*/
/* Probably a disk controller, we need it to pass dummy values to get past    */
/* the bootstrap of some Kickstart versions.                                  */
/*============================================================================*/

UBY memoryMysteryReadByte(ULO address)
{
  return memoryUnmappedReadByte(address);
}

UWO memoryMysteryReadWord(ULO address)
{
  return memoryUnmappedReadWord(address);
}

ULO memoryMysteryReadLong(ULO address)
{
  return memoryUnmappedReadLong(address);
}

void memoryMysteryWriteByte(UBY data, ULO address)
{
}

void memoryMysteryWriteWord(UWO data, ULO address)
{
}

void memoryMysteryWriteLong(ULO data, ULO address)
{
}

void memoryMysteryMap()
{
  memoryBankSet(MemoryBankDescriptor{
      .Kind = MemoryKind::UnmappedRandom,
      .BankNumber = 0xe9,
      .BaseBankNumber = 0xe9,
      .ReadByteFunc = memoryMysteryReadByte,
      .ReadWordFunc = memoryMysteryReadWord,
      .ReadLongFunc = memoryMysteryReadLong,
      .WriteByteFunc = memoryMysteryWriteByte,
      .WriteWordFunc = memoryMysteryWriteWord,
      .WriteLongFunc = memoryMysteryWriteLong,
      .BasePointer = nullptr,
      .IsBasePointerWritable = false});

  memoryBankSet(MemoryBankDescriptor{
      .Kind = MemoryKind::UnmappedRandom,
      .BankNumber = 0xde,
      .BaseBankNumber = 0xde,
      .ReadByteFunc = memoryMysteryReadByte,
      .ReadWordFunc = memoryMysteryReadWord,
      .ReadLongFunc = memoryMysteryReadLong,
      .WriteByteFunc = memoryMysteryWriteByte,
      .WriteWordFunc = memoryMysteryWriteWord,
      .WriteLongFunc = memoryMysteryWriteLong,
      .BasePointer = nullptr,
      .IsBasePointerWritable = false});
}

/*============================================================================*/
/* IO Registers                                                               */
/*============================================================================*/

UBY memoryIoReadByte(ULO address)
{
  ULO adr = address & 0x1fe;
  if (address & 0x1)
  { // Odd address
    return (UBY)memory_iobank_read[adr >> 1](adr);
  }
  else
  { // Even address
    return (UBY)(memory_iobank_read[adr >> 1](adr) >> 8);
  }
}

UWO memoryIoReadWord(ULO address)
{
  return memory_iobank_read[(address & 0x1fe) >> 1](address & 0x1fe);
}

ULO memoryIoReadLong(ULO address)
{
  ULO adr = address & 0x1fe;
  ULO r1 = (ULO)memory_iobank_read[adr >> 1](adr);
  ULO r2 = (ULO)memory_iobank_read[(adr + 2) >> 1](adr + 2);
  return (r1 << 16) | r2;
}

void memoryIoWriteByte(UBY data, ULO address)
{
  ULO adr = address & 0x1fe;
  if (address & 0x1)
  { // Odd address
    memory_iobank_write[adr >> 1]((UWO)data, adr);
  }
  else
  { // Even address
    memory_iobank_write[adr >> 1](((UWO)data) << 8, adr);
  }
}

void memoryIoWriteWord(UWO data, ULO address)
{
  ULO adr = address & 0x1fe;
  memory_iobank_write[adr >> 1](data, adr);
}

void memoryIoWriteLong(ULO data, ULO address)
{
  ULO adr = address & 0x1fe;
  memory_iobank_write[adr >> 1]((UWO)(data >> 16), adr);
  memory_iobank_write[(adr + 2) >> 1]((UWO)data, adr + 2);
}

ULO memoryIoGetStartBank()
{
  if (memoryGetSlowSize() > 0x1c0000)
  {
    return 0xdc0000 >> 16;
  }

  return (0xc00000 + memoryGetSlowSize()) >> 16;
}

void memoryIoMap()
{
  const ULO startbank = memoryIoGetStartBank();
  constexpr ULO lastbank = 0xe0;

  for (ULO bank = startbank; bank < lastbank; bank++)
  {
    memoryBankSet(MemoryBankDescriptor{
        .Kind = MemoryKind::ChipsetRegisters,
        .BankNumber = bank,
        .BaseBankNumber = startbank,
        .ReadByteFunc = memoryIoReadByte,
        .ReadWordFunc = memoryIoReadWord,
        .ReadLongFunc = memoryIoReadLong,
        .WriteByteFunc = memoryIoWriteByte,
        .WriteWordFunc = memoryIoWriteWord,
        .WriteLongFunc = memoryIoWriteLong,
        .BasePointer = nullptr,
        .IsBasePointerWritable = false});
  }
}

/*===========================================================================*/
/* Initializes one entry in the IO register access table                     */
/*===========================================================================*/

void memorySetIoReadStub(ULO index, IoReadFunc ioreadfunction)
{
  memory_iobank_read[index >> 1] = ioreadfunction;
}

void memorySetIoWriteStub(ULO index, IoWriteFunc iowritefunction)
{
  memory_iobank_write[index >> 1] = iowritefunction;
}

/*===========================================================================*/
/* Clear all IO-register accessors                                           */
/*===========================================================================*/

void memoryIoClear()
{
  // Array has 257 elements to account for long writes to the last address.
  for (ULO i = 0; i <= 512; i += 2)
  {
    memorySetIoReadStub(i, rdefault);
    memorySetIoWriteStub(i, wdefault);
  }
}

/*============================================================================*/
/* Kickstart handling                                                         */
/*============================================================================*/

/*============================================================================*/
/* Map the Kickstart image into Amiga memory                                  */
/*============================================================================*/

UBY memoryKickReadByte(ULO address)
{
  UBY *p = memory_kick + ((address & 0xffffff) - 0xf80000);
  return memoryReadByteFromPointer(p);
}

UBY memoryKickExtendedReadByte(ULO address)
{
  UBY *p = memory_kick_ext + ((address & 0xffffff) - 0xe00000);
  return memoryReadByteFromPointer(p);
}

UWO memoryKickReadWord(ULO address)
{
  UBY *p = memory_kick + ((address & 0xffffff) - 0xf80000);
  return memoryReadWordFromPointer(p);
}

UWO memoryKickExtendedReadWord(ULO address)
{
  UBY *p = memory_kick_ext + ((address & 0xffffff) - 0xe00000);
  return memoryReadWordFromPointer(p);
}

ULO memoryKickReadLong(ULO address)
{
  UBY *p = memory_kick + ((address & 0xffffff) - 0xf80000);
  return memoryReadLongFromPointer(p);
}

ULO memoryKickExtendedReadLong(ULO address)
{
  UBY *p = memory_kick_ext + ((address & 0xffffff) - 0xe00000);
  return memoryReadLongFromPointer(p);
}

void memoryKickWriteByte(UBY data, ULO address)
{
  // NOP
}

void memoryKickExtendedWriteByte(UBY data, ULO address)
{
  // NOP
}

void memoryKickWriteWord(UWO data, ULO address)
{
  // NOP
}

void memoryKickExtendedWriteWord(UWO data, ULO address)
{
  // NOP
}

void memoryKickWriteLong(ULO data, ULO address)
{
  // NOP
}

void memoryKickExtendedWriteLong(ULO data, ULO address)
{
  // NOP
}

void memoryKickWriteByteA1000WCS(UBY data, ULO address)
{
  if (address >= 0xfc0000)
  {
    address = (address & 0xffffff) - 0xf80000;
    UBY *p = memory_kick + address;
    memoryWriteByteToPointer(data, p);
  }
  else
    memoryKickA1000BootstrapSetMapped(false);
}

void memoryKickWriteWordA1000WCS(UWO data, ULO address)
{
  if (address >= 0xfc0000)
  {
    address = (address & 0xffffff) - 0xf80000;
    UBY *p = memory_kick + address;
    memoryWriteWordToPointer(data, p);
  }
  else
    memoryKickA1000BootstrapSetMapped(false);
}

void memoryKickWriteLongA1000WCS(ULO data, ULO address)
{
  if (address >= 0xfc0000)
  {
    address = (address & 0xffffff) - 0xf80000;
    UBY *p = memory_kick + address;
    memoryWriteLongToPointer(data, p);
  }
  else
    memoryKickA1000BootstrapSetMapped(false);
}

void memoryKickMap()
{
  const ULO basebank = memory_kickimage_basebank & 0xf8;
  const ULO lastbank = basebank + 8;

  for (ULO bank = basebank; bank < lastbank; bank++)
  {
    if (!memory_a1000_bootstrap_mapped)
    {
      memoryBankSet(MemoryBankDescriptor{
          .Kind = MemoryKind::KickstartROM,
          .BankNumber = bank,
          .BaseBankNumber = memory_kickimage_basebank,
          .ReadByteFunc = memoryKickReadByte,
          .ReadWordFunc = memoryKickReadWord,
          .ReadLongFunc = memoryKickReadLong,
          .WriteByteFunc = memoryKickWriteByte,
          .WriteWordFunc = memoryKickWriteWord,
          .WriteLongFunc = memoryKickWriteLong,
          .BasePointer = memory_kick,
          .IsBasePointerWritable = false});
    }
    else
    {
      memoryBankSet(MemoryBankDescriptor{
          .Kind = MemoryKind::A1000BootstrapROM,
          .BankNumber = bank,
          .BaseBankNumber = memory_kickimage_basebank,
          .ReadByteFunc = memoryKickReadByte,
          .ReadWordFunc = memoryKickReadWord,
          .ReadLongFunc = memoryKickReadLong,
          .WriteByteFunc = memoryKickWriteByteA1000WCS,
          .WriteWordFunc = memoryKickWriteWordA1000WCS,
          .WriteLongFunc = memoryKickWriteLongA1000WCS,
          .BasePointer = memory_kick,
          .IsBasePointerWritable = false});
    }
  }
}

void memoryKickExtendedMap()
{
  if (memory_kickimage_ext_size == 0) return;

  const ULO basebank = memory_kickimage_ext_basebank;
  const ULO numbanks = memory_kickimage_ext_size / 65536;
  const ULO lastbank = (basebank + numbanks);

  for (ULO bank = basebank; bank < lastbank; bank++)
  {
    memoryBankSet(MemoryBankDescriptor{
        .Kind = MemoryKind::ExtendedROM,
        .BankNumber = bank,
        .BaseBankNumber = basebank,
        .ReadByteFunc = memoryKickExtendedReadByte,
        .ReadWordFunc = memoryKickExtendedReadWord,
        .ReadLongFunc = memoryKickExtendedReadLong,
        .WriteByteFunc = memoryKickExtendedWriteByte,
        .WriteWordFunc = memoryKickExtendedWriteWord,
        .WriteLongFunc = memoryKickExtendedWriteLong,
        .BasePointer = memory_kick_ext,
        .IsBasePointerWritable = false});
  }
}

void memoryKickA1000BootstrapSetMapped(const bool bBootstrapMapped)
{
  if (!memory_a1000_wcs || !memory_a1000_bootstrap) return;

  Service->Log.AddLog("memoryKickSetA1000BootstrapMapped(%s)\n", bBootstrapMapped ? "true" : "false");

  if (bBootstrapMapped)
  {
    memcpy(memory_kick, memory_a1000_bootstrap, 262144);
    memory_kickimage_version = 0;
  }
  else
  {
    memcpy(memory_kick, memory_kick + 262144, 262144);
    memory_kickimage_version = (memory_kick[262144 + 12] << 8) | memory_kick[262144 + 13];
    if (memory_kickimage_version == 0xffff) memory_kickimage_version = 0;
  }

  if (bBootstrapMapped != memory_a1000_bootstrap_mapped)
  {
    memory_a1000_bootstrap_mapped = bBootstrapMapped;
    memoryKickMap();
  }
}

void memoryKickA1000BootstrapFree()
{
  if (memory_a1000_bootstrap != nullptr)
  {
    free(memory_a1000_bootstrap);
    memory_a1000_bootstrap = nullptr;
    memory_a1000_bootstrap_mapped = false;
    memory_a1000_wcs = false;
  }
}

/*============================================================================*/
/* An error occured during loading a kickstart file. Uses GUI to display      */
/* an errorstring.                                                            */
/*============================================================================*/

void memoryKickError(ULO errorcode, ULO data)
{
  static STR error1[80], error2[160], error3[160];

  sprintf(error1, "Kickstart file could not be loaded");
  sprintf(error2, "%s", memory_kickimage);
  error3[0] = '\0';
  switch (errorcode)
  {
    case MEMORY_ROM_ERROR_SIZE: sprintf(error3, "Illegal size: %u bytes, size must be either 8kB (A1000 bootstrap ROM), 256kB or 512kB.", data); break;
    case MEMORY_ROM_ERROR_AMIROM_VERSION: sprintf(error3, "Unsupported encryption method, version found was %u", data); break;
    case MEMORY_ROM_ERROR_AMIROM_READ: sprintf(error3, "Read error in encrypted Kickstart or keyfile"); break;
    case MEMORY_ROM_ERROR_KEYFILE: sprintf(error3, "Unable to access keyfile %s", memory_key); break;
    case MEMORY_ROM_ERROR_EXISTS_NOT: sprintf(error3, "File does not exist"); break;
    case MEMORY_ROM_ERROR_FILE: sprintf(error3, "File is a directory"); break;
    case MEMORY_ROM_ERROR_KICKDISK_NOT: sprintf(error3, "The ADF-image is not a kickdisk"); break;
    case MEMORY_ROM_ERROR_CHECKSUM: sprintf(error3, "The Kickstart image has a checksum error, checksum is %X", data); break;
    case MEMORY_ROM_ERROR_KICKDISK_SUPER: sprintf(error3, "The ADF-image contains a superkickstart. Fellow can not handle it."); break;
    case MEMORY_ROM_ERROR_BAD_BANK: sprintf(error3, "The ROM has a bad baseaddress: %X", memory_kickimage_basebank * 0x10000); break;
  }
  Service->Log.AddLogRequester(FELLOW_REQUESTER_TYPE::FELLOW_REQUESTER_TYPE_ERROR, "%s\n%s\n%s\n", error1, error2, error3);
  memoryKickSettingsClear();
}

/*============================================================================*/
/* Returns the checksum of the current kickstart image.                       */
/*============================================================================*/

ULO memoryKickChksum()
{
  ULO lastsum;

  ULO sum = lastsum = 0;
  for (ULO i = 0; i < 0x80000; i += 4)
  {
    UBY *p = memory_kick + i;
    sum += memoryReadLongFromPointer(p);
    if (sum < lastsum) sum++;
    lastsum = sum;
  }
  return ~sum;
}

/*============================================================================*/
/* Identifies a loaded Kickstart                                              */
/*============================================================================*/

STR *memoryKickIdentify(STR *s)
{
  UBY *rom = memory_kick;

  ULO ver = (rom[12] << 8) | rom[13];
  ULO rev = (rom[14] << 8) | rom[15];
  if (ver == 65535)
    memory_kickimage_version = 28;
  else if (ver < 29)
    memory_kickimage_version = 29;
  else if (ver > 41)
    memory_kickimage_version = 41;
  else
    memory_kickimage_version = ver;
  sprintf(s, "%s (%u.%u)", memory_kickimage_versionstrings[memory_kickimage_version - 28], ver, rev);
  return s;
}

/*============================================================================*/
/* Verifies that a loaded Kickstart is OK                                     */
/*============================================================================*/

void memoryKickOK()
{
  ULO chksum;
  bool bVerifyChecksum = !memory_a1000_wcs;

  if (bVerifyChecksum && ((chksum = memoryKickChksum()) != 0))
    memoryKickError(MEMORY_ROM_ERROR_CHECKSUM, chksum);
  else
  {
    ULO basebank = memory_kick[5];
    if ((basebank == 0xf8) || (basebank == 0xfc))
    {
      memory_kickimage_basebank = basebank;
      memory_kickimage_none = FALSE;
      memoryKickIdentify(memory_kickimage_versionstr);
      memory_initial_PC = memoryReadLongFromPointer((memory_kick + 4));
      memory_initial_SP = memoryReadLongFromPointer(memory_kick);
    }
    else
      memoryKickError(MEMORY_ROM_ERROR_BAD_BANK, basebank);
  }
}

/*============================================================================*/
/* Returns size of decoded kickstart                                          */
/*============================================================================*/

int memoryKickDecodeAF(STR *filename, STR *keyfile, UBY *memory_kick)
{
  STR *keybuffer = nullptr;
  ULO keysize, filesize = 0, keypos = 0, c;
  FILE *KF, *RF;

  /* Read key */

  if ((KF = fopen(keyfile, "rb")) != nullptr)
  {
    fseek(KF, 0, SEEK_END);
    keysize = ftell(KF);
    keybuffer = (STR *)malloc(keysize);
    if (keybuffer != nullptr)
    {
      fseek(KF, 0, SEEK_SET);
      fread(keybuffer, 1, keysize, KF);
    }
    fclose(KF);
  }
  else
  {
#ifdef WIN32
    HMODULE hAmigaForeverDLL = nullptr;
    STR *strLibName = TEXT("amigaforever.dll");
    STR strPath[CFG_FILENAME_LENGTH] = "";
    STR strAmigaForeverRoot[CFG_FILENAME_LENGTH] = "";
    DWORD dwRet = 0;

    // the preferred way to locate the DLL is by relative path, so it is
    // of a matching version (DVD/portable installation with newer version
    // than what is installed)
    if (Service->Fileops.GetWinFellowInstallationPath(strPath, CFG_FILENAME_LENGTH))
    {
      strncat(strPath, "\\..\\Player\\", 11);
      strncat(strPath, strLibName, strlen(strLibName) + 1);
      hAmigaForeverDLL = LoadLibrary(strPath);
    }

    if (!hAmigaForeverDLL)
    {
      // DLL not found via relative path, fallback to env. variable
      dwRet = GetEnvironmentVariable("AMIGAFOREVERROOT", strAmigaForeverRoot, CFG_FILENAME_LENGTH);
      if ((dwRet > 0) && strAmigaForeverRoot)
      {
        TCHAR strTemp[CFG_FILENAME_LENGTH];
        _tcscpy(strTemp, strAmigaForeverRoot);
        if (strTemp[_tcslen(strTemp) - 1] == '/' || strTemp[_tcslen(strTemp) - 1] == '\\') _tcscat(strTemp, TEXT("\\"));
        _stprintf(strPath, TEXT("%sPlayer\\%s"), strTemp, strLibName);
        hAmigaForeverDLL = LoadLibrary(strPath);
      }
    }

    if (hAmigaForeverDLL)
    {
      typedef DWORD(STDAPICALLTYPE * PFN_GetKey)(LPVOID lpvBuffer, DWORD dwSize);

      PFN_GetKey pfnGetKey = (PFN_GetKey)GetProcAddress(hAmigaForeverDLL, "GetKey");
      if (pfnGetKey)
      {
        keysize = pfnGetKey(nullptr, 0);
        if (keysize)
        {
          keybuffer = (STR *)malloc(keysize);

          if (keybuffer)
          {
            if (pfnGetKey(keybuffer, keysize) == keysize)
            {
              // key successfully retrieved
            }
            else
            {
              memoryKickError(MEMORY_ROM_ERROR_KEYFILE, 0);
              return -1;
            }
          }
        }
      }
      FreeLibrary(hAmigaForeverDLL);
    }

    if (!keybuffer)
    {
      memoryKickError(MEMORY_ROM_ERROR_KEYFILE, 0);
      return -1;
    }
#endif
  }

  if (!keybuffer) return -1;

  /* Read file */

  if ((RF = fopen(filename, "rb")) != NULL)
  {
    fseek(RF, 11, SEEK_SET);
    while (((c = fgetc(RF)) != EOF) && filesize < 524288)
    {
      if (keysize != 0) c ^= keybuffer[keypos++];
      if (keypos == keysize) keypos = 0;
      memory_kick[filesize++] = (UBY)c;
    }
    while ((c = fgetc(RF)) != EOF)
      filesize++;
    fclose(RF);
    free(keybuffer);
    return filesize;
  }
  free(keybuffer);
  return -1;
}

/*============================================================================*/
/* Load Amiga Forever encrypted ROM-files                                     */
/* Return TRUE if file was handled, that is both if the file is               */
/* valid, or has wrong version                                                */
/*============================================================================*/

int memoryKickLoadAF2(STR *filename, FILE *F, UBY *memory_kick, const bool suppressgui)
{
  STR IDString[12];

  memory_a1000_wcs = false;

  fread(IDString, 11, 1, F);
  ULO version = IDString[10] - '0';
  IDString[10] = '\0';
  if (stricmp(IDString, "AMIROMTYPE") == 0)
  { /* Header seems OK */
    if (version != 1)
    {
      if (!suppressgui) memoryKickError(MEMORY_ROM_ERROR_AMIROM_VERSION, version);
      return TRUE; /* File was handled */
    }
    else
    { /* Seems to be a file we can handle */

      fclose(F);

      ULO size = memoryKickDecodeAF(filename, memory_key, memory_kick);
      if (size == -1)
      {
        if (!suppressgui) memoryKickError(MEMORY_ROM_ERROR_AMIROM_READ, 0);
        return TRUE;
      }
      if (size != 8192 && size != 262144 && size != 524288)
      {
        if (!suppressgui) memoryKickError(MEMORY_ROM_ERROR_SIZE, size);
        return TRUE;
      }

      if (size == 8192)
      { /* Load A1000 bootstrap ROM */
        memory_a1000_wcs = true;

        if (memory_a1000_bootstrap == nullptr) memory_a1000_bootstrap = (UBY *)malloc(262144);

        if (memory_a1000_bootstrap)
        {
          ULO lCRC32 = 0;
          memset(memory_a1000_bootstrap, 0xff, 262144);
          memcpy(memory_a1000_bootstrap, memory_kick, 8192);
          lCRC32 = crc32(0, memory_kick, 8192);
          if (lCRC32 != 0x62F11C04)
          {
            free(memory_a1000_bootstrap);
            memory_a1000_bootstrap = nullptr;
            memoryKickError(MEMORY_ROM_ERROR_CHECKSUM, lCRC32);
            return FALSE;
          }
        }
      }
      else if (size == 262144)
        memcpy(memory_kick + 262144, memory_kick, 262144);

      memory_kickimage_none = FALSE;
      memoryKickIdentify(memory_kickimage_versionstr);
      return TRUE;
    }
  }
  /* Here, header was not recognized */
  return FALSE;
}

/*============================================================================*/
/* Detect and load kickdisk                                                   */
/* Based on information provided by Jerry Lawrence                            */
/*============================================================================*/

void memoryKickDiskLoad(FILE *F)
{
  STR head[5];

  /* Check header */

  fseek(F, 0, SEEK_SET);
  fread(head, 4, 1, F);
  head[4] = '\0';
  if (strcmp(head, "KICK") != 0)
  {
    memoryKickError(MEMORY_ROM_ERROR_KICKDISK_NOT, 0);
    return;
  }
  fread(head, 3, 1, F);
  head[3] = '\0';
  if (strcmp(head, "SUP") == 0)
  {
    memoryKickError(MEMORY_ROM_ERROR_KICKDISK_SUPER, 0);
    return;
  }
  fseek(F, 512, SEEK_SET); /* Load image */
  fread(memory_kick, 262144, 1, F);
  memcpy(memory_kick + 262144, memory_kick, 262144);
}

/*============================================================================*/
/* memory_kickimage is the file we want to load                               */
/*============================================================================*/

void memoryKickLoad()
{
  FILE *F = nullptr;
  BOOLE kickdisk = FALSE;
  BOOLE afkick = FALSE;

  memory_a1000_wcs = false;

  /* New file is different from previous */
  /* Must load file */

  fs_wrapper_object_info *fsnp = Service->FSWrapper.GetFSObjectInfo(memory_kickimage);

  memory_kickimage_none = FALSE; // Initially Kickstart is expected to be OK
  if (fsnp == nullptr)
  {
    memoryKickError(MEMORY_ROM_ERROR_EXISTS_NOT, 0);
  }
  else
  {
    if (fsnp->type != fs_wrapper_object_types::FILE)
      memoryKickError(MEMORY_ROM_ERROR_FILE, 0);
    else
    { /* File passed initial tests */
      if ((F = fopen(memory_kickimage, "rb")) == nullptr)
        memoryKickError(MEMORY_ROM_ERROR_EXISTS_NOT, 0);
      else
        memory_kickimage_size = fsnp->size;
    }
    delete fsnp;
  }

  /* Either the file is open, or memory_kickimage_none is TRUE */

  if (!memory_kickimage_none)
  {

    /* File opened successfully */

    /* Kickdisk flag */

    STR *suffix = strchr(memory_kickimage, '.');
    if (suffix != nullptr)
    {
      STR *lastsuffix = suffix;
      while ((suffix = strchr(lastsuffix + 1, '.')) != nullptr)
        lastsuffix = suffix;
      kickdisk = (stricmp(lastsuffix + 1, "ADF") == 0);
    }
    /* mem_loadrom_af2 will return TRUE if file was handled */
    /* Handled also means any error conditions */
    /* The result can be that no kickstart was loaded */

    if (kickdisk)
      memoryKickDiskLoad(F);
    else
      afkick = memoryKickLoadAF2(memory_kickimage, F, memory_kick, false);
    if (!kickdisk && !afkick)
    { /* Normal kickstart image */
      fseek(F, 0, SEEK_SET);
      if (memory_kickimage_size == 8192)
      { /* Load A1000 bootstrap ROM */
        memory_a1000_wcs = true;

        if (memory_a1000_bootstrap == nullptr) memory_a1000_bootstrap = (UBY *)malloc(262144);

        if (memory_a1000_bootstrap)
        {
          memset(memory_a1000_bootstrap, 0xff, 262144);
          fread(memory_a1000_bootstrap, 1, 8192, F);
          memcpy(memory_kick, memory_a1000_bootstrap, 262144);
          memcpy(memory_kick + 262144, memory_a1000_bootstrap, 262144);
          ULO lCRC32 = crc32(0, memory_a1000_bootstrap, 8192);
          if (lCRC32 != 0x62F11C04)
          {
            free(memory_a1000_bootstrap);
            memory_a1000_bootstrap = nullptr;
            memoryKickError(MEMORY_ROM_ERROR_CHECKSUM, lCRC32);
            return;
          }
        }
      }
      else if (memory_kickimage_size == 262144)
      { /* Load 256k ROM */
        fread(memory_kick, 1, 262144, F);
        memcpy(memory_kick + 262144, memory_kick, 262144);
      }
      else if (memory_kickimage_size == 524288) /* Load 512k ROM */
        fread(memory_kick, 1, 524288, F);
      else
      { /* Rom size is wrong */
        memoryKickError(MEMORY_ROM_ERROR_SIZE, memory_kickimage_size);
      }
      fclose(F);
    }
  }
  if (!memory_kickimage_none)
  {
    memoryKickOK();
    memoryKickA1000BootstrapSetMapped(true);
  }
}

/*============================================================================*/
/* Clear memory used for kickstart image                                      */
/*============================================================================*/

void memoryKickClear()
{
  memset(memory_kick, 0, 0x80000);
}

/*============================================================================*/
/* Free memory used for extended kickstart image                              */
/*============================================================================*/

void memoryKickExtendedFree()
{
  if (memory_kick_ext)
  {
    free(memory_kick_ext);
    memory_kick_ext = nullptr;
  }
  memory_kickimage_ext_size = 0;
  memory_kickimage_ext_basebank = 0;
}

/*============================================================================*/
/* Load extended Kickstart ROM into memory                                    */
/*============================================================================*/

void memoryKickExtendedLoad()
{
  // New file is different from previous, must load file

  memoryKickExtendedFree();

  if (stricmp(memory_kickimage_ext, "") == 0)
  {
    return;
  }

  fs_wrapper_object_info *fsnp = Service->FSWrapper.GetFSObjectInfo(memory_kickimage_ext);
  if (fsnp == nullptr)
  {
    return;
  }

  if (fsnp->type != fs_wrapper_object_types::FILE)
  {
    delete fsnp;
    return;
  }

  // File passed initial tests
  FILE *F = fopen(memory_kickimage_ext, "rb");
  if (F == nullptr)
  {
    delete fsnp;
    return;
  }

  ULO size = fsnp->size;
  delete fsnp;

  if (F)
  {
    fseek(F, 0, SEEK_SET);

    if (size == 262155 || size == 524299)
    {
      STR IDString[12];

      fread(IDString, 11, 1, F);
      ULO version = IDString[10] - '0';
      IDString[10] = '\0';
      if (stricmp(IDString, "AMIROMTYPE") == 0)
      { // Header seems OK
        if (version != 1)
        {
          fclose(F);
          return;
        }

        // Seems to be a file we can handle
        memory_kick_ext = (UBY *)malloc(size - 11);
        size = memoryKickDecodeAF(memory_kickimage_ext, memory_key, memory_kick_ext);
        memory_kickimage_ext_size = size;
      }
    }
    else
    {
      if (size == 262144 || size == 524288)
      {
        memory_kick_ext = (UBY *)malloc(size);

        if (memory_kick_ext)
        {
          memset(memory_kick_ext, 0xff, size);
          fread(memory_kick_ext, 1, size, F);
          memory_kickimage_ext_size = size;
        }
        else
        {
          fclose(F);
          return;
        }
      }
    }

    memory_kickimage_ext_basebank = memory_kick_ext[5];

    // AROS extended ROM does not have basebank at byte 6, override
    if (memory_kickimage_ext_basebank == 0xf8)
    {
      memory_kickimage_ext_basebank = 0xe0;
    }

    fclose(F);
  }
}

/*============================================================================*/
/* Top-level memory access functions                                          */
/*============================================================================*/

/*==============================================================================
Raises exception 3 when a word or long is accessing an odd address
and the CPU is < 020
==============================================================================*/

static void memoryOddRead(ULO address)
{
  if (address & 1)
  {
    if (cpuGetModelMajor() < 2)
    {
      memory_fault_read = TRUE;
      memory_fault_address = address;
      cpuThrowAddressErrorException();
    }
  }
}

static void memoryOddWrite(ULO address)
{
  if (address & 1)
  {
    if (cpuGetModelMajor() < 2)
    {
      memory_fault_read = FALSE;
      memory_fault_address = address;
      cpuThrowAddressErrorException();
    }
  }
}

UBY memoryReadByteViaBankHandler(ULO address)
{
  return memory_bank_readbyte[address >> 16](address);
}

UBY memoryReadByte(ULO address)
{
  UBY *memory_ptr = memory_bank_pointer[address >> 16];
  if (memory_ptr != nullptr)
  {
    UBY *p = memory_ptr + address;
    return memoryReadByteFromPointer(p);
  }
  return memoryReadByteViaBankHandler(address);
}

UWO memoryReadWordViaBankHandler(ULO address)
{
  memoryOddRead(address);
  return memory_bank_readword[address >> 16](address);
}

UWO memoryReadWord(ULO address)
{
  UBY *memory_ptr = memory_bank_pointer[address >> 16];
  if ((memory_ptr != nullptr) && !(address & 1))
  {
    UBY *p = memory_ptr + address;
    return memoryReadWordFromPointer(p);
  }
  return memoryReadWordViaBankHandler(address);
}

ULO memoryReadLongViaBankHandler(ULO address)
{
  memoryOddRead(address);
  return memory_bank_readlong[address >> 16](address);
}

ULO memoryReadLong(ULO address)
{
  UBY *memory_ptr = memory_bank_pointer[address >> 16];
  if ((memory_ptr != nullptr) && !(address & 1))
  {
    UBY *p = memory_ptr + address;
    return memoryReadLongFromPointer(p);
  }
  return memoryReadLongViaBankHandler(address);
}

void memoryWriteByte(UBY data, ULO address)
{
  ULO bank = address >> 16;
  if (memory_bank_pointer_can_write[bank])
  {
    memoryWriteByteToPointer(data, memory_bank_pointer[bank] + address);
  }
  else
  {
    memory_bank_writebyte[bank](data, address);
  }
}

void memoryWriteWordViaBankHandler(UWO data, ULO address)
{
  memoryOddWrite(address);
  memory_bank_writeword[address >> 16](data, address);
}

void memoryWriteWord(UWO data, ULO address)
{
  ULO bank = address >> 16;
  if (memory_bank_pointer_can_write[bank] && !(address & 1))
  {
    memoryWriteWordToPointer(data, memory_bank_pointer[bank] + address);
  }
  else
  {
    memoryWriteWordViaBankHandler(data, address);
  }
}

void memoryWriteLongViaBankHandler(ULO data, ULO address)
{
  memoryOddWrite(address);
  memory_bank_writelong[address >> 16](data, address);
}

void memoryWriteLong(ULO data, ULO address)
{
  ULO bank = address >> 16;
  if (memory_bank_pointer_can_write[bank] && !(address & 1))
  {
    memoryWriteLongToPointer(data, memory_bank_pointer[bank] + address);
  }
  else
  {
    memoryWriteLongViaBankHandler(data, address);
  }
}

/*============================================================================*/
/* Memory configuration interface                                             */
/*============================================================================*/

bool memorySetChipSize(ULO chipsize)
{
  bool needreset = (memory_chipsize != chipsize);
  memory_chipsize = chipsize;
  return needreset;
}

ULO memoryGetChipSize()
{
  return memory_chipsize;
}

bool memorySetFastSize(ULO fastsize)
{
  bool needreset = (memory_fastsize != fastsize);
  memory_fastsize = fastsize;
  if (needreset) memoryFastAllocate();
  return needreset;
}

ULO memoryGetFastSize()
{
  return memory_fastsize;
}

void memorySetFastAllocatedSize(ULO fastallocatedsize)
{
  memory_fastallocatedsize = fastallocatedsize;
}

ULO memoryGetFastAllocatedSize()
{
  return memory_fastallocatedsize;
}

bool memorySetSlowSize(ULO slowsize)
{
  bool needreset = (memory_slowsize != slowsize);
  memory_slowsize = slowsize;
  return needreset;
}

ULO memoryGetSlowSize()
{
  return memory_slowsize;
}

bool memorySetUseAutoconfig(bool useautoconfig)
{
  bool needreset = memory_useautoconfig != useautoconfig;
  memory_useautoconfig = useautoconfig;
  return needreset;
}

bool memoryGetUseAutoconfig()
{
  return memory_useautoconfig;
}

bool memorySetAddressSpace32Bit(bool addressSpace32Bit)
{
  bool needreset = memory_addressSpace32Bit != addressSpace32Bit;
  memory_addressSpace32Bit = addressSpace32Bit;
  return needreset;
}

bool memoryGetAddressSpace32Bit()
{
  return memory_addressSpace32Bit;
}

bool memorySetKickImage(STR *kickimage)
{
  bool needreset = !!strncmp(memory_kickimage, kickimage, CFG_FILENAME_LENGTH);
  strncpy(memory_kickimage, kickimage, CFG_FILENAME_LENGTH);
  if (needreset) memoryKickLoad();
  return needreset;
}

bool memorySetKickImageExtended(STR *kickimageext)
{
  bool needreset = !!strncmp(memory_kickimage_ext, kickimageext, CFG_FILENAME_LENGTH);
  strncpy(memory_kickimage_ext, kickimageext, CFG_FILENAME_LENGTH);
  if (needreset) memoryKickExtendedLoad();
  return needreset;
}

STR *memoryGetKickImage()
{
  return memory_kickimage;
}

void memorySetKey(STR *key)
{
  strncpy(memory_key, key, CFG_FILENAME_LENGTH);
}

STR *memoryGetKey()
{
  return memory_key;
}

ULO memoryGetKickImageBaseBank()
{
  return memory_kickimage_basebank;
}

ULO memoryGetKickImageVersion()
{
  return memory_kickimage_version;
}

BOOLE memoryGetKickImageOK()
{
  return !memory_kickimage_none;
}

ULO memoryInitialPC()
{
  return memory_initial_PC;
}

ULO memoryInitialSP()
{
  return memory_initial_SP;
}

/*============================================================================*/
/* Sets all settings a clean state                                            */
/*============================================================================*/

void memoryChipSettingsClear()
{
  memorySetChipSize(0x200000);
  memoryChipClear();
}

void memoryFastSettingsClear()
{
  memory_fast = nullptr;
  memory_fast_baseaddress = 0;
  memorySetFastAllocatedSize(0);
  memorySetFastSize(0);
}

void memorySlowSettingsClear()
{
  memorySetSlowSize(0x1c0000);
  memorySlowClear();
}

void memoryIoSettingsClear()
{
  memoryIoClear();
}

void memoryKickSettingsClear()
{
  memory_kickimage[0] = '\0';
  memory_kickimage_none = TRUE;
  memoryKickClear();
}

void memoryEmemSettingsClear()
{
  memoryEmemCardsRemove();
  memoryEmemClear();
}

void memoryDmemSettingsClear()
{
  memoryDmemSetCounter(0);
  memoryDmemClear();
}

void memoryBankSettingsClear()
{
  memoryBankClearAll();
}

/*==============*/
/* Generic init */
/*==============*/

void memoryEmulationStart()
{
  memoryIoClear();
}

void memoryEmulationStop()
{
}

void memorySoftReset()
{
  memoryDmemClear();
  memoryEmemClear();
  memoryEmemCardsRemove();
  memoryFastCardAdd();
  memoryBankClearAll();
  memoryChipMap(true);
  memorySlowMap();
  memoryIoMap();
  memoryEmemMap();
  memoryDmemMap();
  memoryMysteryMap();
  memoryKickA1000BootstrapSetMapped(true);
  memoryKickMap();
  memoryKickExtendedMap();
  rtcMap();
}

void memoryHardReset()
{
  Service->Log.AddLog("memoryHardReset()\n");

  memoryChipClear(), memoryFastClear();
  memorySlowClear();
  memoryDmemClear();
  memoryEmemClear();
  memoryEmemCardsRemove();
  memoryFastCardAdd();
  memoryBankClearAll();
  memoryChipMap(true);
  memorySlowMap();
  memoryIoMap();
  memoryEmemMap();
  memoryDmemMap();
  memoryMysteryMap();
  memoryKickMap();
  memoryKickExtendedMap();
  rtcMap();
}

void memoryHardResetPost()
{
  memoryEmemCardInit();
}

void memoryStartup()
{
  memorySetAddressSpace32Bit(true);
  memoryBankSettingsClear();
  memorySetAddressSpace32Bit(false);
  memoryChipSettingsClear();
  memoryFastSettingsClear();
  memorySlowSettingsClear();
  memoryIoSettingsClear();
  memoryKickSettingsClear();
  memoryEmemSettingsClear();
  memoryDmemSettingsClear();
}

void memoryShutdown()
{
  memoryFastFree();
  memoryKickExtendedFree();
  memoryKickA1000BootstrapFree();
}
