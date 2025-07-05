#include <cmoc.h>
#include "serial.h"

/* Interrupt driven serial driver for the 6551. This  
   provides a configurable receive ring buffer which is populated by
   the interrupt handler. 
   
   When used with the emulator, you need to throttle back the
   transmit rate through the RX FIFO (see 'tap-slip-gw) otherwise
   you'll overwhelm it and trigger massive overruns. Using a 
   figure of 1000 as the tx-delay(us) seems to yield roughly the 
   same ping times as when connected to a real Dragon @19200 baud.

   Below comments originate from the original, fixed 256 byte ring
   buffer, in theory should be even better now, even with 3 wire serial:
      
   On the real hardware, with 3-wire serial this works quite nicely
   99% of the time, where sporadic overruns start showing up is when 
   the Dragon performs large packet receives/sends at the same time eg.
   doing 'ping' requests with increased payload sizes although
   it's really not until you hit the 1KB threshold they start
   becoming more prominent. In part, this is helped by the
   tap-slip-gw app which is single threaded and blocks till a
   transmit packet has fully been received ie. no sends will
   be occurring during that window. So it's very much down to
   timing as in a large packet has just been sent at the point
   the Dragon is commencing a similarly large response. 
   
   If hardware flow control is used (DTR from the Dragon connected to
   CTS on the Linux side) AND enabled on the Linux side, the SLIP
   driver de-asserts DTR when performing a send of a packet greater
   than half the ring buffer size which should stop the Linux side
   sending during that period. In this mode, I've seen no overruns */

// DTR bit in the control 6551 cmd register
#define CMD_DTR (1)

static u8_t *sy6551_holding = (u8_t*)0xff04 ;
static u8_t *sy6551_status = (u8_t*)0xff05 ;
static u8_t *sy6551_cmd = (u8_t*)0xff06 ;
static u8_t dtr_c ;

// most of the functionality has now been moved into assembler...
extern void install_6551_int_handler(void) ;
extern void restore_int_handler(void) ;

#if 0
int main(void)
{
  u8_t overruns ;
  u8_t *p = (u8_t*)1056 ;
  u8_t c ;
  
  serial_init() ;
  
  while(1)
  {
    c = serial_get() ;
    //*p = ring_read_off ;
    printf("%c", c ) ;
  }

  //restore_int_handler() ;
  return 0 ;
}
#endif

void serial_init(void)
{
  // setup ring buffer pointers
  u8_t **p_rx_ring_buffer = (u8_t**)0xe6 ;
  u8_t **p_rx_ring_buffer_end = (u8_t**)0xe8 ;
  
#ifndef RX_RING_BUFFER_PTR
  // locate the ring buffer in the first graphics page  
  u8_t **p_graphics_base = (u8_t**)0xba ;
  *p_rx_ring_buffer = *p_graphics_base ;
#else
  *p_rx_ring_buffer = (u8_t**)RX_RING_BUFFER_PTR ;
#endif
  *p_rx_ring_buffer_end = *p_rx_ring_buffer + RX_RING_BUFZ ;
  
  install_6551_int_handler() ;
}

void set_dtr(void)
{
  dtr_c-- ;
  if ( !dtr_c )
    *sy6551_cmd = (*sy6551_cmd) | CMD_DTR ;
}

void clear_dtr(void)
{
#ifdef HW_FLOW_CONTROL
  if ( !dtr_c )
    *sy6551_cmd = (*sy6551_cmd) & (~CMD_DTR) ;
#endif
  dtr_c++ ;
}
