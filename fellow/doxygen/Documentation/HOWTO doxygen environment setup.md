HOWTO: doxygen environment setup
================================

Doxygen is a documentation system for C++, C, Java, and various other languages.

It can help you in three ways:

- It can generate an on-line documentation browser (in HTML) and/or an off-line reference manual (in LaTeX) from a set of documented source files. There is also support for generating output in RTF (MS-Word), PostScript, hyperlinked PDF, compressed HTML, and Unix man pages. The documentation is extracted directly from the sources, which makes it much easier to keep the documentation consistent with the source code.  
- You can configure doxygen to extract the code structure from undocumented source files. This is very useful to quickly find your way in large source distributions. You can also visualize the relations between the various elements by means of include dependency graphs, inheritance diagrams, and collaboration diagrams, which are all generated automatically.
- You can also use doxygen for creating normal documentation (as I did for this manual).

Doxygen is developed under Linux and Mac OS X, but is set-up to be highly portable. As a result, it runs on most other Unix flavors as well. Furthermore, executables for Windows are available.

# doxygen setup using Windows Subsystem for Linux

Since the compilation of the doxygen documentation using Windows has become error-prone and frustrating, involving components that are not necessarily well-maintained, an alternate method to compile it using the Windows Subsystem for Linux has been devised. Obviously, the same thing can be achieved on a native Ubuntu Linux system.

First, setup WSL on Windows by executing

`Enable-WindowsOptionalFeature -Online -FeatureName Microsoft-Windows-Subsystem-Linux`

Reboot the system if required.

Then install Ubuntu from the Windows store: https://www.microsoft.com/store/p/ubuntu/9nblggh4msv6

When everything is set up and an initial username/password have been configured, install the required components by executing

`sudo apt install doxygen graphviz texlive-latex-base texlive-latex-recommended texlive-latex-extra`

Accept/confirm all prompts and wait for the packages to install.

cd into the WinFellow source code directory and find the doxygen subdirectory. 

Execute `Build-Doxygen-Documentation.sh`

# doxygen setup using Windows

The following setup files were used in the past to generate the WinFellow documentation on Windows 10. The newest version of doxygen where this could be tested successfully was 1.8.10.

- doxygen: doxygen-1.8.10.windows.x64.bin (http://www.stack.nl/~dimitri/doxygen/download.html)
- MiKTeX: basic MiKTeX installer (http://miktex.org/download)
- GhostScript: gs927w64.exe (https://www.ghostscript.com/download/gsdnld.html)
- Graphviz: graphviz-2.38.0.msi (https://graphviz.gitlab.io/_pages/Download/Download_windows.html)

1. Install doxygen using the default settings. Later versions than 1.8.10 have recently shown problematic; this needs to be examined in more detail.
2. Install MiKTeX using the default settings. 
3. Install GhostScript (64 bit Windows version) using the default settings. Append the directory "C:\Program Files\gs\gs9.27\bin" to the PATH environment variable, separated by a semicolon.
4. Install Graphviz using the default settings. Confirm any UAC (user account control) prompts you might encounter.
5. Verify that the following files can be executed from the command-line before attempting to build the documentation (directories need to be included in PATH - if one of them does not work, try logging off and back on again):
   - doxygen.exe
   - latex.exe
   - pdflatex.exe
   - gswin64c.exe
   - dot.exe
   

The doxygen environment should now be properly configured to build the WinFellow documentation.

To build the documentation, enter the doxygen directory and execute the file Build-Doxygen-Documentation.cmd. Verify that the table of contents contains proper page numbers, and that the documentation contains graphs. If one of these is missing, then something is wrong with the installation that requires more troubleshooting.

The documentation should be found as WinFellow-doxygen.pdf within the doxygen directory. 

