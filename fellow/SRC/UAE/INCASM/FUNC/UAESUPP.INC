%ifndef FUNC_UAESUPP_INC
%define FUNC_UAESUPP_INC

;==============================================================================
; Includes functions declared in the UAE C modules
; These are added to hook up some memory access functions to the Fellow
; memory system. (Petter Schau did it.)
;==============================================================================

CFUNCEXTERN rtarea_bgetC
CFUNCEXTERN rtarea_wgetC
CFUNCEXTERN rtarea_lgetC
CFUNCEXTERN rtarea_bputC
CFUNCEXTERN rtarea_lputC
CFUNCEXTERN filesys_bgetC
CFUNCEXTERN filesys_wgetC
CFUNCEXTERN filesys_lgetC
CFUNCEXTERN filesys_bputC
CFUNCEXTERN filesys_wputC
CFUNCEXTERN filesys_lputC

%endif
