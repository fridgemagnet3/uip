#include "solar-udp.h"
#include "uip.h"
#include <time.h>
#ifndef _CMOC_VERSION_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define atoff(a) strtof(a,NULL)
#else
#include <cmoc.h>
#endif

typedef struct {
  time_t dataTimeStamp ;  // Unix timestamp
  unsigned short batteryCapacitySoc; // (%)
  float   batteryPower; // (kW)
  float   pac;    // generation (kW)
  float   psum;   // grid in/out (kW)
  float   familyLoadPower; // load (kW)
  float   eToday;  // generation today (kW)
} ModbusSolisRegister_t;

static ModbusSolisRegister_t SolarData ;

// socat udp-recv:52005 udp-sendto:192.168.3.2:52005 
static struct uip_udp_conn *solar_conn = NULL;

static const u16_t solar_udp_port = 52005 ;
static time_t last_timestamp ;

static int extract_quoted_string(const char *src, char *dst)
{
  char *start, *end ;
  int len ;

  start = strchr(src,'\"') ;
  
  if ( start )
  {
    end = strchr(start+1,'\"') ;
    if ( end )
    {
      len = end-start-1 ;
      strncpy(dst,start+1,len) ;
      dst[len] = 0 ;
      return len ;
    }
  }
  return -1 ;
}

// if the weather app is enabled, this also defines a display_str
// function so use that rather than generating our own
#ifndef APP_WEATHERUDP
static void display_str(const char *str)
{
#ifdef _CMOC_VERSION_
  putstr(str,strlen(str)) ;
#else
  fputs(str,stdout);
#endif
}
#else
extern void display_str(const char *str) ;
#endif

static void display_json_solar_data(char *json_data)
{
  char *delim, *ptr ;
  char name[100] ;
  char val[100] ;
  int len ;
  u8_t toks = 0 ;
  const u8_t sol_toks = 7 ;
  time_t timestamp ;
  
  // a very rough and ready JSON parser for the solar data..
  
  // break the string at each comma, this gives us each JSON name/value pair 
  delim = strtok(json_data,",") ;
  while(delim != NULL)
  {
    // extract the name
    if ( ( len = extract_quoted_string(delim,name)) > 0 )
    {
      // look for the delimiter to the value
      ptr = strchr(delim+len,':') ;
      if ( ptr )
      {
        ptr++ ;
        // the cmoc string to number routines don't seem to
        // like leading spaces...
        while((*ptr==' ') || (*ptr=='\t'))
          ptr++ ;
        // find and decode matches of interest
        if ( !strcmp(name,"dataTimestamp") )
        {
          if ( extract_quoted_string(ptr,val) > 0 )
          {
            // we don't have a 64-bit data type so this needs to be converted to a float
            // before storing in a 32-bit long
            float f = atoff(val);
            timestamp = (time_t)(f / 1000) ;
            if ( timestamp < last_timestamp )
              return ;
            SolarData.dataTimeStamp = timestamp ;
            toks++ ;
          }
        }
        else if ( !strcmp(name,"eToday") )
        {
          SolarData.eToday = atoff(ptr);
          toks++ ;
        }
        else if ( !strcmp(name,"pac") )
        {
          SolarData.pac = atoff(ptr);
          toks++ ;
        }
        else if ( !strcmp(name,"batteryCapacitySoc") )
        {
          SolarData.batteryCapacitySoc = atoi(ptr);
          toks++ ;
        }
        else if ( !strcmp(name,"batteryPower") )
        {
          SolarData.batteryPower = atoff(ptr);
          toks++ ;
        }
        else if ( !strcmp(name,"psum") )
        {
          SolarData.psum = atoff(ptr);
          toks++ ;
        }
        else if ( !strcmp(name,"familyLoadPower") )
        {
          SolarData.familyLoadPower = atoff(ptr);
          toks++ ;
        }
      }
    }
    delim = strtok(NULL,",") ;
  }

  if ( toks == sol_toks ) 
  {
#ifdef DRAGON
    // clear the screen and reposition cursor at top left
    memset16(0x400,0x6060,0x100) ;
    u16_t *current_cursor = (u16_t*)0x88 ;
    *current_cursor = 0x400 ;
#endif
    
    last_timestamp = SolarData.dataTimeStamp ;
    
    output_solar_metrics(display_str);
  }
}


void solar_udp_appcall(void)
{
  if(uip_udp_conn->lport == HTONS(solar_udp_port)) 
  {
    if(uip_newdata()) 
    {
      char *solar_data = (char*)uip_appdata ;
      solar_data[uip_datalen()] = 0;
      
      // sense check to see if this is solar metric data
      // or just some other string data - decide whether to decode or just 
      // print it out
      if ( strstr(solar_data,"\"code\":") )
        display_json_solar_data(solar_data);
      else
        printf("%s\n",solar_data);
    }
  }
}

void solar_udp_init(void)
{
  // create a UDP connection, listen on the solar metrics port
  solar_conn = uip_udp_new(0, 0);
  if (solar_conn)
  {
    uip_udp_bind(solar_conn, HTONS(solar_udp_port));
    printf( "Listening on UDP Port 52005\n") ;
  }
}

// output the solar metrics using the supplied callback.
// By implementing it in this manner, it avoids duplicating
// all of the string constants eg. if the telnet daemon 
// is enabled, is uses this same routine but to send the data
// down a TCP connection
void output_solar_metrics(output_str_t output_str_cback)
{
  char buf[36] ;
  struct tm data_tm ;

  gmtime_r(&SolarData.dataTimeStamp,&data_tm) ;
  asctime_r(&data_tm,buf) ;
  output_str_cback(buf) ;

  sprintf(buf,"POWER TODAY  : %f kW\n", SolarData.eToday);
  output_str_cback(buf) ;
  sprintf(buf,"GENERATION   : %f kW\n", SolarData.pac);
  output_str_cback(buf) ;
  sprintf(buf,"HOUSE LOAD   : %f kW\n", SolarData.familyLoadPower);
  output_str_cback(buf) ;
  sprintf(buf,"BATTERY SOC  : %u%%\n", SolarData.batteryCapacitySoc);
  output_str_cback(buf) ;
  sprintf(buf,"BATTERY POWER: %f kW\n", SolarData.batteryPower);
  output_str_cback(buf) ;
  sprintf(buf,"GRID         : %f kW\n", SolarData.psum);
  output_str_cback(buf) ;
}
