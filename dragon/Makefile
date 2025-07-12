#
# Makefile for building the Dragon binary
# using the CMOC-C cross compiler
#

all: libuip.a libapps.a UIP.BIN

CC     = cmoc
AR     = lwar
# define the apps included in the binary
APPS   = telnetd weather-udp solar-udp callchain
#APPS    = webclient resolv callchain
CFLAGS+= -I../uip -I../lib -I. -I../time --dragon -c 
#CFLAGS+= -DSERIAL_DRIVER 

# set compiler flags based on enabled apps
ifneq (,$(findstring telnetd,$(APPS)))
CFLAGS+=-DAPP_TELNETD
endif
ifneq (,$(findstring solar-udp,$(APPS)))
CFLAGS+=-DAPP_SOLARUDP
LIBS = -L../time -ltime
endif
ifneq (,$(findstring weather-udp,$(APPS)))
CFLAGS+=-DAPP_WEATHERUDP
LIBS = -L../time -ltime
endif
ifneq (,$(findstring webclient,$(APPS)))
CFLAGS+=-DAPP_WEBCLIENT
# set to download a graphics image otherwise
# a text file is used
CFLAGS+=-DWEB_GRAPHICS
endif
ifneq (,$(findstring resolv,$(APPS)))
CFLAGS+=-DAPP_RESOLV
endif
ifneq (,$(findstring dhcpc,$(APPS)))
CFLAGS+=-DAPP_DHCPC
endif
ifneq (,$(findstring callchain,$(APPS)))
CFLAGS+=-DAPP_CALLCHAIN
endif

ARFLAGS = -c
LDFLAGS+= -i --dragon --org=0x1500
LIBS+= -L./ -luip -lapps
-include ../uip/Makefile.include

$(OBJECTDIR)/%.o: %.s
	$(CC) $(CFLAGS) -o $@ -c $<

OBJS = $(addprefix $(OBJECTDIR)/, main.o clock-arch.o slipdev.o uip_arch.o) 
ifneq (,$(findstring WEB_GRAPHICS,$(CFLAGS)))
OBJS+=$(addprefix $(OBJECTDIR)/,mapmode.o graphics.o)
endif
ifneq (,$(findstring SERIAL_DRIVER,$(CFLAGS)))
OBJS+=$(addprefix $(OBJECTDIR)/,sy6551_int_handler.o)
endif

UIP.BIN: $(OBJS) libuip.a libapps.a
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	# needed to load via Drivewire
	./pad256.sh $@
	
clean:
	rm -fr *.o *~ *core UIP.BIN $(OBJECTDIR) *.a
