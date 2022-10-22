/*=========================================================================*/
/* Fellow                                                                  */
/* Host framebuffer driver                                                 */
/* DirectX implementation                                                  */
/* Author: Petter Schau                                                    */
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

/***********************************************************************/
/**
 * @file
 * Graphics device module
 *
 * Framebuffer modes of operation:
 *
 * 1. Using the primary buffer non-clipped, with possible back-buffers.
 *    This applies to fullscreen mode with a framepointer.
 * 2. Using a secondary buffer to render and then applying the blitter
 *    to the primary buffer. The primary buffer can be clipped (window)
 *    or non-clipped (fullscreen). In this mode there are no backbuffers.
 *
 * blitmode is used:
 *  1. When running on the desktop in a window.
 *  2. In fullscreen mode with a primary surface that
 *     can not supply a framebuffer pointer.
 *
 * Windows:
 * Two types of windows:
 *  1. Normal desktop window for desktop operation
 *  2. Full-screen window for fullscreen mode
 *
 * Windows are created and destroyed on emulation start and stop.
 *
 * Buffers:
 * Buffers are created when emulation starts and destroyed
 * when emulation stops. (Recreated when lost also)
 *
 * if blitmode, create single primary buffer
 * and a secondary buffer for actual rendering in system memory.
 * if windowed also add a clipper to the primary buffer
 *
 * else create a primary buffer (with backbuffers)
 ***************************************************************************/

#define INITGUID

#include <list>
#include <string>
#include <sstream>

#include "fellow/os/windows/directdraw/GfxDrvDirectDraw.h"
#include "fellow/os/windows/directdraw/DirectDrawErrorLogger.h"

#include <WindowsX.h>

#include "fellow/os/windows/graphics/GfxDrvCommon.h"
#include "fellow/api/Services.h"
#include "fellow/application/HostRenderer.h"
#include "fellow/os/windows/directdraw/DirectDrawDevice.h"

using namespace std;

//========================================================================
// Callback used to collect information about available DirectDraw devices
// Called on emulator startup
//========================================================================

BOOL WINAPI EnumerateDirectDrawDeviceStatic(GUID FAR *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext)
{
  return ((GfxDrvDirectDraw *)lpContext)->EnumerateDirectDrawDevice(lpGUID, lpDriverDescription, lpDriverName);
}

BOOL GfxDrvDirectDraw::EnumerateDirectDrawDevice(GUID FAR *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName)
{
  DirectDrawDevice *newDevice = new DirectDrawDevice();

  if (lpGUID == nullptr)
  {
    newDevice->GUID = {};
    _ddrawDeviceCurrent = newDevice;
  }
  else
  {
    newDevice->GUID = *lpGUID;
  }

  newDevice->DriverDescription = lpDriverDescription;
  newDevice->DriverName = lpDriverName;

  _ddrawDevices.emplace_back(newDevice);

  return DDENUMRET_OK;
}

void GfxDrvDirectDraw::LogDirectDrawDeviceInformation()
{
  ostringstream summary;
  summary << "gfxdrv: DirectDraw devices found: " << _ddrawDevices.size() << endl;
  fellow::api::Service->Log.AddLog(summary.str());

  for (DirectDrawDevice *device : _ddrawDevices)
  {
    ostringstream description;
    description << "gfxdrv: DirectDraw Driver Description: " << device->DriverDescription << endl;
    fellow::api::Service->Log.AddLog(description.str());

    ostringstream driverName;
    driverName << "gfxdrv: DirectDraw Driver Name       : " << device->DriverName << endl;
    fellow::api::Service->Log.AddLog(driverName.str());
  }
}

//===============================================
// Creates a list of available DirectDraw devices
// Called on emulator startup
//===============================================

bool GfxDrvDirectDraw::DirectDrawDeviceInformationInitialize()
{
  _ddrawDevices.clear();
  _ddrawDeviceCurrent = nullptr;

  HRESULT err = DirectDrawEnumerate(EnumerateDirectDrawDeviceStatic, this);
  if (err != DD_OK)
  {
    DirectDrawErrorLogger::LogError("DirectDrawDeviceInformationInitialize(), DirectDrawEnumerate(): ", err);
  }

  if (_ddrawDeviceCurrent == nullptr)
  {
    _ddrawDeviceCurrent = _ddrawDevices.front();
  }

  LogDirectDrawDeviceInformation();

  return !_ddrawDevices.empty();
}

//==================================================
// Releases the list of available DirectDraw devices
// Called on emulator shutdown
//==================================================

void GfxDrvDirectDraw::DirectDrawDeviceInformationRelease()
{
  for (DirectDrawDevice *device : _ddrawDevices)
  {
    delete device;
  }

  _ddrawDevices.clear();
  _ddrawDeviceCurrent = nullptr;
}

void GfxDrvDirectDraw::RenderHud(const MappedChipsetFramebuffer &mappedFramebuffer)
{
  if (_legacyHud != nullptr)
  {
    _legacyHud->DrawHUD(mappedFramebuffer, fellow::api::Service->HUD.GetHudConfiguration());
  }
}

//====================
// Flip to next buffer
//====================

void GfxDrvDirectDraw::Flip()
{
  _ddrawDeviceCurrent->SurfaceBlit();
}

std::list<DisplayMode> GfxDrvDirectDraw::GetDisplayModeList()
{
  return _ddrawDeviceCurrent->GetDisplayModeList();
}

//==========================================
// Open the default DirectDraw device
// and record information about capabilities
//==========================================

bool GfxDrvDirectDraw::Initialize()
{
  bool result = DirectDrawDeviceInformationInitialize();
  if (!result)
  {
    return false;
  }

  result = _ddrawDeviceCurrent->Initialize();
  if (!result)
  {
    DirectDrawDeviceInformationRelease();
    return false;
  }

  return true;
}

