#ifndef WEATHER_UDP_H

#define WEATHER_UDP_H

// Simple app designed to pick up and display weather data
// broadcast over UDP 

// the thing that displays the data
void weather_udp_appcall(void);

#ifndef UIP_UDP_APPCALL
// unused but needs to be defined
typedef int uip_udp_appstate_t;
#define UIP_UDP_APPCALL weather_udp_appcall
#endif

#include "uipopt.h"

void weather_udp_init(void);

#ifndef SOLAR_UDP_H
typedef void(*output_str_t)(const char *str) ;
#endif
// output the weather data using the supplied callback
void output_weather_data(output_str_t output_str_cback) ;

#endif
