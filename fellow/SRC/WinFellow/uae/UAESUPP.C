/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* UAE support                                                             */
/*                                                                         */
/* Stuff that Fellow does not have, and the UAE filesystem-hander needs    */
/*                                                                         */
/* (w)1997 Petter Schau                                                    */
/* (w)2004 by Torsten Enderling <carfesh@gmx.net>                          */
/*                                                                         */
/* Copyright (C) 1991, 1992, 1996 Free Software Foundation, Inc.           */
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

#include <stdio.h>

#ifdef _FELLOW_DEBUG_CRT_MALLOC
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#ifdef _FELLOW_DEBUG_CRT_MALLOC
#include <crtdbg.h>
#endif

#include "uae2fell.h"
#include "MemoryInterface.h"
#include "VirtualHost/Core.h"

char uaehf0[256];
uint32_t hardfile_size = 0;
char warning_buffer[256];

uint32_t do_get_mem_byte(uae_u8 *address)
{
  unsigned char *addr = (unsigned char *)address;
  return *addr;
}

uint32_t do_get_mem_word(uae_u16 *address)
{
  unsigned char *addr = (unsigned char *)address;
  return ((*addr) << 8) | *(addr + 1);
}

uint32_t do_get_mem_long(uae_u32 *address)
{
  unsigned char *addr = (unsigned char *)address;
  return ((*addr) << 24) | ((*(addr + 1)) << 16) | ((*(addr + 2)) << 8) | *(addr + 3);
}

void do_put_mem_long(uae_u32 *address, uae_u32 data)
{
  unsigned char *addr = (unsigned char *)address;
  *(addr) = data >> 24;
  *(addr + 1) = data >> 16;
  *(addr + 2) = data >> 8;
  *(addr + 3) = data;
}

void do_put_mem_word(uae_u16 *address, uae_u16 data)
{
  unsigned char *addr = (unsigned char *)address;
  *(addr) = data >> 8;
  *(addr + 1) = (unsigned char)data;
}

void do_put_mem_byte(uae_u8 *address, uae_u8 data)
{
  unsigned char *addr = (unsigned char *)address;
  *addr = data;
}

/*-------------------------------------------------*/

int valid_address(uaecptr adr, uae_u32 size)
{
  return ((memory_bank_pointer[(adr & 0xffffff) >> 16] != nullptr) && (memory_bank_pointer[((adr + size) & 0xffffff) >> 16] != nullptr));
}

/* This was taken from compiler.c in duae075b */

void m68k_do_rts()
{
  m68k_setpc(get_long(m68k_areg(regs, 7)));
  cpuSetAReg(7, m68k_areg(regs, 7) + 4);
  /* FELLOW OUT (START)----------------
     if (jsr_num > 0)
  jsr_num--;
     FELLOW OUT (END)------------------ */
}

/* This was taken from wuae 088 and modified */

#define WRITE_LOG_BUF_SIZE 128

void write_log(const char *format, ...)
{
  /* DWORD numwritten; */
  char buffer[WRITE_LOG_BUF_SIZE];
  va_list parms;
  int count = 0;
  /* FELLOW REMOVE: int *blah = (int *)0xdeadbeef; */

  va_start(parms, format);
  count = _vsnprintf(buffer, WRITE_LOG_BUF_SIZE - 1, format, parms);
  _core.Log->AddLog(buffer);
  va_end(parms);
}