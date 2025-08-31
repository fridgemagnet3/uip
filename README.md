# A Dragon IP stack using uIP

This is a project to create an IPv4 stack for the old [Dragon 8-bit computer](https://en.wikipedia.org/wiki/Dragon_32/64)

I should start by saying that if you're looking to shuffle data from the Internet (eg. download a file or web page) there are far easier ways of accomplishing this. For example you could use the [Drivewire protocol](https://archive.worldofdragon.org/index.php?title=DriveWire), where you offload the actual IP protocol to a more modern/capable machine. There's no real obvious point to doing this other than it seemed an interesting (mad?) thing to do. 

Originally I envisaged writing a very basic IP stack in assembler, really to just support the basic ARP and UDP protocols but then uncovered an archive of the UIP I'd downloaded about 10 years ago which seemed a better starting point (not least because it supports TCP as well).

At present this only runs on a Dragon 64, not specifically because it has more memory than the 32 (although one of the demos does make use of the 64's additional memory for displaying a bitmap), rather that it has a RS232 port (which the 32 lacks). It's unlikely to be able to run on a D32, using a bitbanger type port (in a similar vein to Drivewire) due to the asynchronous nature of network traffic.

## Current status

At present the stack builds and runs on:

- [modified version of the XRoar emulator](https://github.com/fridgemagnet3/xroar) using a [Linux TAP device](https://en.wikipedia.org/wiki/TUN/TAP)
- Dragon 64 connected via serial to a [Linux TAP device](https://en.wikipedia.org/wiki/TUN/TAP)
- Dragon 64 connected via serial (and suitable transceiver) to a [Waveshare ESP32-S3-ETH microcontroller](/esp32-eth-slip-gw)

In all build configurations, the Dragon will respond to ping requests. 

The default configuration includes a simple UDP server which listens on port 52005 and will display any textual received on the screen. You can use something like [netcat](https://nc110.sourceforge.io/) to demonstrate this:

<img width="640" height="323" alt="hello-dragon" src="https://github.com/user-attachments/assets/5f4eabe1-3b9d-40c3-a1dd-694edae3052b" />

Coincidently, this happens to be the same port I use for [broadcasting my solar (JSON) data](https://github.com/fridgemagnet3/modbus-solis5g) and in the (probably rare) event it DOES contain solar JSON data, it will also decode & display it nicely:

![solar-weather-metrics](https://github.com/user-attachments/assets/8e00a911-3eaf-4bd7-bc88-7a983dbc8233)

Also pictured is the output of another app which receives & decodes UDP data from my [weather station](https://www.oasw.co.uk/weather/about.html).

The configuration also includes a telnet server which allows this data to be retrieved:

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

Which applications are included is controlled by settings in the [Makefile](dragon/Makefile), this includes a DHCP client:

![PXL_20250628_084356960_crop](https://github.com/user-attachments/assets/f238efca-10fe-4435-bc4a-8e5f753ece88)

And a DNS resolver and webclient applications. The webclient demo configuration can be used to (optionally) perform a DNS lookup, then download a bitmap:

![PXL_20250723_180706459_crop](https://github.com/user-attachments/assets/bd8b3532-e7ea-4a8b-a1cc-5ebdae2ba70f)

and display it on the Dragon's monitor:

![PXL_20250705_140002674_crop](https://github.com/user-attachments/assets/4de47aad-5c21-4a78-a4ea-ee1b23967b1e)

This only works if you've set things like IP routing/gateways etc. to allow the Dragon to reach the wider Internet - ie. the configuration includes both DHCP and DNS components. In all other configurations you'll need to modify the various IP addresses that are hardcoded into the [main application](dragon/main.c). 

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
- the Dragon will NOT transmit unless it has asserted DTR, the software automatically does this when it starts

At present hardware flow control is NOT used by the software however there may be circumstances where it could be useful - see the section on [serial overruns](#serial-overruns-and-the-use-and-abuse-of-hardware-handshaking) below. 

As such, you **either** need to wire up the flow control pins, from the Dragon side DTR to CTS and CTS to RTS **OR** tie CTS to +12V (or DTR).

As of [4862108](https://github.com/fridgemagnet3/uip/commit/4862108f67842cfb450d10b7e0e31bf8a1737191), I've re-engineered the serial driver to allow for a configurable RX ring buffer size (previously it was fixed at 256 bytes) as I was starting to see overruns whilst some of the sample applications were running. Additionally, I've relocated the buffer to the first graphics page in memory (nominally $600 or $C00 if a DOS is present). The default value is 1600 bytes and this seems to have improved things significantly. There's more information on this and why/when you may see issues in the [serial overruns](#serial-overruns-and-the-use-and-abuse-of-hardware-handshaking) section below.

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

That number (0) between the IP address and serial device specifies the inter-character delay (in microseconds) between each character that is transmitted. This can also help in the case of [serial overruns](#serial-overruns-and-the-use-and-abuse-of-hardware-handshaking), at the cost of transfer speed.

Note that the serial driver will also work with the emulator however there's no real benefit in operating it in that mode (it's primarily useful for driver debugging purposes) and will ultimately run slower. If you do opt to use it in that mode, you'll need to specify an inter-character delay of at least 400/500us in order to avoid massive overruns (which roughly equates to a 19200 baud rate).

### Serial overruns and the use (and abuse) of hardware handshaking

Serial overruns can manifest themselves in two ways.

The first is when the serial driver writes to the ring buffer faster than the application code can read out of it. There is a counter that the interrupt handler increments every it drops a byte due to the buffer being full and this is reported by the application code the next time round the main processing loop. This will typically be followed by one or more errors from the uIP core - checksum failure or unexpected packet size. 

There's a few ways you can try and deal with this:

- Reduce the processing performed on the incoming packet data.
- Look at the [packet filter](#packet-filter) configuration, see if more packets are coming through that aren't used by the Dragon.
- Increase the ring buffer size. On a non DOS Dragon, it should be safe to near enough double the size to 3Kbytes, possibly more depending on what applications have been enabled.
- specify a non-zero inter-character delay
- Use hardware flow control to tell the sender to stop transmitting data.

This last point is worthy of a bit more detail in that I had planned and indeed did some experiments around using this approach ie. de-assert DTR when the ring buffer reached a certain size. The problem with this is that the 6551 immediately stops clocking in data which means that any byte that is currently being transmitted (and possibly a few more depending on the implementation of the sender) will be dropped. This behaviour can be seen on the following trace:

<img width="788" height="319" alt="dtr-drop" src="https://github.com/user-attachments/assets/552216b5-9eda-4c23-9b3a-09b4543e8cbe" />

Here, we can see interrupts being raised for the first two bytes coming in however none are generated after DTR is de-asserted - those last two are therefore lost. Since there is no way to predict when a packet will start being transmitted, within the core packet transmit/receive code there is never an occassion where it is safe to de-assert DTR and not run the risk of losing data. That said, there may well be application specific instances where the incoming packets **are** predictable and/or minor data loss is acceptabe & therefore whilst the core code itself never uses this mechanism, an interface is provided in the [serial.h](dragon/serial.h) header file.

The second scenario is when the serial driver IRQ service routine fails to read data from the 6551 fast enough. There is an overrun bit in the status register which can be used to test for this condition however in order to reduce the interrupt handler latency (more on that in a bit), this isn't explicitly checked (although there is some commented out code in the handler which can be enabled which will cause it to increment the overrun counter). As it stands then, this condition manifests itself as an error from the uIP core - checksum failure, unexpected packet size however no serial overruns are reported.

At 19200 baud, bytes are coming in at a rate of one every 0.5ms, that means the Dragon is having to work pretty hard to keep up. It's worse that though because the time between 1 byte fully arriving and the next one starting to be clocked in by the 6551 is around 110us - meaning the interrupt handler has that amount of time to fetch the data before it gets overwritten. Also as you can see, the interrupt is asserted some time after the serial stop bit has arrived so it's even less than that!

<img width="658" height="394" alt="inter-char-delay" src="https://github.com/user-attachments/assets/0dd0ad5f-d7f7-4550-b290-b76bb3bc7c71" />

This is why they invented things like the 16550 UART with it's 16 byte FIFO, even early PCs struggled to keep up with baud rates much beyond this.

The following trace shows a typical 6809 IRQ response time with the serial driver, it's around 42us so _should_ be fine.

<img width="494" height="380" alt="irq-service" src="https://github.com/user-attachments/assets/ecf3da09-8351-435e-9cd6-e7f505e20e3d" />

Rough estimate shows this to be about right:

18 cycles = Interrupt stacking and vector sequence (datasheet)\
14 cycles = IRQ service routine (see timing diagram at the end of this section)\
Assume ~5 cycle to complete current instruction\
Total 37 cycles * MC6809 @ 0.9Mhz = ~1.1us per instruction = ~41us

Despite that though, you can still get 6551 serial overruns, typically every few packets (particularly if they are large). Even accounting for time consuming instructions (some of the branches can take ~9 cycles), that still should complete within plenty of time. I suspect where the problem lies is when a serial AND the 50Hz timer interrupt occur more or less at the same time:

<img width="450" height="445" alt="serial-clock-irq" src="https://github.com/user-attachments/assets/dfbc63cc-fa79-4aa3-b502-dd52dc59cfd2" />

CB1 is the 50Hz frame sync from the 6821. Here it's occurring very shortly before one from the 6551, the net effect is that this delays the servicing of the latter such that we're into overrun territory - the first bit is already in the process of being clocked in at the point we read out the previous byte.

To (partially) address this, the interrupt handler uses a variation of the approach used by the WD2797 disk controller logic. For those unfamiliar, the 6809 simply can't keep up with reading data from the controller if each byte were transferred via an IRQ. Instead, it masks all interrupts, then goes into a tight loop using the SYNC instruction which is woken up by the 2797 raising an FIRQ, at which point the next byte is read from the controller and it goes around the loop again. At the end of the sector, the controller raises an NMI which breaks out the loop.

Here I do something similar in that when the first serial interrupt is raised, after reading out the data, it stays in the handler waiting on a SYNC instruction on the premise another byte will be along shortly. This continues until another non serial interrupt is detected (ie. the 50Hz timer) at which point it returns from the handler.

This is what it looks like in practice:

<img width="989" height="327" alt="irq-sync" src="https://github.com/user-attachments/assets/fac8f2c7-8838-470d-ac51-576ac0ca2264" />

You can see the much shorter response times when it's servicing the 6551 from within the existing handler. When the 20ms interrupt (CB1) fires, that's the interrupt which finally returns from the handler, back to the application. The next serial interrupt then takes that bit longer again as it's going through the full interrupt raise processing again, subsequently they are again much shorter. Significantly so, around 14us:

<img width="579" height="480" alt="irq-sync-service" src="https://github.com/user-attachments/assets/c2605a5d-1e82-430d-b61c-daa9ae090010" />

The net effect is that this significantly reduces those delays where two interrupts occur around the same time. It doesn't _eliminate_ them (and therefore the odd overrun can still happen) because in the case where the timer interrupt comes in just before the serial one, that will always delay things. However it does improve the situation when the order is reversed.

What this means is that the application code is pretty much locked out for significant periods whilst a packet is being received however given how busy the processor is servicing all those interrupts, it really doesn't make that much difference. 

Of course the other approach to all of this is to reduce the baud rate (or add delays on the transmit side) but frankly where's the fun in that.

## Packet Filter

There is a rudimentary packet filter built into both the [tap-slip-gw](/tap-slip-gw) and [esp32-eth-slip-gw](/esp32-eth-slip-gw) applications which serves to try and stop the Dragon from being overloaded with unrequired network traffic. The filter performs the following:

- forwards any ARP packets (these are typically required for any higher level IP protocols to operate) to the Dragon (serial interface)
- drops any non IPv4 packets
  
There is then a configurable option regarding the handling of UDP broadcast packets whereby the filter can operate in one of two modes:

- forward all UDP broadcast packets to the Dragon
- drop all UDP broadcast packets accept for those specifically allowed through via a list of port exceptions

The [tap-slip-gw](/tap-slip-gw) application is set to drop all packets. Given the TAP interface is point to point, unless broadcast packets are specifically injected there is generally no need to let them through. This can of course be changed by modifying the software - see the [packet-filter.h](/tap-slip-gw/packet-filter.h) for details.

The [esp32-eth-slip-gw](/esp32-eth-slip-gw) application is set to drop all packets except for DHCP. This behaviour can be changed via the settings in [esp32-eth-slip-gw/config.h](/esp32-eth-slip-gw/config.h). 
