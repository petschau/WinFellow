 /* 
  * UAE - The Un*x Amiga Emulator
  *
  * AutoConfig (tm) Expansions (ZorroII/III)
  *
  * Copyright 1996,1997 Stefan Reinauer <stepan@linux.de>
  * Copyright 1997 Brian King <Brian_King@Mitel.com>
  *   - added gfxcard code
  *
  */


/* Only code needed to run the filesystem driver is retained in this file */
/* Removed is code relating to different autoconfig devices.              */


/* FELLOW OUT (START)--------------------
#include "sysconfig.h"
#include "sysdeps.h"

#include "config.h"
#include "options.h"
#include "uae.h"
#include "memory.h"
#include "autoconf.h"
#include "picasso96.h"
   FELLOW OUT (END)---------------------*/

/* FELLOW IN (START)----------------------*/

#include <stdio.h>
#include "uae2fell.h"
#include "fmem.h"
#include "autoconf.h"

/* FELLOW IN (END)-----------------------*/

#define MAX_EXPANSION_BOARDS	8

/* ********************************************************** */
/* 00 / 02 */
/* er_Type */

#define Z2_MEM_8MB	0x00 /* Size of Memory Block */
#define Z2_MEM_4MB	0x07
#define Z2_MEM_2MB	0x06
#define Z2_MEM_1MB	0x05
#define Z2_MEM_512KB	0x04
#define Z2_MEM_256KB	0x03
#define Z2_MEM_128KB	0x02
#define Z2_MEM_64KB	0x01
/* extended definitions */
#define Z2_MEM_16MB	0x00
#define Z2_MEM_32MB	0x01
#define Z2_MEM_64MB	0x02
#define Z2_MEM_128MB	0x03
#define Z2_MEM_256MB	0x04
#define Z2_MEM_512MB	0x05
#define Z2_MEM_1GB	0x06

#define chainedconfig	0x08 /* Next config is part of the same card */
#define rom_card	0x10 /* ROM vector is valid */
#define add_memory	0x20 /* Link RAM into free memory list */

#define zorroII		0xc0 /* Type of Expansion Card */
#define zorroIII	0x80

/* ********************************************************** */
/* 04 - 06 & 10-16 */

/* Manufacturer */
#define commodore_g	 513 /* Commodore Braunschweig (Germany) */
#define commodore	 514 /* Commodore West Chester */
#define gvp		2017 /* GVP */
#define ass		2102 /* Advanced Systems & Software */
#define hackers_id	2011 /* Special ID for test cards */

/* Card Type */
#define commodore_a2091	     3 /* A2091 / A590 Card from C= */
#define commodore_a2091_ram 10 /* A2091 / A590 Ram on HD-Card */
#define commodore_a2232	    70 /* A2232 Multiport Expansion */
#define ass_nexus_scsi	     1 /* Nexus SCSI Controller */

#define gvp_series_2_scsi   11
#define gvp_iv_24_gfx	    32

/* ********************************************************** */
/* 08 - 0A  */
/* er_Flags */
#define Z3_MEM_64KB	0x02
#define Z3_MEM_128KB	0x03
#define Z3_MEM_256KB	0x04
#define Z3_MEM_512KB	0x05
#define Z3_MEM_1MB	0x06 /* Zorro III card subsize */
#define Z3_MEM_2MB	0x07
#define Z3_MEM_4MB	0x08
#define Z3_MEM_6MB	0x09
#define Z3_MEM_8MB	0x0a
#define Z3_MEM_10MB	0x0b
#define Z3_MEM_12MB	0x0c
#define Z3_MEM_14MB	0x0d
#define Z3_MEM_16MB	0x00
#define Z3_MEM_AUTO	0x01
#define Z3_MEM_defunct1	0x0e
#define Z3_MEM_defunct2	0x0f

#define force_z3	0x10 /* *MUST* be set if card is Z3 */
#define ext_size	0x20 /* Use extended size table for bits 0-2 of er_Type */
#define no_shutup	0x40 /* Card cannot receive Shut_up_forever */
#define care_addr	0x80 /* Adress HAS to be $200000-$9fffff */

