#include <cmoc.h>
#include "time.h"

int main(void)
{
  // 	Sun May 11 16:35:08 2025 UTC
  time_t TestTime = 1746981308 ;
  char *TimeStr ;
  struct tm *Tm ;
  
  Tm = gmtime(&TestTime) ;
  printf("Year: %d\n", Tm->tm_year) ;
  printf("Month: %d\n", Tm->tm_mon) ;
  printf("Day: %d\n", Tm->tm_mday) ;
  printf("Hour: %d\n", Tm->tm_hour) ;
  printf("Min: %d\n", Tm->tm_min) ;
  printf("Second: %d\n", Tm->tm_sec) ;
  
  TimeStr = asctime(Tm) ;
  printf("%s\n", TimeStr) ;
  return 0 ; 
}
