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
  const u8_t mac_addr[] = { 0xa3,0x6f,0x6b,0xbb,0xc9,0xb9 } ;
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
  // set our IP
  uip_ipaddr(ipaddr, 192,168,3,2);
  uip_sethostaddr(ipaddr);
  // set default GW
  uip_ipaddr(ipaddr, 192,168,3,1);
  uip_setdraddr(ipaddr);
  // set netmask
  uip_ipaddr(ipaddr, 255,255,255,0);
  uip_setnetmask(ipaddr);

  hello_world_init();
  printf( "Entering main loop\n" ) ;
  while(1)
  {
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
    else if(timer_expired(&periodic_timer))
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
	}
    /* Call the ARP timer function every 10 seconds. */
    if(timer_expired(&arp_timer)) 
    {
      printf("ARP timer fired\n") ;
	  timer_reset(&arp_timer);
	  uip_arp_timer();
    }  
  }
  return 0 ;
}

void uip_log(char *m)
{
  printf("uIP log message: %s\n", m);
}
