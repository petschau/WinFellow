fhfile_init:	movem.l	d0-d7/a0-a6,-(a7)
		move.l	$4.w,a6			; Open explib
		lea	explibname(pc),a1
		jsr	-408(a6)
		move.l	d0,a4
		
		lea	endofcode(pc),a0
		move.l	a0,d7
loop2:		
		move.l	d7,a0
		tst.l	(a0)
		bmi	loopend
		addq.l	#4,d7
		move.l	#88,d0			; Alloc mem for
		moveq	#1,d1			; mkdosdev packet
		move.l	$4.w,a6
		jsr	-198(a6)
		move.l	d0,a5

		move.l	d7,a0
			
		moveq	#84,d0
loop:		move.l	(a0,d0.l),(a5,d0.l)
		subq.l	#4,d0
		bcc.s	loop

		move.l	a5,a0			; make dos node 
		jsr	-144(a4)				

		move.l	d0,a3
		moveq	#0,d0
		move.l	d0,8(a3)
		move.l	d0,16(a3)
		move.l	d0,32(a3)

		move.l	$4.w,a6
		moveq	#20,d0
		moveq	#0,d1
		jsr	-198(a6)                ; Alloc


		move.l	d7,a1
		move.l	-4(a1),d6		
		move.l	d0,a1	;  Bootnode
		
		moveq	#0,d0
		move.l	d0,(a1)
		move.l	d0,4(a1)
		move.w	d0,14(a1)
		move.w	#$10ff,8(a1)
		sub.w	d6,8(a1)
		move.l	$f40ffc,10(a1)
		move.l	a3,16(a1)
		lea	74(a4),a0
		jsr	-270(a6)
	
		add.l	#88,d7
		bra	loop2
loopend:	
	
;		move.l	d0,a0			; add dos node
;		moveq.l	#-1,d0
;		move.l	d0,a1
;		jsr	-150(a4)

		move.l	$4.w,a6			; Close explib
		move.l	a4,a1
		jsr	-414(a6)

		movem.l (a7)+,d0-d7/a0-a6
		rts

explibname:	dc.b "expansion.library",0
even
endofcode:
