;===============================================================================
; Fellow Amiga Emulator      
; Virtual Memory
;
; Author: Petter Schau (peschau@online.no)
;
; This file is under the GNU Public License (GPL)
;===============================================================================

;===============================================================================
; Summary of what is in this file:
;
; * Routines which implements read and write to Amiga memory locations.
; * Get and Set IRQ hardware registers.
; * Some wrappers for hardware register access routines implemented in C in other
;   modules. Avoided creating separate assembly files for those modules.
;===============================================================================


%include "mac/nasm.mac"
%include "mac/renaming.mac"
%include "mac/callconv.mac"
%include "generic/defs.inc"

%define FMEM_ASM

FASMFILESTART
FDATASECTIONSTART


%include "data/fmem.inc"
%include "data/floppy.inc"
%include "data/sound.inc"
%include "data/graph.inc"
%include "data/bus.inc"
%include "data/blit.inc"
%include "data/draw.inc"
	
FDATASECTIONEND
FCODESECTIONSTART

		FALIGNHELPER

global _memorystart_
_memorystart_:			; Used to check code alignment


%include "func/cpu.inc"
%include "func/sound.inc"
%include "func/draw.inc"
%include "func/graph.inc"
%include "func/copper.inc"
%include "func/blit.inc"
%include "func/cia.inc"
%include "func/floppy.inc"
%include "func/gameport.inc"
%include "func/fmem.inc"
%include "mac/fmem.mac"
%include "mac/cia.mac"
%include "mac/floppy.mac"
%include "mac/graph.mac"
%include "mac/gameport.mac"
		
		
;==============================================================================
; Fetches a byte from memory, returns byte in dl.  Takes ecx as <ea>
;==============================================================================


		FALIGN32

global _fetb_
_fetb_:		ASMCALLCONV_IN_ECX
		call	fetb
		ASMCALLCONV_OUT_EDX
		ret


		FALIGN32

global fetb
fetb:		mov	edx, ecx
		shr	edx, 16
		jmp	dword [memory_bank_readbyte + 4*edx]


;==============================================================================
; Fetches a word from memory, returns word in dx.  Takes ecx as <ea>
;==============================================================================


		FALIGN32

global _fetw_
_fetw_:		ASMCALLCONV_IN_ECX
		call	fetw
		ASMCALLCONV_OUT_EDX
		ret



		FALIGN32

global fetw
fetw:
		TEST_ODD_ACCESS	oddread
		mov	edx, ecx
		shr	edx, 16
		jmp	dword [memory_bank_readword + 4*edx]


;==============================================================================
; Fetches a long from memory, returns long in edx.  Takes ecx as <ea>
;==============================================================================


		FALIGN32


global _fetl_
_fetl_:		ASMCALLCONV_IN_ECX
		call	fetl
		ASMCALLCONV_OUT_EDX
		ret


		FALIGN32

global fetl
fetl:
		TEST_ODD_ACCESS	oddread
		mov	edx, ecx
		shr	edx, 16
		jmp	dword [memory_bank_readlong + 4*edx]

;==============================================================================
; Writes a byte to memory, takes data in dl, ea in ecx
;==============================================================================


		FALIGN32

global _wrib_
_wrib_:		ASMCALLCONV_IN_EDXECX
		call	wrib
		ASMCALLCONV_OUT_NONE
		ret


		FALIGN32

global wrib
wrib:		mov	dword [memory_wriorgadr], ecx
		shr	ecx, 16
		jmp	dword [memory_bank_writebyte + 4*ecx]


;==============================================================================
; Writes a word to memory, takes data in dx, ea in ecx
;==============================================================================


		FALIGN32

global _wriw_
_wriw_:		ASMCALLCONV_IN_EDXECX
		call	wriw
		ASMCALLCONV_OUT_NONE
		ret



		FALIGN32

