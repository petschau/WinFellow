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
#include "fellow/api/module/IHardfileHandler.h"
#include "hardfile/HardfileHandler.h"
#include "hardfile/rdb/RDBHandler.h"
#include "hardfile/hunks/HunkRelocator.h"
#include "VirtualHost/Core.h"

using namespace Service;
using namespace fellow::hardfile::rdb;
using namespace fellow::api::service;
using namespace fellow::api::module;
using namespace fellow::api;
using namespace std;

namespace fellow::api::module
{
  IHardfileHandler* HardfileHandler = new fellow::hardfile::HardfileHandler();
}

namespace fellow::hardfile
{
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

  void HardfileHandler_CardInit()
  {
    fellow::api::module::HardfileHandler->CardInit();
  }

  void HardfileHandler_CardMap(uint32_t mapping)
  {
    fellow::api::module::HardfileHandler->CardMap(mapping);
  }

  uint8_t HardfileHandler_ReadByte(uint32_t address)
  {
    return fellow::api::module::HardfileHandler->ReadByte(address);
  }

  uint16_t HardfileHandler_ReadWord(uint32_t address)
  {
    return fellow::api::module::HardfileHandler->ReadWord(address);
  }

  uint32_t HardfileHandler_ReadLong(uint32_t address)
  {
    return fellow::api::module::HardfileHandler->ReadLong(address);
  }

  void HardfileHandler_WriteByte(uint8_t data, uint32_t address)
  {
    // Deliberately left blank
  }

  void HardfileHandler_WriteWord(uint16_t data, uint32_t address)
  {
    // Deliberately left blank
  }

  void HardfileHandler_WriteLong(uint32_t data, uint32_t address)
  {
    // Deliberately left blank
  }

  /*================================================================*/
  /* fhfile_card_init                                               */
  /* In order to obtain a configDev struct.                         */
  /*================================================================*/

  void HardfileHandler::CardInit()
  {
    VM->Memory.EmemSet(0, 0xd1);
    VM->Memory.EmemSet(8, 0xc0);
    VM->Memory.EmemSet(4, 2);
    VM->Memory.EmemSet(0x10, 2011 >> 8);
    VM->Memory.EmemSet(0x14, 2011 & 0xf);
    VM->Memory.EmemSet(0x18, 0);
    VM->Memory.EmemSet(0x1c, 0);
    VM->Memory.EmemSet(0x20, 0);
    VM->Memory.EmemSet(0x24, 1);
    VM->Memory.EmemSet(0x28, 0x10);
    VM->Memory.EmemSet(0x2c, 0);
    VM->Memory.EmemSet(0x40, 0);
    VM->Memory.EmemMirror(0x1000, _rom + 0x1000, 0xa0);
  }

  /*====================================================*/
  /* fhfile_card_map                                    */
  /* The rom must be remapped to the location specified.*/
  /*====================================================*/

  void HardfileHandler::CardMap(uint32_t mapping)
  {
    _romstart = (mapping << 8) & 0xff0000;
    uint32_t bank = _romstart >> 16;
    VM->Memory.BankSet(HardfileHandler_ReadByte,
      HardfileHandler_ReadWord,
      HardfileHandler_ReadLong,
      HardfileHandler_WriteByte,
      HardfileHandler_WriteWord,
      HardfileHandler_WriteLong,
      _rom,
      bank,
      bank,
      FALSE);
  }

  /*============================================================================*/
  /* Functions to get and set data in the fhfile memory area                    */
  /*============================================================================*/

  uint8_t HardfileHandler::ReadByte(uint32_t address)
  {
    return _rom[address & 0xffff];
  }

  uint16_t HardfileHandler::ReadWord(uint32_t address)
  {
    uint8_t* p = _rom + (address & 0xffff);
    return (static_cast<uint16_t>(p[0]) << 8) | static_cast<uint16_t>(p[1]);
  }

  uint32_t HardfileHandler::ReadLong(uint32_t address)
  {
    uint8_t* p = _rom + (address & 0xffff);
    return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) | (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
  }

  bool HardfileHandler::HasZeroDevices()
  {
    for (auto& _device : _devices)
    {
      if (_device.F != nullptr)
      {
        return false;
      }
    }
    return true;
  }

