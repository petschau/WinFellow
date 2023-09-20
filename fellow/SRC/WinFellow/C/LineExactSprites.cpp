#include "SpriteRegisters.h"
#include "SpriteP2CDecoder.h"
#include "SpriteMerger.h"
#include "LineExactSprites.h"
#include "FELLOW.H"
#include "BUS.H"
#include "FMEM.H"
#include "DRAW.H"
#include "chipset.h"
#include "CoreHost.h"

spr_register_func LineExactSprites::sprxptl_functions[8] =
{
  &LineExactSprites::aspr0ptl,
  &LineExactSprites::aspr1ptl,
  &LineExactSprites::aspr2ptl,
  &LineExactSprites::aspr3ptl,
  &LineExactSprites::aspr4ptl,
  &LineExactSprites::aspr5ptl,
  &LineExactSprites::aspr6ptl,
  &LineExactSprites::aspr7ptl
};

spr_register_func LineExactSprites::sprxpth_functions[8] =
{
  &LineExactSprites::aspr0pth,
  &LineExactSprites::aspr1pth,
  &LineExactSprites::aspr2pth,
  &LineExactSprites::aspr3pth,
  &LineExactSprites::aspr4pth,
  &LineExactSprites::aspr5pth,
  &LineExactSprites::aspr6pth,
  &LineExactSprites::aspr7pth
};

void LineExactSprites::aspr0pth(uint16_t data, uint32_t address)
{
  sprite_registers.sprpt[0] = chipsetReplaceHighPtr(sprite_registers.sprpt[0], data);
}
void LineExactSprites::aspr0ptl(uint16_t data, uint32_t address)
{
  sprite_registers.sprpt[0] = chipsetReplaceLowPtr(sprite_registers.sprpt[0], data);
}

void LineExactSprites::aspr1pth(uint16_t data, uint32_t address)
{
  sprite_registers.sprpt[1] = chipsetReplaceHighPtr(sprite_registers.sprpt[1], data);
}
void LineExactSprites::aspr1ptl(uint16_t data, uint32_t address)
{
  sprite_registers.sprpt[1] = chipsetReplaceLowPtr(sprite_registers.sprpt[1], data);
}

void LineExactSprites::aspr2pth(uint16_t data, uint32_t address)
{
  sprite_registers.sprpt[2] = chipsetReplaceHighPtr(sprite_registers.sprpt[2], data);
}
void LineExactSprites::aspr2ptl(uint16_t data, uint32_t address)
{
  sprite_registers.sprpt[2] = chipsetReplaceLowPtr(sprite_registers.sprpt[2], data);
}

void LineExactSprites::aspr3pth(uint16_t data, uint32_t address)
{
  sprite_registers.sprpt[3] = chipsetReplaceHighPtr(sprite_registers.sprpt[3], data);
}
void LineExactSprites::aspr3ptl(uint16_t data, uint32_t address)
{
  sprite_registers.sprpt[3] = chipsetReplaceLowPtr(sprite_registers.sprpt[3], data);
}

void LineExactSprites::aspr4pth(uint16_t data, uint32_t address)
{
  sprite_registers.sprpt[4] = chipsetReplaceHighPtr(sprite_registers.sprpt[4], data);
}
void LineExactSprites::aspr4ptl(uint16_t data, uint32_t address)
{
  sprite_registers.sprpt[4] = chipsetReplaceLowPtr(sprite_registers.sprpt[4], data);
}

void LineExactSprites::aspr5pth(uint16_t data, uint32_t address)
{
  sprite_registers.sprpt[5] = chipsetReplaceHighPtr(sprite_registers.sprpt[5], data);
}
void LineExactSprites::aspr5ptl(uint16_t data, uint32_t address)
{
  sprite_registers.sprpt[5] = chipsetReplaceLowPtr(sprite_registers.sprpt[5], data);
}
void LineExactSprites::aspr6pth(uint16_t data, uint32_t address)
{
  sprite_registers.sprpt[6] = chipsetReplaceHighPtr(sprite_registers.sprpt[6], data);
}
void LineExactSprites::aspr6ptl(uint16_t data, uint32_t address)
{
  sprite_registers.sprpt[6] = chipsetReplaceLowPtr(sprite_registers.sprpt[6], data);
}

void LineExactSprites::aspr7pth(uint16_t data, uint32_t address)
{
  sprite_registers.sprpt[7] = chipsetReplaceHighPtr(sprite_registers.sprpt[7], data);
}
void LineExactSprites::aspr7ptl(uint16_t data, uint32_t address)
{
  sprite_registers.sprpt[7] = chipsetReplaceLowPtr(sprite_registers.sprpt[7], data);
}

void LineExactSprites::asprxpos(uint16_t data, uint32_t address)
{
  uint32_t sprnr = (address >> 3) & 7;

  // retrieve some of the horizontal and vertical position bits
  sprx[sprnr] = (sprx[sprnr] & 0x001) | ((data & 0xff) << 1);
  spry[sprnr] = (spry[sprnr] & 0x100) | ((data & 0xff00) >> 8);
}

void LineExactSprites::asprxctl(uint16_t data, uint32_t address)
{
  uint32_t sprnr = (address >> 3) & 7;

  // retrieve the rest of the horizontal and vertical position bits
  sprx[sprnr] = (sprx[sprnr] & 0x1fe) | (data & 0x1);
  spry[sprnr] = (spry[sprnr] & 0x0ff) | ((data & 0x4) << 6);

  // attach bit only applies when having an odd sprite
  if (sprnr & 1)
  {
    spratt[sprnr & 6] = !!(data & 0x80);
  }
  spratt[sprnr] = !!(data & 0x80);
  sprly[sprnr] = ((data & 0xff00) >> 8) | ((data & 0x2) << 7);

  spr_arm_data[sprnr] = FALSE;
}

void LineExactSprites::asprxdata(uint16_t data, uint32_t address)
{
  uint32_t sprnr = (address >> 3) & 7;

  *((uint16_t *)&sprdat[sprnr]) = (uint16_t)data;

  spr_arm_data[sprnr] = TRUE;
}

void LineExactSprites::asprxdatb(uint16_t data, uint32_t address)
{
  uint32_t sprnr = (address >> 3) & 7;
  *(((uint16_t *)&sprdat[sprnr]) + 1) = data;
}

#ifdef _DEBUG
static uint32_t max_items_seen = 0;
#endif

/* Increases the item count with 1 and returns the new (uninitialized) item */
spr_action_list_item* LineExactSprites::ActionListAddLast(spr_action_list_master* l)
{
#ifdef _DEBUG
  if (max_items_seen < l->count)
  {
    max_items_seen = l->count;
  }
  if (l->count >= SPRITE_MAX_LIST_ITEMS)
  {
    fellowAddLogRequester(FELLOW_REQUESTER_TYPE_ERROR, "Failure: Exceeded max count of sprite action list items");
  }
#endif

  return &l->items[l->count++];
}

/* Returns the number of items in the list */
uint32_t LineExactSprites::ActionListCount(spr_action_list_master* l)
{
  return l->count;
}

/* Returns the list item at position i */
spr_action_list_item* LineExactSprites::ActionListGet(spr_action_list_master* l, uint32_t i)
{
  if (i >= l->count) return nullptr;
  return &l->items[i];
}

/* Clears the list */
void LineExactSprites::ActionListClear(spr_action_list_master* l)
{
  l->count = 0;
}

/* Makes room for an item in the list based on the raster x position of the action */
/* Returns the new uninitialized item. */
spr_action_list_item* LineExactSprites::ActionListAddSorted(spr_action_list_master* l, uint32_t raster_x, uint32_t raster_y)
{
  for (uint32_t i = 0; i < l->count; i++)
  {
    if (l->items[i].raster_y >= raster_y && l->items[i].raster_x > raster_x)
    {
      for (uint32_t j = l->count; j > i; --j) l->items[j] = l->items[j - 1];

#ifdef _DEBUG
      if (max_items_seen < l->count)
      {
        max_items_seen = l->count;
      }
      if (l->count >= SPRITE_MAX_LIST_ITEMS)
      {
        fellowAddLogRequester(FELLOW_REQUESTER_TYPE_ERROR, "Failure: Exceeded max count of sprite action list items");
      }
#endif

      l->count++;
      return &l->items[i];
    }
  }
  return ActionListAddLast(l);
}


/* Increases the item count with 1 and returns the new (uninitialized) item */
spr_merge_list_item* LineExactSprites::MergeListAddLast(spr_merge_list_master* l)
{
#ifdef _DEBUG
  if (max_items_seen < l->count)
  {
    max_items_seen = l->count;
  }
  if (l->count >= SPRITE_MAX_LIST_ITEMS)
  {
    fellowAddLogRequester(FELLOW_REQUESTER_TYPE_ERROR, "Failure: Exceeded max count of sprite merge list items");
  }
#endif
  return &l->items[l->count++];
}

/* Returns the number of items in the list */
uint32_t LineExactSprites::MergeListCount(spr_merge_list_master* l)
{
  return l->count;
}

/* Returns the list item at position i */
spr_merge_list_item* LineExactSprites::MergeListGet(spr_merge_list_master* l, uint32_t i)
{
  if (i >= l->count) return nullptr;
  return &l->items[i];
}

/* Clears the list */
void LineExactSprites::MergeListClear(spr_merge_list_master* l)
{
  l->count = 0;
}

/*===========================================================================*/
/* Save sprite data for later processing on HAM bitmaps                      */
/*===========================================================================*/

void LineExactSprites::MergeHAM(graph_line *linedescription)
{
  sprite_ham_slot *ham_slot = &sprite_ham_slots[sprite_ham_slot_next];
  for (uint32_t i = 0; i < 8; i++)
  {
    uint32_t merge_list_count = spr_merge_list[i].count;
    ham_slot->merge_list_master[i].count = merge_list_count;

    for (uint32_t j = 0; j < merge_list_count; j++)
    {
      ham_slot->merge_list_master[i].items[j] = spr_merge_list[i].items[j];
    }
  }

  linedescription->sprite_ham_slot = sprite_ham_slot_next;
  linedescription->has_ham_sprites_online = true;
  sprite_ham_slot_next++;
}

/*===========================================================================*/
/* Merge sprites with HAM, actual drawing                                    */
/* 16-bit pixels, 2x horisontal scale                                        */
/*===========================================================================*/

void LineExactSprites::MergeHAM2x1x16(uint32_t *frameptr, graph_line *linedescription)
{
  if (linedescription->sprite_ham_slot != 0xffffffff)
  {
    sprite_ham_slot &ham_slot = sprite_ham_slots[linedescription->sprite_ham_slot];
    uint32_t DIW_first_visible = linedescription->DIW_first_draw;
    uint32_t DIW_last_visible = DIW_first_visible + linedescription->DIW_pixel_count;

    linedescription->sprite_ham_slot = 0xffffffff;
    for (uint32_t i = 0; i < 8; i++)
    {
      spr_merge_list_master &master = ham_slot.merge_list_master[i];

      for (uint32_t j = 0; j < ham_slot.merge_list_master[i].count; j++)
      {
        spr_merge_list_item &item = master.items[j];

        if ((item.sprx < DIW_last_visible) && ((item.sprx + 16) > DIW_first_visible))
        {
          uint32_t first_visible_cylinder = item.sprx;
          uint32_t last_visible_cylinder = first_visible_cylinder + 16;

          if (first_visible_cylinder < DIW_first_visible)
          {
            first_visible_cylinder = DIW_first_visible;
          }
          if (last_visible_cylinder > DIW_last_visible)
          {
            last_visible_cylinder = DIW_last_visible;
          }
          uint8_t *spr_ptr = &(item.sprite_data[first_visible_cylinder - item.sprx]);
          /* frameptr points to the first visible HAM pixel in the framebuffer */
          uint32_t *frame_ptr = frameptr + (first_visible_cylinder - DIW_first_visible);
          int32_t pixel_count = last_visible_cylinder - first_visible_cylinder;

          while (--pixel_count >= 0)
          {
            uint8_t pixel = *spr_ptr++;
            if (pixel != 0)
            {
              uint32_t color = graph_color_shadow[pixel >> 2];
              *frame_ptr = color;
            }
            frame_ptr++;
          }
        }
      }
    }
  }
}

