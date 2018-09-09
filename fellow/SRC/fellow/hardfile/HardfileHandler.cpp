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
#include "fellow/hardfile/HardfileHandler.h"
#include "fellow/hardfile/rdb/RDBHandler.h"
#include "fellow/hardfile/hunks/HunkRelocator.h"

using namespace fellow::hardfile::rdb;
using namespace fellow::api::service;
using namespace fellow::api::module;
using namespace fellow::api;
using namespace std;

namespace fellow::api::module
{
  IHardfileHandler *HardfileHandler = new fellow::hardfile::HardfileHandler();
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

    void HardfileHandler_CardMap(ULO mapping)
    {
      fellow::api::module::HardfileHandler->CardMap(mapping);
    }

    UBY HardfileHandler_ReadByte(ULO address)
    {
      return fellow::api::module::HardfileHandler->ReadByte(address);
    }

    UWO HardfileHandler_ReadWord(ULO address)
    {
      return fellow::api::module::HardfileHandler->ReadWord(address);
    }

    ULO HardfileHandler_ReadLong(ULO address)
    {
      return fellow::api::module::HardfileHandler->ReadLong(address);
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

    void HardfileHandler::CardMap(ULO mapping)
    {
      _romstart = (mapping << 8) & 0xff0000;
      ULO bank = _romstart >> 16;
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
      for (ULO i = 0; i < FHFILE_MAX_DEVICES; i++)
      {
        if (_devices[i].F != nullptr)
        {
          return false;
        }
      }
      return true;
    }

    bool HardfileHandler::PreferredNameExists(const string& preferredName)
    {
      return std::any_of(_mountList.begin(), _mountList.end(), [preferredName](auto& mountListEntry) {return preferredName == mountListEntry->Name;});
    }

    string HardfileHandler::MakeDeviceName(int no)
    {
      ostringstream o;
      o << "DH" << no;
      return o.str();
    }

    string HardfileHandler::MakeDeviceName(const string& preferredName, int no)
    {
      if (!PreferredNameExists(preferredName))
      {
        return preferredName;
      }
      return MakeDeviceName(no);
    }

    void HardfileHandler::CreateMountList()
    {
      int totalPartitionCount = 0;

      _mountList.clear();

      for (int deviceIndex = 0; deviceIndex < FHFILE_MAX_DEVICES; deviceIndex++)
      {
        if (_devices[deviceIndex].F != nullptr)
        {
          if (_devices[deviceIndex].hasRDB)
          {
            RDB *rdbHeader = _devices[deviceIndex].rdb;

            for (ULO partitionIndex = 0; partitionIndex < rdbHeader->Partitions.size(); partitionIndex++)
            {
              RDBPartition* rdbPartition = rdbHeader->Partitions[partitionIndex].get();
              if (rdbPartition->IsAutomountable())
              {
                _mountList.push_back(unique_ptr<HardfileMountListEntry>(new HardfileMountListEntry(deviceIndex, partitionIndex, MakeDeviceName(rdbPartition->DriveName, totalPartitionCount++))));
              }
            }
          }
          else
          {
            _mountList.push_back(unique_ptr<HardfileMountListEntry>(new HardfileMountListEntry(deviceIndex, -1, MakeDeviceName(totalPartitionCount++))));
          }
        }
      }
    }

    int HardfileHandler::FindOlderOrSameFileSystemVersion(ULO DOSType, ULO version)
    {
      ULO size = (ULO) _fileSystems.size();
      for (ULO index = 0; index < size; index++)
      {
        if (_fileSystems[index]->IsOlderOrSameFileSystemVersion(DOSType, version))
        {
          return index;
        }
      }
      return -1;
    }

    HardfileFileSystemEntry *HardfileHandler::GetFileSystemForDOSType(ULO DOSType)
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
      if (device.F == nullptr || !device.hasRDB)
      {
        return;
      }

      for (auto& header : device.rdb->FileSystemHeaders)
      {
        int olderVersionIndex = FindOlderOrSameFileSystemVersion(header->DOSType, header->Version);
        if (olderVersionIndex == -1)
        {
          _fileSystems.push_back(unique_ptr<HardfileFileSystemEntry>(new HardfileFileSystemEntry(header.get(), 0)));
        }
        else if (_fileSystems[olderVersionIndex]->IsOlderVersion(header->Version))
        {
          // Replace older fs version with this one
          _fileSystems[olderVersionIndex].reset(new HardfileFileSystemEntry(header.get(), 0));
        }
        // Ignore if newer or same fs version already added
      }
    }

    void HardfileHandler::AddFileSystemsFromRdb()
    {
      for (int i = 0; i < FHFILE_MAX_DEVICES; i++)
      {
        AddFileSystemsFromRdb(_devices[i]);
      }
    }

    void HardfileHandler::EraseOlderOrSameFileSystemVersion(ULO DOSType, ULO version)
    {
      int olderOrSameVersionIndex = FindOlderOrSameFileSystemVersion(DOSType, version);
      if (olderOrSameVersionIndex != -1)
      {
        Service->Log.AddLog("fhfile: Erased RDB filesystem entry (%.8X, %.8X), newer version (%.8X, %.8X) found in RDB or newer/same version supported by Kickstart.\n",
          _fileSystems[olderOrSameVersionIndex]->GetDOSType(), _fileSystems[olderOrSameVersionIndex]->GetVersion(), DOSType, version);

        _fileSystems.erase(_fileSystems.begin() + olderOrSameVersionIndex);
      }
    }

    void HardfileHandler::SetPhysicalGeometryFromRDB(HardfileDevice *fhfile)
    {
      fhfile->bytespersector = fhfile->rdb->BlockSize;
      fhfile->bytespersector_original = fhfile->rdb->BlockSize;
      fhfile->sectorspertrack = fhfile->rdb->SectorsPerTrack * fhfile->rdb->Heads;
      fhfile->surfaces = fhfile->rdb->Heads;
      fhfile->tracks = fhfile->rdb->Cylinders;
      fhfile->reservedblocks = 1;
      fhfile->reservedblocks_original = 1;
      fhfile->lowCylinder = fhfile->rdb->LowCylinder;
      fhfile->highCylinder = fhfile->rdb->HighCylinder;
    }

