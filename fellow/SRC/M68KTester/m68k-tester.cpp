/*
 *  m68k-tester.cpp - M68K emulator tester
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
// PS #include <unistd.h>
// PS #include <alloca.h>
#include <errno.h>
// PS #include <netinet/in.h>

#include "sysdeps.h"
#include "m68k-tester.h"

#ifdef ENABLE_MON
#include "mon.h"

static uint32 mon_do_read_byte(uintptr addr)
{
	return vm_get_byte(addr);
}

static void mon_do_write_byte(uintptr addr, uint32 b)
{
	vm_put_byte(addr, b);
}
#endif

#define DEBUG 0
#include "debug.h"

// Define to enable instruction patching to alternate opcodes
#define ENABLE_PATCHES 1


struct m68k_instruction_t {
	int mnemo;
	char name[8];
	int n_words;
	uint16 words[1];
};

struct m68k_cpu_state_t {
	uint32 ccr;
	uint32 dregs[8];
	uint32 aregs[8];
	uint8 use_dregs;
	uint8 use_aregs;
};

struct m68k_testcase_t {
	m68k_instruction_t *inst;
	m68k_cpu_state_t input_state;
	m68k_cpu_state_t output_state;
	m68k_cpu_state_t gen_state;
};

enum {
	PARSER_SW_BINARY	= -4,
	PARSER_ERROR		= -3,
	PARSER_EOF			= -2,
	PARSER_READ_SERIES	= -1,
	PARSER_READ_TEST	= 1,
	PARSER_READ_OPCODE	= 2,
};


/* ------------------------------------------------------------------------- */
/* --- Helper Functions                                                 --- */
/* ------------------------------------------------------------------------- */

static int get_num(uint32 *vp, const char *lp, int base)
{
	errno = 0;
	*vp = strtoul(lp, NULL, base);
	if (errno != 0)
		return -1;
	return 0;
}

static inline int get_dec(uint32 *vp, const char *lp)
{
	return get_num(vp, lp, 10);
}

static inline int get_hex(uint32 *vp, const char *lp)
{
	return get_num(vp, lp, 16);
}

static int get_str(char *str, const char *lp, char sep = '=')
{
	if ((lp = strchr(lp, sep)) == NULL)
		return -1;
	if (sscanf(lp + 1, "%s", str) != 1)
		return -1;
	return 0;
}

static int get_be32(FILE *fp, uint32 *vp)
{
	uint32 v;
	if (fread(&v, sizeof(v), 1, fp) != 1)
		return -1;
	*vp = ntohl(v);
	return 0;
}

static int put_be32(FILE *fp, uint32 v)
{
	uint32 out = htonl(v);
	if (fwrite(&out, sizeof(out), 1, fp) != 1)
		return -1;
	return 0;
}

static const char *ccr2str(uint32 ccr)
{
	static char ccr_str[6];
	char *p = ccr_str;
	*p++ = (ccr & 0x10) ? 'X' : '.';
	*p++ = (ccr & 0x08) ? 'N' : '.';
	*p++ = (ccr & 0x04) ? 'Z' : '.';
	*p++ = (ccr & 0x02) ? 'V' : '.';
	*p++ = (ccr & 0x01) ? 'C' : '.';
	*p = '\0';
	return ccr_str;
}

struct stdout_state_t {
	int old_stdout;
};

static int stdout_state_init(stdout_state_t *s)
{
	if (s == NULL)
		return -1;
	s->old_stdout = -1;
	return 0;
}

static int stdout_off(stdout_state_t *s)
{
	assert(s != NULL);
	fflush(stdout);
	if (s->old_stdout == -1) {
		s->old_stdout = dup(STDOUT_FILENO);
		freopen("/dev/null", "w", stdout);
	}
	if (s->old_stdout == 1)
		return -1;
	return 0;
}

static int stdout_on(stdout_state_t *s)
{
	assert(s != NULL);
	fflush(stdout);
	if (s->old_stdout != -1) {
		dup2(s->old_stdout, STDOUT_FILENO);
		close(s->old_stdout);
		s->old_stdout = -1;
	}
	return 0;
}


/* ------------------------------------------------------------------------- */
/* --- Default VM accessors                                              --- */
/* ------------------------------------------------------------------------- */

#ifdef VM_DEFAULT_ACCESSORS
template< int N, int I = N >
struct vm_read_memory {
	static inline uint32 apply(uint8 *addr)
		{ return (vm_read_memory<N, I - 1>::apply(addr) << 8) | addr[I - 1]; }
};

template< int N >
struct vm_read_memory<N, 1> {
	static inline uint32 apply(uint8 *addr)
		{ return addr[0]; }
};

template< int N, int I = N >
struct vm_write_memory {
	static inline void apply(uint8 *addr, uint32 value)
		{ addr[I - 1] = value; vm_write_memory<N, I - 1>::apply(addr, value >> 8); }
};

template< int N >
struct vm_write_memory<N, 1> {
	static inline void apply(uint8 *addr, uint32 value)
		{ addr[0] = value; }
};