/*===========================================================================*/
/* Merge sprites with HAM, actual drawing                                    */
/* 16-bit pixels, 2x horisontal scale                                        */
/*===========================================================================*/

void LineExactSprites::MergeHAM2x2x16(uint32_t *frameptr, graph_line *linedescription, uint32_t nextlineoffset)
{
  if (linedescription->sprite_ham_slot != 0xffffffff)
  {
    sprite_ham_slot &ham_slot = sprite_ham_slots[linedescription->sprite_ham_slot];
    uint32_t DIW_first_visible = linedescription->DIW_first_draw;
    uint32_t DIW_last_visible = DIW_first_visible + linedescription->DIW_pixel_count;

    linedescription->sprite_ham_slot = 0xffffffff;
    for (uint32_t i = 0; i < 8; i++)
    {
      spr_merge_list_master &master = ham_slot.merge_list_master[i];

      for (uint32_t j = 0; j < ham_slot.merge_list_master[i].count; j++)
      {
        spr_merge_list_item &item = master.items[j];

        if ((item.sprx < DIW_last_visible) && ((item.sprx + 16) > DIW_first_visible))
        {
          uint32_t first_visible_cylinder = item.sprx;
          uint32_t last_visible_cylinder = first_visible_cylinder + 16;

          if (first_visible_cylinder < DIW_first_visible)
          {
            first_visible_cylinder = DIW_first_visible;
          }
          if (last_visible_cylinder > DIW_last_visible)
          {
            last_visible_cylinder = DIW_last_visible;
          }
          uint8_t *spr_ptr = &(item.sprite_data[first_visible_cylinder - item.sprx]);
          /* frameptr points to the first visible HAM pixel in the framebuffer */
          uint32_t *frame_ptr = frameptr + (first_visible_cylinder - DIW_first_visible);
          int32_t pixel_count = last_visible_cylinder - first_visible_cylinder;

          while (--pixel_count >= 0)
          {
            uint8_t pixel = *spr_ptr++;
            if (pixel != 0)
            {
              uint32_t color = graph_color_shadow[pixel >> 2];
              frame_ptr[0] = color;
              frame_ptr[nextlineoffset] = color;
            }
            frame_ptr++;
          }
        }
      }
    }
  }
}

/*===========================================================================*/
/* Merge sprites with HAM, actual drawing                                    */
/* 16-bit pixels, 4x2 scale                                                  */
/*===========================================================================*/

void LineExactSprites::MergeHAM4x2x16(uint64_t *frameptr, graph_line *linedescription, uint32_t nextlineoffset)
{
  if (linedescription->sprite_ham_slot != 0xffffffff)
  {
    sprite_ham_slot &ham_slot = sprite_ham_slots[linedescription->sprite_ham_slot];
    uint32_t DIW_first_visible = linedescription->DIW_first_draw;
    uint32_t DIW_last_visible = DIW_first_visible + linedescription->DIW_pixel_count;

    linedescription->sprite_ham_slot = 0xffffffff;
    for (uint32_t i = 0; i < 8; i++)
    {
      spr_merge_list_master &master = ham_slot.merge_list_master[i];

      for (uint32_t j = 0; j < ham_slot.merge_list_master[i].count; j++)
      {
        spr_merge_list_item &item = master.items[j];

        if ((item.sprx < DIW_last_visible) && ((item.sprx + 16) > DIW_first_visible))
        {
          uint32_t first_visible_cylinder = item.sprx;
          uint32_t last_visible_cylinder = first_visible_cylinder + 16;

          if (first_visible_cylinder < DIW_first_visible)
          {
            first_visible_cylinder = DIW_first_visible;
          }
          if (last_visible_cylinder > DIW_last_visible)
          {
            last_visible_cylinder = DIW_last_visible;
          }
          uint8_t *spr_ptr = &(item.sprite_data[first_visible_cylinder - item.sprx]);
          /* frameptr points to the first visible HAM pixel in the framebuffer */
          uint64_t *frame_ptr = frameptr + (first_visible_cylinder - DIW_first_visible);
          int32_t pixel_count = last_visible_cylinder - first_visible_cylinder;

          while (--pixel_count >= 0)
          {
            uint8_t pixel = *spr_ptr++;
            if (pixel != 0)
            {
              uint64_t color = drawMake64BitColorFrom32Bit(graph_color_shadow[pixel >> 2]);
              frame_ptr[0] = color;
              frame_ptr[nextlineoffset] = color;
            }
            frame_ptr++;
          }
        }
      }
    }
  }
}

/*===========================================================================*/
/* Merge sprites with HAM, actual drawing                                    */
/* 16-bit pixels, 4x4 scale                                                  */
/*===========================================================================*/

void LineExactSprites::MergeHAM4x4x16(uint64_t *frameptr, graph_line *linedescription, uint32_t nextlineoffset, uint32_t nextlineoffset2, uint32_t nextlineoffset3)
{
  if (linedescription->sprite_ham_slot != 0xffffffff)
  {
    sprite_ham_slot &ham_slot = sprite_ham_slots[linedescription->sprite_ham_slot];
    uint32_t DIW_first_visible = linedescription->DIW_first_draw;
    uint32_t DIW_last_visible = DIW_first_visible + linedescription->DIW_pixel_count;

    linedescription->sprite_ham_slot = 0xffffffff;
    for (uint32_t i = 0; i < 8; i++)
    {
      spr_merge_list_master &master = ham_slot.merge_list_master[i];

      for (uint32_t j = 0; j < ham_slot.merge_list_master[i].count; j++)
      {
        spr_merge_list_item &item = master.items[j];

        if ((item.sprx < DIW_last_visible) && ((item.sprx + 16) > DIW_first_visible))
        {
          uint32_t first_visible_cylinder = item.sprx;
          uint32_t last_visible_cylinder = first_visible_cylinder + 16;

          if (first_visible_cylinder < DIW_first_visible)
          {
            first_visible_cylinder = DIW_first_visible;
          }
          if (last_visible_cylinder > DIW_last_visible)
          {
            last_visible_cylinder = DIW_last_visible;
          }
          uint8_t *spr_ptr = &(item.sprite_data[first_visible_cylinder - item.sprx]);
          /* frameptr points to the first visible HAM pixel in the framebuffer */
          uint64_t *frame_ptr = frameptr + (first_visible_cylinder - DIW_first_visible);
          int32_t pixel_count = last_visible_cylinder - first_visible_cylinder;

          while (--pixel_count >= 0)
          {
            uint8_t pixel = *spr_ptr++;
            if (pixel != 0)
            {
              uint64_t color = drawMake64BitColorFrom32Bit(graph_color_shadow[pixel >> 2]);
              frame_ptr[0] = color;
              frame_ptr[nextlineoffset] = color;
              frame_ptr[nextlineoffset2] = color;
              frame_ptr[nextlineoffset3] = color;
            }
            frame_ptr++;
          }
        }
      }
    }
  }
}

union sprham24helper
{
  uint32_t color_i;
  uint8_t color_b[4];
};

/*===========================================================================*/
/* Merge sprites with HAM, actual drawing                                    */
/* 24-bit pixels, 2x horisontal scale                                        */
/*===========================================================================*/

void LineExactSprites::MergeHAM2x1x24(uint8_t *frameptr, graph_line *linedescription)
{
  if (linedescription->sprite_ham_slot != 0xffffffff)
  {
    sprite_ham_slot &ham_slot = sprite_ham_slots[linedescription->sprite_ham_slot];
    uint32_t DIW_first_visible = linedescription->DIW_first_draw;
    uint32_t DIW_last_visible = DIW_first_visible + linedescription->DIW_pixel_count;

    linedescription->sprite_ham_slot = 0xffffffff;
    for (uint32_t i = 0; i < 8; i++)
    {
      spr_merge_list_master &master = ham_slot.merge_list_master[i];

      for (uint32_t j = 0; j < ham_slot.merge_list_master[i].count; j++)
      {
        spr_merge_list_item &item = master.items[j];

        if ((item.sprx < DIW_last_visible) && ((item.sprx + 16) > DIW_first_visible))
        {
          uint32_t first_visible_cylinder = item.sprx;
          uint32_t last_visible_cylinder = first_visible_cylinder + 16;

          if (first_visible_cylinder < DIW_first_visible)
          {
            first_visible_cylinder = DIW_first_visible;
          }
          if (last_visible_cylinder > DIW_last_visible)
          {
            last_visible_cylinder = DIW_last_visible;
          }
          uint8_t *spr_ptr = &(item.sprite_data[first_visible_cylinder - item.sprx]);
          /* frameptr points to the first visible HAM pixel in the framebuffer */
          uint8_t *frame_ptr = frameptr + 6 * (first_visible_cylinder - DIW_first_visible);
          int32_t pixel_count = last_visible_cylinder - first_visible_cylinder;

          while (--pixel_count >= 0)
          {
            uint8_t pixel = *spr_ptr++;
            if (pixel != 0)
            {
              union sprham24helper color;
              color.color_i = graph_color_shadow[pixel >> 2];
              *frame_ptr++ = color.color_b[0];
              *frame_ptr++ = color.color_b[1];
              *frame_ptr++ = color.color_b[2];
              *frame_ptr++ = color.color_b[0];
              *frame_ptr++ = color.color_b[1];
              *frame_ptr++ = color.color_b[2];
            }
          }
        }
      }
    }
  }
}

/*===========================================================================*/
/* Merge sprites with HAM, actual drawing                                    */
/* 24-bit pixels, 2x horisontal scale                                        */
/*===========================================================================*/

void LineExactSprites::MergeHAM2x2x24(uint8_t *frameptr, graph_line *linedescription, uint32_t nextlineoffset)
{
  if (linedescription->sprite_ham_slot != 0xffffffff)
  {
    sprite_ham_slot &ham_slot = sprite_ham_slots[linedescription->sprite_ham_slot];
    uint32_t DIW_first_visible = linedescription->DIW_first_draw;
    uint32_t DIW_last_visible = DIW_first_visible + linedescription->DIW_pixel_count;

    linedescription->sprite_ham_slot = 0xffffffff;
    for (uint32_t i = 0; i < 8; i++)
    {
      spr_merge_list_master &master = ham_slot.merge_list_master[i];

      for (uint32_t j = 0; j < ham_slot.merge_list_master[i].count; j++)
      {
        spr_merge_list_item &item = master.items[j];

        if ((item.sprx < DIW_last_visible) && ((item.sprx + 16) > DIW_first_visible))
        {
          uint32_t first_visible_cylinder = item.sprx;
          uint32_t last_visible_cylinder = first_visible_cylinder + 16;

          if (first_visible_cylinder < DIW_first_visible)
          {
            first_visible_cylinder = DIW_first_visible;
          }
          if (last_visible_cylinder > DIW_last_visible)
          {
            last_visible_cylinder = DIW_last_visible;
          }
          uint8_t *spr_ptr = &(item.sprite_data[first_visible_cylinder - item.sprx]);
          /* frameptr points to the first visible HAM pixel in the framebuffer */
          uint8_t *frame_ptr = frameptr + 6 * (first_visible_cylinder - DIW_first_visible);
          int32_t pixel_count = last_visible_cylinder - first_visible_cylinder;

          while (--pixel_count >= 0)
          {
            uint8_t pixel = *spr_ptr++;
            if (pixel != 0)
            {
              union sprham24helper color;
              color.color_i = graph_color_shadow[pixel >> 2];
              frame_ptr[0] = color.color_b[0];
              frame_ptr[1] = color.color_b[1];
              frame_ptr[2] = color.color_b[2];
              frame_ptr[3] = color.color_b[0];
              frame_ptr[4] = color.color_b[1];
              frame_ptr[5] = color.color_b[2];

              frame_ptr[nextlineoffset] = color.color_b[0];
              frame_ptr[1 + nextlineoffset] = color.color_b[1];
              frame_ptr[2 + nextlineoffset] = color.color_b[2];
              frame_ptr[3 + nextlineoffset] = color.color_b[0];
              frame_ptr[4 + nextlineoffset] = color.color_b[1];
              frame_ptr[5 + nextlineoffset] = color.color_b[2];
            }
            frame_ptr += 6;
          }
        }
      }
    }
  }
}

