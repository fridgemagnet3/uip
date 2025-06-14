#include <cmoc.h>
#include "uip.h"
#include "uip_arp.h"
#include "timer.h"
#include "slipdev.h"

/* main processing loop, heavily generated from Unix version */

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

int main(void)
{
  uip_ipaddr_t ipaddr;
  struct timer periodic_timer, arp_timer;
  // online random MAC generator! 
  const u8_t mac_addr[] = { 0xa0,0x6f,0x6b,0xbb,0xc9,0xb9 } ;
  struct uip_eth_addr eth_mac_addr ;
  int i ;
  
  printf( "Starting up..\n" );
  timer_set(&periodic_timer, CLOCK_SECOND / 2);
  timer_set(&arp_timer, CLOCK_SECOND * 10);

  slipdev_init();
  uip_init();
  for(i=0;i<6;i++)
    eth_mac_addr.addr[i] = mac_addr[i] ;

  // set our MAC
  uip_setethaddr(eth_mac_addr);
#ifndef APP_DHCPC  
  // set our IP
  uip_ipaddr(ipaddr, 192,168,3,2);
  uip_sethostaddr(ipaddr);
  // set default GW
  uip_ipaddr(ipaddr, 192,168,3,1);
  uip_setdraddr(ipaddr);
  // set netmask
  uip_ipaddr(ipaddr, 255,255,255,0);
  uip_setnetmask(ipaddr);
  // set broadcast
  uip_ipaddr(ipaddr, 192,168,3,255);
  uip_setbroadcast(ipaddr) ;
#else
  dhcpc_init(&mac_addr, 6);
#endif
  
#ifdef APP_TELNETD
  telnetd_init();
#endif
#ifdef APP_SOLARUDP
  solar_udp_init() ;
#endif
#ifdef APP_WEATHERUDP
  weather_udp_init() ;
#endif
#ifdef APP_WEBCLIENT
    u8_t key = 0 ;
    webclient_init();
    printf("Press a key to issue web request\n") ;
#endif
#ifdef APP_RESOLV
    resolv_init();
    // address of DNS server
    uip_ipaddr(ipaddr, 192,168,0,201);
    resolv_conf(ipaddr);
#endif

  printf( "Entering main loop\n" ) ;
  while(1)
  {
#ifdef APP_WEBCLIENT
    // poll for keypress
    asm
    {
      jsr $8006
      sta :key
    }
    // issue request when keypress detected
    if ( key )
    {
#ifdef APP_RESOLV
      printf("Issuing DNS lookup...\n") ;
      resolv_query("monolith.onasticksoftware.net");
#else
    printf("Issuing web request...\n") ;
    webclient_get("192.168.0.201", 80, "/index.html");
#endif
    }
#endif

    uip_len = slipdev_read();
    if(uip_len > 0) 
    {
      if(BUF->type == HTONS(UIP_ETHTYPE_IP)) 
      {
	    uip_arp_ipin();
	    uip_input();
	    /* If the above function invocation resulted in data that
	       should be sent out on the network, the global variable
	       uip_len is set to a value > 0. */
	    if(uip_len > 0) 
	    {
	      uip_arp_out();
	      slipdev_send();
	    }
      } 
      else if(BUF->type == HTONS(UIP_ETHTYPE_ARP)) 
      {
	    uip_arp_arpin();
	    /* If the above function invocation resulted in data that
	       should be sent out on the network, the global variable
	       uip_len is set to a value > 0. */
	    if(uip_len > 0) 
	    {
	      slipdev_send();
	    }
      }
    }
    if(timer_expired(&periodic_timer))
    {
      //printf("Periodic timer fired\n") ;
      timer_reset(&periodic_timer);
      for(i = 0; i < UIP_CONNS; i++)
      {
        uip_periodic(i);
        /* If the above function invocation resulted in data that
	       should be sent out on the network, the global variable
	       uip_len is set to a value > 0. */
	    if(uip_len > 0) 
	    {
	      uip_arp_out();
	      slipdev_send();
	    }
	  }
	  
#if UIP_UDP
      for(i = 0; i < UIP_UDP_CONNS; i++) 
      {
        uip_udp_periodic(i);
	    /* If the above function invocation resulted in data that
	       should be sent out on the network, the global variable
	       uip_len is set to a value > 0. */
	    if(uip_len > 0) 
	    {
	      uip_arp_out();
	      slipdev_send();
	    }
	  }
#endif /* UIP_UDP */
	}
    /* Call the ARP timer function every 10 seconds. */
    if(timer_expired(&arp_timer)) 
    {
      //printf("ARP timer fired\n") ;
	  timer_reset(&arp_timer);
	  uip_arp_timer();
    }  
  }
  return 0 ;
}

void uip_log(const char *m)
{
  printf("uIP log message: %s\n", m);
}

#ifdef APP_RESOLV
void resolv_found(char *name, u16_t *ipaddr)
{
  u16_t *ipaddr2;
  
  if(ipaddr == NULL) 
  {
    printf("Host '%s' not found.\n", name);
  } else 
  {
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
#endif

#ifdef APP_DHCPC
void dhcpc_configured(const struct dhcpc_state *s)
{
  printf("DHCP configured\n") ;
  uip_sethostaddr(s->ipaddr);
  uip_setnetmask(s->netmask);
  uip_setdraddr(s->default_router); 
  uip_setbroadcast(s->broadcast_addr) ;
  
#ifdef APP_RESOLV
  resolv_conf(s->dnsaddr);
#endif
}
#endif 

#ifdef APP_WEBCLIENT
void webclient_closed(void)
{
  printf("Webclient: connection closed\n");
}

void webclient_aborted(void)
{
  printf("Webclient: connection aborted\n");
}
void webclient_timedout(void)
{
  printf("Webclient: connection timed out\n");
}
void webclient_connected(void)
{
  printf("Webclient: connected, waiting for data...\n");
}
void webclient_datahandler(char *data, u16_t len)
{
  putstr(data,len);
  //printf("Webclient: got %d bytes of data.\n", len);
}
#endif