#ifdef REAL_ADDRESSING
static inline uint8 *vm_translate_addr(uint32 addr)
{
	return (uint8 *)(uintptr)addr;
}
#endif

uint32 vm_get_byte(uint32 addr)
{
	return vm_read_memory<1>::apply(vm_translate_addr(addr));
}

void vm_put_byte(uint32 addr, uint8 v)
{
	vm_write_memory<1>::apply(vm_translate_addr(addr), v);
}

uint32 vm_get_word(uint32 addr)
{
	return vm_read_memory<2>::apply(vm_translate_addr(addr));
}

void vm_put_word(uint32 addr, uint16 v)
{
	vm_write_memory<2>::apply(vm_translate_addr(addr), v);
}

uint32 vm_get_long(uint32 addr)
{
	return vm_read_memory<4>::apply(vm_translate_addr(addr));
}

void vm_put_long(uint32 addr, uint32 v)
{
	vm_write_memory<4>::apply(vm_translate_addr(addr), v);
}
#endif


/* ------------------------------------------------------------------------- */
/* --- Text Format Parser                                                --- */
/* ------------------------------------------------------------------------- */

/*
 *  opcode_bin:4201
 *  good opcode: clrb
 *  before d0=00000000    d1=00000000    CCR=..... (0)
 *  M68K   d0=00000000    d1=00000000    CCR=..Z.. (4) NV_same
 *  GEN    d0=00000000    d1=00000000    CCR=..Z..   (4)
 *
 *  good opcode: clrb
 *  before d0=00000000    d1=00000000    CCR=....C (1)
 *  M68K   d0=00000000    d1=00000000    CCR=..Z.. (4) NV_same
 *  GEN    d0=00000000    d1=00000000    CCR=..Z..   (4)
 *
 *  good opcode: clrb
 *  before d0=00000000    d1=00000000    CCR=...V. (2)
 *  M68K   d0=00000000    d1=00000000    CCR=..Z.. (4) NV_changed
 *  GEN    d0=00000000    d1=00000000    CCR=..Z..   (4)
 *
 *  [...]
 */

static int get_cpu_state_text(char *str, m68k_cpu_state_t *csp)
{
	uint32 value;
	int error = 0;
	const char *cp = str;
	const char * const ep = cp + strlen(cp);
	while (cp < ep && *cp != '\0') {
		switch (*cp) {
		case 'd':
			if ((cp + 11) <= ep && cp[2] == '=') {
				error |= get_hex(&value, cp + 3);
				int regno = cp[1] - '0';
				csp->dregs[regno] = value;
				csp->use_dregs |= 1 << regno;
				cp += 11;
			}
			break;
		case 'a':
			if ((cp + 11) <= ep && cp[2] == '=') {
				error |= get_hex(&value, cp + 3);
				int regno = cp[1] - '0';
				csp->aregs[regno] = value;
				csp->use_aregs |= 1 << regno;
				cp += 11;
			}
			break;
		case 'f':
			// XXX: floating-point registers
			break;
		case 'C':
			if ((cp + 9) <= ep && cp[1] == 'C' && cp[2] == 'R') {
				if ((cp = strchr(cp + 9, '(')) != NULL) {
					error |= get_dec(&value, &cp[1]);
					csp->ccr = value;
				}
				cp = ep; // CCR field is the last item to parse
			}
			break;
		}
		++cp;
	}
	return error;
}

static int read_testcase_text(FILE *fp, m68k_testcase_t *tp)
{
	const int N_INSN_WORDS_MAX = 8;

	if (tp == NULL)
		return PARSER_ERROR;

	m68k_instruction_t *inst = tp->inst;
	if (inst == NULL) {
		inst = (m68k_instruction_t *)malloc(sizeof(*inst) + (N_INSN_WORDS_MAX * 2));
		if (inst == NULL)
			return PARSER_ERROR;
		tp->inst = inst;
	}

	int rc = 0;
	int error = 0;
	char line[256];
	uint32 value;

	if (feof(fp))
		return PARSER_EOF;
	if (ferror(fp))
		return PARSER_ERROR;

	while (fgets(line, sizeof(line), fp) != NULL) {
		// Read line
		int len = strlen(line);
		if (len == 1) {
			rc |= PARSER_READ_TEST;
			break;
		}
		line[len - 1] = '\0';

		// Parse line
		if (strncmp(line, "binary", 6) == 0)
			return PARSER_SW_BINARY;
		else if (strncmp(line, "opcode_bin", 10) == 0) {
			inst->name[0] = '\0';
			inst->n_words = 0;
			const char *cp = &line[11];
			const char * const ep = line + len;
			while (cp < ep) {
				error |= get_hex(&value, cp);
				inst->words[inst->n_words] = value;
				if ((cp += 4) <= ep) {
					switch (*cp) {
					case ' ':
					case '\0':
						++cp;
						inst->n_words++;
						break;
					}
				}
			}
			rc = PARSER_READ_OPCODE;
		}
		else if (strncmp(line, "good opcode", 11) == 0) {
			if (inst->name[0] == '\0')
				error |= get_str(inst->name, &line[11], ':');
		}
		else if (strncmp(line, "broken opcode", 13) == 0) {
			if (inst->name[0] == '\0')
				error |= get_str(inst->name, &line[13], ':');
		}
		else if (strncmp(line, "before", 6) == 0)
			error |= get_cpu_state_text(&line[7], &tp->input_state);
		else if (strncmp(line, "M68K", 4) == 0)
			error |= get_cpu_state_text(&line[7], &tp->output_state);
		else if (strncmp(line, "GEN", 3) == 0)
			error |= get_cpu_state_text(&line[7], &tp->gen_state);
		else {
			char dummy[sizeof(line)];
			int gen_errors, gen_tests;
			if (sscanf(line, "%d %[^0-9] %d tests done", &gen_errors, dummy, &gen_tests) == 3) {
				D(bug("* Results file reports %d errors out of %d tests\n", gen_errors, gen_tests));
				return PARSER_READ_SERIES;
			}
			fprintf(stderr, "ERROR: could not parse '%s'\n", line);
			return PARSER_ERROR;
		}
	}

	if (error)
		return PARSER_ERROR;

	return rc;
}