/*===========================================================================*/
/* Merge sprites with HAM, actual drawing                                    */
/* 24-bit pixels, 4x2 scale                                                  */
/*===========================================================================*/

void LineExactSprites::MergeHAM4x2x24(uint8_t *frameptr, graph_line *linedescription, uint32_t nextlineoffset)
{
  if (linedescription->sprite_ham_slot != 0xffffffff)
  {
    sprite_ham_slot &ham_slot = sprite_ham_slots[linedescription->sprite_ham_slot];
    uint32_t DIW_first_visible = linedescription->DIW_first_draw;
    uint32_t DIW_last_visible = DIW_first_visible + linedescription->DIW_pixel_count;

    linedescription->sprite_ham_slot = 0xffffffff;
    for (uint32_t i = 0; i < 8; i++)
    {
      spr_merge_list_master &master = ham_slot.merge_list_master[i];

      for (uint32_t j = 0; j < ham_slot.merge_list_master[i].count; j++)
      {
        spr_merge_list_item &item = master.items[j];

        if ((item.sprx < DIW_last_visible) && ((item.sprx + 16) > DIW_first_visible))
        {
          uint32_t first_visible_cylinder = item.sprx;
          uint32_t last_visible_cylinder = first_visible_cylinder + 16;

          if (first_visible_cylinder < DIW_first_visible)
          {
            first_visible_cylinder = DIW_first_visible;
          }
          if (last_visible_cylinder > DIW_last_visible)
          {
            last_visible_cylinder = DIW_last_visible;
          }
          uint8_t *spr_ptr = &(item.sprite_data[first_visible_cylinder - item.sprx]);
          /* frameptr points to the first visible HAM pixel in the framebuffer */
          uint8_t *frame_ptr = frameptr + 12 * (first_visible_cylinder - DIW_first_visible);
          int32_t pixel_count = last_visible_cylinder - first_visible_cylinder;

          while (--pixel_count >= 0)
          {
            uint8_t pixel = *spr_ptr++;
            if (pixel != 0)
            {
              union sprham24helper color;
              color.color_i = graph_color_shadow[pixel >> 2];
              frame_ptr[0] = color.color_b[0];
              frame_ptr[1] = color.color_b[1];
              frame_ptr[2] = color.color_b[2];
              frame_ptr[3] = color.color_b[0];
              frame_ptr[4] = color.color_b[1];
              frame_ptr[5] = color.color_b[2];
              frame_ptr[6] = color.color_b[0];
              frame_ptr[7] = color.color_b[1];
              frame_ptr[8] = color.color_b[2];
              frame_ptr[9] = color.color_b[0];
              frame_ptr[10] = color.color_b[1];
              frame_ptr[11] = color.color_b[2];

              frame_ptr[nextlineoffset] = color.color_b[0];
              frame_ptr[1 + nextlineoffset] = color.color_b[1];
              frame_ptr[2 + nextlineoffset] = color.color_b[2];
              frame_ptr[3 + nextlineoffset] = color.color_b[0];
              frame_ptr[4 + nextlineoffset] = color.color_b[1];
              frame_ptr[5 + nextlineoffset] = color.color_b[2];
              frame_ptr[6 + nextlineoffset] = color.color_b[1];
              frame_ptr[7 + nextlineoffset] = color.color_b[2];
              frame_ptr[8 + nextlineoffset] = color.color_b[0];
              frame_ptr[9 + nextlineoffset] = color.color_b[1];
              frame_ptr[10 + nextlineoffset] = color.color_b[2];
              frame_ptr[11 + nextlineoffset] = color.color_b[2];
            }
            frame_ptr += 12;
          }
        }
      }
    }
  }
}

/*===========================================================================*/
/* Merge sprites with HAM, actual drawing                                    */
/* 24-bit pixels, 4x4 scale                                                  */
/*===========================================================================*/

void LineExactSprites::MergeHAM4x4x24(uint8_t *frameptr, graph_line *linedescription, uint32_t nextlineoffset, uint32_t nextlineoffset2, uint32_t nextlineoffset3)
{
  if (linedescription->sprite_ham_slot != 0xffffffff)
  {
    sprite_ham_slot &ham_slot = sprite_ham_slots[linedescription->sprite_ham_slot];
    uint32_t DIW_first_visible = linedescription->DIW_first_draw;
    uint32_t DIW_last_visible = DIW_first_visible + linedescription->DIW_pixel_count;

    linedescription->sprite_ham_slot = 0xffffffff;
    for (uint32_t i = 0; i < 8; i++)
    {
      spr_merge_list_master &master = ham_slot.merge_list_master[i];

      for (uint32_t j = 0; j < ham_slot.merge_list_master[i].count; j++)
      {
        spr_merge_list_item &item = master.items[j];

        if ((item.sprx < DIW_last_visible) && ((item.sprx + 16) > DIW_first_visible))
        {
          uint32_t first_visible_cylinder = item.sprx;
          uint32_t last_visible_cylinder = first_visible_cylinder + 16;

          if (first_visible_cylinder < DIW_first_visible)
          {
            first_visible_cylinder = DIW_first_visible;
          }
          if (last_visible_cylinder > DIW_last_visible)
          {
            last_visible_cylinder = DIW_last_visible;
          }
          uint8_t *spr_ptr = &(item.sprite_data[first_visible_cylinder - item.sprx]);
          /* frameptr points to the first visible HAM pixel in the framebuffer */
          uint8_t *frame_ptr = frameptr + 12 * (first_visible_cylinder - DIW_first_visible);
          int32_t pixel_count = last_visible_cylinder - first_visible_cylinder;

          while (--pixel_count >= 0)
          {
            uint8_t pixel = *spr_ptr++;
            if (pixel != 0)
            {
              union sprham24helper color;
              color.color_i = graph_color_shadow[pixel >> 2];
              frame_ptr[0] = color.color_b[0];
              frame_ptr[1] = color.color_b[1];
              frame_ptr[2] = color.color_b[2];
              frame_ptr[3] = color.color_b[0];
              frame_ptr[4] = color.color_b[1];
              frame_ptr[5] = color.color_b[2];
              frame_ptr[6] = color.color_b[0];
              frame_ptr[7] = color.color_b[1];
              frame_ptr[8] = color.color_b[2];
              frame_ptr[9] = color.color_b[0];
              frame_ptr[10] = color.color_b[1];
              frame_ptr[11] = color.color_b[2];

              frame_ptr[nextlineoffset] = color.color_b[0];
              frame_ptr[1 + nextlineoffset] = color.color_b[1];
              frame_ptr[2 + nextlineoffset] = color.color_b[2];
              frame_ptr[3 + nextlineoffset] = color.color_b[0];
              frame_ptr[4 + nextlineoffset] = color.color_b[1];
              frame_ptr[5 + nextlineoffset] = color.color_b[2];
              frame_ptr[6 + nextlineoffset] = color.color_b[1];
              frame_ptr[7 + nextlineoffset] = color.color_b[2];
              frame_ptr[8 + nextlineoffset] = color.color_b[0];
              frame_ptr[9 + nextlineoffset] = color.color_b[1];
              frame_ptr[10 + nextlineoffset] = color.color_b[2];
              frame_ptr[11 + nextlineoffset] = color.color_b[2];

              frame_ptr[nextlineoffset2] = color.color_b[0];
              frame_ptr[1 + nextlineoffset2] = color.color_b[1];
              frame_ptr[2 + nextlineoffset2] = color.color_b[2];
              frame_ptr[3 + nextlineoffset2] = color.color_b[0];
              frame_ptr[4 + nextlineoffset2] = color.color_b[1];
              frame_ptr[5 + nextlineoffset2] = color.color_b[2];
              frame_ptr[6 + nextlineoffset2] = color.color_b[1];
              frame_ptr[7 + nextlineoffset2] = color.color_b[2];
              frame_ptr[8 + nextlineoffset2] = color.color_b[0];
              frame_ptr[9 + nextlineoffset2] = color.color_b[1];
              frame_ptr[10 + nextlineoffset2] = color.color_b[2];
              frame_ptr[11 + nextlineoffset2] = color.color_b[2];

              frame_ptr[nextlineoffset3] = color.color_b[0];
              frame_ptr[1 + nextlineoffset3] = color.color_b[1];
              frame_ptr[2 + nextlineoffset3] = color.color_b[2];
              frame_ptr[3 + nextlineoffset3] = color.color_b[0];
              frame_ptr[4 + nextlineoffset3] = color.color_b[1];
              frame_ptr[5 + nextlineoffset3] = color.color_b[2];
              frame_ptr[6 + nextlineoffset3] = color.color_b[1];
              frame_ptr[7 + nextlineoffset3] = color.color_b[2];
              frame_ptr[8 + nextlineoffset3] = color.color_b[0];
              frame_ptr[9 + nextlineoffset3] = color.color_b[1];
              frame_ptr[10 + nextlineoffset3] = color.color_b[2];
              frame_ptr[11 + nextlineoffset3] = color.color_b[2];
            }
            frame_ptr += 12;
          }
        }
      }
    }
  }
}

/*===========================================================================*/
/* Merge sprites with HAM, actual drawing                                    */
/* 32-bit pixels, 2x horisontal scale                                        */
/*===========================================================================*/

void LineExactSprites::MergeHAM2x1x32(uint64_t *frameptr, graph_line *linedescription)
{
  if (linedescription->sprite_ham_slot != 0xffffffff)
  {
    sprite_ham_slot &ham_slot = sprite_ham_slots[linedescription->sprite_ham_slot];
    uint32_t DIW_first_visible = linedescription->DIW_first_draw;
    uint32_t DIW_last_visible = DIW_first_visible + linedescription->DIW_pixel_count;

    linedescription->sprite_ham_slot = 0xffffffff;
    for (uint32_t i = 0; i < 8; i++)
    {
      spr_merge_list_master &master = ham_slot.merge_list_master[i];

      for (uint32_t j = 0; j < ham_slot.merge_list_master[i].count; j++)
      {
        spr_merge_list_item &item = master.items[j];

        if ((item.sprx < DIW_last_visible) && ((item.sprx + 16) > DIW_first_visible))
        {
          uint32_t first_visible_cylinder = item.sprx;
          uint32_t last_visible_cylinder = first_visible_cylinder + 16;

          if (first_visible_cylinder < DIW_first_visible)
          {
            first_visible_cylinder = DIW_first_visible;
          }
          if (last_visible_cylinder > DIW_last_visible)
          {
            last_visible_cylinder = DIW_last_visible;
          }
          uint8_t *spr_ptr = &(item.sprite_data[first_visible_cylinder - item.sprx]);
          /* frameptr points to the first visible HAM pixel in the framebuffer */
          uint64_t *frame_ptr = frameptr + (first_visible_cylinder - DIW_first_visible);
          int32_t pixel_count = last_visible_cylinder - first_visible_cylinder;

          while (--pixel_count >= 0)
          {
            uint8_t pixel = *spr_ptr++;
            if (pixel != 0)
            {
              uint64_t color = drawMake64BitColorFrom32Bit(graph_color_shadow[pixel >> 2]);
              frame_ptr[0] = color;
            }
            frame_ptr++;
          }
        }
      }
    }
  }
}

