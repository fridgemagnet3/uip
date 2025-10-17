/*
 * time.h
 * 
 * Struct and function declarations for dealing with time.
 */
 
#ifndef _TIME_H_
#define _TIME_H_

struct tm
{
  int	tm_sec;
  int	tm_min;
  int	tm_hour;
  int	tm_mday;
  int	tm_mon;
  int	tm_year;
  int	tm_wday;
  int	tm_yday;
  int	tm_isdst;
#ifdef __TM_GMTOFF
  long	__TM_GMTOFF;
#endif
#ifdef __TM_ZONE
  const char *__TM_ZONE;
#endif
};

// making this unsigned might make us Y2038 compliant (least for another 50 odd years) :)
#define _TIME_T_ unsigned long
typedef	_TIME_T_ time_t;

#define _CLOCK_T_ unsigned short 
typedef _CLOCK_T_ clock_t ;

// adjust to 60 if you're in the US or somewhere else exotic
#define CLOCKS_PER_SEC 50

struct tm *gmtime (const time_t *_timer);

char *asctime (const struct tm *tim_p);
char *asctime_r	(const struct tm *tim_p,
				 char *result);

struct tm *gmtime_r (const time_t *tim_p,
				     struct tm *res);

// return system time
// note that in the absence of anything calling
// 'stime', the time will originate from the point
// this library was built
time_t time(time_t *tloc) ;

// set system time
int stime(const time_t *t) ;

#endif

