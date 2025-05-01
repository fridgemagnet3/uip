#ifndef SLIPDEV_H

#define SLIPDEV_H

#include "uip.h"

void slipdev_init(void) ;

unsigned int slipdev_read(void) ;

void slipdev_send(void) ;

#endif
