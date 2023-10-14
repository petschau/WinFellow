#pragma once

#include "Service/IFSWrapper.h"

class FSWrapperWin32 : public Service::IFSWrapper
{
private:
  Service::fs_wrapper_point* MakePointInternal(const char* point);

public:
  Service::fs_wrapper_point *MakePoint(const char *point) override;
  int Stat(const char* szFilename, struct stat* pStatBuffer) override;
  FSWrapperWin32();
};
