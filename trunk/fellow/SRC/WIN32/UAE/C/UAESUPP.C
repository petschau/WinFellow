/*======================================================================*/
/* Fellow Amiga Emulator                                                */
/* UAE support                                                          */
/* Stuff that Fellow does not have, and the UAE filesystem-hander needs */
/* (C) 1997 Petter Schau                                                */
/*======================================================================*/


#include <stdio.h>
#include "uae2fell.h"
#include "fmem.h"

char uaehf0[256];
ULO hardfile_size = 0;
char warning_buffer[256];

ULO do_get_mem_byte(uae_u8 *address) {
  unsigned char *addr = (unsigned char *) address;
  return *addr;
}

ULO do_get_mem_word(uae_u16 *address) {
  unsigned char *addr = (unsigned char *) address;
  return ((*addr)<<8) | *(addr + 1);
}

ULO do_get_mem_long(uae_u32 *address) {
  unsigned char *addr = (unsigned char *) address;
  return ((*addr)<<24) | ((*(addr + 1))<<16) | ((*(addr + 2))<<8) | *(addr+3);
}

void do_put_mem_long(uae_u32 *address, uae_u32 data) {
  unsigned char *addr = (unsigned char *) address;
  *(addr) = data>>24;
  *(addr + 1) = data>>16;
  *(addr + 2) = data>>8;
  *(addr + 3) = data;
}

void do_put_mem_word(uae_u16 *address, uae_u16 data) {
  unsigned char *addr = (unsigned char *) address;
  *(addr) = data>>8;
  *(addr + 1) = data;
}

void do_put_mem_byte(uae_u8 *address, uae_u8 data) {
  unsigned char *addr = (unsigned char *) address;
  *addr = data;
}

/*-------------------------------------------------*/

int valid_address(uaecptr adr, uae_u32 size) {
  return ((memory_bank_pointer[(adr & 0xffffff)>>16] != NULL) &&
	  (memory_bank_pointer[((adr + size) & 0xffffff)>>16] != NULL));
}

/* This was taken from compiler.c in duae075b */

void m68k_do_rts(void)
{
    m68k_setpc(get_long(m68k_areg(regs, 7)));
    m68k_areg(regs, 7) += 4;
    /* FELLOW OUT (START)------------
       if (jsr_num > 0)
	jsr_num--;
       FELLOW OUT (END)------------------*/
}

/* This was taken from wuae 088 and modified */

#define WRITE_LOG_BUF_SIZE 128

void write_log (const char *format,...)
{
    DWORD numwritten;
    char buffer[WRITE_LOG_BUF_SIZE];
    va_list parms;
    int count = 0;
    int *blah = (int *)0xdeadbeef;

    va_start (parms, format);
    count = _vsnprintf( buffer, WRITE_LOG_BUF_SIZE-1, format, parms );
	fellowAddLog(buffer);
    va_end (parms);
}
