// Clock arch specific definition for the Dragon
// This uses the timer provided by the 50Hz (in the UK!!)
// video frame sync. 

#include "clock-arch.h"

clock_time_t clock_time(void)
{
  // location of the TIMER value
  const clock_time_t *sys_timer = (clock_time_t*)0x112 ;
  
  return *sys_timer ;
}
