#include "ntpclient.h"
#include "uip.h"

#ifdef _CMOC_VERSION_
#include <cmoc.h>
#define be32toh(x) x
#define htobe32(x) x
typedef unsigned long u32_t;
#else
#include <string.h>
#include <endian.h>
typedef uint32_t u32_t;
#endif

#define NTP_EPOCH_TO_UNIX 2208988800

// NTP packet structure
typedef struct {
  u8_t flags ;
  u8_t stratum ;
  u8_t poll ;
  u8_t precision ;
  u32_t root_delay ;
  u32_t root_dispersion ;
  u32_t ref_id ;
  u32_t ref_time_s ;
  u32_t ref_time_us ;
  u32_t org_time_s ;
  u32_t org_time_us ;
  u32_t rec_time_s ;
  u32_t rec_time_us ;
  u32_t xmt_time_s ;
  u32_t xmt_time_us ;
} ntp_pkt_t ;

// state machine gubbins
typedef enum { NTP_REQUEST, NTP_PENDING, NTP_DONE } ntp_state_t ;

static struct uip_udp_conn *ntp_conn = NULL;
static u8_t retries = 0 ;
static time_t ntp_time ;
static ntp_state_t ntp_state = NTP_DONE ;

void ntp_init(u16_t *ntpserver)
{
#ifdef _CMOC_VERSION_  
  ntp_conn = uip_udp_new(ntpserver, HTONS(123));
#else
  ntp_conn = uip_udp_new((uip_ipaddr_t*)ntpserver, HTONS(123));
#endif
}

void ntp_query(void)
{
  ntp_state = NTP_REQUEST ;
  retries = 5 ;
}

void ntp_appcall(void)
{
  if(uip_udp_conn->rport == HTONS(123)) 
  {
    if(uip_newdata())
    {
      ntp_pkt_t *ntp_pkt = (ntp_pkt_t*)uip_appdata ;
      ntp_time = be32toh(ntp_pkt->xmt_time_s) - NTP_EPOCH_TO_UNIX ;
      ntp_done(ntp_time) ;
      ntp_state = NTP_DONE ;
    }

    switch ( ntp_state )
    {
      case NTP_REQUEST :
      
        if ( uip_poll() )
        {
          ntp_pkt_t *ntp_pkt = (ntp_pkt_t*)uip_appdata ;
          time_t time_now ;

          // as a client, most of this can be left as zero      
          memset(ntp_pkt,0,sizeof(ntp_pkt_t)) ;
          ntp_pkt->flags = 0x23 ;  // NTP ver 4, client
          ntp_pkt->precision = 0x20 ;
          time_now = time(NULL) ;
          ntp_pkt->xmt_time_s = htobe32(time_now + NTP_EPOCH_TO_UNIX) ;
          uip_udp_send(sizeof(ntp_pkt_t)) ;
          ntp_state = NTP_PENDING ;
        }
        break ;
        
      case NTP_PENDING :
      
        // as NTP uses UDP which is an unreliable transport mechanism AND
        // the UIP stack can discard the initial packet, replacing it with an
        // ARP request for the IP address of the NTP server, if we haven't got
        // a response back within a few polls, re-issue the request
        retries-- ;
        if ( !retries )
          ntp_query() ;
        break ;
        
      default :
      
        break ;
    }
  }
}

time_t get_ntptime(void)
{
  return ntp_time ;
}
