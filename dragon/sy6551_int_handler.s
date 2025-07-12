	SECTION bss
irqvect   RMB 2
	ENDSECTION
	
	SECTION code

 INCLUDE "serial_defs.s"

; install the ISR handler
_install_6551_int_handler
	; update the IRQ handler with ours
	ORCC #$50
	LEAX irq_handler,PCR
	; don't update it if already set (warm start)
	CMPX $10D
	BEQ isrset
	LDD $10D
	STD irqvect,pcr
	STX $10D
isrset
	ANDCC #$EF
	; set 19200 baud, 2 stop bits
	LDA #159
	STA reg_ctrl
	; turn on the receiver & transmitter, enable RX interrupts
	LDA #9
	STA reg_cmd
	RTS

_install_6551_int_handler EXPORT

_restore_int_handler
	LDX irqvect,pcr
	STX $10D
	RTS
	
_restore_int_handler EXPORT

irq_handler
	;LDA reg_status
	;ANDA #4
	;BEQ nrun
	;INC ring_overruns
nrun
	; check to see if interrupt is from the 6551, jump out if not
	LDA reg_status
	ANDA #8
	BEQ fin
	; read data from the ACIA
	LDA reg_rx
	; fetch current write pointer, increment
	LDX <ring_write_ptr
	STA ,X+
	; check for end of ring buffer
	CMPX <rx_ring_buffer_end
	BNE nring
	LDX <rx_ring_buffer
nring
	;STX 1024
	; check to see if we've wrapped, if so drop the byte
	CMPX <ring_read_ptr
	BEQ drop
	; update the next write offset
	STX <ring_write_ptr
	; stay in the handler until a non ACIA interrupt kicks us out
	SYNC
	;LDA $ff02
	BRA irq_handler
	;BRA fin
drop
	; update overrun counter
	INC <ring_overruns
	;INC 1028
fin	JMP [irqvect]

; test for serial data in the ring buffer
_serial_rx_pending
	CLRB
	LDX <ring_read_ptr
	CMPX <ring_write_ptr
	BEQ rx_empty
	INCB
rx_empty
	RTS

_serial_rx_pending EXPORT
	
; fetch no of serial overruns
_serial_overruns
	LDB <ring_overruns
	CLR <ring_overruns
	RTS

_serial_overruns EXPORT

	ENDSECTION
	
