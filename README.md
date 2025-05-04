# A Dragon IP stack using uIP

This is (currently) a **very** early project to create an IPv4 stack for the old [Dragon 8-bit computer](https://en.wikipedia.org/wiki/Dragon_32/64), if my enthusasm continues, I plan to add more updates to it over the next few months.

I should start by saying that if you're looking to shuffle data from the Internet (eg. download a file or web page) there are far easier ways of accomplishing this. For example you could use the [Drivewire protocol](https://archive.worldofdragon.org/index.php?title=DriveWire), where you offload the actual IP protocol to a more modern/capable machine. There's no real obvious point to doing this other than it seemed an interesting (mad?) thing to do. Notionally I have this idea that the finished solution will be using something like the [ESP32 Ethernet kit](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-ethernet-kit/user_guide.html) connected to the Dragon via RS232 but we're some way off that right now!

Originally I envisaged writing a very basic IP stack in assembler, really to just support the basic ARP and UDP protocols but then uncovered an archive of the UIP I'd downloaded about 10 years ago which seemed a better starting point (not least because it supports TCP as well).

At present the plan is for this to (eventually) run on a Dragon 64, not specifically because it has more memory than the 32 but that it has a RS232 port (which the 32 lacks). In theory it could run on a 32, using the parallel port as a bitbanger (this is after all what Drivewire does) but that's not something I'm planning on tackling any time soon.

## Current status

At present, the stack builds and runs on a [modified version of the XRoar emulator](https://github.com/fridgemagnet3/xroar) and will respond to ping requests sent via a [Linux TAP device](https://en.wikipedia.org/wiki/TUN/TAP). The current configuration includes the telnet server application which can be successfully commuincated with, it doesn't do much mind aside from displaying the 'help' menu although I think there is more down to the other features not having been implemented rather than broken functionality. It does however serve to prove the TCP/IP protocol works. 

`telnet 192.168.3.2`\
`Trying 192.168.3.2...`\
`Connected to 192.168.3.2.`\
`Escape character is '^]'.`\
`uIP command shell`\
`Type '?' and return for help`\
`uIP 1.0>`


It also includes a simple UDP app which listens on port 52005 and will display any textual data received. Coincidently, this happens to be the same port I use for [broadcasting my solar (JSON) data](https://github.com/fridgemagnet3/modbus-solis5g) but should work with any packets containing text.

![solar-metrics](https://github.com/user-attachments/assets/acb3cec9-3c8f-4de2-9dce-c40da9b3ca4b)

Any application which uses the [protosockets library](doc/html/a00158.html) (including the simple [hello world](apps/hello-world) example) **won't work properly.** This is because the underlying [protothreads library](doc/html/a00142.html) makes a whacky use of the select() call that is similar to something called the [Duff's device](https://en.wikipedia.org/wiki/Duff%27s_device) which the current incarnation of the CMOC (6809 cross) compiler specifically states it does not support. In a nutshell, the state machine used to track the TCP connection state gets repeatedly reset & confusion then rains.

## How to build/run the stack

The stack is currently built using the [CMOC 6809 cross compiler](http://sarrazip.com/dev/cmoc.html) so you'll need to build/install this first. Then build the uIP stack:

`cd dragon`\
`make`

This results in a ~15K DragonDOS compatible binary file named UIP.BIN

You'll also need [the XRoar emulator](https://github.com/fridgemagnet3/xroar). Unfortunately the current XRoar releases don't emulate the Dragon 64's serial port, you'll therefore need to build my branch, which additionally only runs under Linux. This does a very basic emulation of the serial port, mapping read/write requests to two device files which are intended to be Linux FIFOs. As such, once you've built the emulator, you'll need to create these in the ~./xroar folder:

`mkdir -p ~/.xroar`\
`mkfifo ~./xroar/tx_uart`\
`mkfifo ~./xroar/rx_uart`

Every character then written to the serial port on the Dragon, will then be sent to the **tx_uart** file. Anything sent to the **rx_uart** file from the Linux side will then appear on the serial port. 

You then need to load the uIP binary image into the emulator. There are various ways of accomplishing this, including writing it to a virtual disk image, for ease of use I use an instance of Drivewire with the [Becker port](https://www.6809.org.uk/xroar/doc/xroar.shtml#Becker-port-options). At present, with the image being ~15Kbytes in size, I currently target this at the 12K address offset which then gives a reasonable bit of room for growth - something like:

`CLEAR 1000,&H3000`\
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

## Will this work on the real hardware?

It might but I'm skeptical.... At present, the serial port handling on the Dragon side is a very basic, polled implementation & I strongly suspect that unless you slow the baud rate down, it's liable to drop characters. Once I've done some more work proving the IP protocol handling is all working nicely, one of my jobs will be to replace this with an interrupt driven solution, till then I'm not planning on going near real hardware.

Aside from that issue though, the transition should be relatively straightforward. You'll obviously need to make up a suitable serial cable but then it should just be a case of changing the [tap-slip-gw application](/tap-slip-gw) code to send/receive from the one file descriptor (connected to the serial port) rather than the two used with the FIFOs.
