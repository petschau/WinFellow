fhfile_init:
	movem.l	d0-d7/a0-a6, -(a7)

	; -----------------------------------------------------
	; Load filesystems from RDB

	bsr		GetRDBFileSystemCount		; Check if there are RDB filesystems to load
	tst.l	d0
	beq.s	EndOfLoadRDBFileSystems

	bsr		LogAvailableResources	

	bsr		OpenFileSystemResource		; Open FileSystem.resource
	tst.l	d0
	bne.s	FileSystemResourceExists

	bsr		CreateFileSystemResource	; FileSystem.resource did not exist, create it
	bsr		OpenFileSystemResource
	tst.l	d0
	beq.s	EndOfLoadRDBFileSystems		; Still no FileSystem.resource, give up filesystems

FileSystemResourceExists:

	bsr		RemoveRDBFileSystemsAlreadySupportedBySystem
	bsr		AddRDBFileSystemEntries
	bsr		LogAvailableFileSystems

EndOfLoadRDBFileSystems:

	; -----------------------------------------------------
	; Make dos dev packets

MakeDosDevPackages:
	move.l	$4.w, a6			; Open expansion.library, ref in a4
	lea		explibname(pc), a1
	jsr		-408(a6)
	move.l	d0, a4
		
	lea		EndOfCode(pc), a0
	move.l	a0, d7
loop2:		
	move.l	d7, a0				; Loop through packets prepared by fhfile
	tst.l	(a0)
	bmi		loopend

	addq.l	#4, d7

	move.l	#88, d0				; Alloc mem for mkdosdev packet
	bsr		AllocMem
	move.l	d0, a5				; Memory for packet in a5

	move.l	d7, a0
			
	moveq	#84, d0
loop:
	move.l	(a0, d0.l), (a5, d0.l)	; Copy packet to a5
	subq.l	#4, d0
	bcc.s	loop

	exg		a4, a6				; KS 1.3 expects libary in a6....
	move.l	a5, a0				; MakeDosNode(a0 - packet) 
	jsr		-144(a6)
	exg		a4, a6				; Keep expansion.library so we can close it later

	bsr		PatchDOSDeviceNode	; D0 - DOSNode, A5 - packet
	move.l	d0, a3

	moveq.l	#20, d0				; Alloc memory for bootnode
	bsr		AllocMem

	move.l	d7, a1
	move.l	-4(a1), d6			; Our unit number	
	move.l	d0, a1				; Bootnode in a1
		
	moveq	#0, d0
	move.l	d0, (a1)			; Successor = 0
	move.l	d0, 4(a1)			; Predecessor = 0
	move.w	d0, 14(a1)			; bn_Flags
	move.w	#$10ff, 8(a1)		; Node type (0x10 bootnode) and priority
	sub.w	d6, 8(a1)			; Subtract unit no from priority
	move.l	$f40ffc, 10(a1)		; Configdev pointer (ln_Name ?)
	move.l	a3, 16(a1)			; Device node pointer (bn_DeviceNode)

	lea		74(a4), a0
	jsr		-270(a6)			; Enqueue()
	
	add.l	#88, d7
	bra		loop2
loopend:	
	
	move.l	$4.w, a6			; Close expansion.library
	move.l	a4, a1
	jsr		-414(a6)

initend:
	movem.l	(a7)+, d0-d7/a0-a6
	rts

;--------------------------------------------------------------------
LogAllocMemResult:
	move.l	#$000200a0, $f40000 ; Input D0 Allocated memory ptr
	rts

;--------------------------------------------------------------------
LogOpenResourceResult:
	move.l	#$000200a1, $f40000	; Input D0 Resource pointer
	rts

;--------------------------------------------------------------------
LogAvailableResources:
	move.l	#$000200a2, $f40000	; No input, reads resources from $150(execbase)
	rts

;--------------------------------------------------------------------
LogAvailableFileSystems:
	move.l	#$000200a3, $f40000	; Input D0 FileSystem.resource
	rts

;--------------------------------------------------------------------
GetRDBFileSystemCount:
	move.l	#$00020001, $f40000	; Returns D0 FileSystem count
	rts

;--------------------------------------------------------------------
GetRDBFileSystemHunkCount:
	move.l	#$00020002, $f40000	; Input D1 FileSystem index, Returns D0 Hunk count
	rts

;--------------------------------------------------------------------
GetRDBFileSystemHunkSize:
	move.l	#$00020003, $f40000	; Input D1 FileSystem index, D2 Hunk index, Returns D0 Hunk size
	rts

;--------------------------------------------------------------------
CopyRDBFileSystemHunk:
	move.l	#$00020004, $f40000	; Input D0 Memory allocated for hunk, D1 FileSystem index, D2 Hunk index
	rts

;--------------------------------------------------------------------
InitializeRDBFileSystemEntry:
	move.l	#$00020005, $f40000	; Input D0 Pointer to allocated memory
	rts

;--------------------------------------------------------------------
RemoveRDBFileSystemsAlreadySupportedBySystem:
	move.l	#$00020006, $f40000	; Input D0 Pointer to FileSystem.resource
	rts