/* ********************************************************** */
/* 40-42 */
/* ec_interrupt (unused) */

#define enable_irq	0x01 /* enable Interrupt */
#define reset_card	0x04 /* Reset of Expansion Card - must be 0 */
#define card_int2	0x10 /* READ ONLY: IRQ 2 active */
#define card_irq6	0x20 /* READ ONLY: IRQ 6 active */
#define card_irq7	0x40 /* READ ONLY: IRQ 7 active */
#define does_irq	0x80 /* READ ONLY: Card currently throws IRQ */

/* ********************************************************** */

/* ROM defines (DiagVec) */

#define rom_4bit	(0x00<<14) /* ROM width */
#define rom_8bit	(0x01<<14)
#define rom_16bit	(0x02<<14)

#define rom_never	(0x00<<12) /* Never run Boot Code */
#define rom_install	(0x01<<12) /* run code at install time */
#define rom_binddrv	(0x02<<12) /* run code with binddrivers */

uaecptr ROM_filesys_resname = 0, ROM_filesys_resid = 0;
uaecptr ROM_filesys_diagentry = 0;
uaecptr ROM_hardfile_resname = 0, ROM_hardfile_resid = 0;
uaecptr ROM_hardfile_init = 0;

/* FELLOW OUT, autoconfig support and fast memory code removed */

/*
 * Filesystem device ROM
 * This is very simple, the Amiga shouldn't be doing things with it.
 */

static uae_u32 filesys_start; /* Determined by the OS */
uae_u8 filesysory[65536];

/* FELLOW OUT (START)----------------------------------

static uae_u32 filesys_lget(uaecptr) REGPARAM;
static uae_u32 filesys_wget(uaecptr) REGPARAM;
static uae_u32 filesys_bget(uaecptr) REGPARAM;
static void filesys_lput(uaecptr, uae_u32) REGPARAM;
static void filesys_wput(uaecptr, uae_u32) REGPARAM;
static void filesys_bput(uaecptr, uae_u32) REGPARAM;

uae_u32 REGPARAM2 filesys_lget(uaecptr addr)
{
    uae_u8 *m;
    addr -= filesys_start & 65535;
    addr &= 65535;
    m = filesysory + addr;
    return do_get_mem_long ((uae_u32 *)m);
}

uae_u32 REGPARAM2 filesys_wget(uaecptr addr)
{
    uae_u8 *m;
    addr -= filesys_start & 65535;
    addr &= 65535;
    m = filesysory + addr;
    return do_get_mem_word ((uae_u16 *)m);
}

uae_u32 REGPARAM2 filesys_bget(uaecptr addr)
{
    addr -= filesys_start & 65535;
    addr &= 65535;
    return filesysory[addr];
}

static void REGPARAM2 filesys_lput(uaecptr addr, uae_u32 l)
{
    write_log ("filesys_lput called\n");
}

static void REGPARAM2 filesys_wput(uaecptr addr, uae_u32 w)
{
    write_log ("filesys_wput called\n");
}

static void REGPARAM2 filesys_bput(uaecptr addr, uae_u32 b)
{
    fprintf (stderr, "filesys_bput called. This usually means that you are using\n");
    fprintf (stderr, "Kickstart 1.2. Please give UAE the \"-a\" option next time\n");
    fprintf (stderr, "you start it. If you are _not_ using Kickstart 1.2, then\n");
    fprintf (stderr, "there's a bug somewhere.\n");
    fprintf (stderr, "Exiting...\n");
    uae_quit ();
}

   FELLOW OUT (END)-----------------------------------*/

/* FELLOW IN (START)----------------------------------*/

ULO filesys_lgetC(ULO addr)
{
    uae_u8 *m;
    addr -= filesys_start & 65535;
    addr &= 65535;
    m = filesysory + addr;
    return do_get_mem_long ((uae_u32 *)m);
}

ULO filesys_wgetC(ULO addr)
{
    uae_u8 *m;
    addr -= filesys_start & 65535;
    addr &= 65535;
    m = filesysory + addr;
    return do_get_mem_word ((uae_u16 *)m);
}

ULO filesys_bgetC(ULO addr)
{
    addr -= filesys_start & 65535;
    addr &= 65535;
    return filesysory[addr];
}

