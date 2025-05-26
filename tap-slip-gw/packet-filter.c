#include "packet-filter.h"
#include <string.h>
#ifdef linux
#include <net/ethernet.h>
#else
#include "ethernet.h"
#endif
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#define MAX_UDP_PORTS 256

static bool filter_udp_broadcast = true ;
static uint16_t udp_broadcast_ports[MAX_UDP_PORTS] ;
static uint16_t next_port_idx = 0 ;

// return true if packet should be filtered
bool filter_packet(uint8_t *pkt, uint16_t size)
{
  const uint8_t broadcast_mac[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } ;
  struct ether_header *frame_hdr = (struct ether_header*)pkt ;

  // test for broadcast MAC
  if ( !memcmp(frame_hdr->ether_dhost, broadcast_mac, ETH_ALEN ) )
  {
    // broadcast packet....
    
    // don't filter ARP requests
    if ( frame_hdr->ether_type == htons(ETHERTYPE_ARP) )
      return false ;
    
    // filter out all other non IPv4 traffic
    if ( frame_hdr->ether_type != htons(ETHERTYPE_IP) )
      return true ;
      
    struct ip *ip_hdr = (struct ip*)(pkt + sizeof(struct ether_header)) ;
    
    if ( ip_hdr->ip_p == IPPROTO_UDP )
    {
      // apply UDP filtering logic..
      
      // if not filtering broadcasts, let the packet through
      if ( !filter_udp_broadcast )
        return false ;
      // if no exceptions in place, discard it
      if ( !next_port_idx )
        return true ;
        
      struct udphdr *udp_hdr = (struct udphdr*)(pkt+sizeof(struct ether_header)+sizeof(struct ip)) ;
      uint16_t i ;
            
      // walk thru the list of allowed UDP ports
      for ( i=0 ; i < next_port_idx ; i++ )
      {
        // if matches incoming packet, allow it through
        if ( udp_broadcast_ports[i] == htons(udp_hdr->uh_dport) )
          return false ;
      }
      // discard
      return true ;
    }
    
  }
  else if ( frame_hdr->ether_dhost[0] & 0x1 ) // test for multicast and discard
    return true ;

  return false ;    
}

void enable_udp_broadcast(bool enable)
{
  filter_udp_broadcast = !enable ;
}

void enable_broadcast_udp_port(uint16_t port) 
{
  if ( next_port_idx < MAX_UDP_PORTS )
    udp_broadcast_ports[next_port_idx++] = port ;
}