global wriw
wriw:
		TEST_ODD_ACCESS	oddwrite
		mov	dword [memory_wriorgadr], ecx
		shr	ecx, 16
		jmp	dword [memory_bank_writeword + 4*ecx]


;==============================================================================
; Writes a long to memory, takes data in edx, ea in ecx
;==============================================================================

		FALIGN32

global _wril_
_wril_:		ASMCALLCONV_IN_EDXECX
		call	wril
		ASMCALLCONV_OUT_NONE
		ret



		FALIGN32

global wril
wril:
		TEST_ODD_ACCESS	oddwrite
		mov	dword [memory_wriorgadr], ecx
		shr	ecx, 16
		jmp	dword [memory_bank_writelong + 4*ecx]


;==============================================================================
; These raises exception 3 when reading an odd address
;==============================================================================


		FALIGN32

global oddread
oddread:	mov	dword [memory_fault_read] , 1
		mov	dword [memory_fault_address], ecx
		mov	ebx, 0ch
		mov	eax, 44
		jmp	_cpuPrepareException_


		FALIGN32

global oddwrite
oddwrite:	mov	dword [memory_fault_read] , 0
		mov	ecx, dword [memory_wriorgadr]
		mov	dword [memory_fault_address], ecx
		mov	ebx, 0ch
		mov	eax, 44
		jmp	_cpuPrepareException_


; ==============================================================================
; These functions are the bank functions	    
; Called from the fetX wriX functions via the mapping tables		    
; ==============================================================================

; --------- 
; Chip bank 
; --------- 


		FALIGN32

global _memoryChipReadByte_
_memoryChipReadByte_:	
		and	ecx, 0ffffffh
		movzx	edx, byte [memory_chip + ecx]
		ret


		FALIGN32

global _memoryChipReadWord_
_memoryChipReadWord_:	
		and	ecx, 0ffffffh
		xor	edx, edx
		mov	dh, byte [memory_chip + ecx]
		mov	dl, byte [memory_chip + 1 + ecx]
		ret


		FALIGN32

global _memoryChipReadLong_
_memoryChipReadLong_:
		and	ecx, 0ffffffh
		mov	edx, dword [memory_chip + ecx]
		bswap	edx
		ret

	
		FALIGN32

global _memoryChipWriteByte_
_memoryChipWriteByte_:
		mov	ecx, dword [memory_wriorgadr]
		and	ecx, 0ffffffh
		mov	byte [memory_chip + ecx], dl
		ret


		FALIGN32

global _memoryChipWriteWord_
_memoryChipWriteWord_:
		mov	ecx, dword [memory_wriorgadr]
		and	ecx, 0ffffffh
		mov	byte [memory_chip + ecx], dh
		mov	byte [memory_chip + 1 + ecx], dl
		ret


		FALIGN32

global _memoryChipWriteLong_
_memoryChipWriteLong_:
		mov	ecx, dword [memory_wriorgadr]
		and	ecx, 0ffffffh
		bswap	edx
		mov	dword [memory_chip + ecx], edx
		ret


; ---------
; Fast bank 
; ---------


		FALIGN32

global _memoryFastReadByte_
_memoryFastReadByte_:
		mov	edx, dword [memory_fast]
		and	ecx, 0ffffffh
		movzx	edx, byte [ecx + edx - 0200000h]
		ret


		FALIGN32

global _memoryFastReadWord_
_memoryFastReadWord_:	
		push	ebx
		mov	ebx, dword [memory_fast]
		and	ecx, 0ffffffh
		xor	edx, edx
		mov	dh, byte [ecx + ebx - 0200000h]
		mov	dl, byte [ecx + ebx - 01fffffh]
		pop	ebx
		ret


		FALIGN32

global _memoryFastReadLong_
_memoryFastReadLong_:	
		mov	edx, dword [memory_fast]
		and	ecx, 0ffffffh
		mov	edx, dword [ecx + edx - 0200000h]
		bswap	edx
		ret


		FALIGN32

