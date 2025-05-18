# A Dragon IP stack using uIP

This is (currently) an early project to create an IPv4 stack for the old [Dragon 8-bit computer](https://en.wikipedia.org/wiki/Dragon_32/64), if my enthusasm continues, I plan to fiddle about with it to varying degrees over the next few months.

I should start by saying that if you're looking to shuffle data from the Internet (eg. download a file or web page) there are far easier ways of accomplishing this. For example you could use the [Drivewire protocol](https://archive.worldofdragon.org/index.php?title=DriveWire), where you offload the actual IP protocol to a more modern/capable machine. There's no real obvious point to doing this other than it seemed an interesting (mad?) thing to do. Notionally I have this idea that the finished solution will be using something like the [ESP32 Ethernet kit](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-ethernet-kit/user_guide.html) connected to the Dragon via RS232 but we're some way off that right now!

Originally I envisaged writing a very basic IP stack in assembler, really to just support the basic ARP and UDP protocols but then uncovered an archive of the UIP I'd downloaded about 10 years ago which seemed a better starting point (not least because it supports TCP as well).

At present the plan is for this to only run on a Dragon 64, not specifically because it has more memory than the 32 but that it has a RS232 port (which the 32 lacks). In theory it could run on a 32, using the parallel port as a bitbanger (this is after all what Drivewire does) but that's not something I'm planning on tackling any time soon.

## Current status

At present, the stack builds and runs on a [modified version of the XRoar emulator](https://github.com/fridgemagnet3/xroar) AND a physical Dragon 64. When interfaced to a  [Linux TAP device](https://en.wikipedia.org/wiki/TUN/TAP), it will respond to ping requests and the current configuration includes a simple telnet server application and some simple UDP based receivers of my own, including one which listens on port 52005 and will display any textual data received. Coincidently, this happens to be the same port I use for [broadcasting my solar (JSON) data](https://github.com/fridgemagnet3/modbus-solis5g) but should work with any packets containing text. In the event it DOES contain solar JSON data, it will also decode & display it nicely:

![solar-weather-metrics](https://github.com/user-attachments/assets/8e00a911-3eaf-4bd7-bc88-7a983dbc8233)

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

Any application which uses the [protosockets library](doc/html/a00158.html) (including the simple [hello world](apps/hello-world) example) **won't work properly.** This is because the underlying [protothreads library](doc/html/a00142.html) makes a whacky use of the select() call that is similar to something called the [Duff's device](https://en.wikipedia.org/wiki/Duff%27s_device) which the current incarnation of the CMOC (6809 cross) compiler specifically states it does not support. In a nutshell, the state machine used to track the TCP connection state gets repeatedly reset & confusion then rains.

## How to build/run the stack (Xroar emulator)

The stack is currently built using the [CMOC 6809 cross compiler](http://sarrazip.com/dev/cmoc.html) so you'll need to build/install this first. Then build the uIP stack:

`cd dragon`\
`make`

This results in a ~15K DragonDOS compatible binary file named UIP.BIN

You'll also need [the XRoar emulator](https://github.com/fridgemagnet3/xroar). Unfortunately the current XRoar releases don't emulate the Dragon 64's serial port, you'll therefore need to build my branch, which additionally only runs under Linux. This does a very basic emulation of the serial port, mapping read/write requests to two device files which are intended to be Linux FIFOs. As such, once you've built the emulator, you'll need to create these in the ~./xroar folder:

`mkdir -p ~/.xroar`\
`mkfifo ~./xroar/tx_uart`\
`mkfifo ~./xroar/rx_uart`

Every character then written to the serial port on the Dragon, will then be sent to the **tx_uart** file. Anything sent to the **rx_uart** file from the Linux side will then appear on the serial port. 

You then need to load the uIP binary image into the emulator. There are various ways of accomplishing this, including writing it to a virtual disk image, for ease of use I use an instance of Drivewire with the [Becker port](https://www.6809.org.uk/xroar/doc/xroar.shtml#Becker-port-options). At present, with the image being ~19Kbytes in size, I currently target this at the 8K address offset which then gives a reasonable bit of room for growth - something like:

`CLEAR 512,&H2000`\
`DLOAD "UIP.BIN`

That last command loads, then runs the application.

Next you'll need to build (under Linux) the [tap-slip-gw application](/tap-slip-gw):

`cd tap-slip-gw`\
`make`\
`sudo setcap 'cap_net_admin+ep' ./tap-slip-gw`\

That last step allows you to run the application as a normal user (instead of 'root'), if you're not fussed about security you can omit it and run as 'root' anyway. This application brings up a Linux TAP interface, then sends/receives packets to the XRoar serial port FIFOs using [SLIP](https://en.wikipedia.org/wiki/Serial_Line_Internet_Protocol) (and hence to the uIP stack running in the emulator). The uIP binary itself is currently hardcoded to use an IP address of 192.168.3.2, with a gateway address of 192.168.3.1 so assuming that doesn't conflict with your network settings, should then just be a case of running the application as follows:

`./tap-slip-gw 192.168.3.1`

There's normally a brief burst of network traffic which the Dragon takes a little while to digest but after a few seconds it should be able to __ping__ it:

`ping 192.168.3.2`

and get responses back from the Dragon.

![PXL_20250501_164229167](https://github.com/user-attachments/assets/ac6c7621-9615-4385-a421-4a6f26ad9ec3)

## Running on the Dragon 64

For this, you'll need to wire up a serial cable between the Dragon and Linux machine. In addition to the usual 3-wires required (RX,TX,GND), I also recommend you wire up the flow control pins, from the Dragon side DTR to CTS and CTS to RTS. The [serial driver](dragon/serial.c) uses DTR to reduce (prevent?) receive overruns, it'll still work if you choose not to (although from memory, I think the 6551 requires CTS to be asserted before it will transmit) however you might see dropped bytes, particularly when sending large (>1KByte) packets.

Unsurprisingly, the build/setup process is pretty similiar when using the emulator. When building the stack, enable the serial driver:

`cd dragon`\
`CFLAGS=-DSERIAL_DRIVER make`

This configures the 6551 to run at it's maximum baud rate of 19200. On the Linux side, configure the serial port to match - for example:

`stty -F /dev/ttyUSB0 19200 raw -echo crtscts ctopb`

Note the use of the **crtscts** option, this enables hardware flow control so if you've NOT wired this up, don't set it.

Build the __tap-slip-gw__ app, this time when running it, pass in the name of the serial device to override the use of the Xroar FIFOs.

`./tap-slip-gw 192.168.3.1 0 /dev/ttyUSB0`

Again, you'll need to allow a minute or two for the Dragon to digest the burst of traffic that tends to occur whenever a new interface is brought up but after that you should be able to successfully ping it and start playing with the other apps.
