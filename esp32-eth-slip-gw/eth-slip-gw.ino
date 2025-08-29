// ESP32S3 Dev Module
// for Waveshare ESP32-S3-ETH module
//
// Ethernet SLIP Gateway 

#include <ETH.h>
#include "config.h"
#include "ethernet.h"
#include "packet-filter.h"
#include <atomic>
#ifdef DWIRE_CLIENT
#include <WiFi.h>
#include <WiFiClient.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <freertos/FreeRTOS.h>
#endif

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

#define MAX_SIZE 1500

static bool EthConnected = false ;
// current MAC address of the SLIP client
static char MacAddr[ETHER_ADDR_LEN] ;
static bool MacSet = false ;
// loop counter
static std::atomic_uint32_t Lp = 1u ;

#ifdef DWIRE_CLIENT
static int Sfd = -1 ;
#endif

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

#ifdef FLOW_CONTROL1
  // discard everything till the MAC address of the SLIP client has been set
  // ie. until we've received at least one packet
  if ( !MacSet )
    return ESP_OK ;
#endif

  // apply packet filtering logic
  if ( filter_packet(Buffer,Length))
    return ESP_OK ;

  Serial.printf("E") ;
  Lp++ ;

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
  unsigned long StartTime = millis() ;
  const unsigned long Timeout = 2000u ;

  Serial.printf("S") ;
  Lp++ ;

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
          while ( (!Rc) && ((millis() - StartTime ) < Timeout) )
          {
            Rc = Serial1.read(&Byte,1) ;
            if ( Rc > 0 )
            {
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
      Serial.println("\nError reading serial data for SLIP packet");
      break ;
    }
    if ( (millis() - StartTime ) > Timeout )
    {
      Serial.println("\nTimed out reading SLIP packet");
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
        Serial.printf("\nSPI Ethernet MAC address config failed: %d", Ret);
      else
      {
        Serial.println("\nSuccessfully set MAC address to SLIP client");
        // save copy as current MAC
        memcpy(MacAddr,FrameHdr->ether_shost,ETHER_ADDR_LEN);
        MacSet = true ;
      }
    }
  }

  // transmit frame
  if (EofPacket && EthConnected)
    esp_eth_transmit(ETH.handle(),PktBuf,Len) ;
}

#ifdef DWIRE_CLIENT

static void CloseDWireServer(void)
{
  close(Sfd) ;
  Sfd = -1 ;
  Serial.println("Closed connection to Drivewire server") ;
}

// receive any pending Drivewire serial data and send it to the server
static void SendDWireSerialData(void)
{
  ssize_t BytesIn, BytesOut ;
  uint8_t Buf[1024] ;

  Serial.printf("d") ;
  Lp++ ;

  BytesIn = Serial2.readBytes(Buf,sizeof(Buf)) ;
  
  if ( BytesIn > 0 )
  {
    BytesOut = send(Sfd,Buf,BytesIn,0) ;
    if ( BytesOut < 0 )
    {
      Serial.printf("send: %s\n",strerror(errno));
      CloseDWireServer() ;
    }
    else if ( BytesOut != BytesIn )
    {
      Serial.println("\nFailed to send all data to server");
      CloseDWireServer() ;
    }
  }
}

// receive any pending Drivewire server data and send it to the serial port
static void SendDWireTcpData(void)
{
  ssize_t BytesIn, BytesOut ;
  uint8_t Buf[1024] ;

  Serial.printf("D") ;
  Lp++ ;

  // receive any pending TCP data
  BytesIn = recv(Sfd,Buf,sizeof(Buf),0) ;

  if ( BytesIn < 0 )
  {
    Serial.printf("\nrecv: %s\n",strerror(errno));
    CloseDWireServer() ;
  }
  else
  {
    BytesOut = Serial2.write(Buf,BytesIn) ;
    if ( BytesOut != BytesIn )
      Serial.println("\nError sending Drivewire data to serial");
  }
}

// task to service Drivewire requests
static void DWireServiceTask(void *Arg)
{
  while ( Sfd >= 0 )
  {
    struct timeval Timeout ;

    Timeout.tv_sec = 0 ;
    fd_set FdSet ;

    // check for and forward any serial data to the server
    if ( Serial2.available())
    {
      SendDWireSerialData() ;
      Timeout.tv_usec = 0 ;
    }
    else
      Timeout.tv_usec = 10*1000 ; // 10ms

    // check for and forward any server data to the serial client
    FD_ZERO(&FdSet);
    FD_SET(Sfd,&FdSet) ;
    int Rc = select(Sfd+1,&FdSet,NULL,NULL,&Timeout) ;
    if ( Rc < 0 )
    {
      Serial.printf("select failed: %s\n",strerror(errno)) ;
      CloseDWireServer() ;
    }
    else if ( Rc > 0 )
      SendDWireTcpData() ;

    if ( Lp > 80 )
    {
      Serial.printf("\n");
      Lp = 1u;
    }
  }
  vTaskDelete(NULL);
}

#endif

void setup() 
{
  // setup debug serial
  Serial.begin(115200);
  Serial.println("Starting Ethernet SLIP gateway...");

#ifdef STATUS_LED
  // turn on status led, indicate we're alive
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, HIGH);
#endif

  // initialise serial port used for SLIP
  // increase RX buffer size 
  Serial1.setRxBufferSize(MAX_SIZE) ;

#ifndef FLOW_CONTROL1
  Serial1.begin(19200, SERIAL_8N2, RXD1, TXD1);
