#pragma once

#include <string>
#include <vector>
#include "fellow/api/module/IHardfileHandler.h"
#include "hardfile/HardfileStructs.h"

#define FHFILE_MAX_DEVICES 20

namespace fellow::hardfile
{
  class HardfileHandler : public fellow::api::module::IHardfileHandler
  {
  private:
    HardfileDevice _devices[FHFILE_MAX_DEVICES];
    std::vector<std::unique_ptr<HardfileFileSystemEntry>> _fileSystems;
    std::vector<std::unique_ptr<HardfileMountListEntry>> _mountList;
    uint32_t _devicename = 0;
    uint32_t _romstart = 0;
    uint32_t _bootcode = 0;
    uint32_t _configdev = 0;
    uint32_t _fsname = 0;
    uint32_t _endOfDmem = 0;
    uint32_t _dosDevPacketListStart = 0;
    uint8_t _rom[65536] = {};
    bool _enabled = false;
    unsigned int _deviceNameStartNumber = 0;

    bool HasZeroDevices();

    void CreateMountList();
    std::string MakeDeviceName();
    std::string MakeDeviceName(const std::string& preferredName);
    bool PreferredNameExists(const std::string& preferredName);
    bool FindOlderOrSameFileSystemVersion(uint32_t DOSType, uint32_t version, unsigned int& olderOrSameFileSystemIndex);
    HardfileFileSystemEntry *GetFileSystemForDOSType(uint32_t DOSType);
    void AddFileSystemsFromRdb(HardfileDevice& device);
    void AddFileSystemsFromRdb();
    void EraseOlderOrSameFileSystemVersion(uint32_t DOSType, uint32_t version);
    void SetHardfileConfigurationFromRDB(fellow::api::module::HardfileConfiguration& config, rdb::RDB* rdb, bool readonly);
    bool OpenHardfileFile(HardfileDevice& device);
    void InitializeHardfile(unsigned int index);
    void RebuildHardfileConfiguration();
    void ClearDeviceRuntimeInfo(HardfileDevice& device);

    void SetIOError(int8_t errorCode);
    void SetIOActual(uint32_t ioActual);
    uint32_t GetUnitNumber();
    uint16_t GetCommand();
    unsigned int GetIndexFromUnitNumber(uint32_t unit);
    uint32_t GetUnitNumberFromIndex(unsigned int index);

    // BeginIO commands
    void IgnoreOK(uint32_t index);
    int8_t Read(uint32_t index);
    int8_t Write(uint32_t index);
    int8_t GetNumberOfTracks(uint32_t index);
    int8_t GetDiskDriveType(uint32_t index);
    void WriteProt(uint32_t index);
    int8_t ScsiDirect(uint32_t index);

    void DoDiag();
    void DoOpen();
    void DoClose();
    void DoExpunge();
    void DoNULL();
    void DoBeginIO();
    void DoAbortIO();

    uint32_t DoGetRDBFileSystemCount();
    uint32_t DoGetRDBFileSystemHunkCount(uint32_t fileSystemIndex);
    uint32_t DoGetRDBFileSystemHunkSize(uint32_t fileSystemIndex, uint32_t hunkIndex);
    void DoCopyRDBFileSystemHunk(uint32_t destinationAddress, uint32_t fileSystemIndex, uint32_t hunkIndex);
    void DoRelocateFileSystem(uint32_t fileSystemIndex);
    void DoInitializeRDBFileSystemEntry(uint32_t fileSystemEntry, uint32_t fileSystemIndex);
    void DoPatchDOSDeviceNode(uint32_t node, uint32_t packet);
    void DoUnknownOperation(uint32_t operation);
    uint32_t DoGetDosDevPacketListStart();

    std::string LogGetStringFromMemory(uint32_t address);

    void DoLogAvailableFileSystems(uint32_t fileSystemResource);
    void DoLogAvailableResources();
    void DoLogAllocMemResult(uint32_t result);
    void DoLogOpenResourceResult(uint32_t result);
    void DoRemoveRDBFileSystemsAlreadySupportedBySystem(uint32_t filesystemResource);

    void CreateDOSDevPackets(uint32_t devicename);
    void MakeDOSDevPacketForPlainHardfile(const HardfileMountListEntry& mountListEntry, uint32_t deviceNameAddress);
    void MakeDOSDevPacketForRDBPartition(const HardfileMountListEntry& mountListEntry, uint32_t deviceNameAddress);

  public:
    // Autoconfig and ROM memory
    void CardInit() override;
    void CardMap(uint32_t mapping) override;
    uint8_t ReadByte(uint32_t address) override;
    uint16_t ReadWord(uint32_t address) override;
    uint32_t ReadLong(uint32_t address) override;

    // Native callback
    void Do(uint32_t data) override;

    // Configuration
    void SetEnabled(bool enabled) override;
    bool GetEnabled() override;
    void Clear() override;
    bool CompareHardfile(const fellow::api::module::HardfileConfiguration& configuration, unsigned int index) override;
    void SetHardfile(const fellow::api::module::HardfileConfiguration& configuration, unsigned int index) override;
    bool RemoveHardfile(unsigned int index) override;
    unsigned int GetMaxHardfileCount() override;

    // UI helper function
    bool Create(const fellow::api::module::HardfileConfiguration& configuration, uint32_t size) override;
    fellow::api::module::rdb_status HasRDB(const std::string& filename) override;
    fellow::api::module::HardfileConfiguration GetConfigurationFromRDBGeometry(const std::string& filename) override;

    // Global events
    void EmulationStart() override;
    void EmulationStop() override;
    void HardReset() override;
    void Startup() override;
    void Shutdown() override;

    HardfileHandler();
    virtual ~HardfileHandler();
  };
}
