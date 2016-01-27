/*
 *  m68k-tester-uae.cpp - M68K emulator tester, glue for UAE-based cores
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
#include "vm_alloc.h"
#include "m68k-tester.h"

#define DEBUG 0
#include "debug.h"

typedef int8 uae_s8;
typedef int16 uae_s16;
typedef int32 uae_s32;
typedef int64 uae_s64;
typedef uint8 uae_u8;
typedef uint16 uae_u16;
typedef uint32 uae_u32;
typedef uint64 uae_u64;
typedef uint32 uaecptr;

#define ENUMDECL typedef enum
#define ENUMNAME(name) name
#define ASM_SYM_FOR_FUNC(a)
#define STATIC_INLINE static inline
#define __always_inline inline __attribute__((always_inline))

#if defined __i386__ || defined __x86_64__
# define REGPARAM __attribute__((regparm(3)))
#endif

#ifndef REGPARAM
# define REGPARAM
#endif
#define REGPARAM2

#ifndef EMU_UAE
#define call_mem_get_func(func, addr) ((*func)(addr))
#define call_mem_put_func(func, addr, v) ((*func)(addr, v))
static inline uae_u32 do_get_mem_long(uae_u32 *a) {uint8 *b = (uint8 *)a; return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];}
static inline uae_u32 do_get_mem_word(uae_u16 *a) {uint8 *b = (uint8 *)a; return (b[0] << 8) | b[1];}
static inline uae_u32 do_get_mem_byte(uae_u8 *a) {return *a;}
static inline void do_put_mem_long(uae_u32 *a, uae_u32 v) {uint8 *b = (uint8 *)a; b[0] = v >> 24; b[1] = v >> 16; b[2] = v >> 8; b[3] = v;}
static inline void do_put_mem_word(uae_u16 *a, uae_u32 v) {uint8 *b = (uint8 *)a; b[0] = v >> 8; b[1] = v;}
static inline void do_put_mem_byte(uae_u8 *a, uae_u32 v) {*a = v;}
#endif

const int JIT_CACHE_SIZE = 1024;

#ifdef EMU_BASILISK
#define RAM_BASE RAMBaseMac
#define RAM_HOST RAMBaseHost
#define RAM_SIZE RAMSize

uint32 ROMBaseMac;
uint8 *ROMBaseHost;
uint32 ROMSize;

bool PrefsFindBool(const char *name)
{
	D(bug("* PrefsFindBool('%s')\n", name));
#ifdef USE_JIT
	if (strcmp(name, "jit") == 0)
		return true;
	else if (strcmp(name, "jitfpu") == 0)
		return true;
	else if (strcmp(name, "jitlazyflush") == 0)
		return true;
	else if (strcmp(name, "jitinline") == 0)
		return false;
#ifdef JIT_DEBUG
	else if (strcmp(name, "jitdebug") == 0)
		return true;
#endif
#endif
	abort();
}

int32 PrefsFindInt32(const char *name)
{
	D(bug("* PrefsFindInt32('%s')\n", name));
#ifdef USE_JIT
	if (strcmp(name, "jitcachesize") == 0)
		return JIT_CACHE_SIZE;
#endif
	abort();
	return 0;
}

char *PrefsFindString(const char *name, int index)
{
	D(bug("* PrefsFindString('%s', %d)\n", name, index));
#ifdef USE_JIT
	if (strcmp(name, "jitblacklist") == 0)
		return "";
#endif
	abort();
	return NULL;
}

extern bool quit_program;
#endif

#ifdef EMU_ARANYM
#define RAM_BASE RAMBase
#define RAM_HOST RAMBaseHost
#define RAM_SIZE RAMSize

uint32 ROMBaseMac;
uint8 *ROMBaseHost;
uint32 ROMSize;

uint32 FastRAMSize;
uint32 VideoRAMBase;

uint32 nf_get_id(uaecptr stack)
{
	D(bug("* nf_get_id stack %08x\n", stack));
	abort();
	return 0;
}

uint32 nf_call(uaecptr stack, bool inSuper)
{
	D(bug("* nf_call stack %08x, inSuper %d\n", stack, inSuper));
	abort();
	return 0;
}

/* <SDL_keyboard.h> */
typedef int SDLKey;
typedef int SDLMod;
struct SDL_keysym {
	uint8 scancode;
	SDLKey sym;
	SDLMod mod;
	uint16 unicode;
};

