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
    ULO _romstart;
    ULO _bootcode;
    ULO _configdev;
    ULO _fsname;
    UBY _rom[65536];
    bool _enabled;

    bool HasZeroDevices();

    void CreateMountList();
    std::string MakeDeviceName(int no);
    std::string MakeDeviceName(const std::string& preferredName, int no);
    bool PreferredNameExists(const std::string& preferredName);
    int FindOlderOrSameFileSystemVersion(ULO dosType, ULO version);
    HardfileFileSystemEntry *GetFileSystemForDOSType(ULO dosType);
    void AddFileSystemsFromRdb(HardfileDevice& device);
    void AddFileSystemsFromRdb();
    void EraseOlderOrSameFileSystemVersion(ULO dosType, ULO version);
    void SetPhysicalGeometryFromRDB(HardfileDevice *fhfile);
    void InitializeHardfile(ULO index);

    // BeginIO commands
    void Ignore(ULO index);
    BYT Read(ULO index);
    BYT Write(ULO index);
    void GetNumberOfTracks(ULO index);
    void GetDriveType(ULO index);
    void WriteProt(ULO index);
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

    std::string LogGetStringFromMemory(ULO address);

    void DoLogAvailableFileSystems(ULO fileSystemResource);
    void DoLogAvailableResources();
    void DoLogAllocMemResult(ULO result);
    void DoLogOpenResourceResult(ULO result);
    void DoRemoveRDBFileSystemsAlreadySupportedBySystem(ULO filesystemResource);

    void MakeDOSDevPacketForPlainHardfile(const HardfileMountListEntry& partition, ULO deviceNameAddress);
    void MakeDOSDevPacketForRDBPartition(const HardfileMountListEntry& partition, ULO deviceNameAddress);

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
    bool CompareHardfile(fellow::api::module::HardfileDevice hardfile, ULO index) override;
    void SetHardfile(fellow::api::module::HardfileDevice hardfile, ULO index) override;
    bool RemoveHardfile(unsigned int index) override;
    unsigned int GetMaxHardfileCount() override;

    // UI helper function
    bool Create(fellow::api::module::HardfileDevice hfile) override;

    // Global events
    void HardReset() override;
    void Startup() override;
    void Shutdown() override;

    HardfileHandler();
    ~HardfileHandler();
  };
}

#endif
