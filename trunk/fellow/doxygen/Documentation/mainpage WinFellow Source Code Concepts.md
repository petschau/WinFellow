WinFellow Source Code Concepts     {#mainpage}
==============================


Petter Schau, May 2000

This document contains **some** core concepts for the source code in Fellow v0.4.

It attempts to describe the overall structure and organization of the source-code.


Module Types
============


WinFellow is a modular design where there are basically 3 kinds of modules:

*Emulation modules* which emulate some part of the Amiga hardware. These modules are independent of, and free from any OS-specific code. The modules are in some cases heavily implemented with x86-assembly code.

*Device wrapper* modules which isolate the emulation modules from the device implementation modules. They provide functions to the emulation modules which does exactly what the emulation modules need. In turn, the device wrapper modules use OS-specific code in order to program the hardware for a specific OS.

*Administration modules* which perform the overall management of the emulator.

Standard Module API
===================


##Standard Events##

Each module implements the following standard Fellow module functions to respond to various events:

* Initialization
* Shutdown
* Starting emulation
* Stop emulation
* Hard reset
* End of line
* End of frame

##Module API / Configuration##

Additionally there are module specific functions to perform the following functions:

* Configuration
* Program the associated hardware

Examples
========

##Sound Device Module##

For example, the sound device module implements most of the standard events. It has a collection of configuration functions in order to set the desired sound quality, and it has functions which allow the sound-emulation module to add new samples to the sound device. Lastly, there is a function which starts playback of a new buffer.

##Graphics Device Module##

Another example is the graphics device module, which also implements most standard events. Configuration involves setting the current graphics mode to one of the available modes. It has a function which delivers a framepointer to the emulation draw module, which enables the emulation to draw pixels on the graphics-device. Note that it does not matter to the draw-module whether the actual device is a window, full-screen or just a buffer in normal memory as long as the device wrapper module can provide a pointer to the image-buffer.

Minor / Major modules
=====================

In most cases, device-wrapper modules are owned and initialized by the emulation modules. The emulation draw module initializes a graphics device wrapper modules which it uses to perform the drawing.

##Module List##

In general, each emulation module corresponds to a pair of files (C part and assembly part). In one case, the emulation is so complex that the emulation process has been divided into 3 modules. (Graphics.)

###Emulation Modules###

* Blitter emulation module (blit.c / blita.s)
* Bus emulation module (bus.c / busa.s)
* Cia chips emulation module (cia.c / ciaa.s)
* Copper emulation module (copper.c / coppera.s)
* CPU emulation module (cpu.c / cpua.s)
* Drawing emulation module (draw.c / drawa.s)
* Filesystem mapping module (ffilesys.c and UAE specific files)
* Floppy emulation module (floppy.c)
* Hardfile emulation module (fhfile.c / fhfile.s)
* Gameport emulation module (gameport.c)
* Graphics emulation module (graph.c / grapha.s)
* Keyboard emulation module (kbd.c)
* Memory emulation module (fmem.c / fmema.s)
* Sound emulation module (sound.c / sounda.s)
* Sprite emulation module (sprite.c / spritea.s)
* Wav file sound output module (wav.c)

###Device Wrapper Modules###

* Graphics device module (gfxdrv.c)
* Joystick device module (joydrv.c)
* Keyboard device module (kbddrv.c / kbdparser.c)
* Mouse device module (mousedrv.c)
* Sound device module (sound.c)
* GUI module (wgui.c)

###Administration Modules###

* Fellow module (fellow.c)
* Configuration module (config.c)
* OS-specific startup module (winmain.c)

##Modules Overview##

This overview explains each module in terms of how it takes input, what makes the module do work, and what the output is. It also tries explain how it is related to other modules and how it cooperates with those modules.

###Blitter Emulation Module Overview###

The blitter module emulates the Amiga blitter in software. The blitter is basically a chip that reads data from one or more source memory locations, combines the data read using a logical expression and finally writes the result to a destination location. The blitter operates in 3 major modes, copy, line or fill.

On initialization, the module creates a lot of static lookup tables which is used during emulation of a particular mode.

The state of the blitter is contained in a number of "registers". These registers are accessed using memory-access functions registered in the memory emulation module. The memory module will call those functions whenever data is written to the accociated memory locations.

When the BLTSIZE register is written, a blit starts. (Once again happens indirectly through the memory module via. a memory access function. )

The blit is defined by the current state of the blitter registers. The state of the blitter is analyzed to decide what needs to be done. The time needed for the operation is calculated, ie. the time an Amiga blitter would have needed to complete the blit.

What happens next is that a "blitter-event" is added to the bus emulation module. This causes a function to be called a number of virtual clock ticks (which we calculated) into the future. Nothing else is done, and by returning, the emulator goes on to emulate something else.

Later, when the event we scheduled in the bus emulation module is due, code is run that emulates the blit. And an IRQ is raised which cause other things to happen which is not described here.

###Bus Emulation Module Overview###

The bus emulation module is the heart of the emulator. It contains a virtual tick counter, and a queue of events.
Basically it is just one loop which does the following:

- Take event off the start of the queue.
- Set virtual tick counter to the time the next event happens.
- Run code associated with the event.

Everything emulated is run from this loop. Each module needs to schedule new events to keep running.
Possible events are:

* CPU instruction event
* Copper instruction event
* End of line event
* Blitter event
* IRQ event
* CIA event
* End of frame event

At any time, there will always be at least two events in the queue, End of line and End of frame. Unless the CPU runs a STOP instruction, the CPU is also in the queue.

This model is chosen to reflect the fact that at any one time, there are many things going on in parallell inside an Amiga. It is more efficient and a lot cleaner that the different modules schedule their own tasks through this mechanism than the more obvious one where the main loop would have to ask each module in turn, do you want to do something now? Mostly, the answer would have been no, and a lot of wasted CPU-time.

The module also contains the handlers for End of line and End of frame, these two events do a lot of bookkeeping work for various modules (by means of calling <ModuleName>EndOfFrame() and <ModuleName>EndOfLine() functions, for instance, indirectly, these events drive the sound, floppy and graphics emulation.

###CIA Chips Emulation Module Overview###

The two Cia chips controls various peripherals, such as the floppy-drives and keyboard. They also contain a number of timers and some other features that are uncommon and not emulated.

Basically, the Cia module is driven by writes to the registers. (Memory access functions registered with the memory module.) In turn this causes functions in the floppy-emulation module to be called when floppy-related status registers are being written.

The timers are handled by always keeping the expiration time updated. This is turn affects the Cia event that is on the bus emulation queue. When the Cia event expires, the Cia event is called and there is no additional overhead to keep track of the timers.

The third aspect of Cia emulation is that the floppy and keyboard module will sometimes update their status registers located in the Cia module. Such as making a new keyboard scancode visible, or asking the Cia to generate an IRQ. (Cia IRQs are in turn scheduled through the CPU-emulation module.)

This module has no device wrapper.

###Copper Emulation Module Overview###

The copper is a simple processor that writes data one word at a time into custom chip registers. It can wait for a specified raster beam position and for the blitter. The copper is programmed by creating a list of instructions in memory. (Somewhat oversimplified view of the operation.)

The copper emulation is driven from one side by memory access functions registered in the memory emulation module. (Start and stop copper, define the memory location of the copper instruction list to name most.)

On the other side, the emulation of copper lists is event driven through the bus emulation module. The time for the next copper instruction is an event, and emulation code is called in the copper module. The copper module calls memory location access functions registered in the memory emulation module to write values to custom chip registers.

This module has no device wrapper.

###CPU Emulation Module Overview###

The CPU is the largest emulation module. It emulates the Motorola 68k CPU. The overall path of emulation of a CPU-instruction is:

* The bus emulation module runs a CPU-instruction event.
* The opcode-word is read using the memory-emulation module.
* The opcode-word is used as an index into a jump-table which contains a pointer to a function which emulates the opcode.
* The instruction is parsed, usually it includes reading one or more words using the memory-emulation module, do an operation on the data, and write the result to memory once again using the memory-emulation module. The internal state of the CPU is updated (flags, PC, stack etc.).
* The next CPU-instruction event is scheduled by calculating the clock ticks for this module.

The CPU module also handles the scheduling of IRQ events.

Other details of internal aspects of the CPU-emulation is omitted here. There are for instance a lot of pre-calculated data in tables in order to help parsing what to do for each opcode, and the case of handling IRQs, exceptions and maintaining the integrity of the status-register. There are also some optimizations to speed up reading and writing to memory, and a couple of optimization defects that are difficult to remove without replacing the module. (Evaluating EA twice in many cases and much faster flag handling.)

This module has no device wrapper.

###Graphics, Sprite and Drawing Emulation Module Overview###

The graphics handling is divided in three. The custom registers for graphics and sprites are held by the graphics and sprite modules. They are modified the usual way by registering memory location access functions with the memory module. In order to keep track of what happens to the screen and to figure out what to draw, much bookkeeping is done to calculate helper variables that describe the current screen properties in a cleaner way than the actual registers.

The overall process of rendering a line of the Amiga screen is as follows:

* At the end of each virtual line, the current state of the custom registers define the appearance of the line. The rendering is one line at the time, which does not capture some special effects accurately L The EndOfFrame() handler cause rendering to happen.
* The appearance of the line is rendered into a temporary buffer in a format that is preprocessed to aid fast drawing later. This includes translating the planar Amiga graphics to chunky pixels which describe the color for each pixel on the line. Calculations involve location of the horisontal borders, and possible hidden pixels that must be skipped, but not shown. Or the line might be a line that shows the vertical border. In any case, the temporary buffer contains a full Amiga line rendered as chunky pixels. There are also some flags to catch special cases, such as a line with no bitmap pixels, in that case the color of the entire line is remembered.
* Sprites are added to the temporary line. Similar calculations about location or absense of sprites are done to figure out where they are.
* For each virtual line we repeat this process, building a temporary buffer for the entire screen.
* When the entire frame is done, the EndOfFrame() handler cause the draw-module to do work.
* A framepointer to the current buffer (in case of double/triple buffering) is obtained from the graphics device wrapper module.
* Each line for the entire screen is processed one at the time. The temporary rendering provided by the graphics module is translated to the pixel-format on the host screen one pixel at the time. Blank lines are skipped if their colors are unchanged.

The draw module uses the graphics device wrapper to gain access to the screen.

###Filesystem Mapping Module Overview###

The filesystem module is provided from the UAE-project. It plugs itself into AmigaOS by making itself available as an expansion card. (Expansion cards are added using some functions in the memory emulation module.) When the expansion card is discovered by AmigaOS during boot-up, the "filesystems expansion-card ROM" is mapped into memory and an initialization routine is called. This routine creates descriptions for each drive mounted as a filesystem in the "ROM" tagged as resident modules. Later in the boot-process, AmigaOS will scan the ROM for resident module identifiers and initialize each drive for use with AmigaOS. The code in the filesystem is written in C (x86 native), but the ROM contains short stubs which execute illegal instructions which the emulator recognizes and in turn the correct routines for handling the entire filesystem is called.

That description leaves a lot desired, but on the overall, that’s how it operates.

This module has no device wrapper. It is implemented using standard C IO.

###Hardfile Module Overview###

The basic concepts for hooking the hardfiles into AmigaOS is the same as for the Filesystem mapping module. Using a virtual expansion card and resident module tags in a ROM area provided by the expansion card. The only difference is that native functions are not called by executing illegal instructions, but by writing certain values to predefined locations in the hardfile device ROM and letting the memory module call memory location access functions for those addresses.

This module has no device wrapper. It is implemented using standard C IO.

###Floppy Emulation Module Overview###

The floppy emulation module state is modified through writes to certain CIA registers, and some custom registers. Apart from handling inserted disk-images, such as keeping track of where the head is and whether the motor is started, the main operation in the emulation is reading and writing data located on the disk. Disks are emulated as files which is a complete (non-MFM encoded) image of a floppy disk.

Reading and writing starts when a dedicated custom register is written, the module keeps track of that the usual way by implementing a memory location access function. The current state for the drive defines the operation (read/write, track, side, motor, length of read). Floppy data is fed slowly into the Amiga memory to improve compatibility with loaders that do not expect data arriving at high speed. (Though it can be configured to do this fast.)

Words are read at the end of each virtual line using the EndOfLine() handler.

This module has no device wrapper. It is implemented using standard C IO.

###Gameport Emulation Module Overview###

The gameport emulation module is a collection of memory location access functions which responds to the custom registers allocated to the gameport.

Additionally it has variables that contains the current state of a joystick and mouse for each of the two ports, there are returned by the memory location access functions whenever they are called.

The gameport emulation module depends on device wrapper modules for mouse and joystick support. Each module must support asychronous notification of changes to the mouse or joystick state, and call functions in the gameport emulation module to change the current state for the mouse and joystick.

This module requires device wrapper modules for mouse and joystick.

###Keyboard Emulation Module Overview###

The keyboard emulation module maintains the current state of a virtual Amiga keyboard. There are three levels of symbolic key mappings for the virtual keyboard. The module itself keeps track of keypresses using a symbolic PC keymap. This PC keyboard is mapped onto actual Amiga keys on one side, allowing great freedom in choosing how to map the keys. On the other hand, there are the actual keycodes, which are OS-dependent and translated into symbolic PC-keys by the keyboard device wrapper module.

The module cooperates with the Cia emulation module. The current (or last) scancode is contained in a register in the Cia-module. The keyboard module writes the new scancode to the Cia when there is a new one (and the Cia will raise an IRQ for it.)

From the other side, the keyboard device wrapper module is assumed to receive asynchronous notification about keychanges on the real keyboard and call a function in the keyboard module to get it processed for emulation of the keypress.

In between there is a queue to buffer fast arriving keypresses, since there is a minimum of virtual ticks between each new scancode to arrive in the emulated Amiga.

This module requires device wrapper for the actual keyboard.

###Memory Emulation Module Overview###

The memory emulation module sets up a virtual memory space for use by the other emulated Amiga devices. At the core is description tables which define the entire memory space in terms of the following:

* Memory is divided up into 64K banks.
* Each bank is described by memory location access functions for reading and writing byte, word and long from the memory in the bank. There is an optional pointer to the actual memory for this bank, and a flag indicating whether executable code can exist in the bank.

The module provides functions for setting up chip, fast, bogo, expansion cards, Kickstart image and Custom chip register memory areas. Other modules can map themselves into memory using standard functions.
In the case of register handling, a second level of memory location access functions are used to trap access to a specific register. The memory emulation module handles this for custom registers, the Cia sets this up by itself in its own bank access functions.
This module has no device wrapper.

###Sound and Wav Emulation Modules Overview###

Sound is emulated through the use of a state-machine which is needed to accurately emulate all aspects of the Amiga sound hardware. (See HRM).

Sound is controlled through trapping access to custom chip registers through the memory module the usual way. In addition, actual sound output is produced by running the audio state machine a number of times in each EndOfLine().

In order to play the sample values produced by the emulation, a sound device wrapper module is needed.

The Wav module is an alternate sound-device wrapper module, although it implements the sound device wrapper API, samples are not played, but written to a Wav file on disk.

More information on the sound module exists in a separate document.

This module requires device wrapper for a sound device.

##Device Wrapper Module Overviews##

There is no documentation for the device wrappers. Read the include files for each device wrapper to see the API, read comments in the source code and see how the device-independent modules make use of them to accomplish their tasks.