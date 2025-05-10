	SECTION bss
irqvect RMB 2
	ENDSECTION
	
	SECTION code

; sy6551 registers
reg_rx EQU $FF04
reg_status EQU $FF05
reg_cmd EQU $FF06
reg_ctrl EQU $FF07

; install the ISR handler
_install_6551_int_handler
	; update the IRQ handler with ours
	ORCC #$50
	LDX $10D
	STX irqvect,pcr
	LEAX irq_handler,PCR
	STX $10D
	ANDCC #$EF
	; set 19200 baud, 2 stop bits
	LDA #159
	STA reg_ctrl
	; turn on the receiver & transmitter, enable RX interrupts
	LDA #5
	STA reg_cmd
	RTS

_install_6551_int_handler EXPORT

_restore_int_handler
	LDX irqvect,pcr
	STX $10D
	RTS
	
_restore_int_handler EXPORT

_rx_ring_buffer IMPORT
_ring_read_off IMPORT
_ring_write_off IMPORT
_ring_overruns IMPORT

irq_handler
	; check to see if interrupt is from the 6551, jump out if not
	LDA reg_status
	ANDA #8
	BEQ fin
	; fetch current write offset
	LDB _ring_write_off
	;STB 1024
	INCB
	; check to see if we've wrapped, if so drop the byte
	CMPB _ring_read_off
	BEQ drop
	; update the next write offset
	STB _ring_write_off
	; finally write in the data from the 6551
	LEAX _rx_ring_buffer,pcr
	DECB
	CLRA
	LEAX D,X
	LDA reg_rx
	STA ,X
	BRA fin
drop
	LDA reg_rx
	; update overrun counter
	INC _ring_overruns
	;INC 1028
fin	JMP [irqvect]
	
	ENDSECTION
	
