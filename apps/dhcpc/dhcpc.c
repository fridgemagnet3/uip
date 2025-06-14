/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack
 *
 * @(#)$Id: dhcpc.c,v 1.2 2006/06/11 21:46:37 adam Exp $
 */

#ifndef _CMOC_VERSION_
#include <stdio.h>
#include <string.h>
#else
#include <cmoc.h>
//#define DHCP_STATUS
#endif

#include "uip.h"
#include "dhcpc.h"
#include "timer.h"

#define STATE_INITIAL         0
#define STATE_SENDING         1
#define STATE_OFFER_RECEIVED  2
#define STATE_CONFIG_RECEIVED 3
#define STATE_DONE            4

static struct dhcpc_state s;

struct dhcp_msg {
  u8_t op, htype, hlen, hops;
  u8_t xid[4];
  u16_t secs, flags;
  u8_t ciaddr[4];
  u8_t yiaddr[4];
  u8_t siaddr[4];
  u8_t giaddr[4];
  u8_t chaddr[16];
#ifndef UIP_CONF_DHCP_LIGHT
  u8_t sname[64];
  u8_t file[128];
#endif
  u8_t options[312];
};

#define BOOTP_BROADCAST 0x8000

#define DHCP_REQUEST        1
#define DHCP_REPLY          2
#define DHCP_HTYPE_ETHERNET 1
#define DHCP_HLEN_ETHERNET  6
#define DHCP_MSG_LEN      236

#define DHCPC_SERVER_PORT  67
#define DHCPC_CLIENT_PORT  68

#define DHCPDISCOVER  1
#define DHCPOFFER     2
#define DHCPREQUEST   3
#define DHCPDECLINE   4
#define DHCPACK       5
#define DHCPNAK       6
#define DHCPRELEASE   7

#define DHCP_OPTION_SUBNET_MASK   1
#define DHCP_OPTION_ROUTER        3
#define DHCP_OPTION_DNS_SERVER    6
#define DHCP_OPTION_REQ_IPADDR   50
#define DHCP_OPTION_LEASE_TIME   51
#define DHCP_OPTION_MSG_TYPE     53
#define DHCP_OPTION_SERVER_ID    54
#define DHCP_OPTION_REQ_LIST     55
#define DHCP_OPTION_END         255

static const u8_t xid[4] = {0xad, 0xde, 0x12, 0x23};
static const u8_t magic_cookie[4] = {99, 130, 83, 99};
/*---------------------------------------------------------------------------*/
static u8_t *
add_msg_type(u8_t *optptr, u8_t type)
{
#ifdef _CMOC_VERSION_
  asm
  {
    ldx :optptr
    lda #DHCP_OPTION_MSG_TYPE
    sta ,x+
    lda #1
    sta ,x+
    lda :type
    sta ,x+
    tfr x,d
  }
#else
  *optptr++ = DHCP_OPTION_MSG_TYPE;
  *optptr++ = 1;
  *optptr++ = type;
  return optptr;
#endif
}
/*---------------------------------------------------------------------------*/
static u8_t *
add_server_id(u8_t *optptr)
{
#ifdef _CMOC_VERSION_
  asm
  {
    ldx :optptr
    lda #DHCP_OPTION_SERVER_ID
    sta ,x+
    lda #4
    sta ,x+
    pshs y
    ; memcpy(optptr, s.serverid, 4);
    leay :s.serverid
    ldd ,y++
    std ,x++
    ldd ,y++
    std ,x++
    puls y
    ; return in d
    tfr x,d
  }
#else
  *optptr++ = DHCP_OPTION_SERVER_ID;
  *optptr++ = 4;
  memcpy(optptr, s.serverid, 4);
  return optptr + 4;
#endif
}
/*---------------------------------------------------------------------------*/
static u8_t *
add_req_ipaddr(u8_t *optptr)
{
#ifdef _CMOC_VERSION_
  asm
  {
    ldx :optptr
    lda #DHCP_OPTION_REQ_IPADDR
    sta ,x+
    lda #4
    sta ,x+
    pshs y
    ; memcpy(optptr, s.ipaddr, 4);
    leay :s.ipaddr
    ldd ,y++
    std ,x++
    ldd ,y
    std ,x++
    puls y
    tfr x,d
  }
#else
  *optptr++ = DHCP_OPTION_REQ_IPADDR;
  *optptr++ = 4;
  memcpy(optptr, s.ipaddr, 4);
  return optptr + 4;
#endif
}
/*---------------------------------------------------------------------------*/
static u8_t *
add_req_options(u8_t *optptr)
{
#ifdef _CMOC_VERSION_
  asm
  {
    ldx :optptr
    lda #DHCP_OPTION_REQ_LIST
    sta ,x+
    lda #3
    sta ,x+
    lda #DHCP_OPTION_SUBNET_MASK
    sta ,x+
    lda #DHCP_OPTION_ROUTER
    sta ,x+
    lda #DHCP_OPTION_DNS_SERVER
    sta ,x+
    tfr x,d
  }
#else
  *optptr++ = DHCP_OPTION_REQ_LIST;
  *optptr++ = 3;
  *optptr++ = DHCP_OPTION_SUBNET_MASK;
  *optptr++ = DHCP_OPTION_ROUTER;
  *optptr++ = DHCP_OPTION_DNS_SERVER;
  return optptr;
#endif
}
/*---------------------------------------------------------------------------*/
static u8_t *
add_end(u8_t *optptr)
{
#ifdef _CMOC_VERSION_
  asm
  {
    ldx :optptr
    lda #DHCP_OPTION_END
    sta ,x+
    tfr x,d
  }
#else
  *optptr++ = DHCP_OPTION_END;
  return optptr;
#endif
}
/*---------------------------------------------------------------------------*/
static void
create_msg(register struct dhcp_msg *m)
{
  m->op = DHCP_REQUEST;
  m->htype = DHCP_HTYPE_ETHERNET;
  m->hlen = s.mac_len;
  m->hops = 0;
  memcpy(m->xid, xid, sizeof(m->xid));
  m->secs = 0;
  m->flags = HTONS(BOOTP_BROADCAST); /*  Broadcast bit. */
  /*  uip_ipaddr_copy(m->ciaddr, uip_hostaddr);*/
  memcpy(m->ciaddr, uip_hostaddr, sizeof(m->ciaddr));
#ifdef _CMOC_VERSION_
  memset16(m->yiaddr, 0, (sizeof(m->yiaddr)+sizeof(m->siaddr)+sizeof(m->giaddr))/sizeof(u16_t));
#else
  memset(m->yiaddr, 0, sizeof(m->yiaddr));
  memset(m->siaddr, 0, sizeof(m->siaddr));
  memset(m->giaddr, 0, sizeof(m->giaddr));
#endif
  memcpy(m->chaddr, s.mac_addr, s.mac_len);
  memset(&m->chaddr[s.mac_len], 0, sizeof(m->chaddr) - s.mac_len);
#ifndef UIP_CONF_DHCP_LIGHT
#ifdef _CMOC_VERSION_
  memset16(m->sname, 0, (sizeof(m->sname)+sizeof(m->file))/sizeof(u16_t));
#else
  memset(m->sname, 0, sizeof(m->sname));
  memset(m->file, 0, sizeof(m->file));
#endif
#endif
  memcpy(m->options, magic_cookie, sizeof(magic_cookie));
}
/*---------------------------------------------------------------------------*/
static void
send_discover(void)
{
  u8_t *end;
  struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata;

#ifdef DHCP_STATUS
  printf("DHCP:send_discover\n") ;
#endif

  create_msg(m);

  end = add_msg_type(&m->options[4], DHCPDISCOVER);
  end = add_req_options(end);
  end = add_end(end);

  uip_send(uip_appdata, end - (u8_t *)uip_appdata);
}
/*---------------------------------------------------------------------------*/
static void
send_request(void)
{
  u8_t *end;
  struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata;

#ifdef DHCP_STATUS
  printf("DHCP: send_request\n") ;
#endif

  create_msg(m);
  
  end = add_msg_type(&m->options[4], DHCPREQUEST);
  end = add_server_id(end);
  end = add_req_ipaddr(end);
  end = add_end(end);
  
  uip_send(uip_appdata, end - (u8_t *)uip_appdata);
}
/*---------------------------------------------------------------------------*/
static u8_t
parse_options(u8_t *optptr, int len)
{
  u8_t *end = optptr + len;
  u8_t type = 0;

  while(optptr < end) {
    switch(*optptr) {
    case DHCP_OPTION_SUBNET_MASK:
      memcpy(s.netmask, optptr + 2, 4);
      break;
    case DHCP_OPTION_ROUTER:
      memcpy(s.default_router, optptr + 2, 4);
      break;
    case DHCP_OPTION_DNS_SERVER:
      memcpy(s.dnsaddr, optptr + 2, 4);
      break;
    case DHCP_OPTION_MSG_TYPE:
      type = *(optptr + 2);
      break;
    case DHCP_OPTION_SERVER_ID:
      memcpy(s.serverid, optptr + 2, 4);
      break;
    case DHCP_OPTION_LEASE_TIME:
      memcpy(s.lease_time, optptr + 2, 4);
      break;
    case DHCP_OPTION_END:
      return type;
    }

    optptr += optptr[1] + 2;
  }
  return type;
}
/*---------------------------------------------------------------------------*/
static u8_t
parse_msg(void)
{
  struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata;
  
  if(m->op == DHCP_REPLY &&
     memcmp(m->xid, xid, sizeof(xid)) == 0 &&
     memcmp(m->chaddr, s.mac_addr, s.mac_len) == 0) {
    memcpy(s.ipaddr, m->yiaddr, 4);
    return parse_options(&m->options[4], uip_datalen());
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void handle_dhcp(void)
{
  uip_ipaddr_t broadcast_mask ;
  
  if ( s.state==STATE_DONE )
    return ;

  switch(s.state)
  {
    case STATE_INITIAL :
    
      send_discover();
      s.state = STATE_SENDING;
      s.ticks = CLOCK_SECOND;
      timer_set(&s.timer, s.ticks);
      break ;
 
    case STATE_SENDING :
    
      if(uip_newdata() && parse_msg() == DHCPOFFER) 
      {
        send_request();
        s.state = STATE_OFFER_RECEIVED;
        s.ticks = CLOCK_SECOND;
        timer_set(&s.timer, s.ticks);
      }
      break ;
    
    case STATE_OFFER_RECEIVED :
    
      if(uip_newdata() && parse_msg() == DHCPACK) 
        s.state = STATE_CONFIG_RECEIVED;
      else
        break;
      
    case STATE_CONFIG_RECEIVED :

#ifdef DHCP_STATUS
      printf("Got IP address %d.%d.%d.%d\n",
        uip_ipaddr1(s.ipaddr), uip_ipaddr2(s.ipaddr),
        uip_ipaddr3(s.ipaddr), uip_ipaddr4(s.ipaddr));
      printf("Got netmask %d.%d.%d.%d\n",
        uip_ipaddr1(s.netmask), uip_ipaddr2(s.netmask),
        uip_ipaddr3(s.netmask), uip_ipaddr4(s.netmask));
      printf("Got DNS server %d.%d.%d.%d\n",
        uip_ipaddr1(s.dnsaddr), uip_ipaddr2(s.dnsaddr),
        uip_ipaddr3(s.dnsaddr), uip_ipaddr4(s.dnsaddr));
      printf("Got default router %d.%d.%d.%d\n",
        uip_ipaddr1(s.default_router), uip_ipaddr2(s.default_router),
        uip_ipaddr3(s.default_router), uip_ipaddr4(s.default_router));
      printf("Lease expires in %ld seconds\n",
	     ntohs(s.lease_time[0])*65536ul + ntohs(s.lease_time[1]));
#endif
	  // compute the broadcast address
      uip_ipaddr_mask(s.broadcast_addr,s.ipaddr,s.netmask) ;
      broadcast_mask[0] = ~s.netmask[0] ;
      broadcast_mask[1] = ~s.netmask[1] ;
      s.broadcast_addr[0]|=broadcast_mask[0] ;
      s.broadcast_addr[1]|=broadcast_mask[1] ;
#ifdef DHCP_STATUS
      printf("Broadcast addr %d.%d.%d.%d\n", 
       uip_ipaddr1(s.broadcast_addr),
       uip_ipaddr2(s.broadcast_addr),
       uip_ipaddr3(s.broadcast_addr),
       uip_ipaddr4(s.broadcast_addr)) ;
#endif	    
      dhcpc_configured(&s);
      s.state = STATE_DONE ;
      break ;
      
    default :
      break ;
  }
  
  if ( timer_expired(&s.timer) )
  {
    if(s.ticks < CLOCK_SECOND * 60)
    {
      s.ticks *= 2;
      timer_set(&s.timer, s.ticks);
      // doing this here is a bit hacky..
      if ( s.state == STATE_SENDING )
        send_discover();
      else
        send_request();
    }
    else
      s.state = STATE_INITIAL ;
  }  
}
/*---------------------------------------------------------------------------*/
void
dhcpc_init(const void *mac_addr, int mac_len)
{
  uip_ipaddr_t addr;
  
  s.mac_addr = mac_addr;
  s.mac_len  = mac_len;

  s.state = STATE_INITIAL;
  uip_ipaddr(addr, 255,255,255,255);
  s.conn = uip_udp_new(&addr, HTONS(DHCPC_SERVER_PORT));
  if(s.conn != NULL) {
    uip_udp_bind(s.conn, HTONS(DHCPC_CLIENT_PORT));
  }
}
/*---------------------------------------------------------------------------*/
void
dhcpc_appcall(void)
{
  if(s.conn->lport == HTONS(DHCPC_CLIENT_PORT)) 
    handle_dhcp();
}
/*---------------------------------------------------------------------------*/
void
dhcpc_request(void)
{
  u16_t ipaddr[2];
  
  if(s.state == STATE_INITIAL) {
    uip_ipaddr(ipaddr, 0,0,0,0);
    uip_sethostaddr(ipaddr);
    /*    handle_dhcp(PROCESS_EVENT_NONE, NULL);*/
  }
}
/*---------------------------------------------------------------------------*/
