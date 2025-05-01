# A Dragon IP stack using uIP

This is (currently) a **very** early project to create an IPv4 stack for the old [Dragon 8-bit computer](https://en.wikipedia.org/wiki/Dragon_32/64), if my enthusasm continues, I plan to add more updates to it over the next few months.

I should start by saying that if you're looking to shuffle data from the Internet (eg. download a file or web page) there are far easier ways of accomplishing this. For example you could use the [Drivewire protocol](https://archive.worldofdragon.org/index.php?title=DriveWire), where you offload the actual IP protocol to a more modern/capable machine. There's no real obvious point to doing this other than it seemed an interesting thing to do. Notionally I have this idea that the finished solution will be using something like the [ESP32 Ethernet kit](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-ethernet-kit/user_guide.html) connected to the Dragon via RS232 but we're some way off that right now!

Originally I envisaged writing a very basic IP stack in assembler, really to just support the basic ARP and UDP protocols but then uncovered an archive of the UIP I'd downloaded about 10 years ago which seemed a better starting point (not least because it supports TCP as well).

At present the plan is for this to (eventually) run on a Dragon 64, not specifically because it has more memory than the 32 but that it has a RS232 port (which the 32 lacks). In theory it could run on a 32, using the parallel port as a bitbanger (this is after all what Drivewire does) but that's not something I'm planning on tackling any time soon.

## Current status

At present, the stack builds and runs on a [modified version of the XRoar emulator](https://github.com/fridgemagnet3/xroar) and will respond to ping requests sent via a [Linux TAP device](https://en.wikipedia.org/wiki/TUN/TAP). The basic [hello world](apps/hello-world) application is currently built into code and sort of works but doesn't appear to pick up the response data properly - I've not investigated why just yet.

## How to build/run the stack

The stack is currently built using the [CMOC 6809 cross compiler](http://sarrazip.com/dev/cmoc.html) so you'll need to build/install this first. Then build the uIP stack:

`cd dragon`\
`make`

This results in a ~12K DragonDOS compatible binary file named UIP.BIN

You'll also need [the XRoar emulator](/fridgemagnet3/xroar). Unfortunately the current XRoar releases don't emulate the Dragon 64's serial port, you'll therefore need to build my branch, which additionally only runs under Linux. This does a very basic emulation of the serial port, mapping read/write requests to two device files which are intended to be Linux FIFOs. As such, once you've built the emulator, you'll need to create these in the ~./xroar folder:

`mkdir -p ~/.xroar`\
`mkfifo ~./xroar/tx_uart`\
`mkfifo ~./xroar/rx_uart`

Every character then written to the serial port on the Dragon, will then be sent to the **tx_uart** file. Anything sent to the **rx_uart** file from the Linux side will then appear on the serial port. 

You then need to load the uIP binary image into the emulator. There are various ways of accomplishing this, including writing it to a virtual disk image, for ease of use I use an instance of Drivewire with the [Becker port](https://www.6809.org.uk/xroar/doc/xroar.shtml#Becker-port-options). At present, with the image being ~12Kbytes in size, I currently target this at the 16K address offset which then gives a reasonable bit of room for growth - something like:

`CLEAR 1000,&H4000`\
`DLOAD "UIP.BIN`

That last command loads, then runs the application.

Next you'll need to build (under Linux) the [tap-slip-gw application](/tap-slip-gw):

`cd tap-sli-gw`\
`make`\
`sudo setcap 'cap_net_admin+ep' ./tap-slip-gw`\

That last step allows you to run the application as a normal user (instead of 'root'), if you're not fussed about security you can omit it and run as 'root' anyway. This application brings up a Linux TAP interface, then sends/receives packets to the XRoar serial port FIFOs (and hence to the uIP stack running in the emulator). The uIP binary itself is currently hardcoded to use an IP address of 192.168.3.2, with a gateway address of 192.168.3.1 so assuming that doesn't conflict with your network settings, should then just be a case of running the application as follows:

`./tap-slip-gw 192.168.3.1`

There's normally a brief burst of network traffic which the Dragon takes a little while to digest but after a few seconds it should be able to __ping__ it:

`ping 192.168.3.2`

and get responses back from the Dragon.

![PXL_20250501_164229167](https://github.com/user-attachments/assets/ac6c7621-9615-4385-a421-4a6f26ad9ec3)

