;===============================================================================
; Fellow Amiga Emulator      
; Bus Event Scheduling
;
; Author: Petter Schau (peschau@online.no)
;
; This file is under the GNU Public License (GPL)
;===============================================================================

;===============================================================================
; Summary of what is in this file:
;
; This is the heart of the emulator. It works with an event queue, and plainly
; executes the code associated with that event.
;
; * Startup code that will run 3 variations of the event queue scheduler.
; * Optimized run-as fast as you can scheduler.
; * A more considerate version the debugger can use to run only some
;   events at the time.
; * A special scheduler that is automatically entered when the CPU
;   is in trace mode and thus needs to check some more stuff each time.
; * End of line handler (An event type).
; * End of frame handler (An event type).
; 
; Summary of optimizations:
;
; * The queue is kept sorted based on an obscure criterion which assumes that
;   some event types happen much more often than others.
;   A CPU instruction is special and not really in the queue,
;   the queue is considered to be inferior to the emulated CPU.
; * Different modules schedule themselves using some macros defined in
;   bus.mac, the scheduler itself will only take events off the queue.
; * Decoding the next CPU instruction is an inlined macro in the scheduler
;   loop, which saves lots of time.
;
; Status of the code in this file:
;
; Drawbacks:
; * The scheduler is unreadable, difficult to modify. And the queue-insert
;   macros are even weirder due to the odd sorting method.
;
; Advantages:
; * It works.
;===============================================================================


%include "mac/nasm.mac"
%include "mac/renaming.mac"
%include "mac/callconv.mac"
%include "generic/defs.inc"

FASMFILESTART
FDATASECTIONSTART

%include "data/fellow.inc"
%include "data/cpu.inc"
%include "data/bus.inc"
%include "data/fmem.inc"
%include "data/draw.inc"
%include "data/floppy.inc"
%include "data/sound.inc"
%include "data/graph.inc"
%include "data/cia.inc"
%include "data/copper.inc"
%include "data/blit.inc"
%include "data/timer.inc"		
%include "data/graph.inc"

CDATAEXTERN f12pressed	

global _bus_blitter_delay
_bus_blitter_delay:	db	0,1,0,0

FDATASECTIONEND
FCODESECTIONSTART

		FALIGNHELPER

global _busstart_
_busstart_:	 ; Used to check code alignment

extern _gfxDrvEndOfFrame

%include "generic/sound.inc"
%include "func/fmem.inc"
%include "func/cpu.inc"
%include "func/floppy.inc"
%include "func/sound.inc"
%include "func/gameport.inc"
%include "func/copper.inc"
%include "func/cia.inc"
%include "func/draw.inc"
%include "func/blit.inc"
%include "func/graph.inc"
%include "func/sprite.inc"
%include "func/kbd.inc"
%include "mac/fmem.mac"
%include "mac/drawmode.mac"
%include "mac/kbd.mac"
%include "mac/bus.mac"
%include "mac/cpu.mac"
%include "mac/cia.mac"
%include "mac/draw.mac"
%include "mac/sound.mac"
%include "mac/sprite.mac"


;===========================================================
; bus_run_
; The entrypoint to the emulation when no debugging is done.
; No trace-bit is tested, if updatesr detects a tracebit set
; it will modify the stack so the 68k emu returns to a
; routine that does......
;===========================================================


		FALIGN32

global _bus_run_
_bus_run_:	pushad
		call	bus_run
		popad
		ret


bus_run:	SETUP_EXCEPTION_HACK bus_exception_ret, bus_loop_events
		test	byte [sr + 1], 0c0h
		jnz	near bus_cpu_trace_start
		cmp	dword [cpu_next], -1
		je	near bus_loop_events
		jmp	bus_loop_cpu

		FALIGN32

global _checkadr_
_checkadr_:
bus_loop_cpu:
	
		EXEC_NEXT_68K
		mov	ecx, dword [cpu_speed]
		mov	ebx, dword [thiscycle]
		mov	dword [thiscycle], 0
		shr	eax, cl
		add	eax, ebx
		add	eax, dword [curcycle]
		mov	dword [cpu_next], eax
		NEXT_EVENT_AFTER_CPU bus_loop_cpu, bus_loop_events
		ret

		
		FALIGN32

bus_exception_ret:
		mov	dword [thiscycle], 0
		add	eax, dword [curcycle]
		mov	dword [cpu_next], eax
		NEXT_EVENT_AFTER_CPU bus_loop_cpu, bus_loop_events
