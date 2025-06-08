#ifndef SOLAR_UDP_H

#define SOLAR_UDP_H

// Simple app designed to pick up and display solar metrics
// broadcast over UDP from either my solis-broadcast app
// https://github.com/fridgemagnet3/solar-display-micropython/tree/main/solis-broadcast
// or direct from the inverter via modbus-solis-broadcast
// https://github.com/fridgemagnet3/modbus-solis5g


// the thing that displays the metrics
void solar_udp_appcall(void);

#ifndef UIP_UDP_APPCALL
// unused but needs to be defined
typedef int uip_udp_appstate_t;
#define UIP_UDP_APPCALL solar_udp_appcall
#endif

// output the solar metrics using the supplied callback
typedef void(*output_str_t)(const char *str) ;

#include "uipopt.h"

void solar_udp_init(void);

void output_solar_metrics(output_str_t output_str_cback) ;

#endif