;--------------------------------------------------------------------
PatchDOSDeviceNode:
	move.l	#$00020007, $f40000	; Input D0 Pointer to DOS device node, A5 Packet
	rts

;--------------------------------------------------------------------
RelocateFileSystem:
	move.l	#$00020008, $f40000	; Input D1 FileSystemIndex
	rts

;--------------------------------------------------------------------
AllocMem:
	movem.l d1-d4, -(a7)
	move.l	#$00010001, d1		; MEMF_PUBLIC, NULL out area before use
	move.l	$4.w, a6
	jsr		-198(a6)			; AllocMem()
	bsr		LogAllocMemResult
	movem.l	(a7)+, d1-d4
	rts

;--------------------------------------------------------------------
CreateFileSystemResource:
	move.l	#32, d0                ; Alloc mem for FileSystem.resource
	bsr		AllocMem
	move.l	d0, a5                 ; Memory for resource in a5
		
	move.b	#8, 8(a5)              ; Node type NT_RESOURCE
	lea		fsresourcename(pc), a0
	move.l	a0, 10(a5)             ; Node name "FileSystem.resource"
	lea		devicename(pc), a0
	move.l	a0, 14(a5)             ; Node creator name

	lea		18(a5), a4             ; Initialize filesystem entry list - Type 0
	move.l	a4, (a4)               ; Head - Address of tail pointer
	add.l	#4, (a4)
	clr.l	4(a4)                  ; Tail - Clear
	move.l	a4, 8(a4)              ; TailPred - Address of head pointer

	                               ; Add FileSystem.resource to exec resource list
	move.l	a5, a1                 ; a1 - FileSystem.resource
	jsr		-486(a6)               ; AddResource()
	rts

;--------------------------------------------------------------------
OpenFileSystemResource:
	move.l	$4.w, a6
	moveq.l	#0, d0
	lea		fsresourcename(pc), a1
	jsr		-498(a6)		; OpenResource(), Returns D0 - pointer to resource
	bsr		LogOpenResourceResult
	rts

;--------------------------------------------------------------------
; Allocate memory for hunks in the filesystem file image and relocate
; Input D1 - filesystem index
AllocateAndRelocateHunks:
	bsr		GetRDBFileSystemHunkCount
	tst.l	d0				; No hunks?
	beq		AllocateHunksEnd

	move.l	d0, d4			; Remember hunk count in D4
	moveq.l	#0, d2

AllocateHunksLoop:
	bsr		GetRDBFileSystemHunkSize
	tst.l	d0				; Skip hunk if zero size
	beq		AllocateNextHunk

	addq.l	#8, d0			; Make space for pointer to next segment and hunk size
	bsr		AllocMem
	bsr		CopyRDBFileSystemHunk
	
AllocateNextHunk:
	addq.l	#1, d2
	cmp.l	d2, d4
	bne		AllocateHunksLoop

	bsr		RelocateFileSystem
AllocateHunksEnd:
	rts

;--------------------------------------------------------------------
; Input A0 - FileSystem resource pointer
; Input D1 - FileSystem index
AddRDBFileSystemEntry:
	move.l	a0, -(a7)		; Save FileSystem resource pointer
	move.l	d1, -(a7)		; Save FileSystem index
	bsr		AllocateAndRelocateHunks	; SegList is recorded by native code

	move.l	#190, d0		; Alloc mem for FileSysEntry, is this size correct?
	bsr		AllocMem		; D0 - Pointer to FileSysEntry

	move.l	(a7)+, d1		; Restore FileSystem index
	bsr		InitializeRDBFileSystemEntry	; Input D0 - FileSysEntry, D1 - FileSystem index

	move.l	$4, a6			; Add FileSysEntry to filesystem list
	move.l	(a7), a0		; Get FileSystem resource pointer
	lea		18(a0), a0		; Pointer to FileSystem list in a0
	move.l	d0, a1			; Pointer to FileSysEntry in a1
	jsr		-240(a6)		; AddHead()

	move.l	(a7)+, a0		; Restore FileSystem resource pointer
	rts
		
;--------------------------------------------------------------------
; Input D0 - FileSystem resource pointer
AddRDBFileSystemEntries:
	move.l	d0, -(a7)		; Save FileSystem resource pointer
	move.l	d0, a0          ; Remember FileSystem.resource in A0

	bsr		GetRDBFileSystemCount
	tst.l	d0
	beq		EndOfAddRDBFileSystemEntries

	move.l	d0, d3          ; Remember total filesystem count in D3
	moveq.l	#0, d1

AddRDBFileSystemEntriesLoop:
	bsr		AddRDBFileSystemEntry

	addq.l	#1, d1
	cmp.l	d1, d3
	bne		AddRDBFileSystemEntriesLoop

EndOfAddRDBFileSystemEntries:
	move.l	(a7)+, d0		; Restore FileSystem resource pointer
	rts

;--------------------------------------------------------------------
explibname:	dc.b "expansion.library",0
fsresourcename:	dc.b "FileSystem.resource",0
devicename: dc.b "Fellow hardfile device",0
even
EndOfCode:
