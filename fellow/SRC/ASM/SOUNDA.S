;===============================================================================
; Fellow Amiga Emulator
; Audio State Machine and sample stream generation
;
; Authors: Petter Schau (peschau@online.no)
;          Rainer Sinsch (sinsch@informatik.uni-frankfurt.de)
;
; This file is under the GNU Public License (GPL)
;===============================================================================

;===============================================================================
; Summary of what is in this file:
;
; * Audio hardware register get and set functions.
; * Implementation of a subset of the audio-state machine.
;   (Ignores CPU driven sound, no big loss.)
; * Implements translation and scaling of the 4 Amiga sound streams into a
;   single fixed frequency audio stream.
; * Implements 7 khz low-pass filter for the Amiga sound.
;
;===============================================================================


%include "mac/nasm.mac"
%include "mac/renaming.mac"
%include "generic/defs.inc"
	
FASMFILESTART
FDATASECTIONSTART

filter_value45:	DD 0.857270436755215389 ; 7000 Hz at 45454 Hz samplingrate
filter_value33:	DD 0.809385175167476725 ; 7000 Hz at 33100 Hz samplingrate
filter_value22:	DD 0.727523105310746957 ; 7000 Hz at 22005 Hz samplingrate
filter_value15:	DD 0.639362082983339100 ; 7000 Hz at 15650 Hz samplingrate
amplitude_div45:DD 3.5000000000
amplitude_div33:DD 2.8000000000
amplitude_div22:DD 1.9000000000
amplitude_div15:DD 1.4000000000
last_right:	DD 0.0000000000
last_left:	DD 0.0000000000

%include "data/sound.inc"
%include "data/fmem.inc"
%include "data/cia.inc"

FDATASECTIONEND
FCODESECTIONSTART

		FALIGNHELPER

global _soundStart_
_soundStart_:				; Used to check code alignment

%include "func/fmem.inc"
%include "generic/sound.inc"


;==============================================================================
; Audio IO Registers
;==============================================================================

; Shared stubs for all 4 channels, the channel number is extracted
; from ecx (destination address)

%macro GET_CHANNEL_NUMBER 0
		and	ecx, 070h
		shr	ecx, 4
		sub	ecx, 2
%endmacro


;==================
; AUDXPT
;==================
; $dff0a0,b0,c0,d0


		FALIGN32

global _waudXpth_
_waudXpth_:	GET_CHANNEL_NUMBER
		and	edx, 01fh			; Store 21 bit (2 MB)
		mov	word [audpt + 2 + 4*ecx], dx
		ret

; $dff0a2,b2,c2,d2

		FALIGN32

global _waudXptl_
_waudXptl_:	GET_CHANNEL_NUMBER
		and	edx, 0fffffffeh			; No odd adresses
		mov	word [audpt + 4*ecx], dx
		ret


;==================
; AUDXLEN
;==================
; $dff0a4,b4,c4,d4

		FALIGN32

global _waudXlen_
_waudXlen_:	GET_CHANNEL_NUMBER
		and	edx, 0ffffh
		mov	dword [audlen + 4*ecx], edx
		ret


;==================
; AUDXPER
;==================
; $dff0a6,b6,c6,d6

		FALIGN32

global _waudXper_
_waudXper_:	GET_CHANNEL_NUMBER
		and	edx, 0ffffh
		mov	edx, dword [periodtable + 4*edx]
		mov	dword [audper + 4*ecx], edx
		ret


;==================
; AUDXVOL
;==================
; $dff0a8,b8,c8,d8

; Replay routines sometimes access volume as a byte register...

		FALIGN32

global _waudXvol_
_waudXvol_:	GET_CHANNEL_NUMBER
		test	dl, dl		; Kludge, if lo is zero and hi data
		jnz	.l1		; switch, because byte access likely
		test	dh, dh
		jz	.l1
		mov	dl, dh
.l1:		and	edx, 0ffh
		test	edx, 64
		jz	.l2
		mov	edx, 63
.l2:		and	edx, 3fh
		shl	edx, 1
		mov	dword [audvol + 4*ecx], edx
		ret


;==============================================================================
; Audio state machine
; IN:  edx - channel*4
; OUT: ecx and edx - garbage
;==============================================================================


