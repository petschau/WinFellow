%ifndef DATA_GRAPH_INC
%define DATA_GRAPH_INC

;==============================================================================
; Includes data declared in the C graph module
;==============================================================================

CDATAEXTERN graph_deco320hi1
CDATAEXTERN graph_deco320hi2
CDATAEXTERN graph_deco320hi3
CDATAEXTERN graph_deco320hi4
CDATAEXTERN graph_deco1
CDATAEXTERN graph_deco2
CDATAEXTERN graph_deco3
CDATAEXTERN graph_deco4
CDATAEXTERN graph_deco5
CDATAEXTERN graph_deco6
CDATAEXTERN graph_decode_line_ptr
CDATAEXTERN graph_decode_line_tab
CDATAEXTERN graph_decode_line_dual_tab
CDATAEXTERN graph_decode_nop_table
CDATAEXTERN graph_playfield_on
CDATAEXTERN graph_color_shadow
CDATAEXTERN graph_color
CDATAEXTERN graph_DDF_start
CDATAEXTERN graph_DDF_word_count
CDATAEXTERN graph_DIW_first_visible
CDATAEXTERN graph_DIW_last_visible
CDATAEXTERN graph_raster_x
CDATAEXTERN graph_raster_y
CDATAEXTERN bpl1pt
CDATAEXTERN bpl2pt
CDATAEXTERN bpl3pt
CDATAEXTERN bpl4pt
CDATAEXTERN bpl5pt
CDATAEXTERN bpl6pt
CDATAEXTERN bplcon0
CDATAEXTERN bplcon1
CDATAEXTERN bplcon2
CDATAEXTERN bpl1mod
CDATAEXTERN bpl2mod
CDATAEXTERN ddfstrt
CDATAEXTERN ddfstop
CDATAEXTERN diwxleft
CDATAEXTERN diwxright
CDATAEXTERN diwytop
CDATAEXTERN diwybottom
CDATAEXTERN oddscroll
CDATAEXTERN evenscroll
CDATAEXTERN oddhiscroll
CDATAEXTERN evenhiscroll
CDATAEXTERN lof
CDATAEXTERN diwstrt
CDATAEXTERN diwstop

; temp variables inserted by Worfje
CDATAEXTERN serper_debug_register_ecx
CDATAEXTERN serper_debug_register_edx

%ifdef USE_ASM
CDATAEXTERN graph_decode_tmp
%endif

CDATAEXTERN graph_line1_tmp
CDATAEXTERN graph_line2_tmp
CDATAEXTERN graph_allow_bpl_line_skip

CDATAEXTERN graph_frame

; graph_line types

GRAPH_LINE_BG   equ 0
GRAPH_LINE_BPL  equ 1
GRAPH_LINE_SKIP equ 2
GRAPH_LINE_BPL_SKIP equ 3

; Offsets into the graph_line struct

%define graph_line_type                         0
%define graph_line_line1			4
%define graph_line_line2			1028
%define graph_line_colors			2052
%define graph_line_DIW_first_draw		2308
%define graph_line_DIW_pixel_count		2312
%define graph_line_BG_pad_front			2316
%define graph_line_BG_pad_back			2320
%define graph_line_draw_routine			2324
%define graph_line_draw_BPL_res_routine		2328
%define graph_line_DDF_start			2332
%define graph_line_frames_left_until_BG_skip	2336
%define graph_line_sprite_ham_slot		2340
%define graph_line_bplcon2			2344
%define graph_line_reserved1			2346
%define graph_line_end				2348

%endif