#pragma once

#include <stdio.h>
#include <vector>
#include "fellow/api/defs.h"
#include "fellow/api/vm/MemorySystemTypes.h"

/* Access for chipset emulation that already have validated addresses */

#define chipmemReadByte(address) (memory_chip[address])
#define chipmemReadWord(address) ((((UWO)memory_chip[address]) << 8) | ((UWO)memory_chip[(address) + 1]))
#define chipmemWriteWord(data, address)                                                                                                                                                                \
  memory_chip[address] = (UBY)((data) >> 8);                                                                                                                                                           \
  memory_chip[(address) + 1] = (UBY)data

/* Memory access functions */

extern UBY memoryReadByte(ULO address);
extern UWO memoryReadWord(ULO address);
extern ULO memoryReadLong(ULO address);
extern void memoryWriteByte(UBY data, ULO address);
extern void memoryWriteWord(UWO data, ULO address);
extern void memoryWriteLong(ULO data, ULO address);

extern UWO memoryChipReadWord(ULO address);
extern void memoryChipWriteWord(UWO data, ULO address);

#define memoryReadByteFromPointer(address) ((address)[0])
#define memoryReadWordFromPointer(address) (((address)[0] << 8) | (address)[1])
#define memoryReadLongFromPointer(address) (((address)[0] << 24) | ((address)[1] << 16) | ((address)[2] << 8) | (address)[3])

extern void memoryWriteLongToPointer(ULO data, UBY *address);
extern bool memoryIsValidPointerAddress(ULO address, ULO size);

/* IO Bank functions */

extern void memorySetIoReadStub(ULO index, fellow::api::vm::IoReadFunc ioreadfunction);
extern void memorySetIoWriteStub(ULO index, fellow::api::vm::IoWriteFunc iowritefunction);

/* For the copper and testing */
extern fellow::api::vm::IoReadFunc memory_iobank_read[257];
extern fellow::api::vm::IoWriteFunc memory_iobank_write[257];

/* Expansion card functions */

extern void memoryEmemClear();
extern void memoryEmemCardAdd(fellow::api::vm::EmemCardInitFunc cardinit, fellow::api::vm::EmemCardMapFunc cardmap);
extern void memoryEmemSet(ULO index, ULO value);
extern void memoryEmemMirror(ULO emem_offset, UBY *src, ULO size);

/* Device memory functions. fhfile is using these. */

extern void memoryDmemSetByte(UBY data);
extern void memoryDmemSetWord(UWO data);
extern void memoryDmemSetLong(ULO data);
extern void memoryDmemSetLongNoCounter(ULO data, ULO offset);
extern void memoryDmemSetString(const STR *st);
extern void memoryDmemSetCounter(ULO val);
extern ULO memoryDmemGetCounter();
extern ULO memoryDmemGetCounterWithoutOffset();
extern void memoryDmemClear();

/* Module management functions */

extern void memorySoftReset();
extern void memoryHardReset();
extern void memoryHardResetPost();
extern void memoryEmulationStart();
extern void memoryEmulationStop();
extern void memoryStartup();
extern void memoryShutdown();

// Memory bank functions

extern void memoryBankSet(const fellow::api::vm::MemoryBankDescriptor &memoryBankDescriptor);
extern UBY *memoryAddressToPtr(ULO address);
extern void memoryChipMap(bool overlay);

/* Memory configuration properties */

extern bool memorySetChipSize(ULO chipsize);
extern ULO memoryGetChipSize();
extern bool memorySetFastSize(ULO fastsize);
extern ULO memoryGetFastSize();
extern void memorySetFastAllocatedSize(ULO fastallocatedsize);
extern ULO memoryGetFastAllocatedSize();
extern bool memorySetSlowSize(ULO slowsize);
extern ULO memoryGetSlowSize();
extern bool memorySetUseAutoconfig(bool useautoconfig);
extern bool memoryGetUseAutoconfig();
extern bool memorySetAddressSpace32Bit(bool address32bit);
extern bool memoryGetAddressSpace32Bit();
extern bool memorySetKickImage(STR *kickimage);
extern bool memorySetKickImageExtended(STR *kickimageext);
extern STR *memoryGetKickImage();
extern void memorySetKey(STR *key);
extern STR *memoryGetKey();
extern BOOLE memoryGetKickImageOK();

extern std::vector<fellow::api::vm::MemoryMapDescriptor> memoryGetMemoryMapDescriptors();

/* Derived from memory configuration */

extern ULO memoryGetKickImageBaseBank();
extern ULO memoryGetKickImageVersion();
extern ULO memoryInitialPC();
extern ULO memoryInitialSP();

/* Kickstart decryption */
extern int memoryKickLoadAF2(STR *filename, FILE *F, UBY *memory_kick, const bool);

/* Kickstart load error handling */

#define MEMORY_ROM_ERROR_SIZE 0
#define MEMORY_ROM_ERROR_AMIROM_VERSION 1
#define MEMORY_ROM_ERROR_AMIROM_READ 2
#define MEMORY_ROM_ERROR_KEYFILE 3
#define MEMORY_ROM_ERROR_EXISTS_NOT 4
#define MEMORY_ROM_ERROR_FILE 5
#define MEMORY_ROM_ERROR_KICKDISK_NOT 6
#define MEMORY_ROM_ERROR_CHECKSUM 7
#define MEMORY_ROM_ERROR_KICKDISK_SUPER 8
#define MEMORY_ROM_ERROR_BAD_BANK 9

/* Global variables */

extern UBY memory_chip[];
extern UBY *memory_fast;
extern UBY memory_slow[];
extern UBY memory_kick[];
extern ULO memory_chipsize;
extern UBY memory_emem[];

extern ULO potgor;

extern ULO memory_fault_address;
extern BOOLE memory_fault_read;
