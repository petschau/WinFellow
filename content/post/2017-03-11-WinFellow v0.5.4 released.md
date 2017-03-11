+++
date = "2017-03-11T18:15:00+00:00"
draft = false
title = "WinFellow v0.5.4 released"
tags = [ "News", "Releases" ]
+++

**New features:**

* re-designed display settings to make use of modern higher resolution displays more intuitive; instead of configuring a window resolution, scaling and overscan area can be configured automatically to create a window of the optimal size and scaling factor; both can also be configured manually to arrive at frequently window sizes
* support for the Direct3D 11 graphics API; this requires DirectX 11 to be installed
  * on Windows Vista, the platform update [KB971644](https://support.microsoft.com/en-us/help/971644) should be installed, as it contains required updates to DirectX
  * the graphics card has to provide hardware acceleration for Direct3D 
* ability to save screenshots to the pictures folder (hit <Print Screen> to trigger)
* support for automatic, as well as 3x and 4x scaling, which is useful for 4K/high DPI displays; for the best results, try leaving the scaling at automatic
* very basic serial port/UART emulation; nothing can be connected to the serial port; this is mostly used for serial debug logging (which is active only in debug builds of WinFellow), but the port is always being emulated

**Bug Fixes:**
* improved frame timing to present frames in a more evenly paced manner
* improvements to CIA timer handling
* improvements to CPU instruction timing (TAS and PEA instructions)
* fixed sound volume restoration during reset of Amiga session in Amiga Forever
* improved error handling for DMS floppy image extraction
* fixed minor memory leaks

Please see the included file ChangeLog.txt for a full list of changes, including minor bug fixes which are not listed above.
