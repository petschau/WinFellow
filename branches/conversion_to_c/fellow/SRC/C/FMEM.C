/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Virtual Memory System                                                      */
/*                                                                            */
/* Authors: Petter Schau (peschau@online.no)                                  */
/*          Roman Dolejsi                                                     */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

#include "portable.h"
#include "renaming.h"

#include "defs.h"
#include "fellow.h"
#include "draw.h"
#include "cpu.h"
#include "fhfile.h"
#include "graph.h"
#include "floppy.h"
#include "copper.h"
#include "cia.h"
#include "blit.h"
#include "fmem.h"
#include "memorya.h"
#include "fswrap.h"
#include "wgui.h"


/*============================================================================*/
/* Holds configuration for memory                                             */
/*============================================================================*/

ULO memory_chipsize;
ULO memory_fastsize;
ULO memory_bogosize;
BOOLE memory_useautoconfig;
BOOLE memory_address32bit;
STR memory_kickimage[CFG_FILENAME_LENGTH];
STR memory_key[256];


/*============================================================================*/
/* Holds actual memory                                                        */
/*============================================================================*/

UBY memory_chip[0x200000 + 32];
UBY memory_bogo[0x1c0000 + 32];
UBY memory_kick[0x080000 + 32];
UBY *memory_fast = NULL;
ULO memory_fastallocatedsize;


/*============================================================================*/
/* Autoconfig data                                                            */
/*============================================================================*/

#define EMEM_MAX_CARDS 4
UBY memory_emem[0x10000];
memoryEmemCardInitFunc memory_emem_card_initfunc[EMEM_MAX_CARDS];
memoryEmemCardMapFunc memory_emem_card_mapfunc[EMEM_MAX_CARDS];
ULO memory_emem_card_count;                                /* Number of cards */
ULO memory_emem_cards_finished_count;                         /* Current card */


/*============================================================================*/
/* Device memory data                                                         */
/*============================================================================*/

#define MEMORY_DMEM_OFFSET 0xf40000

UBY memory_dmem[65536];
ULO memory_dmem_counter;


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

ULO memory_mystery_value;          /* Pattern needed in an unknown bank (hmm) */
ULO memory_wriorgadr;                  /* Stores original address for a write */
ULO memory_noise[2];                     /* Returns alternating bitpattern in */
ULO memory_noise_counter;    /* unused IO-registers to keep apps from hanging */
ULO memory_undefined_io_write_counter = 0;

void memoryLogUndefinedIOWrites(void) {
  STR s[80];
  sprintf(s, "Write: %.6X\n", memory_wriorgadr);
  fellowAddLog(s);
}

void memoryLogUndefinedIOReads(void) {
  STR s[80];
  sprintf(s, "Read : %.6X\n", memory_wriorgadr);
  fellowAddLog(s);
}


/*============================================================================*/
/* Table of register read/write functions                                     */
/*============================================================================*/

memoryIOReadFunc memory_iobank_read[256] = {
	rdefault_C,rdmaconr_C,rvposr_C,rvhposr_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rserdatr_C,rdefault_C,rintenar_C,rintreqr_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rid_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rcopjmp1_C,rcopjmp2_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C,
	rdefault_C,rdefault_C,rdefault_C,rdefault_C
};

memoryIOWriteFunc memory_iobank_write[256]={
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wvpos_C ,wdefault_C,
	wdefault_C,wserdat_C,wserper_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wbltcdat_C,wbltbdat_C,wbltadat_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wcop1lch,wcop1lcl,wcop2lch,wcop2lcl,
	wcopjmp1,wcopjmp2,wdefault_C,wdiwstrt_C,
	wdiwstop_C,wddfstrt_C,wddfstop_C,wdmacon_C,
	wdefault_C,wintena_C,wintreq_C,wadcon_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wbpl1pth_C,wbpl1ptl_C,wbpl2pth_C,wbpl2ptl_C,
	wbpl3pth_C,wbpl3ptl_C,wbpl4pth_C,wbpl4ptl_C,
	wbpl5pth_C,wbpl5ptl_C,wbpl6pth_C,wbpl6ptl_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wbplcon0_C,wbplcon1_C,wbplcon2_C,wdefault_C,
	wbpl1mod_C,wbpl2mod_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wcolor_C,wcolor_C,wcolor_C,wcolor_C,
	wcolor_C,wcolor_C,wcolor_C,wcolor_C,
	wcolor_C,wcolor_C,wcolor_C,wcolor_C,
	wcolor_C,wcolor_C,wcolor_C,wcolor_C,
	wcolor_C,wcolor_C,wcolor_C,wcolor_C,
	wcolor_C,wcolor_C,wcolor_C,wcolor_C,
	wcolor_C,wcolor_C,wcolor_C,wcolor_C,
	wcolor_C,wcolor_C,wcolor_C,wcolor_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C,
	wdefault_C,wdefault_C,wdefault_C,wdefault_C
};


/*============================================================================*/
/* Memory mapping tables                                                      */
/*============================================================================*/

