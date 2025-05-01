CC?=gcc
CFLAGS=-g -O2 -D_FILE_OFFSET_BITS=64 -fmessage-length=0 -fPIC 

OBJS=tap-slip-gw.o
LIBS=-lcap
APP=tap-slip-gw

all: $(APP)

$(APP): $(OBJS)
	$(CC) -o $(APP) $^ $(LIBS) 

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean install
clean:
	rm -f *.o
	rm -f $(APP)

