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
%include "data/floppy.inc"
%include "data/sound.inc"
%include "data/cia.inc"
%include "data/copper.inc"
%include "data/timer.inc"		

CDATAEXTERN blitend
CDATAEXTERN draw_frame_count;
CDATAEXTERN draw_frame_skip;
CDATAEXTERN draw_frame_skip_factor;

global _bus_blitter_delay
_bus_blitter_delay:	db	0,1,0,0

FDATASECTIONEND
FCODESECTIONSTART

		FALIGNHELPER

global _busstart_
_busstart_:	 ; Used to check code alignment

%include "generic/sound.inc"
%include "func/fmem.inc"
%include "func/cpu.inc"
%include "func/floppy.inc"
%include "func/sound.inc"
%include "func/gameport.inc"
%include "func/copper.inc"
%include "func/cia.inc"
%include "func/sprite.inc"
%include "func/kbd.inc"
%include "mac/fmem.mac"
%include "mac/kbd.mac"
%include "mac/bus.mac"
%include "mac/cpu.mac"
%include "mac/cia.mac"
%include "mac/sound.mac"
%include "mac/sprite.mac"

CFUNCEXTERN timerEndOfFrame
CFUNCEXTERN blitFinishBlit
CFUNCEXTERN busScanEventsLevel3
CFUNCEXTERN busScanEventsLevel4
CFUNCEXTERN busScanEventsLevel6
CFUNCEXTERN graphEndOfFrame

CDATAEXTERN graph_raster_x;
CDATAEXTERN graph_raster_y;
CDATAEXTERN lof;
CDATAEXTERN graph_playfield_on;

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

		test	byte [fellow_request_emulation_stop_immediately], 1
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

		test	byte [fellow_request_emulation_stop_immediately], 1
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
; Global end of frame handler
;==============================================================================

CFUNCEXTERN drawEndOfFramePreC


		FALIGN32

global _endOfFrame_
_endOfFrame_:	push	edx
		push	ebx


		;==============================================================
		; Draw the frame in the host buffer
		;==============================================================

		pushad
		push edx
		push ebx
		;call	_drawEndOfFrame_
		
		call drawEndOfFramePreC
		pop ebx
		pop edx
		popad


		;==============================================================
		; Handle keyboard events
		;==============================================================

		KBDEVENTEOFHANDLER_CWRAP


		;==============================================================
		; Reset some aspects of the graphics emulation
		;==============================================================

		xor	dword [lof], 08000h		; Short/long frame 
		xor	edx, edx
		mov	ecx, 0102h			; Zero scroll

		pushad
		push ecx
		push edx
		call	dword [memory_iobank_write + 2*ecx]
		pop edx
		pop ecx
		popad


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

		pushad
		push ecx
		push edx
		call	dword [memory_iobank_write + 0138h]
		pop edx
		pop ecx
		popad


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
		; Perform graphics end of frame
		;==============================================================

		pushad
		call	graphEndOfFrame
		call	timerEndOfFrame
		popad

		;==============================================================
		; Recalculate the entire event queue
		;==============================================================

		pushad
		call busScanEventsLevel6
		popad
		;SCAN_EVENTS_LVL6


		;==============================================================
		; Bail out of emulation if we're asked to
		;==============================================================

		test	dword [fellow_request_emulation_stop], 1
		jnz	.l18
		test	dword [fellow_request_emulation_stop_immediately], 1
		jz	.l17
.l18:		mov	esp, dword [exceptionstack]
		jmp	bus_exit

.l17:		pop	ebx
		pop	edx
		ret


		FALIGN32

FCODESECTIONEND
FBSSSECTIONSTART
FBSSSECTIONEND
FASMFILEEND


	