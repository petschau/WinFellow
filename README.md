![WinFellow](./fellow/Docs/WinFellow/winfellow_logo_large.png)
# Amiga Emulator for Windows

WinFellow source code archive
=============================

This is a README file describing the contents of the source code archive.

Introduction
------------
WinFellow is a high performance Amiga Emulator primarily targeted for Windows.
Its distinguished API and core do however allow a fairly easy port to other operating systems.

WinFellow is targeted for Windows XP/Vista/7/8/10.

Obtaining these sources
-----------------------
The entire source tree can be found as a Git repository at http://github.com/petschau/WinFellow.

GNU General Public License (GPLv2)
----------------------------------
WinFellow and its source code are developed and distributed under the terms of the
[GNU General Public License version 2.0 (GPLv2)](http://www.gnu.org/licenses/old-licenses/gpl-2.0.html).

Source organisation
-------------------
```
fellow/src/c
fellow/src/include
```
These directories contain what can be refered to as the emulation engine. This is a
generic implementation.

The C-files are ANSI-C or C++, although the OS-dependent parts might require a specific
compiler. In the event of changing the C-compiler, some generic header files need to be
changed for the emulation engine files to work with it.
```
fellow/src/uae
fellow/src/win32/uae
```
These directories contain the filesystem module from WinUAE V8.8 and some
other stripped down UAE files needed to interact with it. This module is GPL.
Explicit permission has been granted to use these files from the respective authors.
```
fellow/src/win32/c
fellow/src/win32/include
```
These directories contain pure win32 (and DirectX) implementations needed to
support Fellow on Win32. A workspace setup to compile WinFellow into
an executable is provided for MS Visual Studio.
```
fellow/src/win32/msvc
```
This directory contains a MS Visual Studio workspace for the entire Fellow sources.

What you need to compile the sources
------------------------------------

Microsoft Visual Studio 2017 and the February 2010 DirectX SDK.
The community edition of Visual Studio can be used to compile WinFellow.

Other notes
-----------

There is a new configuration format, which was decided ages ago
to be common for UAE and Fellow. Brian King specified most of it.

These is also a keymapping file which allows you to redefine the
mapping of PC-keys to Amiga keys. It is called mapping.key

If you need more detailed information, you can contact me via mail at
[petschau@gmail.com](mailto:petschau@gmail.com).
