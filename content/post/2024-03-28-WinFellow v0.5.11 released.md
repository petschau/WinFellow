+++
date = "2024-03-28T05:49:00+00:00"
draft = false
title = "WinFellow v0.5.11 released"
tags = [ "News", "Releases" ]
+++

There is a new public release of WinFellow v0.5.11 in the GitHub releases section; it had been published earlier as release candidate 1.

Compared to the earlier version 0.5.10, the following changes are included in this build:

**New features**
- the default display driver was changed from DirectDraw to Direct3D 11, where supported
- implemented support for keyboard-initiated reset in Amiga Forever

**Bug Fixes**
- improved logging in case of failure to initialize Direct3d graphics driver
- fix several issues related to creating, loading, applying and saving configuration files
- fix potential memory leaks
- implement more failure checks for Direct3D initialization
- fix crash to desktop when using an emulated joystick in Amiga Forever

**Maintenance updates**
- code refactoring
  * added sound driver interface and classes
  * redesign filesystem wrapper and logging
  * upgrade to cpp20
-  removed a Windows XP specific workaround from the filesystem module
- reformatted the project using a uniform clang-format configuration

Please see the included file ChangeLog.txt for a full list of changes, including minor bug fixes which may not be listed above.
