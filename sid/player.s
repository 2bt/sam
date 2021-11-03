;	!to	"sid.sid", plain
;	!cpu	6510

load	= $4000
*	= load - $7e
pos	= $fb

	; sid header
	.byt	"PSID"
	.byt	$00, $02
	.byt	$00, $7C
	.byt	$00, $00
	.byt	>init, <init
	.byt	>play, <play
	.byt	$00, $01, $00, $01
	.byt	$00, $00, $00, $00
title
	.byt	"S.A.M. test"
	.dsb	title + 32 - *, 0
	.byt	"2bt"
	.dsb	title + 64 - *, 0
	.byt	"2021"
	.dsb	title + 96 - *, 0
	.byt	$00, $24
	.byt	$00, $00, $00, $00
	.byt	<load, >load

init
	; filter type & volume
	lda	#$2f
	sta	$d418

	; adsr
	lda	#$00
	sta	$d405
	sta	$d40c
	sta	$d413

	lda	#$30
	sta	$d406
	lda	#$f0
	sta	$d40d
	lda	#$40
	sta	$d414

	; wave
	lda	#$11
	sta	$d404
	lda	#$13
	sta	$d40b


initpos
	; init pos
	lda	#<data
	sta	pos
	lda	#>data
	sta	pos + 1
return
	rts

play
	ldy	#0

	; voice 1 freq
	lda	(pos), y
	tax
	lda	freqlo, x
	sta	$d400
	lda	freqhi, x
	sta	$d401
	iny


	; voice 2 freq
	lda	(pos), y
	tax
	lda	freqlo, x
	sta	$d407
	lda	freqhi, x
	sta	$d408
	iny

	; voice 3 freq
	lda	(pos), y
	tax
	lda	freqlo, x
	sta	$d40e
	lda	freqhi, x
	sta	$d40f
	iny

	; voice 3 filter
	lda	(pos), y
	sta	$d416
	beq	filter1
	ldx	#$f4
	lda	#$81
	jmp	filter2
filter1
	ldx	#$f0
	lda	#$13
filter2
	stx	$d417
	sta	$d412
	iny


	; update pos
	tya
	adc	pos
	sta	pos
	bcc	done
	inc	pos + 1

done
	lda	pos + 1
	cmp	#>dataend
	bcc	return
	bne	initpos
	lda	pos
	cmp	#<dataend
	bcc	return
	jmp	initpos

