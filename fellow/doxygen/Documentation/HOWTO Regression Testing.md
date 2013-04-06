HOWTO: Regression Testing
===================================================

Several issues were fixed over time in WinFellow's emulation core. 
To avoid them resurfacing in the future, the following tests should be performed 
on a regular basis, at the very least after major changes.

r749: Changed the constant for lines in a frame
-----------------------------------------------
Test that the demo "Global Trash" by The Silents will proceed beyond the second
screen and display the "Global Trash" logo. Before this commit, it would freeze
before that appeared.

r764: Flags for eorw and orw, calculation of flags fixed
--------------------------------------------------------
Test that Micro Machines [cr CSL] can be started without issues/the main menu
appear. Before this commit, it would crash after the cracktro.

r814: removed filling in mfm encoding of floppy sectors
-------------------------------------------------------
Test that Ballistix [cr Defjam] can be started without issues. Before this commit,
it froze right after starting it up.

r824: Changed how the CPU driven sound stub issues irq's
--------------------------------------------------------
Test that Ballistix [cr Defjam] can be started without issues. Before this commit,
it would freeze before displaying the Psyclapse logo.

Other known titles affected:
Bombuzal, Brides of Dracula, Dungeon Master, F-18 Interceptor,
Fiendish Freddy, Grand Prix Circuit, Indiana Jones 3,
Loom, Neuromancer, Personal Nightmare, 
Revenge of the Mutant Camels, Robocop, Test Drive,
Test Drive 2, Vroom, Where in the world is Carmen Sandiego,
Wings of Fury

r831: Redesigned chipset interrupt handling
-------------------------------------------
Test that "Seeing is Believing" can be started without issues - 
before this commit, the session would freeze immediately upon start.

Other known titles affected:
Championship Manager 95 Italy, Crystal Kingdom Dizzy, Flashback,
Life & Death, Lionheart, Nitro, Pacland, Pipemania, Populous,
Puffy's Saga, Speedball, Ugh!

r832: fixed movep instruction bug, improved CPU exception handling
------------------------------------------------------------------
Test that Airborne Ranger can be started sucessfully, without graphical issues.
Before this commit, the screen would look garbled.

Other known titles affected:
Barbarian 1 (Psygnosis), Fire Force, KGB, Larry 3, Larry 5, Police Quest 2,
The Lost Vikings.

r833: More checks for emulation stop (ie F11) to avoid deadlock in rare cases.
------------------------------------------------------------------------------
No test case is known yet.

r865: Loaders using disksync different from 0x4489 and 0x8914 with standard ADF files
-------------------------------------------------------------------------------------
Test that Prince of Persia loads. It would not start earlier.

Test that Lemmings 2 (uncracked) loads successfully; before the fix, it would
indefinitely read from the floppy disk while loading the main menu.

Comment in 0.4.4 source also suggests North and South is affected, though this could not be
reproduced recently. This fix supersedes the fix in r843, which did not take 0x8914 into 
account.
