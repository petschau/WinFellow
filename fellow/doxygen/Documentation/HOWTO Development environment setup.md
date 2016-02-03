HOWTO: Development environment setup    {#howtodevenvsetup}
====================================

WinFellow is designed to be portable to different operating systems.
However the project files contained in the Git repository are built using Microsoft Visual Studio.

Currently the following software should be used for development in the Git master branch:
- Visual Studio 2013 Update 5
- <a href="http://www.microsoft.com/en-us/download/details.aspx?id=10084">February 2010 DirectX SDK</a>

Visual Studio editions:
- The community edition of Visual Studio 2013 can be used to compile WinFellow; it even features debugging and profiling.

For access to the Git repository, the <a href="http://desktop.github.com">GitHub Desktop</a> client is required.

WinFellow was ported from DOS Fellow, which was based in large parts on assembler code that has been converted to C in the master branch.
The assembler based code still exists in the assembly_based branch. To work with that branch, <a href="http://nasm.sourceforge.net|nasm2">NASM2</a> is needed additionally.

The WinFellow user manual was created and is being edited using <a href="http://www.lyx.org">LyX</a>.

A documentation (work in progress) targeted specifically at developers can also be created using <a href="http://www.stack.nl/~dimitri/doxygen/">doxygen</a> and the batch file "Build-Doxygen-Documentation.cmd" included in the source code archive; a TeX distribution like <a href="http://miktex.org/">MikTeX</a> or <a href="https://www.tug.org/texlive/">TeX Live</a> must be installed for the generation to succeed.

Coding Style Guidelines
-----------------------
To ensure consistency across the different modules that are developed by different authors, we
believe it is reasonable to have a common set of guidelines all developers follow.

- For optimum layout, configure the tab width to 8, and enable the replacing of tabs with spaces.
- Configure the indentations to 2.
- No line should be longer than 200 characters.
- Prefer the use of built-in C++ datatypes for new code (bool instead of BOOLE).
- New modules can be created in C++.
- Instead of one-line statements, the use of the curly brackets {} is preferred, but not mandatory.