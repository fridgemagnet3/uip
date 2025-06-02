#ifndef APP_CALLCHAIN_H

#define APP_CALLCHAIN_H

/* This app is designed to assist with having multiple applications included in the build
   in that the architecture only allows a single application handler and state data to be
   defined per protocol. Essentially the two calls defined, simply daisy chain the calls
   into the individual applications.
   
   This should be the LAST header file included from uip-conf.h */

#ifndef UIP_APPCALL
// unused but needs to be defined
typedef int uip_tcp_appstate_t;
#else
// replace whatever TCP app call was defined with ours
#undef UIP_APPCALL
#endif

#define UIP_APPCALL callchain_tcp_appcall

// the UDP component is disabled if no UDP based apps are defined
// so in that case we don't need to define any state data 
#ifdef UIP_UDP_APPCALL
// replace whatever UDP app call was defined with ours
#undef UIP_UDP_APPCALL
#define UIP_UDP_APPCALL callchain_udp_appcall
#endif

void callchain_tcp_appcall(void);

void callchain_udp_appcall(void);

#endif