/* ------------------------------------------------------------------------- */
/* --- Binary Format Parser                                              --- */
/* ------------------------------------------------------------------------- */

/*
 *  INS (instruction)
 *    uint32 signature	: 2;	// signature == 1
 *    uint32 name_len	: 3;	// name length - 1 (trailing '\0')
 *    uint32 extra_longs: 11;	// extra instruction words (through 32-bit chunks)
 *    uint32 opcode		: 16;
 *
 *  CPU (CPU state)
 *    uint32 signature	: 2;	// signature == 2
 *    uint32 reg1_type	: 2;
 *    uint32 reg1_id	: 3;
 *    uint32 reg2_type	: 2;
 *    uint32 reg2_id	: 3;
 *    uint32 reg3_type	: 2;
 *    uint32 reg3_id	: 3;
 *    uint32 reg4_type	: 2;
 *    uint32 reg4_id	: 3;
 *    uint32 reg5_type	: 2;
 *    uint32 reg5_id	: 3;
 *    uint32 ccr_value	: 5;
 *
 *  RES (results stats)
 *    uint32 signature	: 2;	// signature == 3
 *    uint32 type		: 1;
 *    uint32 count		: 29;
 *
 *  END (end marker)
 *    uint32 signature	: 2;	// signature == 0
 *    uint32 type		: 2;
 *    uint32 _padding	: 12;
 *    uint32 char_ff	: 8;	// FF ('\f')
 *    uint32 char_lf	: 8;	// LF ('\n')
 */

enum {
	BF_SIGNATURE_END			= 0,
	BF_SIGNATURE_INSTRUCTION	= 1,
	BF_SIGNATURE_CPU_STATE		= 2,
	BF_SIGNATURE_RESULTS_STATS	= 3
};

enum {
	BF_CPU_REG_NONE				= 0,
	BF_CPU_REG_DATA				= 1,
	BF_CPU_REG_ADDR				= 2,
	BF_CPU_REG_FP				= 3
};

enum {
	BF_END_TEST					= 1,
	BF_END_SERIES				= 2,
};

enum {
	BF_RES_N_TESTS				= 0,
	BF_RES_N_ERRORS				= 1
};

template< int FB, int FE >
struct static_mask {
	enum { value = (0xffffffff >> FB) ^ (0xffffffff >> (FE + 1)) };
};

template< int FB >
struct static_mask<FB, 31> {
	enum { value  = 0xffffffff >> FB };
};

template< int FB, int FE >
struct bit_field {
	static inline uint32 mask() {
		return static_mask<FB, FE>::value;
	}
	static inline bool test(uint32 value) {
		return value & mask();
	}
	static inline uint32 extract(uint32 value) {
		const uint32 m = mask() >> (31 - FE);
		return (value >> (31 - FE)) & m;
	}
	static inline void insert(uint32 & data, uint32 value) {
		const uint32 m = mask();
		data = (data & ~m) | ((value << (31 - FE)) & m);
	}
};

typedef bit_field<  0,  1 > BLK_signature;
typedef bit_field<  2,  3 > END_type;
typedef bit_field< 17, 23 > END_char_ff;
typedef bit_field< 24, 31 > END_char_lf;
typedef bit_field<  2,  4 > INS_name_length;
typedef bit_field<  5, 15 > INS_extra_longs;
typedef bit_field< 16, 31 > INS_opcode;
typedef bit_field<  2,  3 > CPU_reg1_type;
typedef bit_field<  4,  6 > CPU_reg1_id;
typedef bit_field<  7,  8 > CPU_reg2_type;
typedef bit_field<  9, 11 > CPU_reg2_id;
typedef bit_field< 12, 13 > CPU_reg3_type;
typedef bit_field< 14, 16 > CPU_reg3_id;
typedef bit_field< 17, 18 > CPU_reg4_type;
typedef bit_field< 19, 21 > CPU_reg4_id;
typedef bit_field< 22, 23 > CPU_reg5_type;
typedef bit_field< 24, 26 > CPU_reg5_id;
typedef bit_field< 27, 31 > CPU_ccr_value;
typedef bit_field<  2,  2 > RES_type;
typedef bit_field<  3, 31 > RES_count;

