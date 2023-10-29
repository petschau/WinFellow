#pragma once

#include <string>
#include <vector>
#include "Defs.h"
#include "Module/Hardfile/HardfileConfiguration.h"
#include "Module/Hardfile/RdbStatus.h"

constexpr unsigned int FHFILE_MAX_DEVICES = 20;

namespace Module::Hardfile
{
  class IHardfileHandler
  {
  public:
    virtual ~IHardfileHandler() = default;

    // Autoconfig and ROM memory
    virtual void CardInit() = 0;
    virtual void CardMap(uint32_t mapping) = 0;
    virtual uint8_t ReadByte(uint32_t address) = 0;
    virtual uint16_t ReadWord(uint32_t address) = 0;
    virtual uint32_t ReadLong(uint32_t address) = 0;

    // Native callback
    virtual void Do(uint32_t data) = 0;

    // Configuration
    virtual void SetEnabled(bool enabled) = 0;
    virtual bool GetEnabled() = 0;
    virtual void Clear() = 0;
    virtual bool CompareHardfile(const HardfileConfiguration &hardfile, unsigned int index) = 0;
    virtual void SetHardfile(const HardfileConfiguration &hardfile, unsigned int index) = 0;
    virtual bool RemoveHardfile(unsigned int index) = 0;
    virtual unsigned int GetMaxHardfileCount() = 0;

    // UI helper function
    virtual bool Create(const HardfileConfiguration &configuration, uint32_t size) = 0;
    virtual rdb_status HasRDB(const std::string &filename) = 0;
    virtual HardfileConfiguration GetConfigurationFromRDBGeometry(const std::string &filename) = 0;

    // Global events
    virtual void EmulationStart() = 0;
    virtual void EmulationStop() = 0;
    virtual void HardReset() = 0;
    virtual void Startup() = 0;
    virtual void Shutdown() = 0;
  };
}
