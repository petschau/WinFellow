%ifndef FUNC_CPU_INC
%define FUNC_CPU_INC

;==============================================================================
; Includes functions declared in the C CPU module
;==============================================================================

%ifdef PC_PTR
CFUNCEXTERN cpu_pc_in_unmapped_mem
%endif

CFUNCEXTERN i320_C


;==============================================================================
; Includes functions declared in the asm CPU module
;==============================================================================

%ifndef ASM_CPU_ASM

extern _cpuRaiseInterrupt_
extern _cpuPrepareException_
extern _cpuSetUpInterrupt_
extern _cpuPrivilegieViolation_

%endif
%endif