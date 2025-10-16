#include "uipopt.h"
#include "uip.h"

static uip_ipaddr_t uip_ipaddr_zero ;

void callchain_tcp_appcall(void)
{
  // don't do anything if we've not got an IP address
  if ( !uip_ipaddr_cmp(uip_hostaddr,uip_ipaddr_zero) )
  {
#ifdef APP_WEBCLIENT
    webclient_appcall() ;
#endif
#ifdef APP_TELNETD
    telnetd_appcall() ;
#endif
#ifdef APP_HTTPD
    httpd_appcall() ;
#endif
#ifdef APP_SMTP
    smtp_appcall() ;
#endif
#ifdef APP_HELLOWORLD
    hello_world_appcall() ;
#endif
  }
}

void callchain_udp_appcall(void)
{
#ifdef APP_DHCPC
  dhcpc_appcall() ;
#endif
  // don't do anything if we've not got an IP address
  if ( !uip_ipaddr_cmp(uip_hostaddr,uip_ipaddr_zero) )
  {
#ifdef APP_RESOLV
    resolv_appcall() ;
#endif
#ifdef APP_SOLARUDP
    solar_udp_appcall() ;
#endif
#ifdef APP_WEATHERUDP
    weather_udp_appcall() ;
#endif
  }
}