;==============================================================================
; State 0
;==============================================================================


		FALIGN32

global _soundState0_
_soundState0_:

		;-------------------
		; Statechange 0 to 1
		;-------------------

		mov	ecx, dword [audlen + edx]
		mov	dword [audlenw + edx], ecx
		mov	ecx, dword [audpt + edx]
		mov	dword [audptw + edx], ecx
		mov	dword [audpercounter + edx], 0
		mov	dword [audstate + edx], dword _soundState1_
		ret


;==============================================================================
; State 1
;==============================================================================


		FALIGN32

global _soundState1_
_soundState1_:

		;-------------------
		; Statechange 1 to 5
		;-------------------

		cmp	dword [audlenw + edx], 1 
		je	.l1
		dec	dword [audlenw + edx]
.l1:		mov	dword [audstate + edx], dword _soundState5_
		mov	edx, dword [audioirqmask + edx]
		mov	ecx, 0138h			   ; 09c * 2
		or	dh, 080h
		jmp	dword [memory_iobank_write + ecx]


;==============================================================================
; State 2
;==============================================================================


		FALIGN32

global _soundState2_
_soundState2_:	cmp	dword [audpercounter + edx], 010000h
		jb	.ss22


		;-------------------
		; Statechange 2 to 3
		;-------------------

		sub	dword [audpercounter + edx], 010000h
		mov	ecx, dword [audper + edx]
		add	dword [audpercounter + edx], ecx
		mov	ecx, dword [audvol + edx]
		mov	dword [audvolw + edx], ecx
		mov	dword [audstate + edx], dword _soundState3_
		xor	ecx,ecx
		mov	cl, byte [auddat + edx]
		shl	ecx, 7
		add	ecx, dword [audvolw + edx]
		mov	cx, word [volumes + ecx]
		mov	word [auddatw + edx], cx
		ret


		;-------------------
		; Statechange 2 to 2
		;-------------------


		FALIGN32

.ss22:		mov	ecx, dword [audper + edx]
		add	dword [audpercounter + edx], ecx
		ret


;==============================================================================
; State 2
;==============================================================================


		FALIGN32

global _soundState3_
_soundState3_:	cmp	dword [audpercounter + edx], 010000h
		jb	near .ss33


		;-------------------
		; Statechange 3 to 2
		;-------------------

		sub	dword [audpercounter + edx], 010000h
		mov	ecx, dword [audper + edx]
		add	dword [audpercounter + edx], ecx
		mov	ecx, dword [audvol + edx]
		mov	dword [audvolw + edx], ecx
		mov	dword [audstate + edx], dword _soundState2_
		xor	ecx, ecx
		mov	cl, byte [auddat + 1 + edx]
		shl	ecx, 7
		add	ecx, dword [audvolw + edx]
		mov	cx, word [volumes + ecx]
		mov	word [auddatw + edx], cx
		mov	ecx, dword [audptw + edx]
		add	dword [audptw + edx], 2
		and	dword [audptw + edx], 01fffffh
		mov	cx, word [memory_chip + ecx]
		mov	word [auddat + edx], cx
		cmp	dword [audlenw + edx], 1
		je	.ss32a
		dec	dword [audlenw + edx]
		ret

.ss32a:		mov	ecx, dword [audlen + edx]
		mov	dword [audlenw + edx], ecx
		mov	ecx, dword [audpt + edx]
		mov	dword [audptw + edx], ecx
		mov	edx, dword [audioirqmask + edx]
		mov	ecx, 0138h				; 9c*2
		or	dh, 080h
		jmp	dword [memory_iobank_write + ecx]


		;-------------------
		; Statechange 3 to 3
		;-------------------


		FALIGN32

.ss33:		mov	ecx, dword [audper + edx]
		add	dword [audpercounter + edx], ecx
		ret


;==============================================================================
; State 4 and State 6, no operation
;==============================================================================


		FALIGN32

global _soundState4_
global _soundState6_
_soundState6_:
_soundState4_:	ret


;==============================================================================
; State 5
;==============================================================================


		FALIGN32

