#ifndef CONFIG_H

#define CONFIG_H

// Ethernet configuration for the Waveshare ESP32-S3-ETH module
#define ETH_PHY_TYPE     ETH_PHY_W5500
#define ETH_PHY_ADDR     1
#define ETH_PHY_CS       14
#define ETH_PHY_IRQ      10
#define ETH_PHY_RST      9
#define ETH_PHY_SPI_HOST SPI3_HOST
#define ETH_PHY_SPI_SCK  13
#define ETH_PHY_SPI_MISO 12
#define ETH_PHY_SPI_MOSI 11

// serial GPIO pins
#define RXD1 15
#define TXD1 16

// define to enable hardware flow control
//#define FLOW_CONTROL1

// used if FLOW_CONTROL1 is defined but (if defined) also configured
// for "normal" operation in 3-wire serial mode on the premise they
// may still be wired up but not used by software...
#define RTS1 17
#define CTS1 18

// GPIO used for status LED
#define STATUS_LED 48

// define to filter UDP packets
#define FILTER_UDP_BROADCASTS 

// list of broadcast UDP ports excluded from filtering
// only applicable if FILTER_UDP_BROADCASTS is defined
// terminate list with a zero
// 68 = DHCP
// 52003 = weather data
// 52005 = solar data
//#define UDP_PORT_EXCLUSIONS { 68, 52003, 52005, 0 }

#define UDP_PORT_EXCLUSIONS { 68, 0 }

// define to operate as a Drivewire client over Wifi
#define DWIRE_CLIENT

// Drivewire client settings
#ifdef DWIRE_CLIENT

// Drivewire server TCP Port, default is 65504
#ifndef DWIRE_TCP_PORT 
#define DWIRE_TCP_PORT 65504
#endif

// Drivewire server IP
#define DWIRE_SERVER_IP "192.168.0.201"

// wifi settings
#define SSID1 "your-wifi-ssid"
#define PWD1 "your-wifi-password"

// serial GPIO pins for Drivewire client
#define RXD2 46
#define TXD2 45

// define to invert the RX line which negates the need to implement a hardware inverter on the Dragon side
//#define INVERT_RXD2

#endif

#endif
