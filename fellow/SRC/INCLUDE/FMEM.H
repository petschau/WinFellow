#ifndef FMEM_H
#define FMEM_H

/* Rename assembly functions */

#define fetb ASMRENAME(fetb)
#define fetw ASMRENAME(fetw)
#define fetl ASMRENAME(fetl)
#define wrib ASMRENAME(wrib)
#define wriw ASMRENAME(wriw)
#define wril ASMRENAME(wril)

#define memoryChipReadByte ASMRENAME(memoryChipReadByte)
#define memoryChipReadWord ASMRENAME(memoryChipReadWord)
#define memoryChipReadLong ASMRENAME(memoryChipReadLong)
#define memoryChipWriteByte ASMRENAME(memoryChipWriteByte)
#define memoryChipWriteWord ASMRENAME(memoryChipWriteWord)
#define memoryChipWriteLong ASMRENAME(memoryChipWriteLong)

#define memoryFastReadByte ASMRENAME(memoryFastReadByte)
#define memoryFastReadWord ASMRENAME(memoryFastReadWord)
#define memoryFastReadLong ASMRENAME(memoryFastReadLong)
#define memoryFastWriteByte ASMRENAME(memoryFastWriteByte)
#define memoryFastWriteWord ASMRENAME(memoryFastWriteWord)
#define memoryFastWriteLong ASMRENAME(memoryFastWriteLong)

#define memoryBogoReadByte ASMRENAME(memoryBogoReadByte)
#define memoryBogoReadWord ASMRENAME(memoryBogoReadWord)
#define memoryBogoReadLong ASMRENAME(memoryBogoReadLong)
#define memoryBogoWriteByte ASMRENAME(memoryBogoWriteByte)
#define memoryBogoWriteWord ASMRENAME(memoryBogoWriteWord)
#define memoryBogoWriteLong ASMRENAME(memoryBogoWriteLong)

#define memoryKickReadByte ASMRENAME(memoryKickReadByte)
#define memoryKickReadWord ASMRENAME(memoryKickReadWord)
#define memoryKickReadLong ASMRENAME(memoryKickReadLong)
#define memoryKickWriteByte ASMRENAME(memoryKickWriteByte)
#define memoryKickWriteWord ASMRENAME(memoryKickWriteWord)
#define memoryKickWriteLong ASMRENAME(memoryKickWriteLong)

#define memoryIOReadByte ASMRENAME(memoryIOReadByte)
#define memoryIOReadWord ASMRENAME(memoryIOReadWord)
#define memoryIOReadLong ASMRENAME(memoryIOReadLong)
#define memoryIOWriteByte ASMRENAME(memoryIOWriteByte)
#define memoryIOWriteWord ASMRENAME(memoryIOWriteWord)
#define memoryIOWriteLong ASMRENAME(memoryIOWriteLong)

#define memoryEmemReadByteASM ASMRENAME(memoryEmemReadByteASM)
#define memoryEmemReadWordASM ASMRENAME(memoryEmemReadWordASM)
#define memoryEmemReadLongASM ASMRENAME(memoryEmemReadLongASM)
#define memoryEmemWriteByteASM ASMRENAME(memoryEmemWriteByteASM)
#define memoryEmemWriteWordASM ASMRENAME(memoryEmemWriteWordASM)
#define memoryEmemWriteLongASM ASMRENAME(memoryEmemWriteLongASM)

#define memoryNAReadByte ASMRENAME(memoryNAReadByte)
#define memoryNAReadWord ASMRENAME(memoryNAReadWord)
#define memoryNAReadLong ASMRENAME(memoryNAReadLong)
#define memoryNAWriteByte ASMRENAME(memoryNAWriteByte)
#define memoryNAWriteWord ASMRENAME(memoryNAWriteWord)
#define memoryNAWriteLong ASMRENAME(memoryNAWriteLong)

#define memoryMysteryReadByte ASMRENAME(memoryMysteryReadByte)
#define memoryMysteryReadWord ASMRENAME(memoryMysteryReadWord)
#define memoryMysteryReadLong ASMRENAME(memoryMysteryReadLong)
#define memoryMysteryWriteByte ASMRENAME(memoryMysteryWriteByte)
#define memoryMysteryWriteWord ASMRENAME(memoryMysteryWriteWord)
#define memoryMysteryWriteLong ASMRENAME(memoryMysteryWriteLong)

#define memoryDmemReadByte ASMRENAME(memoryDmemReadByte)
#define memoryDmemReadWord ASMRENAME(memoryDmemReadWord)
#define memoryDmemReadLong ASMRENAME(memoryDmemReadLong)
#define memoryDmemWriteLongASM ASMRENAME(memoryDmemWriteLongASM)

#define rserdatr ASMRENAME(rserdatr)
//#define wserdat ASMRENAME(wserdat)
//#define wserper ASMRENAME(wserper)
#define rintenar ASMRENAME(rintenar)
#define rintreqr ASMRENAME(rintreqr)
#define rdefault ASMRENAME(rdefault)

