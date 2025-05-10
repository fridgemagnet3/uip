#include <cmoc.h>
#include "serial.h"

/* Interrupt driven serial driver for the 6551. This  
   provides a 256 byte receive ring buffer which is populated by
   the interrupt handler. 
   
   When used with the emulator, you need to throttle back the
   transmit rate through the RX FIFO (see 'tap-slip-gw) otherwise
   you'll overwhelm it and trigger massive overruns. Using a 
   figure of 1000 as the tx-delay(us) seems to yield roughly the 
   same ping times as when connected to a real Dragon @19200 baud
   however even then you'll see get sporadic overruns, particularly
   during the burst of traffic at startup.
   
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

// status bits for TX full in the 6551
#define STAT_TX (1<<4)
#define CMD_DTR (1)

static u8_t *sy6551_holding = (u8_t*)0xff04 ;
static u8_t *sy6551_status = (u8_t*)0xff05 ;
static u8_t *sy6551_cmd = (u8_t*)0xff06 ;

// RX ring buffer 
u8_t rx_ring_buffer[RX_RING_BUFZ] ;
// read pointer in ring buffer
u8_t ring_read_off = 0u ;
// write pointer in ring buffer
u8_t ring_write_off = 0u ;
// no. of overruns detected
u8_t ring_overruns = 0u;

extern void install_6551_int_handler(void) ;
extern void restore_int_handler(void) ;
#if 0
int main(void)
{
  u8_t overruns ;
  u8_t *p = (u8_t*)1056 ;
  u8_t c ;
  
  install_6551_int_handler() ;
  
  printf ( "rx_ring_buffer: %x\n", (u16_t)rx_ring_buffer) ;
  printf( "ring_write_off: %x\n", (u16_t)&ring_write_off) ;
  printf( "ring_overruns: %x\n", (u16_t)&ring_overruns) ;

  while(1)
  {
    c = serial_get() ;
    *p = ring_read_off ;
    //printf("%c", c ) ;
  }

  //restore_int_handler() ;
  return 0 ;
}
#endif

void serial_init(void)
{
  install_6551_int_handler() ;
}

u8_t serial_rx_pending(void)
{
  return (ring_write_off != ring_read_off) ; 
}

u8_t serial_tx_empty(void)
{
  return (*sy6551_status) & STAT_TX ; 
}

// transmit works the same as in polled mode
void serial_put(u8_t c)
{
  while(!serial_tx_empty() )
  {
  }
  *sy6551_holding = c ;
}

u8_t serial_get(void)
{
  u8_t c ;
  
  while(ring_write_off == ring_read_off)
  {
  }
  c = rx_ring_buffer[ring_read_off++] ;
  
  return c ;
}

u8_t serial_overruns(void)
{
  u8_t c = ring_overruns ;
  ring_overruns = 0u ;
  return c ;
}

void set_dtr(void)
{
  *sy6551_cmd = (*sy6551_cmd) | CMD_DTR ;
}

void clear_dtr(void)
{
  *sy6551_cmd = (*sy6551_cmd) & (~CMD_DTR) ;
}
