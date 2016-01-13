WinFellow source code archive
-----------------------------

This is a readme file describing the contents of this archive.


Introduction
------------

WinFellow is an Amiga Emulator targeted for Windows XP/Vista/7/8/10.

Upon getting these sources
--------------------------

The entire source tree can be found as a Git repository at http://github.com/petschau/WinFellow

GNU Public License (GPL)
------------------------

The source files are tagged with the GNU Public License. Fellow and its source are GPL.
(Consult http://www.gnu.org for information on GPL)


Source organisation
-------------------

src/c
src/include

These directories contain what can be refered to as the emulation engine. This is a
generic implementation.
The C-files are ANSI-C or C++, although the OS-dependent parts might require a specific
compiler. In the event of changing the C-compiler, some generic header files need to be
changed for the emulation engine files to work with it.

src/uae
src/win32/uae

These directories contain the filesystem module from WinUAE V8.8 and some
other stripped down UAE files needed to interact with it. This module is GPL.
Explicit permission has been granted to use these files from the respective authors.

src/win32/c
src/win32/include

These directories contain pure win32 (and DirectX) implementations needed to 
support Fellow on Win32. A workspace setup to compile Fellow into
an executable is provided for MS Visual Studio.

src/win32/msvc

This directory contains a MS Visual Studio workspace for the entire Fellow sources.


What you need to compile the sources:
-------------------------------------

MS Visual Studio 2013 and the February 2010 DirectX SDK. 
The community edition can be used to compile WinFellow.

Other notes:
------------

There is a new configuration format, which was decided ages ago
to be common for UAE and Fellow. Brian King specified most of it.

These is also a keymapping file which allows you to redefine the
mapping of PC-keys to Amiga keys. It is called mapping.key

If you need more detailed information, you can contact me via mail at petschau@gmail.com
