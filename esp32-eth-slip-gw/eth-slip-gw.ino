// ESP32S3 Dev Module
// for Waveshare ESP32-S3-ETH module
//
// Ethernet SLIP Gateway 

#include <ETH.h>
#include "config.h"
#include "ethernet.h"
#include "packet-filter.h"

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

#define MAX_SIZE 1500

static bool EthConnected = false ;
// current MAC address of the SLIP client
static char MacAddr[ETHER_ADDR_LEN] ;

static esp_err_t EthRxFrameCBack(esp_eth_handle_t Handle, uint8_t *Buffer, uint32_t Length, void *Private) ;

void onEvent(arduino_event_id_t Event, arduino_event_info_t Info) {
  switch (Event) 
  {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
      ETH.setHostname("esp32-eth0");
      break;

    case ARDUINO_EVENT_ETH_CONNECTED: 
      Serial.println("ETH Connected"); 
      EthConnected = true;
      break;

    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      EthConnected = false;
      break;

    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      EthConnected = false;
      break;

    default: break;
  }
}

// callback invoked on receipt of in incoming Ethernet frame
static esp_err_t EthRxFrameCBack(esp_eth_handle_t Handle, uint8_t *Buffer, uint32_t Length, void *Private)
{
  uint8_t *Ptr = (uint8_t*)Buffer ;
  uint32_t i ;
  uint8_t Byte ;

  Serial.println("EthRxFrameCBack");

  // apply packet filtering logic
  if ( filter_packet(Buffer,Length))
    return ESP_OK ;

  // transfer the frame as a SLIP packet
  Serial1.write(SLIP_END) ;

  for (i=0 ; i < Length ; i++)
  {
    Byte = *Ptr++ ;
    switch(Byte)
    {
      case SLIP_END:
        Serial1.write(SLIP_ESC);
        Serial1.write(SLIP_ESC_END);
        break ;
      case SLIP_ESC:
        Serial1.write(SLIP_ESC);
        Serial1.write(SLIP_ESC_ESC);
        break ;
      default:
        Serial1.write(Byte) ;
    }
  }

  Serial1.write(SLIP_END) ;

  return ESP_OK ;
}

// Receive a SLIP packet and transmit it over Ethernet
static void SlipSendEthFrame(void)
{
  uint32_t Len = 0 ;
  bool EofPacket = false ;
  uint8_t PktBuf[MAX_SIZE] ;
  uint8_t *Ptr = PktBuf ;
  size_t Rc ;
  uint8_t Byte ;

  Serial.println("SlipSendEthFrame");

  // receive a SLIP packet
  while(!EofPacket)
  {
    if ( Len == MAX_SIZE )
    {
      Len = 0 ;
      Ptr = PktBuf ;
    }
    Rc = Serial1.read(&Byte,1) ;
    if ( Rc > 0 )
    {
      switch(Byte)
      {
        case SLIP_END :
        
          if ( Len )
      	    EofPacket = true ;
     	    break ;
      	  
        case SLIP_ESC :
        
          Rc = 0 ;
          while ( !Rc )
          {
            Rc = Serial1.read(&Byte,1) ;
            switch(Byte) 
            {
              case SLIP_ESC_END:
                Byte = SLIP_END;
                break;
              case SLIP_ESC_ESC:
                Byte = SLIP_ESC;
                break ;
            }
          }
          if ( Rc < 0 )
            break ;
                
        default:
        
          *Ptr++ = Byte ;
          Len++ ;
          break ;
      }
    }
    if ( Rc < 0 )
    {
      Serial.println("Timed out waiting for incoming SLIP packet");
      break ;
    }
  }

  if (EofPacket)
  {
    struct ether_header *FrameHdr = (struct ether_header*)PktBuf ;
    int Ret ;

    // the MAC on the ESP will filter out packets that don't match it's assigned address
    // (which is set by the ETHClass object to that fused into the device) so we need
    // to set it to match that of the SLIP client.

    // check the header of the incoming packet against our current MAC
    if ( memcmp(FrameHdr->ether_shost,MacAddr,ETHER_ADDR_LEN))
    {
      // address differs so update the driver
      Ret = esp_eth_ioctl(ETH.handle(), ETH_CMD_S_MAC_ADDR, FrameHdr->ether_shost);
      if (Ret != ESP_OK) 
        Serial.printf("SPI Ethernet MAC address config failed: %d", Ret);
      else
      {
        Serial.println("Successfully set MAC address to SLIP client");
        // save copy as current MAC
        memcpy(MacAddr,FrameHdr->ether_shost,ETHER_ADDR_LEN);
      }
    }
  }

  // transmit frame
  if (EofPacket && EthConnected)
    esp_eth_transmit(ETH.handle(),PktBuf,Len) ;
}

void setup() 
{
  // setup debug serial
  Serial.begin(115200);
  Serial.println("Starting...");

  // initialise serial port used for SLIP
  // increase RX buffer size 
  Serial1.setRxBufferSize(MAX_SIZE) ;
  // RX1 = GPIO15, TX1 = GPIO16
  Serial1.begin(19200, SERIAL_8N2, RX1, TX1);
  Serial1.setTimeout(100) ;

  // setup event handler for network events,
  // eseentially notification of ethernet connected/disconnected
  Network.onEvent(onEvent);

  // packet filter
  enable_udp_broadcast(FILTER_UDP_BROADCASTS);
  
  // configure and start the Ethernet interface
  ETH.begin(ETH_PHY_TYPE, 
            ETH_PHY_ADDR, 
            ETH_PHY_CS, 
            ETH_PHY_IRQ, 
            ETH_PHY_RST, 
            ETH_PHY_SPI_HOST, 
            ETH_PHY_SPI_SCK, 
            ETH_PHY_SPI_MISO, 
            ETH_PHY_SPI_MOSI);

  // callback invoked on receipt of an Ethernet frame
  esp_eth_update_input_path(ETH.handle(),EthRxFrameCBack,NULL) ;
}

void loop() 
{
  if ( Serial1.available())
    SlipSendEthFrame() ;
  else
    delay(10) ;
}

