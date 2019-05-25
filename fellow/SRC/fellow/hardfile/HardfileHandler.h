#ifndef FELLOW_HARDFILE_HARDFILEHANDLER_H
#define FELLOW_HARDFILE_HARDFILEHANDLER_H

#include <string>
#include <vector>
#include "fellow/api/module/IHardfileHandler.h"
#include "fellow/hardfile/HardfileStructs.h"

#define FHFILE_MAX_DEVICES 20

namespace fellow::hardfile
{
  class HardfileHandler : public fellow::api::module::IHardfileHandler
  {
  private:
    HardfileDevice _devices[FHFILE_MAX_DEVICES];
    std::vector<std::unique_ptr<HardfileFileSystemEntry>> _fileSystems;
    std::vector<std::unique_ptr<HardfileMountListEntry>> _mountList;
    ULO _devicename = 0;
    ULO _romstart = 0;
    ULO _bootcode = 0;
    ULO _configdev = 0;
    ULO _fsname = 0;
    ULO _endOfDmem = 0;
    ULO _dosDevPacketListStart = 0;
    UBY _rom[65536];
    bool _enabled = false;
    unsigned int _deviceNameStartNumber;

    bool HasZeroDevices();

    void CreateMountList();
    std::string MakeDeviceName();
    std::string MakeDeviceName(const std::string& preferredName);
    bool PreferredNameExists(const std::string& preferredName);
    bool FindOlderOrSameFileSystemVersion(ULO DOSType, ULO version, unsigned int& olderOrSameFileSystemIndex);
    HardfileFileSystemEntry *GetFileSystemForDOSType(ULO DOSType);
    void AddFileSystemsFromRdb(HardfileDevice& device);
    void AddFileSystemsFromRdb();
    void EraseOlderOrSameFileSystemVersion(ULO DOSType, ULO version);
    void SetHardfileConfigurationFromRDB(fellow::api::module::HardfileConfiguration& config, rdb::RDB* rdb, bool readonly);
    void InitializeHardfile(unsigned int index);
    void RebuildHardfileConfiguration();

    void SetIOError(BYT errorCode);
    void SetIOActual(ULO ioActual);
    ULO GetUnitNumber();
    UWO GetCommand();
    unsigned int GetIndexFromUnitNumber(ULO unit);
    ULO GetUnitNumberFromIndex(unsigned int index);

    // BeginIO commands
    void IgnoreOK(ULO index);
    BYT Read(ULO index);
    BYT Write(ULO index);
    BYT GetNumberOfTracks(ULO index);
    BYT GetDiskDriveType(ULO index);
    void WriteProt(ULO index);
    BYT ScsiDirect(ULO index);

    void DoDiag();
    void DoOpen();
    void DoClose();
    void DoExpunge();
    void DoNULL();
    void DoBeginIO();
    void DoAbortIO();

    ULO DoGetRDBFileSystemCount();
    ULO DoGetRDBFileSystemHunkCount(ULO fileSystemIndex);
    ULO DoGetRDBFileSystemHunkSize(ULO fileSystemIndex, ULO hunkIndex);
    void DoCopyRDBFileSystemHunk(ULO destinationAddress, ULO fileSystemIndex, ULO hunkIndex);
    void DoRelocateFileSystem(ULO fileSystemIndex);
    void DoInitializeRDBFileSystemEntry(ULO fileSystemEntry, ULO fileSystemIndex);
    void DoPatchDOSDeviceNode(ULO node, ULO packet);
    void DoUnknownOperation(ULO operation);
    ULO DoGetDosDevPacketListStart();

    std::string LogGetStringFromMemory(ULO address);

    void DoLogAvailableFileSystems(ULO fileSystemResource);
    void DoLogAvailableResources();
    void DoLogAllocMemResult(ULO result);
    void DoLogOpenResourceResult(ULO result);
    void DoRemoveRDBFileSystemsAlreadySupportedBySystem(ULO filesystemResource);

    void CreateDOSDevPackets(ULO devicename);
    void MakeDOSDevPacketForPlainHardfile(const HardfileMountListEntry& mountListEntry, ULO deviceNameAddress);
    void MakeDOSDevPacketForRDBPartition(const HardfileMountListEntry& mountListEntry, ULO deviceNameAddress);

  public:
    // Autoconfig and ROM memory
    void CardInit() override;
    void CardMap(ULO mapping) override;
    UBY ReadByte(ULO address) override;
    UWO ReadWord(ULO address) override;
    ULO ReadLong(ULO address) override;

    // Native callback
    void Do(ULO data) override;

    // Configuration
    void SetEnabled(bool enabled) override;
    bool GetEnabled() override;
    void Clear() override;
    bool CompareHardfile(const fellow::api::module::HardfileConfiguration& configuration, unsigned int index) override;
    void SetHardfile(const fellow::api::module::HardfileConfiguration& configuration, unsigned int index) override;
    bool RemoveHardfile(unsigned int index) override;
    unsigned int GetMaxHardfileCount() override;
    void SetDeviceNameStartNumber(unsigned int unitNoStartNumber) override;

    // UI helper function
    bool Create(const fellow::api::module::HardfileConfiguration& configuration, ULO size) override;
    bool HasRDB(const std::string& filename) override;
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

#endif
