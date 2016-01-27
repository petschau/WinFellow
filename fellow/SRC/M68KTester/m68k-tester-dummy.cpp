/*
 *  m68k-tester-dummy.cpp - M68K emulator tester, glue template
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
// PS #include "vm_alloc.h"
#include "m68k-tester.h"

#define DEBUG 0
#include "debug.h"


m68k_cpu::m68k_cpu()
{
}

m68k_cpu::~m68k_cpu()
{
}

uint32 m68k_cpu::get_pc() const
{
	return 0;
}

void m68k_cpu::set_pc(uint32 pc)
{
}

uint32 m68k_cpu::get_ccr() const
{
	return 0;
}

void m68k_cpu::set_ccr(uint32 ccr)
{
}

uint32 m68k_cpu::get_dreg(int r) const
{
	return 0;
}

void m68k_cpu::set_dreg(int r, uint32 v)
{
}

uint32 m68k_cpu::get_areg(int r) const
{
	return 0;
}

void m68k_cpu::set_areg(int r, uint32 v)
{
}

void m68k_cpu::reset(void)
{
}

void m68k_cpu::reset_jit(void)
{
}

void m68k_cpu::execute(uint32 pc)
{
	D(bug("* execute code at %08x\n", pc));
}

uint32 vm_get_byte(uint32 addr)
{
	return 0;
}

void vm_put_byte(uint32 addr, uint8 v)
{
}

uint32 vm_get_word(uint32 addr)
{
	return 0;
}

void vm_put_word(uint32 addr, uint16 v)
{
}

uint32 vm_get_long(uint32 addr)
{
	return 0;
}

void vm_put_long(uint32 addr, uint32 v)
{
}
