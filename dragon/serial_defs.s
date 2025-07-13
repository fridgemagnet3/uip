
; serial definitions for assembler modules

; 6551 registers
reg_rx     EQU $FF04
reg_status EQU $FF05
reg_cmd    EQU $FF06
reg_ctrl   EQU $FF07
reg_tx	   EQU reg_rx

; store ring buffer pointers in zero page for efficiency
rx_ring_buffer     EQU $E6
rx_ring_buffer_end EQU $E8
ring_read_ptr      EQU $EA
ring_write_ptr     EQU $EC
ring_overruns      EQU $EE
