/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* Hardfile device                                                         */
/*                                                                         */
/* Authors: Petter Schau                                                   */
/*          Torsten Enderling (carfesh@gmx.net)                            */
/*                                                                         */
/* Copyright (C) 1991, 1992, 1996 Free Software Foundation, Inc.           */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2, or (at your option)     */
/* any later version.                                                      */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software Foundation, */
/* Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.          */
/*=========================================================================*/

#include <sstream>
#include <algorithm>

#include "fellow/api/defs.h"
#include "fellow/api/Services.h"
#include "fellow/api/VM.h"
#include "fellow/api/modules/IHardfileHandler.h"
#include "fellow/hardfile/HardfileHandler.h"
#include "fellow/hardfile/rdb/RDBHandler.h"
#include "fellow/hardfile/hunks/HunkRelocator.h"

using namespace fellow::hardfile::rdb;
using namespace fellow::api::modules;
using namespace fellow::api::vm;
using namespace fellow::api;
using namespace std;

namespace fellow::hardfile
{
  static HardfileHandler *hardfileHandlerInstance = nullptr;

  /*====================*/
  /* fhfile.device      */
  /*====================*/

  /*===================================*/
  /* Fixed locations in fhfile_rom:    */
  /* ------------------------------    */
  /* offset 4088 - max number of units */
  /* offset 4092 - configdev pointer   */
  /*===================================*/

  /*===========================================================================*/
  /* Hardfile card init                                                        */
  /*===========================================================================*/

  // TODO: Should not use hardcoded instance, will have to be done later
  void HardfileHandler_CardInit()
  {
    hardfileHandlerInstance->CardInit();
  }

  void HardfileHandler_CardMap(ULO mapping)
  {
    hardfileHandlerInstance->CardMap(mapping);
  }

  UBY HardfileHandler_ReadByte(ULO address)
  {
    return hardfileHandlerInstance->ReadByte(address);
  }

  UWO HardfileHandler_ReadWord(ULO address)
  {
    return hardfileHandlerInstance->ReadWord(address);
  }

  ULO HardfileHandler_ReadLong(ULO address)
  {
    return hardfileHandlerInstance->ReadLong(address);
  }

  void HardfileHandler_WriteByte(UBY data, ULO address)
  {
    // Deliberately left blank
  }

  void HardfileHandler_WriteWord(UWO data, ULO address)
  {
    // Deliberately left blank
  }

  void HardfileHandler_WriteLong(ULO data, ULO address)
  {
    // Deliberately left blank
  }

  /*================================================================*/
  /* fhfile_card_init                                               */
  /* In order to obtain a configDev struct.                         */
  /*================================================================*/

  void HardfileHandler::CardInit()
  {
    _vm->Memory->EmemSet(0, 0xd1);
    _vm->Memory->EmemSet(8, 0xc0);
    _vm->Memory->EmemSet(4, 2);
    _vm->Memory->EmemSet(0x10, 2011 >> 8);
    _vm->Memory->EmemSet(0x14, 2011 & 0xf);
    _vm->Memory->EmemSet(0x18, 0);
    _vm->Memory->EmemSet(0x1c, 0);
    _vm->Memory->EmemSet(0x20, 0);
    _vm->Memory->EmemSet(0x24, 1);
    _vm->Memory->EmemSet(0x28, 0x10);
    _vm->Memory->EmemSet(0x2c, 0);
    _vm->Memory->EmemSet(0x40, 0);
    _vm->Memory->EmemMirror(0x1000, _rom + 0x1000, 0xa0);
  }

  /*====================================================*/
  /* fhfile_card_map                                    */
  /* The rom must be remapped to the location specified.*/
  /*====================================================*/

  void HardfileHandler::CardMap(ULO mapping)
  {
    _romstart = (mapping << 8) & 0xff0000;
    ULO bank = _romstart >> 16;
    _vm->Memory->BankSet(MemoryBankDescriptor{.Kind = MemoryKind::FellowHardfileROM,
                                            .BankNumber = bank,
                                            .BaseBankNumber = bank,
                                            .ReadByteFunc = HardfileHandler_ReadByte,
                                            .ReadWordFunc = HardfileHandler_ReadWord,
                                            .ReadLongFunc = HardfileHandler_ReadLong,
                                            .WriteByteFunc = HardfileHandler_WriteByte,
                                            .WriteWordFunc = HardfileHandler_WriteWord,
                                            .WriteLongFunc = HardfileHandler_WriteLong,
                                            .BasePointer = _rom,
                                            .IsBasePointerWritable = false});
  }

  /*============================================================================*/
  /* Functions to get and set data in the fhfile memory area                    */
  /*============================================================================*/

  UBY HardfileHandler::ReadByte(ULO address)
  {
    return _rom[address & 0xffff];
  }

  UWO HardfileHandler::ReadWord(ULO address)
  {
    UBY *p = _rom + (address & 0xffff);
    return (static_cast<UWO>(p[0]) << 8) | static_cast<UWO>(p[1]);
  }

  ULO HardfileHandler::ReadLong(ULO address)
  {
    UBY *p = _rom + (address & 0xffff);
    return (static_cast<ULO>(p[0]) << 24) | (static_cast<ULO>(p[1]) << 16) | (static_cast<ULO>(p[2]) << 8) | static_cast<ULO>(p[3]);
  }

  bool HardfileHandler::HasZeroDevices()
  {
    for (auto &_device : _devices)
    {
      if (_device.F != nullptr)
      {
        return false;
      }
    }
    return true;
  }

  bool HardfileHandler::PreferredNameExists(const string &preferredName)
  {
    return std::any_of(_mountList.begin(), _mountList.end(), [preferredName](auto &mountListEntry) { return preferredName == mountListEntry->Name; });
  }

  string HardfileHandler::MakeDeviceName()
  {
    string deviceName;
    do
    {
      ostringstream o;
      o << "DH" << _deviceNameStartNumber++;
      deviceName = o.str();
    } while (PreferredNameExists(deviceName));

    return deviceName;
  }

  string HardfileHandler::MakeDeviceName(const string &preferredName)
  {
    if (preferredName.empty() || PreferredNameExists(preferredName))
    {
      return MakeDeviceName();
    }
    return preferredName;
  }

  void HardfileHandler::CreateMountList()
  {
    _mountList.clear();

    for (unsigned int deviceIndex = 0; deviceIndex < FHFILE_MAX_DEVICES; deviceIndex++)
    {
      if (_devices[deviceIndex].F != nullptr)
      {
        if (_devices[deviceIndex].HasRDB)
        {
          RDB *rdbHeader = _devices[deviceIndex].RDB;

          for (unsigned int partitionIndex = 0; partitionIndex < rdbHeader->Partitions.size(); partitionIndex++)
          {
            RDBPartition *rdbPartition = rdbHeader->Partitions[partitionIndex].get();
            if (rdbPartition->IsAutomountable())
            {
              _mountList.push_back(make_unique<HardfileMountListEntry>(deviceIndex, partitionIndex, MakeDeviceName(rdbPartition->DriveName)));
            }
          }
        }
        else
        {
          _mountList.push_back(make_unique<HardfileMountListEntry>(deviceIndex, -1, MakeDeviceName()));
        }
      }
    }
  }

  bool HardfileHandler::FindOlderOrSameFileSystemVersion(ULO DOSType, ULO version, unsigned int &olderOrSameFileSystemIndex)
  {
    unsigned int size = (unsigned int)_fileSystems.size();
    for (unsigned int index = 0; index < size; index++)
    {
      if (_fileSystems[index]->IsOlderOrSameFileSystemVersion(DOSType, version))
      {
        olderOrSameFileSystemIndex = index;
        return true;
      }
    }
    return false;
  }

  HardfileFileSystemEntry *HardfileHandler::GetFileSystemForDOSType(ULO DOSType)
  {
    for (auto &fileSystemEntry : _fileSystems)
    {
      if (fileSystemEntry->IsDOSType(DOSType))
      {
        return fileSystemEntry.get();
      }
    }
    return nullptr;
  }

  void HardfileHandler::AddFileSystemsFromRdb(HardfileDevice &device)
  {
    if (device.F == nullptr || !device.HasRDB)
    {
      return;
    }

    for (auto &header : device.RDB->FileSystemHeaders)
    {
      unsigned int olderOrSameFileSystemIndex = 0;
      bool hasOlderOrSameFileSystemVersion = FindOlderOrSameFileSystemVersion(header->DOSType, header->Version, olderOrSameFileSystemIndex);
      if (!hasOlderOrSameFileSystemVersion)
      {
        _fileSystems.push_back(make_unique<HardfileFileSystemEntry>(header.get(), 0, _vm->Memory));
      }
      else if (_fileSystems[olderOrSameFileSystemIndex]->IsOlderVersion(header->Version))
      {
        // Replace older fs version with this one
        _fileSystems[olderOrSameFileSystemIndex].reset(new HardfileFileSystemEntry(header.get(), 0, _vm->Memory));
      }
      // Ignore if newer or same fs version already added
    }
  }

  void HardfileHandler::AddFileSystemsFromRdb()
  {
    for (auto &_device : _devices)
    {
      AddFileSystemsFromRdb(_device);
    }
  }

  void HardfileHandler::EraseOlderOrSameFileSystemVersion(ULO DOSType, ULO version)
  {
    unsigned int olderOrSameFileSystemIndex = 0;
    bool hasOlderOrSameFileSystemVersion = FindOlderOrSameFileSystemVersion(DOSType, version, olderOrSameFileSystemIndex);
    if (hasOlderOrSameFileSystemVersion)
    {
      Service->Log.AddLogDebug("fhfile: Erased RDB filesystem entry (%.8X, %.8X), newer version (%.8X, %.8X) found in RDB or newer/same version supported by Kickstart.\n",
                               _fileSystems[olderOrSameFileSystemIndex]->GetDOSType(), _fileSystems[olderOrSameFileSystemIndex]->GetVersion(), DOSType, version);

      _fileSystems.erase(_fileSystems.begin() + olderOrSameFileSystemIndex);
    }
  }

  void HardfileHandler::SetHardfileConfigurationFromRDB(HardfileConfiguration &config, RDB *rdb, bool readonly)
  {
    HardfileGeometry &geometry = config.Geometry;
    geometry.LowCylinder = rdb->LowCylinder;
    geometry.HighCylinder = rdb->HighCylinder;
    geometry.BytesPerSector = rdb->BlockSize;
    geometry.SectorsPerTrack = rdb->SectorsPerTrack;
    geometry.Surfaces = rdb->Heads;
    geometry.Tracks = rdb->Cylinders * rdb->Heads;

    config.Partitions.clear();
    unsigned int partitionCount = (unsigned int)rdb->Partitions.size();
    for (unsigned int i = 0; i < partitionCount; i++)
    {
      fellow::hardfile::rdb::RDBPartition *rdbPartition = rdb->Partitions[i].get();
      HardfilePartition partition;
      partition.PreferredName = rdbPartition->DriveName;
      partition.Geometry.LowCylinder = rdbPartition->LowCylinder;
      partition.Geometry.HighCylinder = rdbPartition->HighCylinder;
      partition.Geometry.BytesPerSector = rdbPartition->SizeBlock * 4;
      partition.Geometry.SectorsPerTrack = rdbPartition->BlocksPerTrack;
      partition.Geometry.Surfaces = rdbPartition->Surfaces;
      partition.Geometry.Tracks = (rdbPartition->HighCylinder - rdbPartition->LowCylinder + 1) * rdbPartition->Surfaces;
      config.Partitions.push_back(partition);
    }
  }

  bool HardfileHandler::OpenHardfileFile(HardfileDevice &device)
  {
    if (device.Configuration.Filename.empty())
    {
      return false;
    }

    fs_wrapper_object_info *fsnp = Service->FSWrapper.GetFSObjectInfo(device.Configuration.Filename.c_str());
    if (fsnp == nullptr)
    {
      Service->Log.AddLog("ERROR: Unable to access hardfile '%s', it is either inaccessible, or too big (2GB or more).\n", device.Configuration.Filename.c_str());
      return false;
    }

    if (fsnp != nullptr)
    {
      device.Readonly = device.Configuration.Readonly || (!fsnp->writeable);
      device.F = fopen(device.Configuration.Filename.c_str(), device.Readonly ? "rb" : "r+b");
      device.FileSize = fsnp->size;
      delete fsnp;
    }

    const auto &geometry = device.Configuration.Geometry;
    ULO cylinderSize = geometry.Surfaces * geometry.SectorsPerTrack * geometry.BytesPerSector;
    if (device.FileSize < cylinderSize)
    {
      fclose(device.F);
      device.F = nullptr;
      Service->Log.AddLog("ERROR: Hardfile '%s' was not mounted, size is less than one cylinder.\n", device.Configuration.Filename.c_str());
      return false;
    }
    return true;
  }

