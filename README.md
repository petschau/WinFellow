![WinFellow](./fellow/Docs/WinFellow/winfellow_logo_large.png)

# Amiga Emulator for Windows

WinFellow source code archive
=============================

This is a README file describing the contents of the source code archive.

Introduction
------------

WinFellow is a high performance Amiga Emulator primarily targeted for Windows.
Its distinguished API and core do however allow a fairly easy port to other operating systems.

WinFellow is targeted for Windows 7/8/10/11.

Obtaining these sources
-----------------------

The entire source tree can be found as a Git repository at http://github.com/petschau/WinFellow.

GNU General Public License (GPLv2)
----------------------------------

WinFellow and its source code are developed and distributed under the terms of the
[GNU General Public License version 2.0 (GPLv2)](http://www.gnu.org/licenses/old-licenses/gpl-2.0.html).

Source organisation
-------------------
The source code is being reorganized into a structure based on solution/project files.

```
fellow/SRC
```

This directory is the main directory for source code. It contains the Visual Studio solution, as well
as subdirectories for child project specific implementations like the emulator core, the hardfile code,
68k generator and unit testing infrastructure.

The WinFellow folder contains the original project files that have not yet been reorganized.

```
fellow/SRC/WinFellow/C
fellow/SRC/WinFellow/INCLUDE
```

These directories contain the original parts of what can be refered to as the emulation engine.
This is a generic implementation.

The C-files are ANSI-C or C++, although the OS-dependent parts might require a specific
compiler. In the event of changing the C-compiler, some generic header files need to be
changed for the emulation engine files to work with it.

```
fellow/SRC/WinFellow/uae
```

This directory contains the filesystem module from WinUAE V8.8 and some
other stripped down UAE files needed to interact with it. This module is GPL.
Explicit permission has been granted to use these files from the respective authors.

```
fellow/SRC/WinFellow/Windows
```

This directory contain pure win32 (and DirectX) implementations needed to
support Fellow on Win32. A workspace setup to compile WinFellow into
an executable is provided for MS Visual Studio.

```
fellow/SRC/WinFellow
```

This directory contains an MS Visual Studio workspace for the entire Fellow sources.


What you need to compile the sources
------------------------------------

Microsoft Visual Studio 2022; the community edition of Visual Studio is sufficient to compile WinFellow.

The build process currently also requires git to be located in the search path, as well as posh-git to be installed. The execution policy must allow execution of PowerShell scripts for both 32 as well as 64 bit PowerShell processes.

Other notes
-----------

There is a new configuration format, which was decided ages ago
to be common for UAE and Fellow. Brian King specified most of it.

These is also a keymapping file which allows you to redefine the
mapping of PC-keys to Amiga keys. It is called mapping.key

If you need more detailed information, you can contact me via mail at
[petschau@gmail.com](mailto:petschau@gmail.com).
