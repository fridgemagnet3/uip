#ifndef PACKET_FILTER_H

#define PACKET_FILTER_H

#include <stdbool.h>
#include <stdint.h>

// filter packet, return true if packet should be filtered
bool filter_packet(uint8_t *pkt, uint16_t size) ;

// enable/disable UDP broadcasts
void enable_udp_broadcast(bool enable) ;

// enable a specific UDP broadcast port
void enable_broadcast_udp_port(uint16_t port) ;

#endif
