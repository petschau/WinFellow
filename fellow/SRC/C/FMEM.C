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

#include "defs.h"
#include "fellow.h"
#include "chipset.h"
#include "draw.h"
#include "CpuModule.h"
#include "CpuIntegration.h"
#include "fellow/api/module/IHardfileHandler.h"
#include "graph.h"
#include "floppy.h"
#include "copper.h"
#include "cia.h"
#include "blit.h"
#include "fmem.h"
#include "fswrap.h"
#include "wgui.h"
#include "rtc.h"
#include "fileops.h"
#include "zlib.h" // crc32 function

#ifdef WIN32
#include <tchar.h>
#endif

using namespace fellow::api::module;

/*============================================================================*/
/* Holds configuration for memory                                             */
/*============================================================================*/

ULO memory_chipsize;
ULO memory_fastsize;
ULO memory_slowsize;
bool memory_useautoconfig;
BOOLE memory_address32bit;
STR memory_kickimage[CFG_FILENAME_LENGTH];
STR memory_kickimage_ext[CFG_FILENAME_LENGTH];
STR memory_key[256];
bool memory_a1000_wcs = false;              ///< emulate the Amiga 1000 WCS (writable control store)
UBY *memory_a1000_bootstrap = NULL;         ///< hold A1000 bootstrap ROM, if used
bool memory_a1000_bootstrap_mapped = false; ///< true while A1000 bootstrap ROM mapped to KS area

/*============================================================================*/
/* Holds actual memory                                                        */
/*============================================================================*/

UBY memory_chip[0x200000 + 32];
UBY memory_slow[0x1c0000 + 32];
UBY memory_kick[0x080000 + 32];
UBY *memory_kick_ext = NULL;
UBY *memory_fast = NULL;
ULO memory_fast_baseaddress;
ULO memory_fastallocatedsize;
UBY *memory_slow_base;


/*============================================================================*/
/* Autoconfig data                                                            */
/*============================================================================*/