//=========================================================
// Release any resources allocated by gfxDrvDDrawInitialize
//=========================================================

void GfxDrvDirectDraw::Release()
{
  _ddrawDeviceCurrent->Release();
  DirectDrawDeviceInformationRelease();
}

//======================================================
// Locks surface and obtains a valid framebuffer pointer
//======================================================

GfxDrvMappedBufferPointer GfxDrvDirectDraw::MapChipsetFramebuffer()
{
  ULO pitch;
  UBY *buffer = _ddrawDeviceCurrent->SurfaceLock(&pitch);
  if (buffer == nullptr)
  {
    return GfxDrvMappedBufferPointer{.Buffer = nullptr, .Pitch = 0, .IsValid = false};
  }

  return GfxDrvMappedBufferPointer{.Buffer = buffer, .Pitch = pitch, .IsValid = true};
}

//================
// Unlocks surface
//================

void GfxDrvDirectDraw::UnmapChipsetFramebuffer()
{
  _ddrawDeviceCurrent->SurfaceUnlock();
}

void GfxDrvDirectDraw::SizeChanged(unsigned int width, unsigned int height)
{
  _ddrawDeviceCurrent->SizeChanged(width, height);
}

void GfxDrvDirectDraw::PositionChanged()
{
  _ddrawDeviceCurrent->PositionChanged();
}

void GfxDrvDirectDraw::InitializeLegacyHud(HudPropertyProvider *hudPropertyProvider, const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings)
{
  _legacyHud = new GfxDrvHudLegacy(
      hudPropertyProvider,
      chipsetBufferRuntimeSettings.Dimensions,
      chipsetBufferRuntimeSettings.MaxClip,
      chipsetBufferRuntimeSettings.OutputClipPixels,
      Draw.GetChipsetBufferScaleFactor(),
      chipsetBufferRuntimeSettings.ColorBits,
      (ULO)Draw.GetHostColorForColor12Bit(0x0f0),
      (ULO)Draw.GetHostColorForColor12Bit(0x000),
      (ULO)Draw.GetHostColorForColor12Bit(0xf00),
      (ULO)Draw.GetHostColorForColor12Bit(0x400));
}

void GfxDrvDirectDraw::ReleaseLegacyHud()
{
  if (_legacyHud != nullptr)
  {
    delete _legacyHud;
    _legacyHud = nullptr;
  }
}

bool GfxDrvDirectDraw::SaveScreenshot(bool bTakeFilteredScreenshot, const char *filename)
{
  return _ddrawDeviceCurrent->SaveScreenshot(bTakeFilteredScreenshot, filename);
}

void GfxDrvDirectDraw::ClearCurrentBuffer()
{
  _ddrawDeviceCurrent->ClearCurrentBuffer();
}

GfxDrvColorBitsInformation GfxDrvDirectDraw::GetColorBitsInformation()
{
  return _ddrawDeviceCurrent->GetColorBitsInformation();
}

/************************************************************************/
/**
 * Emulation is starting.
 *
 * Called on emulation start.
 * Subtlety: In exclusive mode, the window that is attached to the device
 * appears to become activated, even if it is not shown at the time.
 * The WM_ACTIVATE message triggers DirectInput acquisition, which means
 * that the DirectInput object needs to have been created at that time.
 * Unfortunately, the window must be created as well in order to attach DI
 * objects to it. So we create window, create DI objects in between and then
 * do the rest of the gfx init.
 * That is why gfxDrvEmulationStart is split in two.
 ***************************************************************************/

bool GfxDrvDirectDraw::EmulationStart(
    const HostRenderConfiguration &hostRenderConfiguration,
    const HostRenderRuntimeSettings &hostRenderRuntimeSettings,
    const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings,
    HudPropertyProvider *hudPropertyProvider)
{
  _ddrawDeviceCurrent->SetDisplayMode(
      hostRenderRuntimeSettings, chipsetBufferRuntimeSettings, hostRenderConfiguration.Scale); // Remembers the displayMode and finds corresponding fullscreen mode record

  InitializeLegacyHud(hudPropertyProvider, chipsetBufferRuntimeSettings);

  return true;
}

/************************************************************************/
/**
 * Emulation is starting, post
 * Returns number of buffers
 ***************************************************************************/

unsigned int GfxDrvDirectDraw::EmulationStartPost(const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings, HWND hwnd)
{
  bool activateDisplayModeSuccess = _ddrawDeviceCurrent->ActivateDisplayMode(chipsetBufferRuntimeSettings.Dimensions, hwnd); // This requires a window handle, only available on ...post
  if (!activateDisplayModeSuccess)
  {
    fellow::api::Service->Log.AddLog("gfxdrv: gfxDrvDDrawEmulationStartPost(): gfxDrvDDrawSetMode() failed to initialize buffers\n");
    return 0;
  }
  return 1;
}

//==================================================
// Emulation is stopping, clean up current videomode
//==================================================

void GfxDrvDirectDraw::EmulationStop()
{
  _ddrawDeviceCurrent->SurfacesRelease();
  _ddrawDeviceCurrent->SetCooperativeLevelNormal();
  ReleaseLegacyHud();
}

//=====================================================================
// Collects information about the DirectX capabilities of this computer
// After this, a DDraw object exists
//=====================================================================

bool GfxDrvDirectDraw::Startup()
{
  graph_buffer_lost = false;
  _initialized = Initialize();
  return _initialized;
}

//=====================
// Free DDraw resources
//=====================

void GfxDrvDirectDraw::Shutdown()
{
  if (_initialized)
  {
    Release();
    _initialized = false;
  }
}