#ifdef RTS1
  // if defined, assert RTS to indicate we're always ready to receive
  pinMode(RTS1, OUTPUT);
  digitalWrite(RTS1, LOW);
#endif
#ifdef CTS1
  // if defined, ensure CTS pin is an input
  pinMode(CTS1, INPUT);
#endif
#else
  Serial1.setPins(RXD1,TXD1,CTS1,-1);
  Serial1.begin(19200, SERIAL_8N2) ;
  // Currently, we only use CTS flow control ie. allow the Dragon to request
  // us to stop transmitting. There's no real need to use RTS because we 
  // should always be able to keep up with the Dragon sends. Additionally
  // it's possible to get into a deadlock situation because RTS is normally
  // automatically de-asserted when the ESP UART is half full (64-bytes) which
  // can easily happen if the Dragon is transmitting a large packet, which is also
  // the time when it de-asserts DTR.
  Serial1.setHwFlowCtrlMode(UART_HW_FLOWCTRL_CTS) ;
  // Assert RTS to indicate we're always ready to receive
  pinMode(RTS1, OUTPUT);
  digitalWrite(RTS1, LOW);
#endif
  Serial1.setTimeout(100) ;

  // setup event handler for network events,
  // eseentially notification of ethernet connected/disconnected
  Network.onEvent(onEvent);

  // packet filter
#ifndef FILTER_UDP_BROADCASTS
  enable_udp_broadcast(true) ;
#else
  enable_udp_broadcast(false);
  
  // list of UDP ports to allow through the filter
  const uint16_t UdpUnfilteredPorts[] = UDP_PORT_EXCLUSIONS ;
  uint16_t i = 0 ;

  while ( UdpUnfilteredPorts[i] )
  {
    enable_broadcast_udp_port(UdpUnfilteredPorts[i]) ;
    i++ ;
  }
#endif

#ifdef DWIRE_CLIENT

  IPAddress LocalIp ;

  // connect to Wifi for Drivewire client
  Serial.println("Connecting to WiFi for Drivewire client");
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID1, PWD1);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(F("."));
  }
  LocalIp = WiFi.localIP();
  Serial.println(F("WiFi connected"));
  Serial.println(LocalIp);

  // create the socket used for Drivewire
  Sfd = socket(AF_INET,SOCK_STREAM,0);
  if ( Sfd < 0 )
    Serial.println("Failed to create Drivewire client socket");
  else
  {
    int NoDelay = 1 ;
    struct sockaddr_in DestAddr ;
    TaskHandle_t DWireTaskHandle ;

    if ( setsockopt(Sfd,IPPROTO_TCP,TCP_NODELAY,&NoDelay,sizeof(int)) < 0 )
      Serial.printf("Failed to set TCP_NODELAY on Drivewire client socket: %s\n",strerror(errno));

      memset(&DestAddr,0,sizeof(DestAddr)) ;
      DestAddr.sin_family = AF_INET;
      inet_pton(AF_INET, DWIRE_SERVER_IP, &DestAddr.sin_addr);
      DestAddr.sin_port = htons(DWIRE_TCP_PORT);

      // connect to the server
      if ( connect(Sfd,(struct sockaddr *)&DestAddr, sizeof(DestAddr)) < 0 )
      {
        Serial.printf("Failed to connect to Drivewire server: %s\n",strerror(errno)) ;
        close(Sfd) ;
        Sfd = -1 ;
      }
      else
      {
        Serial.println("Successfully connected to Drivewire server");

        // start the Drivewire serial port
        Serial2.begin(57600, SERIAL_8N1, RXD2, TXD2);
        Serial2.setTimeout(10) ;
#ifdef INVERT_RXD2        
        Serial2.setRxInvert(true);
#endif
        // launch task to service drivewire client
        xTaskCreate(DWireServiceTask,"DriveWireServiceTask",4096,NULL,2,&DWireTaskHandle) ;
        if ( DWireTaskHandle == NULL )
          Serial.println("Failed to create DriveWire service task");
        else
        {
#ifdef STATUS_LED
          // pulse status led on successful dwire connection
          for(uint32_t i=0 ; i < 3 ; i++ )
          {
            digitalWrite(STATUS_LED, LOW);
            delay(400);
            digitalWrite(STATUS_LED, HIGH);
            delay(400);
          }
#endif
        }
      }
  }
  Serial.println("D = Drivewire rx from server/tx to client");
  Serial.println("d = Drivewire rx from client/tx to server");
#endif
  Serial.println("E = Ethernet rx/tx to SLIP");
  Serial.println("S = SLIP rx/tx to Ethernet");
  Serial.println("Ready to service connections");

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
#ifdef STATUS_LED
  static unsigned long LastStatusUpdate = 0 ;
  const unsigned long StatusUpdateInterval = 1000 ;
  unsigned long TimeNow ;

  TimeNow = millis() ;
  // blink the status LED every second to provide indication of alive
  if ( (TimeNow - LastStatusUpdate) > StatusUpdateInterval )
  {
    LastStatusUpdate = TimeNow ;
    if ( digitalRead(STATUS_LED) == LOW )
      digitalWrite(STATUS_LED, HIGH);
    else
      digitalWrite(STATUS_LED, LOW);
  }
#endif

  // poll for serial data on the SLIP interface
  if ( Serial1.available())
    SlipSendEthFrame() ;
  else
    delay(10) ;

  if ( Lp > 80 )
  {
    Serial.printf("\n");
    Lp = 1u;
  }
}