#include "parameters.h"
bx_options_t bx_options;

extern int quit_program;

bool debugging = false;
void debug(void) {}
#define LOGGER pdbprintf
#define LOGGER_NEEDS_NEWLINE

void AtariReset(void) {}
int MFPdoInterrupt(void) { return 0; }
void invoke200HzInterrupt(void) {}
void report_double_bus_error(void) {}

uae_u32 HWget_l(uaecptr addr) {return 0;}
uae_u16 HWget_w(uaecptr addr) {return 0;}
uae_u8 HWget_b(uaecptr addr) {return 0;}
void HWput_l(uaecptr addr, uae_u32 l) {}
void HWput_w(uaecptr addr, uae_u16 w) {}
void HWput_b(uaecptr addr, uae_u8 b) {}

#ifdef ENABLE_EXCLUSIVE_SPCFLAGS
extern "C" {
	struct SDL_mutex;
#define SDL_LockMutex SDL_mutexP
	int SDL_mutexP(SDL_mutex *) {return 0;}
#define SDL_UnlockMutex SDL_mutexV
	int SDL_mutexV(SDL_mutex *) {return 0;}
	struct SDL_cond;
	int SDL_CondWait(SDL_cond *, SDL_mutex *) {return 0;}
	int SDL_CondSignal(SDL_cond *) {return 0;}
}
SDL_mutex *spcflags_lock = NULL;
SDL_cond *stop_condition = NULL;
#endif

#define options_init() aranym_options_init()

static void aranym_options_init(void)
{
#ifdef USE_JIT
	bx_options.jit.jit = true;
	bx_options.jit.jitfpu = true;
	bx_options.jit.jitcachesize = JIT_CACHE_SIZE;
	bx_options.jit.jitlazyflush = 1;
	bx_options.jit.jitinline = true;
#endif
}
#endif

#ifdef EMU_UAE
#define IS_C_CORE

int quit_program;

#define SPCFLAG_STOP 2
#define SPCFLAG_INT 8
#define SPCFLAG_BRK 16
#define SPCFLAG_TRACE 64
#define SPCFLAG_DOTRACE 128
#define SPCFLAG_MODE_CHANGE 8192

