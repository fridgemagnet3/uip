#
# Makefile for building the Dragon binary
# using the CMOC-C cross compiler
#

all: UIP.BIN

CC     = cmoc
AR     = lwar
APPS   = hello-world
CFLAGS = -I../uip -I. --dragon -c
LDFLAGS = --dragon --org=0x4000
-include ../uip/Makefile.include

UIP.BIN: $(addprefix $(OBJECTDIR)/, main.o clock-arch.o slipdev.o uip_arch.o) apps.a uip.a
	$(CC) $(LDFLAGS) -o $@ $^
	# needed to load via Drivewire
	./pad256.sh $@
	
clean:
	rm -fr *.o *~ *core UIP.BIN $(OBJECTDIR) *.a