/*===========================================================================*/
/* Merge sprites with HAM, actual drawing                                    */
/* 32-bit pixels, 2x horisontal scale                                        */
/*===========================================================================*/

void LineExactSprites::MergeHAM2x2x32(uint64_t *frameptr, graph_line *linedescription, uint32_t nextlineoffset)
{
  if (linedescription->sprite_ham_slot != 0xffffffff)
  {
    sprite_ham_slot &ham_slot = sprite_ham_slots[linedescription->sprite_ham_slot];
    uint32_t DIW_first_visible = linedescription->DIW_first_draw;
    uint32_t DIW_last_visible = DIW_first_visible + linedescription->DIW_pixel_count;

    linedescription->sprite_ham_slot = 0xffffffff;
    for (uint32_t i = 0; i < 8; i++)
    {
      spr_merge_list_master &master = ham_slot.merge_list_master[i];

      for (uint32_t j = 0; j < ham_slot.merge_list_master[i].count; j++)
      {
        spr_merge_list_item &item = master.items[j];

        if ((item.sprx < DIW_last_visible) && ((item.sprx + 16) > DIW_first_visible))
        {
          uint32_t first_visible_cylinder = item.sprx;
          uint32_t last_visible_cylinder = first_visible_cylinder + 16;

          if (first_visible_cylinder < DIW_first_visible)
          {
            first_visible_cylinder = DIW_first_visible;
          }
          if (last_visible_cylinder > DIW_last_visible)
          {
            last_visible_cylinder = DIW_last_visible;
          }
          uint8_t *spr_ptr = &(item.sprite_data[first_visible_cylinder - item.sprx]);
          /* frameptr points to the first visible HAM pixel in the framebuffer */
          uint64_t *frame_ptr = frameptr + (first_visible_cylinder - DIW_first_visible);
          int32_t pixel_count = last_visible_cylinder - first_visible_cylinder;

          while (--pixel_count >= 0)
          {
            uint8_t pixel = *spr_ptr++;
            if (pixel != 0)
            {
              uint64_t color = drawMake64BitColorFrom32Bit(graph_color_shadow[pixel >> 2]);
              frame_ptr[0] = color;
              frame_ptr[nextlineoffset] = color;
            }
            frame_ptr++;
          }
        }
      }
    }
  }
}

/*===========================================================================*/
/* Merge sprites with HAM, actual drawing                                    */
/* 32-bit pixels, 4x2 scale                                                  */
/*===========================================================================*/

void LineExactSprites::MergeHAM4x2x32(uint64_t *frameptr, graph_line *linedescription, uint32_t nextlineoffset)
{
  if (linedescription->sprite_ham_slot != 0xffffffff)
  {
    sprite_ham_slot &ham_slot = sprite_ham_slots[linedescription->sprite_ham_slot];
    uint32_t DIW_first_visible = linedescription->DIW_first_draw;
    uint32_t DIW_last_visible = DIW_first_visible + linedescription->DIW_pixel_count;

    linedescription->sprite_ham_slot = 0xffffffff;
    for (uint32_t i = 0; i < 8; i++)
    {
      spr_merge_list_master &master = ham_slot.merge_list_master[i];

      for (uint32_t j = 0; j < ham_slot.merge_list_master[i].count; j++)
      {
        spr_merge_list_item &item = master.items[j];

        if ((item.sprx < DIW_last_visible) && ((item.sprx + 16) > DIW_first_visible))
        {
          uint32_t first_visible_cylinder = item.sprx;
          uint32_t last_visible_cylinder = first_visible_cylinder + 16;

          if (first_visible_cylinder < DIW_first_visible)
          {
            first_visible_cylinder = DIW_first_visible;
          }
          if (last_visible_cylinder > DIW_last_visible)
          {
            last_visible_cylinder = DIW_last_visible;
          }
          uint8_t *spr_ptr = &(item.sprite_data[first_visible_cylinder - item.sprx]);
          /* frameptr points to the first visible HAM pixel in the framebuffer */
          uint64_t *frame_ptr = frameptr + 2 * (first_visible_cylinder - DIW_first_visible);
          int32_t pixel_count = last_visible_cylinder - first_visible_cylinder;

          while (--pixel_count >= 0)
          {
            uint8_t pixel = *spr_ptr++;
            if (pixel != 0)
            {
              uint64_t color = drawMake64BitColorFrom32Bit(graph_color_shadow[pixel >> 2]);
              frame_ptr[0] = color;
              frame_ptr[1] = color;
              frame_ptr[nextlineoffset] = color;
              frame_ptr[1 + nextlineoffset] = color;
            }
            frame_ptr += 2;
          }
        }
      }
    }
  }
}

/*===========================================================================*/
/* Merge sprites with HAM, actual drawing                                    */
/* 32-bit pixels, 4x4 scale                                                  */
/*===========================================================================*/

void LineExactSprites::MergeHAM4x4x32(uint64_t *frameptr, graph_line *linedescription, uint32_t nextlineoffset, uint32_t nextlineoffset2, uint32_t nextlineoffset3)
{
  if (linedescription->sprite_ham_slot != 0xffffffff)
  {
    sprite_ham_slot &ham_slot = sprite_ham_slots[linedescription->sprite_ham_slot];
    uint32_t DIW_first_visible = linedescription->DIW_first_draw;
    uint32_t DIW_last_visible = DIW_first_visible + linedescription->DIW_pixel_count;

    linedescription->sprite_ham_slot = 0xffffffff;
    for (uint32_t i = 0; i < 8; i++)
    {
      spr_merge_list_master &master = ham_slot.merge_list_master[i];

      for (uint32_t j = 0; j < ham_slot.merge_list_master[i].count; j++)
      {
        spr_merge_list_item &item = master.items[j];

        if ((item.sprx < DIW_last_visible) && ((item.sprx + 16) > DIW_first_visible))
        {
          uint32_t first_visible_cylinder = item.sprx;
          uint32_t last_visible_cylinder = first_visible_cylinder + 16;

          if (first_visible_cylinder < DIW_first_visible)
          {
            first_visible_cylinder = DIW_first_visible;
          }
          if (last_visible_cylinder > DIW_last_visible)
          {
            last_visible_cylinder = DIW_last_visible;
          }
          uint8_t *spr_ptr = &(item.sprite_data[first_visible_cylinder - item.sprx]);
          /* frameptr points to the first visible HAM pixel in the framebuffer */
          uint64_t *frame_ptr = frameptr + 2 * (first_visible_cylinder - DIW_first_visible);
          int32_t pixel_count = last_visible_cylinder - first_visible_cylinder;

          while (--pixel_count >= 0)
          {
            uint8_t pixel = *spr_ptr++;
            if (pixel != 0)
            {
              uint64_t color = drawMake64BitColorFrom32Bit(graph_color_shadow[pixel >> 2]);
              frame_ptr[0] = color;
              frame_ptr[1] = color;
              frame_ptr[nextlineoffset] = color;
              frame_ptr[1 + nextlineoffset] = color;
              frame_ptr[nextlineoffset2] = color;
              frame_ptr[1 + nextlineoffset2] = color;
              frame_ptr[nextlineoffset3] = color;
              frame_ptr[1 + nextlineoffset3] = color;
            }
            frame_ptr += 2;
          }
        }
      }
    }
  }
}

/*===========================================================================*/
/* Sprite control registers                                                  */
/*===========================================================================*/

void LineExactSprites::BuildItem(spr_action_list_item ** item)
{
  uint32_t currentX = busGetRasterX();
  if (currentX >= 18)
  {
    // Petter has put an delay in the Copper calls of 16 cycles, we need to compensate for that
    if (_core.RegisterUtility.IsHiresEnabled()) // check if hires bit is set (bit 15 of register BPLCON0)
    {
      // hires (x position is cycle position times four)
      (*item)->raster_x = (currentX - 16) * 4;
    }
    else
    {
      // lores (x position is cycle position times two)
      (*item)->raster_x = (currentX - 20) * 2;
    }
  }
  else
  {
    if (_core.RegisterUtility.IsHiresEnabled())
    {
      // hires
      (*item)->raster_x = 8;
    }
    else
    {
      // lores
      (*item)->raster_x = 4;
    }
  }
  (*item)->raster_y = busGetRasterY();
}

bool LineExactSprites::HasSpritesOnLine()
{
  return sprites_online;
}

/* Makes a log of the writes to the sprpt registers */
void LineExactSprites::NotifySprpthChanged(uint16_t data, unsigned int sprite_number)
{
  spr_action_list_item *item = ActionListAddLast(&spr_dma_action_list[sprite_number]);
  BuildItem(&item);
  item->called_function = sprxpth_functions[sprite_number];
  item->data = data;
  item->address = 0xdff120 + sprite_number * 4;

  if (output_sprite_log == TRUE)
  {
    *((uint16_t *)((uint8_t *)sprpt_debug + sprite_number * 4 + 2)) = (uint16_t)data & 0x01f;
    sprintf(buffer,
      "(y, x) = (%u, %u): call to spr%upth (sprx = %d, spry = %d, sprly = %d)\n",
      busGetRasterY(),
      2 * (busGetRasterX() - 16),
      sprite_number,
      (chipmemReadByte(sprpt_debug[sprite_number] + 1) << 1) | (chipmemReadByte(sprpt_debug[sprite_number] + 3) & 0x01),
      chipmemReadByte(sprpt_debug[sprite_number]) | ((chipmemReadByte(sprpt_debug[sprite_number] + 3) & 0x04) << 6),
      chipmemReadByte(sprpt_debug[sprite_number] + 2) | ((chipmemReadByte(sprpt_debug[sprite_number] + 3) & 0x02) << 7));
    fellowAddLog2(buffer);
  }
}

/* Makes a log of the writes to the sprpt registers */
void LineExactSprites::NotifySprptlChanged(uint16_t data, unsigned int sprite_number)
{
  spr_action_list_item *item = ActionListAddLast(&spr_dma_action_list[sprite_number]);
  BuildItem(&item);
  item->called_function = sprxptl_functions[sprite_number];
  item->data = data;
  item->address = 0xdff122 + sprite_number * 4;

  if (output_sprite_log == TRUE)
  {
    *((uint16_t *)((uint8_t *)sprpt_debug + sprite_number * 4 + 2)) = (uint16_t)data & 0x01f;
    sprintf(buffer,
      "(y, x) = (%u, %u): call to spr%upth (sprx = %d, spry = %d, sprly = %d)\n",
      busGetRasterY(),
      2 * (busGetRasterX() - 16),
      sprite_number,
      (chipmemReadByte(sprpt_debug[sprite_number] + 1) << 1) | (chipmemReadByte(sprpt_debug[sprite_number] + 3) & 0x01),
      chipmemReadByte(sprpt_debug[sprite_number]) | ((chipmemReadByte(sprpt_debug[sprite_number] + 3) & 0x04) << 6),
      chipmemReadByte(sprpt_debug[sprite_number] + 2) | ((chipmemReadByte(sprpt_debug[sprite_number] + 3) & 0x02) << 7));
    fellowAddLog2(buffer);
  }
}

