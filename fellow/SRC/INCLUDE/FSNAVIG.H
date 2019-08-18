#ifndef FSNAVIG_H
#define FSNAVIG_H

#include "LISTTREE.H"

enum class fs_navig_file_types 
{
  FS_NAVIG_FILE,
  FS_NAVIG_DIR,
  FS_NAVIG_OTHER
};

struct fs_navig_point
{
  UBY drive;
  STR name[FS_WRAP_MAX_PATH_LENGTH];
  BOOLE relative;
  BOOLE writeable;
  ULO size;
  fs_navig_file_types type;
//  felist *lnode;
};

#endif