extern "C" {
#ifdef EMU_EUAE
#include "machdep/machdep.h"
#endif
#include "options.h"
struct uae_prefs currprefs, changed_prefs;

#include "zfile.h"
FILE *zfile_open(const char *, const char *) {return NULL;}
int zfile_close(FILE *) {}
struct zfile *zfile_fopen(const char *, const char *) {return NULL;}
void zfile_fclose(struct zfile *) {}
int zfile_fseek(struct zfile *, long, int) {return 0;}
size_t zfile_fread(void *, size_t, size_t, struct zfile *) {return 0;}
int zfile_exists(const char *) {return 0;}

#include "events.h"
#ifdef EMU_EUAE
volatile frame_time_t vsyncmintime;
#else
frame_time_t vsyncmintime;
#endif
frame_time_t syncbase;
frame_time_t idletime;

int vpos = 0;
uint16 dmacon = 0;
uint8 *rtarea = NULL;
int cloanto_rom = 0;
unsigned long currcycle = 0;
unsigned long nextevent = 0;
#ifdef EMU_EUAE
unsigned int is_lastline = 0;
#else
unsigned long is_lastline = 0;
#endif
struct ev eventtab[ev_max];

void debug(void) {}
bool debugging = false;
bool exception_debugging = false;

int action_replay_flag = 0;
uaecptr wait_for_pc = 0;
void action_replay_enter(void) {}
void action_replay_hide(void) {}
void action_replay_init(int) {}
void action_replay_cleanup(void) {}
void action_replay_map_banks(void) {}
void action_replay_memory_reset(void) {}
int action_replay_load(void) {return 0;}
int is_ar_pc_in_rom(void) {return 0;}
int hrtmon_flag = 0;
void hrtmon_enter (void) {}
void hrtmon_breakenter (void) {}
void hrtmon_hide (void) {}
void hrtmon_map_banks(void) {}
uae_u32 hrtmem_start = 0, hrtmem_size = 0;

int savestate_state = 0;
FILE *savestate_file = NULL;
const char *savestate_filename = NULL;
void save_u16_func(uae_u8 **, uae_u16) {}
void save_u32_func(uae_u8 **, uae_u32) {}
uae_u16 restore_u16_func(const uae_u8 **) {return 0;}
uae_u32 restore_u32_func(const uae_u8 **) {return 0;}
void restore_ram(size_t, uae_u8 *) {}
void restore_state(char *) {}
void savestate_restore_finish(void) {}

void do_cycles_ce(long) {}
uae_u32 wait_cpu_cycle_read(uaecptr, int) {return 0;}
void wait_cpu_cycle_write(uaecptr, int, uae_u32) {}
void compute_vsynctime(void) {}
#if defined(__linux__)
frame_time_t linux_get_tsc_freq (void) {return 0;}
#endif

uae_u32 get_crc32(uae_u8 *, unsigned int) {return 0;}
int uae_get_state(void) {return 0;}
void uae_reset(int) {}
void uae_restart(int, char*) {}
bool uae_state_change_pending(void) {return quit_program;}
void customreset(void) {}
void init_ersatz_rom(uae_u8 *) {}
void ersatz_perform(uae_u16) {}
#ifdef EMU_EUAE
void events_schedule(void) { nextevent += ~0ul; }
#endif
void reset_frame_rate_hack(void) {}
void do_copper(void) {}
unsigned int blitnasty(void) {return 0;}
void gui_message(const char *, ...) {}
void m68k_handle_trap(unsigned int, void *) {}
void call_calltrap(int nr) {}

#ifdef EMU_EUAE
#define LOGGER write_log
#else
#define LOGGER write_log_standard
#endif

#ifdef JIT
#include <stddef.h>

extern void set_cache_state(int);

signed long pissoff = 0;
int have_done_picasso = 1;

// XXX: don't bother with multiple memory.o objects...
enum {
	JIT_CACHE_MAGIC_1 = 0xc0de,
	JIT_CACHE_MAGIC_2 = 0xdead
};
struct cache_t {
	uint32 magic_1;
	uint32 cache_size;
	uint32 magic_2;
	uint32 _pad;
	uint8 code[1];
};
void *cache_alloc(int size)
{
	int page_size = getpagesize();
	int aligned_size = (sizeof(cache_t) + size + page_size - 1) & -page_size;
	cache_t *cp = (cache_t *)vm_acquire(aligned_size);
	if (cp == VM_MAP_FAILED)
		return NULL;
	if (vm_protect(cp, aligned_size, VM_PAGE_READ|VM_PAGE_WRITE|VM_PAGE_EXECUTE) < 0) {
		vm_release(cp, aligned_size);
		return NULL;
	}
	// inject the size cookie...
	cp->magic_1 = JIT_CACHE_MAGIC_1;
	cp->cache_size = aligned_size;
	cp->magic_2 = JIT_CACHE_MAGIC_2;
	return &cp->code[0];
}

void cache_free(void *cache)
{
	if (cache) {
		cache_t *cp = (cache_t *)((uint8 *)cache - offsetof(cache_t, code[0]));
		if (cp->magic_1 == JIT_CACHE_MAGIC_1 || cp->magic_2 == JIT_CACHE_MAGIC_2)
			vm_release(cp, cp->cache_size);
	}
}
#endif

void *xmalloc(size_t n)
{
	void *a = malloc(n);
	if (a == NULL) {
		fprintf(stderr, "xmalloc(%d): virtual memory exhausted\n", n);
		abort();
	}
	return a;
}
}

#define options_init() uae_options_init()

static void uae_options_init(void)
{
	// XXX: handle cpu_compatible and cpu_cycle_exact options?
	memset(&currprefs, 0, sizeof(currprefs));
	currprefs.cpu_level = CPUType;
	if (FPUType >= 1)
		currprefs.cpu_level = 3;
	if (FPUType >= 4)
		currprefs.cpu_level = 4;
#ifdef JIT
	currprefs.cachesize = JIT_CACHE_SIZE;
#endif
	memcpy(&changed_prefs, &currprefs, sizeof(changed_prefs));
}
#endif

#ifdef LOGGER
#include <stdarg.h>

#ifdef IS_C_CORE
extern "C" void LOGGER(const char *format, ...);
#endif

