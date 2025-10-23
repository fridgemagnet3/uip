#ifndef NTPCLIENT_H

#define NTPCLIENT_H

/* simple NTP client */

#include "uipopt.h"
#include "time.h"

#ifndef UIP_UDP_APPCALL
typedef int uip_udp_appstate_t;
#define UIP_UDP_APPCALL ntp_appcall
#endif

void ntp_init(u16_t *ntpserver);

// issue a NTP query to the server
void ntp_query(void);

void ntp_appcall(void);

// app defined callback invoked when NTP time has been acquired
extern void ntp_done(time_t ntp_time);

// return is zero if NTP time has never been acquired
time_t get_ntptime(void) ;

#endif
