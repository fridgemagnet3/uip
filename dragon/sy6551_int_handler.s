	SECTION bss
irqvect   RMB 2
ringbufsz RMB 2
	ENDSECTION
	
	SECTION code

; sy6551 registers
reg_rx     EQU $FF04
reg_status EQU $FF05
reg_cmd    EQU $FF06
reg_ctrl   EQU $FF07
reg_tx	   EQU reg_rx

; store ring buffer pointer in zero page for efficiency
rx_ring_buffer     EQU $E6
rx_ring_buffer_end EQU $E8
ring_read_ptr      EQU $EA
ring_write_ptr     EQU $EC
ring_overruns      EQU $EE

; install the ISR handler
_install_6551_int_handler
	; setup the ring buffer pointers
	LDX <rx_ring_buffer
	STX <ring_read_ptr
	STX <ring_write_ptr
	CLR <ring_overruns
	LDD <rx_ring_buffer_end
	SUBD <rx_ring_buffer
	STD ringbufsz
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
	BRA irq_handler
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
	
; get amount of space used
_serial_rx_ring_buffer_used
	LDD <ring_write_ptr
	SUBD <ring_read_ptr
	BCC rused_out
	; read pointer ahead of write pointer
	LDD ringbufsz
	SUBD <ring_read_ptr
	ADDD <ring_write_ptr
rused_out
	RTS

_serial_rx_ring_buffer_used EXPORT

; fetch no of serial overruns
_serial_overruns
	LDB <ring_overruns
	CLR <ring_overruns
	RTS

_serial_overruns EXPORT

; fetch next byte from the ring buffer
; spin if none available
_serial_get
	LDX <ring_read_ptr
	CMPX <ring_write_ptr
	BEQ _serial_get
	; fetch next byte from ring buffer
	; return in B
	LDB ,X+
	; text for wrap
    CMPX <rx_ring_buffer_end
    BNE nrout
    LDX <rx_ring_buffer
nrout
	STX <ring_read_ptr
	RTS

_serial_get EXPORT

; test for tx reg empty
_serial_tx_empty
	LDB reg_status
	; return in B
	ANDB #$10
	RTS

_serial_tx_empty EXPORT

; tx a byte	
_serial_put
	LDB reg_status
	ANDB #$10
	BEQ _serial_put
	; fetch param on stack
	LDA 3,S
	STA reg_tx
	RTS

_serial_put EXPORT

	ENDSECTION
	
