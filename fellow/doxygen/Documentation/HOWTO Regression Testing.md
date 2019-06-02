HOWTO: Regression Testing {#howtoregressiontesting}
===================================================

Several issues were fixed over time in WinFellow's emulation core.
To avoid them resurfacing in the future, the following tests should be performed
on a regular basis, at the very least after major changes.

When testing large batches of titles, to speed up testing it is possible to make
the emulation run at maximum speed by setting the configuration file option
_sound_output=interrupts_; this can also be configured in Amiga Forever's
override.ini, allowing quick testing of a group of RP9 packages.

## Automated Test Cases

The following is a list of test cases where the excution can be automated using WinFellow's automation capabilities;
most of these are emulator core bugfixes.

The actual individual test case result must be gathered by examining the resulting screenshots.

### r749: Changed the constant for lines in a frame
---------------------------------------------------
Test that the demo "Global Trash" by The Silents will proceed beyond the second
screen and display the "Global Trash" logo. Before this commit, it would freeze
before that appeared.

### r764: Flags for eorw and orw, calculation of flags fixed
------------------------------------------------------------
Test that Micro Machines [cr CSL] can be started without issues/the main menu
appears. Before this commit, it would crash after the cracktro.

### r814: removed filling in MFM encoding of floppy sectors
-----------------------------------------------------------
Test that Ballistix [cr Defjam] can be started without issues. Before this commit,
it froze right after starting it up.

### r824: Changed how the CPU driven sound stub issues IRQs
-----------------------------------------------------------
Test that Ballistix [cr Defjam] can be started without issues. Before this commit,
it would freeze before displaying the Psyclapse logo.

Other known titles affected:
Bombuzal, Brides of Dracula, Dungeon Master, F-18 Interceptor,
Fiendish Freddy, Grand Prix Circuit, Indiana Jones 3,
Loom, Neuromancer, Personal Nightmare,
Revenge of the Mutant Camels, Robocop, Test Drive,
Test Drive 2, Vroom, Where in the world is Carmen Sandiego,
Wings of Fury

### r831: Redesigned chipset interrupt handling
-----------------------------------------------
Test that the demo "Seeing is Believing" can be started without issues -
before this commit, the session would freeze immediately upon start.

Other known titles affected:
Championship Manager 95 Italy, Crystal Kingdom Dizzy, Flashback,
Life & Death, Lionheart, Nitro, Pacland, Pipemania, Populous,
Puffy's Saga, Speedball, Ugh!

### r832: fixed movep instruction bug, improved CPU exception handling
----------------------------------------------------------------------
Test that the game Airborne Ranger can be started sucessfully, without graphical issues.
Before this commit, the screen would look garbled.

Other known titles affected:
Barbarian 1 (Psygnosis), Fire Force, KGB, Larry 3, Larry 5, Police Quest 2,
The Lost Vikings.

### r865: Loaders using disksync different from 0x4489 and 0x8914 with standard ADF files
-----------------------------------------------------------------------------------------
Test that Prince of Persia loads. It would not start earlier.

Test that Lemmings 2 (uncracked) loads successfully; before the fix, it would
indefinitely read from the floppy disk while loading the main menu.

Comment in 0.4.4 source also suggests North and South is affected, though this could not be
reproduced recently. This fix supersedes the fix in r843, which did not take 0x8914 into 
account.

### r873: floppy words per line should be 2 instead of 3
--------------------------------------------------------
Test that the demo Guardian Dragon II by Kefrens loads successfully, and continues beyond
the first Kefrens logo. The scrolling text between the two blue statues needs to appear. 
Before this change, the sound would start garbling and the emulation session would reboot.

### r888: sprites in hires dual playfield mode
----------------------------------------------
Test that during the loader screen in Decaying Paradise by Andromeda, a blue rotating
triangle-shaped logo is visible. Before this change, the triangle was invisible.

### r898: increased sprite action/merge item list sizes to a maximum of 5.000 entries
-------------------------------------------------------------------------------------
Start the 1MB chipmem version of State of the Art (Spaceballs) with only 512kB of
chipmem configured. The demo will cause a crash of the emulator session, but the
emulator should remain responsive.
Before this change, a crash to the desktop would occur.

Update: 
This fix is replaced by Git f99a94de. "To be safe increase sprite max list items to 100"
It follows other code changes that reduced the need for very large sprite list buffers.
This is still a valid test. 
Also test the game Megalomania, it creates a larger sprite list than State of the Art
when it crashes after the initial intro text when OCS is selected. (The actual crash is due
to copy-protection.)

### r906: Sprite DMA was being disabled instead of waiting
----------------------------------------------------------
Start Arkanoid, hit F1 and start in round 7 (move mouse to the right to select
level). Verify that enemy sprites are coming down from the top of the screen.
Before this change, they were not visible.

Arkanoid is updating vstart/vstop with the copper.

### r949: PC -2 saved on the stack for address exception
--------------------------------------------------------
Start the game Double Dragon II [cr Oracle] and verify that it loads the main
menu after exiting the cracktro by hitting Enter.

Before this change, a loader animation would be displayed indefinitely.

### r950: proper "short frame" when display is interlaced and frame is short
----------------------------------------------------------------------------
Start the game Project-X Special Edition '93 and enter the game. Before
this change, there would be graphical corruption and the emulator would
become unresponsive.

### r954: fix copper list load
------------------------------
Start the demo Sequential by Andromeda with automatic interlace compensation.
The (interlaced) intro graphic must be displayed properly, without interlace
flickering.

Before this change, it would have inverted lines.

Also affected by this is the game The Ninja Warriors; the game would not load
before this change and can now be started.

If copper DMA was off during "end of frame", and is being turned on for the first
time after that, it also loads the copper list pointer.

### r955: graph frame pointer NULL pointer exception
----------------------------------------------------
Start the game First Samurai and proceed through the intro. When prompted
to insert disk 2 into DF0:, do not change the disk, just press fire. The
game will crash, but the emulation session should remain open indefinitely.

Before this commit, the emulator would crash to the desktop with a NULL
pointer exception.

### r957: clear DMA pending flag when blit is initiated from enabling DMA in wdmacon
------------------------------------------------------------------------------------
Start the demo "Megademo 8" (Kefrens). Enter the "snake bite" section and verify
that it loads normally.

Before this change, the emulator would hang in an endless loop and become
unresponsive.

### r958: set floppy change bit high when no disk is selected
-------------------------------------------------------------
Start the game "Silkworm [cr Trilogy/t+4 Trilogy]" and verify that the cracktro
can be left by pressing both mouse buttons simultaneously.

Before this commit, it was impossible to proceed beyond the cracktro.

Also known to be affected by this change is the game "Plan 9 From Outer Space",
which would fail to proceed loading the second disk.

### r963: copjmp  lost if triggered while dma was off and copper had already run to the end of its copper list
--------------------------------------------------------------------------------------------------------------
Start the demo Multica by Andromeda with automatic interlace compensation enabled.
Verify that the initial intro screen featuring the Andromeda logo looks right.

Before this commit, it had an issue with inverted lines.

### r967: implemented chipmem / bogomem aliasing
------------------------------------------------
Start the demo Wayfarer by Spaceballs with 512kB chipmem and no bogo memory. It should load normally, the
emulator should not crash.

Before this commit, the session would crash. Related titles impacted by this change are Sensible Soccer 1.0,
Cannon Fodder XMAS Edition and Toki.

### r969, r971: CIA timer fix
-----------------------------
Verify in the game Atomix that the main game can be started.

Before this fix, it would hang when entering the game.

### r974: bit-field '020 instruction code reimplemented
-------------------------------------------------------
Start the demo Lotus Esprit Turbo Challenge 96k by Scarab. It should start correctly.

Before this change it would fail to load with a black screen after the intro. I makes use of bit-field instructions during decrunching.

In the same commit, ASL overflow handling was improved, and a flag check regarding MULU was fixed; no test cases are known for these changes.

### Git 86bd011 and Git xxxxxx: Two disk related fixes and modification later
-----------------------------------------------------------------------------
The game Amegas, packed into one file, does a disk access right after decrunching that requires the motor bit to be set in advance, and hung.
This is a detail metioned in the HRM. Check that the game starts.

The game "The Games: Summer edition" stepped the disk head beyond the end of the disk and hung. It reads (with disksync) from track 80
where no data is. Max track set to 83 now, and data for upper tracks are random. (Will eventually generate sync.)
Check that the game loads. Note that the game has broken intro graphics.

Outrun steps to track 80 and needs the extended max track limit. Check that Outrun loads.

### Git f99a94de: Last of several commits regarding "Sprites on HAM"
--------------------------------------------------------------------
Verify that sprites on top of HAM resolution bitplane graphics works.

Two cases using this:

Fairlight - My Room demo
A red filled vector cube (the sprite) is moving around on the screen, part of the middle screen is HAM resolution.
Make sure the cube looks perfect wherever it moves.

Silents - Ice demo
One of the first parts is a white screen with a picture of a warrior girl. The screen should contain a basic filled vector 
geometry object (the sprite) on top of the image moving around with no artifacts.

### Git 9a9ceb4: Added missing checks for disabled drives
---------------------------------------------------------
Disabled floppy drives were not properly off-line with regard to some signals. This caused the game Winter Olympics 94
to endlessly scan for more drives. Check that the game loads. 

Note: The game's scan is bugged, if you enable 4 drives, it will scan forever.

### Git 8b0c112: Re-encode track data to MFM from ADF after floppy-write
------------------------------------------------------------------------
Supercars with bytebandit virus would not load because the virus re-wrote track 0. The internal MFM buffer's sector headers
was out of order with what the game's trackloader expected, and it failed when pressing fire to start game. (Loads some first.)
Emulator now regenerates MFM from disk for ADF's after writes to get the sector order.

Note: Supercars needs additional fix to work. See Git <TBD>.

## Manual Test Cases

The following test cases must be executed manually as they have not been automated.

### r897: Alt+F4 in RetroPlatform mode when undo is enabled and a change was performed
--------------------------------------------------------------------------------------
Test that Alt+F4 in RetroPlatform mode will close the emulation session.
Perform a write operation on a floppy where undo is enabled, and close the session
using Alt+F4. The undo dialog must be usable both when clicking ok or cancel.
Before this change, the emulation session would always be closed.

### r941: reset of RetroPlatform causes input devices to no longer function
---------------------------------------------------------------------------
Start the A500 system, click into the emulator window to capture the cursor.
Hold Esc to release cursor and click the Reset button.
After the reset, verify that the mouse is usable in Workbench.

### r972, r973: RetroPlatform escape key handling
-------------------------------------------------
Using default escape key ESC

1. verify that the cracktro of Cannon Fodder 2 [cr PDX] can be left by tipping ESC; holding ESC and releasing after the interval should have no effect (release input devices)
2. verify in Turrican II that the main game will be quit when tipping ESC; holding for the configured interval and releasing should have no effect (release input devices)
3. verify that pressing the key for longer than the configured interval releases the mouse cursor BEFORE releasing the key (end of frame handler)
4. verify in the game F-19 Stealth Fighter, that tipping the ESC key will delete a character from the roster; releasing after the configured interval should not have an effect, except to release the input devices
5. verify that using the escape key while emulation is paused will release the devices, but will not send the escape key when the session is resumed (Cannon Fodder 2 cracktro)

Configure escape key to A

1. verify in a Workbench CLI that tipping A will produce a single "a" on the screen; holding and releasing A should have no effect (release input devices)

### r987: clipping and scaling, screenshots in Amiga Forever
------------------------------------------------------------
Edit a title to use PAL standard clipping and verify it is displayed correctly in 1x mode. Take a screenshot, verify it is ok. 

Edit a title to use PAL (maximum) clipping and verify it is displayed correctly in 1x mode. Take a screenshot, verify it is ok.

Start Arkanoid and verify it is displayed correctly
- in 1x mode. Take a screenshot, verify it is ok.
- in 2x mode. Take a screenshot, verify it is ok.

Edit a title to use Automatic clipping and verify the maximum screen area is visible in 1x mode. Take a screenshot, verify it is ok. 

Edit a title to use custom clipping of a small area and verify it is displayed correctly
- in 1x mode. Take a screenshot, verify it is ok.
- in 2x mode. Take a screenshot, verify it is ok.

### Git 78ff087 (and other previous commits)
--------------------------------------------
Start a standalone WinFellow session, configure it for DirectDraw mode. Configure scaling to 1x and and load Workbench. 
Hit <PrntScrn> to take a screenshot and ensure it is being saved to the pictures folder. 
Reconfigure to 2x, 3x and 4x as well as auto scaling and save a screenshot each time.
Reconfigure the WinFellow session for Direct3D mode with 1x scaling and take another screenshot. 
Do the same for 2, 3x, 4x and auto scaling. Ensure the screenshots are saved properly and that they look like expected.
Test this both for reconfiguring a running session, as well as starting a new emulation session/restarting the emulator.

Start Amiga Forever.
Configure the WinFellow plugin for DirectDraw mode (Tools->Options->Emulation, Plugins, WinFellow, Graphics API). 
Start the Amiga 1000 system; click the 1x scaling button and close the emulation session.
Restart the emulation session (it should still be in 1x mode upon start), wait for it to boot, escape the mouse cursor 
and start the clipping editor (Tools->Edit Screen Clip). Ensure that the whole Amiga screen buffer is visible with a blue 
background in the clipping editor, there should be no missing black areas. 
Cancel the clipping editor dialog. Save a screenshot to the pictures folder via the screenshot button and verify its creation.
Do the same in 2x, 3x and 4x modes and ensure the screenshots are saved correctly; use the clipping editor accordingly.
Close the emulation window, reopen it and take screenshots again.

Configure the WinFellow plugin for Direct3D mode (Tools->Options->Emulation, Plugins, WinFellow, Graphics API). 
Start the Amiga 1000 system; click the 1x scaling button and close the emulation session.
Restart the emulation session (it should still be in 1x mode upon start), wait for it to boot, escape the mouse cursor 
and start the clipping editor (Tools->Edit Screen Clip). Ensure that the whole Amiga screen buffer is visible with a blue 
background in the clipping editor, there should be no missing black areas. 
Cancel the clipping editor dialog. Save a screenshot to the pictures folder via the screenshot button and verify its creation.
Do the same in 2x, 3x and 4x modes and ensure the screenshots are saved correctly; use the clipping editor accordingly.
Close the emulation window, reopen it and take screenshots again.

### Git a8178da: RDB hardfiles, read-only hardfile configuration & improved hardware interrupt handling

Apply the Amiga 3000 model preset to a new configuration.

Enable all floppy drives, configure all floppy drives to read-only and enable the "fast disk-DMA" configuration option. Insert the following floppies into DF0: through DF3:

- amiga-os-310-install.adf (DF0:)
- amiga-os-310-locale.adf (DF1:)
- amiga-os-310-workbench.adf (DF2:)
- amiga-os-310-extras.adf (DF3:)

Create 3 new hardfiles:

- test1.hdf (300 MB, leave writable)
- test2.hdf (200 MB, edit hardfile to set read-only flag)
- test3.hdf (100 MB, leave writable)

Set the write-protection flag in the Windows file properties of test3.hdf.

Disable the "disable Autoconfig-devices" configuration option.

Start the emulation session. Attempt to quick format drives DH0: through DH2: as FHF0 to FHF2; compare the capacities shown on screen with the expected disk capacities. DH0: should be writable, the others should fail to format with an error message about the disk being write-protected.

Hold the right mouse button and from the Workbench menu, select "Execute Command...". Execute the command "DF0:HDTools/HDToolBox fhfile.device"; HD Toolbox should launch and show 3 SCSI drives/LUNs. Select the disk with LUN 0 and click "Change Drive Type". With the drive type being SCSI, click "Define New...". Click "Read Configuration" and "Continue". Verify the drive capacity and confirm by clicking "OK" twice. Click "Save Changes to Drive". Click "Exit".

Press F11, and in the emulator window that appears click "hard reset" and "start emulation". The emulation session should restart and two unformatted partitions should appear (FDH0: and FDH1:). Format partition FDH0: by holding the right mouse button and selecting the menu Icons==>Format Disk...; enter "Workbench" as disk name and quick-format the disk using default parameters.

Start the Workbench 3.1 installation by opening the Install3.1 floppy, browsing into the "Install" directory and opening the installer in the language of your choice. Follow the on-screen steps to finish the Workbench installation; during installation, keep moving the mouse continuously to verify that no freezes related to hardware interrupts occur (a8178daa2195acdc15f586d3fef6fdcc492e5417) . When prompted, swap the floppy disks as they are requested (F11, click ... besides one of the floppies to exchange it with another one and click "start emulation"). The installation should finish successfully, without freezes.

Eject all floppies (F11 and click all four eject buttons, save configuration and click "start emulation"), click "Continue" to reset the emulation session. Workbench should boot up from the hardfile.

Repeat a second time to ensure that no freezes occur when the mouse is being moved permanently.

