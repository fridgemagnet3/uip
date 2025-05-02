/* Architecture specific operations */

#include "uip.h"

// 32-bit addition
void uip_add32(u8_t *op32, u16_t op16)
{
  asm
  {
    pshs y
    ; fetch value to add
    ldd :op16
    ldx :op32
    leay :uip_acc32
    ; add the low half
    addd 2,x
    ; store it
    std 2,y
    ; fetch the high half
    ldd 0,x
    ; add 0, carry to B
    adcb #0
    ; add 0, carry to A
    adca #0
    ; update high half
    std 0,y
    puls y
  }
}


