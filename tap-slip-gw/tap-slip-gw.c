#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/capability.h>
#include <linux/prctl.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pwd.h>
#include <alloca.h>
#include <sys/select.h>
#include <stdint.h>
#include <stdbool.h>

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

/* This creates a TAP interface, brings it up then
   forwards packets to/from an XRoar instance via the
   two FIFOs which emulate the SY6551 on the D64 */

static const uint8_t slip_end = SLIP_END, 
                     slip_esc = SLIP_ESC, 
                     slip_esc_end = SLIP_ESC_END, 
                     slip_esc_esc = SLIP_ESC_ESC;
                           
int open_tap_dev(const char *ip_addr)
{
  struct ifreq ifr;
  cap_t caps;
  const cap_value_t cap_list[1] = { CAP_NET_ADMIN } ;
    
  /* To allow running as non-root user, run 'sudo setcap 'cap_net_admin+ep' ./tap-slip-gw' 
     on the o/p executable each time it is built */
  caps = cap_get_proc() ;
  if ( !caps )
  {
  	perror("cap_get_proc" );
   	return -1 ;
  }
  if (cap_set_flag(caps, CAP_EFFECTIVE, 1, cap_list, CAP_SET) == -1)
  {
    perror("cap_set_flag") ;
    return -1 ;
  }
  if (cap_set_flag(caps, CAP_INHERITABLE, 1, cap_list, CAP_SET) == -1)
  {
    perror("cap_set_flag") ;
    return -1 ;
  }

  if (cap_set_proc(caps) == -1)
  {
    perror("cap_set_proc") ;
    return -1 ;
  }
  if (cap_free(caps) == -1)
  {
  	perror("cap_free") ;
  }
  /* add to ambiant set to allow the permissions to persist across execv calls
     when calling 'system' below to raise the interface */
  if ( prctl(PR_CAP_AMBIENT,PR_CAP_AMBIENT_RAISE, CAP_NET_ADMIN,0,0) == -1 )
  {
  	perror("prctl") ;
  	return -1 ;
  }

  int fd = open("/dev/net/tun", O_RDWR);
  
  if ( fd < 0 )
  {
    perror("Failed to open tap device:" );
    return -1 ;
  }
    
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TAP|IFF_NO_PI;
  if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) 
  {
    perror("ioctl");
    close(fd);
  }

  char buf[256];  
  snprintf(buf, sizeof(buf), "/sbin/ifconfig tap0 inet %s",
	   ip_addr);
  if ( system(buf) )
  {
    perror("tapdev: tapdev_init: system");
    exit(1);
  }

  return fd ; 
}

int open_xroar_uart_fifos(int *tx_fd, int *rx_fd)
{
  long buflen = sysconf(_SC_GETPW_R_SIZE_MAX) ;
  char *buf = alloca(buflen) ;
  struct passwd pwd, *p_pwd ;
  char path[256] ;

  // path to home folder
  if (!getpwuid_r(getuid(), &pwd, buf, buflen, &p_pwd ))
  {
    // open the TX & RX FIFOs (if they exist)
    snprintf(path,sizeof(path), "%s/.xroar/tx_uart", pwd.pw_dir) ;
    *rx_fd = open( path, O_RDWR ) ;
    if ( *rx_fd < 0 )
    {
      perror("xroar tx_uart:" ) ;
      return -1 ;
    }
    snprintf(path,sizeof(path), "%s/.xroar/rx_uart", pwd.pw_dir) ;
    *tx_fd = open( path, O_RDWR ) ;
    if ( *tx_fd < 0 )
    {
      perror("xroar rx_uart:" ) ;
      close(*rx_fd) ;
      return -1 ;
    }
  }
  else
  {
    perror("getpwuid_r") ;
    return -1 ;
  }
}

static int tx_slip_pkt(int fd, const uint8_t *pkt, const int size, useconds_t tx_delay)
{
  int rc ;
  int i ;
  const uint8_t *ptr = pkt ;
  uint8_t c ;
  
  //printf("tx_slip_pkt: %d\n",size) ;

  rc = write(fd,&slip_end,1) ;
  if ( rc < 0 )
  {
    perror("tx_slip_pkt") ;
    return -1 ;
  }
  for(i=0 ; i < size ; i++ )
  {
    c = *ptr++ ;
    switch(c) 
    {
      case SLIP_END:
        rc = write(fd,&slip_esc,1);
        usleep(tx_delay) ;
        if ( rc > 0 )
        {
          rc = write(fd,&slip_esc_end,1);
          usleep(tx_delay) ;
        }
        break;
      case SLIP_ESC:
        rc = write(fd,&slip_esc,1) ;
        usleep(tx_delay) ;
        if ( rc > 0 )
        {
          rc = write(fd,&slip_esc_esc,1);
          usleep(tx_delay) ;
        }
        break;
      default:
        rc = write(fd,&c,1) ;
        usleep(tx_delay) ;
        break;
    }
    
    if ( rc < 0 )
    {
      perror("tx_slip_pkt");
      return -1 ;
    }
  }
  rc = write(fd,&slip_end,1) ;
  usleep(tx_delay) ;
  if ( rc < 0 )
  {
    perror("tx_slip_pkt") ;
    return -1 ;
  }
  return size ;
}

