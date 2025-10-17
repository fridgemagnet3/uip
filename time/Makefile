#
# time library
#
all: libtime.a

# the build timestamp will be used to initialise
# the starting point for "time()"
BUILD_TIMESTAMP=$(shell date +%s)
CFLAGS+=-DBUILD_TIMESTAMP=$(BUILD_TIMESTAMP)

CC     = cmoc
AR     = lwar
CFLAGS+= --dragon -c
ARFLAGS = -c

libtime.a: asctime.o asctime_r.o gmtime.o gmtime_r.o time.o
	$(AR) $(ARFLAGS) $@ $^
	
%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o
	rm -f *.a
	