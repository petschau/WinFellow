The native M68KTester is a work in progress port of the M68ktester to an Amiga
with a Motorola 68000 CPU, with the goal to gather CPU test data natively.

Compilation on Windows
======================

The tester can be compiled on Windows using [Cygwin](https://www.cygwin.com/) and
the [AmigaOS cross toolchain](https://github.com/cahirwpz/amigaos-cross-toolchain/).

Download and install the 32 bit version of Cygwin. During installation, select the following
components that are required to be installed for successful compilation of the AmigaOS cross
toolchain:
- GNU gcc-g++ 4.x
- GNU make
- python 2.7
- perl
- git
- subversion
- patch
- bison

Then, start a Cygwin Terminal and execute the following commands:

	git clone https://github.com/cahirwpz/amigaos-cross-toolchain
    cd amigaos-cross-toolchain

The AmigaOS cross toolchain is currently hardcoded to work with libncurses5-dev.
Apply libncurses-dev-6.patch from the tester's patches subdirectory to the clone
as a workaround to be able to compile it with current versions of Cygwin. The
toolchain's author is looking into fixing that issue; alternatively, apply the
patch internalize-libncurses.patch that brings the script to a state provided
for testing by the author.

Build the toolchain by executing

    ./toolchain-m68k --prefix=/opt/m68k-amigaos build

Sit back and wait until the toolchain has been built. 

Once the build process is done successfully, append the following line to your
user's .bashrc file (usually located at C:\Cygwin\home\<username>\.bashrc):

    export PATH=$PATH:/opt/m68k-amigaos/bin
    
That will make the cross-compiler's binary files executable by including them in
your search path. Close and re-open the Cygwin Terminal to apply the change.
    
Additional SDKs are supported by the toolchain. They can be listed by executing

    cd ~/amigaos-cross-toolchain
    ./toolchain-m68k --prefix=/opt/m68k-amigaos list-sdk

and installed by executing (for example)

    ./toolchain-m68k --prefix=/opt/m68k-amigaos install-sdk mui

A few example Amiga programs are included with the toolchain. They can by compiled
by executing

    cd ~/amigaos-cross-toolchain/examples
    make

If the example programs can be compiled (one of them requires the mui SDK), you
should be able to cd into the tester's source directory and build it by executing

    make