    void HardfileHandler::InitializeHardfile(ULO index)
    {
      fs_wrapper_point *fsnp;

      if (_devices[index].F != nullptr)                     /* Close old hardfile */
      {
        fclose(_devices[index].F);
      }

      _devices[index].F = nullptr;                           /* Set config values */
      _devices[index].status = FHFILE_NONE;
  
      if ((fsnp = Service->FSWrapper.MakePoint(_devices[index].filename)) != nullptr)
      {
        _devices[index].readonly |= (!fsnp->writeable);
        ULO size = fsnp->size;
        fopen_s(&_devices[index].F, _devices[index].filename, _devices[index].readonly ? "rb" : "r+b");

        if (_devices[index].F != nullptr)                          /* Open file */
        {
          RDBFileReader reader(_devices[index].F);
          _devices[index].hasRDB = RDBHandler::HasRigidDiskBlock(reader);

          if (_devices[index].hasRDB)
          {
            // RDB configured hardfile
            _devices[index].rdb = RDBHandler::GetDriveInformation(reader);
            SetPhysicalGeometryFromRDB(&_devices[index]);
            _devices[index].size = size;
            _devices[index].status = FHFILE_HDF;
          }
          else
          {
            // Manually configured hardfile
            ULO track_size = (_devices[index].sectorspertrack * _devices[index].surfaces * _devices[index].bytespersector);
            if (size < track_size)
            {
              /* Error: File must be at least one track long */
              fclose(_devices[index].F);
              _devices[index].F = nullptr;
              _devices[index].status = FHFILE_NONE;
            }
            else                                                    /* File is OK */
            {
              _devices[index].tracks = size / track_size;
              _devices[index].size = _devices[index].tracks * track_size;
              _devices[index].status = FHFILE_HDF;
            }
          }
        }
        delete fsnp;
      }
    }

    /* Returns true if a hardfile was inserted */

    bool HardfileHandler::RemoveHardfile(unsigned int index)
    {
      bool result = false;
      if (index >= GetMaxHardfileCount())
      {
        return result;
      }
      if (_devices[index].F != nullptr)
      {
        fflush(_devices[index].F);
        fclose(_devices[index].F);
        result = true;
      }
      if (_devices[index].hasRDB)
      {
        delete _devices[index].rdb;
        _devices[index].rdb = nullptr;
        _devices[index].hasRDB = false;
      }
      memset(&(_devices[index]), 0, sizeof(HardfileDevice));
      _devices[index].status = FHFILE_NONE;
      return result;
    }

    void HardfileHandler::SetEnabled(bool enabled)
    {
      _enabled = enabled;
    }

    bool HardfileHandler::GetEnabled()
    {
      return _enabled;
    }

    void HardfileHandler::SetHardfile(fellow::api::module::HardfileDevice hardfile, ULO index)
    {
      if (index >= FHFILE_MAX_DEVICES)
      {
        return;
      }
      RemoveHardfile(index);
      strncpy_s(_devices[index].filename, hardfile.filename, CFG_FILENAME_LENGTH);
      _devices[index].readonly = hardfile.readonly_original;
      _devices[index].readonly_original = hardfile.readonly_original;
      _devices[index].bytespersector = (hardfile.bytespersector_original & 0xfffffffc);
      _devices[index].bytespersector_original = hardfile.bytespersector_original;
      _devices[index].sectorspertrack = hardfile.sectorspertrack;
      _devices[index].surfaces = hardfile.surfaces;
      _devices[index].reservedblocks = hardfile.reservedblocks_original;
      _devices[index].reservedblocks_original = hardfile.reservedblocks_original;
      if (_devices[index].reservedblocks < 1)
      {
        _devices[index].reservedblocks = 1;
      }
      InitializeHardfile(index);

      Service->RP.SendHardDriveContent(index, hardfile.filename, hardfile.readonly_original ? true : false);
    }

    bool HardfileHandler::CompareHardfile(fellow::api::module::HardfileDevice hardfile, ULO index)
    {
      if (index >= FHFILE_MAX_DEVICES)
      {
        return false;
      }
      return (_devices[index].readonly_original == hardfile.readonly_original) &&
        (_devices[index].bytespersector_original == hardfile.bytespersector_original) &&
        (_devices[index].sectorspertrack == hardfile.sectorspertrack) &&
        (_devices[index].surfaces == hardfile.surfaces) &&
        (_devices[index].reservedblocks_original == hardfile.reservedblocks_original) &&
        (strncmp(_devices[index].filename, hardfile.filename, CFG_FILENAME_LENGTH) == 0);
    }

    void HardfileHandler::Clear()
    {
      for (ULO i = 0; i < FHFILE_MAX_DEVICES; i++)
      {
        RemoveHardfile(i);
      }
      _fileSystems.clear();
      _mountList.clear();
    }

    /*==================*/
    /* BeginIO Commands */
    /*==================*/

    void HardfileHandler::Ignore(ULO index)
    {
      VM->Memory.WriteLong(0, VM->CPU.GetAReg(1) + 32);
      VM->CPU.SetDReg(0, 0);
    }

    BYT HardfileHandler::Read(ULO index)
    {
      ULO dest = VM->Memory.ReadLong(VM->CPU.GetAReg(1) + 40);
      ULO offset = VM->Memory.ReadLong(VM->CPU.GetAReg(1) + 44);
      ULO length = VM->Memory.ReadLong(VM->CPU.GetAReg(1) + 36);

      if ((offset + length) > _devices[index].size)
      {
        return -3;
      }
  
      Service->HUD.SetHarddiskLED(index, true, false);

      fseek(_devices[index].F, offset, SEEK_SET);
      fread(VM->Memory.AddressToPtr(dest), 1, length, _devices[index].F);
      VM->Memory.WriteLong(length, VM->CPU.GetAReg(1) + 32);

      Service->HUD.SetHarddiskLED(index, false, false);

      return 0;
    }

    BYT HardfileHandler::Write(ULO index)
    {
      ULO dest = VM->Memory.ReadLong(VM->CPU.GetAReg(1) + 40);
      ULO offset = VM->Memory.ReadLong(VM->CPU.GetAReg(1) + 44);
      ULO length = VM->Memory.ReadLong(VM->CPU.GetAReg(1) + 36);

      if (_devices[index].readonly || (offset + length) > _devices[index].size)
      {
        return -3;
      }

      Service->HUD.SetHarddiskLED(index, true, true);

      fseek(_devices[index].F, offset, SEEK_SET);
      fwrite(VM->Memory.AddressToPtr(dest), 1, length, _devices[index].F);
      VM->Memory.WriteLong(length, VM->CPU.GetAReg(1) + 32);

      Service->HUD.SetHarddiskLED(index, false, true);

      return 0;
    }

    void HardfileHandler::GetNumberOfTracks(ULO index)
    {
      VM->Memory.WriteLong(_devices[index].tracks, VM->CPU.GetAReg(1) + 32);
    }

    void HardfileHandler::GetDriveType(ULO index)
    {
      VM->Memory.WriteLong(1, VM->CPU.GetAReg(1) + 32);
    }

    void HardfileHandler::WriteProt(ULO index)
    {
      VM->Memory.WriteLong(_devices[index].readonly, VM->CPU.GetAReg(1) + 32);
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
      _configdev = VM->CPU.GetAReg(3);
      VM->Memory.DmemSetLongNoCounter(FHFILE_MAX_DEVICES, 4088);
      VM->Memory.DmemSetLongNoCounter(_configdev, 4092);
      VM->CPU.SetDReg(0, 1);
    }

    /*======================================*/
    /* Native callbacks for device commands */
    /*======================================*/

