This library provides a minimal set of date/time functions primarily
for converting Unix time (time_t) into a human readable format. The
only time zone supported is UTC, with no daylight savings logic.

This is a modified subset of the corresponding files from newlib 4.5.0
see COPYING.NEWLIB for licensing information.

In the absence of adjusting the time via the "stime()" call, the
"time()" call will originate at the point the library was built, offset
by the system timer (TIMER function from BASIC).
