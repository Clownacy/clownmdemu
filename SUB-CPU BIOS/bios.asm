	dc.l	$5E80,           EntryPoint,      ErrorTrap,       $5F40
	dc.l	$5F46,           $5F4C,           $5F52,           $5F58
	dc.l	$5F5E,           $5F64,           $5F6A,           $5F70
	dc.l	ErrorTrap,       ErrorTrap,       ErrorTrap,       ErrorTrap
	dc.l	ErrorTrap,       ErrorTrap,       ErrorTrap,       ErrorTrap
	dc.l	ErrorTrap,       ErrorTrap,       ErrorTrap,       0		; Done to align "SEGA" at right spot when compressed
	dc.l	ErrorTrap,       $5F76,           $5F7C,           $5F82
	dc.l	$5F88,           $5F8E,           $5F94,           $5F9A
	dc.l	$5FA0,           $5FA6,           $5FAC,           $5FB2
	dc.l	$5FB8,           $5FBE,           $5FC4,           $5FCA
	dc.l	$5FD0,           $5FD6,           $5FDC,           $5FE2
	dc.l	$5FE8,           $5FEE,           $5FF4,           $5FFA
	dc.l	ErrorTrap,       ErrorTrap,       ErrorTrap,       ErrorTrap
	dc.l	ErrorTrap,       ErrorTrap,       ErrorTrap,       ErrorTrap
	dc.l	ErrorTrap,       ErrorTrap,       ErrorTrap,       ErrorTrap
	dc.l	ErrorTrap,       ErrorTrap,       ErrorTrap,       ErrorTrap

	dc.b	"SEGA"

ErrorTrap:
	nop
	nop
	bra.s	ErrorTrap

NullException:
	rte

NullCaller:
	rts

EntryPoint:
	moveq	#0,d0
	move.b	d0,($FFFF800F).w
	move.l	d0,($FFFF8020).w
	move.l	d0,($FFFF8024).w
	move.l	d0,($FFFF8028).w
	move.l	d0,($FFFF802C).w
	move.b	d0,($FFFF8033).w

	lea	$5F0A,a0
	move.w	#$4EF9,d0
	move.w	d0,(a0)+
	move.l	#NullCaller,(a0)+		; _SETJMPTBL
	move.w	d0,(a0)+
	move.l	#VSync,(a0)+			; _WAITVSYNC
	
	moveq	#7-1,d1				; Callers

.setcallers:
	move.w	d0,(a0)+
	move.l	#NullCaller,(a0)+
	dbf	d1,.setcallers

	moveq	#9-1,d1				; Error exceptions

.seterrors:
	move.w	d0,(a0)+
	move.l	#ErrorTrap,(a0)+
	dbf	d1,.seterrors
	
	move.w	d0,(a0)+			; _LEVEL1
	move.l	#NullException,(a0)+
	move.w	d0,(a0)+			; _LEVEL2
	move.l	#Level2Interrupt,(a0)+

	moveq	#21-1,d1			; Set interrupts and TRAPs

.setinttraps:
	move.w	d0,(a0)+
	move.l	#NullException,(a0)+
	dbf	d1,.setinttraps

.waitsp:
	lea	$6000(pc),a0			; Wait for system program to be loaded
	cmpi.l	#'MAIN',(a0)
	bne.s	.waitsp
	
	ori.b	#%00010100,($FFFF8033).w	; Enable Mega Drive and CDD interrupts

	adda.l	$18(a0),a0			; Set up user callers
	move.l	a0,d1
	lea	$5F2A,a2

.setusercalls:
	moveq	#0,d0
	move.w	(a0)+,d0
	beq.s	.userscallsdone
	add.l	d1,d0
	move.l	d0,(a2)+
	addq.w	#2,a2
	bra.s	.setusercalls

.userscallsdone:
	movem.l	.zero(pc),d0-a6			; Clear registers

.init:
	move.w	#$2200,sr			; Initialize system program
	bsr.w	$5F28
	move.w	#$2000,sr

.loop:
	move.w	$5EA2,d0			; Run system program
	bsr.w	$5F2E
	move.w	d0,$5EA2
	bsr.w	VSync

	cmpi.w	#-1,$5EA2			; Check return code
	bne.s	.loop
	
	movem.l	.zero(pc),d0-a6			; Re-initialize
	bra.s	.init

.zero:
	dcb.l	7+6, 0

Level2Interrupt:
	movem.l	d0-a6,-(sp)
	movea.w	#0,a5
	bsr.w	$5F34
	bclr	#0,$5EA4
	movem.l	(sp)+,d0-a6
	rte

VSync:
	bset	#0,$5EA4

.wait:
	btst	#0,$5EA4
	bne.s	.wait
	rts