memoryReadFunc memory_bank_readbyte[65536];
memoryReadFunc memory_bank_readword[65536];
memoryReadFunc memory_bank_readlong[65536];
memoryWriteFunc memory_bank_writebyte[65536];
memoryWriteFunc memory_bank_writeword[65536];
memoryWriteFunc memory_bank_writelong[65536];
UBY *memory_bank_pointer[65536];                   /* Used to get opcodewords */
UBY *memory_bank_datapointer[65536];                   /* Used to access data */


/* Variables that correspond to various registers */

ULO intenar, intena, intreq;


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

void memoryBankSet(memoryReadFunc rb, 
		   memoryReadFunc rw, 
		   memoryReadFunc rl, 
		   memoryWriteFunc wb,
		   memoryWriteFunc ww, 
		   memoryWriteFunc wl, 
		   UBY *basep, 
		   ULO bank,
		   ULO basebank, 
		   BOOLE datadirect) {
  ULO i, j;

  j = (memoryGetAddress32Bit()) ? 65536 : 256;
  for (i = bank; i < 65536; i += j) {
    memory_bank_readbyte[i] = rb;
    memory_bank_readword[i] = rw;
    memory_bank_readlong[i] = rl;
    memory_bank_writebyte[i] = wb;
    memory_bank_writeword[i] = ww;
    memory_bank_writelong[i] = wl;
    if (basep != NULL) {
      memory_bank_pointer[i] = basep - (basebank<<16);
      if (datadirect) memory_bank_datapointer[i] = memory_bank_pointer[i];
      else memory_bank_datapointer[i] = NULL;
    }
    else memory_bank_pointer[i] = memory_bank_datapointer[i] = NULL;
    basebank += j;
  }
}


/*============================================================================*/
/* Clear one bank data to safe "do-nothing" values                            */
/*============================================================================*/

void memoryBankClear(ULO bank) {
  memoryBankSet(memoryNAReadByte,
		memoryNAReadWord,
		memoryNAReadLong, 
		memoryNAWriteByte,
		memoryNAWriteWord, 
		memoryNAWriteLong, 
		NULL,
		bank,
		0,
		FALSE);
}


/*============================================================================*/
/* Clear all bank data to safe "do-nothing" values                            */
/*============================================================================*/

void memoryBankClearAll(void) {
  ULO bank, hilim;

  hilim = (memoryGetAddress32Bit()) ? 65536 : 256;
  for (bank = 0; bank < hilim; bank++)
    memoryBankClear(bank);
}


/*============================================================================*/
/* Expansion cards autoconfig                                                 */
/*============================================================================*/


/*============================================================================*/
/* Clear the expansion config bank                                            */
/*============================================================================*/

void memoryEmemClear(void) {
  memset(memory_emem, 0xff, 65536);
}


/*============================================================================*/
/* Add card to table                                                          */
/*============================================================================*/

void memoryEmemCardAdd(memoryEmemCardInitFunc cardinit,
		       memoryEmemCardMapFunc cardmap) {
  if (memory_emem_card_count < EMEM_MAX_CARDS) {
    memory_emem_card_initfunc[memory_emem_card_count] = cardinit;
    memory_emem_card_mapfunc[memory_emem_card_count] = cardmap;
    memory_emem_card_count++;
  }
}


/*============================================================================*/
/* Advance the card pointer                                                   */
/*============================================================================*/

void memoryEmemCardNext(void) {
  memory_emem_cards_finished_count++;
}


/*============================================================================*/
/* Init the current card                                                      */
/*============================================================================*/

void memoryEmemCardInit(void) {
  memoryEmemClear();
  if (memory_emem_cards_finished_count != memory_emem_card_count)
    memory_emem_card_initfunc[memory_emem_cards_finished_count]();
} 


/*============================================================================*/
/* Map this card                                                              */
/* Mapping is bank number set by AmigaOS                                      */
/*============================================================================*/

void memoryEmemCardMap(ULO mapping) {
  if (memory_emem_cards_finished_count == memory_emem_card_count)
    memoryEmemClear();
  else memory_emem_card_mapfunc[memory_emem_cards_finished_count](mapping);
}


/*============================================================================*/
/* Reset card setup                                                           */
/*============================================================================*/

void memoryEmemCardsReset(void) {
  memory_emem_cards_finished_count = 0;
  memoryEmemCardInit();
}


/*============================================================================*/
/* Clear the card table                                                       */
/*============================================================================*/

void memoryEmemCardsRemove(void) {
  memory_emem_card_count = memory_emem_cards_finished_count = 0;
}


/*============================================================================*/
/* Set a byte in autoconfig space, for initfunc routines                      */
/* so they can make their configuration visible                               */
/*============================================================================*/

