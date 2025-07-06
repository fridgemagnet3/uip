#ifndef GRAPHICS_H

#define GRAPHICS_H

// Graphics 6R
#define MODE6R 0xf0
// Graphics 6C
#define MODE6C 0xe0
// Graphics 3R
#define MODE3R 0xd0
// Graphics 3C
#define MODE3C 0xc0
// Graphics 2R
#define MODE2R 0xB0
// Graphics 2C
#define MODE2C 0xa0
// Graphics 1R
#define MODE1R 0x90
// Graphics 1C 
#define MODE1C 0x80
// Internal alphanumeric
#define ALPHAI 0x00
// External alphanumeric
#define ALPHAE 0x10

// Base address of alpha display
#define ALPHA_BASEADDR 0x400

// set graphics mode - one of MODE definitions above
void gmode(unsigned char mode) ;
// set colour set (ie. SCREEN n,colour_set)
void css(unsigned char colour_set) ;
// set graphics page base address
void pagex(void *page_addr) ;

#endif
