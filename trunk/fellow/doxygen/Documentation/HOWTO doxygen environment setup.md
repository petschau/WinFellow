HOWTO: doxygen environment setup
================================

Doxygen is a documentation system for C++, C, Java, and various other languages.

It can help you in three ways:

- It can generate an on-line documentation browser (in HTML) and/or an off-line reference manual (in $\mbox{\LaTeX}$) from a set of documented source files. There is also support for generating output in RTF (MS-Word), PostScript, hyperlinked PDF, compressed HTML, and Unix man pages. The documentation is extracted directly from the sources, which makes it much easier to keep the documentation consistent with the source code.  
- You can configure doxygen to extract the code structure from undocumented source files. This is very useful to quickly find your way in large source distributions. You can also visualize the relations between the various elements by means of include dependency graphs, inheritance diagrams, and collaboration diagrams, which are all generated automatically.
- You can also use doxygen for creating normal documentation (as I did for this manual).

Doxygen is developed under Linux and Mac OS X, but is set-up to be highly portable. As a result, it runs on most other Unix flavors as well. Furthermore, executables for Windows are available.

The following setup files were used to generate the WinFellow documentation on Windows 7 SP1.
Windows 8 will work, but causes warnings, as it is an untested Windows version.

- doxygen: doxygen-1.8.3-setup.exe (http://www.stack.nl/~dimitri/doxygen/download.html)
- MiKTeX: basic-miktex-2.9.4521.exe (http://miktex.org/2.9/setup)
- GhostScript: gs905w32.exe (http://www.ghostscript.com/GPL_Ghostscript_9.05.html)
- Graphviz: graphviz-2.28.0.msi (http://www.graphviz.org/Download_windows.php)

1. Install doxygen using the default settings.
2. Install MikTex (32 bit Windows Basic version) using the default settings (preferred paper size is A4, missing packages will be installed "on-the-fly"). 
3. Install GhostScript (32 bit Windows version) using the default settings. Append the directory C:\Program Files (x86)\gs\gs9.05\bin to the PATH variable, separated by a semicolon.
4. Install Graphviz using the default settings. Confirm any UAC (user account control) prompts you might encounter.
5. Verify that the following files can be executed from the commandline (directories need to be included in PATH - if one of them does not work, try logging off and back on again):
   doxygen.exe
   latex.exe
   pdflatex.exe
   gswin32c.exe
   dot.exe
   
The doxygen environment should now be properly configured to build the WinFellow documentation.

To build the documentation, enter the doxygen directory and execute the file Build-Doxygen-Documentation.cmd.
On the first execution, you will be prompted to install several missing MikTex packages. Install them from the nearest package repository, and for anyone who uses this computer.
Confirm any UAC prompts that appear.

The installation of colortbl.sty failed on my system (from the ftp.uni-erlangen.de mirror), as colortbl.cab could not be found online (404 error).
Abort the execution if that happens, and start the MiKTeX Package Manager (Admin). Synchronize the package repository, search for the file name colortbl.sty and enter the package that is shown below (right click, install).
The installation should now work, so close the package manager and the restart the batch file. After all missing packages have been installed, the batch should finished. The documentation should then be found as WinFellow-doxygen.pdf within the doxygen directory. 

Run the MiKTeX Update (Admin) tool to update all existing MiKTeX packages to the current version. The first run should update miktex-bin-2.9, the second run should update further packages. Repeat until no more updates are found.
