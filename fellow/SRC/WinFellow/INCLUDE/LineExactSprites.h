#ifndef LINEEXACTSPRITES_H
#define LINEEXACTSPRITES_H

#include "DEFS.H"
#include "SPRITE.H"

#define SPRITE_MAX_LIST_ITEMS 275

class LineExactSprites;

typedef void(LineExactSprites::*spr_register_func)(uint16_t data, uint32_t address);

typedef struct {
  uint32_t raster_y;
  uint32_t raster_x;
  spr_register_func called_function;
  uint16_t data;
  uint32_t address;
} spr_action_list_item;

typedef struct {
  uint32_t count;
  spr_action_list_item items[SPRITE_MAX_LIST_ITEMS]; /* How many is actually needed? */
} spr_action_list_master;


typedef struct {
  uint8_t sprite_data[16];
  uint32_t sprx;
} spr_merge_list_item;

typedef struct {
  uint32_t count;
  spr_merge_list_item items[SPRITE_MAX_LIST_ITEMS]; /* How many is actually needed? */
} spr_merge_list_master;

class LineExactSprites : public Sprites
{
private:
  void aspr0pth(uint16_t data, uint32_t address);
  void aspr0ptl(uint16_t data, uint32_t address);
  void aspr1pth(uint16_t data, uint32_t address);
  void aspr1ptl(uint16_t data, uint32_t address);
  void aspr2pth(uint16_t data, uint32_t address);
  void aspr2ptl(uint16_t data, uint32_t address);
  void aspr3pth(uint16_t data, uint32_t address);
  void aspr3ptl(uint16_t data, uint32_t address);
  void aspr4pth(uint16_t data, uint32_t address);
  void aspr4ptl(uint16_t data, uint32_t address);
  void aspr5pth(uint16_t data, uint32_t address);
  void aspr5ptl(uint16_t data, uint32_t address);
  void aspr6pth(uint16_t data, uint32_t address);
  void aspr6ptl(uint16_t data, uint32_t address);
  void aspr7pth(uint16_t data, uint32_t address);
  void aspr7ptl(uint16_t data, uint32_t address);
  void asprxpos(uint16_t data, uint32_t address);
  void asprxctl(uint16_t data, uint32_t address);
  void asprxdata(uint16_t data, uint32_t address);
  void asprxdatb(uint16_t data, uint32_t address);

  uint32_t sprite_to_block;
  BOOLE output_sprite_log;
  BOOLE output_action_sprite_log;

  /* Sprite derived data */

  uint32_t sprpt_debug[8];
  uint32_t sprx[8];
  uint32_t sprx_debug[8];
  uint32_t spry[8];
  uint32_t spry_debug[8];
  uint32_t sprly[8];
  uint32_t sprly_debug[8];
  uint32_t spratt[8];
  uint16_t sprdat[8][2];
  BOOLE spr_arm_data[8];
  BOOLE spr_arm_comparator[8];

  spr_action_list_master spr_action_list[8];
  spr_action_list_master spr_dma_action_list[8];
  spr_merge_list_master spr_merge_list[8];

  static spr_register_func sprxptl_functions[8];
  static spr_register_func sprxpth_functions[8];

  STR buffer[128]; // Used for debug logging

  uint32_t sprite_state[8];
  uint32_t sprite_state_old[8];
  uint32_t sprite_16col[8];
  uint32_t sprite_online[8];
  bool sprites_online;

  /*===========================================================================*/
  /* Sprite appearance data                                                    */
  /*===========================================================================*/

  uint8_t sprite[8][16];

  typedef struct {
    spr_merge_list_master merge_list_master[8];
  } sprite_ham_slot;

  sprite_ham_slot sprite_ham_slots[313];
  uint32_t sprite_ham_slot_next;

  uint32_t sprite_write_buffer[128][2];
  uint32_t sprite_write_next;
  uint32_t sprite_write_real;

  spr_action_list_item* ActionListAddLast(spr_action_list_master* l);
  uint32_t ActionListCount(spr_action_list_master* l);
  spr_action_list_item* ActionListGet(spr_action_list_master* l, uint32_t i);
  void ActionListClear(spr_action_list_master* l);
  spr_action_list_item* ActionListAddSorted(spr_action_list_master* l, uint32_t raster_x, uint32_t raster_y);
  spr_merge_list_item* MergeListAddLast(spr_merge_list_master* l);
  uint32_t MergeListCount(spr_merge_list_master* l);
  spr_merge_list_item* MergeListGet(spr_merge_list_master* l, uint32_t i);
  void MergeListClear(spr_merge_list_master* l);