  void HardfileHandler::ClearDeviceRuntimeInfo(HardfileDevice &device)
  {
    if (device.F != nullptr)
    {
      fclose(device.F);
      device.F = nullptr;
    }

    device.Status = fhfile_status::FHFILE_NONE;
    device.FileSize = 0;
    device.GeometrySize = 0;
    device.Readonly = true;

    if (device.RDB != nullptr)
    {
      delete device.RDB;
      device.RDB = nullptr;
      device.HasRDB = false;
    }
  }

  void HardfileHandler::InitializeHardfile(unsigned int index)
  {
    HardfileDevice &device = _devices[index];

    ClearDeviceRuntimeInfo(device);

    if (OpenHardfileFile(device))
    {
      RDBFileReader reader(device.F);
      rdb_status rdbResult = RDBHandler::HasRigidDiskBlock(reader);

      if (rdbResult == rdb_status::RDB_FOUND_WITH_HEADER_CHECKSUM_ERROR)
      {
        ClearDeviceRuntimeInfo(device);

        Service->Log.AddLog("Hardfile: File skipped '%s', RDB header has checksum error.\n", device.Configuration.Filename.c_str());
        return;
      }

      if (rdbResult == rdb_status::RDB_FOUND_WITH_PARTITION_ERROR)
      {
        ClearDeviceRuntimeInfo(device);

        Service->Log.AddLog("Hardfile: File skipped '%s', RDB partition has checksum error.\n", device.Configuration.Filename.c_str());
        return;
      }

      device.HasRDB = rdbResult == rdb_status::RDB_FOUND;

      if (device.HasRDB)
      {
        // RDB configured hardfile
        RDB *rdb = RDBHandler::GetDriveInformation(reader);

        if (rdb->HasFileSystemHandlerErrors)
        {
          ClearDeviceRuntimeInfo(device);

          Service->Log.AddLog("Hardfile: File skipped '%s', RDB filesystem handler has checksum error.\n", device.Configuration.Filename.c_str());
          return;
        }

        device.RDB = rdb;
        SetHardfileConfigurationFromRDB(device.Configuration, device.RDB, device.Readonly);
      }

      HardfileGeometry &geometry = device.Configuration.Geometry;
      if (!device.HasRDB)
      {
        // Manually configured hardfile
        ULO cylinderSize = geometry.Surfaces * geometry.SectorsPerTrack * geometry.BytesPerSector;
        ULO cylinders = device.FileSize / cylinderSize;
        geometry.Tracks = cylinders * geometry.Surfaces;
        geometry.LowCylinder = 0;
        geometry.HighCylinder = cylinders - 1;
      }
      device.GeometrySize = geometry.Tracks * geometry.SectorsPerTrack * geometry.BytesPerSector;
      device.Status = fhfile_status::FHFILE_HDF;

      if (device.FileSize < device.GeometrySize)
      {
        ClearDeviceRuntimeInfo(device);

        Service->Log.AddLog("Hardfile: File skipped, geometry for %s is larger than the file.\n", device.Configuration.Filename.c_str());
      }
    }
  }

  /* Returns true if a hardfile was previously inserted */

  bool HardfileHandler::RemoveHardfile(unsigned int index)
  {
    if (index >= GetMaxHardfileCount())
    {
      return false;
    }

    return _devices[index].Clear();
  }

  void HardfileHandler::SetEnabled(bool enabled)
  {
    _enabled = enabled;
  }

  bool HardfileHandler::GetEnabled()
  {
    return _enabled;
  }

  unsigned int HardfileHandler::GetIndexFromUnitNumber(ULO unit)
  {
    ULO address = unit % 10;
    ULO lun = (unit / 10) % 10;

    if (lun > 7)
    {
      Service->Log.AddLogDebug("ERROR: Unit number is not in a valid format.\n");
      return 0xffffffff;
    }
    return lun + address * 8;
  }

  ULO HardfileHandler::GetUnitNumberFromIndex(unsigned int index)
  {
    ULO address = index / 8;
    ULO lun = index % 8;

    return lun * 10 + address;
  }
  void HardfileHandler::SetHardfile(const HardfileConfiguration &configuration, unsigned int index)
  {
    if (index >= GetMaxHardfileCount())
    {
      return;
    }

    RemoveHardfile(index);
    HardfileDevice &device = _devices[index];
    device.Configuration = configuration;
    InitializeHardfile(index);

    Service->Log.AddLog("SetHardfile('%s', %u)\n", configuration.Filename.c_str(), index);

    Service->RP.SendHardDriveContent(index, configuration.Filename.c_str(), configuration.Readonly);
  }

  bool HardfileHandler::CompareHardfile(const HardfileConfiguration &configuration, unsigned int index)
  {
    if (index >= GetMaxHardfileCount())
    {
      return false;
    }

    return _devices[index].Configuration == configuration;
  }

  void HardfileHandler::Clear()
  {
    for (unsigned int i = 0; i < FHFILE_MAX_DEVICES; i++)
    {
      RemoveHardfile(i);
    }
    _fileSystems.clear();
    _mountList.clear();
    _deviceNameStartNumber = 0;
  }

  void HardfileHandler::SetIOError(BYT errorCode)
  {
    _vm->Memory->WriteByte(errorCode, _vm->CPU->GetAReg(1) + 31);
  }

  void HardfileHandler::SetIOActual(ULO ioActual)
  {
    _vm->Memory->WriteLong(ioActual, _vm->CPU->GetAReg(1) + 32);
  }

  ULO HardfileHandler::GetUnitNumber()
  {
    return _vm->Memory->ReadLong(_vm->CPU->GetAReg(1) + 24);
  }

  UWO HardfileHandler::GetCommand()
  {
    return _vm->Memory->ReadWord(_vm->CPU->GetAReg(1) + 28);
  }

  /*==================*/
  /* BeginIO Commands */
  /*==================*/

  void HardfileHandler::IgnoreOK(ULO index)
  {
    SetIOError(0); // io_Error - 0 - success
  }

  BYT HardfileHandler::Read(ULO index)
  {
    if (index == 2)
    {
      int ihj = 0;
    }

    if (_devices[index].F == nullptr)
    {
      Service->Log.AddLogDebug("CMD_READ Unit %d (%d) ERROR-TDERR_BadUnitNum\n", GetUnitNumberFromIndex(index), index);
      return 32; // TDERR_BadUnitNum
    }

    ULO dest = _vm->Memory->ReadLong(_vm->CPU->GetAReg(1) + 40);
    ULO offset = _vm->Memory->ReadLong(_vm->CPU->GetAReg(1) + 44);
    ULO length = _vm->Memory->ReadLong(_vm->CPU->GetAReg(1) + 36);

    Service->Log.AddLogDebug("CMD_READ Unit %d (%d) Destination %.8X Offset %.8X Length %.8X\n", GetUnitNumberFromIndex(index), index, dest, offset, length);

    if ((offset + length) > _devices[index].GeometrySize)
    {
      return -3; // TODO: Out of range, -3 is not the right code
    }

    Service->HUD.NotifyHarddiskLEDChanged(index, true, false);

    fseek(_devices[index].F, offset, SEEK_SET);
    fread(_vm->Memory->AddressToPtr(dest), 1, length, _devices[index].F);
    SetIOActual(length);

    Service->HUD.NotifyHarddiskLEDChanged(index, false, false);

    return 0;
  }

  BYT HardfileHandler::Write(ULO index)
  {
    if (_devices[index].F == nullptr)
    {
      Service->Log.AddLogDebug("CMD_WRITE Unit %d (%d) ERROR-TDERR_BadUnitNum\n", GetUnitNumberFromIndex(index), index);
      return 32; // TDERR_BadUnitNum
    }

    ULO dest = _vm->Memory->ReadLong(_vm->CPU->GetAReg(1) + 40);
    ULO offset = _vm->Memory->ReadLong(_vm->CPU->GetAReg(1) + 44);
    ULO length = _vm->Memory->ReadLong(_vm->CPU->GetAReg(1) + 36);

    Service->Log.AddLogDebug("CMD_WRITE Unit %d (%d) Destination %.8X Offset %.8X Length %.8X\n", GetUnitNumberFromIndex(index), index, dest, offset, length);

    if (_devices[index].Readonly || (offset + length) > _devices[index].GeometrySize)
    {
      return -3; // TODO: Out of range, -3 is probably not the right one.
    }

    Service->HUD.NotifyHarddiskLEDChanged(index, true, true);

    fseek(_devices[index].F, offset, SEEK_SET);
    fwrite(_vm->Memory->AddressToPtr(dest), 1, length, _devices[index].F);
    SetIOActual(length);

    Service->HUD.NotifyHarddiskLEDChanged(index, false, true);

    return 0;
  }

  BYT HardfileHandler::GetNumberOfTracks(ULO index)
  {
    if (_devices[index].F == nullptr)
    {
      return 32; // TDERR_BadUnitNum
    }

    SetIOActual(_devices[index].Configuration.Geometry.Tracks);
    return 0;
  }

  BYT HardfileHandler::GetDiskDriveType(ULO index)
  {
    if (_devices[index].F == nullptr)
    {
      return 32; // TDERR_BadUnitNum
    }

    SetIOActual(1);

    return 0;
  }

  void HardfileHandler::WriteProt(ULO index)
  {
    SetIOActual(_devices[index].Readonly ? 1 : 0);
  }