bus_exit:	ret

		FALIGN32
bus_loop_events:
		NEXT_EVENT bus_loop_cpu


;===============================================================
; Routines called when cpu is in tracemode
; When updatesr detects that the cpu has been set in trace-mode
; it switches to these stubs.
; It will run any events in between, and then reenter the main
; loop.
;===============================================================


bus_loop_cpu_trace:

		test	byte [debugging], 1
		jnz	bus_exit

bus_cpu_trace_start:
		EXEC_NEXT_68K_TRACE
global _bus_trace_mode_
_bus_trace_mode_:
		mov	ecx, dword [cpu_speed]
		mov	dword [pc], esi
		shr	eax, cl
		add	eax, dword [thiscycle]
		mov	dword [thiscycle], 0
		add	eax, dword [curcycle]
		mov	dword [cpu_next], eax
		NEXT_EVENT_AFTER_CPU bus_loop_cpu_trace, bus_loop_events_trace
		ret

		FALIGN32
bus_loop_events_trace:
		NEXT_EVENT bus_loop_cpu_trace


;===========================================================
; bus_debug_
; The entrypoint to the emulation when debugging is done.
; Tests trace bit, too.
; The switch is performed by updatesr.
;===========================================================


		FALIGN32

global _bus_debug_
_bus_debug_:	pushad
		call	bus_debug
		popad
		ret


bus_debug:	SETUP_EXCEPTION_HACK bus_debug_exception_ret, bus_debug_loop_events
		cmp	dword [cpu_next], -1
		je	near bus_debug_loop_events_start
		jmp	bus_debug_start

		FALIGN32

bus_debug_loop_cpu:

		test	byte [debugging], 1
		jnz	near bus_exit
bus_debug_start:

		EXEC_NEXT_68K_TRACE
		add	eax, dword [thiscycle]
		mov	ecx, dword [cpu_speed]
		shr	eax, cl
		add	eax, dword [thiscycle]
bus_debug_exception_ret:
		mov	dword [thiscycle], 0 
		add	eax, dword [curcycle]
		mov	dword [cpu_next], eax
		NEXT_EVENT_AFTER_CPU bus_debug_loop_cpu, bus_debug_loop_events
		ret

		FALIGN32
bus_debug_loop_events:
		NEXT_EVENT bus_debug_loop_cpu

		FALIGN32
bus_debug_loop_events_start:  
		NEXT_EVENT bus_debug_start


;==========================================================
; When CPU traces, this routine is called.
; cpuPrepareException will jmp back into the "fast" bus_loop
; or the debug loop, depending on what is set in
; exceptionbackdoors
;==========================================================

		FALIGN32

tracebit_handler:
		call	dword [edi]
		mov	dword [pc], esi
		mov	ebx, 024h
		jmp	_cpuPrepareException_




;===========================================================
; Clean up after STOP was executed and enter bus_loop again 
; Called from 68000.asm only
;===========================================================


		FALIGN32

global _return_from_stop_
_return_from_stop_:
		mov	dword [pc], esi
		mov	dword [thiscycle], 0	
		mov	dword [cpu_next], -1
		jmp	dword [stopreturnadr]


;==============================================================================
; Global end of line handler
;==============================================================================


		FALIGN32

global _endOfLine_
_endOfLine_:	push	edx
		push	ecx
		push	ebx


		;==============================================================
		; Handles graphics planar to chunky conversion
		; and updates the graphics emulation for a new line
		;==============================================================

		call	_graphEndOfLine_


		;==============================================================
		; Update the CIA B event counter
		;==============================================================

		mov	eax, 1
		CIA_UPDATE_EVENTCOUNTER_CWRAP


		;==============================================================
		; Handle disk DMA if it is running
		;==============================================================

		cmp	byte [floppy_DMA_started], 1
		jne	.l2
		cmp	byte [floppy_DMA_read], 1
		jne	.l1
		pushad
		call	floppyDMARead
		popad
		jmp	.l2
.l1:
		pushad
		call	floppyDMAWrite
		popad
.l2:

		;==============================================================
		; Update the sound emulation
		;==============================================================

		SOUND_END_OF_LINE_CWRAP


		;==============================================================
		; Handle keyboard events
		;==============================================================

		KBDQUEUEHANDLER_CWRAP
		KBDEVENTEOLHANDLER_CWRAP


		;==============================================================
		; Set up the next end of line event
		;==============================================================

		add	dword [eol_next], CYCLESPERLINE
		SCAN_EVENTS_LVL3

		pop	ebx
		pop	ecx
		pop	edx
		ret


