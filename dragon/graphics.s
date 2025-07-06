; Utilities for configuring graphics mode
; adapted from the book, Inside the Dragon.
;

; VDG/PIA and SAM addresses
VDGPIA EQU $FF22 ; Port B of PIA - VDG control 
SAMVOC EQU $FFC0 ; Used to clear V0 
SAMV0S EQU $FFC1 ; Used to set V0 
SAMV1C EQU $FFC2 ; Used to clear V1
SAMV1S EQU $FFC3 ; Used to set V1 
SAMV2C EQU $FFC4 ; Used to clear V2 
SAMV2S EQU $FFC5 ; Used to set V2 
SAMF0C EQU $FFC6 ; Base address of F0-F6 

; full graphics modes
MODE1C EQU $80 ; Graphics 1C 
MODE1R EQU $90 ; Graphics 1R 
MODE2C EQU $A0 ; Graphics 2C 
MODE2R EQU $B0 ; Graphics 2R 
MODE3C EQU $C0 ; Graphics 3C 
MODE3R EQU $D0 ; Graphics 3R 
MODE6C EQU $E0 ; Graphics 6C 
MODE6R EQU $F0 ; Graphics 6R

	SECTION code

; sets up VDG chip
; A = config bit pattern
vdgmod 
	PSHS A ; Preserve setup pattern 
	LDA VDGPIA ; Preserve bottom bits 
	ANDA #7 ; of PIA register 
	ORA ,S ; Or in setup pattern 
	STA VDGPIA ; Setup VDG 
	PULS A,PC ; Restore and return 

; sets up SAM chip
; A = config bit pattern
sammod 
	PSHS A ; Preserve VDG pattern 
	STA SAMVOC ; Clear V0 
	STA SAMV1C ; Clear V1 
	STA SAMV2C ; Clear V2 
	ANDA #$F0 ; Clear bottom 4 bits of A 
	BPL NOTGM0 ; Text mode (B7=0) 
	CMPA #MODE1C ; no, is it 1C? 
	BNE NOT1C 
	ORA #$10 ; yes, special case->lR 
NOT1C 
	CMPA #MODE6R ; Is it 6R 
	BNE NOT6R 
	ANDA #$E0 ; yes, special case->6C 
NOT6R 
	ROLA ; Get rid of A/G bit 
	BPL NOTGM2 ; GM2 set? 
	STA SAMV2S ; yes, set V2 
NOTGM2 
	ROLA ; get rid of GM2 bit 
	BPL NOTGMl ; GM1 set 
	STA SAMV1S ; yes, set V1 
NOTGMl 
	ROLA ; get rid of GM1 bit 
	BPL NOTGM0 ; GM0 set? 
	STA SAMV0S ; yes, set V0 
NOTGM0 
	PULS A,PC ; Restore and return
	
; setup graphics hardware
; void gmode(unsigned char mode)
_gmode
	LDA 3,S
	BSR vdgmod
	BSR sammod
	RTS

_gmode EXPORT

; select colour set (0/1)
; void css(unsigned char colour_set)
_css
	LDA VDGPIA ; Read current state of VDG 
	TST 3,S ; Check set selection 
	BEQ CSS0 
	ORA #8 ; Set the CSS line 
	BRA XIT 
CSS0 
	ANDA #$F7 ; Clear the CSS line 
XIT 
	STA VDGPIA ; update the VDG 
	RTS

_css EXPORT
	
; setup graphics segment base address
; void pagex(void *page_addr)
_pagex
	LDA 2,S ; A = (HI), B = (LO, ignored) 
	LSRA ; We only need top 7 bits 
	LDB #7 ; as specified in B 
	LDX #SAMF0C ; Copy to F0-F6 

; configure SAM control bits
; X = address in SAM control register
; A = SAM configuration bit pattern
; B = No. of bits to be copied from A to SAMCR
samset 
	LSRA ; Shift bit 0 to carry 
	BCC NOTSET ; Set corresponding CR bit?
	LEAX 1,X ; Yes, odd address 
	STA ,X+ ; set bit and adjust X for 
	BRA CHKCNT ; next address 
NOTSET 
	STA ,X++ ; Clear bit and continue 
CHKCNT 
	DECB ; All done? 
	BNE samset 
	RTS 
	
_pagex EXPORT

	ENDSECTION