  BYT HardfileHandler::ScsiDirect(ULO index)
  {
    BYT error = 0;
    ULO scsiCmdStruct = _vm->Memory->ReadLong(_vm->CPU->GetAReg(1) + 40); // io_Data

    Service->Log.AddLogDebug("HD_SCSICMD Unit %d (%d) ScsiCmd at %.8X\n", GetUnitNumberFromIndex(index), index, scsiCmdStruct);

    ULO scsiCommand = _vm->Memory->ReadLong(scsiCmdStruct + 12);
    UWO scsiCommandLength = _vm->Memory->ReadWord(scsiCmdStruct + 16);

    Service->Log.AddLogDebug("HD_SCSICMD Command length %d, data", scsiCommandLength);

    for (int i = 0; i < scsiCommandLength; i++)
    {
      Service->Log.AddLogDebug(" %.2X", _vm->Memory->ReadByte(scsiCommand + i));
    }
    Service->Log.AddLogDebug("\n");

    UBY commandNumber = _vm->Memory->ReadByte(scsiCommand);
    ULO returnData = _vm->Memory->ReadLong(scsiCmdStruct);
    switch (commandNumber)
    {
      case 0x25: // Read capacity (10)
        Service->Log.AddLogDebug("SCSI direct command 0x25 Read Capacity\n");
        {
          ULO bytesPerSector = _devices[index].Configuration.Geometry.BytesPerSector;
          bool pmi = !!(_vm->Memory->ReadByte(scsiCommand + 8) & 1);

          if (pmi)
          {
            ULO blocksPerCylinder = (_devices[index].Configuration.Geometry.SectorsPerTrack * _devices[index].Configuration.Geometry.Surfaces) - 1;
            _vm->Memory->WriteByte(blocksPerCylinder >> 24, returnData);
            _vm->Memory->WriteByte(blocksPerCylinder >> 16, returnData + 1);
            _vm->Memory->WriteByte(blocksPerCylinder >> 8, returnData + 2);
            _vm->Memory->WriteByte(blocksPerCylinder, returnData + 3);
          }
          else
          {
            ULO blocksOnDevice = (_devices[index].GeometrySize / _devices[index].Configuration.Geometry.BytesPerSector) - 1;
            _vm->Memory->WriteByte(blocksOnDevice >> 24, returnData);
            _vm->Memory->WriteByte(blocksOnDevice >> 16, returnData + 1);
            _vm->Memory->WriteByte(blocksOnDevice >> 8, returnData + 2);
            _vm->Memory->WriteByte(blocksOnDevice, returnData + 3);
          }
          _vm->Memory->WriteByte(bytesPerSector >> 24, returnData + 4);
          _vm->Memory->WriteByte(bytesPerSector >> 16, returnData + 5);
          _vm->Memory->WriteByte(bytesPerSector >> 8, returnData + 6);
          _vm->Memory->WriteByte(bytesPerSector, returnData + 7);

          _vm->Memory->WriteByte(0, scsiCmdStruct + 8); // data actual length (long word)
          _vm->Memory->WriteByte(0, scsiCmdStruct + 9);
          _vm->Memory->WriteByte(0, scsiCmdStruct + 10);
          _vm->Memory->WriteByte(8, scsiCmdStruct + 11);

          _vm->Memory->WriteByte(0, scsiCmdStruct + 21); // Status
        }
        break;
      case 0x37: // Read defect Data (10)
        Service->Log.AddLogDebug("SCSI direct command 0x37 Read defect Data\n");

        _vm->Memory->WriteByte(0, returnData);
        _vm->Memory->WriteByte(_vm->Memory->ReadByte(scsiCommand + 2), returnData + 1);
        _vm->Memory->WriteByte(0, returnData + 2); // No defects (word)
        _vm->Memory->WriteByte(0, returnData + 3);

        _vm->Memory->WriteByte(0, scsiCmdStruct + 8); // data actual length (long word)
        _vm->Memory->WriteByte(0, scsiCmdStruct + 9);
        _vm->Memory->WriteByte(0, scsiCmdStruct + 10);
        _vm->Memory->WriteByte(4, scsiCmdStruct + 11);

        _vm->Memory->WriteByte(0, scsiCmdStruct + 21); // Status
        break;
      case 0x12: // Inquiry
        Service->Log.AddLogDebug("SCSI direct command 0x12 Inquiry\n");

        _vm->Memory->WriteByte(0, returnData);     // Pheripheral type 0 connected (magnetic disk)
        _vm->Memory->WriteByte(0, returnData + 1); // Not removable
        _vm->Memory->WriteByte(0, returnData + 2); // Does not claim conformance to any standard
        _vm->Memory->WriteByte(2, returnData + 3);
        _vm->Memory->WriteByte(32, returnData + 4); // Additional length
        _vm->Memory->WriteByte(0, returnData + 5);
        _vm->Memory->WriteByte(0, returnData + 6);
        _vm->Memory->WriteByte(0, returnData + 7);
        _vm->Memory->WriteByte('F', returnData + 8);
        _vm->Memory->WriteByte('E', returnData + 9);
        _vm->Memory->WriteByte('L', returnData + 10);
        _vm->Memory->WriteByte('L', returnData + 11);
        _vm->Memory->WriteByte('O', returnData + 12);
        _vm->Memory->WriteByte('W', returnData + 13);
        _vm->Memory->WriteByte(' ', returnData + 14);
        _vm->Memory->WriteByte(' ', returnData + 15);
        _vm->Memory->WriteByte('H', returnData + 16);
        _vm->Memory->WriteByte('a', returnData + 17);
        _vm->Memory->WriteByte('r', returnData + 18);
        _vm->Memory->WriteByte('d', returnData + 19);
        _vm->Memory->WriteByte('f', returnData + 20);
        _vm->Memory->WriteByte('i', returnData + 21);
        _vm->Memory->WriteByte('l', returnData + 22);
        _vm->Memory->WriteByte('e', returnData + 23);
        _vm->Memory->WriteByte(' ', returnData + 24);
        _vm->Memory->WriteByte(' ', returnData + 25);
        _vm->Memory->WriteByte(' ', returnData + 26);
        _vm->Memory->WriteByte(' ', returnData + 27);
        _vm->Memory->WriteByte(' ', returnData + 28);
        _vm->Memory->WriteByte(' ', returnData + 29);
        _vm->Memory->WriteByte(' ', returnData + 30);
        _vm->Memory->WriteByte(' ', returnData + 31);
        _vm->Memory->WriteByte('1', returnData + 32);
        _vm->Memory->WriteByte('.', returnData + 33);
        _vm->Memory->WriteByte('0', returnData + 34);
        _vm->Memory->WriteByte(' ', returnData + 35);

        _vm->Memory->WriteByte(0, scsiCmdStruct + 8); // data actual length (long word)
        _vm->Memory->WriteByte(0, scsiCmdStruct + 9);
        _vm->Memory->WriteByte(0, scsiCmdStruct + 10);
        _vm->Memory->WriteByte(36, scsiCmdStruct + 11);

        _vm->Memory->WriteByte(0, scsiCmdStruct + 21); // Status
        break;
      case 0x1a: // Mode sense
        Service->Log.AddLogDebug("SCSI direct command 0x1a Mode sense\n");
        {
          // Show values for debug
          ULO senseData = _vm->Memory->ReadLong(scsiCmdStruct + 22);            // senseData and related fields are only used for autosensing error condition when the command fail
          UWO senseLengthAllocated = _vm->Memory->ReadWord(scsiCmdStruct + 26); // Primary mode sense data go to scsi_Data
          UBY scsciCommandFlags = _vm->Memory->ReadByte(scsiCmdStruct + 20);

          UBY pageCode = _vm->Memory->ReadByte(scsiCommand + 2) & 0x3f;
          if (pageCode == 3)
          {
            UWO sectorsPerTrack = _devices[index].Configuration.Geometry.SectorsPerTrack;
            UWO bytesPerSector = _devices[index].Configuration.Geometry.BytesPerSector;

            // Header
            _vm->Memory->WriteByte(24 + 3, returnData);
            _vm->Memory->WriteByte(0, returnData + 1);
            _vm->Memory->WriteByte(0, returnData + 2);
            _vm->Memory->WriteByte(0, returnData + 3);

            // Page
            ULO destination = returnData + 4;
            _vm->Memory->WriteByte(3, destination);        // Page 3 format device
            _vm->Memory->WriteByte(0x16, destination + 1); // Page length
            _vm->Memory->WriteByte(0, destination + 2);    // Tracks per zone
            _vm->Memory->WriteByte(1, destination + 3);
            _vm->Memory->WriteByte(0, destination + 4); // Alternate sectors per zone
            _vm->Memory->WriteByte(0, destination + 5);
            _vm->Memory->WriteByte(0, destination + 6); // Alternate tracks per zone
            _vm->Memory->WriteByte(0, destination + 7);
            _vm->Memory->WriteByte(0, destination + 8); // Alternate tracks per volume
            _vm->Memory->WriteByte(0, destination + 9);
            _vm->Memory->WriteByte(sectorsPerTrack >> 8, destination + 10); // Sectors per track
            _vm->Memory->WriteByte(sectorsPerTrack & 0xff, destination + 11);
            _vm->Memory->WriteByte(bytesPerSector >> 8, destination + 12); // Data bytes per physical sector
            _vm->Memory->WriteByte(bytesPerSector & 0xff, destination + 13);
            _vm->Memory->WriteByte(0, destination + 14); // Interleave
            _vm->Memory->WriteByte(1, destination + 15);
            _vm->Memory->WriteByte(0, destination + 16); // Track skew factor
            _vm->Memory->WriteByte(0, destination + 17);
            _vm->Memory->WriteByte(0, destination + 18); // Cylinder skew factor
            _vm->Memory->WriteByte(0, destination + 19);
            _vm->Memory->WriteByte(0x80, destination + 20);
            _vm->Memory->WriteByte(0, destination + 21);
            _vm->Memory->WriteByte(0, destination + 22);
            _vm->Memory->WriteByte(0, destination + 23);

            _vm->Memory->WriteByte(0, scsiCmdStruct + 8); // data actual length (long word)
            _vm->Memory->WriteByte(0, scsiCmdStruct + 9);
            _vm->Memory->WriteByte(0, scsiCmdStruct + 10);
            _vm->Memory->WriteByte(24 + 4, scsiCmdStruct + 11);

            _vm->Memory->WriteByte(0, scsiCmdStruct + 28); // sense actual length (word)
            _vm->Memory->WriteByte(0, scsiCmdStruct + 29);
            _vm->Memory->WriteByte(0, scsiCmdStruct + 21); // Status
          }
          else if (pageCode == 4)
          {
            ULO numberOfCylinders = _devices[index].Configuration.Geometry.HighCylinder + 1;
            UBY surfaces = _devices[index].Configuration.Geometry.Surfaces;

            // Header
            _vm->Memory->WriteByte(24 + 3, returnData);
            _vm->Memory->WriteByte(0, returnData + 1);
            _vm->Memory->WriteByte(0, returnData + 2);
            _vm->Memory->WriteByte(0, returnData + 3);

            // Page
            ULO destination = returnData + 4;
            _vm->Memory->WriteByte(4, destination);                           // Page 4 Rigid disk geometry
            _vm->Memory->WriteByte(0x16, destination + 1);                    // Page length
            _vm->Memory->WriteByte(numberOfCylinders >> 16, destination + 2); // Number of cylinders (3 bytes)
            _vm->Memory->WriteByte(numberOfCylinders >> 8, destination + 3);
            _vm->Memory->WriteByte(numberOfCylinders, destination + 4);
            _vm->Memory->WriteByte(surfaces, destination + 5); // Number of heads
            _vm->Memory->WriteByte(0, destination + 6);        // Starting cylinder write precomp (3 bytes)
            _vm->Memory->WriteByte(0, destination + 7);
            _vm->Memory->WriteByte(0, destination + 8);
            _vm->Memory->WriteByte(0, destination + 9); // Starting cylinder reduces write current (3 bytes)
            _vm->Memory->WriteByte(0, destination + 10);
            _vm->Memory->WriteByte(0, destination + 11);
            _vm->Memory->WriteByte(0, destination + 12); // Drive step rate
            _vm->Memory->WriteByte(0, destination + 13);
            _vm->Memory->WriteByte(numberOfCylinders >> 16, destination + 14); // Landing zone cylinder (3 bytes)
            _vm->Memory->WriteByte(numberOfCylinders >> 8, destination + 15);
            _vm->Memory->WriteByte(numberOfCylinders, destination + 16);
            _vm->Memory->WriteByte(0, destination + 17);    // Nothing
            _vm->Memory->WriteByte(0, destination + 18);    // Rotational offset
            _vm->Memory->WriteByte(0, destination + 19);    // Reserved
            _vm->Memory->WriteByte(0x1c, destination + 20); // Medium rotation rate
            _vm->Memory->WriteByte(0x20, destination + 21);
            _vm->Memory->WriteByte(0, destination + 22); // Reserved
            _vm->Memory->WriteByte(0, destination + 23); // Reserved

            _vm->Memory->WriteByte(0, scsiCmdStruct + 8); // data actual length (long word)
            _vm->Memory->WriteByte(0, scsiCmdStruct + 9);
            _vm->Memory->WriteByte(0, scsiCmdStruct + 10);
            _vm->Memory->WriteByte(24 + 4, scsiCmdStruct + 11);

            _vm->Memory->WriteByte(0, scsiCmdStruct + 28); // sense actual length (word)
            _vm->Memory->WriteByte(0, scsiCmdStruct + 29);
            _vm->Memory->WriteByte(0, scsiCmdStruct + 21); // Status
          }
          else
          {
            error = -3;
          }
        }
        break;
      default:
        Service->Log.AddLogDebug("SCSI direct command Unimplemented 0x%.2X\n", commandNumber);

        error = -3;
        break;
    }
    return error;
  }

  unsigned int HardfileHandler::GetMaxHardfileCount()
  {
    return FHFILE_MAX_DEVICES;
  }

  /*======================================================*/
  /* fhfileDiag native callback                           */
  /*                                                      */
  /* Pointer to our configdev struct is stored in $f40ffc */
  /* For later use when filling out the bootnode          */
  /*======================================================*/

