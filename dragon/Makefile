#
# Makefile for building the Dragon binary
# using the CMOC-C cross compiler
#

all: UIP.BIN

CC     = cmoc
AR     = lwar
APPS   = telnetd
CFLAGS = -I../uip -I../lib -I. --dragon -c
ARFLAGS = -c
LDFLAGS = --dragon --org=0x3000
-include ../uip/Makefile.include

UIP.BIN: $(addprefix $(OBJECTDIR)/, main.o clock-arch.o slipdev.o uip_arch.o) apps.a uip.a
	$(CC) $(LDFLAGS) -o $@ $^
	# needed to load via Drivewire
	./pad256.sh $@
	
clean:
	rm -fr *.o *~ *core UIP.BIN $(OBJECTDIR) *.a
