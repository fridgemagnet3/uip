	SECTION code

; copy BASIC ROM to RAM, then switch to 64K map mode
_switch_mapmode
	ORCC #$50
	LDX #$8000
copyrom:
	LDB ,X
	STA $FFDF
	STB ,X+
	STA $FFDE
	CMPX #$E000
	BNE copyrom
	STA $FFDF
	ANDCC #$EF
	RTS

_switch_mapmode EXPORT

	ENDSECTION
