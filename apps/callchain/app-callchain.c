#include "uipopt.h"

void callchain_tcp_appcall(void)
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
}

void callchain_udp_appcall(void)
{
#ifdef APP_DHCPC
  dhcpc_appcall() ;
#endif
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
