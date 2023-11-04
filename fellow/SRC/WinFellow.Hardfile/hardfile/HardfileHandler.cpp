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

#include "Defs.h"
#include "hardfile/HardfileHandler.h"
#include "hardfile/rdb/RDBHandler.h"
#include "hardfile/hunks/HunkRelocator.h"
#include "VirtualHost/Core.h"

using namespace Service;
using namespace Debug;
using namespace fellow::hardfile::rdb;
using namespace Module::Hardfile;
using namespace std;

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
    _core.HardfileHandler->CardInit();
  }

  void HardfileHandler_CardMap(uint32_t mapping)
  {
    _core.HardfileHandler->CardMap(mapping);
  }

  uint8_t HardfileHandler_ReadByte(uint32_t address)
  {
    return _core.HardfileHandler->ReadByte(address);
  }

  uint16_t HardfileHandler_ReadWord(uint32_t address)
  {
    return _core.HardfileHandler->ReadWord(address);
  }

  uint32_t HardfileHandler_ReadLong(uint32_t address)
  {
    return _core.HardfileHandler->ReadLong(address);
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
    _memory.EmemSet(0, 0xd1);
    _memory.EmemSet(8, 0xc0);
    _memory.EmemSet(4, 2);
    _memory.EmemSet(0x10, 2011 >> 8);
    _memory.EmemSet(0x14, 2011 & 0xf);
    _memory.EmemSet(0x18, 0);
    _memory.EmemSet(0x1c, 0);
    _memory.EmemSet(0x20, 0);
    _memory.EmemSet(0x24, 1);
    _memory.EmemSet(0x28, 0x10);
    _memory.EmemSet(0x2c, 0);
    _memory.EmemSet(0x40, 0);
    _memory.EmemMirror(0x1000, _rom + 0x1000, 0xa0);
  }

  /*====================================================*/
  /* fhfile_card_map                                    */
  /* The rom must be remapped to the location specified.*/
  /*====================================================*/

  void HardfileHandler::CardMap(uint32_t mapping)
  {
    _romstart = (mapping << 8) & 0xff0000;
    uint32_t bank = _romstart >> 16;
    _memory.BankSet(
        HardfileHandler_ReadByte,
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
    uint8_t *p = _rom + (address & 0xffff);
    return (static_cast<uint16_t>(p[0]) << 8) | static_cast<uint16_t>(p[1]);
  }

  uint32_t HardfileHandler::ReadLong(uint32_t address)
  {
    uint8_t *p = _rom + (address & 0xffff);
    return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) | (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
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

  bool HardfileHandler::FindOlderOrSameFileSystemVersion(uint32_t DOSType, uint32_t version, unsigned int &olderOrSameFileSystemIndex)
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

  HardfileFileSystemEntry *HardfileHandler::GetFileSystemForDOSType(uint32_t DOSType)
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
        _fileSystems.push_back(make_unique<HardfileFileSystemEntry>(_memory, header.get(), 0));
      }
      else if (_fileSystems[olderOrSameFileSystemIndex]->IsOlderVersion(header->Version))
      {
        // Replace older fs version with this one
        _fileSystems[olderOrSameFileSystemIndex].reset(new HardfileFileSystemEntry(_memory, header.get(), 0));
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

  void HardfileHandler::EraseOlderOrSameFileSystemVersion(uint32_t DOSType, uint32_t version)
  {
    unsigned int olderOrSameFileSystemIndex = 0;
    bool hasOlderOrSameFileSystemVersion = FindOlderOrSameFileSystemVersion(DOSType, version, olderOrSameFileSystemIndex);
    if (hasOlderOrSameFileSystemVersion)
    {
      _log.AddLogDebug(
          "fhfile: Erased RDB filesystem entry (%.8X, %.8X), newer version (%.8X, %.8X) found in RDB or newer/same version supported by Kickstart.\n",
          _fileSystems[olderOrSameFileSystemIndex]->GetDOSType(),
          _fileSystems[olderOrSameFileSystemIndex]->GetVersion(),
          DOSType,
          version);

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

    FileProperties *fileProperties = _core.FileInformation->GetFileProperties(device.Configuration.Filename.c_str());
    if (fileProperties == nullptr)
    {
      _log.AddLog("ERROR: Unable to access hardfile '%s', it is either inaccessible, or too big (2GB or more).\n", device.Configuration.Filename.c_str());
      return false;
    }

    device.Readonly = device.Configuration.Readonly || (!fileProperties->IsWritable);
    fopen_s(&device.F, device.Configuration.Filename.c_str(), device.Readonly ? "rb" : "r+b");
    device.FileSize = (unsigned int)fileProperties->Size;
    delete fileProperties;

    const auto &geometry = device.Configuration.Geometry;
    uint32_t cylinderSize = geometry.Surfaces * geometry.SectorsPerTrack * geometry.BytesPerSector;
    if (device.FileSize < cylinderSize)
    {
      fclose(device.F);
      device.F = nullptr;
      _log.AddLog("ERROR: Hardfile '%s' was not mounted, size is less than one cylinder.\n", device.Configuration.Filename.c_str());
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
    HardfileDevice &device = _devices[index];

    ClearDeviceRuntimeInfo(device);

    if (OpenHardfileFile(device))
    {
      RDBFileReader reader(device.F);
      rdb_status rdbResult = RDBHandler::HasRigidDiskBlock(reader);

      if (rdbResult == rdb_status::RDB_FOUND_WITH_HEADER_CHECKSUM_ERROR)
      {
        ClearDeviceRuntimeInfo(device);

        _log.AddLog("Hardfile: File skipped '%s', RDB header has checksum error.\n", device.Configuration.Filename.c_str());
        return;
      }

      if (rdbResult == rdb_status::RDB_FOUND_WITH_PARTITION_ERROR)
      {
        ClearDeviceRuntimeInfo(device);

        _log.AddLog("Hardfile: File skipped '%s', RDB partition has checksum error.\n", device.Configuration.Filename.c_str());
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

          _log.AddLog("Hardfile: File skipped '%s', RDB filesystem handler has checksum error.\n", device.Configuration.Filename.c_str());
          return;
        }

        device.RDB = rdb;
        SetHardfileConfigurationFromRDB(device.Configuration, device.RDB, device.Readonly);
      }

      HardfileGeometry &geometry = device.Configuration.Geometry;
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

        _log.AddLog("Hardfile: File skipped, geometry for %s is larger than the file.\n", device.Configuration.Filename.c_str());
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
      _log.AddLogDebug("ERROR: Unit number is not in a valid format.\n");
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

    _log.AddLog("SetHardfile('%s', %u)\n", configuration.Filename.c_str(), index);

    _core.RP->SendHardDriveContent(index, configuration.Filename.c_str(), configuration.Readonly);
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

  void HardfileHandler::SetIOError(int8_t errorCode)
  {
    _memory.WriteByte(errorCode, _cpu.GetAReg(1) + 31);
  }

  void HardfileHandler::SetIOActual(uint32_t ioActual)
  {
    _memory.WriteLong(ioActual, _cpu.GetAReg(1) + 32);
  }

  uint32_t HardfileHandler::GetUnitNumber()
  {
    return _memory.ReadLong(_cpu.GetAReg(1) + 24);
  }

  uint16_t HardfileHandler::GetCommand()
  {
    return _memory.ReadWord(_cpu.GetAReg(1) + 28);
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
      _log.AddLogDebug("CMD_READ Unit %d (%d) ERROR-TDERR_BadUnitNum\n", GetUnitNumberFromIndex(index), index);
      return 32; // TDERR_BadUnitNum
    }

    uint32_t dest = _memory.ReadLong(_cpu.GetAReg(1) + 40);
    uint32_t offset = _memory.ReadLong(_cpu.GetAReg(1) + 44);
    uint32_t length = _memory.ReadLong(_cpu.GetAReg(1) + 36);

    _log.AddLogDebug("CMD_READ Unit %d (%d) Destination %.8X Offset %.8X Length %.8X\n", GetUnitNumberFromIndex(index), index, dest, offset, length);

    if ((offset + length) > _devices[index].GeometrySize)
    {
      return -3; // TODO: Out of range, -3 is not the right code
    }

    _core.Hud->SetHarddiskLED(index, true, false);

    fseek(_devices[index].F, offset, SEEK_SET);
    fread(_memory.AddressToPtr(dest), 1, length, _devices[index].F);
    SetIOActual(length);

    _core.Hud->SetHarddiskLED(index, false, false);

    return 0;
  }

  int8_t HardfileHandler::Write(uint32_t index)
  {
    if (_devices[index].F == nullptr)
    {
      _log.AddLogDebug("CMD_WRITE Unit %d (%d) ERROR-TDERR_BadUnitNum\n", GetUnitNumberFromIndex(index), index);
      return 32; // TDERR_BadUnitNum
    }

    uint32_t dest = _memory.ReadLong(_cpu.GetAReg(1) + 40);
    uint32_t offset = _memory.ReadLong(_cpu.GetAReg(1) + 44);
    uint32_t length = _memory.ReadLong(_cpu.GetAReg(1) + 36);

    _log.AddLogDebug("CMD_WRITE Unit %d (%d) Destination %.8X Offset %.8X Length %.8X\n", GetUnitNumberFromIndex(index), index, dest, offset, length);

    if (_devices[index].Readonly || (offset + length) > _devices[index].GeometrySize)
    {
      return -3; // TODO: Out of range, -3 is probably not the right one.
    }

    _core.Hud->SetHarddiskLED(index, true, true);

    fseek(_devices[index].F, offset, SEEK_SET);
    fwrite(_memory.AddressToPtr(dest), 1, length, _devices[index].F);
    SetIOActual(length);

    _core.Hud->SetHarddiskLED(index, false, true);

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
    uint32_t scsiCmdStruct = _memory.ReadLong(_cpu.GetAReg(1) + 40); // io_Data

    _log.AddLogDebug("HD_SCSICMD Unit %d (%d) ScsiCmd at %.8X\n", GetUnitNumberFromIndex(index), index, scsiCmdStruct);

    uint32_t scsiCommand = _memory.ReadLong(scsiCmdStruct + 12);
    uint16_t scsiCommandLength = _memory.ReadWord(scsiCmdStruct + 16);

    _log.AddLogDebug("HD_SCSICMD Command length %d, data", scsiCommandLength);

    for (int i = 0; i < scsiCommandLength; i++)
    {
      _log.AddLogDebug(" %.2X", _memory.ReadByte(scsiCommand + i));
    }
    _log.AddLogDebug("\n");

    uint8_t commandNumber = _memory.ReadByte(scsiCommand);
    uint32_t returnData = _memory.ReadLong(scsiCmdStruct);
    switch (commandNumber)
    {
      case 0x25: // Read capacity (10)
        _log.AddLogDebug("SCSI direct command 0x25 Read Capacity\n");
        {
          uint32_t bytesPerSector = _devices[index].Configuration.Geometry.BytesPerSector;
          bool pmi = !!(_memory.ReadByte(scsiCommand + 8) & 1);

          if (pmi)
          {
            uint32_t blocksPerCylinder = (_devices[index].Configuration.Geometry.SectorsPerTrack * _devices[index].Configuration.Geometry.Surfaces) - 1;
            _memory.WriteByte(blocksPerCylinder >> 24, returnData);
            _memory.WriteByte(blocksPerCylinder >> 16, returnData + 1);
            _memory.WriteByte(blocksPerCylinder >> 8, returnData + 2);
            _memory.WriteByte(blocksPerCylinder, returnData + 3);
          }
          else
          {
            uint32_t blocksOnDevice = (_devices[index].GeometrySize / _devices[index].Configuration.Geometry.BytesPerSector) - 1;
            _memory.WriteByte(blocksOnDevice >> 24, returnData);
            _memory.WriteByte(blocksOnDevice >> 16, returnData + 1);
            _memory.WriteByte(blocksOnDevice >> 8, returnData + 2);
            _memory.WriteByte(blocksOnDevice, returnData + 3);
          }
          _memory.WriteByte(bytesPerSector >> 24, returnData + 4);
          _memory.WriteByte(bytesPerSector >> 16, returnData + 5);
          _memory.WriteByte(bytesPerSector >> 8, returnData + 6);
          _memory.WriteByte(bytesPerSector, returnData + 7);

          _memory.WriteByte(0, scsiCmdStruct + 8); // data actual length (long word)
          _memory.WriteByte(0, scsiCmdStruct + 9);
          _memory.WriteByte(0, scsiCmdStruct + 10);
          _memory.WriteByte(8, scsiCmdStruct + 11);

          _memory.WriteByte(0, scsiCmdStruct + 21); // Status
        }
        break;
      case 0x37: // Read defect Data (10)
        _log.AddLogDebug("SCSI direct command 0x37 Read defect Data\n");

        _memory.WriteByte(0, returnData);
        _memory.WriteByte(_memory.ReadByte(scsiCommand + 2), returnData + 1);
        _memory.WriteByte(0, returnData + 2); // No defects (word)
        _memory.WriteByte(0, returnData + 3);

        _memory.WriteByte(0, scsiCmdStruct + 8); // data actual length (long word)
        _memory.WriteByte(0, scsiCmdStruct + 9);
        _memory.WriteByte(0, scsiCmdStruct + 10);
        _memory.WriteByte(4, scsiCmdStruct + 11);

        _memory.WriteByte(0, scsiCmdStruct + 21); // Status
        break;
      case 0x12: // Inquiry
        _log.AddLogDebug("SCSI direct command 0x12 Inquiry\n");

        _memory.WriteByte(0, returnData);     // Pheripheral type 0 connected (magnetic disk)
        _memory.WriteByte(0, returnData + 1); // Not removable
        _memory.WriteByte(0, returnData + 2); // Does not claim conformance to any standard
        _memory.WriteByte(2, returnData + 3);
        _memory.WriteByte(32, returnData + 4); // Additional length
        _memory.WriteByte(0, returnData + 5);
        _memory.WriteByte(0, returnData + 6);
        _memory.WriteByte(0, returnData + 7);
        _memory.WriteByte('F', returnData + 8);
        _memory.WriteByte('E', returnData + 9);
        _memory.WriteByte('L', returnData + 10);
        _memory.WriteByte('L', returnData + 11);
        _memory.WriteByte('O', returnData + 12);
        _memory.WriteByte('W', returnData + 13);
        _memory.WriteByte(' ', returnData + 14);
        _memory.WriteByte(' ', returnData + 15);
        _memory.WriteByte('H', returnData + 16);
        _memory.WriteByte('a', returnData + 17);
        _memory.WriteByte('r', returnData + 18);
        _memory.WriteByte('d', returnData + 19);
        _memory.WriteByte('f', returnData + 20);
        _memory.WriteByte('i', returnData + 21);
        _memory.WriteByte('l', returnData + 22);
        _memory.WriteByte('e', returnData + 23);
        _memory.WriteByte(' ', returnData + 24);
        _memory.WriteByte(' ', returnData + 25);
        _memory.WriteByte(' ', returnData + 26);
        _memory.WriteByte(' ', returnData + 27);
        _memory.WriteByte(' ', returnData + 28);
        _memory.WriteByte(' ', returnData + 29);
        _memory.WriteByte(' ', returnData + 30);
        _memory.WriteByte(' ', returnData + 31);
        _memory.WriteByte('1', returnData + 32);
        _memory.WriteByte('.', returnData + 33);
        _memory.WriteByte('0', returnData + 34);
        _memory.WriteByte(' ', returnData + 35);

        _memory.WriteByte(0, scsiCmdStruct + 8); // data actual length (long word)
        _memory.WriteByte(0, scsiCmdStruct + 9);
        _memory.WriteByte(0, scsiCmdStruct + 10);
        _memory.WriteByte(36, scsiCmdStruct + 11);

        _memory.WriteByte(0, scsiCmdStruct + 21); // Status
        break;
      case 0x1a: // Mode sense
        _log.AddLogDebug("SCSI direct command 0x1a Mode sense\n");
        {
          // Show values for debug
          uint32_t senseData = _memory.ReadLong(scsiCmdStruct + 22); // senseData and related fields are only used for autosensing error condition when the command fail
          uint16_t senseLengthAllocated = _memory.ReadWord(scsiCmdStruct + 26); // Primary mode sense data go to scsi_Data
          uint8_t scsciCommandFlags = _memory.ReadByte(scsiCmdStruct + 20);

          uint8_t pageCode = _memory.ReadByte(scsiCommand + 2) & 0x3f;
          if (pageCode == 3)
          {
            uint16_t sectorsPerTrack = _devices[index].Configuration.Geometry.SectorsPerTrack;
            uint16_t bytesPerSector = _devices[index].Configuration.Geometry.BytesPerSector;

            // Header
            _memory.WriteByte(24 + 3, returnData);
            _memory.WriteByte(0, returnData + 1);
            _memory.WriteByte(0, returnData + 2);
            _memory.WriteByte(0, returnData + 3);

            // Page
            uint32_t destination = returnData + 4;
            _memory.WriteByte(3, destination);        // Page 3 format device
            _memory.WriteByte(0x16, destination + 1); // Page length
            _memory.WriteByte(0, destination + 2);    // Tracks per zone
            _memory.WriteByte(1, destination + 3);
            _memory.WriteByte(0, destination + 4); // Alternate sectors per zone
            _memory.WriteByte(0, destination + 5);
            _memory.WriteByte(0, destination + 6); // Alternate tracks per zone
            _memory.WriteByte(0, destination + 7);
            _memory.WriteByte(0, destination + 8); // Alternate tracks per volume
            _memory.WriteByte(0, destination + 9);
            _memory.WriteByte(sectorsPerTrack >> 8, destination + 10); // Sectors per track
            _memory.WriteByte(sectorsPerTrack & 0xff, destination + 11);
            _memory.WriteByte(bytesPerSector >> 8, destination + 12); // Data bytes per physical sector
            _memory.WriteByte(bytesPerSector & 0xff, destination + 13);
            _memory.WriteByte(0, destination + 14); // Interleave
            _memory.WriteByte(1, destination + 15);
            _memory.WriteByte(0, destination + 16); // Track skew factor
            _memory.WriteByte(0, destination + 17);
            _memory.WriteByte(0, destination + 18); // Cylinder skew factor
            _memory.WriteByte(0, destination + 19);
            _memory.WriteByte(0x80, destination + 20);
            _memory.WriteByte(0, destination + 21);
            _memory.WriteByte(0, destination + 22);
            _memory.WriteByte(0, destination + 23);

            _memory.WriteByte(0, scsiCmdStruct + 8); // data actual length (long word)
            _memory.WriteByte(0, scsiCmdStruct + 9);
            _memory.WriteByte(0, scsiCmdStruct + 10);
            _memory.WriteByte(24 + 4, scsiCmdStruct + 11);

            _memory.WriteByte(0, scsiCmdStruct + 28); // sense actual length (word)
            _memory.WriteByte(0, scsiCmdStruct + 29);
            _memory.WriteByte(0, scsiCmdStruct + 21); // Status
          }
          else if (pageCode == 4)
          {
            uint32_t numberOfCylinders = _devices[index].Configuration.Geometry.HighCylinder + 1;
            uint8_t surfaces = _devices[index].Configuration.Geometry.Surfaces;

            // Header
            _memory.WriteByte(24 + 3, returnData);
            _memory.WriteByte(0, returnData + 1);
            _memory.WriteByte(0, returnData + 2);
            _memory.WriteByte(0, returnData + 3);

            // Page
            uint32_t destination = returnData + 4;
            _memory.WriteByte(4, destination);                           // Page 4 Rigid disk geometry
            _memory.WriteByte(0x16, destination + 1);                    // Page length
            _memory.WriteByte(numberOfCylinders >> 16, destination + 2); // Number of cylinders (3 bytes)
            _memory.WriteByte(numberOfCylinders >> 8, destination + 3);
            _memory.WriteByte(numberOfCylinders, destination + 4);
            _memory.WriteByte(surfaces, destination + 5); // Number of heads
            _memory.WriteByte(0, destination + 6);        // Starting cylinder write precomp (3 bytes)
            _memory.WriteByte(0, destination + 7);
            _memory.WriteByte(0, destination + 8);
            _memory.WriteByte(0, destination + 9); // Starting cylinder reduces write current (3 bytes)
            _memory.WriteByte(0, destination + 10);
            _memory.WriteByte(0, destination + 11);
            _memory.WriteByte(0, destination + 12); // Drive step rate
            _memory.WriteByte(0, destination + 13);
            _memory.WriteByte(numberOfCylinders >> 16, destination + 14); // Landing zone cylinder (3 bytes)
            _memory.WriteByte(numberOfCylinders >> 8, destination + 15);
            _memory.WriteByte(numberOfCylinders, destination + 16);
            _memory.WriteByte(0, destination + 17);    // Nothing
            _memory.WriteByte(0, destination + 18);    // Rotational offset
            _memory.WriteByte(0, destination + 19);    // Reserved
            _memory.WriteByte(0x1c, destination + 20); // Medium rotation rate
            _memory.WriteByte(0x20, destination + 21);
            _memory.WriteByte(0, destination + 22); // Reserved
            _memory.WriteByte(0, destination + 23); // Reserved

            _memory.WriteByte(0, scsiCmdStruct + 8); // data actual length (long word)
            _memory.WriteByte(0, scsiCmdStruct + 9);
            _memory.WriteByte(0, scsiCmdStruct + 10);
            _memory.WriteByte(24 + 4, scsiCmdStruct + 11);

            _memory.WriteByte(0, scsiCmdStruct + 28); // sense actual length (word)
            _memory.WriteByte(0, scsiCmdStruct + 29);
            _memory.WriteByte(0, scsiCmdStruct + 21); // Status
          }
          else
          {
            error = -3;
          }
        }
        break;
      default:
        _log.AddLogDebug("SCSI direct command Unimplemented 0x%.2X\n", commandNumber);

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

    _configdev = _cpu.GetAReg(3);
    _memory.DmemSetLongNoCounter(FHFILE_MAX_DEVICES, 4088);
    _memory.DmemSetLongNoCounter(_configdev, 4092);
    _cpu.SetDReg(0, 1);
  }

  /*======================================*/
  /* Native callbacks for device commands */
  /*======================================*/

  // Returns D0 - 0 - Success, non-zero - Error
  void HardfileHandler::DoOpen()
  {
    uint32_t unit = _cpu.GetDReg(0);
    unsigned int index = GetIndexFromUnitNumber(unit);

    if (index < FHFILE_MAX_DEVICES && _devices[index].F != nullptr)
    {
      _memory.WriteByte(7, _cpu.GetAReg(1) + 8);                                                 /* ln_type (NT_REPLYMSG) */
      SetIOError(0);                                                                                   /* io_error */
      _memory.WriteLong(_cpu.GetDReg(0), _cpu.GetAReg(1) + 24);                               /* io_unit */
      _memory.WriteLong(_memory.ReadLong(_cpu.GetAReg(6) + 32) + 1, _cpu.GetAReg(6) + 32); /* LIB_OPENCNT */
      _cpu.SetDReg(0, 0);                                                                           /* Success */
    }
    else
    {
      _memory.WriteLong(static_cast<uint32_t>(-1), _cpu.GetAReg(1) + 20);
      SetIOError(-1);                                /* io_error */
      _cpu.SetDReg(0, static_cast<uint32_t>(-1)); /* Fail */
    }
  }

  void HardfileHandler::DoClose()
  {
    _memory.WriteLong(_memory.ReadLong(_cpu.GetAReg(6) + 32) - 1, _cpu.GetAReg(6) + 32); /* LIB_OPENCNT */
    _cpu.SetDReg(0, 0);                                                                           /* Causes invalid free-mem entry recoverable alert if omitted */
  }

  void HardfileHandler::DoExpunge()
  {
    _cpu.SetDReg(0, 0); /* ? */
  }

  void HardfileHandler::DoNULL()
  {
    _cpu.SetDReg(0, 0); /* ? */
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
      case 2: // CMD_READ
        error = Read(index);
        break;
      case 3: // CMD_WRITE
        error = Write(index);
        break;
      case 11: // TD_FORMAT
        _log.AddLogDebug("TD_FORMAT Unit %d\n", unit);
        error = Write(index);
        break;
      case 18: // TD_GETDRIVETYPE
        _log.AddLogDebug("TD_GETDRIVETYPE Unit %d\n", unit);
        // This does not make sense for us, options are 3.5" and 5 1/4"
        error = GetDiskDriveType(index);
        break;
      case 19: // TD_GETNUMTRACKS
        _log.AddLogDebug("TD_GETNUMTRACKS Unit %d\n", unit);
        error = GetNumberOfTracks(index);
        break;
      case 15: // TD_PROTSTATUS
        _log.AddLogDebug("TD_PROTSTATUS Unit %d\n", unit);
        WriteProt(index);
        break;
      case 4: // CMD_UPDATE ie. flush
        _log.AddLogDebug("CMD_UPDATE Unit %d\n", unit);
        IgnoreOK(index);
        break;
      case 5: // CMD_CLEAR
        _log.AddLogDebug("CMD 5 Unit %d\n", unit);
        IgnoreOK(index);
        break;
      case 9: // TD_MOTOR Only really used to turn motor off since device will turn on the motor automatically if reads and writes are received
        _log.AddLogDebug("TD_MOTOR Unit %d\n", unit);
        // Should set previous state of motor
        IgnoreOK(index);
        break;
      case 10: // TD_SEEK Used to pre-seek in advance, but reads and writes will also do the necessary seeks, so not useful here.
        _log.AddLogDebug("TD_SEEK Unit %d\n", unit);
        IgnoreOK(index);
        break;
      case 12: // TD_REMOVE
        _log.AddLogDebug("TD_REMOVE Unit %d\n", unit);
        // Perhaps unsupported instead?
        IgnoreOK(index);
        break;
      case 13: // TD_CHANGENUM
        _log.AddLogDebug("TD_CHANGENUM Unit %d\n", unit);
        // Should perhaps set some data here
        IgnoreOK(index);
        break;
      case 14: // TD_CHANGESTATE - check if a disk is currently in a drive. io_Actual - 0 - disk, 1 - no disk
        _log.AddLogDebug("TD_CHANGESTATE Unit %d\n", unit);
        SetIOError(0); // io_Error - 0 - success
        SetIOActual(0);
        break;
      case 20: // TD_ADDCHANGEINT
        _log.AddLogDebug("TD_ADDCHANGEINT Unit %d\n", unit);
        IgnoreOK(index);
        break;
      case 21: // TD_REMCHANGEINT
        _log.AddLogDebug("TD_REMCHANGEINT Unit %d\n", unit);
        IgnoreOK(index);
        break;
      case 16: // TD_RAWREAD
        _log.AddLogDebug("TD_RAWREAD Unit %d\n", unit);
        error = -3;
        break;
      case 17: // TD_RAWWRITE
        _log.AddLogDebug("TD_RAWWRITE Unit %d\n", unit);
        error = -3;
        break;
      case 22: // TD_GETGEOMETRY
        _log.AddLogDebug("TD_GEOMETRY Unit %d\n", unit);
        error = -3;
        break;
      case 23: // TD_EJECT
        _log.AddLogDebug("TD_EJECT Unit %d\n", unit);
        error = -3;
        break;
      case 28: error = ScsiDirect(index); break;
      default:
        _log.AddLogDebug("CMD Unknown %d Unit %d\n", cmd, unit);
        error = -3;
        break;
    }
    _memory.WriteByte(5, _cpu.GetAReg(1) + 8); /* ln_type (Reply message) */
    SetIOError(error);                               // ln_error
  }

  void HardfileHandler::DoAbortIO()
  {
    // Set some more success values here, this is ok, we never have pending io.
    _cpu.SetDReg(0, static_cast<uint32_t>(-3));
  }

  // RDB support functions, native callbacks

  uint32_t HardfileHandler::DoGetRDBFileSystemCount()
  {
    uint32_t count = (uint32_t)_fileSystems.size();
    _log.AddLogDebug("fhfile: DoGetRDBFilesystemCount() - Returns %u\n", count);
    return count;
  }

  uint32_t HardfileHandler::DoGetRDBFileSystemHunkCount(uint32_t fileSystemIndex)
  {
    uint32_t hunkCount = (uint32_t)_fileSystems[fileSystemIndex]->Header->FileSystemHandler.FileImage.GetInitialHunkCount();
    _log.AddLogDebug("fhfile: DoGetRDBFileSystemHunkCount(fileSystemIndex: %u) Returns %u\n", fileSystemIndex, hunkCount);
    return hunkCount;
  }

  uint32_t HardfileHandler::DoGetRDBFileSystemHunkSize(uint32_t fileSystemIndex, uint32_t hunkIndex)
  {
    uint32_t hunkSize = _fileSystems[fileSystemIndex]->Header->FileSystemHandler.FileImage.GetInitialHunk(hunkIndex)->GetAllocateSizeInBytes();
    _log.AddLogDebug("fhfile: DoGetRDBFileSystemHunkSize(fileSystemIndex: %u, hunkIndex: %u) Returns %u\n", fileSystemIndex, hunkIndex, hunkSize);
    return hunkSize;
  }

  void HardfileHandler::DoCopyRDBFileSystemHunk(uint32_t destinationAddress, uint32_t fileSystemIndex, uint32_t hunkIndex)
  {
    _log.AddLogDebug("fhfile: DoCopyRDBFileSystemHunk(destinationAddress: %.8X, fileSystemIndex: %u, hunkIndex: %u)\n", destinationAddress, fileSystemIndex, hunkIndex);

    HardfileFileSystemEntry *fileSystem = _fileSystems[fileSystemIndex].get();
    fileSystem->CopyHunkToAddress(destinationAddress + 8, hunkIndex);

    // Remember the address to the first hunk
    if (fileSystem->SegListAddress == 0)
    {
      fileSystem->SegListAddress = destinationAddress + 4;
    }

    uint32_t hunkSize = fileSystem->Header->FileSystemHandler.FileImage.GetInitialHunk(hunkIndex)->GetAllocateSizeInBytes();
    _memory.WriteLong(hunkSize + 8, destinationAddress); // Total size of allocation
    _memory.WriteLong(0, destinationAddress + 4);        // No next segment for now
  }

  void HardfileHandler::DoRelocateFileSystem(uint32_t fileSystemIndex)
  {
    _log.AddLogDebug("fhfile: DoRelocateFileSystem(fileSystemIndex: %u\n", fileSystemIndex);
    HardfileFileSystemEntry *fsEntry = _fileSystems[fileSystemIndex].get();
    fellow::hardfile::hunks::HunkRelocator hunkRelocator(_memory, fsEntry->Header->FileSystemHandler.FileImage);
    hunkRelocator.RelocateHunks();
  }

  void HardfileHandler::DoInitializeRDBFileSystemEntry(uint32_t fileSystemEntry, uint32_t fileSystemIndex)
  {
    _log.AddLogDebug("fhfile: DoInitializeRDBFileSystemEntry(fileSystemEntry: %.8X, fileSystemIndex: %u\n", fileSystemEntry, fileSystemIndex);

    HardfileFileSystemEntry *fsEntry = _fileSystems[fileSystemIndex].get();
    RDBFileSystemHeader *fsHeader = fsEntry->Header;

    _memory.WriteLong(_fsname, fileSystemEntry + 10);
    _memory.WriteLong(fsHeader->DOSType, fileSystemEntry + 14);
    _memory.WriteLong(fsHeader->Version, fileSystemEntry + 18);
    _memory.WriteLong(fsHeader->PatchFlags, fileSystemEntry + 22);
    _memory.WriteLong(fsHeader->DnType, fileSystemEntry + 26);
    _memory.WriteLong(fsHeader->DnTask, fileSystemEntry + 30);
    _memory.WriteLong(fsHeader->DnLock, fileSystemEntry + 34);
    _memory.WriteLong(fsHeader->DnHandler, fileSystemEntry + 38);
    _memory.WriteLong(fsHeader->DnStackSize, fileSystemEntry + 42);
    _memory.WriteLong(fsHeader->DnPriority, fileSystemEntry + 46);
    _memory.WriteLong(fsHeader->DnStartup, fileSystemEntry + 50);
    _memory.WriteLong(fsEntry->SegListAddress >> 2, fileSystemEntry + 54);
    //  _memory.WriteLong(fsHeader.DnSegListBlock, fileSystemEntry + 54);
    _memory.WriteLong(fsHeader->DnGlobalVec, fileSystemEntry + 58);

    for (int i = 0; i < 23; i++)
    {
      _memory.WriteLong(fsHeader->Reserved2[i], fileSystemEntry + 62 + i * 4);
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
    char c = _memory.ReadByte(address);
    address++;
    while (c != 0)
    {
      name.push_back(c == '\n' ? '.' : c);
      c = _memory.ReadByte(address);
      address++;
    }
    return name;
  }

  void HardfileHandler::DoLogAvailableResources()
  {
    _log.AddLogDebug("fhfile: DoLogAvailableResources()\n");

    uint32_t execBase = _memory.ReadLong(4); // Fetch list from exec
    uint32_t rsListHeader = _memory.ReadLong(execBase + 0x150);

    _log.AddLogDebug(
        "fhfile: Resource list header (%.8X): Head %.8X Tail %.8X TailPred %.8X Type %d\n",
        rsListHeader,
        _memory.ReadLong(rsListHeader),
        _memory.ReadLong(rsListHeader + 4),
        _memory.ReadLong(rsListHeader + 8),
        _memory.ReadByte(rsListHeader + 9));

    if (rsListHeader == _memory.ReadLong(rsListHeader + 8))
    {
      _log.AddLogDebug("fhfile: Resource list is empty.\n");
      return;
    }

    uint32_t fsNode = _memory.ReadLong(rsListHeader);
    while (fsNode != 0 && (fsNode != rsListHeader + 4))
    {
      _log.AddLogDebug(
          "fhfile: ResourceEntry Node (%.8X): Succ %.8X Pred %.8X Type %d Pri %d NodeName '%s'\n",
          fsNode,
          _memory.ReadLong(fsNode),
          _memory.ReadLong(fsNode + 4),
          _memory.ReadByte(fsNode + 8),
          _memory.ReadByte(fsNode + 9),
          LogGetStringFromMemory(_memory.ReadLong(fsNode + 10)).c_str());

      fsNode = _memory.ReadLong(fsNode);
    }
  }

  void HardfileHandler::DoLogAllocMemResult(uint32_t result)
  {
    _log.AddLogDebug("fhfile: AllocMem() returned %.8X\n", result);
  }

  void HardfileHandler::DoLogOpenResourceResult(uint32_t result)
  {
    _log.AddLogDebug("fhfile: OpenResource() returned %.8X\n", result);
  }

  void HardfileHandler::DoRemoveRDBFileSystemsAlreadySupportedBySystem(uint32_t filesystemResource)
  {
    _log.AddLogDebug("fhfile: DoRemoveRDBFileSystemsAlreadySupportedBySystem(filesystemResource: %.8X)\n", filesystemResource);

    uint32_t fsList = filesystemResource + 18;
    if (fsList == _memory.ReadLong(fsList + 8))
    {
      _log.AddLogDebug("fhfile: FileSystemEntry list is empty.\n");
      return;
    }

    uint32_t fsNode = _memory.ReadLong(fsList);
    while (fsNode != 0 && (fsNode != fsList + 4))
    {
      uint32_t fsEntry = fsNode + 14;
      uint32_t dosType = _memory.ReadLong(fsEntry);
      uint32_t version = _memory.ReadLong(fsEntry + 4);

      _log.AddLogDebug("fhfile: FileSystemEntry DosType   : %.8X\n", _memory.ReadLong(fsEntry));
      _log.AddLogDebug("fhfile: FileSystemEntry Version   : %.8X\n", _memory.ReadLong(fsEntry + 4));
      _log.AddLogDebug("fhfile: FileSystemEntry PatchFlags: %.8X\n", _memory.ReadLong(fsEntry + 8));
      _log.AddLogDebug("fhfile: FileSystemEntry Type      : %.8X\n", _memory.ReadLong(fsEntry + 12));
      _log.AddLogDebug("fhfile: FileSystemEntry Task      : %.8X\n", _memory.ReadLong(fsEntry + 16));
      _log.AddLogDebug("fhfile: FileSystemEntry Lock      : %.8X\n", _memory.ReadLong(fsEntry + 20));
      _log.AddLogDebug("fhfile: FileSystemEntry Handler   : %.8X\n", _memory.ReadLong(fsEntry + 24));
      _log.AddLogDebug("fhfile: FileSystemEntry StackSize : %.8X\n", _memory.ReadLong(fsEntry + 28));
      _log.AddLogDebug("fhfile: FileSystemEntry Priority  : %.8X\n", _memory.ReadLong(fsEntry + 32));
      _log.AddLogDebug("fhfile: FileSystemEntry Startup   : %.8X\n", _memory.ReadLong(fsEntry + 36));
      _log.AddLogDebug("fhfile: FileSystemEntry SegList   : %.8X\n", _memory.ReadLong(fsEntry + 40));
      _log.AddLogDebug("fhfile: FileSystemEntry GlobalVec : %.8X\n\n", _memory.ReadLong(fsEntry + 44));
      fsNode = _memory.ReadLong(fsNode);

      EraseOlderOrSameFileSystemVersion(dosType, version);
    }
  }

  // D0 - pointer to FileSystem.resource
  void HardfileHandler::DoLogAvailableFileSystems(uint32_t fileSystemResource)
  {
    _log.AddLogDebug("fhfile: DoLogAvailableFileSystems(fileSystemResource: %.8X)\n", fileSystemResource);

    uint32_t fsList = fileSystemResource + 18;
    if (fsList == _memory.ReadLong(fsList + 8))
    {
      _log.AddLogDebug("fhfile: FileSystemEntry list is empty.\n");
      return;
    }

    uint32_t fsNode = _memory.ReadLong(fsList);
    while (fsNode != 0 && (fsNode != fsList + 4))
    {
      uint32_t fsEntry = fsNode + 14;

      uint32_t dosType = _memory.ReadLong(fsEntry);
      uint32_t version = _memory.ReadLong(fsEntry + 4);

      _log.AddLogDebug("fhfile: FileSystemEntry DosType   : %.8X\n", dosType);
      _log.AddLogDebug("fhfile: FileSystemEntry Version   : %.8X\n", version);
      _log.AddLogDebug("fhfile: FileSystemEntry PatchFlags: %.8X\n", _memory.ReadLong(fsEntry + 8));
      _log.AddLogDebug("fhfile: FileSystemEntry Type      : %.8X\n", _memory.ReadLong(fsEntry + 12));
      _log.AddLogDebug("fhfile: FileSystemEntry Task      : %.8X\n", _memory.ReadLong(fsEntry + 16));
      _log.AddLogDebug("fhfile: FileSystemEntry Lock      : %.8X\n", _memory.ReadLong(fsEntry + 20));
      _log.AddLogDebug("fhfile: FileSystemEntry Handler   : %.8X\n", _memory.ReadLong(fsEntry + 24));
      _log.AddLogDebug("fhfile: FileSystemEntry StackSize : %.8X\n", _memory.ReadLong(fsEntry + 28));
      _log.AddLogDebug("fhfile: FileSystemEntry Priority  : %.8X\n", _memory.ReadLong(fsEntry + 32));
      _log.AddLogDebug("fhfile: FileSystemEntry Startup   : %.8X\n", _memory.ReadLong(fsEntry + 36));
      _log.AddLogDebug("fhfile: FileSystemEntry SegList   : %.8X\n", _memory.ReadLong(fsEntry + 40));
      _log.AddLogDebug("fhfile: FileSystemEntry GlobalVec : %.8X\n\n", _memory.ReadLong(fsEntry + 44));
      fsNode = _memory.ReadLong(fsNode);
    }
  }

  void HardfileHandler::DoPatchDOSDeviceNode(uint32_t node, uint32_t packet)
  {
    _log.AddLogDebug("fhfile: DoPatchDOSDeviceNode(node: %.8X, packet: %.8X)\n", node, packet);

    _memory.WriteLong(0, node + 8);                          // dn_Task = 0
    _memory.WriteLong(0, node + 16);                         // dn_Handler = 0
    _memory.WriteLong(static_cast<uint32_t>(-1), node + 36); // dn_GlobalVec = -1

    HardfileFileSystemEntry *fs = GetFileSystemForDOSType(_memory.ReadLong(packet + 80));
    if (fs != nullptr)
    {
      if (fs->Header->PatchFlags & 0x10)
      {
        _memory.WriteLong(fs->Header->DnStackSize, node + 20);
      }
      if (fs->Header->PatchFlags & 0x80)
      {
        _memory.WriteLong(fs->SegListAddress >> 2, node + 32);
      }
      if (fs->Header->PatchFlags & 0x100)
      {
        _memory.WriteLong(fs->Header->DnGlobalVec, node + 36);
      }
    }
  }

  void HardfileHandler::DoUnknownOperation(uint32_t operation)
  {
    _log.AddLogDebug("fhfile: Unknown operation called %X\n", operation);
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
        case 1: _cpu.SetDReg(0, DoGetRDBFileSystemCount()); break;
        case 2: _cpu.SetDReg(0, DoGetRDBFileSystemHunkCount(_cpu.GetDReg(1))); break;
        case 3: _cpu.SetDReg(0, DoGetRDBFileSystemHunkSize(_cpu.GetDReg(1), _cpu.GetDReg(2))); break;
        case 4:
          DoCopyRDBFileSystemHunk(
              _cpu.GetDReg(0),  // VM memory allocated for hunk
              _cpu.GetDReg(1),  // Filesystem index on the device
              _cpu.GetDReg(2)); // Hunk index in filesystem fileimage
          break;
        case 5: DoInitializeRDBFileSystemEntry(_cpu.GetDReg(0), _cpu.GetDReg(1)); break;
        case 6: DoRemoveRDBFileSystemsAlreadySupportedBySystem(_cpu.GetDReg(0)); break;
        case 7: DoPatchDOSDeviceNode(_cpu.GetDReg(0), _cpu.GetAReg(5)); break;
        case 8: DoRelocateFileSystem(_cpu.GetDReg(1)); break;
        case 9: _cpu.SetDReg(0, DoGetDosDevPacketListStart()); break;
        case 0xa0: DoLogAllocMemResult(_cpu.GetDReg(0)); break;
        case 0xa1: DoLogOpenResourceResult(_cpu.GetDReg(0)); break;
        case 0xa2: DoLogAvailableResources(); break;
        case 0xa3: DoLogAvailableFileSystems(_cpu.GetDReg(0)); break;
        default: DoUnknownOperation(operation);
      }
    }
  }

  /*=================================================*/
  /* Make a dosdevice packet about the device layout */
  /*=================================================*/

  void HardfileHandler::MakeDOSDevPacketForPlainHardfile(const HardfileMountListEntry &mountListEntry, uint32_t deviceNameAddress)
  {
    const HardfileDevice &device = _devices[mountListEntry.DeviceIndex];
    if (device.F != nullptr)
    {
      uint32_t unit = GetUnitNumberFromIndex(mountListEntry.DeviceIndex);
      const HardfileGeometry &geometry = device.Configuration.Geometry;
      _memory.DmemSetLong(mountListEntry.DeviceIndex); /* Flag to initcode */

      _memory.DmemSetLong(mountListEntry.NameAddress); /*  0 Unit name "FELLOWX" or similar */
      _memory.DmemSetLong(deviceNameAddress);          /*  4 Device name "fhfile.device" */
      _memory.DmemSetLong(unit);                       /*  8 Unit # */
      _memory.DmemSetLong(0);                          /* 12 OpenDevice flags */

      // Struct DosEnvec
      _memory.DmemSetLong(16);                           /* 16 Environment size in long words */
      _memory.DmemSetLong(geometry.BytesPerSector >> 2); /* 20 Longwords in a block */
      _memory.DmemSetLong(0);                            /* 24 sector origin (unused) */
      _memory.DmemSetLong(geometry.Surfaces);            /* 28 Heads */
      _memory.DmemSetLong(1);                            /* 32 Sectors per logical block (unused) */
      _memory.DmemSetLong(geometry.SectorsPerTrack);     /* 36 Sectors per track */
      _memory.DmemSetLong(geometry.ReservedBlocks);      /* 40 Reserved blocks, min. 1 */
      _memory.DmemSetLong(0);                            /* 44 mdn_prefac - Unused */
      _memory.DmemSetLong(0);                            /* 48 Interleave */
      _memory.DmemSetLong(0);                            /* 52 Lower cylinder */
      _memory.DmemSetLong(geometry.HighCylinder);        /* 56 Upper cylinder */
      _memory.DmemSetLong(0);                            /* 60 Number of buffers */
      _memory.DmemSetLong(0);                            /* 64 Type of memory for buffers */
      _memory.DmemSetLong(0x7fffffff);                   /* 68 Largest transfer */
      _memory.DmemSetLong(~1U);                          /* 72 Add mask */
      _memory.DmemSetLong(static_cast<uint32_t>(-1));    /* 76 Boot priority */
      _memory.DmemSetLong(0x444f5300);                   /* 80 DOS file handler name */
      _memory.DmemSetLong(0);
    }
  }

  void HardfileHandler::MakeDOSDevPacketForRDBPartition(const HardfileMountListEntry &mountListEntry, uint32_t deviceNameAddress)
  {
    HardfileDevice &device = _devices[mountListEntry.DeviceIndex];
    RDBPartition *partition = device.RDB->Partitions[mountListEntry.PartitionIndex].get();
    if (device.F != nullptr)
    {
      uint32_t unit = GetUnitNumberFromIndex(mountListEntry.DeviceIndex);

      _memory.DmemSetLong(mountListEntry.DeviceIndex); /* Flag to initcode */

      _memory.DmemSetLong(mountListEntry.NameAddress); /*  0 Unit name "FELLOWX" or similar */
      _memory.DmemSetLong(deviceNameAddress);          /*  4 Device name "fhfile.device" */
      _memory.DmemSetLong(unit);                       /*  8 Unit # */
      _memory.DmemSetLong(0);                          /* 12 OpenDevice flags */

      // Struct DosEnvec
      _memory.DmemSetLong(16);                         /* 16 Environment size in long words*/
      _memory.DmemSetLong(partition->SizeBlock);       /* 20 Longwords in a block */
      _memory.DmemSetLong(partition->SecOrg);          /* 24 sector origin (unused) */
      _memory.DmemSetLong(partition->Surfaces);        /* 28 Heads */
      _memory.DmemSetLong(partition->SectorsPerBlock); /* 32 Sectors per logical block (unused) */
      _memory.DmemSetLong(partition->BlocksPerTrack);  /* 36 Sectors per track */
      _memory.DmemSetLong(partition->Reserved);        /* 40 Reserved blocks, min. 1 */
      _memory.DmemSetLong(partition->PreAlloc);        /* 44 mdn_prefac - Unused */
      _memory.DmemSetLong(partition->Interleave);      /* 48 Interleave */
      _memory.DmemSetLong(partition->LowCylinder);     /* 52 Lower cylinder */
      _memory.DmemSetLong(partition->HighCylinder);    /* 56 Upper cylinder */
      _memory.DmemSetLong(partition->NumBuffer);       /* 60 Number of buffers */
      _memory.DmemSetLong(partition->BufMemType);      /* 64 Type of memory for buffers */
      _memory.DmemSetLong(partition->MaxTransfer);     /* 68 Largest transfer */
      _memory.DmemSetLong(partition->Mask);            /* 72 Add mask */
      _memory.DmemSetLong(partition->BootPri);         /* 76 Boot priority */
      _memory.DmemSetLong(partition->DOSType);         /* 80 DOS file handler name */
      _memory.DmemSetLong(0);
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
    if (!HasZeroDevices() && GetEnabled() && _memory.GetKickImageVersion() >= 34)
    {
      _memory.DmemSetCounter(0);

      /* Device-name and ID string */

      _devicename = _memory.DmemGetCounter();
      _memory.DmemSetString("fhfile.device");
      uint32_t idstr = _memory.DmemGetCounter();
      _memory.DmemSetString("Fellow Hardfile device V5");
      uint32_t doslibname = _memory.DmemGetCounter();
      _memory.DmemSetString("dos.library");
      _fsname = _memory.DmemGetCounter();
      _memory.DmemSetString("Fellow hardfile RDB fs");

      /* fhfile.open */

      uint32_t fhfile_t_open = _memory.DmemGetCounter();
      _memory.DmemSetWord(0x23fc);
      _memory.DmemSetLong(0x00010002);
      _memory.DmemSetLong(0xf40000); /* move.l #$00010002,$f40000 */
      _memory.DmemSetWord(0x4e75);   /* rts */

      /* fhfile.close */

      uint32_t fhfile_t_close = _memory.DmemGetCounter();
      _memory.DmemSetWord(0x23fc);
      _memory.DmemSetLong(0x00010003);
      _memory.DmemSetLong(0xf40000); /* move.l #$00010003,$f40000 */
      _memory.DmemSetWord(0x4e75);   /* rts */

      /* fhfile.expunge */

      uint32_t fhfile_t_expunge = _memory.DmemGetCounter();
      _memory.DmemSetWord(0x23fc);
      _memory.DmemSetLong(0x00010004);
      _memory.DmemSetLong(0xf40000); /* move.l #$00010004,$f40000 */
      _memory.DmemSetWord(0x4e75);   /* rts */

      /* fhfile.null */

      uint32_t fhfile_t_null = _memory.DmemGetCounter();
      _memory.DmemSetWord(0x23fc);
      _memory.DmemSetLong(0x00010005);
      _memory.DmemSetLong(0xf40000); /* move.l #$00010005,$f40000 */
      _memory.DmemSetWord(0x4e75);   /* rts */

      /* fhfile.beginio */

      uint32_t fhfile_t_beginio = _memory.DmemGetCounter();
      _memory.DmemSetWord(0x23fc);
      _memory.DmemSetLong(0x00010006);
      _memory.DmemSetLong(0xf40000);   /* move.l #$00010006,$f40000  BeginIO */
      _memory.DmemSetLong(0x48e78002); /* movem.l d0/a6,-(a7)    push        */
      _memory.DmemSetLong(0x08290000);
      _memory.DmemSetWord(0x001e);     /* btst   #$0,30(a1)      IOF_QUICK   */
      _memory.DmemSetWord(0x6608);     /* bne    (to pop)                    */
      _memory.DmemSetLong(0x2c780004); /* move.l $4.w,a6         exec        */
      _memory.DmemSetLong(0x4eaefe86); /* jsr    -378(a6)        ReplyMsg(a1)*/
      _memory.DmemSetLong(0x4cdf4001); /* movem.l (a7)+,d0/a6    pop         */
      _memory.DmemSetWord(0x4e75);     /* rts */

      /* fhfile.abortio */

      uint32_t fhfile_t_abortio = _memory.DmemGetCounter();
      _memory.DmemSetWord(0x23fc);
      _memory.DmemSetLong(0x00010007);
      _memory.DmemSetLong(0xf40000); /* move.l #$00010007,$f40000 */
      _memory.DmemSetWord(0x4e75);   /* rts */

      /* Func-table */

      uint32_t functable = _memory.DmemGetCounter();
      _memory.DmemSetLong(fhfile_t_open);
      _memory.DmemSetLong(fhfile_t_close);
      _memory.DmemSetLong(fhfile_t_expunge);
      _memory.DmemSetLong(fhfile_t_null);
      _memory.DmemSetLong(fhfile_t_beginio);
      _memory.DmemSetLong(fhfile_t_abortio);
      _memory.DmemSetLong(0xffffffff);

      /* Data-table */

      uint32_t datatable = _memory.DmemGetCounter();
      _memory.DmemSetWord(0xE000); /* INITBYTE */
      _memory.DmemSetWord(0x0008); /* LN_TYPE */
      _memory.DmemSetWord(0x0300); /* NT_DEVICE */
      _memory.DmemSetWord(0xC000); /* INITLONG */
      _memory.DmemSetWord(0x000A); /* LN_NAME */
      _memory.DmemSetLong(_devicename);
      _memory.DmemSetWord(0xE000); /* INITBYTE */
      _memory.DmemSetWord(0x000E); /* LIB_FLAGS */
      _memory.DmemSetWord(0x0600); /* LIBF_SUMUSED+LIBF_CHANGED */
      _memory.DmemSetWord(0xD000); /* INITWORD */
      _memory.DmemSetWord(0x0014); /* LIB_VERSION */
      _memory.DmemSetWord(0x0002);
      _memory.DmemSetWord(0xD000); /* INITWORD */
      _memory.DmemSetWord(0x0016); /* LIB_REVISION */
      _memory.DmemSetWord(0x0000);
      _memory.DmemSetWord(0xC000); /* INITLONG */
      _memory.DmemSetWord(0x0018); /* LIB_IDSTRING */
      _memory.DmemSetLong(idstr);
      _memory.DmemSetLong(0); /* END */

      /* bootcode */

      _bootcode = _memory.DmemGetCounter();
      _memory.DmemSetWord(0x227c);
      _memory.DmemSetLong(doslibname); /* move.l #doslibname,a1 */
      _memory.DmemSetLong(0x4eaeffa0); /* jsr    -96(a6) ; FindResident() */
      _memory.DmemSetWord(0x2040);     /* move.l d0,a0 */
      _memory.DmemSetLong(0x20280016); /* move.l 22(a0),d0 */
      _memory.DmemSetWord(0x2040);     /* move.l d0,a0 */
      _memory.DmemSetWord(0x4e90);     /* jsr    (a0) */
      _memory.DmemSetWord(0x4e75);     /* rts */

      /* fhfile.init */

      uint32_t fhfile_t_init = _memory.DmemGetCounter();

      _memory.DmemSetByte(0x48);
      _memory.DmemSetByte(0xE7);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0xFE);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xF6);
      _memory.DmemSetByte(0x4A);
      _memory.DmemSetByte(0x80);
      _memory.DmemSetByte(0x67);
      _memory.DmemSetByte(0x24);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xD6);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x01);
      _memory.DmemSetByte(0xB0);
      _memory.DmemSetByte(0x4A);
      _memory.DmemSetByte(0x80);
      _memory.DmemSetByte(0x66);
      _memory.DmemSetByte(0x0C);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x01);
      _memory.DmemSetByte(0x6A);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x01);
      _memory.DmemSetByte(0xA4);
      _memory.DmemSetByte(0x4A);
      _memory.DmemSetByte(0x80);
      _memory.DmemSetByte(0x67);
      _memory.DmemSetByte(0x0C);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x01);
      _memory.DmemSetByte(0x12);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x02);
      _memory.DmemSetByte(0x0A);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xC2);
      _memory.DmemSetByte(0x2C);
      _memory.DmemSetByte(0x78);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x04);
      _memory.DmemSetByte(0x43);
      _memory.DmemSetByte(0xFA);
      _memory.DmemSetByte(0x02);
      _memory.DmemSetByte(0x20);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0xAE);
      _memory.DmemSetByte(0xFE);
      _memory.DmemSetByte(0x68);
      _memory.DmemSetByte(0x28);
      _memory.DmemSetByte(0x40);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x01);
      _memory.DmemSetByte(0x1C);
      _memory.DmemSetByte(0x20);
      _memory.DmemSetByte(0x40);
      _memory.DmemSetByte(0x2E);
      _memory.DmemSetByte(0x08);
      _memory.DmemSetByte(0x20);
      _memory.DmemSetByte(0x47);
      _memory.DmemSetByte(0x4A);
      _memory.DmemSetByte(0x90);
      _memory.DmemSetByte(0x6B);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x70);
      _memory.DmemSetByte(0x58);
      _memory.DmemSetByte(0x87);
      _memory.DmemSetByte(0x20);
      _memory.DmemSetByte(0x3C);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x58);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x01);
      _memory.DmemSetByte(0x10);
      _memory.DmemSetByte(0x2A);
      _memory.DmemSetByte(0x40);
      _memory.DmemSetByte(0x20);
      _memory.DmemSetByte(0x47);
      _memory.DmemSetByte(0x70);
      _memory.DmemSetByte(0x54);
      _memory.DmemSetByte(0x2B);
      _memory.DmemSetByte(0xB0);
      _memory.DmemSetByte(0x08);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x08);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x59);
      _memory.DmemSetByte(0x80);
      _memory.DmemSetByte(0x64);
      _memory.DmemSetByte(0xF6);
      _memory.DmemSetByte(0xCD);
      _memory.DmemSetByte(0x4C);
      _memory.DmemSetByte(0x20);
      _memory.DmemSetByte(0x4D);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0xAE);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0x70);
      _memory.DmemSetByte(0xCD);
      _memory.DmemSetByte(0x4C);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xCE);
      _memory.DmemSetByte(0x26);
      _memory.DmemSetByte(0x40);
      _memory.DmemSetByte(0x70);
      _memory.DmemSetByte(0x14);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xEA);
      _memory.DmemSetByte(0x22);
      _memory.DmemSetByte(0x47);
      _memory.DmemSetByte(0x2C);
      _memory.DmemSetByte(0x29);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0xFC);
      _memory.DmemSetByte(0x22);
      _memory.DmemSetByte(0x40);
      _memory.DmemSetByte(0x70);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x22);
      _memory.DmemSetByte(0x80);
      _memory.DmemSetByte(0x23);
      _memory.DmemSetByte(0x40);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x04);
      _memory.DmemSetByte(0x33);
      _memory.DmemSetByte(0x40);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x0E);
      _memory.DmemSetByte(0x33);
      _memory.DmemSetByte(0x7C);
      _memory.DmemSetByte(0x10);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x08);
      _memory.DmemSetByte(0x9D);
      _memory.DmemSetByte(0x69);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x08);
      _memory.DmemSetByte(0x23);
      _memory.DmemSetByte(0x79);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xF4);
      _memory.DmemSetByte(0x0F);
      _memory.DmemSetByte(0xFC);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x0A);
      _memory.DmemSetByte(0x23);
      _memory.DmemSetByte(0x4B);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x10);
      _memory.DmemSetByte(0x41);
      _memory.DmemSetByte(0xEC);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x4A);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0xAE);
      _memory.DmemSetByte(0xFE);
      _memory.DmemSetByte(0xF2);
      _memory.DmemSetByte(0x06);
      _memory.DmemSetByte(0x87);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x58);
      _memory.DmemSetByte(0x60);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0x8C);
      _memory.DmemSetByte(0x2C);
      _memory.DmemSetByte(0x78);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x04);
      _memory.DmemSetByte(0x22);
      _memory.DmemSetByte(0x4C);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0xAE);
      _memory.DmemSetByte(0xFE);
      _memory.DmemSetByte(0x62);
      _memory.DmemSetByte(0x4C);
      _memory.DmemSetByte(0xDF);
      _memory.DmemSetByte(0x7F);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x23);
      _memory.DmemSetByte(0xFC);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x02);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xA0);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xF4);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x23);
      _memory.DmemSetByte(0xFC);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x02);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xA1);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xF4);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x23);
      _memory.DmemSetByte(0xFC);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x02);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xA2);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xF4);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x23);
      _memory.DmemSetByte(0xFC);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x02);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xA3);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xF4);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x23);
      _memory.DmemSetByte(0xFC);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x02);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x01);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xF4);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x23);
      _memory.DmemSetByte(0xFC);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x02);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x02);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xF4);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x23);
      _memory.DmemSetByte(0xFC);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x02);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x03);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xF4);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x23);
      _memory.DmemSetByte(0xFC);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x02);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x04);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xF4);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x23);
      _memory.DmemSetByte(0xFC);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x02);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x05);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xF4);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x23);
      _memory.DmemSetByte(0xFC);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x02);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x06);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xF4);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x23);
      _memory.DmemSetByte(0xFC);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x02);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x07);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xF4);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x23);
      _memory.DmemSetByte(0xFC);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x02);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x08);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xF4);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x23);
      _memory.DmemSetByte(0xFC);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x02);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x09);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xF4);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x48);
      _memory.DmemSetByte(0xE7);
      _memory.DmemSetByte(0x78);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x22);
      _memory.DmemSetByte(0x3C);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x01);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x01);
      _memory.DmemSetByte(0x2C);
      _memory.DmemSetByte(0x78);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x04);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0xAE);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0x3A);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0x50);
      _memory.DmemSetByte(0x4C);
      _memory.DmemSetByte(0xDF);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x1E);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x20);
      _memory.DmemSetByte(0x3C);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x20);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0xDC);
      _memory.DmemSetByte(0x2A);
      _memory.DmemSetByte(0x40);
      _memory.DmemSetByte(0x1B);
      _memory.DmemSetByte(0x7C);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x08);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x08);
      _memory.DmemSetByte(0x41);
      _memory.DmemSetByte(0xFA);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xD0);
      _memory.DmemSetByte(0x2B);
      _memory.DmemSetByte(0x48);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x0A);
      _memory.DmemSetByte(0x41);
      _memory.DmemSetByte(0xFA);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xDC);
      _memory.DmemSetByte(0x2B);
      _memory.DmemSetByte(0x48);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x0E);
      _memory.DmemSetByte(0x49);
      _memory.DmemSetByte(0xED);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x12);
      _memory.DmemSetByte(0x28);
      _memory.DmemSetByte(0x8C);
      _memory.DmemSetByte(0x06);
      _memory.DmemSetByte(0x94);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x04);
      _memory.DmemSetByte(0x42);
      _memory.DmemSetByte(0xAC);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x04);
      _memory.DmemSetByte(0x29);
      _memory.DmemSetByte(0x4C);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x08);
      _memory.DmemSetByte(0x22);
      _memory.DmemSetByte(0x4D);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0xAE);
      _memory.DmemSetByte(0xFE);
      _memory.DmemSetByte(0x1A);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x2C);
      _memory.DmemSetByte(0x78);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x04);
      _memory.DmemSetByte(0x70);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x43);
      _memory.DmemSetByte(0xFA);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x9E);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0xAE);
      _memory.DmemSetByte(0xFE);
      _memory.DmemSetByte(0x0E);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0x06);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0x30);
      _memory.DmemSetByte(0x4A);
      _memory.DmemSetByte(0x80);
      _memory.DmemSetByte(0x67);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x26);
      _memory.DmemSetByte(0x28);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x74);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0x2E);
      _memory.DmemSetByte(0x4A);
      _memory.DmemSetByte(0x80);
      _memory.DmemSetByte(0x67);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x0C);
      _memory.DmemSetByte(0x50);
      _memory.DmemSetByte(0x80);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0x76);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0x2A);
      _memory.DmemSetByte(0x52);
      _memory.DmemSetByte(0x82);
      _memory.DmemSetByte(0xB8);
      _memory.DmemSetByte(0x82);
      _memory.DmemSetByte(0x66);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0xE6);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x2F);
      _memory.DmemSetByte(0x08);
      _memory.DmemSetByte(0x2F);
      _memory.DmemSetByte(0x01);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0xCA);
      _memory.DmemSetByte(0x20);
      _memory.DmemSetByte(0x3C);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xBE);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0x52);
      _memory.DmemSetByte(0x22);
      _memory.DmemSetByte(0x1F);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0x10);
      _memory.DmemSetByte(0x2C);
      _memory.DmemSetByte(0x79);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x04);
      _memory.DmemSetByte(0x20);
      _memory.DmemSetByte(0x57);
      _memory.DmemSetByte(0x41);
      _memory.DmemSetByte(0xE8);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x12);
      _memory.DmemSetByte(0x22);
      _memory.DmemSetByte(0x40);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0xAE);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0x10);
      _memory.DmemSetByte(0x20);
      _memory.DmemSetByte(0x5F);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x2F);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x20);
      _memory.DmemSetByte(0x40);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xFE);
      _memory.DmemSetByte(0xC2);
      _memory.DmemSetByte(0x4A);
      _memory.DmemSetByte(0x80);
      _memory.DmemSetByte(0x67);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x12);
      _memory.DmemSetByte(0x26);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x72);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0xBE);
      _memory.DmemSetByte(0x52);
      _memory.DmemSetByte(0x81);
      _memory.DmemSetByte(0xB6);
      _memory.DmemSetByte(0x81);
      _memory.DmemSetByte(0x66);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0xFF);
      _memory.DmemSetByte(0xF6);
      _memory.DmemSetByte(0x20);
      _memory.DmemSetByte(0x1F);
      _memory.DmemSetByte(0x4E);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x65);
      _memory.DmemSetByte(0x78);
      _memory.DmemSetByte(0x70);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x6E);
      _memory.DmemSetByte(0x73);
      _memory.DmemSetByte(0x69);
      _memory.DmemSetByte(0x6F);
      _memory.DmemSetByte(0x6E);
      _memory.DmemSetByte(0x2E);
      _memory.DmemSetByte(0x6C);
      _memory.DmemSetByte(0x69);
      _memory.DmemSetByte(0x62);
      _memory.DmemSetByte(0x72);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x72);
      _memory.DmemSetByte(0x79);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x46);
      _memory.DmemSetByte(0x69);
      _memory.DmemSetByte(0x6C);
      _memory.DmemSetByte(0x65);
      _memory.DmemSetByte(0x53);
      _memory.DmemSetByte(0x79);
      _memory.DmemSetByte(0x73);
      _memory.DmemSetByte(0x74);
      _memory.DmemSetByte(0x65);
      _memory.DmemSetByte(0x6D);
      _memory.DmemSetByte(0x2E);
      _memory.DmemSetByte(0x72);
      _memory.DmemSetByte(0x65);
      _memory.DmemSetByte(0x73);
      _memory.DmemSetByte(0x6F);
      _memory.DmemSetByte(0x75);
      _memory.DmemSetByte(0x72);
      _memory.DmemSetByte(0x63);
      _memory.DmemSetByte(0x65);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x46);
      _memory.DmemSetByte(0x65);
      _memory.DmemSetByte(0x6C);
      _memory.DmemSetByte(0x6C);
      _memory.DmemSetByte(0x6F);
      _memory.DmemSetByte(0x77);
      _memory.DmemSetByte(0x20);
      _memory.DmemSetByte(0x68);
      _memory.DmemSetByte(0x61);
      _memory.DmemSetByte(0x72);
      _memory.DmemSetByte(0x64);
      _memory.DmemSetByte(0x66);
      _memory.DmemSetByte(0x69);
      _memory.DmemSetByte(0x6C);
      _memory.DmemSetByte(0x65);
      _memory.DmemSetByte(0x20);
      _memory.DmemSetByte(0x64);
      _memory.DmemSetByte(0x65);
      _memory.DmemSetByte(0x76);
      _memory.DmemSetByte(0x69);
      _memory.DmemSetByte(0x63);
      _memory.DmemSetByte(0x65);
      _memory.DmemSetByte(0x00);
      _memory.DmemSetByte(0x00);

      /* Init-struct */

      uint32_t initstruct = _memory.DmemGetCounter();
      _memory.DmemSetLong(0x100);         /* Data-space size, min LIB_SIZE */
      _memory.DmemSetLong(functable);     /* Function-table */
      _memory.DmemSetLong(datatable);     /* Data-table */
      _memory.DmemSetLong(fhfile_t_init); /* Init-routine */

      /* RomTag structure */

      uint32_t romtagstart = _memory.DmemGetCounter();
      _memory.DmemSetWord(0x4afc);           /* Start of structure */
      _memory.DmemSetLong(romtagstart);      /* Pointer to start of structure */
      _memory.DmemSetLong(romtagstart + 26); /* Pointer to end of code */
      _memory.DmemSetByte(0x81);             /* Flags, AUTOINIT+COLDSTART */
      _memory.DmemSetByte(0x1);              /* Version */
      _memory.DmemSetByte(3);                /* DEVICE */
      _memory.DmemSetByte(0);                /* Priority */
      _memory.DmemSetLong(_devicename);      /* Pointer to name (used in opendev)*/
      _memory.DmemSetLong(idstr);            /* ID string */
      _memory.DmemSetLong(initstruct);       /* Init_struct */

      _endOfDmem = _memory.DmemGetCounterWithoutOffset();

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
      _rom[0x1092] = static_cast<uint8_t>(_bootcode >> 24);
      _rom[0x1093] = static_cast<uint8_t>(_bootcode >> 16);
      _rom[0x1094] = static_cast<uint8_t>(_bootcode >> 8);
      _rom[0x1095] = static_cast<uint8_t>(_bootcode);

      /* NULLIFY pointer to configdev */

      _memory.DmemSetLongNoCounter(0, 4092);
      _memory.EmemCardAdd(HardfileHandler_CardInit, HardfileHandler_CardMap);
    }
    else
    {
      _memory.DmemClear();
    }
  }

  void HardfileHandler::CreateDOSDevPackets(uint32_t devicename)
  {
    _memory.DmemSetCounter(_endOfDmem);

    /* Device name as seen in Amiga DOS */

    for (auto &mountListEntry : _mountList)
    {
      mountListEntry->NameAddress = _memory.DmemGetCounter();
      _memory.DmemSetString(mountListEntry->Name.c_str());
    }

    _dosDevPacketListStart = _memory.DmemGetCounter();

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
    _memory.DmemSetLong(static_cast<uint32_t>(-1)); // Terminate list
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

  bool HardfileHandler::Create(const HardfileConfiguration &configuration, uint32_t size)
  {
    bool result = false;

#ifdef WIN32
    HANDLE hf;

    if (!configuration.Filename.empty() && size != 0)
    {
      if ((hf = CreateFile(configuration.Filename.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr)) !=
          INVALID_HANDLE_VALUE)
      {
        LONG high = 0;
        if (SetFilePointer(hf, size, &high, FILE_BEGIN) == size)
          result = SetEndOfFile(hf) == TRUE;
        else
          _log.AddLog("SetFilePointer() failure.\n");
        CloseHandle(hf);
      }
      else
        _log.AddLog("CreateFile() failed.\n");
    }
    return result;
#else /* os independent implementation */
#define BUFSIZE 32768
    unsigned int tobewritten;
    char buffer[BUFSIZE];
    FILE *hf;

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
            _log.AddLog("Creating hardfile failed. Check the available space.\n");
            fclose(hf);
            return result;
          }
          tobewritten -= BUFSIZE;
        }
        fwrite(buffer, sizeof(char), tobewritten, hf);
        if (errno != 0)
        {
          _log.AddLog("Creating hardfile failed. Check the available space.\n");
          fclose(hf);
          return result;
        }
        fclose(hf);
        result = true;
      }
      else
        _log.AddLog("fhfileCreate is unable to open output file.\n");
    }
    return result;
#endif
  }

  rdb_status HardfileHandler::HasRDB(const std::string &filename)
  {
    rdb_status result = rdb_status::RDB_NOT_FOUND;
    FILE *F = nullptr;
    fopen_s(&F, filename.c_str(), "rb");
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
    FILE *F = nullptr;
    fopen_s(&F, filename.c_str(), "rb");
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

  HardfileHandler::HardfileHandler(IMemorySystem &memory, IM68K &cpu, ILog &log) : _memory(memory), _cpu(cpu), _log(log)
  {
  }

  HardfileHandler::~HardfileHandler() = default;
}