void LOGGER(const char *format, ...)
{
#if 0
	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
#ifdef LOGGER_NEEDS_NEWLINE
	fprintf(stdout, "\n");
#endif
	va_end(args);
#endif
}
#endif

#if defined EMU_BASILISK || defined EMU_ARANYM
struct M68kRegisters;

void EmulOp(uint16 opcode, M68kRegisters *r)
{
	D(bug("* EmulOp %04x\n", opcode));
	abort();
}
#endif

#ifdef IS_C_CORE
extern "C" {
#endif
#include "memory.h"
#include "newcpu.h"

int intlev(void)
{
	return 0;
}
#ifdef IS_C_CORE
}
#endif

#ifndef options_init
#define options_init()
#endif

#ifdef EMU_EUAE
#define m68k_regs		(&regs)
#define m68k_sr			m68k_regs->sr
#define m68k_getpc()	(m68k_getpc)(m68k_regs)
#define m68k_setpc(PC)	(m68k_setpc)(m68k_regs, (PC))
#define MakeSR()		(MakeSR)(m68k_regs)
#define MakeFromSR()	(MakeFromSR)(m68k_regs)
#define set_special(M)	(set_special)(m68k_regs, (M))
#define fill_prefetch_slow() (fill_prefetch_slow)(m68k_regs)

#define m68k_cpu_reset_jit() e_uae_reset_jit()

static void e_uae_reset_jit(void)
{
	flush_icache(666);
	set_special(0); // maximize the translated block length
}
#endif

#ifdef EMU_UAE
#define m68k_cpu_init()	uae_cpu_init()
#define m68k_cpu_exit()	uae_cpu_exit()
#define m68k_cpu_run()	uae_cpu_run()
#define m68k_cpu_reset() uae_cpu_reset()

static void uae_cpu_reset(void)
{
	set_special(0); // maximize the translated block length
}

#ifndef EMU_EUAE
struct flag_struct regflags;
extern "C" void reset_all_systems(void);
void reset_all_systems(void) {memory_reset();}
#endif
#endif

#ifdef USE_JIT
#include "compiler/compemu.h"

#define m68k_cpu_init()	jit_cpu_init()
#define m68k_cpu_exit()	jit_cpu_exit()
#define m68k_cpu_run()	jit_cpu_run()
#define m68k_cpu_reset_jit() jit_cpu_reset()

static void jit_cpu_init(void)
{
	options_init();
	init_m68k();
	compiler_init();
}

static void jit_cpu_exit(void)
{
	compiler_exit();
	exit_m68k();
}

static void jit_cpu_run(void)
{
	set_cache_state(1);
	SPCFLAGS_INIT(0); // maximize the translated block length
	m68k_compile_execute();
}

static void jit_cpu_reset(void)
{
	flush_icache(666);
	SPCFLAGS_INIT(0); // maximize the translated block length
}
#endif

#ifndef m68k_regs
#define m68k_regs		regs
#endif
#ifndef m68k_sr
#define m68k_sr			m68k_regs.sr
#endif
#ifndef m68k_cpu_init
#define m68k_cpu_init()	init_m68k()
#endif
#ifndef m68k_cpu_exit
#define m68k_cpu_exit()	exit_m68k()
#endif
#ifndef m68k_cpu_run
#define m68k_cpu_run()	m68k_execute()
#endif
#ifndef m68k_cpu_reset
#define m68k_cpu_reset()
#endif
#ifndef m68k_cpu_reset_jit
#define m68k_cpu_reset_jit()
#endif

#ifdef EMU_UAE
addrbank cia_bank, clock_bank, custom_bank, rtarea_bank, expamem_bank;

static void emulop_exec_return(uae_u32)
{
	// trigger a return from m68k_run_*() and from m68k_go(), we shall exit
	quit_program = true;
	set_special(SPCFLAG_MODE_CHANGE);
}

static void uae_cpu_init(void)
{
	options_init();
	init_m68k();

	// try to inject EXEC_RETURN EmulOp support without patching UAE sources
	cpufunctbl[M68K_EXEC_RETURN] = (cpuop_func *)emulop_exec_return;
}

static void uae_cpu_exit(void)
{
}

static void uae_cpu_run(void)
{
#ifdef JIT
	set_cache_state(1);
#endif
	fill_prefetch_slow();
	m68k_go(1);
}
#endif

