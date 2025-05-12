#
# Makefile for building the Dragon binary
# using the CMOC-C cross compiler
#

all: libuip.a libapps.a UIP.BIN

CC     = cmoc
AR     = lwar
APPS   = telnetd solar-udp
CFLAGS+= -I../uip -I../lib -I. -I../time --dragon -c
#CFLAGS+= -DSERIAL_DRIVER
ARFLAGS = -c
LDFLAGS+= -i --dragon --org=0x2000
LIBS = -L../time -ltime -L./ -luip -lapps
-include ../uip/Makefile.include

$(OBJECTDIR)/%.o: %.s
	$(CC) $(CFLAGS) -o $@ -c $<

OBJS = $(addprefix $(OBJECTDIR)/, main.o clock-arch.o slipdev.o uip_arch.o serial.o sy6551_int_handler.o) 

UIP.BIN: $(OBJS) libuip.a libapps.a
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	# needed to load via Drivewire
	./pad256.sh $@
	
clean:
	rm -fr *.o *~ *core UIP.BIN $(OBJECTDIR) *.a