    void HardfileHandler::DoOpen()
    {
      if (VM->CPU.GetDReg(0) < FHFILE_MAX_DEVICES)
      {
        VM->Memory.WriteByte(7, VM->CPU.GetAReg(1) + 8);                     /* ln_type (NT_REPLYMSG) */
        VM->Memory.WriteByte(0, VM->CPU.GetAReg(1) + 31);                    /* io_error */
        VM->Memory.WriteLong(VM->CPU.GetDReg(0), VM->CPU.GetAReg(1) + 24); /* io_unit */
        VM->Memory.WriteLong(VM->Memory.ReadLong(VM->CPU.GetAReg(6) + 32) + 1, VM->CPU.GetAReg(6) + 32);  /* LIB_OPENCNT */
        VM->CPU.SetDReg(0, 0);                                                 /* ? */
      }
      else
      {
        VM->Memory.WriteLong(static_cast<ULO>(-1), VM->CPU.GetAReg(1) + 20);
        VM->Memory.WriteByte(static_cast<UBY>(-1), VM->CPU.GetAReg(1) + 31); /* io_error */
        VM->CPU.SetDReg(0, static_cast<ULO>(-1));                              /* ? */
      }
    }

    void HardfileHandler::DoClose()
    {
      VM->Memory.WriteLong(VM->Memory.ReadLong(VM->CPU.GetAReg(6) + 32) - 1, VM->CPU.GetAReg(6) + 32); /* LIB_OPENCNT */
      VM->CPU.SetDReg(0, 0);                                                                                 /* ? */
    }

    void HardfileHandler::DoExpunge()
    {
      VM->CPU.SetDReg(0, 0); /* ? */
    }

    void HardfileHandler::DoNULL()
    {
      // Deliberately left empty
    }

    void HardfileHandler::DoBeginIO()
    {
      BYT error = 0;
      ULO unit = VM->Memory.ReadLong(VM->CPU.GetAReg(1) + 24);

      UWO cmd = VM->Memory.ReadWord(VM->CPU.GetAReg(1) + 28);
      switch (cmd)
      {
        case 2:
          error = Read(unit);
          break;
        case 3:
        case 11:
          error = Write(unit);
          break;
        case 18:
          GetDriveType(unit);
          break;
        case 19:
          GetNumberOfTracks(unit);
          break;
        case 15:
          WriteProt(unit);
          break;
        case 4:
        case 5:
        case 9:
        case 10:
        case 12:
        case 13:
        case 14:
        case 20:
        case 21:
          Ignore(unit);
          break;
        default:
          error = -3;
          VM->CPU.SetDReg(0, 0);
          break;
      }
      VM->Memory.WriteByte(5, VM->CPU.GetAReg(1) + 8);      /* ln_type */
      VM->Memory.WriteByte(error, VM->CPU.GetAReg(1) + 31); /* ln_error */
    }

    void HardfileHandler::DoAbortIO()
    {
      VM->CPU.SetDReg(0, static_cast<ULO>(-3));
    }

    // RDB support functions, native callbacks

    ULO HardfileHandler::DoGetRDBFileSystemCount()
    {
      ULO count = (ULO) _fileSystems.size();
      Service->Log.AddLog("fhfile: DoGetRDBFilesystemCount() - Returns %u\n", count);
      return count;
    }

    ULO HardfileHandler::DoGetRDBFileSystemHunkCount(ULO fileSystemIndex)
    {
      ULO hunkCount = (ULO) _fileSystems[fileSystemIndex]->Header->FileSystemHandler.FileImage.GetInitialHunkCount();
      Service->Log.AddLog("fhfile: DoGetRDBFileSystemHunkCount(fileSystemIndex: %u) Returns %u\n", fileSystemIndex, hunkCount);
      return hunkCount;
    }

    ULO HardfileHandler::DoGetRDBFileSystemHunkSize(ULO fileSystemIndex, ULO hunkIndex)
    {
      ULO hunkSize = _fileSystems[fileSystemIndex]->Header->FileSystemHandler.FileImage.GetInitialHunk(hunkIndex)->GetAllocateSizeInBytes();
      Service->Log.AddLog("fhfile: DoGetRDBFileSystemHunkSize(fileSystemIndex: %u, hunkIndex: %u) Returns %u\n", fileSystemIndex, hunkIndex, hunkSize);
      return hunkSize;
    }

    void HardfileHandler::DoCopyRDBFileSystemHunk(ULO destinationAddress, ULO fileSystemIndex, ULO hunkIndex)
    {
      Service->Log.AddLog("fhfile: DoCopyRDBFileSystemHunk(destinationAddress: %.8X, fileSystemIndex: %u, hunkIndex: %u)\n", destinationAddress, fileSystemIndex, hunkIndex);

      HardfileFileSystemEntry* fileSystem = _fileSystems[fileSystemIndex].get();
      fileSystem->CopyHunkToAddress(destinationAddress + 8, hunkIndex);

      // Remember the address to the first hunk
      if (fileSystem->SegListAddress == 0)
      {
        fileSystem->SegListAddress = destinationAddress + 4;
      }

      ULO hunkSize = fileSystem->Header->FileSystemHandler.FileImage.GetInitialHunk(hunkIndex)->GetAllocateSizeInBytes();
      VM->Memory.WriteLong(hunkSize + 8, destinationAddress); // Total size of allocation
      VM->Memory.WriteLong(0, destinationAddress + 4);  // No next segment for now
    }

    void HardfileHandler::DoRelocateFileSystem(ULO fileSystemIndex)
    {
      Service->Log.AddLog("fhfile: DoRelocateFileSystem(fileSystemIndex: %u\n", fileSystemIndex);
      HardfileFileSystemEntry* fsEntry = _fileSystems[fileSystemIndex].get();
      fellow::hardfile::hunks::HunkRelocator hunkRelocator(fsEntry->Header->FileSystemHandler.FileImage);
      hunkRelocator.RelocateHunks();
    }

    void HardfileHandler::DoInitializeRDBFileSystemEntry(ULO fileSystemEntry, ULO fileSystemIndex)
    {
      Service->Log.AddLog("fhfile: DoInitializeRDBFileSystemEntry(fileSystemEntry: %.8X, fileSystemIndex: %u\n", fileSystemEntry, fileSystemIndex);

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
      VM->Memory.WriteLong(fsEntry->SegListAddress>>2, fileSystemEntry + 54);
    //  VM->Memory.WriteLong(fsHeader.DnSegListBlock, fileSystemEntry + 54);
      VM->Memory.WriteLong(fsHeader->DnGlobalVec, fileSystemEntry + 58);

      for (int i = 0; i < 23; i++)
      {
        VM->Memory.WriteLong(fsHeader->Reserved2[i], fileSystemEntry + 62 + i*4);
      }
    }

