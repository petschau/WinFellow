#pragma once

#include "Defs.h"

/* Access for chipset emulation that already have validated addresses */

#define chipmemReadByte(address) (memory_chip[address])
#define chipmemReadWord(address) ((((uint16_t)memory_chip[address]) << 8) | ((uint16_t)memory_chip[address + 1]))
#define chipmemWriteWord(data, address)                                                                                                                                       \
  memory_chip[address] = (uint8_t)(data >> 8);                                                                                                                                \
  memory_chip[address + 1] = (uint8_t)data

/* Memory access functions */

extern uint8_t memoryReadByte(uint32_t address);
extern uint16_t memoryReadWord(uint32_t address);
extern uint32_t memoryReadLong(uint32_t address);
extern void memoryWriteByte(uint8_t data, uint32_t address);
extern void memoryWriteWord(uint16_t data, uint32_t address);
extern void memoryWriteLong(uint32_t data, uint32_t address);

extern uint16_t memoryChipReadWord(uint32_t address);
extern void memoryChipWriteWord(uint16_t data, uint32_t address);

#define memoryReadByteFromPointer(address) (address[0])
#define memoryReadWordFromPointer(address) ((address[0] << 8) | address[1])
#define memoryReadLongFromPointer(address) ((address[0] << 24) | (address[1] << 16) | (address[2] << 8) | address[3])

extern void memoryWriteLongToPointer(uint32_t data, uint8_t *address);

/* IO Bank functions */

typedef uint16_t (*memoryIoReadFunc)(uint32_t address);
typedef void (*memoryIoWriteFunc)(uint16_t data, uint32_t address);

extern void memorySetIoReadStub(uint32_t index, memoryIoReadFunc ioreadfunction);
extern void memorySetIoWriteStub(uint32_t index, memoryIoWriteFunc iowritefunction);

/* For the copper */
extern memoryIoWriteFunc memory_iobank_write[257];

/* Expansion card functions */

typedef void (*memoryEmemCardInitFunc)();
typedef void (*memoryEmemCardMapFunc)(uint32_t);

extern void memoryEmemClear();
extern void memoryEmemCardAdd(memoryEmemCardInitFunc cardinit, memoryEmemCardMapFunc cardmap);
extern void memoryEmemSet(uint32_t index, uint32_t data);
extern void memoryEmemMirror(uint32_t emem_offset, uint8_t *src, uint32_t size);

/* Device memory functions. fhfile is using these. */

extern void memoryDmemSetByte(uint8_t data);
extern void memoryDmemSetWord(uint16_t data);
extern void memoryDmemSetLong(uint32_t data);
extern void memoryDmemSetLongNoCounter(uint32_t data, uint32_t offset);
extern void memoryDmemSetString(const char *data);
extern void memoryDmemSetCounter(uint32_t val);
extern uint32_t memoryDmemGetCounter();
extern uint32_t memoryDmemGetCounterWithoutOffset();
extern void memoryDmemClear();

/* Module management functions */

extern void memorySaveState(FILE *F);
extern void memoryLoadState(FILE *F);
extern void memorySoftReset();
extern void memoryHardReset();
extern void memoryHardResetPost();
extern void memoryEmulationStart();
extern void memoryEmulationStop();
extern void memoryStartup();
extern void memoryShutdown();

/* Memory bank functions */

typedef uint8_t (*memoryReadByteFunc)(uint32_t address);
typedef uint16_t (*memoryReadWordFunc)(uint32_t address);
typedef uint32_t (*memoryReadLongFunc)(uint32_t address);
typedef void (*memoryWriteByteFunc)(uint8_t data, uint32_t address);
typedef void (*memoryWriteWordFunc)(uint16_t data, uint32_t address);
typedef void (*memoryWriteLongFunc)(uint32_t data, uint32_t address);

extern memoryReadByteFunc memory_bank_readbyte[65536];
extern memoryReadWordFunc memory_bank_readword[65536];
extern memoryReadLongFunc memory_bank_readlong[65536];
extern memoryWriteByteFunc memory_bank_writebyte[65536];
extern memoryWriteWordFunc memory_bank_writeword[65536];
extern memoryWriteLongFunc memory_bank_writelong[65536];

extern uint8_t *memory_bank_pointer[65536];
extern uint8_t *memory_bank_datapointer[65536];

extern void memoryBankSet(
    memoryReadByteFunc rb,
    memoryReadWordFunc rw,
    memoryReadLongFunc rl,
    memoryWriteByteFunc wb,
    memoryWriteWordFunc ww,
    memoryWriteLongFunc wl,
    uint8_t *basep,
    uint32_t bank,
    uint32_t basebank,
    BOOLE pointer_can_write);
extern uint8_t *memoryAddressToPtr(uint32_t address);
extern void memoryChipMap(bool overlay);

/* Memory configuration properties */

extern BOOLE memorySetChipSize(uint32_t chipsize);
extern uint32_t memoryGetChipSize();
extern BOOLE memorySetFastSize(uint32_t fastsize);
extern uint32_t memoryGetFastSize();
extern void memorySetFastAllocatedSize(uint32_t fastallocatedsize);
extern uint32_t memoryGetFastAllocatedSize();
extern BOOLE memorySetSlowSize(uint32_t bogosize);
extern uint32_t memoryGetSlowSize();
extern bool memorySetUseAutoconfig(bool useautoconfig);
extern bool memoryGetUseAutoconfig();
extern BOOLE memorySetAddress32Bit(BOOLE address32bit);
extern BOOLE memoryGetAddress32Bit();
extern BOOLE memorySetKickImage(const char *kickimage);
extern BOOLE memorySetKickImageExtended(const char *kickimageext);
extern char *memoryGetKickImage();
extern void memorySetKey(const char *key);
extern char *memoryGetKey();
extern BOOLE memoryGetKickImageOK();

/* Derived from memory configuration */

extern uint32_t memoryGetKickImageBaseBank();
extern uint32_t memoryGetKickImageVersion();
extern uint32_t memoryInitialPC();
extern uint32_t memoryInitialSP();

/* Kickstart decryption */
extern int memoryKickLoadAF2(char *filename, FILE *F, uint8_t *memory_kick, const bool);

/* Kickstart load error handling */

enum class MemoryRomError
{
  MEMORY_ROM_ERROR_SIZE = 0,
  MEMORY_ROM_ERROR_AMIROM_VERSION = 1,
  MEMORY_ROM_ERROR_AMIROM_READ = 2,
  MEMORY_ROM_ERROR_KEYFILE = 3,
  MEMORY_ROM_ERROR_EXISTS_NOT = 4,
  MEMORY_ROM_ERROR_FILE = 5,
  MEMORY_ROM_ERROR_KICKDISK_NOT = 6,
  MEMORY_ROM_ERROR_CHECKSUM = 7,
  MEMORY_ROM_ERROR_KICKDISK_SUPER = 8,
  MEMORY_ROM_ERROR_BAD_BANK = 9
};

/* Global variables */

extern uint8_t memory_chip[];
extern uint8_t *memory_fast;
extern uint8_t memory_slow[];
extern uint8_t memory_kick[];
extern uint32_t memory_chipsize;
extern uint8_t memory_emem[];

extern uint32_t potgor;

extern uint32_t memory_fault_address;
extern BOOLE memory_fault_read;