const int N_CPU_REGISTERS_MAX = 5;

static int get_register_binary(FILE *fp, m68k_cpu_state_t *csp, int type, int regno)
{
	int error = 0;
	switch (type) {
	case BF_CPU_REG_DATA:
		csp->use_dregs |= 1 << regno;
		error |= get_be32(fp, &csp->dregs[regno]);
		break;
	case BF_CPU_REG_ADDR:
		csp->use_aregs |= 1 << regno;
		error |= get_be32(fp, &csp->aregs[regno]);
		break;
	case BF_CPU_REG_FP:
		error = - 1;
		break;
	}
	return 0;
}

static int get_cpu_state_binary(uint32 value, FILE *fp, m68k_cpu_state_t *csp)
{
	int error = 0;
	if (CPU_reg1_type::test(value))
		error |= get_register_binary(fp, csp, CPU_reg1_type::extract(value), CPU_reg1_id::extract(value));
	if (CPU_reg2_type::test(value))
		error |= get_register_binary(fp, csp, CPU_reg2_type::extract(value), CPU_reg2_id::extract(value));
	if (CPU_reg3_type::test(value))
		error |= get_register_binary(fp, csp, CPU_reg3_type::extract(value), CPU_reg3_id::extract(value));
	if (CPU_reg4_type::test(value))
		error |= get_register_binary(fp, csp, CPU_reg4_type::extract(value), CPU_reg4_id::extract(value));
	if (CPU_reg5_type::test(value))
		error |= get_register_binary(fp, csp, CPU_reg5_type::extract(value), CPU_reg5_id::extract(value));
	return error;
}

static int read_testcase_binary(FILE *fp, m68k_testcase_t *tp)
{
	if (tp == NULL)
		return PARSER_ERROR;

	m68k_instruction_t *inst = tp->inst;
	m68k_cpu_state_t *csp;

	int rc = 0;
	int error = 0;
	int len;
	int n_cpu_blocks = 0;
	int gen_errors = 0, gen_tests = 0;
	uint32 value;

	if (feof(fp))
		return PARSER_EOF;
	if (ferror(fp))
		return PARSER_ERROR;

	while (get_be32(fp, &value) == 0) {
		switch (BLK_signature::extract(value)) {
		case BF_SIGNATURE_END:
			switch (END_type::extract(value)) {
			case BF_END_TEST:
				return rc | PARSER_READ_TEST;
			case BF_END_SERIES:
				return PARSER_READ_SERIES;
			}
			fprintf(stderr, "ERROR: unhandled END type %d\n", END_type::extract(value));
			return PARSER_ERROR;
		case BF_SIGNATURE_INSTRUCTION:
			len = INS_extra_longs::extract(value);
			if (inst != NULL)
				free(inst);
			if ((inst = (m68k_instruction_t *)malloc(sizeof(*inst) + (2 * len))) == NULL)
				return PARSER_ERROR;
			tp->inst = inst;
			inst->n_words = 1 + 2 * len;
			inst->words[0] = INS_opcode::extract(value);
			for (int i = 0; i < len; i++) {
				uint32 opcode;
				error |= get_be32(fp, &value);
				inst->words[2*i + 0] = value >> 16;
				inst->words[2*i + 1] = value;
			}
                        len = INS_name_length::extract(value);
                        { // PS
                          char *name = (char *)alloca(len + 1);
                          memset(name, 0, len + 1);
                          error |= fread(name, len, 1, fp) != 1;
                          error |= strncpy(inst->name, name, sizeof(inst->name)) == NULL;
                          rc |= PARSER_READ_OPCODE;
                        } // PS
			break;
		case BF_SIGNATURE_CPU_STATE:
			switch (++n_cpu_blocks) {
			case 1:
				csp = &tp->input_state;
				break;
			case 2:
				csp = &tp->output_state;
				break;
			case 3:
				csp = &tp->gen_state;
				break;
			default:
				fprintf(stderr, "ERROR: too many CPU blocks\n");
				return PARSER_ERROR;
			}
			error |= get_cpu_state_binary(value, fp, csp);
			csp->ccr = CPU_ccr_value::extract(value);
			break;
		case BF_SIGNATURE_RESULTS_STATS:
			switch (RES_type::extract(value)) {
			case BF_RES_N_TESTS:
				gen_tests = RES_count::extract(value);
				break;
			case BF_RES_N_ERRORS:
				gen_errors = RES_count::extract(value);
				D(bug("* Results file reports %d errors out of %d tests\n", gen_errors, gen_tests));
				break;
			}
			break;
		default:
			fprintf(stderr, "ERROR: unhandled block signature %d\n", BLK_signature::extract(value));
			return PARSER_ERROR;
		}
	}

	if (error)
		return PARSER_ERROR;

	return rc;
}


/* ------------------------------------------------------------------------- */
/* --- Test Functions                                                    --- */
/* ------------------------------------------------------------------------- */

