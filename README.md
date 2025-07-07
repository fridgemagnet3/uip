# A Dragon IP stack using uIP

This is a project to create an IPv4 stack for the old [Dragon 8-bit computer](https://en.wikipedia.org/wiki/Dragon_32/64), if my enthusasm continues, I plan to fiddle about with it to varying degrees over the next few months.

I should start by saying that if you're looking to shuffle data from the Internet (eg. download a file or web page) there are far easier ways of accomplishing this. For example you could use the [Drivewire protocol](https://archive.worldofdragon.org/index.php?title=DriveWire), where you offload the actual IP protocol to a more modern/capable machine. There's no real obvious point to doing this other than it seemed an interesting (mad?) thing to do. 

Originally I envisaged writing a very basic IP stack in assembler, really to just support the basic ARP and UDP protocols but then uncovered an archive of the UIP I'd downloaded about 10 years ago which seemed a better starting point (not least because it supports TCP as well).

At present the plan is for this to only run on a Dragon 64, not specifically because it has more memory than the 32 (although one of the demos does make use of the 64's additional memory for displaying a bitmap), rather that it has a RS232 port (which the 32 lacks). It's unlikely to be able to run on a D32, using a bitbanger type port (in a similar vein to Drivewire) due to the asynchronous nature of network traffic.

## Current status

At present the stack builds and runs on:

- [modified version of the XRoar emulator](https://github.com/fridgemagnet3/xroar) using a [Linux TAP device](https://en.wikipedia.org/wiki/TUN/TAP)
- Dragon 64 connected via serial to a [Linux TAP device](https://en.wikipedia.org/wiki/TUN/TAP)
- Dragon 64 connected via serial (and suitable transceiver) to a [Waveshare ESP32-S3-ETH microcontroller](/esp32-eth-slip-gw) - work in progress

The Dragon will respond to ping requests and the default configuration includes a simple telnet server application and a UDP based receiver of my own which listens on port 52005 and will display any textual data received. Coincidently, this happens to be the same port I use for [broadcasting my solar (JSON) data](https://github.com/fridgemagnet3/modbus-solis5g) but should work with any packets containing text. In the event it DOES contain solar JSON data, it will also decode & display it nicely:

![solar-weather-metrics](https://github.com/user-attachments/assets/8e00a911-3eaf-4bd7-bc88-7a983dbc8233)

I've also implemented another simple app to receive & decode data from my little [weather station](https://www.oasw.co.uk/weather/about.html).

The telnet server also allows this data to be retrieved:

`telnet 192.168.3.2`\
`Trying 192.168.3.2...`\
`Connected to 192.168.3.2.`\
`Escape character is '^]'.`\
`uIP command shell`\
`Type '?' and return for help`\
`uIP 1.0> help`\
`Available commands:`\
`solar   - show solar metrics`\
`weather - show weather data`\
`help, ? - show help`\
`exit    - exit shell`\
`uIP 1.0> solar `\
`Sun May 18 08:01:14 2025`\
`POWER TODAY  : 1.3 kW`\
`GENERATION   : .905 kW`\
`HOUSE LOAD   : .303 kW`\
`BATTERY SOC  : 29%`\
`BATTERY POWER: .606 kW`\
`GRID         : 3E-03 kW`\
`uIP 1.0> weather`\
`Sun May 18 08:01:37 2025`\
`TEMPERATURE : 12.437 C`\
`WIND SPEED  : 1.1777468 MPH`\
`RAINFALL    : 0 MM`\
`uIP 1.0> `

Both the webclient and DNS resolver applications should also work (although they're not currently enabled by default, should just be a case of adjusting the Makefile as needed). The webclient will work with or without the resover enabled (in case of the latter, you need to specify the web server by IP address) and will simply dump out the contents of the requested document. Both apps currently use IP addresses and names local to my network so will need changing to work. Just be aware that odds are if you try and connect to an external IP, it won't work unless you adjust your router/routing tables to connect to the subnet being used by the TAP interface.

Any application which uses the [protosockets library](doc/html/a00158.html) (including the simple [hello world](apps/hello-world) example) **won't work properly.** This is because the underlying [protothreads library](doc/html/a00142.html) makes a whacky use of the switch() call that is similar to something called the [Duff's device](https://en.wikipedia.org/wiki/Duff%27s_device) which the current incarnation of the CMOC (6809 cross) compiler specifically states it does not support. In a nutshell, the state machine used to track the TCP connection state gets repeatedly reset & confusion then rains.

## How to build/run the stack (Xroar emulator)

The stack is currently built using the [CMOC 6809 cross compiler](http://sarrazip.com/dev/cmoc.html) so you'll need to build/install this first. Then build the uIP stack:

`cd dragon`\
`make`

This results in a ~15-20K (depending which apps are enabled) DragonDOS compatible binary file named UIP.BIN. If you enable a lot of them, you will need to adjust the _org_ address in the Makefile to avoid it crashing into the ROM area.

You'll also need [the XRoar emulator](https://github.com/fridgemagnet3/xroar). Unfortunately the current XRoar releases don't emulate the Dragon 64's serial port, you'll therefore need to build my branch, which additionally only runs under Linux. This does a very basic emulation of the serial port, mapping read/write requests to two device files which are intended to be Linux FIFOs. As such, once you've built the emulator, you'll need to create these in the ~./xroar folder:

`mkdir -p ~/.xroar`\
`mkfifo ~./xroar/tx_uart`\
`mkfifo ~./xroar/rx_uart`

Every character then written to the serial port on the Dragon, will then be sent to the **tx_uart** file. Anything sent to the **rx_uart** file from the Linux side will then appear on the serial port. 

You then need to load the uIP binary image into the emulator. There are various ways of accomplishing this, including writing it to a virtual disk image, for ease of use I use an instance of Drivewire with the [Becker port](https://www.6809.org.uk/xroar/doc/xroar.shtml#Becker-port-options), it then just a case of loading it from the server:

`DLOAD "UIP.BIN`

That last command loads, then runs the application.

Next you'll need to build (under Linux) the [tap-slip-gw application](/tap-slip-gw):

`cd tap-slip-gw`\
`make`\
`sudo setcap 'cap_net_admin+ep' ./tap-slip-gw`

That last step allows you to run the application as a normal user (instead of 'root'), if you're not fussed about security you can omit it and run as 'root' anyway. This application brings up a Linux TAP interface, then sends/receives packets to the XRoar serial port FIFOs using [SLIP](https://en.wikipedia.org/wiki/Serial_Line_Internet_Protocol) (and hence to the uIP stack running in the emulator). The uIP binary itself is currently hardcoded to use an IP address of 192.168.3.2, with a gateway address of 192.168.3.1 so assuming that doesn't conflict with your network settings, should then just be a case of running the application as follows:

`./tap-slip-gw 192.168.3.1`

At which point you should then be able to __ping__ it:

`ping 192.168.3.2`

and get responses back from the Dragon.

![PXL_20250501_164229167](https://github.com/user-attachments/assets/ac6c7621-9615-4385-a421-4a6f26ad9ec3)

## Running on the Dragon 64

For this, you'll need to wire up a serial cable between the Dragon and Linux machine. As a minimum, you need to wire up RX,TX and GND. Note the following:

- the Dragon will NOT receive anything unless CTS (input) is asserted
- the Dragon will NOT transmit unless it has asserted DTR, the software automatically does this but also see below

As such, I also recommend you wire up the flow control pins, from the Dragon side DTR to CTS and CTS to RTS. 

The [serial driver](dragon/serial.c) uses DTR to reduce (prevent?) receive overruns and de-asserts DTR when transmitting large (>512K bytes or half the RX ring buffer size) packets. This means if you choose NOT to wire up the control lines, you may see receive overruns and additionally you can't just locally connect DTR to CTS without modifying the software as you'll potentially deadlock things. 

As of [4862108](https://github.com/fridgemagnet3/uip/commit/4862108f67842cfb450d10b7e0e31bf8a1737191), I've re-engineered the serial driver to allow for a configurable RX ring buffer size (previously it was fixed at 256 bytes) as I was starting to see overruns whilst some of the sample applications were running. Additionally, I've relocated the buffer to the first graphics page in memory (nominally $600 or $C00 if a DOS is present). The default value is 1Kbytes and this seems to have improved things significantly, to the extent where hardware flow control may not be required but some more testing is required....

Unsurprisingly, the build/setup process is pretty similiar when using the emulator. When building the stack, enable the serial driver:

`cd dragon`\
`CFLAGS=-DSERIAL_DRIVER make`

This configures the 6551 to run at it's maximum baud rate of 19200. On the Linux side, configure the serial port to match - for example:

`stty -F /dev/ttyUSB0 19200 raw -echo crtscts cstopb`

Note the use of the **crtscts** option, this enables hardware flow control so if you've NOT wired this up, don't set it.

Build the __tap-slip-gw__ app, this time when running it, pass in the name of the serial device to override the use of the Xroar FIFOs.

`./tap-slip-gw 192.168.3.1 0 /dev/ttyUSB0`

At which point you should be able to successfully ping it and start playing with the other apps.

![PXL_20250524_133458707](https://github.com/user-attachments/assets/6a375ef9-8c43-4782-9505-a0aff4759a8e)

Note that the serial driver will also work with the emulator however there's no real benefit in operating it in that mode, you'll need to throttle the transmits from the tap-slip-gw app in order to avoid massive overruns which ultimately makes everything run much slower. See my comments at the top of the [serial.c](dragon/serial.c) file.


## Packet Filter

There is a rudimentary packet filter built into both the [tap-slip-gw](/tap-slip-gw) and [esp32-eth-slip-gw](/esp32-eth-slip-gw) applications which serves to try and stop the Dragon from being overloaded with unrequired network traffic. The filter performs the following:

- forwards any ARP packets (these are typically required for any higher level IP protocols to operate) to the Dragon (serial interface)
- drops any non IPv4 packets
  
There is then a configurable option regarding the handling of UDP broadcast packets whereby the filter can operate in one of two modes:

- forward all UDP broadcast packets to the Dragon
- drop all UDP broadcast packets accept for those specifically allowed through via a list of port exceptions

The [tap-slip-gw](/tap-slip-gw) application is set to drop all packets. Given the TAP interface is point to point, unless broadcast packets are specifically injected there is generally no need to let them through. This can of course be changed by modifying the software - see the [packet-filter.h](/tap-slip-gw/packet-filter.h) for details.

The [esp32-eth-slip-gw](/esp32-eth-slip-gw) application is set to drop all packets except for DHCP. This behaviour can be changed via the settings in [esp32-eth-slip-gw/config.h](/esp32-eth-slip-gw/config.h). 
