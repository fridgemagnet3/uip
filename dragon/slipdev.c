/*
 * Copyright (c) 2001, Adam Dunkels.
 *
 * Heavily modified for use with SY6551 ACIA on Dragon 64 by Jon Bird 
 * (there's really not much left of the original!)
 *
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Adam Dunkels.
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.  
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: slipdev.c,v 1.1 2001/11/20 20:49:45 adam Exp $
 *
 */

#include "slipdev.h"
#include "serial.h"
#include <cmoc.h>

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

#define MAX_SIZE UIP_BUFSIZE

static const unsigned char slip_end = SLIP_END, 
                           slip_esc = SLIP_ESC, 
                           slip_esc_end = SLIP_ESC_END, 
                           slip_esc_esc = SLIP_ESC_ESC;

// DTR bit in the 6551 cmd register
#define CMD_DTR (1)
// receive data available in the status register
#define STAT_RX (1<<3)

static u8_t *sy6551_holding = (u8_t*)0xff04 ;
static u8_t *sy6551_status = (u8_t*)0xff05 ;
static u8_t *sy6551_cmd = (u8_t*)0xff06 ;

extern void install_6551_int_handler(void) ;

// byte to write passed in A
asm serial_put(void)
{
  asm
  {
    INCLUDE "serial_defs.s"
    ; spin till TX reg ready
    ldb reg_status
    andb #$10
    beq serial_put
    ; tx byte and return
    sta reg_tx
    rts
  }
}

/*-----------------------------------------------------------------------------------*/
void
slipdev_send(void)
{
#ifdef SERIAL_DRIVER
  // deassert DTR for large sends
  // to reduce overruns
  if ( uip_len > (UIP_CONF_BUFFER_SIZE/3) )
    clear_dtr() ;
#endif

  asm
  {
    ; serial_put(slip_end);
    pshs y
    lda :slip_end
    bsr serial_put
    ; x (ptr) = uip_buf, y = uip_len
    ldy :uip_len
    ; need to increment by 1 since we do
    ; the decrement at the start of the loop
    ; and test for zero
    leay 1,y
    leax :uip_buf
    ; for(i = 0; i < uip_len; i++)
tx_lp:
    leay -1,y
    beq tx_end
    ; a = *ptr++;
    lda ,x+
    ; switch(a)
    ;   case SLIP_END:
    cmpa :slip_end
    bne nslip_end
    ; serial_put(slip_esc);
    lda :slip_esc
    bsr serial_put
    ; serial_put(slip_esc_end);
    lda :slip_esc_end
    bsr serial_put
    ; break
    bra tx_lp
    ; case SLIP_ESC:
nslip_end:
    cmpa :slip_esc
    bne nslip_esc
    ; serial_put(slip_esc);
  	lda :slip_esc
  	bsr serial_put
  	; serial_put(slip_esc_esc);
  	lda :slip_esc_esc
  	bsr serial_put
  	; break 
  	bra tx_lp
nslip_esc:
	; default:
	; serial_put(c);
	bsr serial_put
	bra tx_lp
tx_end:
    lda :slip_end
    bsr serial_put
    puls y
  }

#ifdef SERIAL_DRIVER
  if ( uip_len > (UIP_CONF_BUFFER_SIZE/3) )
    set_dtr() ;
#endif
}

/*-----------------------------------------------------------------------------------*/
unsigned int slipdev_read(void)
{
  u8_t c;

 if ( !serial_rx_pending() )
   return 0 ;
 
#ifdef SERIAL_DRIVER
  u8_t overruns = serial_overruns() ;
  if ( overruns )
    printf("serial overruns: %u\n", overruns ) ;
#endif

  asm
  {
    pshs y,u
start:
    ; uip_len = 0
    ldy #0
    ; x is used by serial_get
    ; so use u as the buffer pointer
    leau :uip_buf
    ; while(1)
rx_lp:
    ; if(uip_len >= MAX_SIZE)
    cmpy #MAX_SIZE
    bge start
    ; b = serial_get();
    bsr serial_get
    ; switch(b)
    ; case SLIP_END:
    cmpb :slip_end
    bne nslip_rxend
    ; if(uip_len > 0)
    cmpy #0
    bne rx_exit
    ; else goto start;
    bra start
nslip_rxend:
    ; case SLIP_ESC:
    cmpb :slip_esc
    bne nslip_rxesc
    ; b = serial_get();
    bsr serial_get
    ; switch(b)
    ; case SLIP_ESC_END:
    cmpb :slip_esc_end
    bne nslip_rxescend
    ; b = SLIP_END;
    ldb :slip_end
    bra nslip_rxesc
nslip_rxescend:
    ; case SLIP_ESC_ESC
    cmpb :slip_esc_esc
    bne nslip_rxesc
    ; b = SLIP_ESC;
    ldb :slip_esc
nslip_rxesc:
    ; uip_buf[uip_len] = b; 
    stb ,u+
    ; uip_len++
    leay 1,y
    bra rx_lp
rx_exit:
    ; return uip_len;
    sty :uip_len
    puls y,u
  }

  return uip_len ;
}

#ifndef SERIAL_DRIVER

// routines for receiving data in simple
// polled mode, only works when running in emulation

u8_t serial_rx_pending(void)
{
  return (*sy6551_status) & STAT_RX ; 
}

u8_t serial_get(void)
{
  while(!serial_rx_pending() )
  {
  }
  return *sy6551_holding ;
}

#else

// read next byte from rx ring buffer
asm u8_t serial_get(void)
{
  asm
  {
    ldx <ring_read_ptr
    cmpx <ring_write_ptr
    beq serial_get
    ; fetch next byte from ring buffer
    ; return in B
    ldb ,X+
    ; text for wrap
    cmpx <rx_ring_buffer_end
    bne nrout
    ldx <rx_ring_buffer
nrout
    stx <ring_read_ptr
    rts
  }
}

// get amount of used bytes in the ring buffer
asm u16_t serial_rx_ring_buffer_used(void)
{
  asm
  {
    ldd <ring_write_ptr
    subd <ring_read_ptr
    bcc rused_out
	; read pointer ahead of write pointer
    ldd #RX_RING_BUFZ
    subd <ring_read_ptr
    addd <ring_write_ptr
rused_out
    rts
  }
}

#endif

void slipdev_init(void)
{
#ifdef SERIAL_DRIVER
  u8_t *p_rx_ring_buffer ;

#ifndef RX_RING_BUFFER_PTR
  // locate the ring buffer in the first graphics page  
  u8_t **p_graphics_base = (u8_t**)0xba ;
  
  p_rx_ring_buffer = *p_graphics_base ;
#else
  *p_rx_ring_buffer = (u8_t*)RX_RING_BUFFER_PTR ;
#endif

  asm
  {
    // setup the ring buffer pointers
    ldd :p_rx_ring_buffer
    std <rx_ring_buffer
   	std <ring_read_ptr
	std <ring_write_ptr
	addd #RX_RING_BUFZ
	std <rx_ring_buffer_end
	clr <ring_overruns
  }
  install_6551_int_handler() ;
#else
  set_dtr() ;
#endif
}

void set_dtr(void)
{
  *sy6551_cmd = (*sy6551_cmd) | CMD_DTR ;
}

void clear_dtr(void)
{
#ifdef HW_FLOW_CONTROL
  *sy6551_cmd = (*sy6551_cmd) & (~CMD_DTR) ;
#endif
}