static void prepare_test(m68k_cpu *cpu, m68k_instruction_t *inst);

enum {
	OPCODE_NO_ERROR		= 0,
	OPCODE_DELTA_D0		= 1 << 0,
	OPCODE_DELTA_D1		= 1 << 1,
	OPCODE_DELTA_FLAGS	= 1 << 2,
};

static int check_test(m68k_testcase_t *tp)
{
	int error = OPCODE_NO_ERROR;
	if (tp->gen_state.dregs[0] != tp->output_state.dregs[0])
		error |= OPCODE_DELTA_D0;
	if (tp->gen_state.dregs[1] != tp->output_state.dregs[1])
		error |= OPCODE_DELTA_D1;
	if (tp->gen_state.ccr != tp->output_state.ccr)
		error |= OPCODE_DELTA_FLAGS;
	return error;
}

enum {
	i_UNKNOWN,
	i_LSL,
	i_LSR,
	i_ASL,
	i_ASR,
};

static int get_mnemo(m68k_instruction_t *inst)
{
	uint32 opcode = inst->words[0];
	static const struct {
		uint16 mask;
		uint16 match;
		int insn;
	}
	instr_table[] = {
		{ 0xf118, 0xe108, i_LSL },
		{ 0xf118, 0xe008, i_LSR },
		{ 0xf118, 0xe100, i_ASL },
		{ 0xf118, 0xe000, i_ASR },
		{ 0x0000, 0x0000, i_UNKNOWN }
	};
	int insn = i_UNKNOWN;
	for (int i = 0; instr_table[i].insn != i_UNKNOWN; i++) {
		if ((opcode & instr_table[i].mask) == instr_table[i].match)
			return instr_table[i].insn;
	}
	return i_UNKNOWN;
}

enum {
	PATCH_NONE	= 0,
	PATCH_IMMI	= 1 << 0
};

static void patch_immi(m68k_cpu *cpu, m68k_testcase_t *tp)
{
	m68k_instruction_t *inst = tp->inst;
	uint32 d0 = tp->input_state.dregs[0];
	uint32 d1 = tp->input_state.dregs[1];

	// XXX: this is terribly wrong
	static uint16 old_opcode = 0;
	uint16 opcode = inst->words[0];
	switch (inst->mnemo) {
	case i_LSL:
	case i_LSR:
		// match LSd.z D0,D1
		if ((opcode & 0xfe3f) == 0xe029)
			goto do_patch_shift;
	case i_ASL:
	case i_ASR:
		// match ASd.z D0,D1
		if ((opcode & 0xfe3f) == 0xe021)
			goto do_patch_shift;
		break;
	do_patch_shift:
		if (d0 >= 1 && d0 <= 8) {
			opcode &= ~0x0e20;
			opcode |= (d0 == 8 ? 0 : d0) << 9; // i/r == 0 (immediate shift count)
		}
		break;
	}
	if (opcode != old_opcode) {
		printf("* Patch instruction to %04x\n", opcode);
		uint16 saved_opcode = inst->words[0];
		inst->words[0] = opcode;
		prepare_test(cpu, inst);
		inst->words[0] = saved_opcode;
	}
	old_opcode = opcode;
}

static void patch_test(m68k_cpu *cpu, m68k_testcase_t *tp, uint32 patches = 0)
{
#if ENABLE_PATCHES
	if (patches & PATCH_IMMI)
		patch_immi(cpu, tp);
#endif
}

static void prepare_test(m68k_cpu *cpu, m68k_instruction_t *inst)
{
	uint32 addr = M68K_CODE_BASE;
	for (int i = 0; i < inst->n_words; i++) {
		vm_put_word(addr, inst->words[i]);
		addr += 2;
	}
	vm_put_word(addr, M68K_EXEC_RETURN);
	assert(vm_get_word(addr) == M68K_EXEC_RETURN);

	cpu->reset_jit();
}

static int run_test(m68k_cpu *cpu, m68k_testcase_t *tp, uint32 patches = 0)
{
	assert(tp != NULL && tp->inst != NULL);
	patch_test(cpu, tp, patches);

	cpu->reset();

	cpu->set_ccr(tp->input_state.ccr);
	for (int i = 0; i < 8; i++) {
		cpu->set_dreg(i, tp->input_state.dregs[i]);
		cpu->set_areg(i, tp->input_state.aregs[i]);
	}

	cpu->execute(M68K_CODE_BASE);

	tp->gen_state.ccr = cpu->get_ccr();
	for (int i = 0; i < 8; i++) {
		tp->gen_state.dregs[i] = cpu->get_dreg(i);
		tp->gen_state.aregs[i] = cpu->get_areg(i);
	}

	return check_test(tp);
}

enum {
	FORMAT_TEXT,
	FORMAT_BINARY,
	FORMAT_DEFAULT = FORMAT_TEXT
};

static void print_instruction_text(m68k_instruction_t *inst)
{
	printf("opcode_bin:%04x", inst->words[0]);
	for (int i = 1; i < inst->n_words; i++)
		printf(" %04x", inst->words[i]);
	printf("\n");
}