#define wdmacon ASMRENAME(wdmacon)
#define wintena ASMRENAME(wintena)
#define wintreq ASMRENAME(wintreq)
#define wadcon ASMRENAME(wadcon)
#define wdefault ASMRENAME(wdefault)

#define fmemstart ASMRENAME(fmemstart)

/* Declare exported functions from assembly */

extern ULO fetb(ULO adress);
extern ULO fetw(ULO adress);
extern ULO fetl(ULO adress);
extern void wrib(ULO data, ULO adress);
extern void wriw(ULO data, ULO adress);
extern void wril(ULO data, ULO adress);

/* CHIP RAM INTERFACE */

extern ULO memoryChipReadByte(ULO ea);
extern ULO memoryChipReadWord(ULO ea);
extern ULO memoryChipReadLong(ULO ea);
extern void memoryChipWriteByte(ULO data, ULO ea);
extern void memoryChipWriteWord(ULO data, ULO ea);
extern void memoryChipWriteLong(ULO data, ULO ea);

/* FAST RAM INTERFACE */

extern ULO memoryFastReadByte(ULO ea);
extern ULO memoryFastReadWord(ULO ea);
extern ULO memoryFastReadLong(ULO ea);
extern void memoryFastWriteByte(ULO data, ULO ea);
extern void memoryFastWriteWord(ULO data, ULO ea);
extern void memoryFastWriteLong(ULO data, ULO ea);

/* BOGO RAM INTERFACE */

extern ULO memoryBogoReadByte(ULO ea);
extern ULO memoryBogoReadWord(ULO ea);
extern ULO memoryBogoReadLong(ULO ea);
extern void memoryBogoWriteByte(ULO data, ULO ea);
extern void memoryBogoWriteWord(ULO data, ULO ea);
extern void memoryBogoWriteLong(ULO data, ULO ea);

/* KICK ROM INTERFACE */

extern ULO memoryKickReadByte(ULO ea);
extern ULO memoryKickReadWord(ULO ea);
extern ULO memoryKickReadLong(ULO ea);
extern void memoryKickWriteByte(ULO data, ULO ea);
extern void memoryKickWriteWord(ULO data, ULO ea);
extern void memoryKickWriteLong(ULO data, ULO ea);

/* IO REGISTER INTERFACE */

extern ULO memoryIOReadByte(ULO ea);
extern ULO memoryIOReadWord(ULO ea);
extern ULO memoryIOReadLong(ULO ea);
extern void memoryIOWriteByte(ULO data, ULO ea);
extern void memoryIOWriteWord(ULO data, ULO ea);
extern void memoryIOWriteLong(ULO data, ULO ea);

/* EMEM INTERFACE */

extern ULO memoryEmemReadByteASM(ULO ea);
extern ULO memoryEmemReadWordASM(ULO ea);
extern ULO memoryEmemReadLongASM(ULO ea);
extern void memoryEmemWriteByteASM(ULO data, ULO ea);
extern void memoryEmemWriteWordASM(ULO data, ULO ea);
extern void memoryEmemWriteLongASM(ULO data, ULO ea);

/* NOT AVAILABLE INTERFACE */

extern ULO memoryNAReadByte(ULO ea);
extern ULO memoryNAReadWord(ULO ea);
extern ULO memoryNAReadLong(ULO ea);
extern void memoryNAWriteByte(ULO data, ULO ea);
extern void memoryNAWriteWord(ULO data, ULO ea);
extern void memoryNAWriteLong(ULO data, ULO ea);

/* Mystery (SCSI..?) INTERFACE */

extern ULO memoryMysteryReadByte(ULO ea);
extern ULO memoryMysteryReadWord(ULO ea);
extern ULO memoryMysteryReadLong(ULO ea);
extern void memoryMysteryWriteByte(ULO data, ULO ea);
extern void memoryMysteryWriteWord(ULO data, ULO ea);
extern void memoryMysteryWriteLong(ULO data, ULO ea);

/* DMEM INTERFACE */

extern ULO memoryDmemReadByte(ULO ea);
extern ULO memoryDmemReadWord(ULO ea);
extern ULO memoryDmemReadLong(ULO ea);
extern void memoryDmemWriteLongASM(ULO data, ULO ea);

/* IO-Register access functions */

extern ULO rserdatr(ULO address);
extern ULO rintenar(ULO address);
extern ULO rintreqr(ULO address);
extern ULO rdefault(ULO address);
//extern void wserdat(ULO data, ULO address);
//extern void wserper(ULO data, ULO address);
extern void wdmacon(ULO data, ULO address);
extern void wintena(ULO data, ULO address);
extern void wintreq(ULO data, ULO address);
extern void wadcon(ULO data, ULO address);
extern void wdefault(ULO data, ULO address);

extern void fmemstart(void);  /* Code alignment label */

/* End of assembly function declarations */

/* Functions declared in C */

/* IO Bank functions */

typedef ULO (*memoryIOReadFunc)(ULO address);
typedef void (*memoryIOWriteFunc)(ULO data, ULO address);