global _soundState5_
_soundState5_:

		;-------------------
		; Statechange 5 to 2
		;-------------------

		mov	ecx, dword [audvol + edx]
		mov	dword [audvolw + edx], ecx
		mov	dword [audpercounter + edx], 0
		mov	ecx, dword [audptw + edx]
		add	dword [audptw + edx], 2
		and	dword [audptw + edx], 01ffffeh
		mov	cx, word [memory_chip + ecx]
		mov	word [auddat + edx], cx
		mov	dword [audstate + edx], dword _soundState2_
		cmp	dword [audlenw + edx], 1
		je	.ss52a
		dec	dword [audlenw + edx]
		ret

.ss52a:		mov	ecx, dword [audlen + edx]
		mov	dword [audlenw + edx], ecx
		mov	ecx, dword [audpt + edx]
		mov	dword [audptw + edx], ecx
		mov	edx, dword [audioirqmask + edx]
		mov	ecx, 0138h				 ;9c*2
		or	dh, 080h
		jmp	dword [memory_iobank_write + ecx]




;==============================================================================
; This is called by DMACON when DMA is turned off
; In: ecx - channel number
;==============================================================================

		FALIGN32

global _soundChannelKill_
_soundChannelKill_:
		mov	dword [auddatw + 4*ecx], 0
		mov	dword [audstate + 4*ecx], dword _soundState0_
		ret




;==============================================================================
; Frequency handlers:
;
; These are called by the emulation from the end_of_line function.
; It will generate samples in two arrays called _sound_left and _sound_right
; These are always 16-bit samples and is the mixed output of
; the two left channels and the two right channels.
; The number of samples they produce is dependent on the output
; frequency selected for the PC soundcard.
;
; After the production of these samples they call (if necessary)
; the audio_lowpassXX routine.
;
; When these finish, they jump to the function pointed to by
; soundSampleAddCallback, with the number of new samples in ebx
;==============================================================================

