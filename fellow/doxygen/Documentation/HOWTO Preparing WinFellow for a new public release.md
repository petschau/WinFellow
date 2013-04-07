HOWTO: Preparing WinFellow for a new public release
===================================================

This HOWTO describes the steps necessary to prepare a new public WinFellow release. This is to ensure that a uniform process is used that results in the same quality of release archives, whoever takes care of building and spreading the release. This HOWTO should always be updated as necessary.

* Make sure that there are no open issues left in the [WinFellow Pre-Release Agenda](https://sourceforge.net/p/fellow/internal-pre-release-agenda)
* Ensure that the subwcrev.exe output in the buildlog does not list any local modifications (compiling a release builds enforces this).
* There may be testing steps defined in the Pre-Release Agenda that should be tested when preparing for a new release - these should not be closed!
* For compiling the new release, the build environment should be setup like recommended here (\ref howtodevenvsetup).
* Verify that no unnecessary debug/trace logs are written (floppy.log, capsdump.txt, ...)
* The third digit of the version number should be increased by one.
* The build configuration should be changed to Release.
* //Unclear: Do we use profile-guided optimization, and if we do, how do we run it? Or do we use whole-program optimization instead?//
* Update the ReadMe/FAQ.
* Compile the build using the script collection CompileReleaseBuild-v0.6.zip; this will take care of a number of things:
  - ensure that no local modifications exist, perform an SVN update and compile a clean release build
  - copy .exe, .pdb file and PDF user guide into a folder named WinFellow_...
  - Generate ChangeLog using [svn2cl](http://sites.google.com/site/kzmizzz/svn2clwin-en);
    ensure that SVN revisions are included in the resulting file; copy the ChangeLog.txt into the WinFellow_... folder
  - copy GPL license terms (PDF) into the WinFellow_... folder
  - zip the folder using 7-Zip
  - clean up the source code directory, copy the GPL license terms into it and zip the folder using 7-Zip
* beta testing is usually performed in advance; recently it has been performed using Dropbox; in the past we posted the beta to the EAB 
  [private : WinFellow beta release](http://eab.abime.net/forumdisplay.php?f=60) board
* announce availability of the beta in the forum and to the fellow-beta mailing list
* If feedback is positive, post exe to the website, fellow-announce, ... 
  //(how do we ensure the beta exe is not leaked, can we mark it as beta somehow so that beta and release build differ?)//