extern void memorySetIOReadStub(ULO index, memoryIOReadFunc ioreadfunction);
extern void memorySetIOWriteStub(ULO index, memoryIOWriteFunc iowritefunction);

/* Expansion card functions */

typedef void (*memoryEmemCardInitFunc)(void);
typedef void (*memoryEmemCardMapFunc)(ULO);

extern void memoryEmemClear(void);
extern void memoryEmemCardAdd(memoryEmemCardInitFunc cardinit,
			      memoryEmemCardMapFunc cardmap);
extern void memoryEmemSet(ULO index, ULO data);
extern void memoryEmemMirror(ULO emem_offset, UBY *src, ULO size);

/* Module management functions */

extern void memorySoftReset(void);
extern void memoryHardReset(void);
extern void memoryHardResetPost(void);
extern void memoryEmulationStart(void);
extern void memoryEmulationStop(void);
extern void memoryStartup(void);
extern void memoryShutdown(void);

extern void memoryColorTranslationInit(void);

/* Device memory functions */

extern void memoryDmemSetByte(UBY data);
extern void memoryDmemSetWord(UWO data);
extern void memoryDmemSetLong(ULO data);
extern void memoryDmemSetLongNoCounter(ULO data, ULO offset);
extern void memoryDmemSetString(STR *data);
extern void memoryDmemSetCounter(ULO val);
extern ULO memoryDmemGetCounter(void);
extern void memoryDmemClear(void);

/* Memory bank functions */

typedef ULO (*memoryReadFunc)(ULO address);
typedef void (*memoryWriteFunc)(ULO data, ULO address);

extern memoryReadFunc memory_bank_readbyte[65536];
extern memoryReadFunc memory_bank_readword[65536];
extern memoryReadFunc memory_bank_readlong[65536];
extern memoryWriteFunc memory_bank_writebyte[65536];
extern memoryWriteFunc memory_bank_writeword[65536];
extern memoryWriteFunc memory_bank_writelong[65536];
extern UBY *memory_bank_pointer[65536];
extern UBY *memory_bank_datapointer[65536];

extern void memoryBankSet(memoryReadFunc rb,
			  memoryReadFunc rw,
			  memoryReadFunc rl,
			  memoryWriteFunc wb,
			  memoryWriteFunc ww,
			  memoryWriteFunc wl,
			  UBY *basep,
			  ULO bank, 
			  ULO basebank,
			  BOOLE datadirect);
extern UBY *memoryAddressToPtr(ULO address);

/* Memory configuration properties */

extern BOOLE memorySetChipSize(ULO chipsize);
extern ULO memoryGetChipSize(void);
extern BOOLE memorySetFastSize(ULO fastsize);
extern ULO memoryGetFastSize(void);
extern void memorySetFastAllocatedSize(ULO fastallocatedsize);
extern ULO memoryGetFastAllocatedSize(void);
extern BOOLE memorySetBogoSize(ULO bogosize);
extern ULO memoryGetBogoSize(void);
extern BOOLE memorySetUseAutoconfig(BOOLE useautoconfig);
extern BOOLE memoryGetUseAutoconfig(void);
extern BOOLE memorySetAddress32Bit(BOOLE address32bit);
extern BOOLE memoryGetAddress32Bit(void);
extern BOOLE memorySetKickImage(STR *kickimage);
extern STR *memoryGetKickImage(void);
extern void memorySetKey(STR *key);
extern STR *memoryGetKey(void);
extern BOOLE memoryGetKickImageOK(void);

/* Derived from memory configuration */

extern ULO memoryGetKickImageBaseBank(void);
extern ULO memoryGetKickImageVersion(void);
extern ULO memoryInitialPC(void);
extern ULO memoryInitialSP(void);

/* Kickstart load error handling */

#define MEMORY_ROM_ERROR_SIZE           0
#define MEMORY_ROM_ERROR_AMIROM_VERSION 1
#define MEMORY_ROM_ERROR_AMIROM_READ    2
#define MEMORY_ROM_ERROR_KEYFILE        3
#define MEMORY_ROM_ERROR_EXISTS_NOT     4
#define MEMORY_ROM_ERROR_FILE           5
#define MEMORY_ROM_ERROR_KICKDISK_NOT   6
#define MEMORY_ROM_ERROR_CHECKSUM       7
#define MEMORY_ROM_ERROR_KICKDISK_SUPER 8
#define MEMORY_ROM_ERROR_BAD_BANK       9



/* Global variables */

extern ULO memory_wriorgadr;
extern UBY memory_chip[];
extern UBY *memory_fast;
extern UBY memory_bogo[];
extern UBY memory_kick[];
extern ULO memory_chipsize;
extern UBY memory_emem[];
//extern memoryIOReadFunc memory_iobank_write[];

extern ULO intenar,intena,intreq;
extern ULO potgor;

extern ULO adcon;

extern memoryIOWriteFunc memory_iobank_write[256];

#endif
