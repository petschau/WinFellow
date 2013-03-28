HOWTO: Development environment setup    {#howtodevenvsetup}
====================================

WinFellow is designed to be portable.
However the project files contained in the SVN repository are built using Microsoft Visual Studio.

Currently the following software should be used for development in the MAIN branch:
* Visual Studio 2008 or newer
* <a href="http://www.microsoft.com/downloads/details.aspx?familyid=4b78a58a-e672-4b83-a28e-72b5e93bd60a">November 2007 DirectX SDK</a> (not needed with Visual Studio 2012)

Visual Studio editions:
* The professional edition was used for project setup, but does not contain profiling features. These are available in the Visual Studio Team System 2008 Development Edition.
* The express edition can be used to compile WinFellow as well - this has been tested using Visual Studio 2012 (Update 1).

For access to the SVN repository, an SVN client is required.
TortoiseSVN is required to compile WinFellow, as it contains the file SubWCRev.exe, which is used to generate version information that includes the SVN revision number.
TortoiseSVN only needs to be installed, any SVN client can be used for development; tests were sucessfully performed using AnkhSVN, which integrates nicely into Visual Studio.

WinFellow was ported from DOS Fellow which was based in large parts on assembler code that has been converted to C in the MAIN branch.
The assembler based code still exists in the assembly_based branch. To work with that branch, <a href="http://nasm.sourceforge.net|nasm2">NASM2</a> is needed additionally.