+++
date = "2023-10-04T05:49:00+00:00"
draft = false
title = "WinFellow v0.5.10 released"
tags = [ "News", "Releases" ]
+++

There is a new public release of WinFellow v0.5.10 in the GitHub releases section; it had been published earlier as release candidate 1.

Compared to the earlier version 0.5.9, the following changes are included in this build:

**New features**
  - a native ARM64 build of WinFellow is now provided as part of the release zip archive

**Bug Fixes**
  - Thanks to the availability of Toni Wilen's cputester, the following mostly CPU related issues from the basic tests could be addressed:
    * Removed address register as byte source operand in move.
    * Various bit-field instruction fixes.
    * Incorrect pack/unpk behaviour.
    * Trapcc increase pc after condition check to get correct exception stack frame.
    * Cas2 select the first compare result value when both compare registers are the same register. Set v flag.
    * Link/Unlk incorrect result on stack when link register was a7.
    * Split long reads in two word reads for correct values across banks that are not stored consecutively.
    * VPOS wraparound fix.
    * Exception cycle time changes.
    * Fixed various instructions that overwrote exception cycle times with regular instruction time. (when triggering privilege violations etc.)
    * Set div cycle times to values closer to the listed cycle times. Still not dynamically calculated, but should be closer.
    * Don't trace when instruction was aborted by illegal, privilege or address error.
    * CHK.w N flag fix.
    * EOR ea vs. data register check for cycle calculation was inverted.
    * Changes to cycle calculation for BSET/BCGH.
    * Set undefined div flags according to behaviour on 68000.
    * Move to SR, check supervisor level before evaluating ea.
    * Include exception time in cycle time for chk.
    * Handle bkpt differenly from illegal.
    * Broken overflow and other special cases for mull.
    * Handle supervisor check differently for move from sr to avoid returning result when throwing exception.
    * Missing address mask in rtarea memory handling (for when upper address byte contains data).
    * Trapcc had wrong pc in the stack frame.
  - fixed mouse capture for older RetroPlatform hosts

**Maintenance updates**
  - code refactoring:
    * solution file reorganisation
    * static analysis/replace custom datatypes with standard sized C++ types

Please see the included file ChangeLog.txt for a full list of changes, including minor bug fixes which may not be listed above.
