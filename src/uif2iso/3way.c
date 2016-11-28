/*
    Copyright (C) 2008,2009 Luigi Auriemma

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    http://www.gnu.org/licenses/gpl-2.0.txt
*/
// modified by Luigi Auriemma for emulating the shameful customizations of MagicISO
// note: the original file had no copyright or license

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>



#define   STRT_E   0x0b0b
#define   STRT_D   0xb1b1
#define     NMBR       11



void xtheta(uint32_t *a) {
    uint32_t b[3];

    b[0] = a[0] ^ (a[0]>>16) ^ (a[1]<<16) ^ (a[1]>>16) ^ (a[2]<<16) ^
                  (a[1]>>24) ^ (a[2]<<8)  ^ (a[2]>>8)  ^ (a[0]<<24) ^
                  (a[2]>>16) ^ (a[0]<<16) ^ (a[2]>>24) ^ (a[0]<<8);
    b[1] = a[1] ^ (a[1]>>16) ^ (a[2]<<16) ^ (a[2]>>16) ^ (a[0]<<16) ^
                  (a[2]>>24) ^ (a[0]<<8)  ^ (a[0]>>8)  ^ (a[1]<<24) ^
                  (a[0]>>16) ^ (a[1]<<16) ^ (a[0]>>24) ^ (a[1]<<8);
    b[2] = a[2] ^ (a[2]>>16) ^ (a[0]<<16) ^ (a[0]>>16) ^ (a[1]<<16) ^
                  (a[0]>>24) ^ (a[1]<<8)  ^ (a[1]>>8)  ^ (a[2]<<24) ^
                  (a[1]>>16) ^ (a[2]<<16) ^ (a[1]>>24) ^ (a[2]<<8);
    a[0] = b[0];
    a[1] = b[1];
    a[2] = b[2];
}



void xpi1(uint32_t *p) {
    p[0] = (p[0] >> 0x0a) ^ (p[0] << 0x16);
    p[2] = (p[2] >> 0x1f) ^ (p[2] << 1);
}



void xgamma(uint32_t *p) {
    uint32_t     tmp[3];

    tmp[0] = p[0];
    tmp[1] = p[1];
    tmp[2] = p[2];
    p[0] ^= (~tmp[2]) | tmp[1];
    p[1] ^= (~tmp[0]) | tmp[2];
    p[2] ^= (~tmp[1]) | tmp[0];
}



void xpi2(uint32_t *p) {
    p[2] = (p[2] >> 0x0a) ^ (p[2] << 0x16);
    p[0] = (p[0] >> 0x1f) ^ (p[0] << 1);
}



void xrho(uint32_t *p) {
    xtheta(p);
    xpi1(p);
    xgamma(p);
    xpi2(p);
}



void xmu(uint32_t *p) {
    uint32_t     bswap[3];
    int     i;

    bswap[0] = 0;
    bswap[1] = 0;
    bswap[2] = 0;
    for(i = 0; i < 32; i++) {
        bswap[0] <<= 1;
        bswap[1] <<= 1;
        bswap[2] <<= 1;
        if(p[0] & 1) bswap[2] |= 1;
        if(p[1] & 1) bswap[1] |= 1;
        if(p[2] & 1) bswap[0] |= 1;
        p[0] >>= 1;
        p[1] >>= 1;
        p[2] >>= 1;
    }
    p[0] = bswap[0];
    p[1] = bswap[1];
    p[2] = bswap[2];
}



void xrndcon_gen(uint32_t strt,uint32_t *rtab) {
    int     i;

    for(i = 0; i <= NMBR; i++) {
        rtab[i] = strt;
        strt <<= 1;
        if(strt & 0x10000) strt ^= 0x11011;
    }
}



void x3way(uint8_t *data, int datalen, uint32_t *key) {
    uint32_t     rcon[NMBR+1],
            *p;
    int     i;

    xtheta(key);
    xmu(key);
    xrndcon_gen(STRT_D, rcon);

    datalen /= 12;
    for(p = (uint32_t *)data; datalen--; p += 3) {
        p[2] ^= p[0];
        p[0] ^= p[1];
        p[1] ^= 0x7483a3fd;

        xmu(p);
        for(i = 0; i < 11; i++) {
            p[0] ^= key[0] ^ (rcon[i] << 16);
            p[1] ^= key[1];
            p[2] ^= key[2] ^ rcon[i];
            xrho(p);
        }
        p[0] ^= key[0] ^ (rcon[NMBR] << 16);
        p[1] ^= key[1];
        p[2] ^= key[2] ^ rcon[NMBR];
        xtheta(p);
        xmu(p);
    }
}