;==============================================================================
; Global end of frame handler
;==============================================================================


		FALIGN32

global _endOfFrame_
_endOfFrame_:	push	edx
		push	ebx


		;==============================================================
		; Draw the frame in the host buffer
		;==============================================================

		call	_drawEndOfFrame_


		;==============================================================
		; Handle keyboard events
		;==============================================================

		KBDEVENTEOFHANDLER_CWRAP


		;==============================================================
		; Prepare emulation stop of f12 was pressed
		;==============================================================

		cmp	dword [f12pressed], 0
		setne	byte [debugging]

		;==============================================================
		; Reset some aspects of the graphics emulation
		;==============================================================

		xor	dword [lof], 08000h		; Short/long frame 
		xor	edx, edx
		mov	ecx, 0102h			; Zero scroll
		call	dword [memory_iobank_write + 2*ecx]


		;==============================================================
		; Restart copper
		;==============================================================

		call	_copperEndOfFrame_		; Restart copper


		;==============================================================
		; Update frame counters
		;==============================================================

		inc	dword [draw_frame_count]	; Count frames
		dec	dword [draw_frame_skip]		; Frame skipping
		jns	.l6
		mov	edx, dword [draw_frame_skip_factor]
		mov	dword [draw_frame_skip], edx
.l6:

		;==============================================================
		; Update CIA timer counters
		;==============================================================

		CIA_UPDATE_TIMERS_EOF_CWRAP


		;==============================================================
		; Sprite end of frame updates
		;==============================================================

		SPRITE_EOF_CWRAP


		;==============================================================
		; Generate FLAG int (INDEX)  every 16th frame.
		; Just to keep some loaders happy
		;==============================================================

		mov	edx, [draw_frame_count]
		and	edx, 01fh
		jnz	.l11
		mov	eax, 1
		mov	edx, 010h
		CIA_RAISE_IRQ_CWRAP
.l11:

		;==============================================================
		; Generate dsksync value int every 8th frame unless
		; disk DMA is working, let's see what good it does
		; Probably no good at all, take me away
		;==============================================================

		cmp	dword [floppy_DMA_started], 1
		je	.l13
		mov	edx, dword [draw_frame_count]
		and	edx, 7h
		jnz	.l13
		mov	edx, 09000h
		call	dword [memory_iobank_write + 0138h]
.l13:

		;==============================================================
		; Recalculate blitter finished time
		;==============================================================

		mov	dword [graph_playfield_on], ecx
		test	dword [blitend], 0ffffffffh
		js	.l14
		sub	dword [blitend], CYCLESPERFRAME
		setns	byte [blitend]
.l14:

		;==============================================================
		; Flag vertical refresh IRQ
		;==============================================================

		mov	edx, 08020h
		call	dword [memory_iobank_write + 0138h]


		;==============================================================
		; Set up next end of line event
		;==============================================================

		mov	dword [eol_next], CYCLESPERLINE
		dec	dword [eol_next]


		;==============================================================
		; Update next CPU instruction time
		;==============================================================

		mov	ebx, dword [cpu_next]
		test	ebx, ebx
		js	.l15
		sub	ebx, CYCLESPERFRAME
		mov	dword [cpu_next], ebx
.l15:

		;==============================================================
		; Update next IRQ time
		;==============================================================

		mov	ebx, dword [irq_next]
		test	ebx, ebx
		js	.l16
		sub	ebx, CYCLESPERFRAME
		mov	dword [irq_next], ebx
.l16:

		;==============================================================
		; Perform graphics driver end of frame
		;==============================================================

		pushad
		call	_gfxDrvEndOfFrame
		popad


		;==============================================================
		; Perform graphics end of frame
		;==============================================================

		call	graphEndOfFrame


		;==============================================================
		; Recalculate the entire event queue
		;==============================================================

		SCAN_EVENTS_LVL6


		;==============================================================
		; Bail out of emulation if we're asked to
		;==============================================================

		test	dword [debugging], 1
		jz	.l17
		mov	esp, dword [exceptionstack]
		jmp	bus_exit

.l17:

		pop	ebx
		pop	edx
		ret


		FALIGN32

FCODESECTIONEND
FBSSSECTIONSTART
FBSSSECTIONEND
FASMFILEEND