;==============================================================================
; Audio_Lowpass filters the _sound_right and sound_left memory with a
; pass1 lowpass filter at 7000 Hz
; input: bx = no. of samples
; this subroutine REQUIRES the fpu to be initialized with finit
; at startup (question is WHY?! MMX.....
; coded by Rainer Sinsch (sinsch@informatik.uni-frankfurt.de)
;==============================================================================


; 44100 hz


		FALIGN32

_soundLowPass44_:
		mov	eax, dword [sound_buffer_sample_count]
		fninit					; Init FPU, no wait
		
More_FilteringL45:
		fld	dword [last_left]

		fmul	dword [filter_value45]		; 0.844*Last_left
		fiadd	word [sound_left + 2*eax]	;   + _left
		fst	dword [last_left]		;   = Last_Left

		fdiv	dword [amplitude_div45]		;   div by 3
		fistp	word [sound_left + 2*eax]	;  = new sample

		inc	eax
		cmp	eax, ebx
		jne	More_FilteringL45

		mov	eax, dword [sound_buffer_sample_count]
More_FilteringR45:
		fld	dword [last_right]

		fmul	dword [filter_value45]		; 0.844*Last_left
		fiadd	word [sound_right + 2*eax]	;   + _left
		fst	dword [last_right]		;   = Last_Left

		fdiv	dword [amplitude_div45]		;   div by 3
		fistp	word [sound_right + 2*eax]	;  = new sample

		inc	eax
		cmp	eax, ebx
		jne	More_FilteringR45
		mov	dword [sound_buffer_sample_count], ebx
		ret


;==============================================================================
; Generate sound at 44100 hz rate
; Called in every end of line
; Will run the audio state machine the needed number of times
; and move the generated samples to a temporary buffer
;==============================================================================


		FALIGN32

global _soundFrequencyHandler44100_
_soundFrequencyHandler44100_:
		xor	ebx, ebx
		mov	eax, dword [audiocounter]
size441:	add	eax, dword [sound_scale]
		inc	ebx
		cmp	eax, 040000h
		jbe	size441
		sub	eax, 040000h
		mov	dword [audiocounter], eax

		; Here ebx is number of samples to add
		; Make ebx the position in the buffer 1 beyond where the last new sample must be put

		add	ebx, dword [sound_buffer_sample_count]

		; Here eax is the position in the buffer where the new samples must be put

		mov	eax, dword [sound_buffer_sample_count]

		mov	dword [sound_left + 2*eax], 0
		mov	dword [sound_left + 2 + 2*eax], 0
		mov	dword [sound_right + 2*eax], 0
		mov	dword [sound_right + 2 + 2*eax], 0

		test	byte [dmacon], 1	  ; Channel on?
		jz	channel0441off		  ; No, No sound 
chan0441l:	xor	edx, edx
		call	dword [audstate + edx]
		mov	ecx, dword [auddatw]
		mov	word [sound_left + 2*eax], cx
		inc	eax
		cmp	eax, ebx
		jne	chan0441l
		jmp	channel1441
channel0441off:
channel1441:	test	dword [dmacon], 2	  ; Channel on?
		jz	channel1441off		  ; No sound if dma is off

		; Here eax is the position in the buffer where the new samples must be put

		mov	eax, dword [sound_buffer_sample_count]

chan1441l:	mov	edx, 4
		call	dword [audstate + edx]
		mov	ecx, dword [auddatw + 4]
		mov	word [sound_right + 2*eax], cx
		inc	eax
		cmp	eax, ebx
		jne	chan1441l
		jmp	channel2441
channel1441off:
channel2441:	test	dword [dmacon], 4	  ; Channel on?
		jz	channel2441off		  ; No sound if dma is off

		; Here eax is the position in the buffer where the new samples must be put

		mov	eax, dword [sound_buffer_sample_count]

chan2441l:	mov	edx, 8
		call	dword [audstate + edx]
		mov	ecx, dword [auddatw + 8]
		add	word [sound_right + 2*eax], cx
		inc	eax
		cmp	eax, ebx
		jne	chan2441l
		jmp	channel3441
channel2441off:
channel3441:	test	dword [dmacon], 8	  ; Channel on?
		jz	channel3441off		  ; No sound if dma is off

		; Here eax is the position in the buffer where the new samples must be put

		mov	eax, dword [sound_buffer_sample_count]

chan3441l:	mov	edx, 12
		call	dword [audstate + edx]
		mov	ecx, dword [auddatw + 12]
		add	word [sound_left + 2*eax], cx
		inc	eax
		cmp	eax, ebx
		jne	chan3441l
		jmp	channel4441
channel3441off:
channel4441:	
		cmp	byte [sound_filter], SOUND_FILTER_NEVER
		je	filter_off45
		cmp	byte [sound_filter], SOUND_FILTER_ALWAYS
		je	near _soundLowPass44_
		test	dword [cia_pra], 2
		jz	near _soundLowPass44_
filter_off45:	mov	dword [sound_buffer_sample_count], ebx
		ret



; 33100 hz

;==============================================================================
; Audio_Lowpass filters the _sound_right and sound_left memory with a
; pass1 lowpass filter at 7000 Hz
; input: bx = no. of samples
; this subroutine REQUIRES the fpu to be initialized with finit
; at startup (question is WHY?! MMX.....
; coded by Rainer Sinsch (sinsch@informatik.uni-frankfurt.de)
;==============================================================================


		FALIGN32

_soundLowPass33_:
		mov	eax, dword [sound_buffer_sample_count]
		add	ebx, eax
		fninit					; Init FPU, no wait
		
More_FilteringL33:
		fld	dword [last_left]

		fmul	dword [filter_value33]		; 0.844*Last_left
		fiadd	word [sound_left + 2*eax]	;   + _left
		fst	dword [last_left]		;   = Last_Left

		fdiv	dword [amplitude_div33]		;   div by 3
		fistp	word [sound_left + 2*eax]	;  = new sample

		inc	eax
		cmp	eax, ebx
		jne	More_FilteringL33

		mov	eax, dword [sound_buffer_sample_count]
More_FilteringR33:
		fld	dword [last_right]

		fmul	dword [filter_value33]		; 0.844*Last_left
		fiadd	word [sound_right + 2*eax]	;   + _left
		fst	dword [last_right]		;   = Last_Left

		fdiv	dword [amplitude_div33]		;   div by 3
		fistp	word [sound_right + 2*eax]	;  = new sample

		inc	eax
		cmp	eax, ebx
		jne	More_FilteringR33
		mov	dword [sound_buffer_sample_count], ebx
		ret


;==============================================================================
; Generate sound at 31300 hz rate
; Called in every end of line
; Will run the audio state machine the needed number of times
; and move the generated samples to a temporary buffer
;==============================================================================


		FALIGN32

global _soundFrequencyHandler31300_
_soundFrequencyHandler31300_:
channel0313:	xor	edx, edx

		; Here eax is the position in the buffer where the new samples must be put

		mov	eax, dword [sound_buffer_sample_count]

		mov	dword [sound_left + 2*eax], edx
		mov	dword [sound_right + 2*eax], edx
		test	dword [dmacon], 1	  ; Channel on?
		jz	channel0313off		  ; No sound if dma is off
		call	dword [audstate + edx]
		mov	ecx, dword [auddatw] 
		mov	word [sound_left + 2*eax], cx
		xor	edx, edx
		call	dword [audstate + edx]
		mov	ecx, dword [auddatw]
		mov	word [sound_left + 2 + 2*eax], cx
		jmp	channel1313
channel0313off:
channel1313:	test	dword [dmacon], 2	  ; Channel on?
		jz	channel1313off		  ; No sound if dma is off

		; Here eax is the position in the buffer where the new samples must be put

		mov	eax, dword [sound_buffer_sample_count]

		mov	edx, 4
		call	dword [audstate + edx]
		mov	ecx, dword [auddatw + 4] 
		mov	word [sound_right + 2*eax], cx
		mov	edx, 4
		call	dword [audstate + edx]
		mov	ecx, dword [auddatw + 4]
		mov	word [sound_right + 2 + 2*eax], cx
		jmp	channel2313
channel1313off:
channel2313:	test	dword [dmacon], 4	  ; Channel on?
		jz	channel2313off		  ; No sound if dma is off

		; Here eax is the position in the buffer where the new samples must be put

		mov	eax, dword [sound_buffer_sample_count]

		mov	edx, 8
		call	dword [audstate + edx]

		mov	ecx, dword [auddatw + 8]
		add	word [sound_right + 2*eax], cx

		mov	edx, 8
		call	dword [audstate + edx]

		mov	ecx, dword [auddatw + 8]
		add	word [sound_right + 2 + 2*eax], cx
		jmp	channel3313
channel2313off:
channel3313:	test	dword [dmacon], 8	  ; Channel on?
		jz	channel3313off		  ; No sound if dma is off

		; Here eax is the position in the buffer where the new samples must be put

		mov	eax, dword [sound_buffer_sample_count]

		mov	edx, 12
		call	dword [audstate + edx]
		mov	ecx, dword [auddatw + 12] 
		add	word [sound_left + 2*eax], cx
		mov	edx, 12
		call	dword [audstate + edx]
		mov	ecx, dword [auddatw + 12]
		add	word [sound_left + 2 + 2*eax], cx
		jmp	channel4313
channel3313off:
channel4313:
		mov	ebx, 2
		cmp	byte [sound_filter], SOUND_FILTER_NEVER
		je	filter_off33
		cmp	byte [sound_filter], SOUND_FILTER_ALWAYS
		je	near _soundLowPass33_
		test	dword [cia_pra], 2
		jz	near _soundLowPass33_
filter_off33:	add	dword [sound_buffer_sample_count], ebx
		ret



; 22050 hz

;==============================================================================
; Audio_Lowpass filters the _sound_right and sound_left memory with a
; pass1 lowpass filter at 7000 Hz
; input: bx = no. of samples
; this subroutine REQUIRES the fpu to be initialized with finit
; at startup (question is WHY?! MMX.....
; coded by Rainer Sinsch (sinsch@informatik.uni-frankfurt.de)
;==============================================================================


		FALIGN32

global _soundLowPass22_
_soundLowPass22_:
		xor	eax, eax
		fninit					; Init FPU, no wait
		
More_FilteringL22:
		fld	dword [last_left]

		fmul	dword [filter_value22]		; 0.844*Last_left
		fiadd	word [sound_left + 2*eax]	;   + _left
		fst	dword [last_left]		;   = Last_Left

		fdiv	dword [amplitude_div22]		;   div by 3
		fistp	word [sound_left + 2*eax]	;  = new sample

		inc	eax
		cmp	eax, ebx
		jne	More_FilteringL22

		xor	eax, eax
More_FilteringR22:
		fld	dword [last_right]

		fmul	dword [filter_value22]		; 0.844*Last_left
		fiadd	word [sound_right + 2*eax]	;   + _left
		fst	dword [last_right]		;   = Last_Left

		fdiv	dword [amplitude_div22]		;   div by 3
		fistp	word [sound_right + 2*eax]	;  = new sample

		inc	eax
		cmp	eax, ebx
		jne	More_FilteringR22
		ret


;==============================================================================
; Generate sound at 22050 hz rate
; Called in every end of line
; Will run the audio state machine the needed number of times
; and move the generated samples to a temporary buffer
;==============================================================================



		FALIGN32

global _soundFrequencyHandler22050_
_soundFrequencyHandler22050_:
		xor	ebx, ebx		; Calculate number of samples
		mov	eax, dword [audiocounter]
size220:	add	eax, dword [sound_scale]
		inc	ebx
		cmp	eax, 040000h
		jbe	size220
		sub	eax, 040000h
		mov	dword [audiocounter], eax

		; Here ebx is number of samples to add

		push	ebx			; Save number of samples

		; Here eax is the position in the buffer where the new samples must be put

		mov	eax, dword [sound_buffer_sample_count]

		mov	dword [sound_left + 2*eax], 0
		mov	dword [sound_left + 2 + 2*eax], 0
		mov	dword [sound_right + 2*eax], 0
		mov	dword [sound_right + 2 + 2*eax], 0

		; Channel 0

		test	byte [dmacon], 1	  ; Channel on?
		jz	channel0220off		  ; No, No sound 
		mov	dl, byte [audioodd]
		push	edx
chan0220l:	xor	edx, edx
		call	dword [audstate + edx]
		inc	byte [audioodd]
		test	byte [audioodd], 1
		jnz	chan0220dont
		mov	ecx, dword [auddatw]
		mov	word [sound_left + 2*eax], cx
		inc	eax
chan0220dont:	inc	bh
		cmp	bh, bl
		jne	chan0220l
		pop	edx
		mov	byte [audioodd], dl
channel0220off:

		; Channel 1

channel1220:	test	dword [dmacon], 2	  ; Channel on?
		jz	channel1220off		  ; No sound if dma is off

		mov	eax, dword [sound_buffer_sample_count]

		xor	bh, bh
		mov	dl, byte [audioodd]
		push	edx
chan1220l:	mov	edx, 4
		call	dword [audstate + edx]
		inc	byte [audioodd]
		test	byte [audioodd], 1
		jnz	chan1220dont
		mov	ecx, dword [auddatw + 4]
		mov	word [sound_right + 2*eax], cx
		inc	eax
chan1220dont:	inc	bh
		cmp	bh, bl
		jne	chan1220l
		pop	edx
		mov	byte [audioodd], dl
channel1220off:

		; Channel 2

channel2220:	test	dword [dmacon], 4	  ; Channel on?
		jz	channel2220off		  ; No sound if dma is off

		mov	eax, dword [sound_buffer_sample_count]

		xor	bh, bh
		mov	dl, byte [audioodd]
		push	edx
chan2220l:	mov	edx, 8
		call	dword [audstate + edx]
		inc	byte [audioodd]
		test	byte [audioodd], 1
		jnz	chan2220dont
		mov	ecx, dword [auddatw + 8]
		add	word [sound_right + 2*eax], cx
		inc	eax
chan2220dont:	inc	bh
		cmp	bh, bl
		jne	chan2220l
		pop	edx
		mov	byte [audioodd], dl
channel2220off:

		; Channel 3

channel3220:	test	dword [dmacon], 8	  ; Channel on?
		jz	channel3220off		  ; No sound if dma is off

		mov	eax, dword [sound_buffer_sample_count]

		xor	bh, bh
		xor	eax, eax
		mov	dl, byte [audioodd]
		push	edx
chan3220l:	mov	edx, 12
		call	dword [audstate + edx]
		inc	byte [audioodd]
		test	byte [audioodd], 1
		jnz	chan3220dont
		mov	ecx, dword [auddatw + 12]
		add	word [sound_left + 2*eax], cx
		inc	eax
chan3220dont:	inc	bh
		cmp	bh, bl
		jne	chan3220l
		pop	edx
		mov	byte [audioodd], dl
channel3220off:
channel4220:	mov	ebx, dword [esp]	; restore number of samples
		push	eax
		mov	al, bl
		mov	ah, byte [audioodd]
		and	ah, 1
		xor	byte [audioodd], al
		add	al, ah
		shr	al, 1
		mov	bl, al
		pop	eax
		pop	ebx

		cmp	byte [sound_filter], SOUND_FILTER_NEVER
		je	filter_off22
		cmp	byte [sound_filter], SOUND_FILTER_ALWAYS
		je	near _soundLowPass22_
		test	dword [cia_pra], 2
		jz	near _soundLowPass22_
filter_off22:	ret


; 15650

;==============================================================================
; Audio_Lowpass filters the _sound_right and sound_left memory with a
; pass1 lowpass filter at 7000 Hz
; input: bx = no. of samples
; this subroutine REQUIRES the fpu to be initialized with finit
; at startup (question is WHY?! MMX.....
; coded by Rainer Sinsch (sinsch@informatik.uni-frankfurt.de)
;==============================================================================


		FALIGN32

global _soundLowPass15_
_soundLowPass15_:
		fninit					; Init FPU, no wait
		
		fld	dword [last_left]

		fmul	dword [filter_value15]		; 0.844*Last_left
		fiadd	word [sound_left]		;   + _left
		fst	dword [last_left]		;   = Last_Left

		fdiv	dword [amplitude_div15]		;   div by 3
		fistp	word [sound_left]		;  = new sample

		fld	dword [last_right]

		fmul	dword [filter_value15]		; 0.844*Last_left
		fiadd	word [sound_right]		;   + _left
		fst	dword [last_right]		;   = Last_Left

		fdiv	dword [amplitude_div15]		;   div by 3
		fistp	word [sound_right]		;  = new sample

		ret



;==============================================================================
; Generate sound at 15650 hz rate
; Called in every end of line
; Will run the audio state machine the needed number of times
; and move the generated samples to a temporary buffer
;==============================================================================


		FALIGN32

global _soundFrequencyHandler15650_
_soundFrequencyHandler15650_:
channel0156:	xor	edx, edx
		mov	dword [sound_left], edx
		mov	dword [sound_right], edx
		test	dword [dmacon], 1	  ; Channel on?
		jz	channel0156off		  ; No sound if dma is off
		call	dword [audstate + edx]
		mov	eax, dword [auddatw] 
		mov	word [sound_left], ax
		xor	edx, edx
		call	dword [audstate + edx]
		jmp	channel1156
channel0156off:
channel1156:	test	dword [dmacon], 2	  ; Channel on?
		jz	channel1156off		  ; No sound if dma is off
		mov	edx, 4
		call	dword [audstate + edx]
		mov	eax, dword [auddatw + 4] 
		mov	word [sound_right], ax
		mov	edx, 4
		call	dword [audstate + edx]
		jmp	channel2156
channel1156off:
channel2156:	test	dword [dmacon], 4	  ; Channel on?
		jz	channel2156off		  ; No sound if dma is off
		mov	edx, 8
		call	dword [audstate + edx]

		mov	eax, dword [auddatw + 8]
		add	word [sound_right], ax

		mov	edx, 8
		call	dword [audstate + edx]

		jmp	channel3156
channel2156off:
channel3156:	test	dword [dmacon], 8	  ; Channel on?
		jz	channel3156off		  ; No sound if dma is off
		mov	edx, 12
		call	dword [audstate + edx]
		mov	eax,dword [auddatw + 12] 
		add	word [sound_left], ax
		mov	edx, 12
		call	dword [audstate + edx]
		jmp	channel4156
channel3156off:
channel4156:	mov	ebx, 1
		cmp	byte [sound_filter], SOUND_FILTER_NEVER
		je	filter_off15
		cmp	byte [sound_filter], SOUND_FILTER_ALWAYS
		je	near _soundLowPass15_
		test	dword [cia_pra], 2
		jz	near _soundLowPass15_
filter_off15:	ret


		FALIGN32


FCODESECTIONEND
FBSSSECTIONSTART
FBSSSECTIONEND
FASMFILEEND

