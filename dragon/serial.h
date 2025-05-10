#ifndef SERIAL_H

#define SERIAL_H

#include "uip.h"

// interrupt driven serial driver for the 6551

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

#endif
