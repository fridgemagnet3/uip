// Clock arch specific definition for the Dragon
// This uses the timer provided by the 50Hz (in the UK!!)
// video frame sync. 
// Note that as it stands, this will wrap at around 3 and
// a bit hours of power

#ifndef CLOCK_ARCH_H

#define CLOCK_ARCH_H

typedef unsigned short clock_time_t;

// adjust to 60 if you're in the US or somewhere else exotic
#define CLOCK_CONF_SECOND 50

#endif