global _memoryFastWriteByte_
_memoryFastWriteByte_:
		push	ebx
		mov	ecx, dword [memory_wriorgadr]
		and	ecx, 0ffffffh
		mov	ebx, dword [memory_fast]
		mov	byte [ecx + ebx - 0200000h], dl
		pop	ebx
		ret


		FALIGN32

global _memoryFastWriteWord_
_memoryFastWriteWord_:
		push	ebx
		mov	ecx, dword [memory_wriorgadr]
		and	ecx, 0ffffffh
		mov	ebx, dword [memory_fast]
		mov	byte [ecx + ebx - 0200000h], dh
		mov	byte [ecx + ebx - 01fffffh], dl
		pop	ebx
		ret


		FALIGN32

global _memoryFastWriteLong_
_memoryFastWriteLong_:
		push	ebx
		mov	ecx, dword [memory_wriorgadr]
		and	ecx, 0ffffffh
		mov	ebx, dword [memory_fast]
		bswap	edx
		mov	dword [ecx + ebx - 0200000h], edx
		pop	ebx
		ret


; --------- 
; Bogo bank 
; ---------


		FALIGN32

global _memoryBogoReadByte_
_memoryBogoReadByte_:
		and	ecx, 0ffffffh
		movzx	edx, byte [memory_bogo - 0c00000h + ecx]
		ret


		FALIGN32

global _memoryBogoReadWord_
_memoryBogoReadWord_:
		and	ecx, 0ffffffh
		xor	edx, edx
		mov	dh, byte [memory_bogo - 0c00000h + ecx]
		mov	dl, byte [memory_bogo - 0c00000h + 1 + ecx]
		ret


		FALIGN32

global _memoryBogoReadLong_
_memoryBogoReadLong_:
		and	ecx, 0ffffffh
		mov	edx, dword [memory_bogo - 0c00000h + ecx]
		bswap	edx
		ret


		FALIGN32

global _memoryBogoWriteByte_
_memoryBogoWriteByte_:
		mov	ecx, dword [memory_wriorgadr]
		and	ecx, 0ffffffh
		mov	byte [memory_bogo - 0c00000h + ecx], dl
		ret


		FALIGN32

global _memoryBogoWriteWord_
_memoryBogoWriteWord_:
		mov	ecx, dword [memory_wriorgadr]
		and	ecx, 0ffffffh
		mov	byte [memory_bogo - 0c00000h + ecx], dh
		mov	byte [memory_bogo - 0c00000h + 1 + ecx], dl
		ret


		FALIGN32

global _memoryBogoWriteLong_
_memoryBogoWriteLong_:
		mov	ecx, dword [memory_wriorgadr]
		and	ecx, 0ffffffh
		bswap	edx
		mov	dword [memory_bogo - 0c00000h + ecx], edx
		ret


; --------------- 
; Autoconfig bank 
; ---------------


		FALIGN32

global _memoryEmemReadByteASM_
_memoryEmemReadByteASM_:
		EMEMREADBYTE_CWRAP
		ret


		FALIGN32

global _memoryEmemReadWordASM_
_memoryEmemReadWordASM_:
		EMEMREADWORD_CWRAP
		ret


		FALIGN32

global _memoryEmemReadLongASM_
_memoryEmemReadLongASM_:
		EMEMREADLONG_CWRAP
		ret


		FALIGN32

global _memoryEmemWriteByteASM_
_memoryEmemWriteByteASM_:
		EMEMWRITEBYTE_CWRAP
		ret


		FALIGN32

global _memoryEmemWriteWordASM_
_memoryEmemWriteWordASM_:
		EMEMWRITEWORD_CWRAP
		ret


		FALIGN32

global _memoryEmemWriteLongASM_
_memoryEmemWriteLongASM_:
		EMEMWRITELONG_CWRAP
		ret