/* SPRXPOS - $dff140 to $dff178 */

void LineExactSprites::NotifySprposChanged(uint16_t data, unsigned int sprite_number)
{
  spr_action_list_item * item = ActionListAddLast(&spr_action_list[sprite_number]);
  BuildItem(&item);
  item->called_function = &LineExactSprites::asprxpos;
  item->data = data;
  item->address = 0xdff140 + sprite_number * 8;

  // for debugging only
  sprx_debug[sprite_number] = (sprx_debug[sprite_number] & 0x001) | ((data & 0xff) << 1);
  spry_debug[sprite_number] = (spry_debug[sprite_number] & 0x100) | ((data & 0xff00) >> 8);
  if (output_sprite_log == TRUE)
  {
    sprintf(buffer, "(y, x) = (%u, %u): call to spr%upos (sprx = %u, spry = %u)\n", busGetRasterY(), 2 * (busGetRasterX() - 16), sprite_number, sprx_debug[sprite_number], sprly_debug[sprite_number]);
    fellowAddLog2(buffer);
  }
}

/* SPRXCTL $dff142 to $dff17a */

void LineExactSprites::NotifySprctlChanged(uint16_t data, unsigned int sprite_number)
{
  spr_action_list_item * item = ActionListAddLast(&spr_action_list[sprite_number]);
  BuildItem(&item);
  item->called_function = &LineExactSprites::asprxctl;
  item->data = data;
  item->address = 0xdff142 + sprite_number * 8;

  // for debugging only
  sprx_debug[sprite_number] = (sprx_debug[sprite_number] & 0x1fe) | (data & 0x1);
  spry_debug[sprite_number] = (spry_debug[sprite_number] & 0x0ff) | ((data & 0x4) << 6);
  sprly_debug[sprite_number] = ((data & 0xff00) >> 8) | ((data & 0x2) << 7);
  if (output_sprite_log == TRUE)
  {
    sprintf(buffer, "(y, x) = (%u, %u): call to spr%uctl (sprx = %u, spry = %u, sprly = %u)\n", busGetRasterY(), 2 * (busGetRasterX() - 16), sprite_number, sprx_debug[sprite_number], spry_debug[sprite_number], sprly_debug[sprite_number]);
    fellowAddLog2(buffer);
  }
}

/* SPRXDATA $dff144 to $dff17c */

void LineExactSprites::NotifySprdataChanged(uint16_t data, unsigned int sprite_number)
{
  spr_action_list_item *item = ActionListAddLast(&spr_action_list[sprite_number]);
  BuildItem(&item);
  item->called_function = &LineExactSprites::asprxdata;
  item->data = data;
  item->address = 0xdff144 + sprite_number * 8;

  // for debugging only
  if (output_sprite_log == TRUE)
  {
    sprintf(buffer, "(y, x) = (%u, %u): call to spr%udata\n", busGetRasterY(), 2 * (busGetRasterX() - 16), sprite_number);
    fellowAddLog2(buffer);
  }
}

void LineExactSprites::NotifySprdatbChanged(uint16_t data, unsigned int sprite_number)
{
  spr_action_list_item *item = ActionListAddLast(&spr_action_list[sprite_number]);
  BuildItem(&item);
  item->called_function = &LineExactSprites::asprxdatb;
  item->data = data;
  item->address = 0xdff146 + sprite_number * 8;

  // for debugging only
  if (output_sprite_log == TRUE)
  {
    sprintf(buffer, "(y, x) = (%u, %u): call to spr%udatb\n", busGetRasterY(), 2 * (busGetRasterX() - 16), sprite_number);
    fellowAddLog2(buffer);
  }
}

/*===========================================================================*/
/* Sprite decode, C-version                                                  */
/*===========================================================================*/

typedef enum {
  SPRITE_STATE_CONTROL_WORDS = 0,
  SPRITE_STATE_WAITING_FOR_FIRST_LINE = 1,
  SPRITE_STATE_
} sprite_states;

void LineExactSprites::Log()
{
  for (uint32_t no = 0; no < 8; no++)
  {
    char s[80];
    sprintf(s, "%u %u, sprite %u fy %u ly %u x %u state %u att %u atto %u pt %.6X\n", draw_frame_count,
      busGetRasterY(),
      no,
      spry[no],
      sprly[no],
      sprx[no],
      sprite_state[no],
      spratt[no & 6],
      spratt[no | 1],
      sprite_registers.sprpt[no]);
    fellowAddLog(s);
  }
}

/*===========================================================================*/
/* Called on emulation hard reset                                            */
/*===========================================================================*/

void LineExactSprites::ClearState()
{
  for (uint32_t i = 0; i < 8; i++)
  {
    sprpt_debug[i] = 0;
    sprx[i] = 0;
    spry[i] = 0;
    sprly[i] = 0;
    spratt[i] = 0;
    spr_arm_data[i] = FALSE;
    spr_arm_comparator[i] = FALSE;
    for (uint32_t j = 0; j < 2; j++)
    {
      sprdat[i][j];
    }
    sprite_state[i] = 0;
    sprite_state_old[i] = 0;
    sprite_16col[i] = FALSE;
    sprite_online[i] = FALSE;
    for (uint32_t j = 0; j < 16; j++)
    {
      sprite[i][j];
    }
  }
  sprites_online = false;
  for (uint32_t i = 0; i < 128; i++)
  {
    for (uint32_t j = 0; j < 2; j++)
    {
      sprite_write_buffer[i][j];
    }
  }
  sprite_write_next = 0;
  sprite_write_real = 0;

}

void LineExactSprites::LogActiveSprites()
{
  char s[128];
  for (int i = 0; i < 8; i++)
  {
    if (sprite_online[i])
    {
      sprintf(s, "Sprite %d position %u\n", i, sprx[i]);
      fellowAddLog(s);
    }
  }
}

void LineExactSprites::Decode4Sprite(uint32_t sprite_number)
{
  spr_merge_list_item *item = MergeListAddLast(&spr_merge_list[sprite_number]);
  item->sprx = sprx[sprite_number];
  uint32_t *chunky_destination = (uint32_t *)(item->sprite_data);

  SpriteP2CDecoder::Decode4(sprite_number, chunky_destination, sprdat[sprite_number][0], sprdat[sprite_number][1]);
}

void LineExactSprites::Decode16Sprite(uint32_t sprite_number)
{
  spr_merge_list_item *item = MergeListAddLast(&spr_merge_list[sprite_number]);
  item->sprx = sprx[sprite_number];
  uint32_t *chunky_destination = (uint32_t *)(item->sprite_data);

  SpriteP2CDecoder::Decode16(chunky_destination, sprdat[sprite_number & 0xfe][0], sprdat[sprite_number & 0xfe][1], sprdat[sprite_number][0], sprdat[sprite_number][1]);
}

void LineExactSprites::ProcessDMAActionListNOP()
{
  for (uint32_t sprnr = 0; sprnr < 8; sprnr++)
  {
    uint32_t count = ActionListCount(&spr_dma_action_list[sprnr]);
    for (uint32_t i = 0; i < count; i++)
    {
      spr_action_list_item * action_item = ActionListGet(&spr_dma_action_list[sprnr], i);
      // we can execute the coming action item
      (this->*(action_item->called_function))(action_item->data, action_item->address);
    }
    ActionListClear(&spr_dma_action_list[sprnr]);
  }
}

