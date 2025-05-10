#
# Makefile for building the Dragon binary
# using the CMOC-C cross compiler
#

all: UIP.BIN

CC     = cmoc
AR     = lwar
APPS   = telnetd solar-udp
CFLAGS = -I../uip -I../lib -I. --dragon -c
ARFLAGS = -c
LDFLAGS = --dragon --org=0x2000
-include ../uip/Makefile.include

$(OBJECTDIR)/%.o: %.s
	$(CC) $(CFLAGS) -o $@ -c $<

UIP.BIN: $(addprefix $(OBJECTDIR)/, main.o clock-arch.o slipdev.o uip_arch.o serial.o sy6551_int_handler.o) apps.a uip.a
	$(CC) $(LDFLAGS) -o $@ $^
	# needed to load via Drivewire
	./pad256.sh $@
	
clean:
	rm -fr *.o *~ *core UIP.BIN $(OBJECTDIR) *.a