; ---------------- 
; Native trap bank 
; ----------------


		FALIGN32

global _memoryDmemReadByte_
_memoryDmemReadByte_:
		push	ecx
		and	ecx, 0ffffh
		xor	edx, edx
		mov	dl, byte [memory_dmem + ecx]		
		pop	ecx
		ret


		FALIGN32

global _memoryDmemReadWord_
_memoryDmemReadWord_:
		push	ecx
		and	ecx, 0ffffh
		xor	edx, edx
		mov	dh, byte [memory_dmem + ecx]		
		mov	dl, byte [memory_dmem + 1 + ecx]		
		pop	ecx
		ret


		FALIGN32

global _memoryDmemReadLong_
_memoryDmemReadLong_:
		push	ecx
		and	ecx, 0ffffh
		xor	edx, edx
		mov	edx, dword [memory_dmem + ecx]
		pop	ecx
		bswap	edx
		ret


		FALIGN32

global _memoryDmemWriteLongASM_
_memoryDmemWriteLongASM_:
		DMEMWRITELONG_CWRAP
		ret


; ---------
; Kick bank 
; ---------


		FALIGN32

global _memoryKickReadByte_
_memoryKickReadByte_:
		and	ecx, 0ffffffh
		movzx	edx, byte [memory_kick - 0f80000h + ecx]
		ret


		FALIGN32

global _memoryKickReadWord_
_memoryKickReadWord_:
		and	ecx, 0ffffffh
		xor	edx, edx
		mov	dh, byte [memory_kick - 0f80000h + ecx]
		mov	dl, byte [memory_kick - 0f80000h + 1 + ecx]
		ret


		FALIGN32

global _memoryKickReadLong_
_memoryKickReadLong_:
		and	ecx, 0ffffffh
		mov	edx, dword [memory_kick - 0f80000h + ecx]
		bswap	edx
		ret


		FALIGN32

global _memoryKickWriteByte_
global _memoryKickWriteWord_
global _memoryKickWriteLong_
_memoryKickWriteByte_:
_memoryKickWriteWord_:
_memoryKickWriteLong_:
		mov	ecx, dword [memory_wriorgadr]
		ret

		
; ----------------
; Unavailable bank
; ----------------


		FALIGN32

global _memoryNAReadByte_
_memoryNAReadByte_:
		not	dword [memory_mystery_value]
		mov	edx, dword [memory_mystery_value]
		ret


		FALIGN32

global _memoryNAReadWord_
_memoryNAReadWord_:
		mov	edx, 06100h
		ret
	

		FALIGN32

global _memoryNAReadLong_
_memoryNAReadLong_:
		mov	edx, 061006100h
		ret


		FALIGN32
	
global _memoryNAWriteByte_
global _memoryNAWriteWord_
global _memoryNAWriteLong_
_memoryNAWriteByte_:
_memoryNAWriteWord_:
_memoryNAWriteLong_:
		mov	ecx, dword [memory_wriorgadr]
		ret


; ----------------------
; Mystery (SCSI..?) bank
; ----------------------


		FALIGN32

global _memoryMysteryReadByte_
global _memoryMysteryReadWord_
global _memoryMysteryReadLong_
_memoryMysteryReadByte_:
_memoryMysteryReadWord_:
_memoryMysteryReadLong_:
		not	dword [memory_mystery_value]
		mov	edx, dword [memory_mystery_value]
		ret


		FALIGN32
	
global _memoryMysteryWriteByte_
global _memoryMysteryWriteWord_
global _memoryMysteryWriteLong_
_memoryMysteryWriteByte_:
_memoryMysteryWriteWord_:
_memoryMysteryWriteLong_:
		mov	ecx, dword [memory_wriorgadr]
		ret


; ---------------- 
; IO register bank  
; ---------------- 


		FALIGN32