void LineExactSprites::DMASpriteHandler()
{
  spr_action_list_item * dma_action_item;
  spr_action_list_item * item;
  uint32_t local_sprx;
  uint32_t local_spry;
  uint32_t local_sprly;
  uint32_t local_data_ctl;
  uint32_t local_data_pos;
  uint32_t i, count;
  uint32_t currentY = busGetRasterY();

  sprites_online = false;
  uint32_t sprnr = 0;

  while (sprnr < 8)
  {
    switch (sprite_state[sprnr])
    {
    case 0:
      count = ActionListCount(&spr_dma_action_list[sprnr]);
      // inactive (waiting for a write to sprxptl)
      for (i = 0; i < count; i++)
      {
        // move through DMA action list
        dma_action_item = ActionListGet(&spr_dma_action_list[sprnr], i);
        // check if item is a write to register sprxptl
        if (dma_action_item->called_function == sprxptl_functions[sprnr])
        {
          // a write to sprxptl was made during this line, execute it now
          (this->*(dma_action_item->called_function))(dma_action_item->data, dma_action_item->address);

          // data from sprxpos
          local_data_pos = chipmemReadWord(sprite_registers.sprpt[sprnr]);
          local_spry = ((local_data_pos & 0xff00) >> 8);
          local_sprx = ((local_data_pos & 0xff) << 1);

          // data from sprxctl
          local_data_ctl = chipmemReadWord(sprite_registers.sprpt[sprnr] + 2);
          local_sprx = (local_sprx & 0x1fe) | (local_data_ctl & 0x1);
          local_spry = (local_spry & 0x0ff) | ((local_data_ctl & 0x4) << 6);
          local_sprly = ((local_data_ctl & 0xff00) >> 8) | ((local_data_ctl & 0x2) << 7);

          if (((local_spry < local_sprly)) || ((local_data_ctl == 0) && (local_data_pos == 0)))
          {
            // insert a write to sprxpos at time raster_x
            item = ActionListAddSorted(&spr_action_list[sprnr], dma_action_item->raster_x, dma_action_item->raster_y);

            item->raster_x = dma_action_item->raster_x;
            item->raster_y = dma_action_item->raster_y;
            item->called_function = &LineExactSprites::asprxpos;
            item->data = chipmemReadWord(sprite_registers.sprpt[sprnr]);
            item->address = sprnr << 3;

            // insert a write to sprxctl at time raster_x
            item = ActionListAddSorted(&spr_action_list[sprnr], dma_action_item->raster_x, dma_action_item->raster_y);
            item->raster_x = dma_action_item->raster_x;
            item->raster_y = dma_action_item->raster_y;
            item->called_function = &LineExactSprites::asprxctl;
            item->data = chipmemReadWord(sprite_registers.sprpt[sprnr] + 2);
            item->address = sprnr << 3;
          }

          if ((local_spry < local_sprly) && (local_sprx > 40))
          {
            // point to next two data words
            sprite_registers.sprpt[sprnr] = chipsetMaskPtr(sprite_registers.sprpt[sprnr] + 4);
          }

          sprite_state[sprnr] = 1;
        }

        // check if item is a write to register sprxpth
        if (dma_action_item->called_function == sprxpth_functions[sprnr])
        {
          // update the sprxpth
          (this->*(dma_action_item->called_function))(dma_action_item->data, dma_action_item->address);
        }
      }
      ActionListClear(&spr_dma_action_list[sprnr]);
      break;

    case 1:
      // waiting for (graph_raster_y == spry)
      if ((currentY >= spry[sprnr]) && ((currentY < sprly[sprnr]) || (currentY == spry[sprnr] && spry[sprnr] == sprly[sprnr])))
      {
        // we can start to display the first line of the sprite

        // insert a write to sprxdatb 
        item = ActionListAddSorted(&spr_action_list[sprnr], 60, currentY);
        item->raster_x = 60;
        item->raster_y = currentY;
        item->called_function = &LineExactSprites::asprxdatb;
        item->data = chipmemReadWord(sprite_registers.sprpt[sprnr] + 2);
        item->address = sprnr << 3;

        // insert a write to sprxdata 
        item = ActionListAddSorted(&spr_action_list[sprnr], 61, currentY);
        item->raster_x = 61;
        item->raster_y = currentY;
        item->called_function = &LineExactSprites::asprxdata;
        item->data = chipmemReadWord(sprite_registers.sprpt[sprnr]);
        item->address = sprnr << 3;

        // point to next two data words
        sprite_registers.sprpt[sprnr] = chipsetMaskPtr(sprite_registers.sprpt[sprnr] + 4);

        // we move to state 2 to wait for the last line sprly
        sprite_state[sprnr] = 2;
      }

      // handle writes to sprxptl and sprxpth
      count = ActionListCount(&spr_dma_action_list[sprnr]);
      for (i = 0; i < count; i++)
      {
        // move through DMA action list
        dma_action_item = ActionListGet(&spr_dma_action_list[sprnr], i);
        // check if item is a write to register sprxptl
        if (dma_action_item->called_function == sprxptl_functions[sprnr])
        {
          // a write to sprxptl was made during this line, execute it now
          (this->*(dma_action_item->called_function))(dma_action_item->data, dma_action_item->address);
        }

        // check if item is a write to register sprxpth
        if (dma_action_item->called_function == sprxpth_functions[sprnr])
        {
          // update the sprxpth
          (this->*(dma_action_item->called_function))(dma_action_item->data, dma_action_item->address);
        }
      }
      ActionListClear(&spr_dma_action_list[sprnr]);
      break;

    case 2:
      // waiting for (graph_raster_y == sprly)
      if (currentY >= sprly[sprnr])
      {
        // we interpret the next two data words as the next two control words
        local_data_ctl = chipmemReadWord(sprite_registers.sprpt[sprnr]);
        local_spry = ((local_data_ctl & 0xff00) >> 8);
        local_data_pos = chipmemReadWord(sprite_registers.sprpt[sprnr] + 2);
        local_spry = (local_spry & 0x0ff) | ((local_data_pos & 0x4) << 6);
        local_sprly = ((local_data_pos & 0xff00) >> 8) | ((local_data_pos & 0x2) << 7);

        // insert a write to sprxpos at time raster_x
        item = ActionListAddSorted(&spr_action_list[sprnr], 0, currentY);
        item->raster_x = 0;
        item->raster_y = currentY;
        item->called_function = &LineExactSprites::asprxpos;
        item->data = chipmemReadWord(sprite_registers.sprpt[sprnr]);
        item->address = sprnr << 3;

        // insert a write to sprxctl at time raster_x
        item = ActionListAddSorted(&spr_action_list[sprnr], 1, currentY);
        item->raster_x = 1;
        item->raster_y = currentY;
        item->called_function = &LineExactSprites::asprxctl;
        item->data = chipmemReadWord(sprite_registers.sprpt[sprnr] + 2);
        item->address = sprnr << 3;

        if (local_spry < local_sprly)
        {
          // point to next two data words
          sprite_registers.sprpt[sprnr] = chipsetMaskPtr(sprite_registers.sprpt[sprnr] + 4);
        }

        sprite_state[sprnr] = 1;
      }
      else
      {
        // we can continue to display the next line of the sprite
        // insert a write to sprxdatb 
        item = ActionListAddSorted(&spr_action_list[sprnr], 60, currentY);
        item->raster_x = 60;
        item->raster_y = currentY;
        item->called_function = &LineExactSprites::asprxdatb;
        item->data = chipmemReadWord(sprite_registers.sprpt[sprnr] + 2);
        item->address = sprnr << 3;

        // insert a write to sprxdata 
        item = ActionListAddSorted(&spr_action_list[sprnr], 61, currentY);
        item->raster_x = 61;
        item->raster_y = currentY;
        item->called_function = &LineExactSprites::asprxdata;
        item->data = chipmemReadWord(sprite_registers.sprpt[sprnr]);
        item->address = sprnr << 3;

        // point to next two data words
        sprite_registers.sprpt[sprnr] = chipsetMaskPtr(sprite_registers.sprpt[sprnr] + 4);
      }

      // handle writes to sprxptl and sprxpth
      count = ActionListCount(&spr_dma_action_list[sprnr]);
      for (i = 0; i < count; i++)
      {
        // move through DMA action list
        dma_action_item = ActionListGet(&spr_dma_action_list[sprnr], i);
        // check if item is a write to register sprxptl
        if (dma_action_item->called_function == sprxptl_functions[sprnr])
        {
          // a write to sprxptl was made during this line, execute it now
          (this->*(dma_action_item->called_function))(dma_action_item->data, dma_action_item->address);

          // data from sprxpos
          local_data_pos = chipmemReadWord(sprite_registers.sprpt[sprnr]);
          local_spry = ((local_data_pos & 0xff00) >> 8);
          local_sprx = ((local_data_pos & 0xff) << 1);

          // data from sprxctl
          local_data_ctl = chipmemReadWord(sprite_registers.sprpt[sprnr] + 2);
          local_sprx = (local_sprx & 0x1fe) | (local_data_ctl & 0x1);
          local_spry = (local_spry & 0x0ff) | ((local_data_ctl & 0x4) << 6);
          local_sprly = ((local_data_ctl & 0xff00) >> 8) | ((local_data_ctl & 0x2) << 7);

          if (((local_spry < local_sprly)))
          {
            // insert a write to sprxpos at time raster_x
            item = ActionListAddSorted(&spr_action_list[sprnr], dma_action_item->raster_x, dma_action_item->raster_y);
            item->raster_x = dma_action_item->raster_x;
            item->raster_y = dma_action_item->raster_y;
            item->called_function = &LineExactSprites::asprxpos;
            item->data = chipmemReadWord(sprite_registers.sprpt[sprnr]);
            item->address = sprnr << 3;

            // insert a write to sprxctl at time raster_x
            item = ActionListAddSorted(&spr_action_list[sprnr], dma_action_item->raster_x + 1, dma_action_item->raster_y);
            item->raster_x = dma_action_item->raster_x + 1;
            item->raster_y = dma_action_item->raster_y;
            item->called_function = &LineExactSprites::asprxctl;
            item->data = chipmemReadWord(sprite_registers.sprpt[sprnr] + 2);
            item->address = sprnr << 3;
          }

          if ((local_spry < local_sprly) && (local_sprx > 40))
          {
            // point to next two data words
            sprite_registers.sprpt[sprnr] = chipsetMaskPtr(sprite_registers.sprpt[sprnr] + 4);
          }

          if ((currentY < 25) && ((local_data_ctl == 0) && (local_data_pos == 0)))
          {
            sprite_state[sprnr] = 0;
          }
          else
          {
            sprite_state[sprnr] = 1;
          }
        }

        // check if item is a write to register sprxpth
        if (dma_action_item->called_function == sprxpth_functions[sprnr])
        {
          // update the sprxpth
          (this->*(dma_action_item->called_function))(dma_action_item->data, dma_action_item->address);
        }
      }
      ActionListClear(&spr_dma_action_list[sprnr]);
      break;
    }
    sprnr++;
  }
}

void LineExactSprites::ProcessActionList()
{
  uint32_t sprnr = 0;

  sprites_online = false;
  while (sprnr < 8)
  {
    uint32_t x_pos = 0;
    sprite_online[sprnr] = FALSE;
    sprite_16col[sprnr] = FALSE;

    uint32_t count = ActionListCount(&spr_action_list[sprnr]);
    for (uint32_t i = 0; i < count; i++)
    {
      spr_action_list_item* action_item = ActionListGet(&spr_action_list[sprnr], i);
      if (spr_arm_data[sprnr] == TRUE)
      {
        // flipflop is armed by a write to sprxdata
        if (action_item->raster_x > sprx[sprnr])
        {
          if (sprx[sprnr] > x_pos)
          {
            // for lores the horizontal blanking is until 71
            if (sprx[sprnr] >= 71)
            {
              // the comparator is armed before the coming action item, we may display the sprite data
              if ((sprnr & 0x01) == 0)
              {
                // even sprite 
                if (spratt[sprnr + 1] == 0) // if attached, work is done when handling odd sprite 
                {
                  // even sprite not attached to next odd sprite -> 3 color decode 
                  Decode4Sprite(sprnr);
                  sprites_online = true;
                  sprite_online[sprnr] = TRUE;
                }
              }
              else
              {
                // odd sprite 
                if (spratt[sprnr] == 0)
                {
                  // odd sprite not attached to previous even sprite -> 3 color decode 
                  Decode4Sprite(sprnr);
                  sprites_online = true;
                  sprite_online[sprnr] = TRUE;
                }
                else
                {
                  Decode16Sprite(sprnr);
                  sprites_online = true;
                  sprite_online[sprnr] = TRUE;
                  sprite_16col[sprnr] = TRUE;
                }
              }

              x_pos = sprx[sprnr];

              // for debugging only
              if (output_action_sprite_log == TRUE)
              {
                sprintf((char *)&buffer, "sprite %u data displayed on (y, x) = (%u, %u)\n",
                  sprnr, busGetRasterY(), sprx[sprnr]);
                fellowAddLog2(buffer);
              }
            }
          }
        }
      }
      // we can execute the coming action item
      (this->*(action_item->called_function))(action_item->data, action_item->address);
      x_pos = action_item->raster_x;
    }

    // check if flipflop is armed, maybe the comparator will also get armed in time left
    if ((spr_arm_data[sprnr] == TRUE) && (x_pos <= sprx[sprnr]))
    {
      // for lores the horizontal blanking is until 71
      if (sprx[sprnr] >= 71)
      {
        // the comparator becomes armed in time left, we may display the sprite data
        if ((sprnr & 0x01) == 0)
        {
          // even sprite 
          if (spratt[sprnr + 1] == 0) // if attached, work is done when handling odd sprite 
          {
            // even sprite not attached to next odd sprite -> 3 color decode 
            Decode4Sprite(sprnr);
            sprites_online = true;
            sprite_online[sprnr] = TRUE;
          }
        }
        else
        {
          // odd sprite 
          if (spratt[sprnr] == 0)
          {
            // odd sprite not attached to previous even sprite -> 3 color decode 
            Decode4Sprite(sprnr);
            sprites_online = true;
            sprite_online[sprnr] = TRUE;
          }
          else
          {
            Decode16Sprite(sprnr);
            sprites_online = true;
            sprite_online[sprnr] = TRUE;
            sprite_16col[sprnr] = TRUE;
          }
        }

        // for debugging only
        if (output_action_sprite_log == TRUE)
        {
          sprintf((char *)&buffer, "sprite %u data displayed on (y, x) = (%u, %u)\n",
            sprnr, busGetRasterY(), sprx[sprnr]);
          fellowAddLog2(buffer);
        }
      }
    }
    // clear the list at the end
    ActionListClear(&spr_action_list[sprnr]);
    sprnr++;
  }
}

void LineExactSprites::ProcessActionListNOP()
{
  uint32_t sprnr = 0;

  sprites_online = false;
  while (sprnr < 8)
  {
    sprite_online[sprnr] = FALSE;
    sprite_16col[sprnr] = FALSE;

    uint32_t count = ActionListCount(&spr_action_list[sprnr]);
    for (uint32_t i = 0; i < count; i++)
    {
      spr_action_list_item* action_item = ActionListGet(&spr_action_list[sprnr], i);
      // we can execute the coming action item
      (this->*(action_item->called_function))(action_item->data, action_item->address);
    }

    // clear the list at the end
    ActionListClear(&spr_action_list[sprnr]);
    sprnr++;
  }
}

