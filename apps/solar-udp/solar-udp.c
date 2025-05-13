#include "solar-udp.h"
#include "uip.h"
#include <time.h>
#ifndef _CMOC_VERSION_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

static void display_json_solar_data(char *json_data)
{
  char *delim, *ptr ;
  char name[100] ;
  char val[100] ;
  int len ;
  struct tm *data_tm ;
  u8_t toks = 0 ;
  const u8_t sol_toks = 7 ;

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
            float f = strtof(val,NULL);
            SolarData.dataTimeStamp = (time_t)(f / 1000) ;
            toks++ ;
          }
        }
        else if ( !strcmp(name,"eToday") )
        {
          SolarData.eToday = strtof(ptr,NULL);
          toks++ ;
        }
        else if ( !strcmp(name,"pac") )
        {
          SolarData.pac = strtof(ptr,NULL);
          toks++ ;
        }
        else if ( !strcmp(name,"batteryCapacitySoc") )
        {
          SolarData.batteryCapacitySoc = atoi(ptr);
          toks++ ;
        }
        else if ( !strcmp(name,"batteryPower") )
        {
          SolarData.batteryPower = strtof(ptr,NULL);
          toks++ ;
        }
        else if ( !strcmp(name,"psum") )
        {
          SolarData.psum = strtof(ptr,NULL);
          toks++ ;
        }
        else if ( !strcmp(name,"familyLoadPower") )
        {
          SolarData.familyLoadPower = strtof(ptr,NULL);
          toks++ ;
        }
      }
    }
    delim = strtok(NULL,",") ;
  }

  if ( (SolarData.dataTimeStamp > last_timestamp) && (toks == sol_toks))
  {
#ifdef DRAGON
    // clear the screen and reposition cursor at top left
    memset16(0x400,0x6060,0x100) ;
    u16_t *current_cursor = (u16_t*)0x88 ;
    *current_cursor = 0x400 ;
#endif
    
    last_timestamp = SolarData.dataTimeStamp ;
    data_tm = gmtime(&SolarData.dataTimeStamp) ;
  
    printf("SOLAR: %s\n", asctime(data_tm));

    printf("POWER TODAY  : %f kW\n", SolarData.eToday);
    printf("GENERATION   : %f kW\n", SolarData.pac);
    printf("HOUSE LOAD   : %f kW\n", SolarData.familyLoadPower);
    printf("BATTERY SOC  : %u%%\n", SolarData.batteryCapacitySoc);
    printf("BATTERY POWER: %f kW\n", SolarData.batteryPower);
    printf("GRID         : %f kW\n", SolarData.psum);
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
