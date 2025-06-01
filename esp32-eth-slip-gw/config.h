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

// set to 1 to filter out UDP broadcast packets
#define FILTER_UDP_BROADCASTS true

#endif
