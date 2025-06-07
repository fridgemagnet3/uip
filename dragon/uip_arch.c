/* Architecture specific operations */

#include "uip.h"

// 32-bit addition
void uip_add32(u8_t *op32, u16_t op16)
{
  asm
  {
    pshs y
    ; fetch value to add
    ldd :op16
    ldx :op32
    leay :uip_acc32
    ; add the low half
    addd 2,x
    ; store it
    std 2,y
    ; fetch the high half
    ldd 0,x
    ; add 0, carry to B
    adcb #0
    ; add 0, carry to A
    adca #0
    ; update high half
    std 0,y
    puls y
  }
}

// common IP checksum algorithm
static u16_t chksum(u16_t sum, void *sdata, u16_t len)
{
  asm
  {
    pshs y
    ; loop counter
    ldy :len
    ; addr of data
    ldx :sdata
    ; running checksum
    ldd :sum
    ; test for <2 bytes remaining 
lp: cmpy #2
    blo endbyt
    ; decr len
    leay -2,y
    ; add next word
    addd ,x++
    ; test for carry - add 1 if so
    bcc lp
    addd #1
    bra lp
endbyt:
    ; either have 0 or 1 byte remaining
    ; if zero then done
    cmpy #0
    beq end
    ; add final byte 
    adda ,x
    ; test for carry of hi-byte
    bcc end
    addd #1
end:
    ; return is in d
    puls y
  }
}

/*
 * Copyright (c) 2001, Adam Dunkels.
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
 * $Id: uip_arch.c,v 1.1 2002/01/10 06:22:56 adam Exp $
 *
 */

// JRB: After a few iterations, this code is now essentially just a copy
// of that in 'uip.c', it's just making use of the optimised assember above
// to do the actual checksum calculation
//

#define BUF ((struct uip_tcpip_hdr *)&uip_buf[UIP_LLH_LEN])

/*-----------------------------------------------------------------------------------*/
u16_t uip_ipchksum(void)
{
  return chksum(0,(u16_t *)&uip_buf[UIP_LLH_LEN], 20);
}
/*-----------------------------------------------------------------------------------*/

static u16_t upper_layer_chksum(u8_t proto)
{
  u16_t upper_layer_len;
  u16_t sum;
  
#if UIP_CONF_IPV6
  upper_layer_len = (((u16_t)(BUF->len[0]) << 8) + BUF->len[1]);
#else /* UIP_CONF_IPV6 */
  upper_layer_len = (((u16_t)(BUF->len[0]) << 8) + BUF->len[1]) - UIP_IPH_LEN;
#endif /* UIP_CONF_IPV6 */
  
  /* First sum pseudoheader. */
  
  /* IP protocol and length fields. This addition cannot carry. */
  sum = upper_layer_len + proto;
  /* Sum IP source and destination addresses. */
  sum = chksum(sum, (u8_t *)&BUF->srcipaddr[0], 2 * sizeof(uip_ipaddr_t));

  /* Sum TCP header and data. */
  sum = chksum(sum, &uip_buf[UIP_IPH_LEN + UIP_LLH_LEN],
	       upper_layer_len);
    
  return (sum == 0) ? 0xffff : htons(sum);
}

u16_t uip_tcpchksum(void)
{
  return upper_layer_chksum(UIP_PROTO_TCP);
}

u16_t uip_udpchksum(void)
{
  return upper_layer_chksum(UIP_PROTO_UDP);
}

/*-----------------------------------------------------------------------------------*/