void memoryEmemSet(ULO index, ULO value) {
  index &= 0xffff;
  switch (index) {
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

void memoryEmemMirror(ULO emem_offset, UBY *src, ULO size) {
  memcpy(memory_emem + emem_offset, src, size);
}


/*============================================================================*/
/* Read/Write stubs for autoconfig memory                                     */
/*============================================================================*/

ULO memoryEmemReadByteC(ULO address) {
  return memory_emem[address & 0xffff];
}

ULO memoryEmemReadWordC(ULO address) {
  return (memory_emem[address & 0xffff]<<8) |
         memory_emem[(address & 0xffff) + 1];
}

ULO memoryEmemReadLongC(ULO address) {
  return (memoryEmemReadWordC(address)<<16) | memoryEmemReadWordC(address + 2);
}

void memoryEmemWriteByteC(ULO data, ULO address) {
  static ULO mapping;

  data &= 0xff;
  switch (memory_wriorgadr & 0xffff) {
    case 0x30:
    case 0x32:
      mapping = data = 0;
    case 0x48:
      mapping = (mapping & 0xff) | ((data & 0xff)<<8);
      memoryEmemCardMap(mapping);
      memoryEmemCardNext();
      memoryEmemCardInit();
      break;
    case 0x4a:
      mapping = (mapping & 0xff00) | (data & 0xff);
      break;
    case 0x4c:
      memoryEmemCardNext();
      memoryEmemCardInit();
      break;
    }
}

void memoryEmemWriteWordC(ULO data, ULO address) {
}

void memoryEmemWriteLongC(ULO data, ULO address) {
}


/*===========================================================================*/
/* Map the autoconfig memory bank into memory                                */
/*===========================================================================*/

void memoryEmemMap(void) {
  if (memoryGetKickImageBaseBank() >= 0xf8)
    memoryBankSet(memoryEmemReadByteASM,
		  memoryEmemReadWordASM,
		  memoryEmemReadLongASM,
		  memoryEmemWriteByteASM,
		  memoryEmemWriteWordASM,
		  memoryEmemWriteLongASM,
		  NULL,
		  0xe8,
		  0xe8,
		  FALSE);
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

void memoryDmemClear(void) {
  memset(memory_dmem, 0, 4096);
}

void memoryDmemSetCounter(ULO c) {
  memory_dmem_counter = c;
}

ULO memoryDmemGetCounter(void) {
  return memory_dmem_counter + MEMORY_DMEM_OFFSET;
}

void memoryDmemSetString(STR *st) {
  strcpy((STR *) (memory_dmem + memory_dmem_counter), st);
  memory_dmem_counter += strlen(st) + 1;
  if (memory_dmem_counter & 1) memory_dmem_counter++;
}

void memoryDmemSetByte(UBY data) {
  memory_dmem[memory_dmem_counter++] = data;
}

void memoryDmemSetWord(UWO data) {
  memory_dmem[memory_dmem_counter++] = (data & 0xff00)>>8;
  memory_dmem[memory_dmem_counter++] = data & 0xff;
}

void memoryDmemSetLong(ULO data) {
  memory_dmem[memory_dmem_counter++] = (UBY) ((data & 0xff000000)>>24);
  memory_dmem[memory_dmem_counter++] = (UBY) ((data & 0xff0000)>>16);
  memory_dmem[memory_dmem_counter++] = (UBY) ((data & 0xff00)>>8);
  memory_dmem[memory_dmem_counter++] = (UBY) (data & 0xff);
}

void memoryDmemSetLongNoCounter(ULO data, ULO offset) {
  memory_dmem[offset] = (UBY) ((data & 0xff000000)>>24);
  memory_dmem[offset + 1] = (UBY) ((data & 0xff0000)>>16);
  memory_dmem[offset + 2] = (UBY) ((data & 0xff00)>>8);
  memory_dmem[offset + 3] = (UBY) (data & 0xff);
}


/*============================================================================*/
/* Writing a long to $f40000 runs a native function                           */
/*============================================================================*/

void memoryDmemWriteLongC(ULO data, ULO address) {
  if ((memory_wriorgadr & 0xffffff) == 0xf40000) {
    switch (data>>16) {
      case 0x0001:    fhfileDo(data);
                      break;
      default:        break;
      }
    }
}

void memoryDmemMap(void) {
  ULO bank = 0xf40000>>16;

  if (memory_useautoconfig && (memory_kickimage_basebank >= 0xf8))
    memoryBankSet(memoryDmemReadByte,
		  memoryDmemReadWord,
		  memoryDmemReadLong,
		  memoryNAWriteByte,
		  memoryNAWriteWord,
		  memoryDmemWriteLongASM,
		  memory_dmem,
		  bank,
		  bank,
		  FALSE);
}


/*============================================================================*/
/* Converts an address to a direct pointer to memory. Used by hardfile device */
/*============================================================================*/

UBY *memoryAddressToPtr(ULO address) {
  UBY *result;

  if ((result = memory_bank_pointer[address>>16]) != NULL)
    result += address;
  return result;
}


/*============================================================================*/
/* Chip memory handling                                                       */
/*============================================================================*/



void memoryChipClear(void) {
  memset(memory_chip, 0, memoryGetChipSize());
}

void memoryChipMap(BOOLE overlay) {
  ULO bank, lastbank;

  if (overlay)
  {
    for (bank = 0;
	 bank < 8;
	 bank++)
      memoryBankSet(memoryOVLReadByte,
		    memoryOVLReadWord,
		    memoryOVLReadLong,
		    memoryOVLWriteByte,
		    memoryOVLWriteWord,
		    memoryOVLWriteLong,
		    memory_kick,
		    bank,
		    0, 
		    FALSE);
  }

  if (memoryGetChipSize() > 0x200000) lastbank = 0x200000>>16;
  else lastbank = memoryGetChipSize()>>16;

  for (bank = (overlay) ? 8 : 0; bank < lastbank; bank++)
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


/*============================================================================*/
/* Fast memory handling                                                       */
/*============================================================================*/

/*============================================================================*/
/* Set up autoconfig values for fastmem card                                  */
/*============================================================================*/

void memoryFastCardInit(void) {
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

void memoryFastClear(void) {
  if (memory_fast != NULL)
    memset(memory_fast, 0, memoryGetFastSize());
}

void memoryFastFree(void) {
  if (memory_fast != NULL) {
    free(memory_fast);
    memory_fast = NULL;
    memorySetFastAllocatedSize(0);
  }
}

void memoryFastAllocate(void) {
  if (memoryGetFastSize() != memoryGetFastAllocatedSize()) {
    memoryFastFree();
    memory_fast = (UBY *) malloc(memoryGetFastSize());
    if (memory_fast == NULL) memorySetFastSize(0);
    else memoryFastClear();
    memorySetFastAllocatedSize((memory_fast == NULL) ? 0 : memoryGetFastSize());
  }
}


/*============================================================================*/
/* Map fastcard, should really map it to the address amigados tells us        */
/* but assume it is 0x200000 for now                                          */
/*============================================================================*/

void memoryFastCardMap(ULO mapping) {
  ULO bank, lastbank;

  if (memoryGetFastSize() > 0x800000) lastbank = 0xa00000>>16;
  else lastbank = (0x200000 + memoryGetFastSize())>>16;
  for (bank = 0x200000>>16; bank < lastbank; bank++)
    memoryBankSet(memoryFastReadByte,
		  memoryFastReadWord,
		  memoryFastReadLong,
		  memoryFastWriteByte,
		  memoryFastWriteWord, 
		  memoryFastWriteLong,
		  memory_fast, 
		  bank, 
		  0x200000>>16, 
		  TRUE);
  memset(memory_fast, 0, memoryGetFastSize());
}

void memoryFastCardAdd(void) {
  if (memoryGetFastSize() != 0)
    memoryEmemCardAdd(memoryFastCardInit, memoryFastCardMap);
}


/*============================================================================*/
/* Bogo memory handling                                                       */
/*============================================================================*/

void memoryBogoClear(void) {
  memset(memory_bogo, 0, memoryGetBogoSize());
}

void memoryBogoMap(void) {
  ULO bank, lastbank;
  
  if (memoryGetBogoSize() > 0x1c0000) lastbank = 0xdc0000>>16;
  else lastbank = (0xc00000 + memoryGetBogoSize())>>16;
  for (bank = 0xc00000>>16; bank < lastbank; bank++)
    memoryBankSet(memoryBogoReadByte,
		  memoryBogoReadWord,
		  memoryBogoReadLong,
		  memoryBogoWriteByte,
		  memoryBogoWriteWord,
		  memoryBogoWriteLong,
		  memory_bogo,
		  bank, 
		  0xc00000>>16, 
		  TRUE);
}


/*============================================================================*/
/* Probably a disk controller, we need it to pass dummy values to get past    */
/* the bootstrap of some Kickstart versions.                                  */
/*============================================================================*/

void memoryMysteryMap(void) {
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

void memoryIOMap(void) {
  ULO bank, lastbank;

  if (memoryGetBogoSize() > 0x1c0000) lastbank = 0xdc0000>>16;
  else lastbank = (0xc00000 + memoryGetBogoSize())>>16;
  for (bank = lastbank; bank < 0xe00000>>16; bank++)
    memoryBankSet(memoryIOReadByte,
		  memoryIOReadWord, 
		  memoryIOReadLong,
		  memoryIOWriteByte, 
		  memoryIOWriteWord, 
		  memoryIOWriteLong,
		  NULL, 
		  bank, 
		  0, 
		  FALSE);
}


/*===========================================================================*/
/* Initializes one entry in the IO register access table                     */
/*===========================================================================*/

void memorySetIOReadStub(ULO index, memoryIOReadFunc ioreadfunction) {
  memory_iobank_read[index>>1] = ioreadfunction;
}

void memorySetIOWriteStub(ULO index, memoryIOWriteFunc iowritefunction) {
  memory_iobank_write[index>>1] = iowritefunction;
}


/*===========================================================================*/
/* Clear all IO-register accessors                                           */
/*===========================================================================*/

void memoryIOClear(void) {
  ULO i;

  for (i = 0; i < 512; i += 2) {
    memorySetIOReadStub(i, rdefault_C);
    memorySetIOWriteStub(i, wdefault_C);
  }
}


/*============================================================================*/
/* Kickstart handling                                                         */
/*============================================================================*/

/*============================================================================*/
/* Map the Kickstart image into Amiga memory                                  */
/*============================================================================*/

void memoryKickMap(void) {
  ULO bank, basebank;

  basebank = memory_kickimage_basebank & 0xf8;
  for (bank = basebank;
       bank < (basebank + 8);
       bank++)
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
}


/*============================================================================*/
/* An error occured during loading a kickstart file. Uses GUI to display      */
/* an errorstring.                                                            */
/*============================================================================*/

void memoryKickError(ULO errorcode, ULO data) {
  static STR error1[80], error2[160], error3[160];
  
  sprintf(error1, "Kickstart file could not be loaded");
  sprintf(error2, "%s", memory_kickimage);
  error3[0] = '\0';
  switch (errorcode) {
    case MEMORY_ROM_ERROR_SIZE:
      sprintf(error3,
	      "Illegal size: %d bytes, size must be either 256K or 512K",
	      data);
      break;
    case MEMORY_ROM_ERROR_AMIROM_VERSION:
      sprintf(error3, "Unsupported encryption method, version found was %d",
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
  wguiRequester(error1, error2, error3);
  memoryKickSettingsClear();
}


/*============================================================================*/
/* Returns the checksum of the current kickstart image.                       */
/*============================================================================*/

ULO memoryKickChksum(void) {
  ULO sum, lastsum, i;

  sum = lastsum = 0;
  for (i = 0; i < 0x80000; i += 4) {
    sum += ((((ULO) memory_kick[i]) << 24) |
            (((ULO) memory_kick[i + 1]) << 16) |
            (((ULO) memory_kick[i + 2]) << 8) |
            ((ULO) memory_kick[i + 3]));
    if (sum < lastsum) sum++;
    lastsum = sum;
  }
  return ~sum;
}


/*============================================================================*/
/* Identifies a loaded Kickstart                                              */
/*============================================================================*/

STR *memoryKickIdentify(STR *s) {
  ULO i = 0;
  UBY *rom = memory_kick;
  ULO ver, rev;
  
  ver = (rom[12] << 8) | rom[13];
  rev = (rom[14] << 8) | rom[15];
  if (ver == 65535) memory_kickimage_version = 28;
  else if (ver < 29) memory_kickimage_version = 29;
  else if (ver > 41) memory_kickimage_version = 41;
  else memory_kickimage_version = ver;
  sprintf(s, 
	  "%s (%d.%d)",
	  memory_kickimage_versionstrings[memory_kickimage_version - 28],
	  ver,
	  rev);
  return s;
}


/*============================================================================*/
/* Verifies that a loaded Kickstart is OK                                     */
/*============================================================================*/

void memoryKickOK(void) {
  ULO chksum, basebank;
  
  if ((chksum = memoryKickChksum()) != 0)
    memoryKickError(MEMORY_ROM_ERROR_CHECKSUM, chksum);
  else {
    basebank = memory_kick[5];
    if ((basebank == 0xf8) || (basebank == 0xfc)) {
      memory_kickimage_basebank = basebank;
      memory_kickimage_none = FALSE;
      memoryKickIdentify(memory_kickimage_versionstr);
      memory_initial_PC = ((((ULO) memory_kick[4]) << 24) |
			   (((ULO) memory_kick[4 + 1]) << 16) |
			   (((ULO) memory_kick[4 + 2]) << 8) |
			   ((ULO) memory_kick[4 + 3]));
      memory_initial_SP = ((((ULO) memory_kick[0]) << 24) |
			   (((ULO) memory_kick[0 + 1]) << 16) |
			   (((ULO) memory_kick[0 + 2]) << 8) |
			   ((ULO) memory_kick[0 + 3]));
    }
    else
      memoryKickError(MEMORY_ROM_ERROR_BAD_BANK, basebank);
  }
}


/*============================================================================*/
/* Returns size of decoded kickstart                                          */
/*============================================================================*/
      
int memoryKickDecodeAF(STR *filename, STR *keyfile) {
  STR *keybuffer;
  ULO keysize, filesize = 0, keypos = 0, c;
  FILE *KF, *RF;

  /* Read key */
  
  if ((KF = fopen(keyfile, "rb")) != NULL) {
    fseek(KF, 0, SEEK_END);
    keysize = ftell(KF);
    keybuffer = malloc(keysize);
    fseek(KF, 0, SEEK_SET);
    fread(keybuffer, 1, keysize, KF);
    fclose(KF);
  }
  else
    return -1;
  if (!keybuffer)
    return -1;  

  /* Read file */

  if ((RF = fopen(filename, "rb")) != NULL) {
    fseek(RF, 11, SEEK_SET);
    while (((c = fgetc(RF)) != EOF) && filesize < 524288) {
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

int memoryKickLoadAF2(FILE *F) {
  ULO version;
  STR IDString[12];
  FILE *keyfile;
  
  fread(IDString, 11, 1, F);
  version = IDString[10] - '0';
  IDString[10] = '\0';
  if (stricmp(IDString, "AMIROMTYPE") == 0) { /* Header seems OK */
    if (version != 1) {
      memoryKickError(MEMORY_ROM_ERROR_AMIROM_VERSION, version);
      return TRUE;  /* File was handled */
    }
    else { /* Seems to be a file we can handle */
      ULO size;

      fclose(F);

      /* Test if keyfile exists */

      if ((keyfile = fopen(memory_key, "rb")) == NULL) {
	memoryKickError(MEMORY_ROM_ERROR_KEYFILE, 0);
	return TRUE;
      }
      else {
	fclose(keyfile);
	size = memoryKickDecodeAF(memory_kickimage,
				  memory_key);
	if (size == -1) {
	  memoryKickError(MEMORY_ROM_ERROR_AMIROM_READ, 0);
	  return TRUE;
	}
	if (size != 262144 && size != 524288) {
	  memoryKickError(MEMORY_ROM_ERROR_SIZE, size);
	  return TRUE;
	}
	if (size == 262144)
	  memcpy(memory_kick + 262144, memory_kick, 262144);
	memory_kickimage_none = FALSE;
	memoryKickIdentify(memory_kickimage_versionstr);
	return TRUE;
      }
    }
  }
  /* Here, header was not recognized */
  return FALSE;
}


/*============================================================================*/
/* Detect and load kickdisk                                                   */
/* Based on information provided by Jerry Lawrence                            */
/*============================================================================*/

void memoryKickDiskLoad(FILE *F) {
  STR head[5];
 
  /* Check header */

  fseek(F, 0, SEEK_SET);
  fread(head, 4, 1, F);
  head[4] = '\0';
  if (strcmp(head, "KICK") != 0) {
    memoryKickError(MEMORY_ROM_ERROR_KICKDISK_NOT, 0);
    return;
  }
  fread(head, 3, 1, F);
  head[3] = '\0';
  if (strcmp(head, "SUP") == 0) {
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

void memoryKickLoad(void) {
  FILE *F;
  BOOLE kickdisk = FALSE;
  STR *suffix, *lastsuffix;
  BOOLE afkick = FALSE;
  
  /* New file is different from previous */
  /* Must load file */

  fs_navig_point *fsnp;

  memory_kickimage_none = FALSE;/* Initially Kickstart is expected to be OK */
  if ((fsnp = fsWrapMakePoint(memory_kickimage)) == NULL)
    memoryKickError(MEMORY_ROM_ERROR_EXISTS_NOT, 0);
  else {
    if (fsnp->type != FS_NAVIG_FILE)
      memoryKickError(MEMORY_ROM_ERROR_FILE, 0);
    else { /* File passed initial tests */
      if ((F = fopen(memory_kickimage, "rb")) == NULL)
	memoryKickError(MEMORY_ROM_ERROR_EXISTS_NOT, 0);
      else memory_kickimage_size = fsnp->size;
    }
    free(fsnp);
  }

  /* Either the file is open, or memory_kickimage_none is TRUE */ 

  if (!memory_kickimage_none) {

    /* File opened successfully */

    /* Kickdisk flag */

    suffix = strchr(memory_kickimage, '.');
    if (suffix != NULL) {
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
      afkick = memoryKickLoadAF2(F);
    if (!kickdisk && !afkick) { /* Normal kickstart image */
      fseek(F, 0, SEEK_SET);
      if (memory_kickimage_size == 262144) {   /* Load 256k ROM */
        fread(memory_kick, 1, 262144, F);
        memcpy(memory_kick + 262144, memory_kick, 262144);
      }
      else if (memory_kickimage_size == 524288)/* Load 512k ROM */
        fread(memory_kick, 1, 524288, F);
      else {                                     /* Rom size is wrong */
        memoryKickError(MEMORY_ROM_ERROR_SIZE, memory_kickimage_size);
      }
      fclose(F);
    }
  }
  if (!memory_kickimage_none)
    memoryKickOK();
}


/*============================================================================*/
/* Clear memory used for kickstart image                                      */
/*============================================================================*/

void memoryKickClear(void) {
  memset(memory_kick, 0, 0x80000);
}


/*============================================================================*/
/* Memory configuration interface                                             */
/*============================================================================*/

BOOLE memorySetChipSize(ULO chipsize) {
  BOOLE needreset = (memory_chipsize != chipsize);
  memory_chipsize = chipsize;
  return needreset;
}

ULO memoryGetChipSize(void) {
  return memory_chipsize;
}

BOOLE memorySetFastSize(ULO fastsize) {
  BOOLE needreset = (memory_fastsize != fastsize);
  memory_fastsize = fastsize;
  if (needreset) memoryFastAllocate();
  return needreset;
}

ULO memoryGetFastSize(void) {
  return memory_fastsize;
}

void memorySetFastAllocatedSize(ULO fastallocatedsize) {
  memory_fastallocatedsize = fastallocatedsize;
}

ULO memoryGetFastAllocatedSize(void) {
  return memory_fastallocatedsize;
}

BOOLE memorySetBogoSize(ULO bogosize) {
  BOOLE needreset = (memory_bogosize != bogosize);
  memory_bogosize = bogosize;
  return needreset;
}

ULO memoryGetBogoSize(void) {
  return memory_bogosize;
}

BOOLE memorySetUseAutoconfig(BOOLE useautoconfig) {
  BOOLE needreset = memory_useautoconfig != useautoconfig;
  memory_useautoconfig = useautoconfig;
  return needreset;
}

BOOLE memoryGetUseAutoconfig(void) {
  return memory_useautoconfig;
}

BOOLE memorySetAddress32Bit(BOOLE address32bit) {
  BOOLE needreset = memory_address32bit != address32bit;
  memory_address32bit = address32bit;
  return needreset;
}

BOOLE memoryGetAddress32Bit(void) {
  return memory_address32bit;
}

BOOLE memorySetKickImage(STR *kickimage) {
  BOOLE needreset = !!strncmp(memory_kickimage, kickimage, CFG_FILENAME_LENGTH);
  strncpy(memory_kickimage, kickimage, CFG_FILENAME_LENGTH);
  if (needreset) memoryKickLoad();
  return needreset;
}

STR *memoryGetKickImage(void) {
  return memory_kickimage;
}

void memorySetKey(STR *key) {
  strncpy(memory_key, key, CFG_FILENAME_LENGTH);
}

STR *memoryGetKey(void) {
  return memory_key;
}

ULO memoryGetKickImageBaseBank(void) {
  return memory_kickimage_basebank;
}

ULO memoryGetKickImageVersion(void) {
  return memory_kickimage_version;
}

BOOLE memoryGetKickImageOK(void) {
  return !memory_kickimage_none;
}

ULO memoryInitialPC(void) {
  return memory_initial_PC;
}

ULO memoryInitialSP(void) {
  return memory_initial_SP;
}


/*============================================================================*/
/* Sets all settings a clean state                                            */
/*============================================================================*/

void memoryChipSettingsClear(void) {
  memorySetChipSize(0x200000);
  memoryChipClear();
}

void memoryFastSettingsClear(void) {
  memory_fast = NULL;
  memorySetFastAllocatedSize(0);
  memorySetFastSize(0);
}

void memoryBogoSettingsClear(void) {
  memorySetBogoSize(0x1c0000);
  memoryBogoClear();
}

void memoryIOSettingsClear(void) {
  memoryIOClear();
}

void memoryKickSettingsClear(void) {
  memory_kickimage[0] = '\0';
  memory_kickimage_none = TRUE;
  memoryKickClear();
}

void memoryEmemSettingsClear(void) {
  memoryEmemCardsRemove();
  memoryEmemClear();
}

void memoryDmemSettingsClear(void) {
  memoryDmemSetCounter(0);
  memoryDmemClear();
}

void memoryNoiseSettingsClear(void) {
  memory_noise[0] = 0;
  memory_noise[1] = 0xffffffff;
  memory_noise_counter = 0;
}

void memoryMysterySettingsClear(void) {
  memory_mystery_value = 0xff00ff00;
}

void memoryBankSettingsClear(void) {
  memoryBankClearAll();
}

void memoryIOHandlersInstall(void) {
  memorySetIOReadStub(0x018, rserdatr_C);
  memorySetIOReadStub(0x01c, rintenar_C);
  memorySetIOReadStub(0x01e, rintreqr_C);
  memorySetIOWriteStub(0x09a, wintena_C);
  memorySetIOWriteStub(0x09c, wintreq_C);
}


/*==============*/
/* Generic init */
/*==============*/
/*
void memoryEmulationStartOld(void) {
  memoryBankClearAll();
  memoryIOClear();
  memoryChipMap();
  memoryBogoMap();
  memoryIOMap();
  memoryEmemMap();
  memoryDmemMap();
  memoryMysteryMap();
  memoryKickMap();
  memoryIOHandlersInstall();
}
*/
void memoryEmulationStart(void) {
  memoryIOClear();
  memoryIOHandlersInstall();
}

void memoryEmulationStop(void) {
}

void memorySoftReset(void) {
  memoryDmemClear();
  memoryEmemClear();
  memoryEmemCardsRemove();
  memoryFastCardAdd();
  intreq = intena = intenar = 0;
  memoryBankClearAll();
  memoryChipMap(TRUE);
  memoryBogoMap();
  memoryIOMap();
  memoryEmemMap();
  memoryDmemMap();
  memoryMysteryMap();
  memoryKickMap();
}

void memoryHardReset(void) {
  memoryChipClear(),
  memoryFastClear();
  memoryBogoClear();
  memoryDmemClear();
  memoryEmemClear();
  memoryEmemCardsRemove();
  memoryFastCardAdd();
  intreq = intena = intenar = 0;
  memoryBankClearAll();
  memoryChipMap(TRUE);
  memoryBogoMap();
  memoryIOMap();
  memoryEmemMap();
  memoryDmemMap();
  memoryMysteryMap();
  memoryKickMap();
}

void memoryHardResetPost(void) {
  memoryEmemCardInit();
}

void memoryStartup(void) {
  memorySetAddress32Bit(TRUE);
  memoryBankSettingsClear();
  memorySetAddress32Bit(FALSE);
  memoryChipSettingsClear();
  memoryFastSettingsClear();
  memoryBogoSettingsClear();
  memoryIOSettingsClear();
  memoryKickSettingsClear();
  memoryEmemSettingsClear();
  memoryDmemSettingsClear();
  memoryNoiseSettingsClear();
  memoryMysterySettingsClear(); /* ;-) */
}

void memoryShutdown(void) {
  memoryFastFree();
}



/*----------------------------
/* Chip read register functions	
/*---------------------------- 

*/
ULO rintreqr_C(ULO address)
{
	return intreq;
}

/* SERDATR
/* $dff018 */

ULO rserdatr_C(ULO address)
{
	return 0;
}

/*; INTENAR
; $dff01c*/

ULO rintenar_C(ULO address)
{
	return intenar;
}

// To simulate noise, return 0 and -1 every second time.
// Why? Bugged demos test write-only registers for various bit-values
// and to break out of loops, both 0 and 1 values must be returned.

ULO rdefault_C(ULO address)
{
	//memory_wriorgadr = address;
	//memoryLogUndefinedIOReads();
	memory_noise_counter++;
	return memory_noise[memory_noise_counter & 0x1];
}

void wdefault_C(ULO data, ULO address)
{
	memory_undefined_io_write_counter++;
	//memory_wriorgadr = address;
	//memoryLogUndefinedIOWrites();
}

/*========
; ADCON
;========

; $dff09e  - Leses fra $dff010
*/

void wadcon_C(ULO data, ULO address)
{
	if ((address & 0x8000) == 0x8000)
	{
		adcon = (~(data & 0x7fff)) & adcon; 
	}
	else
	{
		adcon = (data & 0x7fff) | adcon;
	}
}

/*========
; INTREQ
;========*/

void wintreq_C(ULO data, ULO address)
{
	if ((data & 0x8000) == 0x0000)
	{
		intreq = (~(data & 0x7fff)) & intreq; 
	}
	else
	{
		intreq = (data & 0x7fff) | intreq;
		__asm 
		{
			pushad
			call cpuRaiseInterrupt
			popad
		}
	}
}

//$dff09a  - Leses fra $dff01c
// If master bit is off, then INTENA is 0, else INTENA = INTENAR
// The master bit can not be read, the memory test in the kickstart
// depends on this.

void wintena_C(ULO data, ULO address)
{
	if ((data & 0x8000) == 0x0000)
	{
		intenar = (~(data & 0x7fff)) & intenar; 
	}
	else
	{
		intenar = (data & 0x7fff) | intenar;
	}

	if ((intenar & 0x00004000) == 0x00004000)
	{
		intena = intenar;
		__asm 
		{
			pushad
			call cpuRaiseInterrupt
			popad
		}
	}
	else
	{
		intena = 0;
	}
/*
		mov	ecx, dword [intenar]	   ; Les inn status
		test	edx, 08000h
		jz	wine1
		and	edx, 7fffh		 ; Sett bits
		or	ecx, edx
		mov	dword [intenar], ecx
		test	ecx, 00004000h
		jnz	wintenanorm
		xor	ecx, ecx
		mov	dword [intena], ecx
		ret
wintenanorm:	mov	dword [intena], ecx
		call	_cpuRaiseInterrupt_
		ret
wine1:		and	edx, 7fffh		 ; Slett bits
		not	edx
		and	ecx, edx			  ; Slett bits
		mov	dword [intenar], ecx
		test	ecx, 00004000h
		jnz	wintenanorm
		xor	ecx, ecx
		mov	dword [intena], ecx
		ret
		*/
}

ULO fetb_C(ULO address)
{
	return (memory_bank_readbyte[(address >> 16)*4](address));
}

ULO fetw_C(ULO address)
{
	if ((address & 0x1) == 0x1)
	{

	}
	return (memory_bank_readword[(address >> 16)*4](address));
}

/*==============================================================================
; These raises exception 3 when reading an odd address
; and the CPU is < 020
;==============================================================================*/

void memoryOddRead(ULO address, ULO pcPtr)
{
	if ((address & 0x1) == 0x1)
	{
		if (cpu_major < 2)
		{
			memory_fault_read = TRUE;
			memory_fault_address = address;
			cpuPrepareExceptionC(0x0c, pcPtr);
		}
	}
}


/*=============================
; Common memory access macros
;=============================*/

UBY memoryFetchB(ULO address)
{
	UBY* dataPointer;

	dataPointer = memory_bank_datapointer[(address >> 16)*4];
	if (dataPointer == NULL)
	{
		return (UBY) memory_bank_readbyte[(address >> 16)];
	}
	else
	{
		return *(dataPointer + address);
	}
}

		
/*

global oddwrite
oddwrite:	cmp	dword [cpu_major], 2
		jl	.ex3
		ret
.ex3:		mov	dword [memory_fault_read] , 0
		mov	ecx, dword [memory_wriorgadr]
		mov	dword [memory_fault_address], ecx
		mov	ebx, 0ch
		mov	eax, 44
		jmp	_cpuPrepareException_
*/

ULO memoryNAReadByteC(ULO address)
{
	memory_mystery_value = !memory_mystery_value;
	return memory_mystery_value;
}

ULO memoryDmemReadByteC(ULO address)
{
	return memory_dmem[address & 0xffff];
}

ULO memoryOVLReadByteC(ULO address)
{
	return memory_kick[address & 0xffffff];
}

ULO memoryChipReadByteC(ULO address)
{
	return memory_chip[address & 0xffffff];
}

ULO memoryFastReadByteC(ULO address)
{
	return memory_fast[(address & 0xffffff) - 0x200000];
}

ULO memoryBogoReadByteC(ULO address)
{
	return memory_bogo[(address & 0xffffff) - 0xc00000];
}

ULO memoryMysteryReadByteC(ULO address)
{
	memory_mystery_value = !memory_mystery_value;
	return memory_mystery_value;
}

ULO memoryIOReadByteC(ULO address)
{
	if (((address & 0x1fe) & 0x1) == 0x0)
	{
		return ((memory_iobank_read[(address & 0x1fe) << 1](address & 0x1fe)) >> 8) & 0xff;
	}
	else
	{
		return memory_iobank_read[(address & 0x1fe) << 1](address & 0x1fe) & 0xff;
	}
}

ULO memoryKickReadByteC(ULO address)
{
	return memory_kick[(address & 0xffffff) - 0xf80000];
}