  void MergeHAM(graph_line *linedescription);
  void BuildItem(spr_action_list_item ** item);

  void Log();
  void ClearState();
  void LogActiveSprites();

  void Decode4Sprite(uint32_t sprite_number);
  void Decode16Sprite(uint32_t sprite_number);
  void SetDebugging();

  void MergeDualLoresPF2loopinfront2(graph_line* current_graph_line, uint32_t sprnr);
  void MergeDualLoresPF1loopinfront2(graph_line* current_graph_line, uint32_t sprnr);
  void MergeDualLoresPF1loopbehind2(graph_line* current_graph_line, uint32_t sprnr);
  void MergeDualLoresPF2loopbehind2(graph_line* current_graph_line, uint32_t sprnr);
  void MergeDualLoresPlayfield(graph_line* current_graph_line);
  void MergeDualHiresPF2loopinfront2(graph_line* current_graph_line, uint32_t sprnr);
  void MergeDualHiresPF1loopinfront2(graph_line* current_graph_line, uint32_t sprnr);
  void MergeDualHiresPF1loopbehind2(graph_line* current_graph_line, uint32_t sprnr);
  void MergeDualHiresPF2loopbehind2(graph_line* current_graph_line, uint32_t sprnr);
  void MergeDualHiresPlayfield(graph_line* current_graph_line);
  void MergeHires(graph_line* current_graph_line);
  void MergeLores(graph_line* current_graph_line);

  void ProcessActionListNOP();
  void ProcessDMAActionListNOP();
public:
  bool HasSpritesOnLine();

  void DMASpriteHandler();
  void ProcessActionList();

  void Merge(graph_line* current_graph_line);

  void MergeHAM2x1x16(uint32_t *frameptr, graph_line *linedescription);
  void MergeHAM2x2x16(uint32_t *frameptr, graph_line *linedescription, uint32_t nextlineoffset);
  void MergeHAM4x2x16(ULL *frameptr, graph_line *linedescription, uint32_t nextlineoffset);
  void MergeHAM4x4x16(ULL *frameptr, graph_line *linedescription, uint32_t nextlineoffset, uint32_t nextlineoffset2, uint32_t nextlineoffset3);

  void MergeHAM2x1x24(uint8_t *frameptr, graph_line *linedescription);
  void MergeHAM2x2x24(uint8_t *frameptr, graph_line *linedescription, uint32_t nextlineoffset);
  void MergeHAM4x2x24(uint8_t *frameptr, graph_line *linedescription, uint32_t nextlineoffset);
  void MergeHAM4x4x24(uint8_t *frameptr, graph_line *linedescription, uint32_t nextlineoffset, uint32_t nextlineoffset2, uint32_t nextlineoffset3);

  void MergeHAM2x1x32(ULL *frameptr, graph_line *linedescription);
  void MergeHAM2x2x32(ULL *frameptr, graph_line *linedescription, uint32_t nextlineoffset);
  void MergeHAM4x2x32(ULL *frameptr, graph_line *linedescription, uint32_t nextlineoffset);
  void MergeHAM4x4x32(ULL *frameptr, graph_line *linedescription, uint32_t nextlineoffset, uint32_t nextlineoffset2, uint32_t nextlineoffset3);

  virtual void NotifySprpthChanged(uint16_t data, unsigned int sprite_number);
  virtual void NotifySprptlChanged(uint16_t data, unsigned int sprite_number);
  virtual void NotifySprposChanged(uint16_t data, unsigned int sprite_number);
  virtual void NotifySprctlChanged(uint16_t data, unsigned int sprite_number);
  virtual void NotifySprdataChanged(uint16_t data, unsigned int sprite_number);
  virtual void NotifySprdatbChanged(uint16_t data, unsigned int sprite_number);

  virtual void HardReset();
  virtual void EndOfLine(uint32_t rasterY);
  virtual void EndOfFrame();
  virtual void EmulationStart();
  virtual void EmulationStop();

  LineExactSprites();
  virtual ~LineExactSprites();

};

#endif
