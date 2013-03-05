HOWTO: Preparing WinFellow for a new public release
===================================================

This HOWTO describes the steps necessary to prepare a new public WinFellow release. This is to ensure that a uniform process is used that results in the same quality of release archives, whoever takes care of building and spreading the release. This HOWTO should always be updated as necessary.

* Make sure that there are no open issues left in the <a href="http://sourceforge.net/tracker/?group_id=3431&atid=658580">WinFellow Pre-Release Agenda</a>
* Ensure that the subwcrev.exe output in the buildlog does not list any local modifications.
* There may be testing steps defined in the Pre-Release Agenda that should be tested when preparing for a new release - these should not be closed!
* For compiling the new release, the build environment should be setup like recommended here (\ref howtodevenvsetup).
* Verify that no unnecessary debug/trace logs are written (floppy.log, capsdump.txt, ...)
* The version number should be increased and the versioning changed from CVS to release.
* The build configuration should be changed to Release.
* //Unclear: Do we use profile-guided optimization, and if we do, how do we run it? Or do we use whole-program optimization instead?//
* Update the ReadMe/FAQ.
* Compile the exe (always do a clean rebuild of the entire solution by pressing Ctrl+Alt+F7)
* copy exe and readme into a folder named WinFellow_...
* //Unclear: Zip the folder using WinZIP/WinRAR (?)//
* post the beta to the EAB [private : WinFellow beta release](http://eab.abime.net/forumdisplay.php?f=60) board
* announce availability of the beta in the forum and to the fellow-beta mailing list
* If feedback is positive, post exe to the website, fellow-announce, ... //(how do we ensure the beta exe is not leaked, can we mark it as beta somehow so that beta and release build differ?)//
