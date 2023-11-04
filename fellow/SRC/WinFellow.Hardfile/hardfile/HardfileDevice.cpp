#pragma once

#include "hardfile/HardfileDevice.h"

namespace fellow::hardfile
{
  bool HardfileDevice::CloseFile()
  {
    if (F != nullptr)
    {
      fflush(F);
      fclose(F);
      return true;
    }

    return false;
  }

  void HardfileDevice::DeleteRDB()
  {
    if (HasRDB)
    {
      delete RDB;
      RDB = nullptr;
      HasRDB = false;
    }
  }

  bool HardfileDevice::Clear()
  {
    bool result = CloseFile();
    DeleteRDB();
    FileSize = 0;
    GeometrySize = 0;
    Readonly = false;
    Status = FHFILE_NONE;
    Configuration.Clear();
    return result;
  }

  HardfileDevice::HardfileDevice() : Readonly(false), FileSize(0), GeometrySize(0), Status(FHFILE_NONE), F(nullptr), HasRDB(false), RDB(nullptr)
  {
  }

  HardfileDevice::~HardfileDevice()
  {
    CloseFile();
    DeleteRDB();
  }
}