static void print_instruction_binary(m68k_instruction_t *inst)
{
	uint32 value;
	FILE *fp = stdout;

	// binary format marker
	printf("binary\n");

	// INS block
	value = 0;
	BLK_signature::insert(value, BF_SIGNATURE_INSTRUCTION);
	INS_name_length::insert(value, strlen(inst->name));
	INS_extra_longs::insert(value, inst->n_words / 2);
	INS_opcode::insert(value, inst->words[0]);
	put_be32(fp, value);

	// additional opcodes
	for (int i = 1; i < inst->n_words; i += 2) {
		value = ((uint32)inst->words[i]) << 16;
		if (i + 1 < inst->n_words)
			value |= inst->words[i + 1];
		else
			value |= M68K_NOP;
		put_be32(fp, value);
	}

	// instruction name
	if (fwrite(inst->name, strlen(inst->name), 1, fp) != 1) {
		fprintf(stderr, "ERROR: could not emit instruction name\n");
		abort();
	}
}

static void print_instruction(m68k_instruction_t *inst, int format = FORMAT_DEFAULT)
{
	switch (format) {
	case FORMAT_TEXT:
		print_instruction_text(inst);
		break;
	case FORMAT_BINARY:
		print_instruction_binary(inst);
		break;
	default:
		fprintf(stderr, "ERROR: unhandled format %d\n", format);
		abort();
	}
}

static void print_test_text(m68k_testcase_t *tp)
{
	int error = check_test(tp);

	printf("%s opcode: %s %s %s %s\n",
		   error ? "broken" : "good",
		   tp->inst->name,
		   (error & OPCODE_DELTA_D0) ? "deltad0" : " ",
		   (error & OPCODE_DELTA_D1) ? "deltad1" : " ",
		   (error & OPCODE_DELTA_FLAGS) ? "deltaflags" : " ");
	printf("before d0=%08x    d1=%08x    CCR=%s (%d)\n",
		   tp->input_state.dregs[0],
		   tp->input_state.dregs[1],
		   ccr2str(tp->input_state.ccr), tp->input_state.ccr);
	printf("M68K   d0=%08x    d1=%08x    CCR=%s (%d) %s\n",
		   tp->output_state.dregs[0],
		   tp->output_state.dregs[1],
		   ccr2str(tp->output_state.ccr), tp->output_state.ccr,
		   ((tp->input_state.ccr ^ tp->output_state.ccr) & (M68K_CCR_N | M68K_CCR_V)) ? "NV_changed" : "NV_same");
	printf("GEN    d0=%08x %c  d1=%08x %c  CCR=%s %c (%d)\n",
		   tp->gen_state.dregs[0], (error & OPCODE_DELTA_D0) ? '*' : ' ',
		   tp->gen_state.dregs[1], (error & OPCODE_DELTA_D1) ? '*' : ' ',
		   ccr2str(tp->gen_state.ccr), (error & OPCODE_DELTA_FLAGS) ? '*' : ' ', tp->gen_state.ccr);
	printf("\n");
}

static int put_CPU_block(FILE *fp, m68k_cpu_state_t *csp)
{
	uint32 values[N_CPU_REGISTERS_MAX + 1];
	memset(values, 0, sizeof(values));

	typedef void (*insert_fn)(uint32 &, uint32);
	static const struct insert_table_t {
		insert_fn type;
		insert_fn id;
	}
	insert_fns[N_CPU_REGISTERS_MAX] = {
		{ &CPU_reg1_type::insert, &CPU_reg1_id::insert },
		{ &CPU_reg2_type::insert, &CPU_reg2_id::insert },
		{ &CPU_reg3_type::insert, &CPU_reg3_id::insert },
		{ &CPU_reg4_type::insert, &CPU_reg4_id::insert },
		{ &CPU_reg5_type::insert, &CPU_reg5_id::insert },
	};

	int n_regs = 0;
	BLK_signature::insert(values[0], BF_SIGNATURE_CPU_STATE);
	CPU_ccr_value::insert(values[0], csp->ccr);
	if (csp->use_dregs) {
		for (int i = 0; i < 8; i++) {
			if (csp->use_dregs & (1U << i)) {
				assert(n_regs < N_CPU_REGISTERS_MAX);
				insert_fns[n_regs].type(values[0], BF_CPU_REG_DATA);
				insert_fns[n_regs].id(values[0], i);
				values[++n_regs] = csp->dregs[i];
			}
		}
	}
	if (csp->use_aregs) {
		for (int i = 0; i < 8; i++) {
			if (csp->use_aregs & (1U << i)) {
				assert(n_regs < N_CPU_REGISTERS_MAX);
				insert_fns[n_regs].type(values[0], BF_CPU_REG_ADDR);
				insert_fns[n_regs].id(values[0], i);
				values[++n_regs] = csp->aregs[i];
			}
		}
	}

	int error = 0;
	for (int i = 0; i <= n_regs; i++)
		error |= put_be32(fp, values[i]);
	return error;
}

