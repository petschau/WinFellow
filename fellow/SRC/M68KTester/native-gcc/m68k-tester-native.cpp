/*
 *  m68k-tester-fellow.cpp - M68K emulator tester, Winfellow CPU core
 *
 *  m68k-tester (C) 2007 Gwenole Beauchesne
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "sysdeps.h"
#include "m68k-tester.h"

#define DEBUG 0
#include "debug.h"


// #include "DEFS.H"
// TE #include "CpuModule.h"




unsigned char *memory;
bool memory_fault_read;                       /* TRUE - read / FALSE - write */
uint32 memory_fault_address;

#define memoryReadByteFromPointer(address) (address[0])
#define memoryReadWordFromPointer(address) ((address[0] << 8) | address[1])
#define memoryReadLongFromPointer(address) ((address[0] << 24) | (address[1] << 16) | (address[2] << 8) | address[3])

void memoryWriteByteToPointer(uint8 data, uint8 *address)
{
  address[0] = data;
}

void memoryWriteWordToPointer(uint16 data, uint8 *address)
{
  address[0] = (uint8)(data >> 8);
  address[1] = (uint8)data;
}

void memoryWriteLongToPointer(uint32 data, uint8 *address)
{
  address[0] = (uint8)(data >> 24);
  address[1] = (uint8)(data >> 16);
  address[2] = (uint8)(data >> 8);
  address[3] = (uint8)data;
}

uint8 memoryReadByte(uint32 address)
{
  uint8 *p = memory + address;
  return memoryReadByteFromPointer(p);
}
uint16 memoryReadWord(uint32 address)
{
  uint8 *p = memory + address;
  return memoryReadWordFromPointer(p);
}
uint32 memoryReadLong(uint32 address)
{
  uint8 *p = memory + address;
  return memoryReadLongFromPointer(p);
}
void memoryWriteByte(uint8 data, uint32 address)
{
  uint8 *p = memory + address;
  memoryWriteByteToPointer(data, p);
}
void memoryWriteWord(uint16 data, uint32 address)
{
  uint8 *p = memory + address;
  memoryWriteWordToPointer(data, p);
}
void memoryWriteLong(uint32 data, uint32 address)
{
  uint8 *p = memory + address;
  memoryWriteLongToPointer(data, p);
}



// extern void cpuSetRaiseInterrupt(bool f);

m68k_cpu::m68k_cpu()
{
  memory = (unsigned char*) malloc(0x1000000);
/*cpuStartup();
  cpuSetModel(CPUType, 0);
  cpuSetRaiseInterrupt(FALSE);*/
}

m68k_cpu::~m68k_cpu()
{
  free(memory);
  memory = NULL;
}

uint32 m68k_cpu::get_pc() const
{
//return cpuGetPC();
}

void m68k_cpu::set_pc(uint32 pc)
{
//cpuSetPC(pc);
}

uint32 m68k_cpu::get_ccr() const
{
//return cpuGetSR() & 0xff;
}

void m68k_cpu::set_ccr(uint32 ccr)
{
//cpuSetSR(cpuGetSR() & 0xff00 | (ccr & 0xff));
}

uint32 m68k_cpu::get_dreg(int r) const
{
//return cpuGetDReg(r);
}

void m68k_cpu::set_dreg(int r, uint32 v)
{
//cpuSetDReg(r, v);
}

uint32 m68k_cpu::get_areg(int r) const
{
//return cpuGetAReg(r);
}

void m68k_cpu::set_areg(int r, uint32 v)
{
//cpuSetAReg(r, v);
}

void m68k_cpu::reset(void)
{
/*cpuStartup();
  cpuSetModel(CPUType, 0);
  cpuSetRaiseInterrupt(FALSE);*/
}

void m68k_cpu::reset_jit(void)
{
/*cpuStartup();
  cpuSetModel(CPUType, 0);
  cpuSetRaiseInterrupt(FALSE);*/
}

void m68k_cpu::execute(uint32 pc)
{
/*cpuInitializeFromNewPC(pc);
  uint16 next_opcode = memoryReadWord(cpuGetPC());
  while (next_opcode != 0x7100)
  {
    cpuExecuteInstruction();
    next_opcode = memoryReadWord(cpuGetPC());
  }*/
}

uint32 vm_get_byte(uint32 addr)
{
  return memoryReadByte(addr);
}
  
void vm_put_byte(uint32 addr, uint8 v)
{
  memoryWriteByte(v, addr);
}

uint32 vm_get_word(uint32 addr)
{
  return memoryReadWord(addr);
}

void vm_put_word(uint32 addr, uint16 v)
{
  memoryWriteWord(v, addr);
}

uint32 vm_get_long(uint32 addr)
{
  return memoryReadLong(addr);
}

void vm_put_long(uint32 addr, uint32 v)
{
  memoryWriteLong(v, addr);
}
