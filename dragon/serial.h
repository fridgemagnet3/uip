#ifndef SERIAL_H

#define SERIAL_H

#include "uip.h"

// interrupt driven serial driver for the 6551

// sizeof the serial RX ring buffer
// by default, on a vanilla Dragon this will end at $c40
// on a DOS system, $1240
#define RX_RING_BUFZ 1600

// if you want to locate the ring buffer at a specific
// address, set it here otherwise it will be located in
// the first graphics page
#undef RX_RING_BUFFER_PTR

// set to use hardware flow control (DTR), if undefined
// 'clear_dtr' (below) does nothing
#define HW_FLOW_CONTROL

void serial_init(void) ;

// indicates if RX data is available
u8_t serial_rx_pending(void) ;

// indicates if the TX output is empty
u8_t serial_tx_empty(void) ;

// write a byte
void serial_put(u8_t c) ;

// get a byte
u8_t serial_get(void) ;

// return no. of RX overruns
u8_t serial_overruns(void) ;

// return no. of bytes pending in the rx ring buffer
u16_t serial_rx_ring_buffer_used(void) ;

// assert DTR
void set_dtr(u8_t force) ;

// de-assert DTR 
void clear_dtr(void) ;

#endif
