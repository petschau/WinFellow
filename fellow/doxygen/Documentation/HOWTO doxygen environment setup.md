HOWTO: doxygen environment setup
================================

Doxygen is a documentation system for C++, C, Java, and various other languages.

It can help you in three ways:

- It can generate an on-line documentation browser (in HTML) and/or an off-line reference manual (in LaTeX) from a set of documented source files. There is also support for generating output in RTF (MS-Word), PostScript, hyperlinked PDF, compressed HTML, and Unix man pages. The documentation is extracted directly from the sources, which makes it much easier to keep the documentation consistent with the source code.  
- You can configure doxygen to extract the code structure from undocumented source files. This is very useful to quickly find your way in large source distributions. You can also visualize the relations between the various elements by means of include dependency graphs, inheritance diagrams, and collaboration diagrams, which are all generated automatically.
- You can also use doxygen for creating normal documentation (as I did for this manual).

Doxygen is developed under Linux and Mac OS X, but is set-up to be highly portable. As a result, it runs on most other Unix flavors as well. Furthermore, executables for Windows are available.

The following setup files were used to generate the WinFellow documentation on Windows 8.1.

- doxygen: doxygen-1.8.3-setup.exe (http://www.stack.nl/~dimitri/doxygen/download.html)
- TeX Live: texlive2013.iso (http://www.tug.org/texlive/acquire-iso.html)
- GhostScript: gs905w32.exe (http://www.ghostscript.com/GPL_Ghostscript_9.05.html)
- Graphviz: graphviz-2.28.0.msi (http://www.graphviz.org/Download_windows.php)

1. Install doxygen using the default settings.
2. Install TeX Live (the full DVD iso installation is hazzle-free)
   using the default settings (preferred paper size is A4).
   After installation, update the installation by executing the commands
   `tlmgr update --self` 
   and 
   `tlmgr update --all`
   in a commandline (it does NOT need to be elevated, any authenticated user is able to update TeX Live by default). 
3. Install GhostScript (32 bit Windows version) using the default settings. Append the directory "C:\Program Files (x86)\gs\gs9.05\bin" to the PATH environment variable, separated by a semicolon.
4. Install Graphviz using the default settings. Confirm any UAC (user account control) prompts you might encounter.
5. Verify that the following files can be executed from the commandline (directories need to be included in PATH - if one of them does not work, try logging off and back on again):
   - doxygen.exe
   - latex.exe
   - pdflatex.exe
   - gswin32c.exe
   - dot.exe
   
The doxygen environment should now be properly configured to build the WinFellow documentation.

To build the documentation, enter the doxygen directory and execute the file Build-Doxygen-Documentation.cmd.

The documentation should be found as WinFellow-doxygen.pdf within the doxygen directory. 