#define EMEM_MAXARDS 4
UBY memory_emem[0x10000];
memoryEmemCardInitFunc memory_ememard_initfunc[EMEM_MAXARDS];
memoryEmemCardMapFunc memory_ememard_mapfunc[EMEM_MAXARDS];
ULO memory_ememardcount;                                /* Number of cards */
ULO memory_ememards_finishedcount;                         /* Current card */


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
  void memoryKickSettingsClear(void);


  /*============================================================================*/
  /* Illegal read / write fault information                                     */
  /*============================================================================*/

  BOOLE memory_fault_read;                       /* TRUE - read / FALSE - write */
  ULO memory_fault_address;


  /*============================================================================*/
  /* Some run-time scratch variables                                            */
  /*============================================================================*/

  void memoryKickA1000BootstrapSetMapped(const bool);

  void memoryWriteByteToPointer(UBY data, UBY *address)
  {
    address[0] = data;
  }

  void memoryWriteWordToPointer(UWO data, UBY *address)
  {
    address[0] = (UBY) (data >> 8);
    address[1] = (UBY) data;
  }

  void memoryWriteLongToPointer(ULO data, UBY *address)
  {
    address[0] = (UBY) (data >> 24);
    address[1] = (UBY) (data >> 16);
    address[2] = (UBY) (data >> 8);
    address[3] = (UBY) data;
  }

  /*----------------------------
  Chip read register functions	
  ----------------------------*/

  // To simulate noise, return 0 and -1 every second time.
  // Why? Bugged demos test write-only registers for various bit-values
  // and to break out of loops, both 0 and 1 values must be returned.

  UWO rdefault(ULO address)
  {
    return (UWO)(rand() % 65536);
  }

  void wdefault(UWO data, ULO address)
  {
  }

  /*============================================================================*/
  /* Table of register read/write functions                                     */
  /*============================================================================*/

  memoryIoReadFunc memory_iobank_read[257]; // Account for long writes to the last
  memoryIoWriteFunc memory_iobank_write[257]; // word

  /*============================================================================*/
  /* Memory mapping tables                                                      */
  /*============================================================================*/

  memoryReadByteFunc memory_bank_readbyte[65536];
  memoryReadWordFunc memory_bank_readword[65536];
  memoryReadLongFunc memory_bank_readlong[65536];
  memoryWriteByteFunc memory_bank_writebyte[65536];
  memoryWriteWordFunc memory_bank_writeword[65536];
  memoryWriteLongFunc memory_bank_writelong[65536];
  UBY *memory_bank_pointer[65536];                   /* Used by the filesystem */
  BOOLE memory_bank_pointer_can_write[65536];

  /*============================================================================*/
  /* Memory bank mapping functions                                              */
  /*============================================================================*/

  /*============================================================================*/
  /* Set read and write stubs for a bank, as well as a direct pointer to        */
  /* its memory. NULL pointer is passed when the memory must always be          */
  /* read through the stubs, like in a bank of regsters where writing           */
  /* or reading the value generates side-effects.                               */
  /* Datadirect is TRUE when data accesses can be made through a pointer        */
  /*============================================================================*/

  void memoryBankSet(memoryReadByteFunc rb, 
                     memoryReadWordFunc rw, 
                     memoryReadLongFunc rl, 
                     memoryWriteByteFunc wb,
                     memoryWriteWordFunc ww, 
                     memoryWriteLongFunc wl, 
                     UBY *basep, 
                     ULO bank,
                     ULO basebank,
                     BOOLE pointer_can_write)
  {
    ULO i, j;

    j = (memoryGetAddress32Bit()) ? 65536 : 256;
    for (i = bank; i < 65536; i += j)
    {
      memory_bank_readbyte[i] = rb;
      memory_bank_readword[i] = rw;
      memory_bank_readlong[i] = rl;
      memory_bank_writebyte[i] = wb;
      memory_bank_writeword[i] = ww;
      memory_bank_writelong[i] = wl;
      memory_bank_pointer_can_write[i] = pointer_can_write;

      if (basep != NULL)
      {
	memory_bank_pointer[i] = basep - (basebank<<16);
      }
      else
      {
	memory_bank_pointer[i] = NULL;
      }
      basebank += j;
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
      val = rand() % 256;
    }
    while (val == 0 || val == memory_previous_unmapped_byte);

    memory_previous_unmapped_byte = val;
    return val;
  }

  static UWO memory_previous_unmapped_word = 0;
  UWO memoryUnmappedReadWord(ULO address)
  {
    UWO val;
    do
    {
      val = rand() % 65536;
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
      val = rand();
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
    memoryBankSet(memoryUnmappedReadByte,
                  memoryUnmappedReadWord,
                  memoryUnmappedReadLong, 
                  memoryUnmappedWriteByte,
                  memoryUnmappedWriteWord, 
                  memoryUnmappedWriteLong, 
                  NULL,
                  bank,
                  0,
                  FALSE);
  }

  /*============================================================================*/
  /* Clear all bank data to safe "do-nothing" values                            */
  /*============================================================================*/

  void memoryBankClearAll(void)
  {
    ULO hilim = (memoryGetAddress32Bit()) ? 65536 : 256;
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

  void memoryEmemClear(void)
  {
    memset(memory_emem, 0xff, 65536);
  }

  /*============================================================================*/
  /* Add card to table                                                          */
  /*============================================================================*/

  void memoryEmemCardAdd(memoryEmemCardInitFunc cardinit,
    memoryEmemCardMapFunc cardmap)
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

  void memoryEmemCardNext(void)
  {
    memory_ememards_finishedcount++;
  }


  /*============================================================================*/
  /* Init the current card                                                      */
  /*============================================================================*/

  void memoryEmemCardInit(void)
  {
    memoryEmemClear();
    if (memory_ememards_finishedcount != memory_ememardcount)
      memory_ememard_initfunc[memory_ememards_finishedcount]();
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

  void memoryEmemCardsReset(void)
  {
    memory_ememards_finishedcount = 0;
    memoryEmemCardInit();
  }

  /*============================================================================*/
  /* Clear the card table                                                       */
  /*============================================================================*/

  void memoryEmemCardsRemove(void)
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
      memory_emem[index] = (UBY) (value & 0xf0);
      memory_emem[index + 2] = (UBY) ((value & 0xf)<<4);
      break;
    default:
      memory_emem[index] = (UBY) (~(value & 0xf0));
      memory_emem[index + 2] = (UBY) (~((value & 0xf)<<4));
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
    case 0x32:
      mapping = data = 0;
    case 0x48:
      mapping = (mapping & 0xff) | (((ULO)data) << 8);
      memoryEmemCardMap(mapping);
      memoryEmemCardNext();
      memoryEmemCardInit();
      break;
    case 0x4a:
      mapping = (mapping & 0xff00) | ((ULO)data);
      break;
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

  /*===========================================================================*/
  /* Map the autoconfig memory bank into memory                                */
  /*===========================================================================*/

  void memoryEmemMap(void)
  {
    if (memoryGetKickImageBaseBank() >= 0xf8)
    {
      memoryBankSet(memoryEmemReadByte,
                    memoryEmemReadWord,
                    memoryEmemReadLong,
                    memoryEmemWriteByte,
                    memoryEmemWriteWord,
                    memoryEmemWriteLong,
                    NULL,
                    0xe8,
                    0xe8,
                    FALSE);
    }
  }


  /*===================*/
  /* End of autoconfig */
  /*===================*/


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

  void memoryDmemClear(void)
  {
    memset(memory_dmem, 0, 4096);
  }

  void memoryDmemSetCounter(ULO c)
  {
    memory_dmemcounter = c;
  }

  ULO memoryDmemGetCounterWithoutOffset()
  {
    return memory_dmemcounter;
  }

  ULO memoryDmemGetCounter(void)
  {
    return memory_dmemcounter + MEMORY_DMEM_OFFSET;
  }

  void memoryDmemSetString(const STR *st)
  {
    strcpy((STR *) (memory_dmem + memory_dmemcounter), st);
    memory_dmemcounter += (ULO) strlen(st) + 1;
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

  void memoryDmemMap(void)
  {
    ULO bank = 0xf40000>>16;

    if (memory_useautoconfig && (memory_kickimage_basebank >= 0xf8))
    {
      memoryBankSet(memoryDmemReadByte,
                    memoryDmemReadWord,
                    memoryDmemReadLong,
                    memoryDmemWriteByte,
                    memoryDmemWriteWord,
                    memoryDmemWriteLong,
                    memory_dmem,
                    bank,
                    bank,
                    FALSE);
    }
  }

  /*============================================================================*/
  /* Converts an address to a direct pointer to memory. Used by hardfile device */
  /*============================================================================*/

  UBY *memoryAddressToPtr(ULO address)
  {
    UBY *result = memory_bank_pointer[address>>16];

    if (result != NULL)
      result += address;
    return result;
  }

  /*============================================================================*/
  /* Chip memory handling                                                       */
  /*============================================================================*/

  #define chipmemMaskAddress(ptr) ((ptr) & chipset.address_mask)

  UBY memoryChipReadByte(ULO address)
  {
    UBY *p = memory_chip + chipmemMaskAddress(address);
    return memoryReadByteFromPointer(p);
  }

  UWO memoryChipReadWord(ULO address)
  {
    UBY *p = memory_chip + chipmemMaskAddress(address);
    return memoryReadWordFromPointer(p);
  }

  ULO memoryChipReadLong(ULO address)
  {
    UBY *p = memory_chip + chipmemMaskAddress(address);
    return memoryReadLongFromPointer(p);
  }

  void memoryChipWriteByte(UBY data, ULO address)
  {
    UBY *p = memory_chip + chipmemMaskAddress(address);
    memoryWriteByteToPointer(data, p);
  }

  void memoryChipWriteWord(UWO data, ULO address)
  {
    UBY *p = memory_chip + chipmemMaskAddress(address);
    memoryWriteWordToPointer(data, p);
  }

  void memoryChipWriteLong(ULO data, ULO address)
  {
    UBY *p = memory_chip + chipmemMaskAddress(address);
    memoryWriteLongToPointer(data, p);
  }

  void memoryChipClear(void)
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

  ULO memoryChipGetLastBank(void)
  {
    ULO lastbank = memoryGetChipSize()>>16;

    if (chipsetGetECS())
    {
      return (lastbank <= 32) ? lastbank : 32;
    }

    // OCS
    return (lastbank <= 8) ? lastbank : 8;
  }

  void memoryChipMap(bool overlay)
  {
    ULO bank;

    // Build first, "real" chipmem area
    if (overlay)
    {
      // 256k ROMs are already mirrored once in the memory_kick area
      // Map entire 512k ROM area to $0
      for (bank = 0; bank < 8; bank++)
      {
	memoryBankSet(memoryOverlayReadByte,
	              memoryOverlayReadWord,
	              memoryOverlayReadLong,
	              memoryOverlayWriteByte,
	              memoryOverlayWriteWord,
	              memoryOverlayWriteLong,
	              memory_kick,
	              bank,
	              0,
	              FALSE);
      }
    }

    // Map 512k to 2MB of chip memory, possibly skipping the overlay area
    ULO lastbank = memoryChipGetLastBank();
    for (bank = (overlay) ? 8 : 0; bank < lastbank; bank++)
    {
      memoryBankSet(memoryChipReadByte,
                    memoryChipReadWord, 
                    memoryChipReadLong,
                    memoryChipWriteByte, 
                    memoryChipWriteWord, 
                    memoryChipWriteLong,
                    memory_chip,
                    bank, 
                    0,
                    TRUE);
    }

    // In the case of 256k chip memory and not overlaying, clear the second 256k map 
    // as this area could have been mapped for the ROM overlay
    if (lastbank < 8 && !overlay)
    {
      for (bank = lastbank; bank < 8; ++bank)
      {
        memoryBankClear(bank);
      }
    }

    if (!chipsetGetECS())
    {
      // OCS: Make 3 more copies of the chipram at $80000, $100000 and $180000
      for (ULO i = 1; i < 4; ++i)
      {
        ULO bank_start = 8*i;
        ULO bank_end = bank_start + lastbank;
        for (bank = bank_start; bank < bank_end; bank++)
        {
          memoryBankSet(memoryChipReadByte,
                        memoryChipReadWord, 
                        memoryChipReadLong,
                        memoryChipWriteByte, 
                        memoryChipWriteWord, 
                        memoryChipWriteLong,
                        memory_chip,
                        bank,
                        bank_start,
                        TRUE);
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

  void memoryFastCardInit(void)
  {
    if (memoryGetFastSize() == 0x100000) memoryEmemSet(0, 0xe5);
    else if (memoryGetFastSize() == 0x200000) memoryEmemSet(0, 0xe6);
    else if (memoryGetFastSize() == 0x400000) memoryEmemSet(0, 0xe7);
    else if (memoryGetFastSize() == 0x800000) memoryEmemSet(0, 0xe0);
    memoryEmemSet(8, 128);
    memoryEmemSet(4, 1);
    memoryEmemSet(0x10, 2011>>8);
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

  void memoryFastClear(void)
  {
    if (memory_fast != NULL)
      memset(memory_fast, 0, memoryGetFastSize());
  }

  void memoryFastFree(void)
  {
    if (memory_fast != NULL)
    {
      free(memory_fast);
      memory_fast = NULL;
      memory_fast_baseaddress = 0;
      memorySetFastAllocatedSize(0);
    }
  }

  void memoryFastAllocate(void)
  {
    if (memoryGetFastSize() != memoryGetFastAllocatedSize())
    {
      memoryFastFree();
      memory_fast = (UBY *) malloc(memoryGetFastSize());
      if (memory_fast == NULL) memorySetFastSize(0);
      else memoryFastClear();
      memorySetFastAllocatedSize((memory_fast == NULL) ? 0 : memoryGetFastSize());
    }
  }

  /*============================================================================*/
  /* Map fastcard.							      */
  /*============================================================================*/

  void memoryFastCardMap(ULO mapping)
  {
    ULO bank, lastbank;

    memory_fast_baseaddress = (mapping >> 8) << 16;
    if (memoryGetFastSize() > 0x800000)
    {
      lastbank = 0xa00000>>16;
    }
    else
    {
      lastbank = (memory_fast_baseaddress + memoryGetFastSize())>>16;
    }
    for (bank = memory_fast_baseaddress>>16; bank < lastbank; bank++)
    {
      memoryBankSet(memoryFastReadByte,
                    memoryFastReadWord,
                    memoryFastReadLong,
                    memoryFastWriteByte,
                    memoryFastWriteWord, 
                    memoryFastWriteLong,
                    memory_fast, 
                    bank, 
                    memory_fast_baseaddress>>16,
                    TRUE);
    }
    memset(memory_fast, 0, memoryGetFastSize());
  }

  void memoryFastCardAdd(void)
  {
    if (memoryGetFastSize() != 0)
      memoryEmemCardAdd(memoryFastCardInit, memoryFastCardMap);
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

  bool memorySlowMapAsChip(void)
  {
    return chipsetGetECS() && memoryGetChipSize() == 0x80000 && memoryGetSlowSize() == 0x80000;
  }

  void memorySlowClear(void)
  {
    memset(memory_slow, 0, memoryGetSlowSize());
    if (memorySlowMapAsChip())
    {
      memset(memory_chip + 0x80000, 0, memoryGetSlowSize());
    }
  }

  void memorySlowMap(void)
  {
    ULO lastbank;

    if (memoryGetSlowSize() > 0x1c0000)
    {
      lastbank = 0xdc0000>>16;
    }
    else
    {
      lastbank = (0xc00000 + memoryGetSlowSize())>>16;
    }

    // Special config on ECS with 512k chip + 512k slow, chips see slow mem at $80000
    memory_slow_base = (memorySlowMapAsChip()) ? (memory_chip + 0x80000) : memory_slow;

    for (ULO bank = 0xc00000>>16; bank < lastbank; bank++)
    {
      memoryBankSet(memorySlowReadByte,
                    memorySlowReadWord,
                    memorySlowReadLong,
                    memorySlowWriteByte,
                    memorySlowWriteWord,
                    memorySlowWriteLong,
                    memory_slow_base,
                    bank, 
                    0xc00000>>16,
                    TRUE);
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

  void memoryMysteryMap(void)
  {
    memoryBankSet(memoryMysteryReadByte,
                  memoryMysteryReadWord,
                  memoryMysteryReadLong,
                  memoryMysteryWriteByte, 
                  memoryMysteryWriteWord, 
                  memoryMysteryWriteLong,
                  NULL, 
                  0xe9, 
                  0,
                  FALSE);
    memoryBankSet(memoryMysteryReadByte, 
                  memoryMysteryReadWord, 
                  memoryMysteryReadLong,
                  memoryMysteryWriteByte, 
                  memoryMysteryWriteWord, 
                  memoryMysteryWriteLong,
                  NULL, 
                  0xde, 
                  0,
                  FALSE);
  }

  /*============================================================================*/
  /* IO Registers                                                               */
  /*============================================================================*/

  UBY memoryIoReadByte(ULO address)
  {
    ULO adr = address & 0x1fe;
    if (address & 0x1)
    { // Odd address
      return (UBY) memory_iobank_read[adr >> 1](adr);
    }
    else
    { // Even address
      return (UBY) (memory_iobank_read[adr >> 1](adr) >> 8);
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
      memory_iobank_write[adr >> 1]((UWO) data, adr);
    }
    else
    { // Even address
      memory_iobank_write[adr >> 1](((UWO) data) << 8, adr);
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

  void memoryIoMap(void)
  {
    ULO lastbank;

    if (memoryGetSlowSize() > 0x1c0000)
    {
      lastbank = 0xdc0000>>16;
    }
    else
    {
      lastbank = (0xc00000 + memoryGetSlowSize())>>16;
    }
    for (ULO bank = lastbank; bank < 0xe00000>>16; bank++)
    {
      memoryBankSet(memoryIoReadByte,
                    memoryIoReadWord, 
                    memoryIoReadLong,
                    memoryIoWriteByte, 
                    memoryIoWriteWord, 
                    memoryIoWriteLong,
                    NULL,
                    bank,
                    0,
                    FALSE);
    }
  }

  /*===========================================================================*/
  /* Initializes one entry in the IO register access table                     */
  /*===========================================================================*/

  void memorySetIoReadStub(ULO index, memoryIoReadFunc ioreadfunction)
  {
    memory_iobank_read[index>>1] = ioreadfunction;
  }

  void memorySetIoWriteStub(ULO index, memoryIoWriteFunc iowritefunction)
  {
    memory_iobank_write[index>>1] = iowritefunction;
  }

  /*===========================================================================*/
  /* Clear all IO-register accessors                                           */
  /*===========================================================================*/

  void memoryIoClear(void)
  {
    ULO i;

    // Array has 257 elements to account for long writes to the last address.
    for (i = 0; i <= 512; i += 2) {
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
    if(address >= 0xfc0000) {
      UBY *p = NULL;
      address = (address & 0xffffff) - 0xf80000;
      p = memory_kick + address;
      memoryWriteByteToPointer(data, p);
    } 
    else
      memoryKickA1000BootstrapSetMapped(false);
  }

  void memoryKickWriteWordA1000WCS(UWO data, ULO address)
  {
    if(address >= 0xfc0000) {
      UBY *p = NULL;
      address = (address & 0xffffff) - 0xf80000;
      p = memory_kick + address;
      memoryWriteWordToPointer(data, p);
    } 
    else
      memoryKickA1000BootstrapSetMapped(false);
  }

  void memoryKickWriteLongA1000WCS(ULO data, ULO address)
  {
    if(address >= 0xfc0000) {
      UBY *p = NULL;
      address = (address & 0xffffff) - 0xf80000;
      p = memory_kick + address ;
      memoryWriteLongToPointer(data, p);
    } 
    else
      memoryKickA1000BootstrapSetMapped(false);
  }

  void memoryKickMap(void)
  {
    ULO basebank = memory_kickimage_basebank & 0xf8;
    for (ULO bank = basebank; bank < (basebank + 8); bank++)
    {
      if(!memory_a1000_bootstrap_mapped)
        memoryBankSet(memoryKickReadByte,
          memoryKickReadWord,
          memoryKickReadLong,
          memoryKickWriteByte,
          memoryKickWriteWord,
          memoryKickWriteLong,
          memory_kick,
          bank,
          memory_kickimage_basebank,
          FALSE);
      else
        memoryBankSet(memoryKickReadByte,
          memoryKickReadWord,
          memoryKickReadLong,
          memoryKickWriteByteA1000WCS,
          memoryKickWriteWordA1000WCS,
          memoryKickWriteLongA1000WCS,
          memory_kick,
          bank,
          memory_kickimage_basebank,
          FALSE);
    }
  }

  void memoryKickExtendedMap(void)
  {
    if (memory_kickimage_ext_size == 0)
      return;

    ULO basebank = memory_kickimage_ext_basebank;
    ULO numbanks = memory_kickimage_ext_size / 65536;

    for (ULO bank = basebank; bank < (basebank + numbanks); bank++)
    {
      memoryBankSet(memoryKickExtendedReadByte,
      memoryKickExtendedReadWord,
      memoryKickExtendedReadLong,
      memoryKickExtendedWriteByte,
      memoryKickExtendedWriteWord,
      memoryKickExtendedWriteLong,
      memory_kick_ext,
      bank,
      basebank,
      FALSE);
    }
  }

  void memoryKickA1000BootstrapSetMapped(const bool bBootstrapMapped)
  {
    if(!memory_a1000_wcs || !memory_a1000_bootstrap) return;

    fellowAddLog("memoryKickSetA1000BootstrapMapped(%s)\n", 
      bBootstrapMapped ? "true" : "false");

    if (bBootstrapMapped) {
      memcpy(memory_kick, memory_a1000_bootstrap, 262144);
      memory_kickimage_version = 0;
    } else {
      memcpy(memory_kick, memory_kick + 262144, 262144);
      memory_kickimage_version = (memory_kick[262144 + 12] << 8) | memory_kick[262144 + 13];
      if (memory_kickimage_version == 0xffff)
        memory_kickimage_version = 0;
    }

    if(bBootstrapMapped != memory_a1000_bootstrap_mapped) {
      memory_a1000_bootstrap_mapped = bBootstrapMapped;
      memoryKickMap();
    }
  }

  void memoryKickA1000BootstrapFree(void)
  {
    if(memory_a1000_bootstrap != NULL) {
      free(memory_a1000_bootstrap);
      memory_a1000_bootstrap = NULL;
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
      case MEMORY_ROM_ERROR_SIZE:
        sprintf(error3,
	  "Illegal size: %u bytes, size must be either 8kB (A1000 bootstrap ROM), 256kB or 512kB.",
	  data);
        break;
      case MEMORY_ROM_ERROR_AMIROM_VERSION:
        sprintf(error3, "Unsupported encryption method, version found was %u",
	  data);
        break;
      case MEMORY_ROM_ERROR_AMIROM_READ:
        sprintf(error3, "Read error in encrypted Kickstart or keyfile");
        break;
      case MEMORY_ROM_ERROR_KEYFILE:
        sprintf(error3, "Unable to access keyfile %s", memory_key);
        break;
      case MEMORY_ROM_ERROR_EXISTS_NOT:
        sprintf(error3, "File does not exist");
        break;
      case MEMORY_ROM_ERROR_FILE:
        sprintf(error3, "File is a directory");
        break;
      case MEMORY_ROM_ERROR_KICKDISK_NOT:
        sprintf(error3, "The ADF-image is not a kickdisk");
        break;
      case MEMORY_ROM_ERROR_CHECKSUM:
        sprintf(error3,
	  "The Kickstart image has a checksum error, checksum is %X",
	  data);
        break;
      case MEMORY_ROM_ERROR_KICKDISK_SUPER:
        sprintf(error3,
	  "The ADF-image contains a superkickstart. Fellow can not handle it.");
        break;
      case MEMORY_ROM_ERROR_BAD_BANK:
        sprintf(error3, "The ROM has a bad baseaddress: %X",
	  memory_kickimage_basebank*0x10000);
        break;
    }
    fellowAddLogRequester(FELLOW_REQUESTER_TYPE_ERROR, "%s\n%s\n%s\n", error1, error2, error3);
    memoryKickSettingsClear();
  }

  /*============================================================================*/
  /* Returns the checksum of the current kickstart image.                       */
  /*============================================================================*/

  ULO memoryKickChksum(void)
  {
    ULO sum, lastsum, i;

    sum = lastsum = 0;
    for (i = 0; i < 0x80000; i += 4) {
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
    ULO ver, rev;

    ver = (rom[12] << 8) | rom[13];
    rev = (rom[14] << 8) | rom[15];
    if (ver == 65535) memory_kickimage_version = 28;
    else if (ver < 29) memory_kickimage_version = 29;
    else if (ver > 41) memory_kickimage_version = 41;
    else memory_kickimage_version = ver;
    sprintf(s, 
      "%s (%u.%u)",
      memory_kickimage_versionstrings[memory_kickimage_version - 28],
      ver,
      rev);
    return s;
  }

  /*============================================================================*/
  /* Verifies that a loaded Kickstart is OK                                     */
  /*============================================================================*/

  void memoryKickOK(void)
  {
    ULO chksum, basebank;
    bool bVerifyChecksum = !memory_a1000_wcs;

    if (bVerifyChecksum && ((chksum = memoryKickChksum()) != 0))
      memoryKickError(MEMORY_ROM_ERROR_CHECKSUM, chksum);
    else
    {
      basebank = memory_kick[5];
      if ((basebank == 0xf8) || (basebank == 0xfc)) {
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
    STR *keybuffer = NULL;
    ULO keysize, filesize = 0, keypos = 0, c;
    FILE *KF, *RF;

    /* Read key */

    if ((KF = fopen(keyfile, "rb")) != NULL)
    {
      fseek(KF, 0, SEEK_END);
      keysize = ftell(KF);
      keybuffer = (STR*)malloc(keysize);
      if (keybuffer != NULL)
      {
        fseek(KF, 0, SEEK_SET);
        fread(keybuffer, 1, keysize, KF);
      }
      fclose(KF);
    }
    else
    {
#ifdef WIN32
      HMODULE hAmigaForeverDLL = NULL;
      STR *strLibName = TEXT("amigaforever.dll");
      STR strPath[CFG_FILENAME_LENGTH] = "";
      STR strAmigaForeverRoot[CFG_FILENAME_LENGTH] = "";
      DWORD dwRet = 0;
     
      // the preferred way to locate the DLL is by relative path, so it is
      // of a matching version (DVD/portable installation with newer version
      // than what is installed)
      if(fileopsGetWinFellowInstallationPath(strPath, CFG_FILENAME_LENGTH)) {
        strncat(strPath, "\\..\\Player\\", 11);
        strncat(strPath, strLibName, strlen(strLibName) + 1);
	hAmigaForeverDLL = LoadLibrary(strPath);
      }

      if(!hAmigaForeverDLL) {
        // DLL not found via relative path, fallback to env. variable
        dwRet = GetEnvironmentVariable("AMIGAFOREVERROOT", strAmigaForeverRoot, CFG_FILENAME_LENGTH);
	if((dwRet > 0) && strAmigaForeverRoot) {
	  TCHAR strTemp[CFG_FILENAME_LENGTH];
	  _tcscpy(strTemp, strAmigaForeverRoot);
          if (strTemp[_tcslen(strTemp) - 1] == '/' || strTemp[_tcslen(strTemp) - 1] == '\\')
          _tcscat(strTemp, TEXT("\\"));
	  _stprintf(strPath, TEXT("%sPlayer\\%s"), strTemp, strLibName);
	  hAmigaForeverDLL = LoadLibrary(strPath);
	}
      }

      if(hAmigaForeverDLL) {
	typedef DWORD (STDAPICALLTYPE *PFN_GetKey)(LPVOID lpvBuffer, DWORD dwSize);

	PFN_GetKey pfnGetKey = (PFN_GetKey)GetProcAddress(hAmigaForeverDLL, "GetKey");
	if (pfnGetKey) {
	  keysize = pfnGetKey(NULL, 0);
	  if (keysize) {
	    keybuffer = (STR*)malloc(keysize);
 
	    if (keybuffer) {
	      if (pfnGetKey(keybuffer, keysize) == keysize) {
                // key successfully retrieved
	      }
              else {
                memoryKickError(MEMORY_ROM_ERROR_KEYFILE, 0);
                return -1;
              }
            }
	  }
        }
        FreeLibrary(hAmigaForeverDLL);
  #endif
      }

      if (!keybuffer) {
        memoryKickError(MEMORY_ROM_ERROR_KEYFILE, 0);
        return -1;
      }
    }

    if (!keybuffer)
      return -1;  

    /* Read file */

    if ((RF = fopen(filename, "rb")) != NULL)
    {
      fseek(RF, 11, SEEK_SET);
      while (((c = fgetc(RF)) != EOF) && filesize < 524288)
      {
	      if (keysize != 0)
	        c ^= keybuffer[keypos++];
	      if (keypos == keysize)
	        keypos = 0;
  	    memory_kick[filesize++] = (UBY) c;
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
    ULO version;
    STR IDString[12];

    memory_a1000_wcs = false;

    fread(IDString, 11, 1, F);
    version = IDString[10] - '0';
    IDString[10] = '\0';
    if (stricmp(IDString, "AMIROMTYPE") == 0)
    { /* Header seems OK */
      if (version != 1)
      {
	if(!suppressgui)
          memoryKickError(MEMORY_ROM_ERROR_AMIROM_VERSION, version);
	return TRUE;  /* File was handled */
      }
      else
      { /* Seems to be a file we can handle */
	ULO size;

	fclose(F);

	size = memoryKickDecodeAF(filename,
	  memory_key, memory_kick);
	if (size == -1)
	{
          if(!suppressgui)
	    memoryKickError(MEMORY_ROM_ERROR_AMIROM_READ, 0);
	  return TRUE;
	}
	if (size != 8192 && size != 262144 && size != 524288)
	{
          if(!suppressgui)
	    memoryKickError(MEMORY_ROM_ERROR_SIZE, size);
	  return TRUE;
	}

        if (size == 8192)
        { /* Load A1000 bootstrap ROM */
          memory_a1000_wcs = true;

          if(memory_a1000_bootstrap == NULL)
            memory_a1000_bootstrap = (UBY *) malloc(262144);

          if(memory_a1000_bootstrap) {
            ULO lCRC32 = 0;
            memset(memory_a1000_bootstrap, 0xff, 262144);
            memcpy(memory_a1000_bootstrap, memory_kick, 8192);
            lCRC32 = crc32(0, memory_kick, 8192);
            if (lCRC32 != 0x62F11C04) {
              free(memory_a1000_bootstrap);
              memory_a1000_bootstrap = NULL;
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

  void memoryKickLoad(void)
  {
    FILE *F;
    BOOLE kickdisk = FALSE;
    STR *suffix, *lastsuffix;
    BOOLE afkick = FALSE;

    memory_a1000_wcs = false;

    /* New file is different from previous */
    /* Must load file */

    fs_navig_point *fsnp;

    memory_kickimage_none = FALSE;/* Initially Kickstart is expected to be OK */
    if ((fsnp = fsWrapMakePoint(memory_kickimage)) == NULL)
      memoryKickError(MEMORY_ROM_ERROR_EXISTS_NOT, 0);
    else
    {
      if (fsnp->type != FS_NAVIG_FILE)
	memoryKickError(MEMORY_ROM_ERROR_FILE, 0);
      else
      { /* File passed initial tests */
	if ((F = fopen(memory_kickimage, "rb")) == NULL)
	  memoryKickError(MEMORY_ROM_ERROR_EXISTS_NOT, 0);
	else memory_kickimage_size = fsnp->size;
      }
      free(fsnp);
    }

    /* Either the file is open, or memory_kickimage_none is TRUE */ 

    if (!memory_kickimage_none)
    {

      /* File opened successfully */

      /* Kickdisk flag */

      suffix = strchr(memory_kickimage, '.');
      if (suffix != NULL)
      {
	lastsuffix = suffix;
	while ((suffix = strchr(lastsuffix + 1, '.')) != NULL)
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
          
          if(memory_a1000_bootstrap == NULL) 
            memory_a1000_bootstrap = (UBY *) malloc(262144);

          if(memory_a1000_bootstrap) {
            ULO lCRC32 = 0;
            memset(memory_a1000_bootstrap, 0xff, 262144);
            fread(memory_a1000_bootstrap, 1, 8192, F);
            memcpy(memory_kick, memory_a1000_bootstrap, 262144);
            memcpy(memory_kick + 262144, memory_a1000_bootstrap, 262144);
            lCRC32 = crc32(0, memory_a1000_bootstrap, 8192);
            if (lCRC32 != 0x62F11C04) {
              free(memory_a1000_bootstrap);
              memory_a1000_bootstrap = NULL;
              memoryKickError(MEMORY_ROM_ERROR_CHECKSUM, lCRC32);
              return;
            }
          }
        }
        else if (memory_kickimage_size == 262144)
	{   /* Load 256k ROM */
	  fread(memory_kick, 1, 262144, F);
	  memcpy(memory_kick + 262144, memory_kick, 262144);
	}
	else if (memory_kickimage_size == 524288)/* Load 512k ROM */
	  fread(memory_kick, 1, 524288, F);
	else
	{                                     /* Rom size is wrong */
	  memoryKickError(MEMORY_ROM_ERROR_SIZE, memory_kickimage_size);
	}
	fclose(F);
      }
    }
    if (!memory_kickimage_none) {
      memoryKickOK();
      memoryKickA1000BootstrapSetMapped(true);
    }
  }

  /*============================================================================*/
  /* Clear memory used for kickstart image                                      */
  /*============================================================================*/

  void memoryKickClear(void)
  {
    memset(memory_kick, 0, 0x80000);
  }

  /*============================================================================*/
  /* Free memory used for extended kickstart image                              */
  /*============================================================================*/

  void memoryKickExtendedFree(void)
  {
    if (memory_kick_ext) {
      free(memory_kick_ext);
      memory_kick_ext = NULL;
    }
    memory_kickimage_ext_size = 0;
    memory_kickimage_ext_basebank = 0;
  }

  /*============================================================================*/
  /* Load extended Kickstart ROM into memory                                    */
  /*============================================================================*/

  void memoryKickExtendedLoad(void)
  {
    FILE *F;
    fs_navig_point *fsnp;
    ULO size = 0;

    /* New file is different from previous, must load file */

    memoryKickExtendedFree();

    if ((fsnp = fsWrapMakePoint(memory_kickimage_ext)) == NULL)
      return;
    else
    {
      if (fsnp->type != FS_NAVIG_FILE)
        return;
      else
      { /* File passed initial tests */
        if ((F = fopen(memory_kickimage_ext, "rb")) == NULL)
          return;
        else 
          size = fsnp->size;
      }
      free(fsnp);
    }

    if (F) {
      fseek(F, 0, SEEK_SET);

      if (size == 262155 || size == 524299) {
        // Amiga Forever - encrypted ROM?
        ULO version;
        STR IDString[12];

        fread(IDString, 11, 1, F);
        version = IDString[10] - '0';
        IDString[10] = '\0';
        if (stricmp(IDString, "AMIROMTYPE") == 0)
        { /* Header seems OK */
          if (version != 1)
            return;
          else
          { /* Seems to be a file we can handle */
            memory_kick_ext = (UBY *) malloc(size - 11);
            size = memoryKickDecodeAF(memory_kickimage_ext, memory_key, memory_kick_ext);
            memory_kickimage_ext_size = size;
          }
        }
      }
      else {
        if (size == 262144 || size == 524288) {
          memory_kick_ext = (UBY *) malloc(size);

          if (memory_kick_ext) {
            memset(memory_kick_ext, 0xff, size);
            fread(memory_kick_ext, 1, size, F);
            memory_kickimage_ext_size = size;
          }
          else
            return;
        }
      }

      memory_kickimage_ext_basebank = memory_kick_ext[5];

      // AROS extended ROM does not have basebank at byte 6, override
      if (memory_kickimage_ext_basebank == 0xf8)
        memory_kickimage_ext_basebank = 0xe0;

      fclose(F);
      F = NULL;
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

__inline  UBY memoryReadByte(ULO address)
  {
    UBY *memory_ptr = memory_bank_pointer[address>>16];
    if (memory_ptr != NULL)
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

__inline  UWO memoryReadWord(ULO address)
  {
    UBY *memory_ptr = memory_bank_pointer[address>>16];
    if ((memory_ptr != NULL) && !(address & 1))
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

  __inline ULO memoryReadLong(ULO address)
  {
    UBY *memory_ptr = memory_bank_pointer[address>>16];
    if ((memory_ptr != NULL) && !(address & 1))
    {
      UBY *p = memory_ptr + address;
      return memoryReadLongFromPointer(p);
    }
    return memoryReadLongViaBankHandler(address);
  }

  void memoryWriteByte(UBY data, ULO address)
  {
    ULO bank = address>>16;
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
    ULO bank = address>>16;
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
    ULO bank = address>>16;
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

  BOOLE memorySetChipSize(ULO chipsize)
  {
    BOOLE needreset = (memory_chipsize != chipsize);
    memory_chipsize = chipsize;
    return needreset;
  }

  ULO memoryGetChipSize(void)
  {
    return memory_chipsize;
  }

  BOOLE memorySetFastSize(ULO fastsize)
  {
    BOOLE needreset = (memory_fastsize != fastsize);
    memory_fastsize = fastsize;
    if (needreset) memoryFastAllocate();
    return needreset;
  }

  ULO memoryGetFastSize(void)
  {
    return memory_fastsize;
  }

  void memorySetFastAllocatedSize(ULO fastallocatedsize)
  {
    memory_fastallocatedsize = fastallocatedsize;
  }

  ULO memoryGetFastAllocatedSize(void)
  {
    return memory_fastallocatedsize;
  }

  BOOLE memorySetSlowSize(ULO slowsize)
  {
    BOOLE needreset = (memory_slowsize != slowsize);
    memory_slowsize = slowsize;
    return needreset;
  }

  ULO memoryGetSlowSize(void)
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

  BOOLE memorySetAddress32Bit(BOOLE address32bit)
  {
    BOOLE needreset = memory_address32bit != address32bit;
    memory_address32bit = address32bit;
    return needreset;
  }

  BOOLE memoryGetAddress32Bit(void)
  {
    return memory_address32bit;
  }

  BOOLE memorySetKickImage(STR *kickimage)
  {
    BOOLE needreset = !!strncmp(memory_kickimage, kickimage, CFG_FILENAME_LENGTH);
    strncpy(memory_kickimage, kickimage, CFG_FILENAME_LENGTH);
    if (needreset) memoryKickLoad();
    return needreset;
  }

  BOOLE memorySetKickImageExtended(STR *kickimageext)
  {
    BOOLE needreset = !!strncmp(memory_kickimage_ext, kickimageext, CFG_FILENAME_LENGTH);
    strncpy(memory_kickimage_ext, kickimageext, CFG_FILENAME_LENGTH);
    if (needreset) 
      memoryKickExtendedLoad();
    return needreset;
  }


  STR *memoryGetKickImage(void)
  {
    return memory_kickimage;
  }

  void memorySetKey(STR *key)
  {
    strncpy(memory_key, key, CFG_FILENAME_LENGTH);
  }

  STR *memoryGetKey(void)
  {
    return memory_key;
  }

  ULO memoryGetKickImageBaseBank(void)
  {
    return memory_kickimage_basebank;
  }

  ULO memoryGetKickImageVersion(void)
  {
    return memory_kickimage_version;
  }

  BOOLE memoryGetKickImageOK(void)
  {
    return !memory_kickimage_none;
  }

  ULO memoryInitialPC(void)
  {
    return memory_initial_PC;
  }

  ULO memoryInitialSP(void)
  {
    return memory_initial_SP;
  }

  /*============================================================================*/
  /* Sets all settings a clean state                                            */
  /*============================================================================*/

  void memoryChipSettingsClear(void)
  {
    memorySetChipSize(0x200000);
    memoryChipClear();
  }

  void memoryFastSettingsClear(void)
  {
    memory_fast = NULL;
    memory_fast_baseaddress = 0;
    memorySetFastAllocatedSize(0);
    memorySetFastSize(0);
  }

  void memorySlowSettingsClear(void)
  {
    memorySetSlowSize(0x1c0000);
    memorySlowClear();
  }

  void memoryIoSettingsClear(void)
  {
    memoryIoClear();
  }

  void memoryKickSettingsClear(void)
  {
    memory_kickimage[0] = '\0';
    memory_kickimage_none = TRUE;
    memoryKickClear();
  }

  void memoryEmemSettingsClear(void)
  {
    memoryEmemCardsRemove();
    memoryEmemClear();
  }

  void memoryDmemSettingsClear(void)
  {
    memoryDmemSetCounter(0);
    memoryDmemClear();
  }

  void memoryBankSettingsClear(void)
  {
    memoryBankClearAll();
  }

  /*==============*/
  /* Generic init */
  /*==============*/

  void memorySaveState(FILE *F)
  {
    fwrite(&memory_chipsize, sizeof(memory_chipsize), 1, F);
    fwrite(&memory_slowsize, sizeof(memory_slowsize), 1, F);
    fwrite(&memory_fastsize, sizeof(memory_fastsize), 1, F);
    if (memory_chipsize > 0)
    {
      fwrite(&memory_chip[0], sizeof(UBY), memory_chipsize, F);
    }
    if (memory_slowsize > 0)
    {
      fwrite(&memory_slow[0], sizeof(UBY), memory_slowsize, F);
    }
    if (memory_fastsize > 0)
    {
      fwrite(memory_fast, sizeof(UBY), memory_fastsize, F);
    }
  }

  void memoryLoadState(FILE *F)
  {
    fread(&memory_chipsize, sizeof(memory_chipsize), 1, F);
    fread(&memory_slowsize, sizeof(memory_slowsize), 1, F);
    fread(&memory_fastsize, sizeof(memory_fastsize), 1, F);
    if (memory_chipsize > 0)
    {
      fread(&memory_chip[0], sizeof(UBY), memory_chipsize, F);
    }
    if (memory_slowsize > 0)
    {
      fread(&memory_slow[0], sizeof(UBY), memory_slowsize, F);
    }
    if (memory_fastsize > 0)
    {
      fread(memory_fast, sizeof(UBY), memory_fastsize, F);
    }
  }

  void memoryEmulationStart(void)
  {
    memoryIoClear();
  }

  void memoryEmulationStop(void)
  {
  }

  void memorySoftReset(void)
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

  void memoryHardReset(void)
  {
    fellowAddLog("memoryHardReset()\n");

    memoryChipClear(),
    memoryFastClear();
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

  void memoryHardResetPost(void)
  {
    memoryEmemCardInit();
  }

  void memoryStartup(void)
  {
    memorySetAddress32Bit(TRUE);
    memoryBankSettingsClear();
    memorySetAddress32Bit(FALSE);
    memoryChipSettingsClear();
    memoryFastSettingsClear();
    memorySlowSettingsClear();
    memoryIoSettingsClear();
    memoryKickSettingsClear();
    memoryEmemSettingsClear();
    memoryDmemSettingsClear();
  }

  void memoryShutdown(void)
  {
    memoryFastFree();
    memoryKickExtendedFree();
    memoryKickA1000BootstrapFree();
  }

