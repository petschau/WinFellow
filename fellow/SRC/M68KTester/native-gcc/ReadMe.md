The native M68KTester is a work in progress port of the M68ktester to an Amiga with an m68k CPU, with the goal to gather CPU test data natively.

Compilation on Windows
======================

The tester can be compiled on Windows using [Cygwin](https://www.cygwin.com/) and the [AmigaOS-Cross-Toolchain](https://github.com/cahirwpz/amigaos-cross-toolchain/).

First, install the following Cygwin components that are required to be installed for successful compilation of the AmigaOS cross toolchain:
- GNU gcc 4.x (32-bit)
- libncurses-devel 6
- GNU make
- Python 2.7
- perl
- git
- subversion
- patch
- bison

Then, start a Cygwin shell and execute the following commands:

	git clone https://github.com/cahirwpz/amigaos-cross-toolchain
    cd amigaos-cross-toolchain

The AmigaOS cross toolchain is currently hardcoded to work with libncurses5-dev. Apply libncurses-dev-6.patch from the tester's patches subdirectory to the clone as a workaround to be able to compile it with current versions of Cygwin. The toolchain's author is looking into fixing that issue.

Build the toolchain by executing

    ./toolchain-m68k --prefix=/opt/m68k-amigaos build

Sit back and wait until the toolchain has been built. Once that is done successfully, cd into the tester's source directory and build it by executing make.


