HOWTO: Development environment setup    {#howtodevenvsetup}
====================================

WinFellow is designed to be portable.
However the project files contained in the CVS repository are built using Microsoft Visual Studio.

Currently the following software should be used for development in the MAIN branch:
* Visual Studio 2008
* <a href="http://www.microsoft.com/downloads/details.aspx?familyid=4b78a58a-e672-4b83-a28e-72b5e93bd60a">November 2007 DirectX SDK</a>

Visual Studio editions:
* The professional edition was used for project setup, but does not contain profiling features. These are available in the Visual Studio Team System 2008 Development Edition.
* The express edition can be used to compile WinFellow as well - this has been tested using Visual Studio 2012 (Update 1).

WinFellow was ported from DOS Fellow which was based in large parts on assembler code that has been converted to C in the MAIN branch.
The assembler based code still exists in the assembly_based CVS branch. To work with that branch, <a href="http://nasm.sourceforge.net|nasm2">NASM2</a> is needed additionally.