    string HardfileHandler::LogGetStringFromMemory(ULO address)
    {
      string name;
      if (address == 0)
      {
        return string();
      }
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
      Service->Log.AddLog("fhfile: DoLogAvailableResources()\n");

      ULO execBase = VM->Memory.ReadLong(4);  // Fetch list from exec
      ULO rsListHeader = VM->Memory.ReadLong(execBase + 0x150);

      Service->Log.AddLog("fhfile: Resource list header (%.8X): Head %.8X Tail %.8X TailPred %.8X Type %d\n", 
        rsListHeader, 
        VM->Memory.ReadLong(rsListHeader), 
        VM->Memory.ReadLong(rsListHeader + 4),
        VM->Memory.ReadLong(rsListHeader + 8), 
        VM->Memory.ReadByte(rsListHeader + 9));

      if (rsListHeader == VM->Memory.ReadLong(rsListHeader + 8))
      {
        Service->Log.AddLog("fhfile: Resource list is empty.\n");
        return;
      }

      ULO fsNode = VM->Memory.ReadLong(rsListHeader);
      while (fsNode != 0 && (fsNode != rsListHeader + 4))
      {
        Service->Log.AddLog("fhfile: ResourceEntry Node (%.8X): Succ %.8X Pred %.8X Type %d Pri %d NodeName '%s'\n", 
          fsNode, 
          VM->Memory.ReadLong(fsNode), 
          VM->Memory.ReadLong(fsNode + 4),
          VM->Memory.ReadByte(fsNode + 8), 
          VM->Memory.ReadByte(fsNode + 9), 
          LogGetStringFromMemory(VM->Memory.ReadLong(fsNode + 10)).c_str());

        fsNode = VM->Memory.ReadLong(fsNode);
      }
    }

    void HardfileHandler::DoLogAllocMemResult(ULO result)
    {
      Service->Log.AddLog("fhfile: AllocMem() returned %.8X\n", result);
    }

    void HardfileHandler::DoLogOpenResourceResult(ULO result)
    {
      Service->Log.AddLog("fhfile: OpenResource() returned %.8X\n", result);
    }

    void HardfileHandler::DoRemoveRDBFileSystemsAlreadySupportedBySystem(ULO filesystemResource)
    {
      Service->Log.AddLog("fhfile: DoRemoveRDBFileSystemsAlreadySupportedBySystem(filesystemResource: %.8X)\n", filesystemResource);

      ULO fsList = filesystemResource + 18;
      if (fsList == VM->Memory.ReadLong(fsList + 8))
      {
        Service->Log.AddLog("fhfile: FileSystemEntry list is empty.\n");
        return;
      }

      ULO fsNode = VM->Memory.ReadLong(fsList);
      while (fsNode != 0 && (fsNode != fsList + 4))
      {
        ULO fsEntry = fsNode + 14;

        ULO dosType = VM->Memory.ReadLong(fsEntry);
        ULO version = VM->Memory.ReadLong(fsEntry + 4);

        Service->Log.AddLog("fhfile: FileSystemEntry DosType   : %.8X\n", VM->Memory.ReadLong(fsEntry));
        Service->Log.AddLog("fhfile: FileSystemEntry Version   : %.8X\n", VM->Memory.ReadLong(fsEntry + 4));
        Service->Log.AddLog("fhfile: FileSystemEntry PatchFlags: %.8X\n", VM->Memory.ReadLong(fsEntry + 8));
        Service->Log.AddLog("fhfile: FileSystemEntry Type      : %.8X\n", VM->Memory.ReadLong(fsEntry + 12));
        Service->Log.AddLog("fhfile: FileSystemEntry Task      : %.8X\n", VM->Memory.ReadLong(fsEntry + 16));
        Service->Log.AddLog("fhfile: FileSystemEntry Lock      : %.8X\n", VM->Memory.ReadLong(fsEntry + 20));
        Service->Log.AddLog("fhfile: FileSystemEntry Handler   : %.8X\n", VM->Memory.ReadLong(fsEntry + 24));
        Service->Log.AddLog("fhfile: FileSystemEntry StackSize : %.8X\n", VM->Memory.ReadLong(fsEntry + 28));
        Service->Log.AddLog("fhfile: FileSystemEntry Priority  : %.8X\n", VM->Memory.ReadLong(fsEntry + 32));
        Service->Log.AddLog("fhfile: FileSystemEntry Startup   : %.8X\n", VM->Memory.ReadLong(fsEntry + 36));
        Service->Log.AddLog("fhfile: FileSystemEntry SegList   : %.8X\n", VM->Memory.ReadLong(fsEntry + 40));
        Service->Log.AddLog("fhfile: FileSystemEntry GlobalVec : %.8X\n\n", VM->Memory.ReadLong(fsEntry + 44));
        fsNode = VM->Memory.ReadLong(fsNode);

        EraseOlderOrSameFileSystemVersion(dosType, version);    
      }
    }

    // D0 - pointer to FileSystem.resource
    void HardfileHandler::DoLogAvailableFileSystems(ULO fileSystemResource)
    {
      Service->Log.AddLog("fhfile: DoLogAvailableFileSystems(fileSystemResource: %.8X)\n", fileSystemResource);

      ULO fsList = fileSystemResource + 18;
      if (fsList == VM->Memory.ReadLong(fsList + 8))
      {
        Service->Log.AddLog("fhfile: FileSystemEntry list is empty.\n");
        return;
      }

      ULO fsNode = VM->Memory.ReadLong(fsList);
      while (fsNode != 0 && (fsNode != fsList + 4))
      {
        ULO fsEntry = fsNode + 14;

        ULO dosType = VM->Memory.ReadLong(fsEntry);
        ULO version = VM->Memory.ReadLong(fsEntry + 4);

        Service->Log.AddLog("fhfile: FileSystemEntry DosType   : %.8X\n", dosType);
        Service->Log.AddLog("fhfile: FileSystemEntry Version   : %.8X\n", version);
        Service->Log.AddLog("fhfile: FileSystemEntry PatchFlags: %.8X\n", VM->Memory.ReadLong(fsEntry + 8));
        Service->Log.AddLog("fhfile: FileSystemEntry Type      : %.8X\n", VM->Memory.ReadLong(fsEntry + 12));
        Service->Log.AddLog("fhfile: FileSystemEntry Task      : %.8X\n", VM->Memory.ReadLong(fsEntry + 16));
        Service->Log.AddLog("fhfile: FileSystemEntry Lock      : %.8X\n", VM->Memory.ReadLong(fsEntry + 20));
        Service->Log.AddLog("fhfile: FileSystemEntry Handler   : %.8X\n", VM->Memory.ReadLong(fsEntry + 24));
        Service->Log.AddLog("fhfile: FileSystemEntry StackSize : %.8X\n", VM->Memory.ReadLong(fsEntry + 28));
        Service->Log.AddLog("fhfile: FileSystemEntry Priority  : %.8X\n", VM->Memory.ReadLong(fsEntry + 32));
        Service->Log.AddLog("fhfile: FileSystemEntry Startup   : %.8X\n", VM->Memory.ReadLong(fsEntry + 36));
        Service->Log.AddLog("fhfile: FileSystemEntry SegList   : %.8X\n", VM->Memory.ReadLong(fsEntry + 40));
        Service->Log.AddLog("fhfile: FileSystemEntry GlobalVec : %.8X\n\n", VM->Memory.ReadLong(fsEntry + 44));
        fsNode = VM->Memory.ReadLong(fsNode);
      }
    }