  void HardfileHandler::DoDiag()
  {
    // Re-read hardfile configuration from disk. Because this could be an in-emulation initiated reset,
    // we can't be sure we have the latest RDB info for our mountlists. (ie. HDToolBox partition changes.)
    RebuildHardfileConfiguration();

    CreateDOSDevPackets(_devicename);

    _configdev = _vm->CPU->GetAReg(3);
    _vm->Memory->DmemSetLongNoCounter(FHFILE_MAX_DEVICES, 4088);
    _vm->Memory->DmemSetLongNoCounter(_configdev, 4092);
    _vm->CPU->SetDReg(0, 1);
  }

  /*======================================*/
  /* Native callbacks for device commands */
  /*======================================*/

  // Returns D0 - 0 - Success, non-zero - Error
  void HardfileHandler::DoOpen()
  {
    ULO unit = _vm->CPU->GetDReg(0);
    unsigned int index = GetIndexFromUnitNumber(unit);

    if (index < FHFILE_MAX_DEVICES && _devices[index].F != nullptr)
    {
      _vm->Memory->WriteByte(7, _vm->CPU->GetAReg(1) + 8);                                                 /* ln_type (NT_REPLYMSG) */
      SetIOError(0);                                                                                   /* io_error */
      _vm->Memory->WriteLong(_vm->CPU->GetDReg(0), _vm->CPU->GetAReg(1) + 24);                               /* io_unit */
      _vm->Memory->WriteLong(_vm->Memory->ReadLong(_vm->CPU->GetAReg(6) + 32) + 1, _vm->CPU->GetAReg(6) + 32); /* LIB_OPENCNT */
      _vm->CPU->SetDReg(0, 0);                                                                           /* Success */
    }
    else
    {
      _vm->Memory->WriteLong(static_cast<ULO>(-1), _vm->CPU->GetAReg(1) + 20);
      SetIOError(-1);                           /* io_error */
      _vm->CPU->SetDReg(0, static_cast<ULO>(-1)); /* Fail */
    }
  }

  void HardfileHandler::DoClose()
  {
    _vm->Memory->WriteLong(_vm->Memory->ReadLong(_vm->CPU->GetAReg(6) + 32) - 1, _vm->CPU->GetAReg(6) + 32); /* LIB_OPENCNT */
    _vm->CPU->SetDReg(0, 0);                                                                           /* Causes invalid free-mem entry recoverable alert if omitted */
  }

  void HardfileHandler::DoExpunge()
  {
    _vm->CPU->SetDReg(0, 0); /* ? */
  }

  void HardfileHandler::DoNULL()
  {
    _vm->CPU->SetDReg(0, 0); /* ? */
  }

  // void BeginIO(io_req)
  void HardfileHandler::DoBeginIO()
  {
    BYT error = 0;
    ULO unit = GetUnitNumber();
    unsigned int index = GetIndexFromUnitNumber(unit);
    UWO cmd = GetCommand();

    switch (cmd)
    {
      case 2: // CMD_READ
        error = Read(index);
        break;
      case 3: // CMD_WRITE
        error = Write(index);
        break;
      case 11: // TD_FORMAT
        Service->Log.AddLogDebug("TD_FORMAT Unit %d\n", unit);
        error = Write(index);
        break;
      case 18: // TD_GETDRIVETYPE
        Service->Log.AddLogDebug("TD_GETDRIVETYPE Unit %d\n", unit);
        // This does not make sense for us, options are 3.5" and 5 1/4"
        error = GetDiskDriveType(index);
        break;
      case 19: // TD_GETNUMTRACKS
        Service->Log.AddLogDebug("TD_GETNUMTRACKS Unit %d\n", unit);
        error = GetNumberOfTracks(index);
        break;
      case 15: // TD_PROTSTATUS
        Service->Log.AddLogDebug("TD_PROTSTATUS Unit %d\n", unit);
        WriteProt(index);
        break;
      case 4: // CMD_UPDATE ie. flush
        Service->Log.AddLogDebug("CMD_UPDATE Unit %d\n", unit);
        IgnoreOK(index);
        break;
      case 5: // CMD_CLEAR
        Service->Log.AddLogDebug("CMD 5 Unit %d\n", unit);
        IgnoreOK(index);
        break;
      case 9: // TD_MOTOR Only really used to turn motor off since device will turn on the motor automatically if reads and writes are received
        Service->Log.AddLogDebug("TD_MOTOR Unit %d\n", unit);
        // Should set previous state of motor
        IgnoreOK(index);
        break;
      case 10: // TD_SEEK Used to pre-seek in advance, but reads and writes will also do the necessary seeks, so not useful here.
        Service->Log.AddLogDebug("TD_SEEK Unit %d\n", unit);
        IgnoreOK(index);
        break;
      case 12: // TD_REMOVE
        Service->Log.AddLogDebug("TD_REMOVE Unit %d\n", unit);
        // Perhaps unsupported instead?
        IgnoreOK(index);
        break;
      case 13: // TD_CHANGENUM
        Service->Log.AddLogDebug("TD_CHANGENUM Unit %d\n", unit);
        // Should perhaps set some data here
        IgnoreOK(index);
        break;
      case 14: // TD_CHANGESTATE - check if a disk is currently in a drive. io_Actual - 0 - disk, 1 - no disk
        Service->Log.AddLogDebug("TD_CHANGESTATE Unit %d\n", unit);
        SetIOError(0); // io_Error - 0 - success
        SetIOActual(0);
        break;
      case 20: // TD_ADDCHANGEINT
        Service->Log.AddLogDebug("TD_ADDCHANGEINT Unit %d\n", unit);
        IgnoreOK(index);
        break;
      case 21: // TD_REMCHANGEINT
        Service->Log.AddLogDebug("TD_REMCHANGEINT Unit %d\n", unit);
        IgnoreOK(index);
        break;
      case 16: // TD_RAWREAD
        Service->Log.AddLogDebug("TD_RAWREAD Unit %d\n", unit);
        error = -3;
        break;
      case 17: // TD_RAWWRITE
        Service->Log.AddLogDebug("TD_RAWWRITE Unit %d\n", unit);
        error = -3;
        break;
      case 22: // TD_GETGEOMETRY
        Service->Log.AddLogDebug("TD_GEOMETRY Unit %d\n", unit);
        error = -3;
        break;
      case 23: // TD_EJECT
        Service->Log.AddLogDebug("TD_EJECT Unit %d\n", unit);
        error = -3;
        break;
      case 28: error = ScsiDirect(index); break;
      default:
        Service->Log.AddLogDebug("CMD Unknown %d Unit %d\n", cmd, unit);
        error = -3;
        break;
    }
    _vm->Memory->WriteByte(5, _vm->CPU->GetAReg(1) + 8); /* ln_type (Reply message) */
    SetIOError(error);                               // ln_error
  }

  void HardfileHandler::DoAbortIO()
  {
    // Set some more success values here, this is ok, we never have pending io.
    _vm->CPU->SetDReg(0, static_cast<ULO>(-3));
  }

  // RDB support functions, native callbacks

  ULO HardfileHandler::DoGetRDBFileSystemCount()
  {
    ULO count = (ULO)_fileSystems.size();
    Service->Log.AddLogDebug("fhfile: DoGetRDBFilesystemCount() - Returns %u\n", count);
    return count;
  }

  ULO HardfileHandler::DoGetRDBFileSystemHunkCount(ULO fileSystemIndex)
  {
    ULO hunkCount = (ULO)_fileSystems[fileSystemIndex]->Header->FileSystemHandler.FileImage.GetInitialHunkCount();
    Service->Log.AddLogDebug("fhfile: DoGetRDBFileSystemHunkCount(fileSystemIndex: %u) Returns %u\n", fileSystemIndex, hunkCount);
    return hunkCount;
  }

  ULO HardfileHandler::DoGetRDBFileSystemHunkSize(ULO fileSystemIndex, ULO hunkIndex)
  {
    ULO hunkSize = _fileSystems[fileSystemIndex]->Header->FileSystemHandler.FileImage.GetInitialHunk(hunkIndex)->GetAllocateSizeInBytes();
    Service->Log.AddLogDebug("fhfile: DoGetRDBFileSystemHunkSize(fileSystemIndex: %u, hunkIndex: %u) Returns %u\n", fileSystemIndex, hunkIndex, hunkSize);
    return hunkSize;
  }

  void HardfileHandler::DoCopyRDBFileSystemHunk(ULO destinationAddress, ULO fileSystemIndex, ULO hunkIndex)
  {
    Service->Log.AddLogDebug("fhfile: DoCopyRDBFileSystemHunk(destinationAddress: %.8X, fileSystemIndex: %u, hunkIndex: %u)\n", destinationAddress, fileSystemIndex, hunkIndex);

    HardfileFileSystemEntry *fileSystem = _fileSystems[fileSystemIndex].get();
    fileSystem->CopyHunkToAddress(destinationAddress + 8, hunkIndex);

    // Remember the address to the first hunk
    if (fileSystem->SegListAddress == 0)
    {
      fileSystem->SegListAddress = destinationAddress + 4;
    }

    ULO hunkSize = fileSystem->Header->FileSystemHandler.FileImage.GetInitialHunk(hunkIndex)->GetAllocateSizeInBytes();
    _vm->Memory->WriteLong(hunkSize + 8, destinationAddress); // Total size of allocation
    _vm->Memory->WriteLong(0, destinationAddress + 4);        // No next segment for now
  }

  void HardfileHandler::DoRelocateFileSystem(ULO fileSystemIndex)
  {
    Service->Log.AddLogDebug("fhfile: DoRelocateFileSystem(fileSystemIndex: %u\n", fileSystemIndex);
    HardfileFileSystemEntry *fsEntry = _fileSystems[fileSystemIndex].get();
    fellow::hardfile::hunks::HunkRelocator hunkRelocator(fsEntry->Header->FileSystemHandler.FileImage, _vm->Memory);
    hunkRelocator.RelocateHunks();
  }

  void HardfileHandler::DoInitializeRDBFileSystemEntry(ULO fileSystemEntry, ULO fileSystemIndex)
  {
    Service->Log.AddLogDebug("fhfile: DoInitializeRDBFileSystemEntry(fileSystemEntry: %.8X, fileSystemIndex: %u\n", fileSystemEntry, fileSystemIndex);

    HardfileFileSystemEntry *fsEntry = _fileSystems[fileSystemIndex].get();
    RDBFileSystemHeader *fsHeader = fsEntry->Header;

    _vm->Memory->WriteLong(_fsname, fileSystemEntry + 10);
    _vm->Memory->WriteLong(fsHeader->DOSType, fileSystemEntry + 14);
    _vm->Memory->WriteLong(fsHeader->Version, fileSystemEntry + 18);
    _vm->Memory->WriteLong(fsHeader->PatchFlags, fileSystemEntry + 22);
    _vm->Memory->WriteLong(fsHeader->DnType, fileSystemEntry + 26);
    _vm->Memory->WriteLong(fsHeader->DnTask, fileSystemEntry + 30);
    _vm->Memory->WriteLong(fsHeader->DnLock, fileSystemEntry + 34);
    _vm->Memory->WriteLong(fsHeader->DnHandler, fileSystemEntry + 38);
    _vm->Memory->WriteLong(fsHeader->DnStackSize, fileSystemEntry + 42);
    _vm->Memory->WriteLong(fsHeader->DnPriority, fileSystemEntry + 46);
    _vm->Memory->WriteLong(fsHeader->DnStartup, fileSystemEntry + 50);
    _vm->Memory->WriteLong(fsEntry->SegListAddress >> 2, fileSystemEntry + 54);
    //  _vm->Memory->WriteLong(fsHeader.DnSegListBlock, fileSystemEntry + 54);
    _vm->Memory->WriteLong(fsHeader->DnGlobalVec, fileSystemEntry + 58);

    for (int i = 0; i < 23; i++)
    {
      _vm->Memory->WriteLong(fsHeader->Reserved2[i], fileSystemEntry + 62 + i * 4);
    }
  }

  ULO HardfileHandler::DoGetDosDevPacketListStart()
  {
    return _dosDevPacketListStart;
  }

  string HardfileHandler::LogGetStringFromMemory(ULO address)
  {
    if (address == 0)
    {
      return string();
    }

    string name;
    char c = _vm->Memory->ReadByte(address);
    address++;
    while (c != 0)
    {
      name.push_back(c == '\n' ? '.' : c);
      c = _vm->Memory->ReadByte(address);
      address++;
    }
    return name;
  }

