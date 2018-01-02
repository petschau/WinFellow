+++
date = "2018-01-02T16:00:00+00:00"
draft = false
title = "WinFellow v0.5.7 released"
tags = [ "News", "Releases" ]
+++

**Bug fixes:**

* floppy code was updated to use a track limit > 80; it will output random data beyond track 80 - fixes Outrun/The Games: Summer Edition; also added missing checks for disabled drives, fixing Winter Olympics 94
* fixed a bug where unexpected pixel format flags besides RGB stopped DirectDraw initialization
* several bugs in the screenshot code were fixed; standalone screenshots were not saved correctly in D3D mode, and used with Amiga Forever raw screenshots were missing screen areas due to faulty initialization of the internal clipping areas
* fixed screen area mismatch when Amiga Forever region/size settings are both set to auto; auto clipping is still not supported
* CPU instruction fixes:
    * fixed a signed bug in 64-bit division
    * minor timing adjustment for btst
* CIA event counter changes according to documentation
* the project files were updated to Visual Studio 2017

Please see the included file ChangeLog.txt for a full list of changes, including minor bug fixes which are not listed above.