  bool HardfileHandler::PreferredNameExists(const string& preferredName)
  {
    return std::any_of(_mountList.begin(), _mountList.end(), [preferredName](auto& mountListEntry) {return preferredName == mountListEntry->Name; });
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

  string HardfileHandler::MakeDeviceName(const string& preferredName)
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
          RDB* rdbHeader = _devices[deviceIndex].RDB;

          for (unsigned int partitionIndex = 0; partitionIndex < rdbHeader->Partitions.size(); partitionIndex++)
          {
            RDBPartition* rdbPartition = rdbHeader->Partitions[partitionIndex].get();
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

  bool HardfileHandler::FindOlderOrSameFileSystemVersion(uint32_t DOSType, uint32_t version, unsigned int& olderOrSameFileSystemIndex)
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

  HardfileFileSystemEntry* HardfileHandler::GetFileSystemForDOSType(uint32_t DOSType)
  {
    for (auto& fileSystemEntry : _fileSystems)
    {
      if (fileSystemEntry->IsDOSType(DOSType))
      {
        return fileSystemEntry.get();
      }
    }
    return nullptr;
  }

  void HardfileHandler::AddFileSystemsFromRdb(HardfileDevice& device)
  {
    if (device.F == nullptr || !device.HasRDB)
    {
      return;
    }

    for (auto& header : device.RDB->FileSystemHeaders)
    {
      unsigned int olderOrSameFileSystemIndex = 0;
      bool hasOlderOrSameFileSystemVersion = FindOlderOrSameFileSystemVersion(header->DOSType, header->Version, olderOrSameFileSystemIndex);
      if (!hasOlderOrSameFileSystemVersion)
      {
        _fileSystems.push_back(make_unique<HardfileFileSystemEntry>(header.get(), 0));
      }
      else if (_fileSystems[olderOrSameFileSystemIndex]->IsOlderVersion(header->Version))
      {
        // Replace older fs version with this one
        _fileSystems[olderOrSameFileSystemIndex].reset(new HardfileFileSystemEntry(header.get(), 0));
      }
      // Ignore if newer or same fs version already added
    }
  }

  void HardfileHandler::AddFileSystemsFromRdb()
  {
    for (auto& _device : _devices)
    {
      AddFileSystemsFromRdb(_device);
    }
  }

  void HardfileHandler::EraseOlderOrSameFileSystemVersion(uint32_t DOSType, uint32_t version)
  {
    unsigned int olderOrSameFileSystemIndex = 0;
    bool hasOlderOrSameFileSystemVersion = FindOlderOrSameFileSystemVersion(DOSType, version, olderOrSameFileSystemIndex);
    if (hasOlderOrSameFileSystemVersion)
    {
      _core.Log->AddLogDebug("fhfile: Erased RDB filesystem entry (%.8X, %.8X), newer version (%.8X, %.8X) found in RDB or newer/same version supported by Kickstart.\n",
        _fileSystems[olderOrSameFileSystemIndex]->GetDOSType(), _fileSystems[olderOrSameFileSystemIndex]->GetVersion(), DOSType, version);

      _fileSystems.erase(_fileSystems.begin() + olderOrSameFileSystemIndex);
    }
  }

  void HardfileHandler::SetHardfileConfigurationFromRDB(HardfileConfiguration& config, RDB* rdb, bool readonly)
  {
    HardfileGeometry& geometry = config.Geometry;
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
      fellow::hardfile::rdb::RDBPartition* rdbPartition = rdb->Partitions[i].get();
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

  bool HardfileHandler::OpenHardfileFile(HardfileDevice& device)
  {
    if (device.Configuration.Filename.empty())
    {
      return false;
    }

    fs_wrapper_point* fsnp = _core.FSWrapper->MakePoint(device.Configuration.Filename.c_str());
    if (fsnp == nullptr)
    {
      _core.Log->AddLog("ERROR: Unable to access hardfile '%s', it is either inaccessible, or too big (2GB or more).\n", device.Configuration.Filename.c_str());
      return false;
    }

    if (fsnp != nullptr)
    {
      device.Readonly = device.Configuration.Readonly || (!fsnp->writeable);
      fopen_s(&device.F, device.Configuration.Filename.c_str(), device.Readonly ? "rb" : "r+b");
      device.FileSize = fsnp->size;
      delete fsnp;
    }

    const auto& geometry = device.Configuration.Geometry;
    uint32_t cylinderSize = geometry.Surfaces * geometry.SectorsPerTrack * geometry.BytesPerSector;
    if (device.FileSize < cylinderSize)
    {
      fclose(device.F);
      device.F = nullptr;
      _core.Log->AddLog("ERROR: Hardfile '%s' was not mounted, size is less than one cylinder.\n", device.Configuration.Filename.c_str());
      return false;
    }
    return true;
  }

  void HardfileHandler::ClearDeviceRuntimeInfo(HardfileDevice& device)
  {
    if (device.F != nullptr)
    {
      fclose(device.F);
      device.F = nullptr;
    }

    device.Status = FHFILE_NONE;
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
    HardfileDevice& device = _devices[index];

    ClearDeviceRuntimeInfo(device);

    if (OpenHardfileFile(device))
    {
      RDBFileReader reader(device.F);
      rdb_status rdbResult = RDBHandler::HasRigidDiskBlock(reader);

      if (rdbResult == rdb_status::RDB_FOUND_WITH_HEADER_CHECKSUM_ERROR)
      {
        ClearDeviceRuntimeInfo(device);

        _core.Log->AddLog("Hardfile: File skipped '%s', RDB header has checksum error.\n", device.Configuration.Filename.c_str());
        return;
      }

      if (rdbResult == rdb_status::RDB_FOUND_WITH_PARTITION_ERROR)
      {
        ClearDeviceRuntimeInfo(device);

        _core.Log->AddLog("Hardfile: File skipped '%s', RDB partition has checksum error.\n", device.Configuration.Filename.c_str());
        return;
      }

      device.HasRDB = rdbResult == rdb_status::RDB_FOUND;

      if (device.HasRDB)
      {
        // RDB configured hardfile
        RDB* rdb = RDBHandler::GetDriveInformation(reader);

        if (rdb->HasFileSystemHandlerErrors)
        {
          ClearDeviceRuntimeInfo(device);

          _core.Log->AddLog("Hardfile: File skipped '%s', RDB filesystem handler has checksum error.\n", device.Configuration.Filename.c_str());
          return;
        }

        device.RDB = rdb;
        SetHardfileConfigurationFromRDB(device.Configuration, device.RDB, device.Readonly);
      }

      HardfileGeometry& geometry = device.Configuration.Geometry;
      if (!device.HasRDB)
      {
        // Manually configured hardfile
        uint32_t cylinderSize = geometry.Surfaces * geometry.SectorsPerTrack * geometry.BytesPerSector;
        uint32_t cylinders = device.FileSize / cylinderSize;
        geometry.Tracks = cylinders * geometry.Surfaces;
        geometry.LowCylinder = 0;
        geometry.HighCylinder = cylinders - 1;
      }
      device.GeometrySize = geometry.Tracks * geometry.SectorsPerTrack * geometry.BytesPerSector;
      device.Status = FHFILE_HDF;

      if (device.FileSize < device.GeometrySize)
      {
        ClearDeviceRuntimeInfo(device);

        _core.Log->AddLog("Hardfile: File skipped, geometry for %s is larger than the file.\n", device.Configuration.Filename.c_str());
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

  unsigned int HardfileHandler::GetIndexFromUnitNumber(uint32_t unit)
  {
    uint32_t address = unit % 10;
    uint32_t lun = (unit / 10) % 10;

    if (lun > 7)
    {
      _core.Log->AddLogDebug("ERROR: Unit number is not in a valid format.\n");
      return 0xffffffff;
    }
    return lun + address * 8;
  }

  uint32_t HardfileHandler::GetUnitNumberFromIndex(unsigned int index)
  {
    uint32_t address = index / 8;
    uint32_t lun = index % 8;

    return lun * 10 + address;
  }
  void HardfileHandler::SetHardfile(const HardfileConfiguration& configuration, unsigned int index)
  {
    if (index >= GetMaxHardfileCount())
    {
      return;
    }

    RemoveHardfile(index);
    HardfileDevice& device = _devices[index];
    device.Configuration = configuration;
    InitializeHardfile(index);

    _core.Log->AddLog("SetHardfile('%s', %u)\n", configuration.Filename.c_str(), index);

    fellow::api::Service->RP.SendHardDriveContent(index, configuration.Filename.c_str(), configuration.Readonly);
  }

  bool HardfileHandler::CompareHardfile(const HardfileConfiguration& configuration, unsigned int index)
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

  void HardfileHandler::SetIOError(int8_t errorCode)
  {
    VM->Memory.WriteByte(errorCode, VM->CPU.GetAReg(1) + 31);
  }

  void HardfileHandler::SetIOActual(uint32_t ioActual)
  {
    VM->Memory.WriteLong(ioActual, VM->CPU.GetAReg(1) + 32);
  }

  uint32_t HardfileHandler::GetUnitNumber()
  {
    return VM->Memory.ReadLong(VM->CPU.GetAReg(1) + 24);
  }

  uint16_t HardfileHandler::GetCommand()
  {
    return VM->Memory.ReadWord(VM->CPU.GetAReg(1) + 28);
  }

  /*==================*/
  /* BeginIO Commands */
  /*==================*/

  void HardfileHandler::IgnoreOK(uint32_t index)
  {
    SetIOError(0); // io_Error - 0 - success
  }

  int8_t HardfileHandler::Read(uint32_t index)
  {
    if (index == 2)
    {
      int ihj = 0;
    }


    if (_devices[index].F == nullptr)
    {
      _core.Log->AddLogDebug("CMD_READ Unit %d (%d) ERROR-TDERR_BadUnitNum\n", GetUnitNumberFromIndex(index), index);
      return 32; // TDERR_BadUnitNum
    }

    uint32_t dest = VM->Memory.ReadLong(VM->CPU.GetAReg(1) + 40);
    uint32_t offset = VM->Memory.ReadLong(VM->CPU.GetAReg(1) + 44);
    uint32_t length = VM->Memory.ReadLong(VM->CPU.GetAReg(1) + 36);

    _core.Log->AddLogDebug("CMD_READ Unit %d (%d) Destination %.8X Offset %.8X Length %.8X\n", GetUnitNumberFromIndex(index), index, dest, offset, length);

    if ((offset + length) > _devices[index].GeometrySize)
    {
      return -3; // TODO: Out of range, -3 is not the right code
    }

    fellow::api::Service->HUD.SetHarddiskLED(index, true, false);

    fseek(_devices[index].F, offset, SEEK_SET);
    fread(VM->Memory.AddressToPtr(dest), 1, length, _devices[index].F);
    SetIOActual(length);

    fellow::api::Service->HUD.SetHarddiskLED(index, false, false);

    return 0;
  }

  int8_t HardfileHandler::Write(uint32_t index)
  {
    if (_devices[index].F == nullptr)
    {
      _core.Log->AddLogDebug("CMD_WRITE Unit %d (%d) ERROR-TDERR_BadUnitNum\n", GetUnitNumberFromIndex(index), index);
      return 32; // TDERR_BadUnitNum
    }

    uint32_t dest = VM->Memory.ReadLong(VM->CPU.GetAReg(1) + 40);
    uint32_t offset = VM->Memory.ReadLong(VM->CPU.GetAReg(1) + 44);
    uint32_t length = VM->Memory.ReadLong(VM->CPU.GetAReg(1) + 36);

    _core.Log->AddLogDebug("CMD_WRITE Unit %d (%d) Destination %.8X Offset %.8X Length %.8X\n", GetUnitNumberFromIndex(index), index, dest, offset, length);

    if (_devices[index].Readonly || (offset + length) > _devices[index].GeometrySize)
    {
      return -3;  // TODO: Out of range, -3 is probably not the right one.
    }

    fellow::api::Service->HUD.SetHarddiskLED(index, true, true);

    fseek(_devices[index].F, offset, SEEK_SET);
    fwrite(VM->Memory.AddressToPtr(dest), 1, length, _devices[index].F);
    SetIOActual(length);

    fellow::api::Service->HUD.SetHarddiskLED(index, false, true);

    return 0;
  }

  int8_t HardfileHandler::GetNumberOfTracks(uint32_t index)
  {
    if (_devices[index].F == nullptr)
    {
      return 32; // TDERR_BadUnitNum
    }

    SetIOActual(_devices[index].Configuration.Geometry.Tracks);
    return 0;
  }

  int8_t HardfileHandler::GetDiskDriveType(uint32_t index)
  {
    if (_devices[index].F == nullptr)
    {
      return 32; // TDERR_BadUnitNum
    }

    SetIOActual(1);

    return 0;
  }

  void HardfileHandler::WriteProt(uint32_t index)
  {
    SetIOActual(_devices[index].Readonly ? 1 : 0);
  }

  int8_t HardfileHandler::ScsiDirect(uint32_t index)
  {
    int8_t error = 0;
    uint32_t scsiCmdStruct = VM->Memory.ReadLong(VM->CPU.GetAReg(1) + 40); // io_Data

    _core.Log->AddLogDebug("HD_SCSICMD Unit %d (%d) ScsiCmd at %.8X\n", GetUnitNumberFromIndex(index), index, scsiCmdStruct);

    uint32_t scsiCommand = VM->Memory.ReadLong(scsiCmdStruct + 12);
    uint16_t scsiCommandLength = VM->Memory.ReadWord(scsiCmdStruct + 16);

    _core.Log->AddLogDebug("HD_SCSICMD Command length %d, data", scsiCommandLength);

    for (int i = 0; i < scsiCommandLength; i++)
    {
      _core.Log->AddLogDebug(" %.2X", VM->Memory.ReadByte(scsiCommand + i));
    }
    _core.Log->AddLogDebug("\n");

    uint8_t commandNumber = VM->Memory.ReadByte(scsiCommand);
    uint32_t returnData = VM->Memory.ReadLong(scsiCmdStruct);
    switch (commandNumber)
    {
    case 0x25: // Read capacity (10)
      _core.Log->AddLogDebug("SCSI direct command 0x25 Read Capacity\n");
      {
        uint32_t bytesPerSector = _devices[index].Configuration.Geometry.BytesPerSector;
        bool pmi = !!(VM->Memory.ReadByte(scsiCommand + 8) & 1);

        if (pmi)
        {
          uint32_t blocksPerCylinder = (_devices[index].Configuration.Geometry.SectorsPerTrack * _devices[index].Configuration.Geometry.Surfaces) - 1;
          VM->Memory.WriteByte(blocksPerCylinder >> 24, returnData);
          VM->Memory.WriteByte(blocksPerCylinder >> 16, returnData + 1);
          VM->Memory.WriteByte(blocksPerCylinder >> 8, returnData + 2);
          VM->Memory.WriteByte(blocksPerCylinder, returnData + 3);
        }
        else
        {
          uint32_t blocksOnDevice = (_devices[index].GeometrySize / _devices[index].Configuration.Geometry.BytesPerSector) - 1;
          VM->Memory.WriteByte(blocksOnDevice >> 24, returnData);
          VM->Memory.WriteByte(blocksOnDevice >> 16, returnData + 1);
          VM->Memory.WriteByte(blocksOnDevice >> 8, returnData + 2);
          VM->Memory.WriteByte(blocksOnDevice, returnData + 3);
        }
        VM->Memory.WriteByte(bytesPerSector >> 24, returnData + 4);
        VM->Memory.WriteByte(bytesPerSector >> 16, returnData + 5);
        VM->Memory.WriteByte(bytesPerSector >> 8, returnData + 6);
        VM->Memory.WriteByte(bytesPerSector, returnData + 7);

        VM->Memory.WriteByte(0, scsiCmdStruct + 8); // data actual length (long word)
        VM->Memory.WriteByte(0, scsiCmdStruct + 9);
        VM->Memory.WriteByte(0, scsiCmdStruct + 10);
        VM->Memory.WriteByte(8, scsiCmdStruct + 11);

        VM->Memory.WriteByte(0, scsiCmdStruct + 21); // Status
      }
      break;
    case 0x37: // Read defect Data (10)
      _core.Log->AddLogDebug("SCSI direct command 0x37 Read defect Data\n");

      VM->Memory.WriteByte(0, returnData);
      VM->Memory.WriteByte(VM->Memory.ReadByte(scsiCommand + 2), returnData + 1);
      VM->Memory.WriteByte(0, returnData + 2); // No defects (word)
      VM->Memory.WriteByte(0, returnData + 3);

      VM->Memory.WriteByte(0, scsiCmdStruct + 8); // data actual length (long word)
      VM->Memory.WriteByte(0, scsiCmdStruct + 9);
      VM->Memory.WriteByte(0, scsiCmdStruct + 10);
      VM->Memory.WriteByte(4, scsiCmdStruct + 11);

      VM->Memory.WriteByte(0, scsiCmdStruct + 21); // Status
      break;
    case 0x12: // Inquiry
      _core.Log->AddLogDebug("SCSI direct command 0x12 Inquiry\n");

      VM->Memory.WriteByte(0, returnData); // Pheripheral type 0 connected (magnetic disk)
      VM->Memory.WriteByte(0, returnData + 1); // Not removable
      VM->Memory.WriteByte(0, returnData + 2); // Does not claim conformance to any standard
      VM->Memory.WriteByte(2, returnData + 3);
      VM->Memory.WriteByte(32, returnData + 4); // Additional length
      VM->Memory.WriteByte(0, returnData + 5);
      VM->Memory.WriteByte(0, returnData + 6);
      VM->Memory.WriteByte(0, returnData + 7);
      VM->Memory.WriteByte('F', returnData + 8);
      VM->Memory.WriteByte('E', returnData + 9);
      VM->Memory.WriteByte('L', returnData + 10);
      VM->Memory.WriteByte('L', returnData + 11);
      VM->Memory.WriteByte('O', returnData + 12);
      VM->Memory.WriteByte('W', returnData + 13);
      VM->Memory.WriteByte(' ', returnData + 14);
      VM->Memory.WriteByte(' ', returnData + 15);
      VM->Memory.WriteByte('H', returnData + 16);
      VM->Memory.WriteByte('a', returnData + 17);
      VM->Memory.WriteByte('r', returnData + 18);
      VM->Memory.WriteByte('d', returnData + 19);
      VM->Memory.WriteByte('f', returnData + 20);
      VM->Memory.WriteByte('i', returnData + 21);
      VM->Memory.WriteByte('l', returnData + 22);
      VM->Memory.WriteByte('e', returnData + 23);
      VM->Memory.WriteByte(' ', returnData + 24);
      VM->Memory.WriteByte(' ', returnData + 25);
      VM->Memory.WriteByte(' ', returnData + 26);
      VM->Memory.WriteByte(' ', returnData + 27);
      VM->Memory.WriteByte(' ', returnData + 28);
      VM->Memory.WriteByte(' ', returnData + 29);
      VM->Memory.WriteByte(' ', returnData + 30);
      VM->Memory.WriteByte(' ', returnData + 31);
      VM->Memory.WriteByte('1', returnData + 32);
      VM->Memory.WriteByte('.', returnData + 33);
      VM->Memory.WriteByte('0', returnData + 34);
      VM->Memory.WriteByte(' ', returnData + 35);

      VM->Memory.WriteByte(0, scsiCmdStruct + 8); // data actual length (long word)
      VM->Memory.WriteByte(0, scsiCmdStruct + 9);
      VM->Memory.WriteByte(0, scsiCmdStruct + 10);
      VM->Memory.WriteByte(36, scsiCmdStruct + 11);

      VM->Memory.WriteByte(0, scsiCmdStruct + 21); // Status
      break;
    case 0x1a: // Mode sense
      _core.Log->AddLogDebug("SCSI direct command 0x1a Mode sense\n");
      {
        // Show values for debug
        uint32_t senseData = VM->Memory.ReadLong(scsiCmdStruct + 22); // senseData and related fields are only used for autosensing error condition when the command fail
        uint16_t senseLengthAllocated = VM->Memory.ReadWord(scsiCmdStruct + 26); // Primary mode sense data go to scsi_Data
        uint8_t scsciCommandFlags = VM->Memory.ReadByte(scsiCmdStruct + 20);

        uint8_t pageCode = VM->Memory.ReadByte(scsiCommand + 2) & 0x3f;
        if (pageCode == 3)
        {
          uint16_t sectorsPerTrack = _devices[index].Configuration.Geometry.SectorsPerTrack;
          uint16_t bytesPerSector = _devices[index].Configuration.Geometry.BytesPerSector;

          // Header
          VM->Memory.WriteByte(24 + 3, returnData);
          VM->Memory.WriteByte(0, returnData + 1);
          VM->Memory.WriteByte(0, returnData + 2);
          VM->Memory.WriteByte(0, returnData + 3);

          // Page
          uint32_t destination = returnData + 4;
          VM->Memory.WriteByte(3, destination);         // Page 3 format device
          VM->Memory.WriteByte(0x16, destination + 1);  // Page length
          VM->Memory.WriteByte(0, destination + 2);     // Tracks per zone
          VM->Memory.WriteByte(1, destination + 3);
          VM->Memory.WriteByte(0, destination + 4);     // Alternate sectors per zone
          VM->Memory.WriteByte(0, destination + 5);
          VM->Memory.WriteByte(0, destination + 6);     // Alternate tracks per zone
          VM->Memory.WriteByte(0, destination + 7);
          VM->Memory.WriteByte(0, destination + 8);     // Alternate tracks per volume
          VM->Memory.WriteByte(0, destination + 9);
          VM->Memory.WriteByte(sectorsPerTrack >> 8, destination + 10); // Sectors per track
          VM->Memory.WriteByte(sectorsPerTrack & 0xff, destination + 11);
          VM->Memory.WriteByte(bytesPerSector >> 8, destination + 12); // Data bytes per physical sector
          VM->Memory.WriteByte(bytesPerSector & 0xff, destination + 13);
          VM->Memory.WriteByte(0, destination + 14);     // Interleave
          VM->Memory.WriteByte(1, destination + 15);
          VM->Memory.WriteByte(0, destination + 16);     // Track skew factor
          VM->Memory.WriteByte(0, destination + 17);
          VM->Memory.WriteByte(0, destination + 18);     // Cylinder skew factor
          VM->Memory.WriteByte(0, destination + 19);
          VM->Memory.WriteByte(0x80, destination + 20);
          VM->Memory.WriteByte(0, destination + 21);
          VM->Memory.WriteByte(0, destination + 22);
          VM->Memory.WriteByte(0, destination + 23);

          VM->Memory.WriteByte(0, scsiCmdStruct + 8); // data actual length (long word)
          VM->Memory.WriteByte(0, scsiCmdStruct + 9);
          VM->Memory.WriteByte(0, scsiCmdStruct + 10);
          VM->Memory.WriteByte(24 + 4, scsiCmdStruct + 11);

          VM->Memory.WriteByte(0, scsiCmdStruct + 28); // sense actual length (word)
          VM->Memory.WriteByte(0, scsiCmdStruct + 29);
          VM->Memory.WriteByte(0, scsiCmdStruct + 21); // Status
        }
        else if (pageCode == 4)
        {
          uint32_t numberOfCylinders = _devices[index].Configuration.Geometry.HighCylinder + 1;
          uint8_t surfaces = _devices[index].Configuration.Geometry.Surfaces;

          // Header
          VM->Memory.WriteByte(24 + 3, returnData);
          VM->Memory.WriteByte(0, returnData + 1);
          VM->Memory.WriteByte(0, returnData + 2);
          VM->Memory.WriteByte(0, returnData + 3);

          // Page
          uint32_t destination = returnData + 4;
          VM->Memory.WriteByte(4, destination);         // Page 4 Rigid disk geometry
          VM->Memory.WriteByte(0x16, destination + 1);  // Page length
          VM->Memory.WriteByte(numberOfCylinders >> 16, destination + 2);     // Number of cylinders (3 bytes)
          VM->Memory.WriteByte(numberOfCylinders >> 8, destination + 3);
          VM->Memory.WriteByte(numberOfCylinders, destination + 4);
          VM->Memory.WriteByte(surfaces, destination + 5);     // Number of heads
          VM->Memory.WriteByte(0, destination + 6);     // Starting cylinder write precomp (3 bytes)
          VM->Memory.WriteByte(0, destination + 7);
          VM->Memory.WriteByte(0, destination + 8);
          VM->Memory.WriteByte(0, destination + 9);     // Starting cylinder reduces write current (3 bytes)
          VM->Memory.WriteByte(0, destination + 10);
          VM->Memory.WriteByte(0, destination + 11);
          VM->Memory.WriteByte(0, destination + 12);    // Drive step rate
          VM->Memory.WriteByte(0, destination + 13);
          VM->Memory.WriteByte(numberOfCylinders >> 16, destination + 14);    // Landing zone cylinder (3 bytes)
          VM->Memory.WriteByte(numberOfCylinders >> 8, destination + 15);
          VM->Memory.WriteByte(numberOfCylinders, destination + 16);
          VM->Memory.WriteByte(0, destination + 17);    // Nothing
          VM->Memory.WriteByte(0, destination + 18);    // Rotational offset
          VM->Memory.WriteByte(0, destination + 19);    // Reserved
          VM->Memory.WriteByte(0x1c, destination + 20);    // Medium rotation rate
          VM->Memory.WriteByte(0x20, destination + 21);
          VM->Memory.WriteByte(0, destination + 22);    // Reserved
          VM->Memory.WriteByte(0, destination + 23);    // Reserved

          VM->Memory.WriteByte(0, scsiCmdStruct + 8); // data actual length (long word)
          VM->Memory.WriteByte(0, scsiCmdStruct + 9);
          VM->Memory.WriteByte(0, scsiCmdStruct + 10);
          VM->Memory.WriteByte(24 + 4, scsiCmdStruct + 11);

          VM->Memory.WriteByte(0, scsiCmdStruct + 28); // sense actual length (word)
          VM->Memory.WriteByte(0, scsiCmdStruct + 29);
          VM->Memory.WriteByte(0, scsiCmdStruct + 21); // Status
        }
        else
        {
          error = -3;
        }
      }
      break;
    default:
      _core.Log->AddLogDebug("SCSI direct command Unimplemented 0x%.2X\n", commandNumber);

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

    _configdev = VM->CPU.GetAReg(3);
    VM->Memory.DmemSetLongNoCounter(FHFILE_MAX_DEVICES, 4088);
    VM->Memory.DmemSetLongNoCounter(_configdev, 4092);
    VM->CPU.SetDReg(0, 1);
  }

  /*======================================*/
  /* Native callbacks for device commands */
  /*======================================*/

  // Returns D0 - 0 - Success, non-zero - Error
  void HardfileHandler::DoOpen()
  {
    uint32_t unit = VM->CPU.GetDReg(0);
    unsigned int index = GetIndexFromUnitNumber(unit);

    if (index < FHFILE_MAX_DEVICES && _devices[index].F != nullptr)
    {
      VM->Memory.WriteByte(7, VM->CPU.GetAReg(1) + 8);                     /* ln_type (NT_REPLYMSG) */
      SetIOError(0);                                                       /* io_error */
      VM->Memory.WriteLong(VM->CPU.GetDReg(0), VM->CPU.GetAReg(1) + 24);   /* io_unit */
      VM->Memory.WriteLong(VM->Memory.ReadLong(VM->CPU.GetAReg(6) + 32) + 1, VM->CPU.GetAReg(6) + 32);  /* LIB_OPENCNT */
      VM->CPU.SetDReg(0, 0);                                               /* Success */
    }
    else
    {
      VM->Memory.WriteLong(static_cast<uint32_t>(-1), VM->CPU.GetAReg(1) + 20);
      SetIOError(-1);                                                      /* io_error */
      VM->CPU.SetDReg(0, static_cast<uint32_t>(-1));                            /* Fail */
    }
  }

  void HardfileHandler::DoClose()
  {
    VM->Memory.WriteLong(VM->Memory.ReadLong(VM->CPU.GetAReg(6) + 32) - 1, VM->CPU.GetAReg(6) + 32); /* LIB_OPENCNT */
    VM->CPU.SetDReg(0, 0);                                                                           /* Causes invalid free-mem entry recoverable alert if omitted */
  }

  void HardfileHandler::DoExpunge()
  {
    VM->CPU.SetDReg(0, 0); /* ? */
  }

  void HardfileHandler::DoNULL()
  {
    VM->CPU.SetDReg(0, 0); /* ? */
  }

  // void BeginIO(io_req)
  void HardfileHandler::DoBeginIO()
  {
    int8_t error = 0;
    uint32_t unit = GetUnitNumber();
    unsigned int index = GetIndexFromUnitNumber(unit);
    uint16_t cmd = GetCommand();

    switch (cmd)
    {
    case 2:  // CMD_READ
      error = Read(index);
      break;
    case 3:  // CMD_WRITE
      error = Write(index);
      break;
    case 11: // TD_FORMAT
      _core.Log->AddLogDebug("TD_FORMAT Unit %d\n", unit);
      error = Write(index);
      break;
    case 18: // TD_GETDRIVETYPE
      _core.Log->AddLogDebug("TD_GETDRIVETYPE Unit %d\n", unit);
      // This does not make sense for us, options are 3.5" and 5 1/4"
      error = GetDiskDriveType(index);
      break;
    case 19: // TD_GETNUMTRACKS
      _core.Log->AddLogDebug("TD_GETNUMTRACKS Unit %d\n", unit);
      error = GetNumberOfTracks(index);
      break;
    case 15: // TD_PROTSTATUS
      _core.Log->AddLogDebug("TD_PROTSTATUS Unit %d\n", unit);
      WriteProt(index);
      break;
    case 4: // CMD_UPDATE ie. flush
      _core.Log->AddLogDebug("CMD_UPDATE Unit %d\n", unit);
      IgnoreOK(index);
      break;
    case 5: // CMD_CLEAR
      _core.Log->AddLogDebug("CMD 5 Unit %d\n", unit);
      IgnoreOK(index);
      break;
    case 9:  // TD_MOTOR Only really used to turn motor off since device will turn on the motor automatically if reads and writes are received
      _core.Log->AddLogDebug("TD_MOTOR Unit %d\n", unit);
      // Should set previous state of motor
      IgnoreOK(index);
      break;
    case 10: // TD_SEEK Used to pre-seek in advance, but reads and writes will also do the necessary seeks, so not useful here.
      _core.Log->AddLogDebug("TD_SEEK Unit %d\n", unit);
      IgnoreOK(index);
      break;
    case 12: // TD_REMOVE
      _core.Log->AddLogDebug("TD_REMOVE Unit %d\n", unit);
      // Perhaps unsupported instead?
      IgnoreOK(index);
      break;
    case 13: // TD_CHANGENUM
      _core.Log->AddLogDebug("TD_CHANGENUM Unit %d\n", unit);
      // Should perhaps set some data here
      IgnoreOK(index);
      break;
    case 14: // TD_CHANGESTATE - check if a disk is currently in a drive. io_Actual - 0 - disk, 1 - no disk
      _core.Log->AddLogDebug("TD_CHANGESTATE Unit %d\n", unit);
      SetIOError(0); // io_Error - 0 - success
      SetIOActual(0);
      break;
    case 20: // TD_ADDCHANGEINT
      _core.Log->AddLogDebug("TD_ADDCHANGEINT Unit %d\n", unit);
      IgnoreOK(index);
      break;
    case 21: // TD_REMCHANGEINT
      _core.Log->AddLogDebug("TD_REMCHANGEINT Unit %d\n", unit);
      IgnoreOK(index);
      break;
    case 16: // TD_RAWREAD
      _core.Log->AddLogDebug("TD_RAWREAD Unit %d\n", unit);
      error = -3;
      break;
    case 17: // TD_RAWWRITE
      _core.Log->AddLogDebug("TD_RAWWRITE Unit %d\n", unit);
      error = -3;
      break;
    case 22: // TD_GETGEOMETRY
      _core.Log->AddLogDebug("TD_GEOMETRY Unit %d\n", unit);
      error = -3;
      break;
    case 23: // TD_EJECT
      _core.Log->AddLogDebug("TD_EJECT Unit %d\n", unit);
      error = -3;
      break;
    case 28:
      error = ScsiDirect(index);
      break;
    default:
      _core.Log->AddLogDebug("CMD Unknown %d Unit %d\n", cmd, unit);
      error = -3;
      break;
    }
    VM->Memory.WriteByte(5, VM->CPU.GetAReg(1) + 8);      /* ln_type (Reply message) */
    SetIOError(error); // ln_error
  }

  void HardfileHandler::DoAbortIO()
  {
    // Set some more success values here, this is ok, we never have pending io.
    VM->CPU.SetDReg(0, static_cast<uint32_t>(-3));
  }

  // RDB support functions, native callbacks

  uint32_t HardfileHandler::DoGetRDBFileSystemCount()
  {
    uint32_t count = (uint32_t)_fileSystems.size();
    _core.Log->AddLogDebug("fhfile: DoGetRDBFilesystemCount() - Returns %u\n", count);
    return count;
  }

  uint32_t HardfileHandler::DoGetRDBFileSystemHunkCount(uint32_t fileSystemIndex)
  {
    uint32_t hunkCount = (uint32_t)_fileSystems[fileSystemIndex]->Header->FileSystemHandler.FileImage.GetInitialHunkCount();
    _core.Log->AddLogDebug("fhfile: DoGetRDBFileSystemHunkCount(fileSystemIndex: %u) Returns %u\n", fileSystemIndex, hunkCount);
    return hunkCount;
  }

  uint32_t HardfileHandler::DoGetRDBFileSystemHunkSize(uint32_t fileSystemIndex, uint32_t hunkIndex)
  {
    uint32_t hunkSize = _fileSystems[fileSystemIndex]->Header->FileSystemHandler.FileImage.GetInitialHunk(hunkIndex)->GetAllocateSizeInBytes();
    _core.Log->AddLogDebug("fhfile: DoGetRDBFileSystemHunkSize(fileSystemIndex: %u, hunkIndex: %u) Returns %u\n", fileSystemIndex, hunkIndex, hunkSize);
    return hunkSize;
  }

  void HardfileHandler::DoCopyRDBFileSystemHunk(uint32_t destinationAddress, uint32_t fileSystemIndex, uint32_t hunkIndex)
  {
    _core.Log->AddLogDebug("fhfile: DoCopyRDBFileSystemHunk(destinationAddress: %.8X, fileSystemIndex: %u, hunkIndex: %u)\n", destinationAddress, fileSystemIndex, hunkIndex);

    HardfileFileSystemEntry* fileSystem = _fileSystems[fileSystemIndex].get();
    fileSystem->CopyHunkToAddress(destinationAddress + 8, hunkIndex);

    // Remember the address to the first hunk
    if (fileSystem->SegListAddress == 0)
    {
      fileSystem->SegListAddress = destinationAddress + 4;
    }

    uint32_t hunkSize = fileSystem->Header->FileSystemHandler.FileImage.GetInitialHunk(hunkIndex)->GetAllocateSizeInBytes();
    VM->Memory.WriteLong(hunkSize + 8, destinationAddress); // Total size of allocation
    VM->Memory.WriteLong(0, destinationAddress + 4);  // No next segment for now
  }

  void HardfileHandler::DoRelocateFileSystem(uint32_t fileSystemIndex)
  {
    _core.Log->AddLogDebug("fhfile: DoRelocateFileSystem(fileSystemIndex: %u\n", fileSystemIndex);
    HardfileFileSystemEntry* fsEntry = _fileSystems[fileSystemIndex].get();
    fellow::hardfile::hunks::HunkRelocator hunkRelocator(fsEntry->Header->FileSystemHandler.FileImage);
    hunkRelocator.RelocateHunks();
  }

  void HardfileHandler::DoInitializeRDBFileSystemEntry(uint32_t fileSystemEntry, uint32_t fileSystemIndex)
  {
    _core.Log->AddLogDebug("fhfile: DoInitializeRDBFileSystemEntry(fileSystemEntry: %.8X, fileSystemIndex: %u\n", fileSystemEntry, fileSystemIndex);

    HardfileFileSystemEntry* fsEntry = _fileSystems[fileSystemIndex].get();
    RDBFileSystemHeader* fsHeader = fsEntry->Header;

    VM->Memory.WriteLong(_fsname, fileSystemEntry + 10);
    VM->Memory.WriteLong(fsHeader->DOSType, fileSystemEntry + 14);
    VM->Memory.WriteLong(fsHeader->Version, fileSystemEntry + 18);
    VM->Memory.WriteLong(fsHeader->PatchFlags, fileSystemEntry + 22);
    VM->Memory.WriteLong(fsHeader->DnType, fileSystemEntry + 26);
    VM->Memory.WriteLong(fsHeader->DnTask, fileSystemEntry + 30);
    VM->Memory.WriteLong(fsHeader->DnLock, fileSystemEntry + 34);
    VM->Memory.WriteLong(fsHeader->DnHandler, fileSystemEntry + 38);
    VM->Memory.WriteLong(fsHeader->DnStackSize, fileSystemEntry + 42);
    VM->Memory.WriteLong(fsHeader->DnPriority, fileSystemEntry + 46);
    VM->Memory.WriteLong(fsHeader->DnStartup, fileSystemEntry + 50);
    VM->Memory.WriteLong(fsEntry->SegListAddress >> 2, fileSystemEntry + 54);
    //  VM->Memory.WriteLong(fsHeader.DnSegListBlock, fileSystemEntry + 54);
    VM->Memory.WriteLong(fsHeader->DnGlobalVec, fileSystemEntry + 58);

    for (int i = 0; i < 23; i++)
    {
      VM->Memory.WriteLong(fsHeader->Reserved2[i], fileSystemEntry + 62 + i * 4);
    }
  }

  uint32_t HardfileHandler::DoGetDosDevPacketListStart()
  {
    return _dosDevPacketListStart;
  }

  string HardfileHandler::LogGetStringFromMemory(uint32_t address)
  {
    if (address == 0)
    {
      return string();
    }

    string name;
    char c = VM->Memory.ReadByte(address);
    address++;
    while (c != 0)
    {
      name.push_back(c == '\n' ? '.' : c);
      c = VM->Memory.ReadByte(address);
      address++;
    }
    return name;
  }

  void HardfileHandler::DoLogAvailableResources()
  {
    _core.Log->AddLogDebug("fhfile: DoLogAvailableResources()\n");

    uint32_t execBase = VM->Memory.ReadLong(4);  // Fetch list from exec
    uint32_t rsListHeader = VM->Memory.ReadLong(execBase + 0x150);

    _core.Log->AddLogDebug("fhfile: Resource list header (%.8X): Head %.8X Tail %.8X TailPred %.8X Type %d\n",
      rsListHeader,
      VM->Memory.ReadLong(rsListHeader),
      VM->Memory.ReadLong(rsListHeader + 4),
      VM->Memory.ReadLong(rsListHeader + 8),
      VM->Memory.ReadByte(rsListHeader + 9));

    if (rsListHeader == VM->Memory.ReadLong(rsListHeader + 8))
    {
      _core.Log->AddLogDebug("fhfile: Resource list is empty.\n");
      return;
    }

    uint32_t fsNode = VM->Memory.ReadLong(rsListHeader);
    while (fsNode != 0 && (fsNode != rsListHeader + 4))
    {
      _core.Log->AddLogDebug("fhfile: ResourceEntry Node (%.8X): Succ %.8X Pred %.8X Type %d Pri %d NodeName '%s'\n",
        fsNode,
        VM->Memory.ReadLong(fsNode),
        VM->Memory.ReadLong(fsNode + 4),
        VM->Memory.ReadByte(fsNode + 8),
        VM->Memory.ReadByte(fsNode + 9),
        LogGetStringFromMemory(VM->Memory.ReadLong(fsNode + 10)).c_str());

      fsNode = VM->Memory.ReadLong(fsNode);
    }
  }

  void HardfileHandler::DoLogAllocMemResult(uint32_t result)
  {
    _core.Log->AddLogDebug("fhfile: AllocMem() returned %.8X\n", result);
  }

  void HardfileHandler::DoLogOpenResourceResult(uint32_t result)
  {
    _core.Log->AddLogDebug("fhfile: OpenResource() returned %.8X\n", result);
  }

  void HardfileHandler::DoRemoveRDBFileSystemsAlreadySupportedBySystem(uint32_t filesystemResource)
  {
    _core.Log->AddLogDebug("fhfile: DoRemoveRDBFileSystemsAlreadySupportedBySystem(filesystemResource: %.8X)\n", filesystemResource);

    uint32_t fsList = filesystemResource + 18;
    if (fsList == VM->Memory.ReadLong(fsList + 8))
    {
      _core.Log->AddLogDebug("fhfile: FileSystemEntry list is empty.\n");
      return;
    }

    uint32_t fsNode = VM->Memory.ReadLong(fsList);
    while (fsNode != 0 && (fsNode != fsList + 4))
    {
      uint32_t fsEntry = fsNode + 14;
      uint32_t dosType = VM->Memory.ReadLong(fsEntry);
      uint32_t version = VM->Memory.ReadLong(fsEntry + 4);

      _core.Log->AddLogDebug("fhfile: FileSystemEntry DosType   : %.8X\n", VM->Memory.ReadLong(fsEntry));
      _core.Log->AddLogDebug("fhfile: FileSystemEntry Version   : %.8X\n", VM->Memory.ReadLong(fsEntry + 4));
      _core.Log->AddLogDebug("fhfile: FileSystemEntry PatchFlags: %.8X\n", VM->Memory.ReadLong(fsEntry + 8));
      _core.Log->AddLogDebug("fhfile: FileSystemEntry Type      : %.8X\n", VM->Memory.ReadLong(fsEntry + 12));
      _core.Log->AddLogDebug("fhfile: FileSystemEntry Task      : %.8X\n", VM->Memory.ReadLong(fsEntry + 16));
      _core.Log->AddLogDebug("fhfile: FileSystemEntry Lock      : %.8X\n", VM->Memory.ReadLong(fsEntry + 20));
      _core.Log->AddLogDebug("fhfile: FileSystemEntry Handler   : %.8X\n", VM->Memory.ReadLong(fsEntry + 24));
      _core.Log->AddLogDebug("fhfile: FileSystemEntry StackSize : %.8X\n", VM->Memory.ReadLong(fsEntry + 28));
      _core.Log->AddLogDebug("fhfile: FileSystemEntry Priority  : %.8X\n", VM->Memory.ReadLong(fsEntry + 32));
      _core.Log->AddLogDebug("fhfile: FileSystemEntry Startup   : %.8X\n", VM->Memory.ReadLong(fsEntry + 36));
      _core.Log->AddLogDebug("fhfile: FileSystemEntry SegList   : %.8X\n", VM->Memory.ReadLong(fsEntry + 40));
      _core.Log->AddLogDebug("fhfile: FileSystemEntry GlobalVec : %.8X\n\n", VM->Memory.ReadLong(fsEntry + 44));
      fsNode = VM->Memory.ReadLong(fsNode);

      EraseOlderOrSameFileSystemVersion(dosType, version);
    }
  }

  // D0 - pointer to FileSystem.resource
  void HardfileHandler::DoLogAvailableFileSystems(uint32_t fileSystemResource)
  {
    _core.Log->AddLogDebug("fhfile: DoLogAvailableFileSystems(fileSystemResource: %.8X)\n", fileSystemResource);

    uint32_t fsList = fileSystemResource + 18;
    if (fsList == VM->Memory.ReadLong(fsList + 8))
    {
      _core.Log->AddLogDebug("fhfile: FileSystemEntry list is empty.\n");
      return;
    }

    uint32_t fsNode = VM->Memory.ReadLong(fsList);
    while (fsNode != 0 && (fsNode != fsList + 4))
    {
      uint32_t fsEntry = fsNode + 14;

      uint32_t dosType = VM->Memory.ReadLong(fsEntry);
      uint32_t version = VM->Memory.ReadLong(fsEntry + 4);

      _core.Log->AddLogDebug("fhfile: FileSystemEntry DosType   : %.8X\n", dosType);
      _core.Log->AddLogDebug("fhfile: FileSystemEntry Version   : %.8X\n", version);
      _core.Log->AddLogDebug("fhfile: FileSystemEntry PatchFlags: %.8X\n", VM->Memory.ReadLong(fsEntry + 8));
      _core.Log->AddLogDebug("fhfile: FileSystemEntry Type      : %.8X\n", VM->Memory.ReadLong(fsEntry + 12));
      _core.Log->AddLogDebug("fhfile: FileSystemEntry Task      : %.8X\n", VM->Memory.ReadLong(fsEntry + 16));
      _core.Log->AddLogDebug("fhfile: FileSystemEntry Lock      : %.8X\n", VM->Memory.ReadLong(fsEntry + 20));
      _core.Log->AddLogDebug("fhfile: FileSystemEntry Handler   : %.8X\n", VM->Memory.ReadLong(fsEntry + 24));
      _core.Log->AddLogDebug("fhfile: FileSystemEntry StackSize : %.8X\n", VM->Memory.ReadLong(fsEntry + 28));
      _core.Log->AddLogDebug("fhfile: FileSystemEntry Priority  : %.8X\n", VM->Memory.ReadLong(fsEntry + 32));
      _core.Log->AddLogDebug("fhfile: FileSystemEntry Startup   : %.8X\n", VM->Memory.ReadLong(fsEntry + 36));
      _core.Log->AddLogDebug("fhfile: FileSystemEntry SegList   : %.8X\n", VM->Memory.ReadLong(fsEntry + 40));
      _core.Log->AddLogDebug("fhfile: FileSystemEntry GlobalVec : %.8X\n\n", VM->Memory.ReadLong(fsEntry + 44));
      fsNode = VM->Memory.ReadLong(fsNode);
    }
  }

  void HardfileHandler::DoPatchDOSDeviceNode(uint32_t node, uint32_t packet)
  {
    _core.Log->AddLogDebug("fhfile: DoPatchDOSDeviceNode(node: %.8X, packet: %.8X)\n", node, packet);

    VM->Memory.WriteLong(0, node + 8); // dn_Task = 0
    VM->Memory.WriteLong(0, node + 16); // dn_Handler = 0
    VM->Memory.WriteLong(static_cast<uint32_t>(-1), node + 36); // dn_GlobalVec = -1

    HardfileFileSystemEntry* fs = GetFileSystemForDOSType(VM->Memory.ReadLong(packet + 80));
    if (fs != nullptr)
    {
      if (fs->Header->PatchFlags & 0x10)
      {
        VM->Memory.WriteLong(fs->Header->DnStackSize, node + 20);
      }
      if (fs->Header->PatchFlags & 0x80)
      {
        VM->Memory.WriteLong(fs->SegListAddress >> 2, node + 32);
      }
      if (fs->Header->PatchFlags & 0x100)
      {
        VM->Memory.WriteLong(fs->Header->DnGlobalVec, node + 36);
      }
    }
  }

  void HardfileHandler::DoUnknownOperation(uint32_t operation)
  {
    _core.Log->AddLogDebug("fhfile: Unknown operation called %X\n", operation);
  }

  /*=================================================*/
  /* fhfile_do                                       */
  /* The M68000 stubs entered in the device tables   */
  /* write a longword to $f40000, which is forwarded */
  /* by the memory system to this procedure.         */
  /* Hardfile commands are issued by 0x0001XXXX      */
  /* RDB filesystem commands by 0x0002XXXX           */
  /*=================================================*/

  void HardfileHandler::Do(uint32_t data)
  {
    uint32_t type = data >> 16;
    uint32_t operation = data & 0xffff;
    if (type == 1)
    {
      switch (operation)
      {
      case 1:
        DoDiag();
        break;
      case 2:
        DoOpen();
        break;
      case 3:
        DoClose();
        break;
      case 4:
        DoExpunge();
        break;
      case 5:
        DoNULL();
        break;
      case 6:
        DoBeginIO();
        break;
      case 7:
        DoAbortIO();
        break;
      default:
        break;
      }
    }
    else if (type == 2)
    {
      switch (operation)
      {
      case 1:
        VM->CPU.SetDReg(0, DoGetRDBFileSystemCount());
        break;
      case 2:
        VM->CPU.SetDReg(0, DoGetRDBFileSystemHunkCount(VM->CPU.GetDReg(1)));
        break;
      case 3:
        VM->CPU.SetDReg(0, DoGetRDBFileSystemHunkSize(VM->CPU.GetDReg(1), VM->CPU.GetDReg(2)));
        break;
      case 4:
        DoCopyRDBFileSystemHunk(VM->CPU.GetDReg(0),      // VM memory allocated for hunk
          VM->CPU.GetDReg(1), // Filesystem index on the device
          VM->CPU.GetDReg(2));// Hunk index in filesystem fileimage
        break;
      case 5:
        DoInitializeRDBFileSystemEntry(VM->CPU.GetDReg(0), VM->CPU.GetDReg(1));
        break;
      case 6:
        DoRemoveRDBFileSystemsAlreadySupportedBySystem(VM->CPU.GetDReg(0));
        break;
      case 7:
        DoPatchDOSDeviceNode(VM->CPU.GetDReg(0), VM->CPU.GetAReg(5));
        break;
      case 8:
        DoRelocateFileSystem(VM->CPU.GetDReg(1));
        break;
      case 9:
        VM->CPU.SetDReg(0, DoGetDosDevPacketListStart());
        break;
      case 0xa0:
        DoLogAllocMemResult(VM->CPU.GetDReg(0));
        break;
      case 0xa1:
        DoLogOpenResourceResult(VM->CPU.GetDReg(0));
        break;
      case 0xa2:
        DoLogAvailableResources();
        break;
      case 0xa3:
        DoLogAvailableFileSystems(VM->CPU.GetDReg(0));
        break;
      default:
        DoUnknownOperation(operation);
      }
    }
  }

  /*=================================================*/
  /* Make a dosdevice packet about the device layout */
  /*=================================================*/

  void HardfileHandler::MakeDOSDevPacketForPlainHardfile(const HardfileMountListEntry& mountListEntry, uint32_t deviceNameAddress)
  {
    const HardfileDevice& device = _devices[mountListEntry.DeviceIndex];
    if (device.F != nullptr)
    {
      uint32_t unit = GetUnitNumberFromIndex(mountListEntry.DeviceIndex);
      const HardfileGeometry& geometry = device.Configuration.Geometry;
      VM->Memory.DmemSetLong(mountListEntry.DeviceIndex);   /* Flag to initcode */

      VM->Memory.DmemSetLong(mountListEntry.NameAddress);   /*  0 Unit name "FELLOWX" or similar */
      VM->Memory.DmemSetLong(deviceNameAddress);            /*  4 Device name "fhfile.device" */
      VM->Memory.DmemSetLong(unit);                         /*  8 Unit # */
      VM->Memory.DmemSetLong(0);                            /* 12 OpenDevice flags */

      // Struct DosEnvec
      VM->Memory.DmemSetLong(16);                           /* 16 Environment size in long words */
      VM->Memory.DmemSetLong(geometry.BytesPerSector >> 2); /* 20 Longwords in a block */
      VM->Memory.DmemSetLong(0);                            /* 24 sector origin (unused) */
      VM->Memory.DmemSetLong(geometry.Surfaces);            /* 28 Heads */
      VM->Memory.DmemSetLong(1);                            /* 32 Sectors per logical block (unused) */
      VM->Memory.DmemSetLong(geometry.SectorsPerTrack);     /* 36 Sectors per track */
      VM->Memory.DmemSetLong(geometry.ReservedBlocks);      /* 40 Reserved blocks, min. 1 */
      VM->Memory.DmemSetLong(0);                            /* 44 mdn_prefac - Unused */
      VM->Memory.DmemSetLong(0);                            /* 48 Interleave */
      VM->Memory.DmemSetLong(0);                            /* 52 Lower cylinder */
      VM->Memory.DmemSetLong(geometry.HighCylinder);        /* 56 Upper cylinder */
      VM->Memory.DmemSetLong(0);                            /* 60 Number of buffers */
      VM->Memory.DmemSetLong(0);                            /* 64 Type of memory for buffers */
      VM->Memory.DmemSetLong(0x7fffffff);                   /* 68 Largest transfer */
      VM->Memory.DmemSetLong(~1U);                          /* 72 Add mask */
      VM->Memory.DmemSetLong(static_cast<uint32_t>(-1));         /* 76 Boot priority */
      VM->Memory.DmemSetLong(0x444f5300);                   /* 80 DOS file handler name */
      VM->Memory.DmemSetLong(0);
    }
  }

  void HardfileHandler::MakeDOSDevPacketForRDBPartition(const HardfileMountListEntry& mountListEntry, uint32_t deviceNameAddress)
  {
    HardfileDevice& device = _devices[mountListEntry.DeviceIndex];
    RDBPartition* partition = device.RDB->Partitions[mountListEntry.PartitionIndex].get();
    if (device.F != nullptr)
    {
      uint32_t unit = GetUnitNumberFromIndex(mountListEntry.DeviceIndex);

      VM->Memory.DmemSetLong(mountListEntry.DeviceIndex);            /* Flag to initcode */

      VM->Memory.DmemSetLong(mountListEntry.NameAddress);            /*  0 Unit name "FELLOWX" or similar */
      VM->Memory.DmemSetLong(deviceNameAddress);                     /*  4 Device name "fhfile.device" */
      VM->Memory.DmemSetLong(unit);                                  /*  8 Unit # */
      VM->Memory.DmemSetLong(0);                                     /* 12 OpenDevice flags */

      // Struct DosEnvec
      VM->Memory.DmemSetLong(16);                                    /* 16 Environment size in long words*/
      VM->Memory.DmemSetLong(partition->SizeBlock);                  /* 20 Longwords in a block */
      VM->Memory.DmemSetLong(partition->SecOrg);                     /* 24 sector origin (unused) */
      VM->Memory.DmemSetLong(partition->Surfaces);                   /* 28 Heads */
      VM->Memory.DmemSetLong(partition->SectorsPerBlock);            /* 32 Sectors per logical block (unused) */
      VM->Memory.DmemSetLong(partition->BlocksPerTrack);             /* 36 Sectors per track */
      VM->Memory.DmemSetLong(partition->Reserved);                   /* 40 Reserved blocks, min. 1 */
      VM->Memory.DmemSetLong(partition->PreAlloc);                   /* 44 mdn_prefac - Unused */
      VM->Memory.DmemSetLong(partition->Interleave);                 /* 48 Interleave */
      VM->Memory.DmemSetLong(partition->LowCylinder);                /* 52 Lower cylinder */
      VM->Memory.DmemSetLong(partition->HighCylinder);               /* 56 Upper cylinder */
      VM->Memory.DmemSetLong(partition->NumBuffer);                  /* 60 Number of buffers */
      VM->Memory.DmemSetLong(partition->BufMemType);                 /* 64 Type of memory for buffers */
      VM->Memory.DmemSetLong(partition->MaxTransfer);                /* 68 Largest transfer */
      VM->Memory.DmemSetLong(partition->Mask);                       /* 72 Add mask */
      VM->Memory.DmemSetLong(partition->BootPri);                    /* 76 Boot priority */
      VM->Memory.DmemSetLong(partition->DOSType);                    /* 80 DOS file handler name */
      VM->Memory.DmemSetLong(0);
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

    CreateMountList();  // (Re-)Builds mountlist with all partitions from each device
    AddFileSystemsFromRdb();
  }

  /*===========================================================*/
  /* fhfileHardReset                                           */
  /* This will set up the device structures and stubs          */
  /* Can be called at every reset, but really only needed once */
  /*===========================================================*/

  void HardfileHandler::HardReset()
  {
    if (!HasZeroDevices() && GetEnabled() && VM->Memory.GetKickImageVersion() >= 34)
    {
      VM->Memory.DmemSetCounter(0);

      /* Device-name and ID string */

      _devicename = VM->Memory.DmemGetCounter();
      VM->Memory.DmemSetString("fhfile.device");
      uint32_t idstr = VM->Memory.DmemGetCounter();
      VM->Memory.DmemSetString("Fellow Hardfile device V5");
      uint32_t doslibname = VM->Memory.DmemGetCounter();
      VM->Memory.DmemSetString("dos.library");
      _fsname = VM->Memory.DmemGetCounter();
      VM->Memory.DmemSetString("Fellow hardfile RDB fs");

      /* fhfile.open */

      uint32_t fhfile_t_open = VM->Memory.DmemGetCounter();
      VM->Memory.DmemSetWord(0x23fc);
      VM->Memory.DmemSetLong(0x00010002); VM->Memory.DmemSetLong(0xf40000); /* move.l #$00010002,$f40000 */
      VM->Memory.DmemSetWord(0x4e75);                                       /* rts */

      /* fhfile.close */

      uint32_t fhfile_t_close = VM->Memory.DmemGetCounter();
      VM->Memory.DmemSetWord(0x23fc);
      VM->Memory.DmemSetLong(0x00010003); VM->Memory.DmemSetLong(0xf40000); /* move.l #$00010003,$f40000 */
      VM->Memory.DmemSetWord(0x4e75);                                       /* rts */

      /* fhfile.expunge */

      uint32_t fhfile_t_expunge = VM->Memory.DmemGetCounter();
      VM->Memory.DmemSetWord(0x23fc);
      VM->Memory.DmemSetLong(0x00010004); VM->Memory.DmemSetLong(0xf40000); /* move.l #$00010004,$f40000 */
      VM->Memory.DmemSetWord(0x4e75);                                       /* rts */

      /* fhfile.null */

      uint32_t fhfile_t_null = VM->Memory.DmemGetCounter();
      VM->Memory.DmemSetWord(0x23fc);
      VM->Memory.DmemSetLong(0x00010005); VM->Memory.DmemSetLong(0xf40000); /* move.l #$00010005,$f40000 */
      VM->Memory.DmemSetWord(0x4e75);                                       /* rts */

      /* fhfile.beginio */

      uint32_t fhfile_t_beginio = VM->Memory.DmemGetCounter();
      VM->Memory.DmemSetWord(0x23fc);
      VM->Memory.DmemSetLong(0x00010006); VM->Memory.DmemSetLong(0xf40000);   /* move.l #$00010006,$f40000  BeginIO */
      VM->Memory.DmemSetLong(0x48e78002);                                     /* movem.l d0/a6,-(a7)    push        */
      VM->Memory.DmemSetLong(0x08290000); VM->Memory.DmemSetWord(0x001e);     /* btst   #$0,30(a1)      IOF_QUICK   */
      VM->Memory.DmemSetWord(0x6608);                                         /* bne    (to pop)                    */
      VM->Memory.DmemSetLong(0x2c780004);                                     /* move.l $4.w,a6         exec        */
      VM->Memory.DmemSetLong(0x4eaefe86);                                     /* jsr    -378(a6)        ReplyMsg(a1)*/
      VM->Memory.DmemSetLong(0x4cdf4001);                                     /* movem.l (a7)+,d0/a6    pop         */
      VM->Memory.DmemSetWord(0x4e75);                                         /* rts */

      /* fhfile.abortio */

      uint32_t fhfile_t_abortio = VM->Memory.DmemGetCounter();
      VM->Memory.DmemSetWord(0x23fc);
      VM->Memory.DmemSetLong(0x00010007); VM->Memory.DmemSetLong(0xf40000); /* move.l #$00010007,$f40000 */
      VM->Memory.DmemSetWord(0x4e75);                                       /* rts */

      /* Func-table */

      uint32_t functable = VM->Memory.DmemGetCounter();
      VM->Memory.DmemSetLong(fhfile_t_open);
      VM->Memory.DmemSetLong(fhfile_t_close);
      VM->Memory.DmemSetLong(fhfile_t_expunge);
      VM->Memory.DmemSetLong(fhfile_t_null);
      VM->Memory.DmemSetLong(fhfile_t_beginio);
      VM->Memory.DmemSetLong(fhfile_t_abortio);
      VM->Memory.DmemSetLong(0xffffffff);

      /* Data-table */

      uint32_t datatable = VM->Memory.DmemGetCounter();
      VM->Memory.DmemSetWord(0xE000);          /* INITBYTE */
      VM->Memory.DmemSetWord(0x0008);          /* LN_TYPE */
      VM->Memory.DmemSetWord(0x0300);          /* NT_DEVICE */
      VM->Memory.DmemSetWord(0xC000);          /* INITLONG */
      VM->Memory.DmemSetWord(0x000A);          /* LN_NAME */
      VM->Memory.DmemSetLong(_devicename);
      VM->Memory.DmemSetWord(0xE000);          /* INITBYTE */
      VM->Memory.DmemSetWord(0x000E);          /* LIB_FLAGS */
      VM->Memory.DmemSetWord(0x0600);          /* LIBF_SUMUSED+LIBF_CHANGED */
      VM->Memory.DmemSetWord(0xD000);          /* INITWORD */
      VM->Memory.DmemSetWord(0x0014);          /* LIB_VERSION */
      VM->Memory.DmemSetWord(0x0002);
      VM->Memory.DmemSetWord(0xD000);          /* INITWORD */
      VM->Memory.DmemSetWord(0x0016);          /* LIB_REVISION */
      VM->Memory.DmemSetWord(0x0000);
      VM->Memory.DmemSetWord(0xC000);          /* INITLONG */
      VM->Memory.DmemSetWord(0x0018);          /* LIB_IDSTRING */
      VM->Memory.DmemSetLong(idstr);
      VM->Memory.DmemSetLong(0);               /* END */

      /* bootcode */

      _bootcode = VM->Memory.DmemGetCounter();
      VM->Memory.DmemSetWord(0x227c); VM->Memory.DmemSetLong(doslibname); /* move.l #doslibname,a1 */
      VM->Memory.DmemSetLong(0x4eaeffa0);                                   /* jsr    -96(a6) ; FindResident() */
      VM->Memory.DmemSetWord(0x2040);                                       /* move.l d0,a0 */
      VM->Memory.DmemSetLong(0x20280016);                                   /* move.l 22(a0),d0 */
      VM->Memory.DmemSetWord(0x2040);                                       /* move.l d0,a0 */
      VM->Memory.DmemSetWord(0x4e90);                                       /* jsr    (a0) */
      VM->Memory.DmemSetWord(0x4e75);                                       /* rts */

      /* fhfile.init */

      uint32_t fhfile_t_init = VM->Memory.DmemGetCounter();

      VM->Memory.DmemSetByte(0x48); VM->Memory.DmemSetByte(0xE7); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0xFE);
      VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF6);
      VM->Memory.DmemSetByte(0x4A); VM->Memory.DmemSetByte(0x80); VM->Memory.DmemSetByte(0x67); VM->Memory.DmemSetByte(0x24);
      VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xD6);
      VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x01); VM->Memory.DmemSetByte(0xB0);
      VM->Memory.DmemSetByte(0x4A); VM->Memory.DmemSetByte(0x80); VM->Memory.DmemSetByte(0x66); VM->Memory.DmemSetByte(0x0C);
      VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x01); VM->Memory.DmemSetByte(0x6A);
      VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x01); VM->Memory.DmemSetByte(0xA4);
      VM->Memory.DmemSetByte(0x4A); VM->Memory.DmemSetByte(0x80); VM->Memory.DmemSetByte(0x67); VM->Memory.DmemSetByte(0x0C);
      VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x01); VM->Memory.DmemSetByte(0x12);
      VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02); VM->Memory.DmemSetByte(0x0A);
      VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xC2);
      VM->Memory.DmemSetByte(0x2C); VM->Memory.DmemSetByte(0x78); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x04);
      VM->Memory.DmemSetByte(0x43); VM->Memory.DmemSetByte(0xFA); VM->Memory.DmemSetByte(0x02); VM->Memory.DmemSetByte(0x20);
      VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0xAE); VM->Memory.DmemSetByte(0xFE); VM->Memory.DmemSetByte(0x68);
      VM->Memory.DmemSetByte(0x28); VM->Memory.DmemSetByte(0x40); VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0x01); VM->Memory.DmemSetByte(0x1C); VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x40);
      VM->Memory.DmemSetByte(0x2E); VM->Memory.DmemSetByte(0x08); VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x47);
      VM->Memory.DmemSetByte(0x4A); VM->Memory.DmemSetByte(0x90); VM->Memory.DmemSetByte(0x6B); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x70); VM->Memory.DmemSetByte(0x58); VM->Memory.DmemSetByte(0x87);
      VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x3C); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x58); VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0x01); VM->Memory.DmemSetByte(0x10); VM->Memory.DmemSetByte(0x2A); VM->Memory.DmemSetByte(0x40);
      VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x47); VM->Memory.DmemSetByte(0x70); VM->Memory.DmemSetByte(0x54);
      VM->Memory.DmemSetByte(0x2B); VM->Memory.DmemSetByte(0xB0); VM->Memory.DmemSetByte(0x08); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0x08); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x59); VM->Memory.DmemSetByte(0x80);
      VM->Memory.DmemSetByte(0x64); VM->Memory.DmemSetByte(0xF6); VM->Memory.DmemSetByte(0xCD); VM->Memory.DmemSetByte(0x4C);
      VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x4D); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0xAE);
      VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x70); VM->Memory.DmemSetByte(0xCD); VM->Memory.DmemSetByte(0x4C);
      VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xCE);
      VM->Memory.DmemSetByte(0x26); VM->Memory.DmemSetByte(0x40); VM->Memory.DmemSetByte(0x70); VM->Memory.DmemSetByte(0x14);
      VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xEA);
      VM->Memory.DmemSetByte(0x22); VM->Memory.DmemSetByte(0x47); VM->Memory.DmemSetByte(0x2C); VM->Memory.DmemSetByte(0x29);
      VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0xFC); VM->Memory.DmemSetByte(0x22); VM->Memory.DmemSetByte(0x40);
      VM->Memory.DmemSetByte(0x70); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x22); VM->Memory.DmemSetByte(0x80);
      VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0x40); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x04);
      VM->Memory.DmemSetByte(0x33); VM->Memory.DmemSetByte(0x40); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x0E);
      VM->Memory.DmemSetByte(0x33); VM->Memory.DmemSetByte(0x7C); VM->Memory.DmemSetByte(0x10); VM->Memory.DmemSetByte(0xFF);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x08); VM->Memory.DmemSetByte(0x9D); VM->Memory.DmemSetByte(0x69);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x08); VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0x79);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4); VM->Memory.DmemSetByte(0x0F); VM->Memory.DmemSetByte(0xFC);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x0A); VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0x4B);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x10); VM->Memory.DmemSetByte(0x41); VM->Memory.DmemSetByte(0xEC);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x4A); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0xAE);
      VM->Memory.DmemSetByte(0xFE); VM->Memory.DmemSetByte(0xF2); VM->Memory.DmemSetByte(0x06); VM->Memory.DmemSetByte(0x87);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x58);
      VM->Memory.DmemSetByte(0x60); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x8C);
      VM->Memory.DmemSetByte(0x2C); VM->Memory.DmemSetByte(0x78); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x04);
      VM->Memory.DmemSetByte(0x22); VM->Memory.DmemSetByte(0x4C); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0xAE);
      VM->Memory.DmemSetByte(0xFE); VM->Memory.DmemSetByte(0x62); VM->Memory.DmemSetByte(0x4C); VM->Memory.DmemSetByte(0xDF);
      VM->Memory.DmemSetByte(0x7F); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75);
      VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xA0); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75);
      VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xA1); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75);
      VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xA2); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75);
      VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xA3); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75);
      VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x01); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75);
      VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75);
      VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x03); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75);
      VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x04); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75);
      VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x05); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75);
      VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x06); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75);
      VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x07); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75);
      VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x08); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75);
      VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x09); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75);
      VM->Memory.DmemSetByte(0x48); VM->Memory.DmemSetByte(0xE7); VM->Memory.DmemSetByte(0x78); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0x22); VM->Memory.DmemSetByte(0x3C); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x01);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x01); VM->Memory.DmemSetByte(0x2C); VM->Memory.DmemSetByte(0x78);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x04); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0xAE);
      VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x3A); VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x50); VM->Memory.DmemSetByte(0x4C); VM->Memory.DmemSetByte(0xDF);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x1E); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75);
      VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x3C); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0xDC); VM->Memory.DmemSetByte(0x2A); VM->Memory.DmemSetByte(0x40);
      VM->Memory.DmemSetByte(0x1B); VM->Memory.DmemSetByte(0x7C); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x08);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x08); VM->Memory.DmemSetByte(0x41); VM->Memory.DmemSetByte(0xFA);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xD0); VM->Memory.DmemSetByte(0x2B); VM->Memory.DmemSetByte(0x48);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x0A); VM->Memory.DmemSetByte(0x41); VM->Memory.DmemSetByte(0xFA);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xDC); VM->Memory.DmemSetByte(0x2B); VM->Memory.DmemSetByte(0x48);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x0E); VM->Memory.DmemSetByte(0x49); VM->Memory.DmemSetByte(0xED);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x12); VM->Memory.DmemSetByte(0x28); VM->Memory.DmemSetByte(0x8C);
      VM->Memory.DmemSetByte(0x06); VM->Memory.DmemSetByte(0x94); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x04); VM->Memory.DmemSetByte(0x42); VM->Memory.DmemSetByte(0xAC);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x04); VM->Memory.DmemSetByte(0x29); VM->Memory.DmemSetByte(0x4C);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x08); VM->Memory.DmemSetByte(0x22); VM->Memory.DmemSetByte(0x4D);
      VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0xAE); VM->Memory.DmemSetByte(0xFE); VM->Memory.DmemSetByte(0x1A);
      VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75); VM->Memory.DmemSetByte(0x2C); VM->Memory.DmemSetByte(0x78);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x04); VM->Memory.DmemSetByte(0x70); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0x43); VM->Memory.DmemSetByte(0xFA); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x9E);
      VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0xAE); VM->Memory.DmemSetByte(0xFE); VM->Memory.DmemSetByte(0x0E);
      VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x06);
      VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75); VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x30); VM->Memory.DmemSetByte(0x4A); VM->Memory.DmemSetByte(0x80);
      VM->Memory.DmemSetByte(0x67); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x26);
      VM->Memory.DmemSetByte(0x28); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x74); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x2E);
      VM->Memory.DmemSetByte(0x4A); VM->Memory.DmemSetByte(0x80); VM->Memory.DmemSetByte(0x67); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x0C); VM->Memory.DmemSetByte(0x50); VM->Memory.DmemSetByte(0x80);
      VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x76);
      VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x2A);
      VM->Memory.DmemSetByte(0x52); VM->Memory.DmemSetByte(0x82); VM->Memory.DmemSetByte(0xB8); VM->Memory.DmemSetByte(0x82);
      VM->Memory.DmemSetByte(0x66); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0xE6);
      VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x4E);
      VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75); VM->Memory.DmemSetByte(0x2F); VM->Memory.DmemSetByte(0x08);
      VM->Memory.DmemSetByte(0x2F); VM->Memory.DmemSetByte(0x01); VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0xCA); VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x3C);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xBE);
      VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x52);
      VM->Memory.DmemSetByte(0x22); VM->Memory.DmemSetByte(0x1F); VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x10); VM->Memory.DmemSetByte(0x2C); VM->Memory.DmemSetByte(0x79);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x04);
      VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x57); VM->Memory.DmemSetByte(0x41); VM->Memory.DmemSetByte(0xE8);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x12); VM->Memory.DmemSetByte(0x22); VM->Memory.DmemSetByte(0x40);
      VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0xAE); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x10);
      VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x5F); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75);
      VM->Memory.DmemSetByte(0x2F); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x40);
      VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xFE); VM->Memory.DmemSetByte(0xC2);
      VM->Memory.DmemSetByte(0x4A); VM->Memory.DmemSetByte(0x80); VM->Memory.DmemSetByte(0x67); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x12); VM->Memory.DmemSetByte(0x26); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0x72); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0xBE); VM->Memory.DmemSetByte(0x52); VM->Memory.DmemSetByte(0x81);
      VM->Memory.DmemSetByte(0xB6); VM->Memory.DmemSetByte(0x81); VM->Memory.DmemSetByte(0x66); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0xF6); VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x1F);
      VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75); VM->Memory.DmemSetByte(0x65); VM->Memory.DmemSetByte(0x78);
      VM->Memory.DmemSetByte(0x70); VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x6E); VM->Memory.DmemSetByte(0x73);
      VM->Memory.DmemSetByte(0x69); VM->Memory.DmemSetByte(0x6F); VM->Memory.DmemSetByte(0x6E); VM->Memory.DmemSetByte(0x2E);
      VM->Memory.DmemSetByte(0x6C); VM->Memory.DmemSetByte(0x69); VM->Memory.DmemSetByte(0x62); VM->Memory.DmemSetByte(0x72);
      VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x72); VM->Memory.DmemSetByte(0x79); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0x46); VM->Memory.DmemSetByte(0x69); VM->Memory.DmemSetByte(0x6C); VM->Memory.DmemSetByte(0x65);
      VM->Memory.DmemSetByte(0x53); VM->Memory.DmemSetByte(0x79); VM->Memory.DmemSetByte(0x73); VM->Memory.DmemSetByte(0x74);
      VM->Memory.DmemSetByte(0x65); VM->Memory.DmemSetByte(0x6D); VM->Memory.DmemSetByte(0x2E); VM->Memory.DmemSetByte(0x72);
      VM->Memory.DmemSetByte(0x65); VM->Memory.DmemSetByte(0x73); VM->Memory.DmemSetByte(0x6F); VM->Memory.DmemSetByte(0x75);
      VM->Memory.DmemSetByte(0x72); VM->Memory.DmemSetByte(0x63); VM->Memory.DmemSetByte(0x65); VM->Memory.DmemSetByte(0x00);
      VM->Memory.DmemSetByte(0x46); VM->Memory.DmemSetByte(0x65); VM->Memory.DmemSetByte(0x6C); VM->Memory.DmemSetByte(0x6C);
      VM->Memory.DmemSetByte(0x6F); VM->Memory.DmemSetByte(0x77); VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x68);
      VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x72); VM->Memory.DmemSetByte(0x64); VM->Memory.DmemSetByte(0x66);
      VM->Memory.DmemSetByte(0x69); VM->Memory.DmemSetByte(0x6C); VM->Memory.DmemSetByte(0x65); VM->Memory.DmemSetByte(0x20);
      VM->Memory.DmemSetByte(0x64); VM->Memory.DmemSetByte(0x65); VM->Memory.DmemSetByte(0x76); VM->Memory.DmemSetByte(0x69);
      VM->Memory.DmemSetByte(0x63); VM->Memory.DmemSetByte(0x65); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00);

      /* Init-struct */

      uint32_t initstruct = VM->Memory.DmemGetCounter();
      VM->Memory.DmemSetLong(0x100);                   /* Data-space size, min LIB_SIZE */
      VM->Memory.DmemSetLong(functable);               /* Function-table */
      VM->Memory.DmemSetLong(datatable);               /* Data-table */
      VM->Memory.DmemSetLong(fhfile_t_init);           /* Init-routine */

      /* RomTag structure */

      uint32_t romtagstart = VM->Memory.DmemGetCounter();
      VM->Memory.DmemSetWord(0x4afc);                  /* Start of structure */
      VM->Memory.DmemSetLong(romtagstart);             /* Pointer to start of structure */
      VM->Memory.DmemSetLong(romtagstart + 26);        /* Pointer to end of code */
      VM->Memory.DmemSetByte(0x81);                    /* Flags, AUTOINIT+COLDSTART */
      VM->Memory.DmemSetByte(0x1);                     /* Version */
      VM->Memory.DmemSetByte(3);                       /* DEVICE */
      VM->Memory.DmemSetByte(0);                       /* Priority */
      VM->Memory.DmemSetLong(_devicename);             /* Pointer to name (used in opendev)*/
      VM->Memory.DmemSetLong(idstr);                   /* ID string */
      VM->Memory.DmemSetLong(initstruct);              /* Init_struct */

      _endOfDmem = VM->Memory.DmemGetCounterWithoutOffset();

      /* Clear hardfile rom */

      memset(_rom, 0, 65536);

      /* Struct DiagArea */

      _rom[0x1000] = 0x90; /* da_Config */
      _rom[0x1001] = 0;    /* da_Flags */
      _rom[0x1002] = 0;    /* da_Size */
      _rom[0x1003] = 0x96;
      _rom[0x1004] = 0;    /* da_DiagPoint */
      _rom[0x1005] = 0x80;
      _rom[0x1006] = 0;    /* da_BootPoint */
      _rom[0x1007] = 0x90;
      _rom[0x1008] = 0;    /* da_Name */
      _rom[0x1009] = 0;
      _rom[0x100a] = 0;    /* da_Reserved01 */
      _rom[0x100b] = 0;
      _rom[0x100c] = 0;    /* da_Reserved02 */
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
      _rom[0x1092] = static_cast<uint8_t>(_bootcode >> 24);
      _rom[0x1093] = static_cast<uint8_t>(_bootcode >> 16);
      _rom[0x1094] = static_cast<uint8_t>(_bootcode >> 8);
      _rom[0x1095] = static_cast<uint8_t>(_bootcode);

      /* NULLIFY pointer to configdev */

      VM->Memory.DmemSetLongNoCounter(0, 4092);
      VM->Memory.EmemCardAdd(HardfileHandler_CardInit, HardfileHandler_CardMap);
    }
    else
    {
      VM->Memory.DmemClear();
    }
  }

  void HardfileHandler::CreateDOSDevPackets(uint32_t devicename)
  {
    VM->Memory.DmemSetCounter(_endOfDmem);

    /* Device name as seen in Amiga DOS */

    for (auto& mountListEntry : _mountList)
    {
      mountListEntry->NameAddress = VM->Memory.DmemGetCounter();
      VM->Memory.DmemSetString(mountListEntry->Name.c_str());
    }

    _dosDevPacketListStart = VM->Memory.DmemGetCounter();

    /* The mkdosdev packets */

    for (auto& mountListEntry : _mountList)
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
    VM->Memory.DmemSetLong(static_cast<uint32_t>(-1));  // Terminate list
  }

  void HardfileHandler::EmulationStart()
  {
    for (int i = 0; i < FHFILE_MAX_DEVICES; i++)
    {
      if (_devices[i].Status == FHFILE_HDF && _devices[i].F == nullptr)
      {
        OpenHardfileFile(_devices[i]);
      }
    }
  }

  void HardfileHandler::EmulationStop()
  {
    for (int i = 0; i < FHFILE_MAX_DEVICES; i++)
    {
      if (_devices[i].Status == FHFILE_HDF && _devices[i].F != nullptr)
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

  bool HardfileHandler::Create(const fellow::api::module::HardfileConfiguration& configuration, uint32_t size)
  {
    bool result = false;

#ifdef WIN32
    HANDLE hf;

    if (!configuration.Filename.empty() && size != 0)
    {
      if ((hf = CreateFile(configuration.Filename.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr)) != INVALID_HANDLE_VALUE)
      {
        LONG high = 0;
        if (SetFilePointer(hf, size, &high, FILE_BEGIN) == size)
          result = SetEndOfFile(hf) == TRUE;
        else
          _core.Log->AddLog("SetFilePointer() failure.\n");
        CloseHandle(hf);
      }
      else
        _core.Log->AddLog("CreateFile() failed.\n");
    }
    return result;
#else	/* os independent implementation */
#define BUFSIZE 32768
    unsigned int tobewritten;
    char buffer[BUFSIZE];
    FILE* hf;

    tobewritten = size;
    errno = 0;

    if (!configuration.Filename.empty() && size != 0)
    {
      fopen_s(&hf, configuration.Filename.c_str(), "wb");
      if (hf != nullptr)
      {
        memset(buffer, 0, sizeof(buffer));
        while (tobewritten >= BUFSIZE)
        {
          fwrite(buffer, sizeof(char), BUFSIZE, hf);
          if (errno != 0)
          {
            _core.Log->AddLog("Creating hardfile failed. Check the available space.\n");
            fclose(hf);
            return result;
          }
          tobewritten -= BUFSIZE;
        }
        fwrite(buffer, sizeof(char), tobewritten, hf);
        if (errno != 0)
        {
          _core.Log->AddLog("Creating hardfile failed. Check the available space.\n");
          fclose(hf);
          return result;
        }
        fclose(hf);
        result = true;
      }
      else
        _core.Log->AddLog("fhfileCreate is unable to open output file.\n");
    }
    return result;
#endif
  }

  rdb_status HardfileHandler::HasRDB(const std::string& filename)
  {
    rdb_status result = rdb_status::RDB_NOT_FOUND;
    FILE* F = nullptr;
    fopen_s(&F, filename.c_str(), "rb");
    if (F != nullptr)
    {
      RDBFileReader reader(F);
      result = RDBHandler::HasRigidDiskBlock(reader);
      fclose(F);
    }
    return result;
  }

  HardfileConfiguration HardfileHandler::GetConfigurationFromRDBGeometry(const std::string& filename)
  {
    HardfileConfiguration configuration;
    FILE* F = nullptr;
    fopen_s(&F, filename.c_str(), "rb");
    if (F != nullptr)
    {
      RDBFileReader reader(F);
      RDB* rdb = RDBHandler::GetDriveInformation(reader, true);
      fclose(F);

      if (rdb != nullptr)
      {
        SetHardfileConfigurationFromRDB(configuration, rdb, false);
        delete rdb;
      }
    }
    return configuration;
  }

  HardfileHandler::HardfileHandler()
  {
  }

  HardfileHandler::~HardfileHandler() = default;
}
