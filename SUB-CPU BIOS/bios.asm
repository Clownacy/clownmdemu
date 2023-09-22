	dc.l	$00005000,       EntryPoint,      ErrorTrap,       ErrorTrap
	dc.l	ErrorTrap,       ErrorTrap,       ErrorTrap,       ErrorTrap
	dc.l	ErrorTrap,       ErrorTrap,       ErrorTrap,       ErrorTrap
	dc.l	ErrorTrap,       ErrorTrap,       ErrorTrap,       ErrorTrap
	dc.l	ErrorTrap,       ErrorTrap,       ErrorTrap,       ErrorTrap
	dc.l	ErrorTrap,       ErrorTrap,       ErrorTrap,       ErrorTrap
	dc.l	ErrorTrap,       Level1Interrupt, Level2Interrupt, Level3Interrupt
	dc.l	Level4Interrupt, Level5Interrupt, Level6Interrupt, Level7Interrupt
	dc.l	ErrorTrap,       ErrorTrap,       ErrorTrap,       ErrorTrap
	dc.l	ErrorTrap,       ErrorTrap,       ErrorTrap,       ErrorTrap
	dc.l	ErrorTrap,       ErrorTrap,       ErrorTrap,       ErrorTrap
	dc.l	ErrorTrap,       ErrorTrap,       ErrorTrap,       ErrorTrap
	dc.l	ErrorTrap,       ErrorTrap,       ErrorTrap,       ErrorTrap
	dc.l	ErrorTrap,       ErrorTrap,       ErrorTrap,       ErrorTrap
	dc.l	ErrorTrap,       ErrorTrap,       ErrorTrap,       ErrorTrap
	dc.l	ErrorTrap,       ErrorTrap,       ErrorTrap,       ErrorTrap

ErrorTrap:
	nop
	nop
	bra.w	ErrorTrap

Level1Interrupt:
Level3Interrupt:
Level4Interrupt:
Level5Interrupt:
Level6Interrupt:
Level7Interrupt:
	rte

EntryPoint:
	lea	$6000(pc),a0
	cmpi.l	#'MAIN',(a0)
	bne.s	EntryPoint
	adda.l	$18(a0),a0
	move.w	(a0),d0
	jsr	(a0,d0.w)
	move.w	#$2700,sr	; Enable interrupts
	lea	$6000(pc),a0
	adda.l	$18(a0),a0
	move.w	2(a0),d0
	jmp	(a0,d0.w)

Level2Interrupt:
	lea	$6000(pc),a0
	cmpi.l	#'MAIN',(a0)
	bne.s	.return
	adda.l	$18(a0),a0
	move.w	4(a0),d0
	jsr	(a0,d0.w)

.return:
	rte