global _memoryIOReadByte_
_memoryIOReadByte_:
		push	ecx
		and	ecx, 01feh
		call	dword [memory_iobank_read + ecx*2]
		pop	ecx
		test	ecx, 1
		jnz	rrb1
		shr	edx, 8
rrb1:		and	edx, 0ffh
		ret


		FALIGN32

global _memoryIOReadWord_
_memoryIOReadWord_:
		push	ecx
		and	ecx, 01feh
		call	dword [memory_iobank_read + ecx*2]
		pop	ecx
		ret

	
		FALIGN32

global _memoryIOReadLong_
_memoryIOReadLong_:
		push	ecx
		and	ecx, 01feh
		push	ecx
		call	dword [memory_iobank_read + ecx*2]
		pop	ecx
		push	edx
		call	dword [memory_iobank_read + 4 + ecx*2]
		pop	ecx
		shl	ecx, 16
		or	edx, ecx
		pop	ecx
		ret


; Byte write to register, well, simply clear the other
; byte and do a word write.


		FALIGN32

global _memoryIOWriteByte_
_memoryIOWriteByte_:
		mov	ecx, dword [memory_wriorgadr]
		push	ecx
		test	ecx, 1
		jnz	wrbodd
		mov	dh, dl

		and	ecx, 01feh
		and	edx, 0ff00h
		call	dword [memory_iobank_write + ecx*2]
		pop	ecx
		ret
wrbodd:
		and	ecx, 01feh
		and	edx, 0ffh
		call	dword [memory_iobank_write + ecx*2]
		pop	ecx
		ret

	
		FALIGN32

global _memoryIOWriteWord_
_memoryIOWriteWord_:
		mov	ecx, dword [memory_wriorgadr]
		push	ecx
		and	ecx, 01feh
		and	edx, 0ffffh		; No extra bits
		call	dword [memory_iobank_write + ecx*2]
		pop	ecx
		ret


		FALIGN32

global _memoryIOWriteLong_
_memoryIOWriteLong_:
		mov	ecx, dword [memory_wriorgadr]
		push	ecx
		push	edx
		and	ecx, 01feh
		push	ecx
		add	ecx, 2
		and	ecx, 01feh	; Unreal writes long to $1fe
		and	edx, 0ffffh   ; No extra bits here
		call	dword [memory_iobank_write + ecx*2]
		pop	ecx
		pop	edx
		shr	edx, 16		; No extra bits! Good!
		call	dword [memory_iobank_write + ecx*2]
		pop	ecx
		ret


; ----------------------------
; Chip read register functions	
; ---------------------------- 

; SERDATR
; $dff018

global _rserdatr_
_rserdatr_:	xor	edx, edx
		ret

; INTENAR
; $dff01c
		FALIGN32

global _rintenar_
_rintenar_:	mov	edx, dword [intenar]
		ret

; INTREQR
; $dff01e
		FALIGN32

global _rintreqr_
_rintreqr_:	mov	edx, dword [intreq]
		ret

; To simulate noise, return 0 and -1 every second time.
; Why? Bugged demos test write-only registers for various bit-values
; and to break out of loops, both 0 and 1 values must be returned.

		FALIGN32

global _rdefault_
_rdefault_:
;		mov	dword [memory_wriorgadr], ecx
;		pushad
;		call	memoryLogUndefinedIOReads
;		popad
  		inc	dword [memory_noise_counter]
		mov	edx, dword [memory_noise_counter]
		and	edx, 1
		mov	edx, dword [memory_noise + 4*edx]
		ret


; -----------------------------
; Chip write register functions	 
; ----------------------------- 

;========
; INTENA
;========
; $dff09a  - Leses fra $dff01c
; If master bit is off, then INTENA is 0, else INTENA = INTENAR
; The master bit can not be read, the memory test in the kickstart
; depends on this.
		FALIGN32

