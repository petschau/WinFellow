#ifndef LINEEXACTSPRITES_H
#define LINEEXACTSPRITES_H

#include "DEFS.H"
#include "SPRITE.H"

#define SPRITE_MAX_LIST_ITEMS 50

class LineExactSprites;

typedef void(LineExactSprites::*spr_register_func)(UWO data, ULO address);

typedef struct {
  ULO raster_y;
  ULO raster_x;
  spr_register_func called_function;
  UWO data;
  ULO address;
} spr_action_list_item;

typedef struct {
  ULO count;
  spr_action_list_item items[SPRITE_MAX_LIST_ITEMS]; /* How many is actually needed? */
} spr_action_list_master;


typedef struct {
  UBY sprite_data[16];
  ULO sprx;
} spr_merge_list_item;

typedef struct {
  ULO count;
  spr_merge_list_item items[SPRITE_MAX_LIST_ITEMS]; /* How many is actually needed? */
} spr_merge_list_master;

class LineExactSprites : public Sprites
{
private:
  void aspr0pth(UWO data, ULO address);
  void aspr0ptl(UWO data, ULO address);
  void aspr1pth(UWO data, ULO address);
  void aspr1ptl(UWO data, ULO address);
  void aspr2pth(UWO data, ULO address);
  void aspr2ptl(UWO data, ULO address);
  void aspr3pth(UWO data, ULO address);
  void aspr3ptl(UWO data, ULO address);
  void aspr4pth(UWO data, ULO address);
  void aspr4ptl(UWO data, ULO address);
  void aspr5pth(UWO data, ULO address);
  void aspr5ptl(UWO data, ULO address);
  void aspr6pth(UWO data, ULO address);
  void aspr6ptl(UWO data, ULO address);
  void aspr7pth(UWO data, ULO address);
  void aspr7ptl(UWO data, ULO address);
  void asprxpos(UWO data, ULO address);
  void asprxctl(UWO data, ULO address);
  void asprxdata(UWO data, ULO address);
  void asprxdatb(UWO data, ULO address);

  ULO sprite_to_block;
  BOOLE output_sprite_log;
  BOOLE output_action_sprite_log;

  /* Sprite derived data */

  ULO sprpt_debug[8];
  ULO sprx[8];
  ULO sprx_debug[8];
  ULO spry[8];
  ULO spry_debug[8];
  ULO sprly[8];
  ULO sprly_debug[8];
  ULO spratt[8];
  UWO sprdat[8][2];
  BOOLE spr_arm_data[8];
  BOOLE spr_arm_comparator[8];

  spr_action_list_master spr_action_list[8];
  spr_action_list_master spr_dma_action_list[8];
  spr_merge_list_master spr_merge_list[8];

  static spr_register_func sprxptl_functions[8];
  static spr_register_func sprxpth_functions[8];

  STR buffer[128]; // Used for debug logging

  ULO sprite_state[8];
  ULO sprite_state_old[8];
  ULO sprite_16col[8];
  ULO sprite_online[8];
  bool sprites_online;

  /*===========================================================================*/
  /* Sprite appearance data                                                    */
  /*===========================================================================*/

  UBY sprite[8][16];

  typedef struct {
    spr_merge_list_master merge_list_master[8];
  } sprite_ham_slot;

  sprite_ham_slot sprite_ham_slots[313];
  ULO sprite_ham_slot_next;

  ULO sprite_write_buffer[128][2];
  ULO sprite_write_next;
  ULO sprite_write_real;

  spr_action_list_item* ActionListAddLast(spr_action_list_master* l);
  ULO ActionListCount(spr_action_list_master* l);
  spr_action_list_item* ActionListGet(spr_action_list_master* l, ULO i);
  void ActionListClear(spr_action_list_master* l);
  spr_action_list_item* ActionListAddSorted(spr_action_list_master* l, ULO raster_x, ULO raster_y);
  spr_merge_list_item* MergeListAddLast(spr_merge_list_master* l);
  ULO MergeListCount(spr_merge_list_master* l);
  spr_merge_list_item* MergeListGet(spr_merge_list_master* l, ULO i);
  void MergeListClear(spr_merge_list_master* l);

  void MergeHAM(graph_line *linedescription);
  void BuildItem(spr_action_list_item ** item);

  void Log();
  void ClearState();
  void LogActiveSprites();

  void Decode4Sprite(ULO sprite_number);
  void Decode16Sprite(ULO sprite_number);
  void SetDebugging();

  void MergeDualLoresPF2loopinfront2(graph_line* current_graph_line, ULO sprnr);
  void MergeDualLoresPF1loopinfront2(graph_line* current_graph_line, ULO sprnr);
  void MergeDualLoresPF1loopbehind2(graph_line* current_graph_line, ULO sprnr);
  void MergeDualLoresPF2loopbehind2(graph_line* current_graph_line, ULO sprnr);
  void MergeDualLoresPlayfield(graph_line* current_graph_line);
  void MergeDualHiresPF2loopinfront2(graph_line* current_graph_line, ULO sprnr);
  void MergeDualHiresPF1loopinfront2(graph_line* current_graph_line, ULO sprnr);
  void MergeDualHiresPF1loopbehind2(graph_line* current_graph_line, ULO sprnr);
  void MergeDualHiresPF2loopbehind2(graph_line* current_graph_line, ULO sprnr);
  void MergeDualHiresPlayfield(graph_line* current_graph_line);
  void MergeHires(graph_line* current_graph_line);
  void MergeLores(graph_line* current_graph_line);

public:
  bool HasSpritesOnLine();

  void DMASpriteHandler();
  void ProcessActionList();

  void Merge(graph_line* current_graph_line);
  void MergeHAM2x16(ULO *frameptr, graph_line *linedescription);
  void MergeHAM4x16(ULL *frameptr, graph_line *linedescription);
  void MergeHAM2x24(UBY *frameptr, graph_line *linedescription);
  void MergeHAM4x24(UBY *frameptr, graph_line *linedescription);
  void MergeHAM2x1x32(ULO *frameptr, graph_line *linedescription);
  void MergeHAM2x2x32(ULL *frameptr, graph_line *linedescription, ULO nextlineoffset);
  void MergeHAM4x2x32(ULL *frameptr, graph_line *linedescription, ULO nextlineoffset);
  void MergeHAM4x4x32(ULL *frameptr, graph_line *linedescription, ULO nextlineoffset, ULO nextlineoffset2, ULO nextlineoffset3);

  virtual void NotifySprpthChanged(UWO data, unsigned int sprite_number);
  virtual void NotifySprptlChanged(UWO data, unsigned int sprite_number);
  virtual void NotifySprposChanged(UWO data, unsigned int sprite_number);
  virtual void NotifySprctlChanged(UWO data, unsigned int sprite_number);
  virtual void NotifySprdataChanged(UWO data, unsigned int sprite_number);
  virtual void NotifySprdatbChanged(UWO data, unsigned int sprite_number);

  virtual void HardReset();
  virtual void EndOfLine(ULO rasterY);
  virtual void EndOfFrame();
  virtual void EmulationStart();
  virtual void EmulationStop();

  LineExactSprites();
  virtual ~LineExactSprites();

};

#endif
