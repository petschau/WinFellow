%ifndef DATA_FMEM_INC
%define DATA_FMEM_INC

;==============================================================================
; Includes data declared in the C fmem module
;==============================================================================

CDATAEXTERN memory_kickimage_none
CDATAEXTERN memory_bank_readbyte
CDATAEXTERN memory_bank_readword
CDATAEXTERN memory_bank_readlong
CDATAEXTERN memory_bank_writebyte
CDATAEXTERN memory_bank_writeword
CDATAEXTERN memory_bank_writelong
CDATAEXTERN memory_bank_pointer
CDATAEXTERN memory_bank_datapointer
CDATAEXTERN memory_iobank_read
CDATAEXTERN memory_iobank_write
CDATAEXTERN memory_chip
CDATAEXTERN memory_fast
CDATAEXTERN memory_bogo
CDATAEXTERN memory_kick
CDATAEXTERN memory_dmem
CDATAEXTERN memory_mystery_value
CDATAEXTERN memory_noise
CDATAEXTERN memory_noise_counter
CDATAEXTERN memory_undefined_io_write_counter
CDATAEXTERN memory_wriorgadr
CDATAEXTERN memory_chipsize
CDATAEXTERN memory_fastsize
CDATAEXTERN memory_bogosize
CDATAEXTERN memory_fault_read
CDATAEXTERN memory_fault_address

CDATAEXTERN adcon
CDATAEXTERN dmaconr
CDATAEXTERN dmacon
CDATAEXTERN intenar
CDATAEXTERN intena
CDATAEXTERN intreq

%endif