void LineExactSprites::SetDebugging()
{
  if (output_sprite_log == TRUE)
  {
    output_sprite_log = FALSE;
    output_action_sprite_log = FALSE;
  }
  else
  {
    output_sprite_log = TRUE;
    output_action_sprite_log = TRUE;
  }
}

// current sprite is in front of playfield 2, and thus also in front of playfield 1
void LineExactSprites::MergeDualLoresPF2loopinfront2(graph_line* current_graph_line, uint32_t sprnr)
{
  uint8_t line2_buildup[4];

  uint32_t count = MergeListCount(&spr_merge_list[sprnr]);
  for (uint32_t j = 0; j < count; j++)
  {
    spr_merge_list_item *next_item = MergeListGet(&spr_merge_list[sprnr], j);
    uint8_t* line2 = current_graph_line->line2 + next_item->sprx + 1;
    uint8_t* sprite_data = next_item->sprite_data;

    for (uint32_t i = 0; i < 4; i++)
    {
      *((uint32_t *)line2_buildup) = *((uint32_t *)line2);
      if ((uint8_t)(*((uint32_t *)sprite_data)) != 0)
      {
        //cl = dl; 
        line2_buildup[0] = (uint8_t)*((uint32_t *)sprite_data);
      }

      // mdlpf21:
      if ((uint8_t)((*((uint32_t *)sprite_data)) >> 8) != 0)
      {
        //ch = dh; 
        line2_buildup[1] = (uint8_t)((*((uint32_t *)sprite_data) >> 8));
      }

      // mdlph22:
      if ((uint8_t)((*((uint32_t *)sprite_data)) >> 16) != 0)
      {
        //cl = dl; 
        line2_buildup[2] = (uint8_t)((*((uint32_t *)sprite_data) >> 16));
      }

      // mdlpf23:
      if ((uint8_t)((*((uint32_t *)sprite_data)) >> 24) != 0)
      {
        //ch = dh; 
        line2_buildup[3] = (uint8_t)((*((uint32_t *)sprite_data) >> 24));
      }

      // mdlpf24:
      *((uint32_t *)line2) = *((uint32_t *)line2_buildup);
      sprite_data += 4;
      line2 += 4;
    }
  }
}

// current sprite is behind of playfield 2, but in front of playfield 1
void LineExactSprites::MergeDualLoresPF1loopinfront2(graph_line* current_graph_line, uint32_t sprnr)
{
  uint8_t line_buildup[4];

  uint32_t count = MergeListCount(&spr_merge_list[sprnr]);
  for (uint32_t j = 0; j < count; j++)
  {
    spr_merge_list_item *next_item = MergeListGet(&spr_merge_list[sprnr], j);
    uint8_t* line1 = current_graph_line->line1 + next_item->sprx + 1;
    uint8_t* sprite_data = next_item->sprite_data;

    for (uint32_t i = 0; i < 4; i++)
    {
      *((uint32_t *)line_buildup) = *((uint32_t *)line1);
      if ((uint8_t)(*((uint32_t *)sprite_data)) != 0)
      {
        //cl = dl; 
        line_buildup[0] = (uint8_t)*((uint32_t *)sprite_data);
      }

      // mdlpf21:
      if ((uint8_t)((*((uint32_t *)sprite_data)) >> 8) != 0)
      {
        //ch = dh; 
        line_buildup[1] = (uint8_t)((*((uint32_t *)sprite_data) >> 8));
      }

      // mdlph22:
      if ((uint8_t)((*((uint32_t *)sprite_data)) >> 16) != 0)
      {
        //cl = dl; 
        line_buildup[2] = (uint8_t)((*((uint32_t *)sprite_data) >> 16));
      }

      // mdlpf23:
      if ((uint8_t)((*((uint32_t *)sprite_data)) >> 24) != 0)
      {
        //ch = dh; 
        line_buildup[3] = (uint8_t)((*((uint32_t *)sprite_data) >> 24));
      }

      // mdlpf24:
      *((uint32_t *)line1) = *((uint32_t *)line_buildup);
      sprite_data += 4;
      line1 += 4;
    }
  }
}

// current sprite is behind of playfield 2, and also behind playfield 1
void LineExactSprites::MergeDualLoresPF1loopbehind2(graph_line* current_graph_line, uint32_t sprnr)
{
  uint8_t line_buildup[4];

  uint32_t count = MergeListCount(&spr_merge_list[sprnr]);
  for (uint32_t j = 0; j < count; j++)
  {
    spr_merge_list_item *next_item = MergeListGet(&spr_merge_list[sprnr], j);
    uint8_t* line1 = current_graph_line->line1 + next_item->sprx + 1;
    uint8_t* sprite_data = next_item->sprite_data;

    for (uint32_t i = 0; i < 4; i++)
    {
      *((uint32_t *)line_buildup) = *((uint32_t *)line1);
      if ((uint8_t)(*((uint32_t *)line1)) == 0)
      {
        if ((uint8_t)(*((uint32_t *)sprite_data)) != 0)
        {
          line_buildup[0] = (uint8_t)*((uint32_t *)sprite_data);
        }
      }

      // mdlb1:
      if ((uint8_t)((*((uint32_t *)line1)) >> 8) == 0)
      {
        if ((uint8_t)((*((uint32_t *)sprite_data)) >> 8) != 0)
        {
          //ch = dh; 
          line_buildup[1] = (uint8_t)((*((uint32_t *)sprite_data) >> 8));
        }
      }

      // mdlb2:
      if ((uint8_t)((*((uint32_t *)line1)) >> 16) == 0)
      {
        if ((uint8_t)((*((uint32_t *)sprite_data)) >> 16) != 0)
        {
          //cl = dl; 
          line_buildup[2] = (uint8_t)((*((uint32_t *)sprite_data) >> 16));
        }
      }

      // mdlb3:
      if ((uint8_t)((*((uint32_t *)line1)) >> 24) == 0)
      {
        if ((uint8_t)((*((uint32_t *)sprite_data)) >> 24) != 0)
        {
          //ch = dh; 
          line_buildup[3] = (uint8_t)((*((uint32_t *)sprite_data) >> 24));
        }
      }

      // mdlb4:
      *((uint32_t *)line1) = *((uint32_t *)line_buildup);
      sprite_data += 4;
      line1 += 4;
    }
  }
}

// current sprite is in behind of playfield 2, and thus also behind playfield 1
void LineExactSprites::MergeDualLoresPF2loopbehind2(graph_line* current_graph_line, uint32_t sprnr)
{
  uint8_t line_buildup[4];

  uint32_t count = MergeListCount(&spr_merge_list[sprnr]);
  for (uint32_t j = 0; j < count; j++)
  {
    spr_merge_list_item *next_item = MergeListGet(&spr_merge_list[sprnr], j);
    uint8_t* line2 = current_graph_line->line2 + next_item->sprx + 1;
    uint8_t* sprite_data = next_item->sprite_data;

    for (uint32_t i = 0; i < 4; i++)
    {
      *((uint32_t *)line_buildup) = *((uint32_t *)line2);
      if ((uint8_t)(*((uint32_t *)line2)) == 0)
      {
        if ((uint8_t)(*((uint32_t *)sprite_data)) != 0)
        {
          line_buildup[0] = (uint8_t)*((uint32_t *)sprite_data);
        }
      }

      // mdlpfb1:
      if ((uint8_t)((*((uint32_t *)line2)) >> 8) == 0)
      {
        if ((uint8_t)((*((uint32_t *)sprite_data)) >> 8) != 0)
        {
          //ch = dh; 
          line_buildup[1] = (uint8_t)((*((uint32_t *)sprite_data) >> 8));
        }
      }

      // mdlpfb2:
      if ((uint8_t)((*((uint32_t *)line2)) >> 16) == 0)
      {
        if ((uint8_t)((*((uint32_t *)sprite_data)) >> 16) != 0)
        {
          //cl = dl; 
          line_buildup[2] = (uint8_t)((*((uint32_t *)sprite_data) >> 16));
        }
      }

      // mdlpfb3:
      if ((uint8_t)((*((uint32_t *)line2)) >> 24) == 0)
      {
        if ((uint8_t)((*((uint32_t *)sprite_data)) >> 24) != 0)
        {
          //ch = dh; 
          line_buildup[3] = (uint8_t)((*((uint32_t *)sprite_data) >> 24));
        }
      }

      // mdlpfb4:
      *((uint32_t *)line2) = *((uint32_t *)line_buildup);
      sprite_data += 4;
      line2 += 4;
    }
  }
}

void LineExactSprites::MergeDualLoresPlayfield(graph_line* current_graph_line)
{
  for (uint32_t sprnr = 0; sprnr < 8; sprnr++)
  {
    if (sprite_online[sprnr] == TRUE)
    {
      // determine whetever this sprite is in front of playfield 1 and/or in front or behind playfield 2
      if (_core.RegisterUtility.IsPlayfield2PriorityEnabled())
      {
        // playfield 2 is in front of playfield 1
        if ((_core.Registers.BplCon2 & 0x38) > sprnr * 4)
        {
          // current sprite is in front of playfield 2, and thus also in front of playfield 1
          MergeDualLoresPF2loopinfront2(current_graph_line, sprnr);
        }
        else
        {
          // current sprite is behind of playfield 2
          if (((_core.Registers.BplCon2 & 0x7) << 1) > sprnr)
          {
            // current sprite is behind of playfield 2, but in front of playfield 1
            MergeDualLoresPF1loopinfront2(current_graph_line, sprnr);
          }
          else
          {
            // current sprite is behind of playfield 2, and also behind playfield 1
            MergeDualLoresPF1loopbehind2(current_graph_line, sprnr);
          }
        }
      }
      else
      {
        // playfield 1 is in front of playfield 2
        if (((_core.Registers.BplCon2 & 0x7) << 1) > sprnr)
        {
          // current sprite is in front of playfield 1, and thus also in front of playfield 2
          MergeDualLoresPF1loopinfront2(current_graph_line, sprnr);
        }
        else
        {
          if ((_core.Registers.BplCon2 & 0x38) > sprnr * 4)
          {
            // current sprite is in front of playfield 2, but behind playfield 1 (in between)
            MergeDualLoresPF2loopinfront2(current_graph_line, sprnr);
          }
          else
          {
            // current sprite is in behind of playfield 2, and thus also behind playfield 1
            MergeDualLoresPF2loopbehind2(current_graph_line, sprnr);
          }
        }
      }
    }
  }
}

// current sprite is in front of playfield 2, and thus also in front of playfield 1
void LineExactSprites::MergeDualHiresPF2loopinfront2(graph_line* current_graph_line, uint32_t sprnr)
{
  uint32_t count = MergeListCount(&spr_merge_list[sprnr]);
  for (uint32_t j = 0; j < count; j++)
  {
    spr_merge_list_item *next_item = MergeListGet(&spr_merge_list[sprnr], j);
    uint8_t *line2 = current_graph_line->line2 + 2 * (next_item->sprx + 1);
    uint8_t *sprite_data = next_item->sprite_data;

    for (uint32_t i = 0; i < 4; i++)
    {
      uint8_t sprite_color = sprite_data[0];
      if (sprite_color != 0)
      {
        line2[0] = sprite_color;
        line2[1] = sprite_color;
      }

      sprite_color = sprite_data[1];
      if (sprite_color != 0)
      {
        line2[2] = sprite_color;
        line2[3] = sprite_color;
      }

      sprite_color = sprite_data[2];
      if (sprite_color != 0)
      {
        line2[4] = sprite_color;
        line2[5] = sprite_color;
      }

      sprite_color = sprite_data[3];
      if (sprite_color != 0)
      {
        line2[6] = sprite_color;
        line2[7] = sprite_color;
      }

      sprite_data += 4;
      line2 += 8;
    }
  }
}

