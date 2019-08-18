#include "fellow/chipset/DualPlayfieldMapper.h"

uint8_t DualPlayfieldMapper::_mappingTable[2][256][256]; // Usage: _mappingTable[0 - PF1 behind, 1 - PF2 behind][PF1data][PF2data]

const uint8_t *const DualPlayfieldMapper::GetMappingPtr(const bool isPlayfield1Pri)
{
  if (isPlayfield1Pri)
  {
    // bit 6 of BPLCON2 is not set, thus playfield 1 is in front of playfield 2
    // write results in draw_dual_translate[1], instead of draw_dual_translate[0]
    return (uint8_t *)&_mappingTable[1][0][0];
  }
  return (const uint8_t *const)_mappingTable;
}

uint8_t DualPlayfieldMapper::Map(const uint8_t oddPlayfieldColorIndex, const uint8_t evenPlayfieldColorIndex) const
{
  return *(_currentMapping + oddPlayfieldColorIndex * 256 + evenPlayfieldColorIndex);
}

void DualPlayfieldMapper::InitializeMappingTable()
{
  for (int32_t k = 0; k < 2; k++)
  {
    for (int32_t i = 0; i < 256; i++)
    {
      for (int32_t j = 0; j < 256; j++)
      {
        int32_t l;

        if (k == 0)
        { /* PF1 behind, PF2 in front */
          if (j == 0)
          {
            l = i; /* PF2 transparent, PF1 visible */
          }
          else
          { /* PF2 visible */
            /* If color is higher than 16 it is a sprite */
            if (j < 16)
            {
              l = j + 8;
            }
            else
            {
              l = j;
            }
          }
        }
        else
        { /* PF1 in front, PF2 behind */
          if (i == 0)
          { /* PF1 transparent, PF2 visible */
            if (j == 0)
            {
              l = 0;
            }
            else
            {
              if (j < 16)
              {
                l = j + 8;
              }
              else
              {
                l = j;
              }
            }
          }
          else
          {
            l = i; /* PF1 visible amd not transparent */
          }
        }

        _mappingTable[k][i][j] = (uint8_t)l;
      }
    }
  }
}

DualPlayfieldMapper::DualPlayfieldMapper(const bool isPlayfield1Pri) : _currentMapping(GetMappingPtr(isPlayfield1Pri))
{
}