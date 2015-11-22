HOWTO: Development environment setup    {#howtodevenvsetup}
====================================

WinFellow is designed to be portable.
However the project files contained in the SVN repository are built using Microsoft Visual Studio.

Currently the following software should be used for development in the MAIN branch:
* Visual Studio 2013 Update 5
* <a href="http://www.microsoft.com/en-us/download/details.aspx?id=10084">February 2010 DirectX SDK</a>

Visual Studio editions:
* The community edition of Visual Studio 2013 can be used to compile WinFellow; it even features debugging and profiling.

For access to the SVN repository, an SVN client is required.
TortoiseSVN is required to compile WinFellow, as it contains the file SubWCRev.exe, which is used to generate version information that includes the SVN revision number.
TortoiseSVN only needs to be installed, any SVN client can be used for development; tests were sucessfully performed using AnkhSVN, which integrates nicely into Visual Studio.

WinFellow was ported from DOS Fellow which was based in large parts on assembler code that has been converted to C in the MAIN branch.
The assembler based code still exists in the assembly_based branch. To work with that branch, <a href="http://nasm.sourceforge.net|nasm2">NASM2</a> is needed additionally.

Coding Style Guidelines
-----------------------
To ensure consistency across the different modules, that are develop by different authors, we
believe it is reasonable to have a common set of guidelines all developers follow.

- For optimum layout, configure the tab width to 8, and enable the replacing of tabs with spaces.
- Configure the indentations to 2.
- No line should be longer than 200 characters.
- prefer the use of built-in C++ datatypes for new code (bool instead of BOOLE)
- new modules can be created in C++
- instead of one-line statements, the use of the curly brackets {} is preferred, but not mandatory