  void HardfileHandler::DoLogAvailableResources()
  {
    Service->Log.AddLogDebug("fhfile: DoLogAvailableResources()\n");

    ULO execBase = _vm->Memory->ReadLong(4); // Fetch list from exec
    ULO rsListHeader = _vm->Memory->ReadLong(execBase + 0x150);

    Service->Log.AddLogDebug("fhfile: Resource list header (%.8X): Head %.8X Tail %.8X TailPred %.8X Type %d\n", rsListHeader, _vm->Memory->ReadLong(rsListHeader), _vm->Memory->ReadLong(rsListHeader + 4),
                             _vm->Memory->ReadLong(rsListHeader + 8), _vm->Memory->ReadByte(rsListHeader + 9));

    if (rsListHeader == _vm->Memory->ReadLong(rsListHeader + 8))
    {
      Service->Log.AddLogDebug("fhfile: Resource list is empty.\n");
      return;
    }

    ULO fsNode = _vm->Memory->ReadLong(rsListHeader);
    while (fsNode != 0 && (fsNode != rsListHeader + 4))
    {
      Service->Log.AddLogDebug("fhfile: ResourceEntry Node (%.8X): Succ %.8X Pred %.8X Type %d Pri %d NodeName '%s'\n", fsNode, _vm->Memory->ReadLong(fsNode), _vm->Memory->ReadLong(fsNode + 4),
                               _vm->Memory->ReadByte(fsNode + 8), _vm->Memory->ReadByte(fsNode + 9), LogGetStringFromMemory(_vm->Memory->ReadLong(fsNode + 10)).c_str());

      fsNode = _vm->Memory->ReadLong(fsNode);
    }
  }

  void HardfileHandler::DoLogAllocMemResult(ULO result)
  {
    Service->Log.AddLogDebug("fhfile: AllocMem() returned %.8X\n", result);
  }

  void HardfileHandler::DoLogOpenResourceResult(ULO result)
  {
    Service->Log.AddLogDebug("fhfile: OpenResource() returned %.8X\n", result);
  }

  void HardfileHandler::DoRemoveRDBFileSystemsAlreadySupportedBySystem(ULO filesystemResource)
  {
    Service->Log.AddLogDebug("fhfile: DoRemoveRDBFileSystemsAlreadySupportedBySystem(filesystemResource: %.8X)\n", filesystemResource);

    ULO fsList = filesystemResource + 18;
    if (fsList == _vm->Memory->ReadLong(fsList + 8))
    {
      Service->Log.AddLogDebug("fhfile: FileSystemEntry list is empty.\n");
      return;
    }

    ULO fsNode = _vm->Memory->ReadLong(fsList);
    while (fsNode != 0 && (fsNode != fsList + 4))
    {
      ULO fsEntry = fsNode + 14;
      ULO dosType = _vm->Memory->ReadLong(fsEntry);
      ULO version = _vm->Memory->ReadLong(fsEntry + 4);

      Service->Log.AddLogDebug("fhfile: FileSystemEntry DosType   : %.8X\n", _vm->Memory->ReadLong(fsEntry));
      Service->Log.AddLogDebug("fhfile: FileSystemEntry Version   : %.8X\n", _vm->Memory->ReadLong(fsEntry + 4));
      Service->Log.AddLogDebug("fhfile: FileSystemEntry PatchFlags: %.8X\n", _vm->Memory->ReadLong(fsEntry + 8));
      Service->Log.AddLogDebug("fhfile: FileSystemEntry Type      : %.8X\n", _vm->Memory->ReadLong(fsEntry + 12));
      Service->Log.AddLogDebug("fhfile: FileSystemEntry Task      : %.8X\n", _vm->Memory->ReadLong(fsEntry + 16));
      Service->Log.AddLogDebug("fhfile: FileSystemEntry Lock      : %.8X\n", _vm->Memory->ReadLong(fsEntry + 20));
      Service->Log.AddLogDebug("fhfile: FileSystemEntry Handler   : %.8X\n", _vm->Memory->ReadLong(fsEntry + 24));
      Service->Log.AddLogDebug("fhfile: FileSystemEntry StackSize : %.8X\n", _vm->Memory->ReadLong(fsEntry + 28));
      Service->Log.AddLogDebug("fhfile: FileSystemEntry Priority  : %.8X\n", _vm->Memory->ReadLong(fsEntry + 32));
      Service->Log.AddLogDebug("fhfile: FileSystemEntry Startup   : %.8X\n", _vm->Memory->ReadLong(fsEntry + 36));
      Service->Log.AddLogDebug("fhfile: FileSystemEntry SegList   : %.8X\n", _vm->Memory->ReadLong(fsEntry + 40));
      Service->Log.AddLogDebug("fhfile: FileSystemEntry GlobalVec : %.8X\n\n", _vm->Memory->ReadLong(fsEntry + 44));
      fsNode = _vm->Memory->ReadLong(fsNode);

      EraseOlderOrSameFileSystemVersion(dosType, version);
    }
  }

  // D0 - pointer to FileSystem.resource
  void HardfileHandler::DoLogAvailableFileSystems(ULO fileSystemResource)
  {
    Service->Log.AddLogDebug("fhfile: DoLogAvailableFileSystems(fileSystemResource: %.8X)\n", fileSystemResource);

    ULO fsList = fileSystemResource + 18;
    if (fsList == _vm->Memory->ReadLong(fsList + 8))
    {
      Service->Log.AddLogDebug("fhfile: FileSystemEntry list is empty.\n");
      return;
    }

    ULO fsNode = _vm->Memory->ReadLong(fsList);
    while (fsNode != 0 && (fsNode != fsList + 4))
    {
      ULO fsEntry = fsNode + 14;

      ULO dosType = _vm->Memory->ReadLong(fsEntry);
      ULO version = _vm->Memory->ReadLong(fsEntry + 4);

      Service->Log.AddLogDebug("fhfile: FileSystemEntry DosType   : %.8X\n", dosType);
      Service->Log.AddLogDebug("fhfile: FileSystemEntry Version   : %.8X\n", version);
      Service->Log.AddLogDebug("fhfile: FileSystemEntry PatchFlags: %.8X\n", _vm->Memory->ReadLong(fsEntry + 8));
      Service->Log.AddLogDebug("fhfile: FileSystemEntry Type      : %.8X\n", _vm->Memory->ReadLong(fsEntry + 12));
      Service->Log.AddLogDebug("fhfile: FileSystemEntry Task      : %.8X\n", _vm->Memory->ReadLong(fsEntry + 16));
      Service->Log.AddLogDebug("fhfile: FileSystemEntry Lock      : %.8X\n", _vm->Memory->ReadLong(fsEntry + 20));
      Service->Log.AddLogDebug("fhfile: FileSystemEntry Handler   : %.8X\n", _vm->Memory->ReadLong(fsEntry + 24));
      Service->Log.AddLogDebug("fhfile: FileSystemEntry StackSize : %.8X\n", _vm->Memory->ReadLong(fsEntry + 28));
      Service->Log.AddLogDebug("fhfile: FileSystemEntry Priority  : %.8X\n", _vm->Memory->ReadLong(fsEntry + 32));
      Service->Log.AddLogDebug("fhfile: FileSystemEntry Startup   : %.8X\n", _vm->Memory->ReadLong(fsEntry + 36));
      Service->Log.AddLogDebug("fhfile: FileSystemEntry SegList   : %.8X\n", _vm->Memory->ReadLong(fsEntry + 40));
      Service->Log.AddLogDebug("fhfile: FileSystemEntry GlobalVec : %.8X\n\n", _vm->Memory->ReadLong(fsEntry + 44));
      fsNode = _vm->Memory->ReadLong(fsNode);
    }
  }

  void HardfileHandler::DoPatchDOSDeviceNode(ULO node, ULO packet)
  {
    Service->Log.AddLogDebug("fhfile: DoPatchDOSDeviceNode(node: %.8X, packet: %.8X)\n", node, packet);

    _vm->Memory->WriteLong(0, node + 8);                     // dn_Task = 0
    _vm->Memory->WriteLong(0, node + 16);                    // dn_Handler = 0
    _vm->Memory->WriteLong(static_cast<ULO>(-1), node + 36); // dn_GlobalVec = -1

    HardfileFileSystemEntry *fs = GetFileSystemForDOSType(_vm->Memory->ReadLong(packet + 80));
    if (fs != nullptr)
    {
      if (fs->Header->PatchFlags & 0x10)
      {
        _vm->Memory->WriteLong(fs->Header->DnStackSize, node + 20);
      }
      if (fs->Header->PatchFlags & 0x80)
      {
        _vm->Memory->WriteLong(fs->SegListAddress >> 2, node + 32);
      }
      if (fs->Header->PatchFlags & 0x100)
      {
        _vm->Memory->WriteLong(fs->Header->DnGlobalVec, node + 36);
      }
    }
  }

  void HardfileHandler::DoUnknownOperation(ULO operation)
  {
    Service->Log.AddLogDebug("fhfile: Unknown operation called %X\n", operation);
  }

  /*=================================================*/
  /* fhfile_do                                       */
  /* The M68000 stubs entered in the device tables   */
  /* write a longword to $f40000, which is forwarded */
  /* by the memory system to this procedure.         */
  /* Hardfile commands are issued by 0x0001XXXX      */
  /* RDB filesystem commands by 0x0002XXXX           */
  /*=================================================*/

  void HardfileHandler::Do(ULO data)
  {
    ULO type = data >> 16;
    ULO operation = data & 0xffff;
    if (type == 1)
    {
      switch (operation)
      {
        case 1: DoDiag(); break;
        case 2: DoOpen(); break;
        case 3: DoClose(); break;
        case 4: DoExpunge(); break;
        case 5: DoNULL(); break;
        case 6: DoBeginIO(); break;
        case 7: DoAbortIO(); break;
        default: break;
      }
    }
    else if (type == 2)
    {
      switch (operation)
      {
        case 1: _vm->CPU->SetDReg(0, DoGetRDBFileSystemCount()); break;
        case 2: _vm->CPU->SetDReg(0, DoGetRDBFileSystemHunkCount(_vm->CPU->GetDReg(1))); break;
        case 3: _vm->CPU->SetDReg(0, DoGetRDBFileSystemHunkSize(_vm->CPU->GetDReg(1), _vm->CPU->GetDReg(2))); break;
        case 4:
          DoCopyRDBFileSystemHunk(_vm->CPU->GetDReg(0),  // VM memory allocated for hunk
                                  _vm->CPU->GetDReg(1),  // Filesystem index on the device
                                  _vm->CPU->GetDReg(2)); // Hunk index in filesystem fileimage
          break;
        case 5: DoInitializeRDBFileSystemEntry(_vm->CPU->GetDReg(0), _vm->CPU->GetDReg(1)); break;
        case 6: DoRemoveRDBFileSystemsAlreadySupportedBySystem(_vm->CPU->GetDReg(0)); break;
        case 7: DoPatchDOSDeviceNode(_vm->CPU->GetDReg(0), _vm->CPU->GetAReg(5)); break;
        case 8: DoRelocateFileSystem(_vm->CPU->GetDReg(1)); break;
        case 9: _vm->CPU->SetDReg(0, DoGetDosDevPacketListStart()); break;
        case 0xa0: DoLogAllocMemResult(_vm->CPU->GetDReg(0)); break;
        case 0xa1: DoLogOpenResourceResult(_vm->CPU->GetDReg(0)); break;
        case 0xa2: DoLogAvailableResources(); break;
        case 0xa3: DoLogAvailableFileSystems(_vm->CPU->GetDReg(0)); break;
        default: DoUnknownOperation(operation);
      }
    }
  }

  /*=================================================*/
  /* Make a dosdevice packet about the device layout */
  /*=================================================*/