static int put_END_block(FILE *fp, int type)
{
	uint32 value = 0;
	BLK_signature::insert(value, BF_SIGNATURE_END);
	END_type::insert(value, type);
	END_char_ff::insert(value, '\f');
	END_char_lf::insert(value, '\n');
	return put_be32(fp, value);
}

static int put_RES_block(FILE *fp, int type, uint32 count)
{
	uint32 value = 0;
	BLK_signature::insert(value, BF_SIGNATURE_RESULTS_STATS);
	RES_type::insert(value, type);
	RES_count::insert(value, count);
	return put_be32(fp, value);
}

static void print_test_binary(m68k_testcase_t *tp)
{
	uint32 value;
	FILE *fp = stdout;

	// CPU block (before)
	put_CPU_block(fp, &tp->input_state);

	// CPU block (M68K)
	put_CPU_block(fp, &tp->output_state);

	// CPU block (GEN)
	put_CPU_block(fp, &tp->gen_state);

	// END block
	put_END_block(fp, BF_END_TEST);
}

static void print_test(m68k_testcase_t *tp, int format = FORMAT_DEFAULT)
{
	switch (format) {
	case FORMAT_TEXT:
		print_test_text(tp);
		break;
	case FORMAT_BINARY:
		print_test_binary(tp);
		break;
	default:
		fprintf(stderr, "ERROR: unhandled format %d\n", format);
		abort();
	}
}

static void print_test_end_text(m68k_testcase_t *tp, int n_errors, int n_tests)
{
	printf("%d errors out of %d tests done for %s (%04x)\n",
		   n_errors, n_tests,
		   tp->inst->name, tp->inst->words[0]);
}

static void print_test_end_binary(m68k_testcase_t *tp, int n_errors, int n_tests)
{
	put_RES_block(stdout, BF_RES_N_TESTS, n_tests);
	put_RES_block(stdout, BF_RES_N_ERRORS, n_errors);
	put_END_block(stdout, BF_END_SERIES);
}

static void print_test_end(m68k_testcase_t *tp, int n_errors, int n_tests, int format = FORMAT_DEFAULT)
{
	switch (format) {
	case FORMAT_TEXT:
		print_test_end_text(tp, n_errors, n_tests);
		break;
	case FORMAT_BINARY:
		print_test_end_binary(tp, n_errors, n_tests);
		break;
	default:
		fprintf(stderr, "ERROR: unhandled format %d\n", format);
		abort();
	}
}

enum {
	CMD_HELP,
	CMD_TEST,
	CMD_PRINT,
	CMD_CONVERT,
#ifdef EMU_DUMMY
	CMD_DEFAULT = CMD_CONVERT
#else
	CMD_DEFAULT = CMD_TEST
#endif
};

int CPUType = 0;
int FPUType = 0;

