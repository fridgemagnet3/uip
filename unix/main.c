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
 * $Id: main.c,v 1.16 2006/06/11 21:55:03 adam Exp $
 *
 */

#include <stdio.h>
#include <string.h>
#include "uip.h"
#include "uip_arp.h"
#include "tapdev.h"

#include "timer.h"

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

#ifndef NULL
#define NULL (void *)0
#endif /* NULL */

/*---------------------------------------------------------------------------*/
int
main(void)
{
  int i;
  uip_ipaddr_t ipaddr;
  struct timer periodic_timer, arp_timer;
  // online random MAC generator! 
  const u8_t mac_addr[] = { 0xa3,0x6f,0x6b,0xbb,0xc9,0xb9 } ;
  struct uip_eth_addr eth_mac_addr ;

  timer_set(&periodic_timer, CLOCK_SECOND / 2);
  timer_set(&arp_timer, CLOCK_SECOND * 10);
  
  tapdev_init();
  uip_init();
  for(i=0;i<6;i++)
    eth_mac_addr.addr[i] = mac_addr[i] ;
  // set our MAC
  uip_setethaddr(eth_mac_addr);
  
  uip_ipaddr(ipaddr, 192,168,3,2);
  uip_sethostaddr(ipaddr);
  uip_ipaddr(ipaddr, 192,168,3,1);
  uip_setdraddr(ipaddr);
  uip_ipaddr(ipaddr, 255,255,255,0);
  uip_setnetmask(ipaddr);

  // httpd_init();
#ifdef APP_TELNETD  
  telnetd_init();
#endif
#ifdef APP_SOLARUDP
  solar_udp_init() ;
#endif

  //hello_world_init();

  /*  {
      u8_t mac[6] = {1,2,3,4,5,6};
      dhcpc_init(&mac, 6);
      }*/
  
  /*uip_ipaddr(ipaddr, 127,0,0,1);
  smtp_configure("localhost", ipaddr);
  SMTP_SEND("adam@sics.se", NULL, "uip-testing@example.com",
	    "Testing SMTP from uIP",
	    "Test message sent by uIP\r\n");*/

#ifdef APP_WEBCLIENT
    webclient_init();
#endif
#ifdef APP_RESOLV
    resolv_init();
    uip_ipaddr(ipaddr, 192,168,0,201);
    resolv_conf(ipaddr);
    printf("Issuing DNS lookup...\n") ;
    resolv_query("monolith.onasticksoftware.net");
#elif defined(APP_WEBCLIENT)
    printf("Issuing web request...\n") ;
    webclient_get("192.168.0.201", 80, "/index.html");
#endif
  
  while(1) {
    uip_len = tapdev_read();
    if(uip_len > 0) {
      if(BUF->type == htons(UIP_ETHTYPE_IP)) {
	uip_arp_ipin();
	uip_input();
	/* If the above function invocation resulted in data that
	   should be sent out on the network, the global variable
	   uip_len is set to a value > 0. */
	if(uip_len > 0) {
	  uip_arp_out();
	  tapdev_send();
	}
      } else if(BUF->type == htons(UIP_ETHTYPE_ARP)) {
	uip_arp_arpin();
	/* If the above function invocation resulted in data that
	   should be sent out on the network, the global variable
	   uip_len is set to a value > 0. */
	if(uip_len > 0) {
	  tapdev_send();
	}
      }

    } else if(timer_expired(&periodic_timer)) {
      timer_reset(&periodic_timer);
      for(i = 0; i < UIP_CONNS; i++) {
	uip_periodic(i);
	/* If the above function invocation resulted in data that
	   should be sent out on the network, the global variable
	   uip_len is set to a value > 0. */
	if(uip_len > 0) {
	  uip_arp_out();
	  tapdev_send();
	}
      }

#if UIP_UDP
      for(i = 0; i < UIP_UDP_CONNS; i++) {
	uip_udp_periodic(i);
	/* If the above function invocation resulted in data that
	   should be sent out on the network, the global variable
	   uip_len is set to a value > 0. */
	if(uip_len > 0) {
	  uip_arp_out();
	  tapdev_send();
	}
      }
#endif /* UIP_UDP */
      
      /* Call the ARP timer function every 10 seconds. */
      if(timer_expired(&arp_timer)) {
      printf("ARP timer fired\n") ;
	timer_reset(&arp_timer);
	uip_arp_timer();
      }
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
uip_log(const char *m)
{
  printf("uIP log message: %s\n", m);
}
void
resolv_found(char *name, u16_t *ipaddr)
{
  u16_t *ipaddr2;
  
  if(ipaddr == NULL) {
    printf("Host '%s' not found.\n", name);
  } else {
    printf("Found name '%s' = %d.%d.%d.%d\n", name,
	   htons(ipaddr[0]) >> 8,
	   htons(ipaddr[0]) & 0xff,
	   htons(ipaddr[1]) >> 8,
	   htons(ipaddr[1]) & 0xff);
#ifdef APP_WEBCLIENT	   
       printf("Issuing web request...\n") ;
       webclient_get(name, 80, "/index.html");
#endif       
  }
}
#ifdef __DHCPC_H__
void
dhcpc_configured(const struct dhcpc_state *s)
{
  uip_sethostaddr(s->ipaddr);
  uip_setnetmask(s->netmask);
  uip_setdraddr(s->default_router);
  resolv_conf(s->dnsaddr);
}
#endif /* __DHCPC_H__ */
void
smtp_done(unsigned char code)
{
  printf("SMTP done with code %d\n", code);
}
void
webclient_closed(void)
{
  printf("Webclient: connection closed\n");
}
void
webclient_aborted(void)
{
  printf("Webclient: connection aborted\n");
}
void
webclient_timedout(void)
{
  printf("Webclient: connection timed out\n");
}
void
webclient_connected(void)
{
  printf("Webclient: connected, waiting for data...\n");
}
void
webclient_datahandler(char *data, u16_t len)
{
  if ( !len )
  {
    webclient_close() ;
  }
  else
  {  
    char buf[len+1] ;
    printf("Webclient: got %d bytes of data.\n", len);
  
    strncpy(buf,data,len) ;
    buf[len]= 0 ;
    fputs(buf,stdout);
  }
}
/*---------------------------------------------------------------------------*/
