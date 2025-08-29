#include "weather-udp.h"
#include "uip.h"
#include <time.h>
#ifndef _CMOC_VERSION_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define atoui(a) (u16_t)strtoul(a,NULL,10)
#else
#include <cmoc.h>
#endif

typedef struct {
  time_t  timestamp ;  // Unix timestamp
  float   temp ; // temperature
  float   wind ; // wind speed
  float   rain ; // rainfall
} weather_t ;

static weather_t weather ;

// socat udp-recv:52003 udp-sendto:192.168.3.2:52003
static struct uip_udp_conn *weather_conn = NULL;
static u8_t first_run = 1u ;

static const u16_t weather_udp_port = 52003 ;

void display_str(const char *str)
{
#ifdef _CMOC_VERSION_
  putstr(str,strlen(str)) ;
#else
  fputs(str,stdout);
#endif
}

void weather_udp_appcall(void)
{
  if(uip_udp_conn->lport == HTONS(weather_udp_port)) 
  {
    if(uip_newdata()) 
    {
      char *weather_data = (char*)uip_appdata ;
      char *delim ;
      
      weather_data[uip_datalen()] = 0;

#ifdef DRAGON
       // clear screen on first run
       if ( first_run )
       {
         memset16(0x400,0x6060,0x100) ;
         first_run = 0u ;
       }
#endif
      // weather data comes in as a single line of text
      // comprising timestamp, temperature, windspeed, rainfall
      // each delimited by a space
#if !defined(_CMOC_VERSION_) || _CMOC_VERSION_>1090   
      weather.timestamp = strtoul(weather_data,&delim,10) ;
      weather.temp = strtof(delim,&delim) ;
      weather.wind = strtof(delim,&delim) ;
      weather.rain = strtof(delim,NULL) ;
#else
      u8_t toks = 0 ;

      delim = strtok(weather_data," ") ;
      while ( delim )
      {
        switch(toks)
        {
          case 0 :
            weather.timestamp = atoul(delim) ;
            break ;
          case 1 :
            weather.temp = atoff(delim);
            break ;
          case 2 :
            weather.wind = atoff(delim); ;
            break ;
          case 3:
            weather.rain = atoff(delim) ;
            break ;
        }
        toks++ ;
        delim = strtok(NULL," ") ;
      }
      if ( toks==4 )
#endif
      {
#ifdef DRAGON
       // output the data half way down the screen
       u16_t *current_cursor = (u16_t*)0x88 ;
       *current_cursor = 0x520 ;
#endif
        output_weather_data(display_str) ;
      }
    }
  }
}

void weather_udp_init(void)
{
  // create a UDP connection, listen on the weather data port
  weather_conn = uip_udp_new(0, 0);
  if (weather_conn)
  {
    uip_udp_bind(weather_conn, HTONS(weather_udp_port));
    printf( "Listening on UDP Port 52003\n") ;
  }
}

void output_weather_data(output_str_t output_str_cback) 
{
  char buf[36] ;
  struct tm data_tm ;

  gmtime_r(&weather.timestamp,&data_tm) ;
  asctime_r(&data_tm,buf) ;
  output_str_cback(buf) ;
  
  sprintf(buf,"TEMPERATURE : %f C\n", weather.temp);
  output_str_cback(buf) ;
  // the formatting of the wind isn't great but that's down to 
  // limitations of the CMOC compiler/BASIC ROM which handles the floating point numbers
  sprintf(buf,"WIND SPEED  : %f MPH\n", weather.wind);
  output_str_cback(buf) ;
  sprintf(buf,"RAINFALL    : %f MM\n", weather.rain);
  output_str_cback(buf) ;
}