static int rx_slip_pkt(int fd, uint8_t *pktbuf, const int bufsz)
{
  int len = 0 ;
  uint8_t c ;
  int rc  ; 
  uint8_t *ptr = pktbuf ;
  bool eof_pkt = false ;
  
  //printf("rx_slip_pkt\n" ) ;
  while(!eof_pkt)
  {
    if ( len == bufsz )
    {
      len = 0 ;
      ptr = pktbuf ;
    }
    rc = read(fd,&c,1) ;
    if ( rc > 0 )
    {
      switch(c)
      {
        case SLIP_END :
        
          if ( len )
      	    eof_pkt = true ;
     	  break ;
      	  
        case SLIP_ESC :
        
          rc = read(fd,&c,1) ;
          if ( rc > 0 )
          {
            switch(c) 
            {
              case SLIP_ESC_END:
                c = SLIP_END;
                break;
              case SLIP_ESC_ESC:
                c = SLIP_ESC;
                break ;
            }
          }
                
        default:
        
          *ptr++ = c ;
          len++ ;
          break ;
      }
    }

    if ( rc < 0 )
    {
      perror("rx_slip_pkt") ;
      len = -1 ;
      break ;
    }
  }

  return len ;
}

int main(int argc, char *argv[] )
{
  useconds_t uart_delay = 0 ;

  if ( argc < 2 )
  {
    printf("Usage: tap-slip-gw <host-ip> {tx-delay[us]} {comms-dev}\n" ) ;
    return -1 ;
  }
  int tap_fd = open_tap_dev(argv[1]) ;
  if ( tap_fd < 0 )
    return -1 ;

  if ( argc > 2 )
  {
    // When testing the serial driver on the Dragon side, 
    // sending all the data as fast as possible isn't representative 
    // so this allows the injection of a short delay between each character.
    // Using a figure of ~1000 gives a latency similar to that seen on
    // the real hardware running at 19200 baud
    uart_delay = strtoul(argv[2],NULL,10) ;
  }
      
  int xroar_tx_fifo, xroar_rx_fifo ;
  if ( argc < 4 )
  {
    if ( open_xroar_uart_fifos ( &xroar_tx_fifo, &xroar_rx_fifo) < 0 )
    {
      close(tap_fd) ;
      return -1 ;
    }
  }
  else
  {
    // allow a specific device to be used (eg. serial) instead of the Xroar
    // FIFOs ie. for communicating with real hardware
    xroar_tx_fifo = open(argv[3], O_RDWR ) ;
    if ( xroar_tx_fifo < 0 )
    {
      perror("device open" ) ;
      close(tap_fd);
      return -1 ;
    }
    xroar_rx_fifo = xroar_tx_fifo ;
  }
  
  fd_set fdset;
  int max_fd ;
  uint8_t pktbuf[1500];
  int rc ;
  
  printf("Starting..\n") ;

  if ( tap_fd > xroar_rx_fifo )
    max_fd = tap_fd ;
  else
    max_fd = xroar_rx_fifo ;
    
  while(1)
  {
    FD_ZERO(&fdset);
    FD_SET(tap_fd, &fdset);
    FD_SET(xroar_rx_fifo, &fdset);
    rc = select(max_fd+1,&fdset, NULL, NULL, NULL) ;
    if ( rc < 0 )
    {
      perror("select") ;
      break ;
    }
    else
    {
      if ( FD_ISSET(tap_fd,&fdset) )
      {
        // data has been received on the TAP device
        rc = read(tap_fd, pktbuf, sizeof(pktbuf));
        if ( rc < 0 )
          perror("TAP read") ;
        else
        {
          // send it to the xroar fifo
          rc = tx_slip_pkt(xroar_tx_fifo,pktbuf,rc,uart_delay) ;
        }
      }
      if ( FD_ISSET(xroar_rx_fifo,&fdset) )
      {
        // data has been received on the xroar fifo
        rc = rx_slip_pkt(xroar_rx_fifo, pktbuf, sizeof(pktbuf));
        if ( rc > 0 )
        {
          // send it to the tap device
          rc = write(tap_fd,pktbuf,rc) ;
          if ( rc < 0 )
            perror("TAP write") ;
        }
      }
    }
  }
  
  close(tap_fd) ;
  close(xroar_tx_fifo) ;
  close(xroar_rx_fifo) ;
  return 0 ;
}