global _wintena_
_wintena_:	mov	ecx, dword [intenar]	   ; Les inn status
		test	edx, 08000h
		jz	wine1
		and	edx, 7fffh		 ; Sett bits
		or	ecx, edx
		mov	dword [intenar], ecx
		test	ecx, 00004000h
		jnz	wintenanorm
		xor	ecx, ecx
		mov	dword [intena], ecx
		ret
wintenanorm:	mov	dword [intena], ecx
		call	_cpuRaiseInterrupt_
		ret
wine1:		and	edx, 7fffh		 ; Slett bits
		not	edx
		and	ecx, edx			  ; Slett bits
		mov	dword [intenar], ecx
		test	ecx, 00004000h
		jnz	wintenanorm
		xor	ecx, ecx
		mov	dword [intena], ecx
		ret

;========
; INTREQ
;========


		FALIGN32

global _wintreq_
_wintreq_:	mov	ecx, dword [intreq]	 ; Les inn status
		test	edx, 08000h
		jz	winr1
		and	edx, 7fffh		 ; Sett bits
		or	ecx, edx
		mov	dword [intreq], ecx
		call	_cpuRaiseInterrupt_
		ret
winr1:		and	edx, 7fffh		 ; Slett bits
		not	edx
		and	ecx, edx			  ; Slett bits
		mov	dword [intreq], ecx
		ret
;========
; ADCON
;========

; $dff09e  - Leses fra $dff010
		FALIGN32

global _wadcon_
_wadcon_:	mov	ecx, dword [adcon]	; Les inn status
		test	edx, 08000h
		jz	adcr1
		and	edx, 7fffh		 ; Sett bits
		or	ecx, edx
		mov	dword [adcon], ecx
		ret
adcr1:		and	edx, 7fffh		 ; Slett bits
		not	edx
		and	ecx, edx			  ; Slett bits
		mov	dword [adcon], ecx
		ret

global _wdefault_
_wdefault_:	inc	dword [memory_undefined_io_write_counter]
;		mov	dword [memory_wriorgadr], ecx
;		pushad
;		call	memoryLogUndefinedIOWrites
;		popad
		ret



;==============================================================================
; Assembly code wrapper functions for various C-implemented memory access stubs
; These are used in the memory access tables.
; Wrapper macros exists for these calls which handles the calling convention
; to these functions, while maintaining another calling convention
; (register based) for the assembly code
; Since some of these functions are called via a jump table, hard copies 
; are found here.
;==============================================================================


;==============================================================================
; Some hardware IO-register access wrappers, calls routines in C
;==============================================================================


		FALIGN32

global _rdskbytr_
_rdskbytr_:
		RDSKBYTR_CWRAP
		ret


		FALIGN32

global _wdskpth_
_wdskpth_:
		WDSKPTH_CWRAP
		ret


		FALIGN32

global _wdskptl_
_wdskptl_:
		WDSKPTL_CWRAP
		ret


		FALIGN32

global _wdsklen_
_wdsklen_:
		WDSKLEN_CWRAP
		ret


		FALIGN32

global _wdsksync_
_wdsksync_:
		WDSKSYNC_CWRAP
		ret


		FALIGN32

global _rjoy0dat_
_rjoy0dat_:
		RJOY0DAT_CWRAP
		ret


		FALIGN32

global _rjoy1dat_
_rjoy1dat_:
		RJOY1DAT_CWRAP
		ret


		FALIGN32

global _rpot0dat_
_rpot0dat_:
		RPOT0DAT_CWRAP
		ret


		FALIGN32

global _rpot1dat_
_rpot1dat_:
		RPOT1DAT_CWRAP
		ret


		FALIGN32

global _rpotgor_
_rpotgor_:
		RPOTGOR_CWRAP
		ret


		FALIGN32

global _wjoytest_
_wjoytest_:
		WJOYTEST_CWRAP
		ret


		FALIGN32

FCODESECTIONEND
FBSSSECTIONSTART
FBSSSECTIONEND
FASMFILEEND
