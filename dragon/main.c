#include <cmoc.h>
#include "uip.h"
#include "uip_arp.h"
#include "timer.h"
#include "slipdev.h"
#ifdef WEB_GRAPHICS
#include "graphics.h"
#include "mapmode.h"
#endif

/* main processing loop, heavily generated from Unix version */

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

#ifdef APP_SMTP
static void send_email(void) ;
#endif

#ifdef APP_RESOLV
// the type of query currently being resolved
typedef enum { RESOLV_NONE, RESOLV_WEBSERVER, RESOLV_SMTPSERVER, RESOLV_NTPSERVER } resolv_query_t ;
static resolv_query_t resolv_q = RESOLV_NONE ;
#endif

int main(void)
{
  uip_ipaddr_t ipaddr;
  struct timer periodic_timer, arp_timer;
  // online random MAC generator! 
  const u8_t mac_addr[] = { 0xa0,0x6f,0x6b,0xbb,0xc9,0xb9 } ;
  struct uip_eth_addr eth_mac_addr ;
  int i ;
  u8_t key = 0 ;
  
  printf( "Starting up..\n" );
  timer_set(&periodic_timer, CLOCK_SECOND / 2);
  timer_set(&arp_timer, CLOCK_SECOND * 10);

  slipdev_init();
  uip_init();
  memcpy(eth_mac_addr.addr,mac_addr,sizeof(mac_addr)) ;

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
  // generate an ARP announcement. The ESP-Eth-GW app won't
  // send us any traffic until we first send a packet so
  // this serves to satisfy that criteria.
  // It's not required for DHCP since the first thing we do 
  // is send out a broadcast packet for configuration...
  uip_arp_announcement() ;
  slipdev_send();
#else
  dhcpc_init(&mac_addr, 6);
#endif
  
  // app initialisations....
#ifdef APP_TELNETD
  telnetd_init();
#endif
#ifdef APP_SOLARUDP
  solar_udp_init() ;
#endif
#ifdef APP_WEATHERUDP
  weather_udp_init() ;
#endif
#ifdef APP_HELLOWORLD
  hello_world_init();
#endif

#ifdef APP_WEBCLIENT
  webclient_init();
#ifdef WEB_GRAPHICS
  // switch to map 1 (64K RAM mode) & copy BASIC ROM across
  // to use hi memory as graphics RAM
  switch_mapmode();
#endif
  printf("Press a key to issue web request\n") ;
#endif

#ifdef APP_RESOLV
  resolv_init();
#ifndef APP_DHCPC  
  // address of DNS server
  uip_ipaddr(ipaddr, 192,168,0,201);
  resolv_conf(ipaddr);
#endif
#endif

#ifdef APP_NTP
#ifdef APP_RESOLV
   printf("DNS lookup for NTP server..\n") ;
   // DNS lookup of NTP server, the NTP request
   // is then handled by the resolv_done callback
   resolv_query("monolith.onasticksoftware.net");
   resolv_q = RESOLV_NTPSERVER ;
#else
  // address of NTP server
  uip_ipaddr(ipaddr, 192,168,0,201);
  ntp_init(ipaddr);
  printf("Issuing NTP query...\n") ;
  ntp_query() ;
#endif
#endif

#ifdef APP_SMTP
  printf("Press a key to send an email\n") ;
#endif

  printf( "Entering main loop\n" ) ;
  while(1)
  {
#if defined(APP_WEBCLIENT) || defined(APP_SMTP)
    // poll for keypress
    asm
    {
      jsr $8006
      sta :key
    }
    // issue request when keypress detected
    if ( key )
    {
#ifdef APP_WEBCLIENT
#ifdef APP_RESOLV
      printf("DNS lookup for web server...\n") ;
#ifdef APP_DHCPC
      // assume if DHCP assigned, can get on t'internet
      resolv_query("www.oasw.co.uk");
#else
      resolv_query("monolith.onasticksoftware.net");
#endif // DHCP
      resolv_q = RESOLV_WEBSERVER ;

#else // no DNS
      printf("Issuing web request...\n") ;
#ifdef WEB_GRAPHICS
      webclient_get("192.168.0.201", 80, "/dragon-logo.bin");
#else
      webclient_get("192.168.0.201", 80, "/dragon.txt");
#endif

#endif // DNS

#else // smtp app

#ifndef APP_RESOLV
  // IP address of SMTP outgoing server
  uip_ipaddr(ipaddr, 192,168,0,201);
  smtp_configure("dragon64", ipaddr);
  send_email() ;
#else
  printf("DNS lookup for mail server...\n") ;
  resolv_query("mail.onasticksoftware.net");
  resolv_q = RESOLV_SMTPSERVER ;
  // email config & send is handled by the resolv_done callback
#endif

#endif
    }

#endif  // webclient or smtp app

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
       // DNS has resolved name of webserver, issue the request
       if ( resolv_q == RESOLV_WEBSERVER )
       {
         printf("Issuing web request...\n") ;
#ifdef WEB_GRAPHICS
         webclient_get(name, 80, "/dragon-logo.bin");
#else
         webclient_get(name, 80, "/dragon.txt");
#endif
       }
#endif       

#ifdef APP_NTP
    // DNS has resolved name of NTP server, issue the request
    if ( resolv_q == RESOLV_NTPSERVER )
    {
      ntp_init(ipaddr);
      printf("Issuing NTP query...\n") ;
      ntp_query() ;
    }
#endif

#ifdef APP_SMTP
    // DNS has resolved name of SMTP server, configure
    // and send the email
    if ( resolv_q == RESOLV_SMTPSERVER )
    {
      smtp_configure("dragon64", ipaddr);
      send_email() ;
    }
#endif
  }
  resolv_q = RESOLV_NONE ;
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

#ifdef WEB_GRAPHICS

#define GRAPHICS_BASE ((char*)0xe000) 
#define GRAPHICS_SIZE 6144

static char *graphics_ptr ;
static u16_t graphics_txfrd ;

#endif

void webclient_closed(void)
{
  printf("Webclient: connection closed\n");
#ifdef WEB_GRAPHICS
  printf("Txfrd: %u\n",graphics_txfrd);
  // switch to hi-res mode to display the downloaded image
  pagex(GRAPHICS_BASE);
  gmode(MODE6R) ;
  css(1);
#endif
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
#ifdef WEB_GRAPHICS
  graphics_txfrd = 0u ;
  graphics_ptr = GRAPHICS_BASE ;
#endif
}

void webclient_datahandler(char *data, u16_t len)
{
#ifdef WEB_GRAPHICS
  printf("Webclient: got %d bytes\n", len);

  if ( data )
  {
    // copy data to graphics memory
    if ( (graphics_txfrd+len) <= GRAPHICS_SIZE)
    {
      memcpy(graphics_ptr,data,len) ;
      graphics_ptr+=len ;
      graphics_txfrd+=len ;
    }
  }
  else
    webclient_close() ;
#else
    putstr(data,len);
#endif
}
#endif

#ifdef APP_SMTP

// beware using this with a configuration that's generating a fair
// amount of traffic because nothing will get serviced whilst this is running...
static void send_email(void)
{
  static char from_addr[40] ;
  static char to_addr[40] ;
  static char subject[50] ;
  static char msg[256] ;
  char *p ;

  printf("FROM: " ) ;
  p = readline() ;
  if ( !p )
  {
    printf("ERROR READING INPUT\n") ;
    return ;
  }
  strcpy(from_addr,p) ;
  
  printf("TO: " ) ;
  p = readline() ;
  if ( !p )
  {
    printf("ERROR READING INPUT\n") ;
    return ;
  }
  strcpy(to_addr,p) ;
  printf("SUBJ: " ) ;
  p = readline() ;
  if ( !p )
  {
    printf("ERROR READING INPUT\n") ;
    return ;
  }
  strcpy(subject,p) ;
  printf("MSG: " ) ;
  p = readline() ;
  if ( !p )
  {
    printf("ERROR READING INPUT\n") ;
    return ;
  }
  strcpy(msg,p) ;
  
  printf("Sending email...\n") ;
  SMTP_SEND(to_addr, NULL, from_addr,
	        subject, msg);
}

// callback invoked when SMTP transaction is complete
void smtp_done(unsigned char code)
{
  printf("SMTP done with code %d\n", code);
}
#endif

#ifdef APP_NTP
// callback invoked when NTP time is acquired
void ntp_done(time_t ntp_time)
{
  struct tm tm ;
  char buf[36] ;
  
  gmtime_r(&ntp_time,&tm) ;
  asctime_r(&tm,buf) ;
  printf("NTP time: %s\n",buf) ;
  // set the system time
  stime(&ntp_time);
}
#endif
