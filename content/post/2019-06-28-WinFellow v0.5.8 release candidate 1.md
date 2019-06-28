+++
date = "2019-06-28T05:49:00+00:00"
draft = false
title = "WinFellow v0.5.8 release candidate 1"
tags = [ "News", "Releases" ]
+++

A new public release candidate build of WinFellow v0.5.8 has been uploaded to the downloads in the GitHub releases section.

Compared to the earlier version 0.5.7, the following changes are included in this build:

**New features**

- support for RDB hardfiles was implemented; while creating a new hardfile from the WinFellow GUI still creates "plain" hardfiles, a Rigid Disk Block can be created using tools like HD Toolbox
  – the default naming scheme for filesystem devices has been changed from DHx: to FSx: to avoid naming conflicts with existing RDB hardfiles
- a 64 bit build is now provided; the NSIS installer will install either the 32 or 64 bit version depending on the operating system that is used

**Bug fixes**

-  improved handling of hardware interrupts and blitter delays; fixes freezes that could occur during installation of Workbench in earlier 0.5.x versions
-  fix wrong selection of render function after return from menu when application did not write to bplcon0 every frame; fixes garbled graphics in a Legend cracktro when returning from menu
-  replaced alternating patterns for unmapped memory with proper random numbers
-  CLR reads before write; fixes loading of Outrun and a fast-mem test in Last Ninja 2
-  improve delay caused by logging in screen-mode enumeration (could amount to many seconds with multiple screens)
-  running in an environment with very slow graphics framerate, like a VM, could cause a deadlock; set wait time after event reset to avoid race condition
-  fix an issue where screenshots would occasionally not be taken in Direct3D graphics mode; this may also have impacted the ability to use Amiga Forever's clipping editor
-  fix an issue where applying a new model preset would leave multiple blitter radio buttons selected

**Maintenance updates**

-  project files were updated to Visual Studio 2019 using the toolset v141_xp
-  the February 2010 DirectX SDK dependency was eliminated
-  IPF Access API files were updated to the latest version, and support for it was enabled in 64 bit builds

See the included file ChangeLog.txt for a full list of changes, including minor bug fixes which may not be listed above.

Please report any issues you find with this pre-release build in our issue tracker: https://github.com/petschau/WinFellow/issues