%ifndef DATA_CPU_INC
%define DATA_CPU_INC

;==============================================================================
; Includes data declared in the C 68000 module
;==============================================================================

CDATAEXTERN da_regs
CDATAEXTERN stopreturnadr
CDATAEXTERN irq_changed
CDATAEXTERN cpu_speed
CDATAEXTERN cpu_type
CDATAEXTERN t
CDATAEXTERN usp
CDATAEXTERN ssp
CDATAEXTERN sr
CDATAEXTERN pc
CDATAEXTERN int2int
CDATAEXTERN exceptionstack
CDATAEXTERN exceptionbackdooraddress
CDATAEXTERN cpu_stop
CDATAEXTERN interruptflag
CDATAEXTERN interruptlevel
CDATAEXTERN interruptaddress
CDATAEXTERN bcd2hex
CDATAEXTERN hex2bcd
CDATAEXTERN statustab
CDATAEXTERN mulstab
CDATAEXTERN mulutab
CDATAEXTERN ccctt
CDATAEXTERN ccdtt
CDATAEXTERN ccett
CDATAEXTERN ccftt
CDATAEXTERN thiscycle
CDATAEXTERN irq_table
CDATAEXTERN cpu_stack_frame_gen
CDATAEXTERN ccr0
CDATAEXTERN ccr1
CDATAEXTERN cpu_next
CDATAEXTERN irq_next
CDATAEXTERN cpu_major

CDATAEXTERN sfc
CDATAEXTERN dfc
CDATAEXTERN cacr
CDATAEXTERN vbr
CDATAEXTERN caar
CDATAEXTERN msp
CDATAEXTERN eac063istab

%ifdef PC_PTR
CDATAEXTERN pcbaseadr
%endif

%ifdef PREFETCH
CDATAEXTERN prefetch_word
%endif

%endif