// current sprite is behind of playfield 2, but in front of playfield 1
void LineExactSprites::MergeDualHiresPF1loopinfront2(graph_line* current_graph_line, uint32_t sprnr)
{
  uint32_t count = MergeListCount(&spr_merge_list[sprnr]);
  for (uint32_t j = 0; j < count; j++)
  {
    spr_merge_list_item *next_item = MergeListGet(&spr_merge_list[sprnr], j);
    uint8_t* line1 = current_graph_line->line1 + 2 * (next_item->sprx + 1);
    uint8_t* sprite_data = next_item->sprite_data;

    for (uint32_t i = 0; i < 4; i++)
    {
      uint8_t sprite_color = sprite_data[0];
      if (sprite_color != 0)
      {
        line1[0] = sprite_color;
        line1[1] = sprite_color;
      }

      sprite_color = sprite_data[1];
      if (sprite_color != 0)
      {
        line1[2] = sprite_color;
        line1[3] = sprite_color;
      }

      sprite_color = sprite_data[2];
      if (sprite_color != 0)
      {
        line1[4] = sprite_color;
        line1[5] = sprite_color;
      }

      sprite_color = sprite_data[3];
      if (sprite_color != 0)
      {
        line1[6] = sprite_color;
        line1[7] = sprite_color;
      }

      sprite_data += 4;
      line1 += 8;
    }
  }
}

// current sprite is behind of playfield 2, and also behind playfield 1
void LineExactSprites::MergeDualHiresPF1loopbehind2(graph_line* current_graph_line, uint32_t sprnr)
{
  uint32_t count = MergeListCount(&spr_merge_list[sprnr]);
  for (uint32_t j = 0; j < count; j++)
  {
    spr_merge_list_item *next_item = MergeListGet(&spr_merge_list[sprnr], j);
    uint8_t* line1 = current_graph_line->line1 + 2 * (next_item->sprx + 1);
    uint8_t* sprite_data = next_item->sprite_data;

    for (uint32_t i = 0; i < 4; i++)
    {
      uint8_t sprite_color = sprite_data[0];
      if (sprite_color != 0)
      {
        if (line1[0] == 0)
        {
          line1[0] = sprite_color;
        }
        if (line1[1] == 0)
        {
          line1[1] = sprite_color;
        }
      }

      sprite_color = sprite_data[1];
      if (sprite_color != 0)
      {
        if (line1[2] == 0)
        {
          line1[2] = sprite_color;
        }
        if (line1[3] == 0)
        {
          line1[3] = sprite_color;
        }
      }

      sprite_color = sprite_data[2];
      if (sprite_color != 0)
      {
        if (line1[4] == 0)
        {
          line1[4] = sprite_color;
        }
        if (line1[5] == 0)
        {
          line1[5] = sprite_color;
        }
      }

      sprite_color = sprite_data[3];
      if (sprite_color != 0)
      {
        if (line1[6] == 0)
        {
          line1[6] = sprite_color;
        }
        if (line1[7] == 0)
        {
          line1[7] = sprite_color;
        }
      }
      sprite_data += 4;
      line1 += 8;
    }
  }
}

// current sprite is in behind of playfield 2, and thus also behind playfield 1
void LineExactSprites::MergeDualHiresPF2loopbehind2(graph_line* current_graph_line, uint32_t sprnr)
{
  uint32_t count = MergeListCount(&spr_merge_list[sprnr]);
  for (uint32_t j = 0; j < count; j++)
  {
    spr_merge_list_item *next_item = MergeListGet(&spr_merge_list[sprnr], j);
    uint8_t *line2 = current_graph_line->line2 + 2 * (next_item->sprx + 1);
    uint8_t *sprite_data = next_item->sprite_data;

    for (uint32_t i = 0; i < 4; i++)
    {
      uint8_t sprite_color = sprite_data[0];
      if (sprite_color != 0)
      {
        if (line2[0] == 0)
        {
          line2[0] = sprite_color;
        }
        if (line2[1] == 0)
        {
          line2[1] = sprite_color;
        }
      }

      sprite_color = sprite_data[1];
      if (sprite_color != 0)
      {
        if (line2[2] == 0)
        {
          line2[2] = sprite_color;
        }
        if (line2[3] == 0)
        {
          line2[3] = sprite_color;
        }
      }

      sprite_color = sprite_data[2];
      if (sprite_color != 0)
      {
        if (line2[4] == 0)
        {
          line2[4] = sprite_color;
        }
        if (line2[5] == 0)
        {
          line2[5] = sprite_color;
        }
      }

      sprite_color = sprite_data[3];
      if (sprite_color != 0)
      {
        if (line2[6] == 0)
        {
          line2[6] = sprite_color;
        }
        if (line2[7] == 0)
        {
          line2[7] = sprite_color;
        }
      }
      sprite_data += 4;
      line2 += 8;
    }
  }
}

void LineExactSprites::MergeDualHiresPlayfield(graph_line* current_graph_line)
{
  for (uint32_t sprnr = 0; sprnr < 8; sprnr++)
  {
    if (sprite_online[sprnr] == TRUE)
    {
      // determine whetever this sprite is in front of playfield 1 and/or in front or behind playfield 2
      if (_core.RegisterUtility.IsPlayfield2PriorityEnabled())
      {
        // playfield 2 is in front of playfield 1
        if ((_core.Registers.BplCon2 & 0x38) > sprnr * 4)
        {
          // current sprite is in front of playfield 2, and thus also in front of playfield 1
          MergeDualHiresPF2loopinfront2(current_graph_line, sprnr);
        }
        else
        {
          // current sprite is behind of playfield 2
          if (((_core.Registers.BplCon2 & 0x7) << 1) > sprnr)
          {
            // current sprite is behind of playfield 2, but in front of playfield 1
            MergeDualHiresPF1loopinfront2(current_graph_line, sprnr);
          }
          else
          {
            // current sprite is behind of playfield 2, and also behind playfield 1
            MergeDualHiresPF1loopbehind2(current_graph_line, sprnr);
          }
        }
      }
      else
      {
        // playfield 1 is in front of playfield 2
        if (((_core.Registers.BplCon2 & 0x7) << 1) > sprnr)
        {
          // current sprite is in front of playfield 1, and thus also in front of playfield 2
          MergeDualHiresPF1loopinfront2(current_graph_line, sprnr);
        }
        else
        {
          if ((_core.Registers.BplCon2 & 0x38) > sprnr * 4)
          {
            // current sprite is in front of playfield 2, but behind playfield 1 (in between)
            MergeDualHiresPF2loopinfront2(current_graph_line, sprnr);
          }
          else
          {
            // current sprite is in behind of playfield 2, and thus also behind playfield 1
            MergeDualHiresPF2loopbehind2(current_graph_line, sprnr);
          }
        }
      }
    }
  }
}

void LineExactSprites::MergeHires(graph_line* current_graph_line)
{
  for (uint32_t sprnr = 0; sprnr < 8; sprnr++)
  {
    if (sprite_online[sprnr] == TRUE)
    {
      uint32_t count = MergeListCount(&spr_merge_list[sprnr]);
      for (uint32_t j = 0; j < count; j++)
      {
        spr_merge_list_item *next_item = MergeListGet(&spr_merge_list[sprnr], j);

        // there is sprite data waiting within this line
        if (next_item->sprx <= graph_DIW_last_visible)
        {
          // set destination and source 
          uint8_t *line1 = (current_graph_line->line1 + 2 * (next_item->sprx + 1));
          uint8_t *sprite_data = next_item->sprite_data;

          SpriteMerger::MergeHires(sprnr, line1, sprite_data, 16);
        }
      }
    }
  }
}

void LineExactSprites::MergeLores(graph_line* current_graph_line)
{
  for (int sprnr = 7; sprnr >= 0; sprnr--)
  {
    if (sprite_online[sprnr] == TRUE)
    {
      uint32_t count = MergeListCount(&spr_merge_list[sprnr]);
      for (uint32_t j = 0; j < count; j++)
      {
        spr_merge_list_item *next_item = MergeListGet(&spr_merge_list[sprnr], j);
        // there is sprite data waiting within this line
        if (next_item->sprx <= graph_DIW_last_visible)
        {
          // set destination and source 
          uint8_t *line1 = (current_graph_line->line1 + (next_item->sprx) + 1);
          uint8_t *sprite_data = next_item->sprite_data;

          SpriteMerger::MergeLores(sprnr, line1, sprite_data, 16);
        }
      }
    }
  }
}

void LineExactSprites::Merge(graph_line* current_graph_line)
{
  sprites_online = false;

  if (_core.RegisterUtility.IsHAMEnabled())
  {
    // HAM mode bit is set
    MergeHAM(current_graph_line);
    return;
  }

  if (_core.RegisterUtility.IsDualPlayfieldEnabled())
  {
    // dual playfield bit is set

    if (_core.RegisterUtility.IsHiresEnabled())
    {
      MergeDualHiresPlayfield(current_graph_line);
    }
    else
    {
      MergeDualLoresPlayfield(current_graph_line);
    }
    return;
  }

  if (_core.RegisterUtility.IsHiresEnabled())
  {
    MergeHires(current_graph_line);
  }
  else
  {
    // lores 
    MergeLores(current_graph_line);
  }
}


/*===========================================================================*/
/* Called on emulation hard reset                                            */
/*===========================================================================*/

void LineExactSprites::HardReset()
{
  ClearState();
}


/*===========================================================================*/
/* Called on emulation end of line                                           */
/*===========================================================================*/

void LineExactSprites::EndOfLine(uint32_t rasterY)
{
  // Make sure action lists and merge lists are empty
  ProcessActionListNOP();
  for (uint32_t i = 0; i < 8; i++)
  {
    MergeListClear(&spr_merge_list[i]);
  }
}


/*===========================================================================*/
/* Called on emulation end of frame                                          */
/*===========================================================================*/

void LineExactSprites::EndOfFrame()
{
  ProcessDMAActionListNOP();
  for (uint32_t i = 0; i < 8; i++)
  {
    sprite_state[i] = 0;
    spr_arm_data[i] = FALSE;
    spr_arm_comparator[i] = FALSE;
    ActionListClear(&spr_action_list[i]);
    ActionListClear(&spr_dma_action_list[i]);
  }
  sprite_ham_slot_next = 0;
}


/*===========================================================================*/
/* Called on emulation start                                                 */
/*===========================================================================*/

void LineExactSprites::EmulationStart()
{
}

/*===========================================================================*/
/* Called on emulation stop                                                  */
/*===========================================================================*/

void LineExactSprites::EmulationStop()
{
}

LineExactSprites::LineExactSprites()
  : Sprites(),
  sprite_to_block(0),
  output_sprite_log(FALSE),
  output_action_sprite_log(FALSE),
  sprite_ham_slot_next(0)
{
  for (int i = 0; i < 8; i++)
  {
    sprite_state[i] = 0;
    spr_arm_data[i] = FALSE;
    spr_arm_comparator[i] = FALSE;
    ActionListClear(&spr_action_list[i]);
    ActionListClear(&spr_dma_action_list[i]);
    MergeListClear(&spr_merge_list[i]);
  }
  ClearState();
}

LineExactSprites::~LineExactSprites()
{
}
