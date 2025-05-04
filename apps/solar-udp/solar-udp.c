#include "solar-udp.h"
#include "uip.h"
#ifndef _CMOC_VERSION_
#include <stdio.h>
#include <string.h>
#else
#include <cmoc.h>
#endif

// socat udp-recv:52005 udp-sendto:192.168.3.2:52005 
static struct uip_udp_conn *solar_conn = NULL;

static const u16_t solar_udp_port = 52005 ;

void solar_udp_appcall(void)
{
  if(uip_udp_conn->lport == HTONS(solar_udp_port)) 
  {
    if(uip_newdata()) 
    {
      char *solar_data = (char*)uip_appdata ;
      solar_data[uip_datalen()] = 0;

      printf("%s\n",solar_data) ;
    }
  }
}

void solar_udp_init(void)
{
  // create a UDP connection, listen on the solar metrics port
  solar_conn = uip_udp_new(0, 0);
  if (solar_conn)
    uip_udp_bind(solar_conn, HTONS(solar_udp_port));
}
