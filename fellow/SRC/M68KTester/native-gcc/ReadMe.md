The native M68KTester is a work in progress port of the M68ktester to an Amiga
with a Motorola 68000 CPU, with the goal to gather CPU test data natively.

Compilation on Windows
======================

The tester can be compiled on Windows using [Cygwin](https://www.cygwin.com/) and the
[AmigaOS cross toolchain](https://github.com/cahirwpz/amigaos-cross-toolchain/).

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

Then, start a Cygwin Terminal and execute the following commands to clone the repository and build the toolchain:

	git clone https://github.com/cahirwpz/amigaos-cross-toolchain
    cd amigaos-cross-toolchain
    ./toolchain-m68k --prefix=/opt/m68k-amigaos build

Sit back and wait until the toolchain has been built. Should there be problems downloading some of the required components, it usually helps to just restart the build process.

Once the build process is done successfully, append the following line to your
user's .bashrc file (usually located at C:\Cygwin\home\<username>\.bashrc):

    export PATH=$PATH:/opt/m68k-amigaos/bin

That will make the cross-compiler's binary files executable by including them in
your search path. Close and re-open the Cygwin Terminal to apply the change.

Additional SDKs are supported by the toolchain. They can be listed by executing

    cd ~/amigaos-cross-toolchain
    ./toolchain-m68k --prefix=/opt/m68k-amigaos list-sdk

and installed by executing (for example)

    ./toolchain-m68k --prefix=/opt/m68k-amigaos install-sdk mui mmu

A few example Amiga programs are included with the toolchain. They can by compiled
by executing

    cd ~/amigaos-cross-toolchain/examples
    make

If the example programs can be compiled (one of them requires the mui SDK, another the MMU SDK), you
should be able to cd into the tester's source directory and build it by executing

    make