void filesys_lputC(ULO l, ULO addr)
{
    write_log ("filesys_lput called\n");
}

void filesys_wputC(ULO w, ULO addr)
{
    write_log ("filesys_wput called\n");
}

void filesys_bputC(ULO b, ULO addr)
{
    fprintf (stderr, "filesys_bput called. This usually means that you are using\n");
    fprintf (stderr, "Kickstart 1.2. Please give UAE the \"-a\" option next time\n");
    fprintf (stderr, "you start it. If you are _not_ using Kickstart 1.2, then\n");
    fprintf (stderr, "there's a bug somewhere.\n");
    fprintf (stderr, "Exiting...\n");
    uae_quit ();
}

/* FELLOW IN (END)----------------------------------*/

/* FELLOW OUT (START) ------------------------------
addrbank filesys_bank = {
    filesys_lget, filesys_wget, filesys_bget,
    filesys_lput, filesys_wput, filesys_bput,
    default_xlate, default_check
};
/* FELLOW OUT (END)---------------------------------*/

/* FELLOW OUT, removed Z3 and Z2 code */

/* ********************************************************** */

/* 
 * Filesystem device
 */


/*static void expamem_map_filesys (void)  FELLOW OUT */
void expamem_map_filesys (int mapping)  /* FELLOW IN */
{
    uaecptr a;
    /* FELLOW IN (START) -------------------*/
    int bank;

    filesys_start = (mapping<<8) & 0xff0000;
    bank = filesys_start>>16;
    memoryBankSet(filesys_bgetASM, filesys_wgetASM, filesys_lgetASM, filesys_bputASM,
                  filesys_wputASM, filesys_lputASM, filesysory, bank, bank, FALSE);
    /* FELLOW IN (END) ----------------------*/
    
    /* FELLOW OUT (START)------------------------
    filesys_start = ((expamem_hi | (expamem_lo >> 4)) << 16);
    map_banks (&filesys_bank, filesys_start >> 16, 1);
    write_log ("Filesystem: mapped memory.\n");
       FELLOW OUT (START)------------------------*/

    /* 68k code needs to know this. */
    a = here ();
    org (0xF0FFFC);
    dl (filesys_start + 0x2000);
    org (a);
}

/* static   FELLOW OUT */
void expamem_init_filesys (void)
{
    uae_u8 diagarea[] = { 0x90, 0x00, 0x01, 0x0C, 0x01, 0x00, 0x01, 0x06 };

    expamem_init_clear();
    expamem_write (0x00, Z2_MEM_64KB | rom_card | zorroII);

    expamem_write (0x08, no_shutup);

    expamem_write (0x04, 2);
    expamem_write (0x10, hackers_id >> 8);
    expamem_write (0x14, hackers_id & 0xff);

    expamem_write (0x18, 0x00); /* ser.no. Byte 0 */
    expamem_write (0x1c, 0x00); /* ser.no. Byte 1 */
    expamem_write (0x20, 0x00); /* ser.no. Byte 2 */
    expamem_write (0x24, 0x01); /* ser.no. Byte 3 */

    expamem_write (0x28, 0x10); /* Rom-Offset hi */
    expamem_write (0x2c, 0x00); /* ROM-Offset lo */

    expamem_write (0x40, 0x00); /* Ctrl/Statusreg.*/

    /* Build a DiagArea */

    memcpy (expamem + 0x1000, diagarea, sizeof diagarea);

    /* Call DiagEntry */
    do_put_mem_word ((uae_u16 *)(expamem + 0x1100), 0x4EF9); /* JMP */
    do_put_mem_long ((uae_u32 *)(expamem + 0x1102), ROM_filesys_diagentry);

    /* What comes next is a plain bootblock */
    do_put_mem_word ((uae_u16 *)(expamem + 0x1106), 0x4EF9); /* JMP */
    do_put_mem_long ((uae_u32 *)(expamem + 0x1108), EXPANSION_bootcode);

    memcpy (filesysory, expamem, 0x3000);
}

/* FELLOW OUT, removed Z3 setup, Picasso mem setup and more expamem setup */









