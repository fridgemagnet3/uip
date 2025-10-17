#include "time.h"

// origin timestamp, defaults to the point the library was built
static time_t origin = BUILD_TIMESTAMP ;

// location of the system timer
static const clock_t *sys_timer = (clock_t*)0x112 ;
// offset to subtract from the timer
static unsigned short timer_offset = 0 ;

// get the system time
time_t time(time_t *tloc) 
{
  clock_t elapsed_s = (*sys_timer)/CLOCKS_PER_SEC  ;
  
  // in the absence of anyone calling 'stime'
  // this will simply equate to the library build time + the no.
  // of seconds since the Dragon has been powered. Note that this
  // DOES NOT account for wrapping back to zero (which will happen after about
  // 3 hours)
  time_t ret = origin+elapsed_s-timer_offset ;
  if (tloc)
    *tloc = ret ;
  return ret ;
}

// set the system time
int stime(const time_t *t)
{
  if (t)
  {
    origin = *t ;
    // the timer_offset is subtracted from the timer
    // when time() is called which ensures that the time
    // originates from the point this is called, rather than earlier
    timer_offset = (*sys_timer)/CLOCKS_PER_SEC ;
    return 0 ;
  }
  else
    return -1 ;
}
