HOWTO: Preparing WinFellow for a new public release
===================================================

This HOWTO describes the steps necessary to prepare a new public WinFellow release. This is to ensure that a uniform process is used that results in the same quality of release archives, regardless of who takes care of building and spreading the release. This HOWTO should always be updated as necessary.

* Make sure that there are no open issues left in the [issue tracker](https://github.com/petschau/WinFellow/issues) that are assigned to be fixed in this release.
* Ensure that there are no local modifications/uncommitted changes in the local Git repository (compiling a release build should automatically enforce this).
* For compiling the new release, the build environment should be setup like recommended here (\ref howtodevenvsetup).
* Verify that no unnecessary debug/trace logs are written (floppy.log, capsdump.txt, ...)
* Using a debug build, verify that no memory leaks occur for basic as well as new functions; these would be logged in files called `WinFellowCrtMallocReport_WinFellowVersion_YYYYMMDD_hhmmss.log`. Ensure that all modules include code to support CRT malloc/C++ new debug logging, so that filenames and line numbers of the leaks are included.
* Perform regression tests according to the \ref howtoregressiontesting.
* The third digit of the version number should usually be increased by one (in the files versioninfo-wcrev.h and versioninfo-wcrev.rc, as well as in the doxyfile, the user guide and the NSIS installer file WinFellow.nsi). Make sure to also update the VS_VERSION_INFO structure in GUI_versioninfo-wcrev.rc.
* Update the ReadMe/FAQ to reflect the latest changes in this release.
* Compile the build using the script CompileReleaseBuild.ps1; this will take care of a number of things:
  - ensure that no local modifications exist, perform a Git update and compile a clean release build
  - copy .exe, .pdb file and PDF user guide into a folder named WinFellow_...
  - Generate ChangeLog; ensure that Git commits are included in the resulting file; copy the ChangeLog.txt into the WinFellow_... folder
  - copy GPL license terms (PDF) into the WinFellow_... folder
  - zip the folder using 7-Zip
  - clean up the source code directory, copy the GPL license terms into it and zip the folder using 7-Zip
* beta testing is usually performed before a public release; recently beta builds have been distributed via Dropbox; in the past we posted the betas to the EAB 
  [private : WinFellow beta release](http://eab.abime.net/forumdisplay.php?f=60) forum
* announce availability of the beta in the forum and to the fellow-beta mailing list
* If feedback is positive, upload the release with highlight information to GitHub and post announcements (public EAB support forum, fellow-announce, ...)
* update the hugo website with the release highlights (WinFellow branch 'hugo', insert a new post), recompile the hugo website and commit/push the resulting files to the gh-pages branch to update the public website

Unclarified points:
* Do we use profile-guided optimization, and if we do, how do we run it? Or do we use whole-program optimization instead?
* how do we ensure the beta exe is not leaked, can we mark it as beta somehow so that beta and release build differ?