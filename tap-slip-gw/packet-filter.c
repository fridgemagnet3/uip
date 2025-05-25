#include "packet-filter.h"
#include <string.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

static bool filter_udp_broadcast = true ;

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
      // apply UDP filtering logic
      // possible future enhancement - allow for individual IP/port numbers to be filtered
      // rather than all or nothing
      return filter_udp_broadcast ;        
    }
    
  }
  else if ( frame_hdr->ether_dhost[0] & 0x1 ) // test for multicast and discard
    return true ;

  return false ;    
}

void enable_udp_broadcast(bool enable)
{
  filter_udp_broadcast = enable ;
}