  void HardfileHandler::MakeDOSDevPacketForPlainHardfile(const HardfileMountListEntry &mountListEntry, ULO deviceNameAddress)
  {
    const HardfileDevice &device = _devices[mountListEntry.DeviceIndex];
    if (device.F != nullptr)
    {
      ULO unit = GetUnitNumberFromIndex(mountListEntry.DeviceIndex);
      const HardfileGeometry &geometry = device.Configuration.Geometry;
      _vm->Memory->DmemSetLong(mountListEntry.DeviceIndex); /* Flag to initcode */

      _vm->Memory->DmemSetLong(mountListEntry.NameAddress); /*  0 Unit name "FELLOWX" or similar */
      _vm->Memory->DmemSetLong(deviceNameAddress);          /*  4 Device name "fhfile.device" */
      _vm->Memory->DmemSetLong(unit);                       /*  8 Unit # */
      _vm->Memory->DmemSetLong(0);                          /* 12 OpenDevice flags */

      // Struct DosEnvec
      _vm->Memory->DmemSetLong(16);                           /* 16 Environment size in long words */
      _vm->Memory->DmemSetLong(geometry.BytesPerSector >> 2); /* 20 Longwords in a block */
      _vm->Memory->DmemSetLong(0);                            /* 24 sector origin (unused) */
      _vm->Memory->DmemSetLong(geometry.Surfaces);            /* 28 Heads */
      _vm->Memory->DmemSetLong(1);                            /* 32 Sectors per logical block (unused) */
      _vm->Memory->DmemSetLong(geometry.SectorsPerTrack);     /* 36 Sectors per track */
      _vm->Memory->DmemSetLong(geometry.ReservedBlocks);      /* 40 Reserved blocks, min. 1 */
      _vm->Memory->DmemSetLong(0);                            /* 44 mdn_prefac - Unused */
      _vm->Memory->DmemSetLong(0);                            /* 48 Interleave */
      _vm->Memory->DmemSetLong(0);                            /* 52 Lower cylinder */
      _vm->Memory->DmemSetLong(geometry.HighCylinder);        /* 56 Upper cylinder */
      _vm->Memory->DmemSetLong(0);                            /* 60 Number of buffers */
      _vm->Memory->DmemSetLong(0);                            /* 64 Type of memory for buffers */
      _vm->Memory->DmemSetLong(0x7fffffff);                   /* 68 Largest transfer */
      _vm->Memory->DmemSetLong(~1U);                          /* 72 Add mask */
      _vm->Memory->DmemSetLong(static_cast<ULO>(-1));         /* 76 Boot priority */
      _vm->Memory->DmemSetLong(0x444f5300);                   /* 80 DOS file handler name */
      _vm->Memory->DmemSetLong(0);
    }
  }

  void HardfileHandler::MakeDOSDevPacketForRDBPartition(const HardfileMountListEntry &mountListEntry, ULO deviceNameAddress)
  {
    HardfileDevice &device = _devices[mountListEntry.DeviceIndex];
    RDBPartition *partition = device.RDB->Partitions[mountListEntry.PartitionIndex].get();
    if (device.F != nullptr)
    {
      ULO unit = GetUnitNumberFromIndex(mountListEntry.DeviceIndex);

      _vm->Memory->DmemSetLong(mountListEntry.DeviceIndex); /* Flag to initcode */

      _vm->Memory->DmemSetLong(mountListEntry.NameAddress); /*  0 Unit name "FELLOWX" or similar */
      _vm->Memory->DmemSetLong(deviceNameAddress);          /*  4 Device name "fhfile.device" */
      _vm->Memory->DmemSetLong(unit);                       /*  8 Unit # */
      _vm->Memory->DmemSetLong(0);                          /* 12 OpenDevice flags */

      // Struct DosEnvec
      _vm->Memory->DmemSetLong(16);                         /* 16 Environment size in long words*/
      _vm->Memory->DmemSetLong(partition->SizeBlock);       /* 20 Longwords in a block */
      _vm->Memory->DmemSetLong(partition->SecOrg);          /* 24 sector origin (unused) */
      _vm->Memory->DmemSetLong(partition->Surfaces);        /* 28 Heads */
      _vm->Memory->DmemSetLong(partition->SectorsPerBlock); /* 32 Sectors per logical block (unused) */
      _vm->Memory->DmemSetLong(partition->BlocksPerTrack);  /* 36 Sectors per track */
      _vm->Memory->DmemSetLong(partition->Reserved);        /* 40 Reserved blocks, min. 1 */
      _vm->Memory->DmemSetLong(partition->PreAlloc);        /* 44 mdn_prefac - Unused */
      _vm->Memory->DmemSetLong(partition->Interleave);      /* 48 Interleave */
      _vm->Memory->DmemSetLong(partition->LowCylinder);     /* 52 Lower cylinder */
      _vm->Memory->DmemSetLong(partition->HighCylinder);    /* 56 Upper cylinder */
      _vm->Memory->DmemSetLong(partition->NumBuffer);       /* 60 Number of buffers */
      _vm->Memory->DmemSetLong(partition->BufMemType);      /* 64 Type of memory for buffers */
      _vm->Memory->DmemSetLong(partition->MaxTransfer);     /* 68 Largest transfer */
      _vm->Memory->DmemSetLong(partition->Mask);            /* 72 Add mask */
      _vm->Memory->DmemSetLong(partition->BootPri);         /* 76 Boot priority */
      _vm->Memory->DmemSetLong(partition->DOSType);         /* 80 DOS file handler name */
      _vm->Memory->DmemSetLong(0);
    }
  }

  void HardfileHandler::RebuildHardfileConfiguration()
  {
    _deviceNameStartNumber = 0;
    _fileSystems.clear(); // This is rebuild later in this functions

    // Re-initialize device in case RDB config and partition table has been rewritten inside the emulator
    for (int i = 0; i < FHFILE_MAX_DEVICES; i++)
    {
      InitializeHardfile(i);
    }

    CreateMountList(); // (Re-)Builds mountlist with all partitions from each device
    AddFileSystemsFromRdb();
  }

  /*===========================================================*/
  /* fhfileHardReset                                           */
  /* This will set up the device structures and stubs          */
  /* Can be called at every reset, but really only needed once */
  /*===========================================================*/