uint32 RAM_BASE;
uint8 *RAM_HOST;
uint32 RAM_SIZE;
#if DIRECT_ADDRESSING || FIXED_ADDRESSING
uintptr MEMBaseDiff;
#endif

static int m68k_memory_init(void)
{
	if (vm_init() < 0)
		return -1;

	RAM_BASE = 0x0000;
	RAM_SIZE = 0x2000;
#if REAL_ADDRESSING
	RAM_HOST = (uint8 *)(uintptr)RAMBaseMac;
	if (vm_acquire_fixed(RAM_HOST, RAM_SIZE, VM_MAP_DEFAULT | VM_MAP_32BIT) < 0)
		return -1;
#elif DIRECT_ADDRESSING
	RAM_HOST = (uint8 *)vm_acquire(RAM_SIZE, VM_MAP_DEFAULT | VM_MAP_32BIT);
	if (RAM_HOST == VM_MAP_FAILED)
		return -1;
	MEMBaseDiff = (uintptr)RAM_HOST;
#elif FIXED_ADDRESSING
	RAM_HOST = (uint8 *)FMEMORY;
	if (vm_acquire_fixed(RAM_HOST, RAM_SIZE, VM_MAP_DEFAULT | VM_MAP_32BIT) < 0)
		return -1;
	MEMBaseDiff = (uintptr)RAM_HOST;
#else
	RAM_HOST = (uint8 *)vm_acquire(RAM_SIZE);
	if (RAM_HOST == VM_MAP_FAILED)
		return -1;
	memory_init();
#endif
#ifdef EMU_UAE
	strcpy(currprefs.romfile, "/dev/null");
	strcpy(changed_prefs.romfile, currprefs.romfile);
	currprefs.chipmem_size = changed_prefs.chipmem_size = RAM_SIZE;
	memory_reset();
#endif

	return 0;
}

static void m68k_memory_exit(void)
{
#ifdef EMU_UAE
	memory_cleanup();
#endif
	vm_exit();
}

m68k_cpu::m68k_cpu()
{
	m68k_memory_init();
	m68k_cpu_init();
}

m68k_cpu::~m68k_cpu()
{
	m68k_cpu_exit();
	m68k_memory_exit();
}

uint32 m68k_cpu::get_pc() const
{
	return m68k_getpc();
}

void m68k_cpu::set_pc(uint32 pc)
{
	m68k_setpc(pc);
}

uint32 m68k_cpu::get_ccr() const
{
	MakeSR();
	return m68k_sr & M68K_CCR_BITS;
}

void m68k_cpu::set_ccr(uint32 ccr)
{
	MakeSR();
	m68k_sr = (m68k_sr & ~M68K_CCR_BITS) | (ccr & M68K_CCR_BITS);
	MakeFromSR();
}

uint32 m68k_cpu::get_dreg(int r) const
{
	return m68k_dreg(m68k_regs, r);
}

void m68k_cpu::set_dreg(int r, uint32 v)
{
	m68k_dreg(m68k_regs, r) = v;
}

uint32 m68k_cpu::get_areg(int r) const
{
	return m68k_areg(m68k_regs, r);
}

void m68k_cpu::set_areg(int r, uint32 v)
{
	m68k_areg(m68k_regs, r) = v;
}

void m68k_cpu::reset(void)
{
	for (int i = 0; i < 8; i++) {
		set_dreg(0, 0);
		set_areg(0, 0);
	}
	set_ccr(0);

	m68k_cpu_reset();
}

void m68k_cpu::reset_jit(void)
{
	m68k_cpu_reset_jit();
}

void m68k_cpu::execute(uint32 pc)
{
	D(bug("* execute code at %08x\n", pc));
	set_pc(pc);
	quit_program = false;
	m68k_cpu_run();
	quit_program = false;
}

uint32 vm_get_byte(uint32 addr)
{
	return get_byte(addr);
}

void vm_put_byte(uint32 addr, uint8 v)
{
	put_byte(addr, v);
}

uint32 vm_get_word(uint32 addr)
{
	return get_word(addr);
}

void vm_put_word(uint32 addr, uint16 v)
{
	put_word(addr, v);
}

uint32 vm_get_long(uint32 addr)
{
	return get_long(addr);
}

void vm_put_long(uint32 addr, uint32 v)
{
	put_long(addr, v);
}
