/*
 *  m68k-tester.h - M68K emulator tester
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

#ifndef M68K_TESTER_H
#define M68K_TESTER_H

// CPU type (0 = 68000, 1 = 68010, 2 = 68020, 3 = 68030, 4 = 68040/060)
extern int CPUType;

// FPU type (0 = no FPU, 1 = 68881, 2 = 68882, 4 = 68040)
extern int FPUType;

class m68k_cpu {
	void *opaque;
 public:
	m68k_cpu();
	~m68k_cpu();

	uint32 get_pc() const;
	void set_pc(uint32 pc);
	uint32 get_ccr() const;
	void set_ccr(uint32 ccr);

	uint32 get_dreg(int r) const;
	void set_dreg(int r, uint32 v);
	uint32 get_areg(int r) const;
	void set_areg(int r, uint32 v);

	void reset(void);
	void reset_jit(void);
	void execute(uint32 pc);
};

enum {
  M68K_CCR_X	= 1 << 4,
  M68K_CCR_N	= 1 << 3,
  M68K_CCR_Z	= 1 << 2,
  M68K_CCR_V	= 1 << 1,
  M68K_CCR_C	= 1 << 0,
  M68K_CCR_BITS	= M68K_CCR_X | M68K_CCR_N | M68K_CCR_Z | M68K_CCR_V | M68K_CCR_C
};

const uint16 M68K_NOP = 0x4e71;
const uint16 M68K_RTS = 0x4e75;
const uint16 M68K_EXEC_RETURN = 0x7100;

const uint32 M68K_CODE_BASE = 0x1000;
const uint32 M68K_CODE_SIZE = 0x2000;

extern uint32 vm_get_byte(uint32 addr);
extern void vm_put_byte(uint32 addr, uint8 v);
extern uint32 vm_get_word(uint32 addr);
extern void vm_put_word(uint32 addr, uint16 v);
extern uint32 vm_get_long(uint32 addr);
extern void vm_put_long(uint32 addr, uint32 v);

#endif /* M68K_TESTER_H */
