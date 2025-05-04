#ifndef SOLAR_UDP_H

#define SOLAR_UDP_H

// Simple app designed to pick up and display solar metrics
// broadcast over UDP from either my solis-broadcast app
// https://github.com/fridgemagnet3/solar-display-micropython/tree/main/solis-broadcast
// or direct from the inverter via modbus-solis-broadcast
// https://github.com/fridgemagnet3/modbus-solis5g

// unused but needs to be defined
typedef int uip_udp_appstate_t;

// the thing that displays the metrics
void solar_udp_appcall(void);
#define UIP_UDP_APPCALL solar_udp_appcall

#include "uipopt.h"

void solar_udp_init(void);

#endif