    void HardfileHandler::DoPatchDOSDeviceNode(ULO node, ULO packet)
    {
      Service->Log.AddLog("fhfile: DoPatchDOSDeviceNode(node: %.8X, packet: %.8X)\n", node, packet);

      VM->Memory.WriteLong(0, node + 8); // dn_Task = 0
      VM->Memory.WriteLong(0, node + 16); // dn_Handler = 0
      VM->Memory.WriteLong(static_cast<ULO>(-1), node + 36); // dn_GlobalVec = -1

      HardfileFileSystemEntry *fs = GetFileSystemForDOSType(VM->Memory.ReadLong(packet + 80));
      if (fs != nullptr)
      {
        if (fs->Header->PatchFlags & 0x10)
        {
          VM->Memory.WriteLong(fs->Header->DnStackSize, node + 20);
        }
        if (fs->Header->PatchFlags & 0x80)
        {
          VM->Memory.WriteLong(fs->SegListAddress>>2, node + 32);
        }
        if (fs->Header->PatchFlags & 0x100)
        {
          VM->Memory.WriteLong(fs->Header->DnGlobalVec, node + 36);
        }
      }  
    }

    void HardfileHandler::DoUnknownOperation(ULO operation)
    {
      Service->Log.AddLog("fhfile: Unknown operation called %X\n", operation);
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

    void HardfileHandler::MakeDOSDevPacketForPlainHardfile(const HardfileMountListEntry& mountListEntry, ULO deviceNameAddress)
    {
      const HardfileDevice& device = _devices[mountListEntry.DeviceIndex];
      if (device.F != nullptr)
      {
        VM->Memory.DmemSetLong(mountListEntry.DeviceIndex);            /* Flag to initcode */

        VM->Memory.DmemSetLong(mountListEntry.NameAddress);            /*  0 Unit name "FELLOWX" or similar */
        VM->Memory.DmemSetLong(deviceNameAddress);                     /*  4 Device name "fhfile.device" */
        // Might need to be partition index?
        VM->Memory.DmemSetLong(mountListEntry.DeviceIndex);            /*  8 Unit # */
        VM->Memory.DmemSetLong(0);                                     /* 12 OpenDevice flags */

        // Struct DosEnvec
        VM->Memory.DmemSetLong(16);                                    /* 16 Environment size in long words*/
        VM->Memory.DmemSetLong(device.bytespersector >> 2);            /* 20 Longwords in a block */
        VM->Memory.DmemSetLong(0);                                     /* 24 sector origin (unused) */
        VM->Memory.DmemSetLong(device.surfaces);                       /* 28 Heads */
        VM->Memory.DmemSetLong(1);                                     /* 32 Sectors per logical block (unused) */
        VM->Memory.DmemSetLong(device.sectorspertrack);                /* 36 Sectors per track */
        VM->Memory.DmemSetLong(device.reservedblocks);                 /* 40 Reserved blocks, min. 1 */
        VM->Memory.DmemSetLong(0);                                     /* 44 mdn_prefac - Unused */
        VM->Memory.DmemSetLong(0);                                     /* 48 Interleave */
        VM->Memory.DmemSetLong(0);                                     /* 52 Lower cylinder */
        VM->Memory.DmemSetLong(device.tracks - 1);                     /* 56 Upper cylinder */
        VM->Memory.DmemSetLong(0);                                     /* 60 Number of buffers */
        VM->Memory.DmemSetLong(0);                                     /* 64 Type of memory for buffers */
        VM->Memory.DmemSetLong(0x7fffffff);                            /* 68 Largest transfer */
        VM->Memory.DmemSetLong(~1U);                                   /* 72 Add mask */
        VM->Memory.DmemSetLong(static_cast<ULO>(-1));                  /* 76 Boot priority */
        VM->Memory.DmemSetLong(0x444f5300);                            /* 80 DOS file handler name */
        VM->Memory.DmemSetLong(0);
      }
    }

    void HardfileHandler::MakeDOSDevPacketForRDBPartition(const HardfileMountListEntry& mountListEntry, ULO deviceNameAddress)
    {
      HardfileDevice& device = _devices[mountListEntry.DeviceIndex];
      RDBPartition* partition = device.rdb->Partitions[mountListEntry.PartitionIndex].get();
      if (device.F != nullptr)
      {
        VM->Memory.DmemSetLong(mountListEntry.DeviceIndex);            /* Flag to initcode */

        VM->Memory.DmemSetLong(mountListEntry.NameAddress);            /*  0 Unit name "FELLOWX" or similar */
        VM->Memory.DmemSetLong(deviceNameAddress);                     /*  4 Device name "fhfile.device" */
        VM->Memory.DmemSetLong(mountListEntry.DeviceIndex);            /*  8 Unit # */
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

    /*===========================================================*/
    /* fhfileHardReset                                           */
    /* This will set up the device structures and stubs          */
    /* Can be called at every reset, but really only needed once */
    /*===========================================================*/

    void HardfileHandler::HardReset()
    {
      _fileSystems.clear();
      CreateMountList();

      if (!HasZeroDevices() && GetEnabled() && VM->Memory.GetKickImageVersion() >= 34)
      {
        VM->Memory.DmemSetCounter(0);

        /* Device-name and ID string */

        ULO devicename = VM->Memory.DmemGetCounter();
        VM->Memory.DmemSetString("fhfile.device");
        ULO idstr = VM->Memory.DmemGetCounter();
        VM->Memory.DmemSetString("Fellow Hardfile device V5");
        ULO doslibname = VM->Memory.DmemGetCounter();
        VM->Memory.DmemSetString("dos.library");
        _fsname = VM->Memory.DmemGetCounter();
        VM->Memory.DmemSetString("Fellow hardfile RDB fs");

        /* Device name as seen in Amiga DOS */

        for (auto& mountListEntry : _mountList)
        {

          mountListEntry->NameAddress = VM->Memory.DmemGetCounter();
          VM->Memory.DmemSetString(mountListEntry->Name.c_str());
        }

        /* fhfile.open */

        ULO fhfile_t_open = VM->Memory.DmemGetCounter();
        VM->Memory.DmemSetWord(0x23fc);
        VM->Memory.DmemSetLong(0x00010002); VM->Memory.DmemSetLong(0xf40000); /* move.l #$00010002,$f40000 */
        VM->Memory.DmemSetWord(0x4e75);                                         /* rts */

        /* fhfile.close */

        ULO fhfile_t_close = VM->Memory.DmemGetCounter();
        VM->Memory.DmemSetWord(0x23fc);
        VM->Memory.DmemSetLong(0x00010003); VM->Memory.DmemSetLong(0xf40000); /* move.l #$00010003,$f40000 */
        VM->Memory.DmemSetWord(0x4e75);                                         /* rts */

        /* fhfile.expunge */

        ULO fhfile_t_expunge = VM->Memory.DmemGetCounter();
        VM->Memory.DmemSetWord(0x23fc);
        VM->Memory.DmemSetLong(0x00010004); VM->Memory.DmemSetLong(0xf40000); /* move.l #$00010004,$f40000 */
        VM->Memory.DmemSetWord(0x4e75);                                         /* rts */

        /* fhfile.null */

        ULO fhfile_t_null = VM->Memory.DmemGetCounter();
        VM->Memory.DmemSetWord(0x23fc);
        VM->Memory.DmemSetLong(0x00010005); VM->Memory.DmemSetLong(0xf40000); /* move.l #$00010005,$f40000 */
        VM->Memory.DmemSetWord(0x4e75);                                         /* rts */

        /* fhfile.beginio */

        ULO fhfile_t_beginio = VM->Memory.DmemGetCounter();
        VM->Memory.DmemSetWord(0x23fc);
        VM->Memory.DmemSetLong(0x00010006); VM->Memory.DmemSetLong(0xf40000); /* move.l #$00010006,$f40000 */
        VM->Memory.DmemSetLong(0x48e78002);                                     /* movem.l d0/a6,-(a7) */
        VM->Memory.DmemSetLong(0x08290000); VM->Memory.DmemSetWord(0x001e);   /* btst   #$0,30(a1)   */
        VM->Memory.DmemSetWord(0x6608);                                         /* bne    (to rts)     */
        VM->Memory.DmemSetLong(0x2c780004);                                     /* move.l $4.w,a6      */
        VM->Memory.DmemSetLong(0x4eaefe86);                                     /* jsr    -378(a6)     */
        VM->Memory.DmemSetLong(0x4cdf4001);                                     /* movem.l (a7)+,d0/a6 */
        VM->Memory.DmemSetWord(0x4e75);                                         /* rts */

        /* fhfile.abortio */

        ULO fhfile_t_abortio = VM->Memory.DmemGetCounter();
        VM->Memory.DmemSetWord(0x23fc);
        VM->Memory.DmemSetLong(0x00010007); VM->Memory.DmemSetLong(0xf40000); /* move.l #$00010007,$f40000 */
        VM->Memory.DmemSetWord(0x4e75);                                         /* rts */

        /* Func-table */

        ULO functable = VM->Memory.DmemGetCounter();
        VM->Memory.DmemSetLong(fhfile_t_open);
        VM->Memory.DmemSetLong(fhfile_t_close);
        VM->Memory.DmemSetLong(fhfile_t_expunge);
        VM->Memory.DmemSetLong(fhfile_t_null);
        VM->Memory.DmemSetLong(fhfile_t_beginio);
        VM->Memory.DmemSetLong(fhfile_t_abortio);
        VM->Memory.DmemSetLong(0xffffffff);

        /* Data-table */

        ULO datatable = VM->Memory.DmemGetCounter();
        VM->Memory.DmemSetWord(0xE000);          /* INITBYTE */
        VM->Memory.DmemSetWord(0x0008);          /* LN_TYPE */
        VM->Memory.DmemSetWord(0x0300);          /* NT_DEVICE */
        VM->Memory.DmemSetWord(0xC000);          /* INITLONG */
        VM->Memory.DmemSetWord(0x000A);          /* LN_NAME */
        VM->Memory.DmemSetLong(devicename);
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

        ULO fhfile_t_init = VM->Memory.DmemGetCounter();

        VM->Memory.DmemSetByte(0x48); VM->Memory.DmemSetByte(0xE7); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0xFE);
        VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4);
        VM->Memory.DmemSetByte(0x4A); VM->Memory.DmemSetByte(0x80); VM->Memory.DmemSetByte(0x67); VM->Memory.DmemSetByte(0x24);
        VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xD4);
        VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x01); VM->Memory.DmemSetByte(0xA2);
        VM->Memory.DmemSetByte(0x4A); VM->Memory.DmemSetByte(0x80); VM->Memory.DmemSetByte(0x66); VM->Memory.DmemSetByte(0x0C);
        VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x01); VM->Memory.DmemSetByte(0x5C);
        VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x01); VM->Memory.DmemSetByte(0x96);
        VM->Memory.DmemSetByte(0x4A); VM->Memory.DmemSetByte(0x80); VM->Memory.DmemSetByte(0x67); VM->Memory.DmemSetByte(0x0C);
        VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x01); VM->Memory.DmemSetByte(0x10);
        VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x01); VM->Memory.DmemSetByte(0xFC);
        VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xC0);
        VM->Memory.DmemSetByte(0x2C); VM->Memory.DmemSetByte(0x78); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x04);
        VM->Memory.DmemSetByte(0x43); VM->Memory.DmemSetByte(0xFA); VM->Memory.DmemSetByte(0x02); VM->Memory.DmemSetByte(0x12);
        VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0xAE); VM->Memory.DmemSetByte(0xFE); VM->Memory.DmemSetByte(0x68);
        VM->Memory.DmemSetByte(0x28); VM->Memory.DmemSetByte(0x40); VM->Memory.DmemSetByte(0x41); VM->Memory.DmemSetByte(0xFA);
        VM->Memory.DmemSetByte(0x02); VM->Memory.DmemSetByte(0x46); VM->Memory.DmemSetByte(0x2E); VM->Memory.DmemSetByte(0x08);
        VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x47); VM->Memory.DmemSetByte(0x4A); VM->Memory.DmemSetByte(0x90);
        VM->Memory.DmemSetByte(0x6B); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x70);
        VM->Memory.DmemSetByte(0x58); VM->Memory.DmemSetByte(0x87); VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x3C);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x58);
        VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x01); VM->Memory.DmemSetByte(0x04);
        VM->Memory.DmemSetByte(0x2A); VM->Memory.DmemSetByte(0x40); VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x47);
        VM->Memory.DmemSetByte(0x70); VM->Memory.DmemSetByte(0x54); VM->Memory.DmemSetByte(0x2B); VM->Memory.DmemSetByte(0xB0);
        VM->Memory.DmemSetByte(0x08); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x08); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x59); VM->Memory.DmemSetByte(0x80); VM->Memory.DmemSetByte(0x64); VM->Memory.DmemSetByte(0xF6);
        VM->Memory.DmemSetByte(0xCD); VM->Memory.DmemSetByte(0x4C); VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x4D);
        VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0xAE); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x70);
        VM->Memory.DmemSetByte(0xCD); VM->Memory.DmemSetByte(0x4C); VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xCE); VM->Memory.DmemSetByte(0x26); VM->Memory.DmemSetByte(0x40);
        VM->Memory.DmemSetByte(0x70); VM->Memory.DmemSetByte(0x14); VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xDE); VM->Memory.DmemSetByte(0x22); VM->Memory.DmemSetByte(0x47);
        VM->Memory.DmemSetByte(0x2C); VM->Memory.DmemSetByte(0x29); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0xFC);
        VM->Memory.DmemSetByte(0x22); VM->Memory.DmemSetByte(0x40); VM->Memory.DmemSetByte(0x70); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x22); VM->Memory.DmemSetByte(0x80); VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0x40);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x04); VM->Memory.DmemSetByte(0x33); VM->Memory.DmemSetByte(0x40);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x0E); VM->Memory.DmemSetByte(0x33); VM->Memory.DmemSetByte(0x7C);
        VM->Memory.DmemSetByte(0x10); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x08);
        VM->Memory.DmemSetByte(0x9D); VM->Memory.DmemSetByte(0x69); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x08);
        VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0x79); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4);
        VM->Memory.DmemSetByte(0x0F); VM->Memory.DmemSetByte(0xFC); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x0A);
        VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0x4B); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x10);
        VM->Memory.DmemSetByte(0x41); VM->Memory.DmemSetByte(0xEC); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x4A);
        VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0xAE); VM->Memory.DmemSetByte(0xFE); VM->Memory.DmemSetByte(0xF2);
        VM->Memory.DmemSetByte(0x06); VM->Memory.DmemSetByte(0x87); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x58); VM->Memory.DmemSetByte(0x60); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x8C); VM->Memory.DmemSetByte(0x2C); VM->Memory.DmemSetByte(0x78);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x04); VM->Memory.DmemSetByte(0x22); VM->Memory.DmemSetByte(0x4C);
        VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0xAE); VM->Memory.DmemSetByte(0xFE); VM->Memory.DmemSetByte(0x62);
        VM->Memory.DmemSetByte(0x4C); VM->Memory.DmemSetByte(0xDF); VM->Memory.DmemSetByte(0x7F); VM->Memory.DmemSetByte(0xFF);
        VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75); VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xA0);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75); VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xA1);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75); VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xA2);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75); VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xA3);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75); VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x01);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75); VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75); VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x03);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75); VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x04);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75); VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x05);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75); VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x06);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75); VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x07);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75); VM->Memory.DmemSetByte(0x23); VM->Memory.DmemSetByte(0xFC);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x02); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x08);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xF4); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75); VM->Memory.DmemSetByte(0x48); VM->Memory.DmemSetByte(0xE7);
        VM->Memory.DmemSetByte(0x78); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x22); VM->Memory.DmemSetByte(0x3C);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x01); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x01);
        VM->Memory.DmemSetByte(0x2C); VM->Memory.DmemSetByte(0x78); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x04);
        VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0xAE); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x3A);
        VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x5C);
        VM->Memory.DmemSetByte(0x4C); VM->Memory.DmemSetByte(0xDF); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x1E);
        VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75); VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x3C);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x20);
        VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0xDC);
        VM->Memory.DmemSetByte(0x2A); VM->Memory.DmemSetByte(0x40); VM->Memory.DmemSetByte(0x1B); VM->Memory.DmemSetByte(0x7C);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x08); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x08);
        VM->Memory.DmemSetByte(0x41); VM->Memory.DmemSetByte(0xFA); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xD0);
        VM->Memory.DmemSetByte(0x2B); VM->Memory.DmemSetByte(0x48); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x0A);
        VM->Memory.DmemSetByte(0x41); VM->Memory.DmemSetByte(0xFA); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xDC);
        VM->Memory.DmemSetByte(0x2B); VM->Memory.DmemSetByte(0x48); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x0E);
        VM->Memory.DmemSetByte(0x49); VM->Memory.DmemSetByte(0xED); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x12);
        VM->Memory.DmemSetByte(0x28); VM->Memory.DmemSetByte(0x8C); VM->Memory.DmemSetByte(0x06); VM->Memory.DmemSetByte(0x94);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x04);
        VM->Memory.DmemSetByte(0x42); VM->Memory.DmemSetByte(0xAC); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x04);
        VM->Memory.DmemSetByte(0x29); VM->Memory.DmemSetByte(0x4C); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x08);
        VM->Memory.DmemSetByte(0x22); VM->Memory.DmemSetByte(0x4D); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0xAE);
        VM->Memory.DmemSetByte(0xFE); VM->Memory.DmemSetByte(0x1A); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75);
        VM->Memory.DmemSetByte(0x2C); VM->Memory.DmemSetByte(0x78); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x04);
        VM->Memory.DmemSetByte(0x70); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x43); VM->Memory.DmemSetByte(0xFA);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x9E); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0xAE);
        VM->Memory.DmemSetByte(0xFE); VM->Memory.DmemSetByte(0x0E); VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x12); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75);
        VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x3C);
        VM->Memory.DmemSetByte(0x4A); VM->Memory.DmemSetByte(0x80); VM->Memory.DmemSetByte(0x67); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x26); VM->Memory.DmemSetByte(0x28); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x74); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x3A); VM->Memory.DmemSetByte(0x4A); VM->Memory.DmemSetByte(0x80);
        VM->Memory.DmemSetByte(0x67); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x0C);
        VM->Memory.DmemSetByte(0x50); VM->Memory.DmemSetByte(0x80); VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x76); VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x36); VM->Memory.DmemSetByte(0x52); VM->Memory.DmemSetByte(0x82);
        VM->Memory.DmemSetByte(0xB8); VM->Memory.DmemSetByte(0x82); VM->Memory.DmemSetByte(0x66); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0xE6); VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x5A); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75);
        VM->Memory.DmemSetByte(0x2F); VM->Memory.DmemSetByte(0x08); VM->Memory.DmemSetByte(0x2F); VM->Memory.DmemSetByte(0x01);
        VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0xCA);
        VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x3C); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xBE); VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x52); VM->Memory.DmemSetByte(0x22); VM->Memory.DmemSetByte(0x1F);
        VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x1C);
        VM->Memory.DmemSetByte(0x2C); VM->Memory.DmemSetByte(0x79); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x04); VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x57);
        VM->Memory.DmemSetByte(0x41); VM->Memory.DmemSetByte(0xE8); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x12);
        VM->Memory.DmemSetByte(0x22); VM->Memory.DmemSetByte(0x40); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0xAE);
        VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0x10); VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x5F);
        VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75); VM->Memory.DmemSetByte(0x2F); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x40); VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0xFE); VM->Memory.DmemSetByte(0xCE); VM->Memory.DmemSetByte(0x4A); VM->Memory.DmemSetByte(0x80);
        VM->Memory.DmemSetByte(0x67); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x12);
        VM->Memory.DmemSetByte(0x26); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x72); VM->Memory.DmemSetByte(0x00);
        VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0xBE);
        VM->Memory.DmemSetByte(0x52); VM->Memory.DmemSetByte(0x81); VM->Memory.DmemSetByte(0xB6); VM->Memory.DmemSetByte(0x81);
        VM->Memory.DmemSetByte(0x66); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0xFF); VM->Memory.DmemSetByte(0xF6);
        VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x1F); VM->Memory.DmemSetByte(0x4E); VM->Memory.DmemSetByte(0x75);
        VM->Memory.DmemSetByte(0x65); VM->Memory.DmemSetByte(0x78); VM->Memory.DmemSetByte(0x70); VM->Memory.DmemSetByte(0x61);
        VM->Memory.DmemSetByte(0x6E); VM->Memory.DmemSetByte(0x73); VM->Memory.DmemSetByte(0x69); VM->Memory.DmemSetByte(0x6F);
        VM->Memory.DmemSetByte(0x6E); VM->Memory.DmemSetByte(0x2E); VM->Memory.DmemSetByte(0x6C); VM->Memory.DmemSetByte(0x69);
        VM->Memory.DmemSetByte(0x62); VM->Memory.DmemSetByte(0x72); VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x72);
        VM->Memory.DmemSetByte(0x79); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x46); VM->Memory.DmemSetByte(0x69);
        VM->Memory.DmemSetByte(0x6C); VM->Memory.DmemSetByte(0x65); VM->Memory.DmemSetByte(0x53); VM->Memory.DmemSetByte(0x79);
        VM->Memory.DmemSetByte(0x73); VM->Memory.DmemSetByte(0x74); VM->Memory.DmemSetByte(0x65); VM->Memory.DmemSetByte(0x6D);
        VM->Memory.DmemSetByte(0x2E); VM->Memory.DmemSetByte(0x72); VM->Memory.DmemSetByte(0x65); VM->Memory.DmemSetByte(0x73);
        VM->Memory.DmemSetByte(0x6F); VM->Memory.DmemSetByte(0x75); VM->Memory.DmemSetByte(0x72); VM->Memory.DmemSetByte(0x63);
        VM->Memory.DmemSetByte(0x65); VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x46); VM->Memory.DmemSetByte(0x65);
        VM->Memory.DmemSetByte(0x6C); VM->Memory.DmemSetByte(0x6C); VM->Memory.DmemSetByte(0x6F); VM->Memory.DmemSetByte(0x77);
        VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x68); VM->Memory.DmemSetByte(0x61); VM->Memory.DmemSetByte(0x72);
        VM->Memory.DmemSetByte(0x64); VM->Memory.DmemSetByte(0x66); VM->Memory.DmemSetByte(0x69); VM->Memory.DmemSetByte(0x6C);
        VM->Memory.DmemSetByte(0x65); VM->Memory.DmemSetByte(0x20); VM->Memory.DmemSetByte(0x64); VM->Memory.DmemSetByte(0x65);
        VM->Memory.DmemSetByte(0x76); VM->Memory.DmemSetByte(0x69); VM->Memory.DmemSetByte(0x63); VM->Memory.DmemSetByte(0x65);
        VM->Memory.DmemSetByte(0x00); VM->Memory.DmemSetByte(0x00);
        
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
        VM->Memory.DmemSetLong(static_cast<ULO>(-1));  // Terminate list

        /* Init-struct */

        ULO initstruct = VM->Memory.DmemGetCounter();
        VM->Memory.DmemSetLong(0x100);                   /* Data-space size, min LIB_SIZE */
        VM->Memory.DmemSetLong(functable);               /* Function-table */
        VM->Memory.DmemSetLong(datatable);               /* Data-table */
        VM->Memory.DmemSetLong(fhfile_t_init);           /* Init-routine */

        /* RomTag structure */

        ULO romtagstart = VM->Memory.DmemGetCounter();
        VM->Memory.DmemSetWord(0x4afc);                  /* Start of structure */
        VM->Memory.DmemSetLong(romtagstart);             /* Pointer to start of structure */
        VM->Memory.DmemSetLong(romtagstart+26);          /* Pointer to end of code */
        VM->Memory.DmemSetByte(0x81);                    /* Flags, AUTOINIT+COLDSTART */
        VM->Memory.DmemSetByte(0x1);                     /* Version */
        VM->Memory.DmemSetByte(3);                       /* DEVICE */
        VM->Memory.DmemSetByte(0);                       /* Priority */
        VM->Memory.DmemSetLong(devicename);              /* Pointer to name (used in opendev)*/
        VM->Memory.DmemSetLong(idstr);                   /* ID string */
        VM->Memory.DmemSetLong(initstruct);              /* Init_struct */

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
        _rom[0x1092] = static_cast<UBY>(_bootcode>>24);
        _rom[0x1093] = static_cast<UBY>(_bootcode >> 16);
        _rom[0x1094] = static_cast<UBY>(_bootcode >> 8);
        _rom[0x1095] = static_cast<UBY>(_bootcode);

        /* NULLIFY pointer to configdev */

        VM->Memory.DmemSetLongNoCounter(0, 4092);
        VM->Memory.EmemCardAdd(HardfileHandler_CardInit, HardfileHandler_CardMap);

        AddFileSystemsFromRdb();
      }
      else
      {
        VM->Memory.DmemClear();
      }
    }

    /*=========================*/
    /* Startup hardfile device */
    /*=========================*/

    void HardfileHandler::Startup()
    {
      /* Clear first to ensure that F is NULL */
      memset(_devices, 0, sizeof(HardfileDevice)*FHFILE_MAX_DEVICES);
      Clear();
    }

    /*==========================*/
    /* Shutdown hardfile device */
    /*==========================*/

    void HardfileHandler::Shutdown()
    {
      Clear();
    }

    /*==========================*/
    /* Create hardfile          */
    /*==========================*/

    bool HardfileHandler::Create(fellow::api::module::HardfileDevice hfile)
    {
      bool result = false;

#ifdef WIN32
      HANDLE hf;

      if(*hfile.filename && hfile.size) 
      {   
        if((hf = CreateFile(hfile.filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL)) != INVALID_HANDLE_VALUE)
        {
          if( SetFilePointer(hf, hfile.size, NULL, FILE_BEGIN) == hfile.size )
	    result = SetEndOfFile(hf) == TRUE;
          else
            Service->Log.AddLog("SetFilePointer() failure.\n");
          CloseHandle(hf);
        }
        else
          Service->Log.AddLog("CreateFile() failed.\n");
      }
      return result;
#else	/* os independent implementation */
#define BUFSIZE 32768
      ULO tobewritten;
      char buffer[BUFSIZE];
      FILE *hf;

      tobewritten = hfile.size;
      errno = 0;

      if(*hfile.filename && hfile.size) 
      {   
        if(hf = fopen(hfile.filename, "wb"))
        {
          memset(buffer, 0, sizeof(buffer));

          while(tobewritten >= BUFSIZE)
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
#endif
    }

    HardfileHandler::HardfileHandler()
      : _romstart(0), _bootcode(0), _configdev(0), _enabled(false)
    {
    }

    HardfileHandler::~HardfileHandler()
    {
    }
}
