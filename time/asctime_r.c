/*
 * asctime_r.c
 */

#include <cmoc.h>
#include "time.h"

const char *day_name[7] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
  };
const char *mon_name[12] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

char *
asctime_r (const struct tm *tim_p,
	char *result)
{
  sprintf (result, "%s %s%3d %02d:%02d:%02d %d\n",
	    day_name[tim_p->tm_wday], 
	    mon_name[tim_p->tm_mon],
	    tim_p->tm_mday, tim_p->tm_hour, tim_p->tm_min,
	    tim_p->tm_sec, 1900 + tim_p->tm_year);
  return result;
}