int main(int argc, char *argv[])
{
#if ENABLE_MON
	// Initialize mon
	mon_init();
	mon_read_byte = mon_do_read_byte;
	mon_write_byte = mon_do_write_byte;
#endif

	int cmd = CMD_DEFAULT;
	int input_format = FORMAT_DEFAULT;
	int output_format = FORMAT_DEFAULT;
	bool verbose = false;
	uint32 patches = PATCH_NONE;

	for (int i = 1; i < argc; i++) {
		const char *arg = argv[i];
		if (i == 1) {
			if (strcmp(arg, "help") == 0) {
				cmd = CMD_HELP;
				continue;
			}
			else if (strcmp(arg, "print") == 0) {
				cmd = CMD_PRINT;
				continue;
			}
			else if (strcmp(arg, "convert") == 0) {
				cmd = CMD_CONVERT;
				continue;
			}
#ifndef EMU_DUMMY
			else if (strcmp(arg, "test") == 0) {
				cmd = CMD_TEST;
				continue;
			}
#endif
		}
                if (strncmp(arg, "--file=", 7) == 0) {
                  if (freopen(&arg[7], "rb", stdin) == NULL)
                  {
                    printf("File not found.\n");
                    exit(1);
                  }
                }
                else if (strcmp(arg, "--verbose") == 0)
			verbose = true;
		else if (strcmp(arg, "--format") == 0) {
			if (++i < argc) {
				arg = argv[i];
				if (strcmp(arg, "text") == 0)
					output_format = FORMAT_TEXT;
				else if (strcmp(arg, "binary") == 0)
					output_format = FORMAT_BINARY;
			}
		}
		else if (strcmp(arg, "--patches") == 0) {
			if (++i < argc) {
				const char *cp = arg = argv[i];
				for (;;) {
					if (*cp == ',' || *cp == '\0') {
						int len = cp - arg;
						if (strncmp(arg, "immi", len) == 0)
							patches |= PATCH_IMMI;
						arg = cp + 1;
						if (*cp == '\0')
							break;
					}
					++cp;
				}
			}
		}
		else if (strcmp(arg, "--cpu") == 0) {
			if (++i < argc) {
				arg = argv[i];
				if (strcmp(arg, "680000") == 0 || strcmp(arg, "0") == 0)
					CPUType = 0;
				else if (strcmp(arg, "68010") == 0 || strcmp(arg, "1") == 0)
					CPUType = 1;
				else if (strcmp(arg, "68020") == 0 || strcmp(arg, "2") == 0)
					CPUType = 2;
				else if (strcmp(arg, "68030") == 0 || strcmp(arg, "3") == 0)
					CPUType = 3;
				else if (strcmp(arg, "68040") == 0 || strcmp(arg, "4") == 0)
					FPUType = CPUType = 4;
				else if (strcmp(arg, "68060") == 0 || strcmp(arg, "6") == 0)
					FPUType = CPUType = 4;
			}
		}
		else if (strcmp(arg, "--fpu") == 0) {
			FPUType = 1;
			if (++i < argc) {
				arg = argv[i];
				if (strcmp(arg, "68881") == 0)
					FPUType = 1;
				else if (strcmp(arg, "68882") == 0)
					FPUType = 2;
				else if (strcmp(arg, "68040") == 0)
					FPUType = 4;
				else if (strcmp(arg, "68060") == 0)
					FPUType = 4;
			}
		}
	}

	if (cmd == CMD_HELP) {
		printf("Usage: %s [<command>] [<option>]*\n", argv[0]);
		printf("\n");
		printf("Commands:\n");
		printf("  help              print this message\n");
#ifndef EMU_DUMMY
		printf("  test              perform the tests (default)\n");
#endif
		printf("  print             print the input results file\n");
		printf("  convert           convert results files\n");
		printf("\n");
		printf("Options:\n");
		printf("  --verbose         flag: set verbose mode\n");
		printf("  --patches p1,...  flag: add p1,... to opcode patches list\n");
		printf("  --cpu CPU         set CPU mode (680x0)\n");
		printf("  --fpu FPU         set FPU mode (68881, 68882, 68040)\n");
		printf("  --format FORMAT   set output FORMAT mode (text, binary)\n");
		return 0;
	}

	stdout_state_t stdout_state;
	stdout_state_init(&stdout_state);
	if (cmd == CMD_CONVERT || !verbose)
		stdout_off(&stdout_state);

	m68k_cpu *cpu = new m68k_cpu;

	stdout_on(&stdout_state);

	m68k_testcase_t testcase;
	memset(&testcase, 0, sizeof(testcase));
	int n_tests, n_tests_total = 0;
	int n_errors, n_errors_total = 0;

	while (!feof(stdin) && !ferror(stdin)) {
		int rc;
		memset(&testcase.input_state, 0, sizeof(testcase.input_state));
		memset(&testcase.output_state, 0, sizeof(testcase.output_state));
		switch (input_format) {
		case FORMAT_TEXT:
			rc = read_testcase_text(stdin, &testcase);
			if (rc != PARSER_SW_BINARY)
				break;
			input_format = FORMAT_BINARY;
			// fall-through
		case FORMAT_BINARY:
			rc = read_testcase_binary(stdin, &testcase);
			break;
		}

		if (rc >= 0) {
			if (rc & PARSER_READ_OPCODE) {
				n_errors = n_tests = 0;
				switch (cmd) {
				case CMD_TEST:
					testcase.inst->mnemo = get_mnemo(testcase.inst);
					prepare_test(cpu, testcase.inst);
					if (!verbose)
						break;
					// fall-through
				case CMD_PRINT:
					print_instruction(testcase.inst, FORMAT_TEXT);
					break;
				case CMD_CONVERT:
					print_instruction(testcase.inst, output_format);
					break;
				}
			}
			if (rc & PARSER_READ_TEST) {
				++n_tests;
				int error = 0;
				switch (cmd) {
				case CMD_TEST:
					error = run_test(cpu, &testcase, patches);
					if (!error && !verbose)
						break;
					// fall-through
				case CMD_PRINT:
					print_test(&testcase, FORMAT_TEXT);
					break;
				case CMD_CONVERT:
					print_test(&testcase, output_format);
					break;
				}
				if (error)
					++n_errors;
			}
		}
		else if (rc == PARSER_EOF)
			break;
		else if (rc == PARSER_READ_SERIES) {
			switch (cmd) {
			case CMD_TEST:
			case CMD_PRINT:
                                stdout_on(&stdout_state);
				print_test_end(&testcase, n_errors, n_tests, FORMAT_TEXT);
				break;
			case CMD_CONVERT:
				print_test_end(&testcase, n_errors, n_tests, output_format);
			}
			fflush(stdout); // XXX: dup2(old_stdout) makes it no longer line buffered?
			n_tests_total += n_tests;
			n_errors_total += n_errors;
			input_format = FORMAT_DEFAULT;
		}
		else {
			fprintf(stderr, "ERROR: could not read results file correctly\n");
			abort();
		}
	}

	if (cmd == CMD_TEST) {
		printf("\n");
		printf("Global summary: %d errors out of %d tests done\n", n_errors_total, n_tests_total);
		printf("\n");
	}

	if (cmd == CMD_CONVERT || !verbose)
		stdout_off(&stdout_state);

	if (testcase.inst)
		free(testcase.inst);
	delete cpu;

	stdout_on(&stdout_state);

	return 0;
}
