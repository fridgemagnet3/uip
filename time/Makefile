#
# time library
#
all: libtime.a

CC     = cmoc
AR     = lwar
CFLAGS+= --dragon -c
ARFLAGS = -c

libtime.a: asctime.o asctime_r.o gmtime.o gmtime_r.o
	$(AR) $(ARFLAGS) $@ $^
	
%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o
	rm -f *.a
	