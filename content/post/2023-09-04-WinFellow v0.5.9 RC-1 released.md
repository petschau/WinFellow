+++
date = "2023-09-04T05:49:00+00:00"
draft = false
title = "WinFellow v0.5.9 RC-1 released"
tags = [ "News", "Releases" ]
+++

There is a new release candidate build of WinFellow hosted in the GitHub releases section. Compared to the earlier version 0.5.8, the following changes are included in this build:

**System requirement changes**
  - support for Windows XP and Vista has been removed; version 0.5.8 will be the last WinFellow release to support Windows XP or Windows Vista. This enables us to take advantage of enhancements in the toolset used during development and testing.

**New features**
  - implemented a new menu option to allow configuration of the emulation pause behavior when the window focus is lost/emulation is running in the background
  - the floppy read-only state can now be adjusted on the fly, without having to restart the emulator

**Bug Fixes**
  - when using alt-tab to switch to different windows, the tab keypress is no longer passed on to the emulation session
  - mouse events via RetroPlatform are being received again
  - fixed minor heap corruption in RetroPlatform initialization code
  - fix a repeated error message during loading of presets when encrypted ROMs are searched and no keyfile is present

**Maintenance updates**
  - zlib was updated to version 1.2.11
  - xDMS was updated to version 1.3.2
  - Visual Studio 2022 and the Windows 11 SDK are used
  - RetroPlatform include has been updated

Please see the included file ChangeLog.txt for a full list of changes, including minor bug fixes which may not be listed above.
