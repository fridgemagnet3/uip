	SECTION bss
irqvect RMB 2
	ENDSECTION
	
	SECTION code

; sy6551 registers
reg_rx     EQU $FF04
reg_status EQU $FF05
reg_cmd    EQU $FF06
reg_ctrl   EQU $FF07
reg_tx	   EQU reg_rx

; store ring buffer pointer in zero page for efficiency
_rx_ring_buffer     EQU $E6
_rx_ring_buffer_end EQU $E8
_ring_read_ptr      EQU $EA
_ring_write_ptr     EQU $EC
_ring_overruns      EQU $EE

; install the ISR handler
_install_6551_int_handler
	; setup the ring buffer pointers
	LDX <_rx_ring_buffer
	STX <_ring_read_ptr
	STX <_ring_write_ptr
	CLR <_ring_overruns
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

_rx_ring_buffer EXPORT
_rx_ring_buffer_end EXPORT
_ring_read_ptr EXPORT
_ring_write_ptr EXPORT
_ring_overruns EXPORT

irq_handler
	;LDA reg_status
	;ANDA #4
	;BEQ nrun
	;INC _ring_overruns
nrun
	; check to see if interrupt is from the 6551, jump out if not
	LDA reg_status
	ANDA #8
	BEQ fin
	; read data from the ACIA
	LDA reg_rx
	; fetch current write pointer, increment
	LDX <_ring_write_ptr
	STA ,X+
	; check for end of ring buffer
    CMPX <_rx_ring_buffer_end
    BNE nring
    LDX <_rx_ring_buffer
nring
	;STX 1024
	; check to see if we've wrapped, if so drop the byte
	CMPX <_ring_read_ptr
	BEQ drop
	; update the next write offset
	STX <_ring_write_ptr
	; stay in the handler until a non ACIA interrupt kicks us out
	SYNC
	BRA irq_handler
drop
	; update overrun counter
	INC <_ring_overruns
	;INC 1028
fin	JMP [irqvect]

; test for serial data in the ring buffer
_serial_rx_pending
	LDD <_ring_read_ptr
	SUBD <_ring_write_ptr
	RTS

_serial_rx_pending EXPORT

; fetch no of serial overruns
_serial_overruns
	LDB <_ring_overruns
	CLR <_ring_overruns
	RTS

_serial_overruns EXPORT

; fetch next byte from the ring buffer
; spin if none available
_serial_get
	LDX <_ring_read_ptr
	CMPX <_ring_write_ptr
	BEQ _serial_get
	; fetch next byte from ring buffer
	; return in B
	LDB ,X+
	; text for wrap
    CMPX <_rx_ring_buffer_end
    BNE nrout
    LDX <_rx_ring_buffer
nrout
	STX <_ring_read_ptr
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
	