  void HardfileHandler::HardReset()
  {
    if (!HasZeroDevices() && GetEnabled() && _vm->Memory->GetKickImageVersion() >= 34)
    {
      _vm->Memory->DmemSetCounter(0);

      /* Device-name and ID string */

      _devicename = _vm->Memory->DmemGetCounter();
      _vm->Memory->DmemSetString("fhfile.device");
      ULO idstr = _vm->Memory->DmemGetCounter();
      _vm->Memory->DmemSetString("Fellow Hardfile device V5");
      ULO doslibname = _vm->Memory->DmemGetCounter();
      _vm->Memory->DmemSetString("dos.library");
      _fsname = _vm->Memory->DmemGetCounter();
      _vm->Memory->DmemSetString("Fellow hardfile RDB fs");

      /* fhfile.open */

      ULO fhfile_t_open = _vm->Memory->DmemGetCounter();
      _vm->Memory->DmemSetWord(0x23fc);
      _vm->Memory->DmemSetLong(0x00010002);
      _vm->Memory->DmemSetLong(0xf40000); /* move.l #$00010002,$f40000 */
      _vm->Memory->DmemSetWord(0x4e75);   /* rts */

      /* fhfile.close */

      ULO fhfile_t_close = _vm->Memory->DmemGetCounter();
      _vm->Memory->DmemSetWord(0x23fc);
      _vm->Memory->DmemSetLong(0x00010003);
      _vm->Memory->DmemSetLong(0xf40000); /* move.l #$00010003,$f40000 */
      _vm->Memory->DmemSetWord(0x4e75);   /* rts */

      /* fhfile.expunge */

      ULO fhfile_t_expunge = _vm->Memory->DmemGetCounter();
      _vm->Memory->DmemSetWord(0x23fc);
      _vm->Memory->DmemSetLong(0x00010004);
      _vm->Memory->DmemSetLong(0xf40000); /* move.l #$00010004,$f40000 */
      _vm->Memory->DmemSetWord(0x4e75);   /* rts */

      /* fhfile.null */

      ULO fhfile_t_null = _vm->Memory->DmemGetCounter();
      _vm->Memory->DmemSetWord(0x23fc);
      _vm->Memory->DmemSetLong(0x00010005);
      _vm->Memory->DmemSetLong(0xf40000); /* move.l #$00010005,$f40000 */
      _vm->Memory->DmemSetWord(0x4e75);   /* rts */

      /* fhfile.beginio */

      ULO fhfile_t_beginio = _vm->Memory->DmemGetCounter();
      _vm->Memory->DmemSetWord(0x23fc);
      _vm->Memory->DmemSetLong(0x00010006);
      _vm->Memory->DmemSetLong(0xf40000);   /* move.l #$00010006,$f40000  BeginIO */
      _vm->Memory->DmemSetLong(0x48e78002); /* movem.l d0/a6,-(a7)    push        */
      _vm->Memory->DmemSetLong(0x08290000);
      _vm->Memory->DmemSetWord(0x001e);     /* btst   #$0,30(a1)      IOF_QUICK   */
      _vm->Memory->DmemSetWord(0x6608);     /* bne    (to pop)                    */
      _vm->Memory->DmemSetLong(0x2c780004); /* move.l $4.w,a6         exec        */
      _vm->Memory->DmemSetLong(0x4eaefe86); /* jsr    -378(a6)        ReplyMsg(a1)*/
      _vm->Memory->DmemSetLong(0x4cdf4001); /* movem.l (a7)+,d0/a6    pop         */
      _vm->Memory->DmemSetWord(0x4e75);     /* rts */

      /* fhfile.abortio */

      ULO fhfile_t_abortio = _vm->Memory->DmemGetCounter();
      _vm->Memory->DmemSetWord(0x23fc);
      _vm->Memory->DmemSetLong(0x00010007);
      _vm->Memory->DmemSetLong(0xf40000); /* move.l #$00010007,$f40000 */
      _vm->Memory->DmemSetWord(0x4e75);   /* rts */

      /* Func-table */

      ULO functable = _vm->Memory->DmemGetCounter();
      _vm->Memory->DmemSetLong(fhfile_t_open);
      _vm->Memory->DmemSetLong(fhfile_t_close);
      _vm->Memory->DmemSetLong(fhfile_t_expunge);
      _vm->Memory->DmemSetLong(fhfile_t_null);
      _vm->Memory->DmemSetLong(fhfile_t_beginio);
      _vm->Memory->DmemSetLong(fhfile_t_abortio);
      _vm->Memory->DmemSetLong(0xffffffff);

      /* Data-table */

      ULO datatable = _vm->Memory->DmemGetCounter();
      _vm->Memory->DmemSetWord(0xE000); /* INITBYTE */
      _vm->Memory->DmemSetWord(0x0008); /* LN_TYPE */
      _vm->Memory->DmemSetWord(0x0300); /* NT_DEVICE */
      _vm->Memory->DmemSetWord(0xC000); /* INITLONG */
      _vm->Memory->DmemSetWord(0x000A); /* LN_NAME */
      _vm->Memory->DmemSetLong(_devicename);
      _vm->Memory->DmemSetWord(0xE000); /* INITBYTE */
      _vm->Memory->DmemSetWord(0x000E); /* LIB_FLAGS */
      _vm->Memory->DmemSetWord(0x0600); /* LIBF_SUMUSED+LIBF_CHANGED */
      _vm->Memory->DmemSetWord(0xD000); /* INITWORD */
      _vm->Memory->DmemSetWord(0x0014); /* LIB_VERSION */
      _vm->Memory->DmemSetWord(0x0002);
      _vm->Memory->DmemSetWord(0xD000); /* INITWORD */
      _vm->Memory->DmemSetWord(0x0016); /* LIB_REVISION */
      _vm->Memory->DmemSetWord(0x0000);
      _vm->Memory->DmemSetWord(0xC000); /* INITLONG */
      _vm->Memory->DmemSetWord(0x0018); /* LIB_IDSTRING */
      _vm->Memory->DmemSetLong(idstr);
      _vm->Memory->DmemSetLong(0); /* END */

      /* bootcode */

      _bootcode = _vm->Memory->DmemGetCounter();
      _vm->Memory->DmemSetWord(0x227c);
      _vm->Memory->DmemSetLong(doslibname); /* move.l #doslibname,a1 */
      _vm->Memory->DmemSetLong(0x4eaeffa0); /* jsr    -96(a6) ; FindResident() */
      _vm->Memory->DmemSetWord(0x2040);     /* move.l d0,a0 */
      _vm->Memory->DmemSetLong(0x20280016); /* move.l 22(a0),d0 */
      _vm->Memory->DmemSetWord(0x2040);     /* move.l d0,a0 */
      _vm->Memory->DmemSetWord(0x4e90);     /* jsr    (a0) */
      _vm->Memory->DmemSetWord(0x4e75);     /* rts */

      /* fhfile.init */

      ULO fhfile_t_init = _vm->Memory->DmemGetCounter();

      _vm->Memory->DmemSetByte(0x48);
      _vm->Memory->DmemSetByte(0xE7);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0xFE);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xF6);
      _vm->Memory->DmemSetByte(0x4A);
      _vm->Memory->DmemSetByte(0x80);
      _vm->Memory->DmemSetByte(0x67);
      _vm->Memory->DmemSetByte(0x24);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xD6);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x01);
      _vm->Memory->DmemSetByte(0xB0);
      _vm->Memory->DmemSetByte(0x4A);
      _vm->Memory->DmemSetByte(0x80);
      _vm->Memory->DmemSetByte(0x66);
      _vm->Memory->DmemSetByte(0x0C);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x01);
      _vm->Memory->DmemSetByte(0x6A);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x01);
      _vm->Memory->DmemSetByte(0xA4);
      _vm->Memory->DmemSetByte(0x4A);
      _vm->Memory->DmemSetByte(0x80);
      _vm->Memory->DmemSetByte(0x67);
      _vm->Memory->DmemSetByte(0x0C);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x01);
      _vm->Memory->DmemSetByte(0x12);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x02);
      _vm->Memory->DmemSetByte(0x0A);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xC2);
      _vm->Memory->DmemSetByte(0x2C);
      _vm->Memory->DmemSetByte(0x78);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x04);
      _vm->Memory->DmemSetByte(0x43);
      _vm->Memory->DmemSetByte(0xFA);
      _vm->Memory->DmemSetByte(0x02);
      _vm->Memory->DmemSetByte(0x20);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0xAE);
      _vm->Memory->DmemSetByte(0xFE);
      _vm->Memory->DmemSetByte(0x68);
      _vm->Memory->DmemSetByte(0x28);
      _vm->Memory->DmemSetByte(0x40);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x01);
      _vm->Memory->DmemSetByte(0x1C);
      _vm->Memory->DmemSetByte(0x20);
      _vm->Memory->DmemSetByte(0x40);
      _vm->Memory->DmemSetByte(0x2E);
      _vm->Memory->DmemSetByte(0x08);
      _vm->Memory->DmemSetByte(0x20);
      _vm->Memory->DmemSetByte(0x47);
      _vm->Memory->DmemSetByte(0x4A);
      _vm->Memory->DmemSetByte(0x90);
      _vm->Memory->DmemSetByte(0x6B);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x70);
      _vm->Memory->DmemSetByte(0x58);
      _vm->Memory->DmemSetByte(0x87);
      _vm->Memory->DmemSetByte(0x20);
      _vm->Memory->DmemSetByte(0x3C);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x58);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x01);
      _vm->Memory->DmemSetByte(0x10);
      _vm->Memory->DmemSetByte(0x2A);
      _vm->Memory->DmemSetByte(0x40);
      _vm->Memory->DmemSetByte(0x20);
      _vm->Memory->DmemSetByte(0x47);
      _vm->Memory->DmemSetByte(0x70);
      _vm->Memory->DmemSetByte(0x54);
      _vm->Memory->DmemSetByte(0x2B);
      _vm->Memory->DmemSetByte(0xB0);
      _vm->Memory->DmemSetByte(0x08);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x08);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x59);
      _vm->Memory->DmemSetByte(0x80);
      _vm->Memory->DmemSetByte(0x64);
      _vm->Memory->DmemSetByte(0xF6);
      _vm->Memory->DmemSetByte(0xCD);
      _vm->Memory->DmemSetByte(0x4C);
      _vm->Memory->DmemSetByte(0x20);
      _vm->Memory->DmemSetByte(0x4D);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0xAE);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0x70);
      _vm->Memory->DmemSetByte(0xCD);
      _vm->Memory->DmemSetByte(0x4C);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xCE);
      _vm->Memory->DmemSetByte(0x26);
      _vm->Memory->DmemSetByte(0x40);
      _vm->Memory->DmemSetByte(0x70);
      _vm->Memory->DmemSetByte(0x14);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xEA);
      _vm->Memory->DmemSetByte(0x22);
      _vm->Memory->DmemSetByte(0x47);
      _vm->Memory->DmemSetByte(0x2C);
      _vm->Memory->DmemSetByte(0x29);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0xFC);
      _vm->Memory->DmemSetByte(0x22);
      _vm->Memory->DmemSetByte(0x40);
      _vm->Memory->DmemSetByte(0x70);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x22);
      _vm->Memory->DmemSetByte(0x80);
      _vm->Memory->DmemSetByte(0x23);
      _vm->Memory->DmemSetByte(0x40);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x04);
      _vm->Memory->DmemSetByte(0x33);
      _vm->Memory->DmemSetByte(0x40);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x0E);
      _vm->Memory->DmemSetByte(0x33);
      _vm->Memory->DmemSetByte(0x7C);
      _vm->Memory->DmemSetByte(0x10);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x08);
      _vm->Memory->DmemSetByte(0x9D);
      _vm->Memory->DmemSetByte(0x69);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x08);
      _vm->Memory->DmemSetByte(0x23);
      _vm->Memory->DmemSetByte(0x79);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xF4);
      _vm->Memory->DmemSetByte(0x0F);
      _vm->Memory->DmemSetByte(0xFC);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x0A);
      _vm->Memory->DmemSetByte(0x23);
      _vm->Memory->DmemSetByte(0x4B);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x10);
      _vm->Memory->DmemSetByte(0x41);
      _vm->Memory->DmemSetByte(0xEC);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x4A);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0xAE);
      _vm->Memory->DmemSetByte(0xFE);
      _vm->Memory->DmemSetByte(0xF2);
      _vm->Memory->DmemSetByte(0x06);
      _vm->Memory->DmemSetByte(0x87);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x58);
      _vm->Memory->DmemSetByte(0x60);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0x8C);
      _vm->Memory->DmemSetByte(0x2C);
      _vm->Memory->DmemSetByte(0x78);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x04);
      _vm->Memory->DmemSetByte(0x22);
      _vm->Memory->DmemSetByte(0x4C);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0xAE);
      _vm->Memory->DmemSetByte(0xFE);
      _vm->Memory->DmemSetByte(0x62);
      _vm->Memory->DmemSetByte(0x4C);
      _vm->Memory->DmemSetByte(0xDF);
      _vm->Memory->DmemSetByte(0x7F);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x23);
      _vm->Memory->DmemSetByte(0xFC);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x02);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xA0);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xF4);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x23);
      _vm->Memory->DmemSetByte(0xFC);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x02);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xA1);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xF4);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x23);
      _vm->Memory->DmemSetByte(0xFC);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x02);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xA2);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xF4);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x23);
      _vm->Memory->DmemSetByte(0xFC);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x02);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xA3);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xF4);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x23);
      _vm->Memory->DmemSetByte(0xFC);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x02);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x01);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xF4);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x23);
      _vm->Memory->DmemSetByte(0xFC);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x02);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x02);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xF4);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x23);
      _vm->Memory->DmemSetByte(0xFC);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x02);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x03);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xF4);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x23);
      _vm->Memory->DmemSetByte(0xFC);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x02);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x04);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xF4);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x23);
      _vm->Memory->DmemSetByte(0xFC);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x02);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x05);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xF4);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x23);
      _vm->Memory->DmemSetByte(0xFC);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x02);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x06);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xF4);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x23);
      _vm->Memory->DmemSetByte(0xFC);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x02);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x07);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xF4);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x23);
      _vm->Memory->DmemSetByte(0xFC);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x02);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x08);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xF4);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x23);
      _vm->Memory->DmemSetByte(0xFC);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x02);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x09);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xF4);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x48);
      _vm->Memory->DmemSetByte(0xE7);
      _vm->Memory->DmemSetByte(0x78);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x22);
      _vm->Memory->DmemSetByte(0x3C);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x01);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x01);
      _vm->Memory->DmemSetByte(0x2C);
      _vm->Memory->DmemSetByte(0x78);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x04);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0xAE);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0x3A);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0x50);
      _vm->Memory->DmemSetByte(0x4C);
      _vm->Memory->DmemSetByte(0xDF);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x1E);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x20);
      _vm->Memory->DmemSetByte(0x3C);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x20);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0xDC);
      _vm->Memory->DmemSetByte(0x2A);
      _vm->Memory->DmemSetByte(0x40);
      _vm->Memory->DmemSetByte(0x1B);
      _vm->Memory->DmemSetByte(0x7C);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x08);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x08);
      _vm->Memory->DmemSetByte(0x41);
      _vm->Memory->DmemSetByte(0xFA);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xD0);
      _vm->Memory->DmemSetByte(0x2B);
      _vm->Memory->DmemSetByte(0x48);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x0A);
      _vm->Memory->DmemSetByte(0x41);
      _vm->Memory->DmemSetByte(0xFA);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xDC);
      _vm->Memory->DmemSetByte(0x2B);
      _vm->Memory->DmemSetByte(0x48);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x0E);
      _vm->Memory->DmemSetByte(0x49);
      _vm->Memory->DmemSetByte(0xED);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x12);
      _vm->Memory->DmemSetByte(0x28);
      _vm->Memory->DmemSetByte(0x8C);
      _vm->Memory->DmemSetByte(0x06);
      _vm->Memory->DmemSetByte(0x94);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x04);
      _vm->Memory->DmemSetByte(0x42);
      _vm->Memory->DmemSetByte(0xAC);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x04);
      _vm->Memory->DmemSetByte(0x29);
      _vm->Memory->DmemSetByte(0x4C);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x08);
      _vm->Memory->DmemSetByte(0x22);
      _vm->Memory->DmemSetByte(0x4D);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0xAE);
      _vm->Memory->DmemSetByte(0xFE);
      _vm->Memory->DmemSetByte(0x1A);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x2C);
      _vm->Memory->DmemSetByte(0x78);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x04);
      _vm->Memory->DmemSetByte(0x70);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x43);
      _vm->Memory->DmemSetByte(0xFA);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x9E);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0xAE);
      _vm->Memory->DmemSetByte(0xFE);
      _vm->Memory->DmemSetByte(0x0E);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0x06);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0x30);
      _vm->Memory->DmemSetByte(0x4A);
      _vm->Memory->DmemSetByte(0x80);
      _vm->Memory->DmemSetByte(0x67);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x26);
      _vm->Memory->DmemSetByte(0x28);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x74);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0x2E);
      _vm->Memory->DmemSetByte(0x4A);
      _vm->Memory->DmemSetByte(0x80);
      _vm->Memory->DmemSetByte(0x67);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x0C);
      _vm->Memory->DmemSetByte(0x50);
      _vm->Memory->DmemSetByte(0x80);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0x76);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0x2A);
      _vm->Memory->DmemSetByte(0x52);
      _vm->Memory->DmemSetByte(0x82);
      _vm->Memory->DmemSetByte(0xB8);
      _vm->Memory->DmemSetByte(0x82);
      _vm->Memory->DmemSetByte(0x66);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0xE6);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x2F);
      _vm->Memory->DmemSetByte(0x08);
      _vm->Memory->DmemSetByte(0x2F);
      _vm->Memory->DmemSetByte(0x01);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0xCA);
      _vm->Memory->DmemSetByte(0x20);
      _vm->Memory->DmemSetByte(0x3C);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xBE);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0x52);
      _vm->Memory->DmemSetByte(0x22);
      _vm->Memory->DmemSetByte(0x1F);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0x10);
      _vm->Memory->DmemSetByte(0x2C);
      _vm->Memory->DmemSetByte(0x79);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x04);
      _vm->Memory->DmemSetByte(0x20);
      _vm->Memory->DmemSetByte(0x57);
      _vm->Memory->DmemSetByte(0x41);
      _vm->Memory->DmemSetByte(0xE8);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x12);
      _vm->Memory->DmemSetByte(0x22);
      _vm->Memory->DmemSetByte(0x40);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0xAE);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0x10);
      _vm->Memory->DmemSetByte(0x20);
      _vm->Memory->DmemSetByte(0x5F);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x2F);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x20);
      _vm->Memory->DmemSetByte(0x40);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xFE);
      _vm->Memory->DmemSetByte(0xC2);
      _vm->Memory->DmemSetByte(0x4A);
      _vm->Memory->DmemSetByte(0x80);
      _vm->Memory->DmemSetByte(0x67);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x12);
      _vm->Memory->DmemSetByte(0x26);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x72);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0xBE);
      _vm->Memory->DmemSetByte(0x52);
      _vm->Memory->DmemSetByte(0x81);
      _vm->Memory->DmemSetByte(0xB6);
      _vm->Memory->DmemSetByte(0x81);
      _vm->Memory->DmemSetByte(0x66);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0xFF);
      _vm->Memory->DmemSetByte(0xF6);
      _vm->Memory->DmemSetByte(0x20);
      _vm->Memory->DmemSetByte(0x1F);
      _vm->Memory->DmemSetByte(0x4E);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x65);
      _vm->Memory->DmemSetByte(0x78);
      _vm->Memory->DmemSetByte(0x70);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x6E);
      _vm->Memory->DmemSetByte(0x73);
      _vm->Memory->DmemSetByte(0x69);
      _vm->Memory->DmemSetByte(0x6F);
      _vm->Memory->DmemSetByte(0x6E);
      _vm->Memory->DmemSetByte(0x2E);
      _vm->Memory->DmemSetByte(0x6C);
      _vm->Memory->DmemSetByte(0x69);
      _vm->Memory->DmemSetByte(0x62);
      _vm->Memory->DmemSetByte(0x72);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x72);
      _vm->Memory->DmemSetByte(0x79);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x46);
      _vm->Memory->DmemSetByte(0x69);
      _vm->Memory->DmemSetByte(0x6C);
      _vm->Memory->DmemSetByte(0x65);
      _vm->Memory->DmemSetByte(0x53);
      _vm->Memory->DmemSetByte(0x79);
      _vm->Memory->DmemSetByte(0x73);
      _vm->Memory->DmemSetByte(0x74);
      _vm->Memory->DmemSetByte(0x65);
      _vm->Memory->DmemSetByte(0x6D);
      _vm->Memory->DmemSetByte(0x2E);
      _vm->Memory->DmemSetByte(0x72);
      _vm->Memory->DmemSetByte(0x65);
      _vm->Memory->DmemSetByte(0x73);
      _vm->Memory->DmemSetByte(0x6F);
      _vm->Memory->DmemSetByte(0x75);
      _vm->Memory->DmemSetByte(0x72);
      _vm->Memory->DmemSetByte(0x63);
      _vm->Memory->DmemSetByte(0x65);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x46);
      _vm->Memory->DmemSetByte(0x65);
      _vm->Memory->DmemSetByte(0x6C);
      _vm->Memory->DmemSetByte(0x6C);
      _vm->Memory->DmemSetByte(0x6F);
      _vm->Memory->DmemSetByte(0x77);
      _vm->Memory->DmemSetByte(0x20);
      _vm->Memory->DmemSetByte(0x68);
      _vm->Memory->DmemSetByte(0x61);
      _vm->Memory->DmemSetByte(0x72);
      _vm->Memory->DmemSetByte(0x64);
      _vm->Memory->DmemSetByte(0x66);
      _vm->Memory->DmemSetByte(0x69);
      _vm->Memory->DmemSetByte(0x6C);
      _vm->Memory->DmemSetByte(0x65);
      _vm->Memory->DmemSetByte(0x20);
      _vm->Memory->DmemSetByte(0x64);
      _vm->Memory->DmemSetByte(0x65);
      _vm->Memory->DmemSetByte(0x76);
      _vm->Memory->DmemSetByte(0x69);
      _vm->Memory->DmemSetByte(0x63);
      _vm->Memory->DmemSetByte(0x65);
      _vm->Memory->DmemSetByte(0x00);
      _vm->Memory->DmemSetByte(0x00);

      /* Init-struct */

      ULO initstruct = _vm->Memory->DmemGetCounter();
      _vm->Memory->DmemSetLong(0x100);         /* Data-space size, min LIB_SIZE */
      _vm->Memory->DmemSetLong(functable);     /* Function-table */
      _vm->Memory->DmemSetLong(datatable);     /* Data-table */
      _vm->Memory->DmemSetLong(fhfile_t_init); /* Init-routine */

      /* RomTag structure */

      ULO romtagstart = _vm->Memory->DmemGetCounter();
      _vm->Memory->DmemSetWord(0x4afc);           /* Start of structure */
      _vm->Memory->DmemSetLong(romtagstart);      /* Pointer to start of structure */
      _vm->Memory->DmemSetLong(romtagstart + 26); /* Pointer to end of code */
      _vm->Memory->DmemSetByte(0x81);             /* Flags, AUTOINIT+COLDSTART */
      _vm->Memory->DmemSetByte(0x1);              /* Version */
      _vm->Memory->DmemSetByte(3);                /* DEVICE */
      _vm->Memory->DmemSetByte(0);                /* Priority */
      _vm->Memory->DmemSetLong(_devicename);      /* Pointer to name (used in opendev)*/
      _vm->Memory->DmemSetLong(idstr);            /* ID string */
      _vm->Memory->DmemSetLong(initstruct);       /* Init_struct */

      _endOfDmem = _vm->Memory->DmemGetCounterWithoutOffset();

      /* Clear hardfile rom */

      memset(_rom, 0, 65536);

      /* Struct DiagArea */

      _rom[0x1000] = 0x90; /* da_Config */
      _rom[0x1001] = 0;    /* da_Flags */
      _rom[0x1002] = 0;    /* da_Size */
      _rom[0x1003] = 0x96;
      _rom[0x1004] = 0; /* da_DiagPoint */
      _rom[0x1005] = 0x80;
      _rom[0x1006] = 0; /* da_BootPoint */
      _rom[0x1007] = 0x90;
      _rom[0x1008] = 0; /* da_Name */
      _rom[0x1009] = 0;
      _rom[0x100a] = 0; /* da_Reserved01 */
      _rom[0x100b] = 0;
      _rom[0x100c] = 0; /* da_Reserved02 */
      _rom[0x100d] = 0;

      _rom[0x1080] = 0x23; /* DiagPoint */
      _rom[0x1081] = 0xfc; /* move.l #$00010001,$f40000 */
      _rom[0x1082] = 0x00;
      _rom[0x1083] = 0x01;
      _rom[0x1084] = 0x00;
      _rom[0x1085] = 0x01;
      _rom[0x1086] = 0x00;
      _rom[0x1087] = 0xf4;
      _rom[0x1088] = 0x00;
      _rom[0x1089] = 0x00;
      _rom[0x108a] = 0x4e; /* rts */
      _rom[0x108b] = 0x75;

      _rom[0x1090] = 0x4e; /* BootPoint */
      _rom[0x1091] = 0xf9; /* JMP fhfile_bootcode */
      _rom[0x1092] = static_cast<UBY>(_bootcode >> 24);
      _rom[0x1093] = static_cast<UBY>(_bootcode >> 16);
      _rom[0x1094] = static_cast<UBY>(_bootcode >> 8);
      _rom[0x1095] = static_cast<UBY>(_bootcode);

      /* NULLIFY pointer to configdev */

      _vm->Memory->DmemSetLongNoCounter(0, 4092);
      _vm->Memory->EmemCardAdd(HardfileHandler_CardInit, HardfileHandler_CardMap);
    }
    else
    {
      _vm->Memory->DmemClear();
    }
  }

  void HardfileHandler::CreateDOSDevPackets(ULO devicename)
  {
    _vm->Memory->DmemSetCounter(_endOfDmem);

    /* Device name as seen in Amiga DOS */

    for (auto &mountListEntry : _mountList)
    {
      mountListEntry->NameAddress = _vm->Memory->DmemGetCounter();
      _vm->Memory->DmemSetString(mountListEntry->Name.c_str());
    }

    _dosDevPacketListStart = _vm->Memory->DmemGetCounter();

    /* The mkdosdev packets */

    for (auto &mountListEntry : _mountList)
    {
      if (mountListEntry->PartitionIndex == -1)
      {
        MakeDOSDevPacketForPlainHardfile(*mountListEntry, devicename);
      }
      else
      {
        MakeDOSDevPacketForRDBPartition(*mountListEntry, devicename);
      }
    }
    _vm->Memory->DmemSetLong(static_cast<ULO>(-1)); // Terminate list
  }

  void HardfileHandler::EmulationStart()
  {
    for (int i = 0; i < FHFILE_MAX_DEVICES; i++)
    {
      if (_devices[i].Status == fhfile_status::FHFILE_HDF && _devices[i].F == nullptr)
      {
        OpenHardfileFile(_devices[i]);
      }
    }
  }

  void HardfileHandler::EmulationStop()
  {
    for (int i = 0; i < FHFILE_MAX_DEVICES; i++)
    {
      if (_devices[i].Status == fhfile_status::FHFILE_HDF && _devices[i].F != nullptr)
      {
        fclose(_devices[i].F);
        _devices[i].F = nullptr;
      }
    }
  }

  void HardfileHandler::Startup()
  {
    Clear();
  }

  void HardfileHandler::Shutdown()
  {
    Clear();
  }

  /*==========================*/
  /* Create hardfile          */
  /*==========================*/

  bool HardfileHandler::Create(const fellow::api::modules::HardfileConfiguration &configuration, ULO size)
  {
    bool result = false;

    constexpr auto BUFSIZE = 32768;
    unsigned int tobewritten;
    char buffer[BUFSIZE];
    FILE *hf;

    tobewritten = size;
    errno = 0;

    if (!configuration.Filename.empty() && size != 0)
    {
      hf = fopen(configuration.Filename.c_str(), "wb");
      if (hf != nullptr)
      {
        memset(buffer, 0, sizeof(buffer));
        while (tobewritten >= BUFSIZE)
        {
          fwrite(buffer, sizeof(char), BUFSIZE, hf);
          if (errno != 0)
          {
            Service->Log.AddLog("Creating hardfile failed. Check the available space.\n");
            fclose(hf);
            return result;
          }
          tobewritten -= BUFSIZE;
        }
        fwrite(buffer, sizeof(char), tobewritten, hf);
        if (errno != 0)
        {
          Service->Log.AddLog("Creating hardfile failed. Check the available space.\n");
          fclose(hf);
          return result;
        }
        fclose(hf);
        result = true;
      }
      else
        Service->Log.AddLog("fhfileCreate is unable to open output file.\n");
    }
    return result;
  }

  rdb_status HardfileHandler::HasRDB(const std::string &filename)
  {
    rdb_status result = rdb_status::RDB_NOT_FOUND;
    FILE *F = fopen(filename.c_str(), "rb");
    if (F != nullptr)
    {
      RDBFileReader reader(F);
      result = RDBHandler::HasRigidDiskBlock(reader);
      fclose(F);
    }
    return result;
  }

  HardfileConfiguration HardfileHandler::GetConfigurationFromRDBGeometry(const std::string &filename)
  {
    HardfileConfiguration configuration;
    FILE *F = fopen(filename.c_str(), "rb");
    if (F != nullptr)
    {
      RDBFileReader reader(F);
      RDB *rdb = RDBHandler::GetDriveInformation(reader, true);
      fclose(F);

      if (rdb != nullptr)
      {
        SetHardfileConfigurationFromRDB(configuration, rdb, false);
        delete rdb;
      }
    }
    return configuration;
  }

  HardfileHandler::HardfileHandler(VirtualMachine *vm) : _vm(vm)
  {
    memset(&_rom, 0, sizeof(_rom));
  }

  HardfileHandler::~HardfileHandler() = default;
}

namespace fellow::api::modules
{
  IHardfileHandler *CreateHardfileHandler(VirtualMachine *vm)
  {
    hardfile::hardfileHandlerInstance = new fellow::hardfile::HardfileHandler(vm);
    return hardfile::hardfileHandlerInstance;
  }
}
