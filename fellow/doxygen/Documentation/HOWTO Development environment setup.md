HOWTO: Development environment setup    {#howtodevenvsetup}
====================================

WinFellow is designed to be portable to different operating systems.
However the project files contained in the Git repository are built using Microsoft Visual Studio.

Currently the following software should be used for development in the Git master branch:

- Visual Studio 2022, including the following additions:

  - the extension "Format document on save" should be installed, to ensure that the solution's .clang-format file is applied on each save
  - reformatting of the whole solution can also be done by executing the following command in the solution directory via WSL:
    `find . -iregex '.*\.\(c\|cpp\|hpp\|cc\|cxx\|h\)' -exec clang-format -style=file -i {} \;`

The community edition of Visual Studio 2022 can be used to compile WinFellow; it even features debugging and profiling.

For access to the Git repository, the <a href="http://desktop.github.com">GitHub Desktop</a> client is required.

For basic contributions, the components mentioned above should be sufficient.

To be able to compile release builds using the automated build script, a number of additional components must be
installed and added to the search path:

- Visual Studio 2022 with C++ desktop development components must be installed
- PowerShell execution policy must be set to unrestricted (both for the 32 as well as the 64 bit PowerShell)
- Git for Windows 64 Bit must be installed and added to the search path; usually Notepad++ is used as default editor
- the module posh-git must be installed
- the LyX Bundle including MikTeX must be installed and lyx.exe added to the search path
- 7-Zip - search path
- NSIS - search path
- Visual Studio Locator (vswhere.exe) must be located somewhere within the search path

As an aid on a clean system, the fellow\SRC\WIN32\Scripts directory contains a Boxstarter script that can be used to quickly perform an unattended setup of all required components. This might later be used to achieve an automated build process.

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