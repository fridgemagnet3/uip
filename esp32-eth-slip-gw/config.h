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

// serial GPIO bins
#define RXD1 15
#define TXD1 16

// define to enable hardware flow control
//#define FLOW_CONTROL1

// hardware flow control pins
// only used if FLOW_CONTROL1 is defined
#define RTS1 17
#define CTS1 21

// define to filter UDP packets
#define FILTER_UDP_BROADCASTS 

// list of broadcast UDP ports excluded from filtering
// only applicable if FILTER_UDP_BROADCASTS is defined
// terminate list with a zero
// 82 = DHCP
#define UDP_PORT_EXCLUSIONS { 68, 0 }

#endif
