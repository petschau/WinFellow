#ifndef FELLOW_API_MODULE_IHARDFILEHANDLER_H
#define FELLOW_API_MODULE_IHARDFILEHANDLER_H

#include "fellow/api/defs.h"

#define FHFILE_MAX_DEVICES 20

namespace fellow::api::module
{
  class HardfileDevice
  {
  public:
    STR filename[256]; /* Config settings */
    bool readonly_original;
    ULO bytespersector_original;
    ULO sectorspertrack;
    ULO reservedblocks_original;
    ULO surfaces;
    ULO size;
  };

  // Abstract interface to HardfileHandler
  class IHardfileHandler
  {
  public:
    // Autoconfig and ROM memory
    virtual void CardInit() = 0;
    virtual void CardMap(ULO mapping) = 0;
    virtual UBY ReadByte(ULO address) = 0;
    virtual UWO ReadWord(ULO address) = 0;
    virtual ULO ReadLong(ULO address) = 0;

    // Native callback
    virtual void Do(ULO data) = 0;

    // Configuration
    virtual void SetEnabled(bool enabled) = 0;
    virtual bool GetEnabled() = 0;
    virtual void Clear() = 0;
    virtual bool CompareHardfile(fellow::api::module::HardfileDevice hardfile, ULO index) = 0;
    virtual void SetHardfile(fellow::api::module::HardfileDevice hardfile, ULO index) = 0;
    virtual bool RemoveHardfile(unsigned int index) = 0;
    virtual unsigned int GetMaxHardfileCount() = 0;

    // UI helper function
    virtual bool Create(fellow::api::module::HardfileDevice hfile) = 0;

    // Global events
    virtual void HardReset() = 0;
    virtual void Startup() = 0;
    virtual void Shutdown() = 0;
  };

  extern IHardfileHandler *HardfileHandler;
}

#endif
