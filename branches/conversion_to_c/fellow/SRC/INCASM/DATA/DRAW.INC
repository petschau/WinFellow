%ifndef DATA_DRAW_INC
%define DATA_DRAW_INC

;==============================================================================
; Includes data declared in the C draw module
;==============================================================================

CDATAEXTERN draw_color_table
CDATAEXTERN draw_line_routine
CDATAEXTERN draw_line_BG_routine
CDATAEXTERN draw_line_BPL_manage_routine
CDATAEXTERN draw_line_BPL_res_routine
CDATAEXTERN draw_line_lores_routine
CDATAEXTERN draw_line_hires_routine
CDATAEXTERN draw_line_dual_lores_routine
CDATAEXTERN draw_line_dual_hires_routine
CDATAEXTERN draw_line_HAM_lores_routine
CDATAEXTERN draw_top
CDATAEXTERN draw_bottom
CDATAEXTERN draw_right
CDATAEXTERN draw_left

CDATAEXTERN draw_width_amiga
CDATAEXTERN draw_frame_count
CDATAEXTERN draw_frame_skip
CDATAEXTERN draw_frame_skip_factor
CDATAEXTERN draw_dual_translate
CDATAEXTERN draw_buffer_draw
CDATAEXTERN draw_buffer_show
CDATAEXTERN draw_buffer_count
CDATAEXTERN draw_deinterlace
CDATAEXTERN draw_hscale
CDATAEXTERN draw_view_scroll
CDATAEXTERN draw_buffer_top_ptr
CDATAEXTERN draw_buffer_current_ptr
CDATAEXTERN draw_HAM_modify_table
CDATAEXTERN draw_switch_bg_to_bpl
CDATAEXTERN draw_8bit_to_color

; Indexes into the HAM drawing table

%define draw_HAM_modify_table_bitindex 0
%define draw_HAM_modify_table_holdmask 4

%endif

