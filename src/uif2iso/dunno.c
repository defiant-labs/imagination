/*
    Copyright (C) 2007,2008,2009 Luigi Auriemma

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

#include <stdint.h>



static const uint8_t dunno_sbox[64] = {
    0xa6,0xac,0xdb,0xb7,0xed,0x96,0x45,0x99,0x47,0xf7,0xe2,0x16,0xd8,0x69,0xa3,0x7e,
    0x8f,0x58,0x58,0xee,0x1d,0xb5,0x39,0x13,0x23,0xf0,0x18,0xef,0xb0,0x0e,0x8b,0x3e,
    0xc1,0x27,0xda,0x60,0xf3,0x94,0x62,0x40,0x6a,0xb6,0x34,0xce,0xaf,0x93,0x11,0x2a,
    0x5d,0xf6,0x16,0x1e,0x33,0x5c,0x81,0x77,0x98,0xaf,0x1b,0x93,0xcc,0x91,0x60,0x32 };



void dunno_key(uint32_t *dunnoctx, signed char *key) {
    dunnoctx[0] = (key[0] << 11) | (key[1] << 3) | (key[2] >> 5);
    dunnoctx[1] = (key[2] << 17) | (key[3] << 9) | (key[4] << 1) | (key[5] >> 7);
    dunnoctx[2] = (key[5] << 15) | (key[6] << 8) | key[7];
    dunnoctx[3] = dunnoctx[0];
    dunnoctx[4] = dunnoctx[1];
    dunnoctx[5] = dunnoctx[2];
}



int dunno_decy(uint32_t x1, uint32_t x2, uint32_t x3) {
    if((((x1 >> 9) & 1) + ((x2 >> 11) & 1) + ((x3 >> 11) & 1)) <= 1) return(1);
    return(0);
}



uint32_t dunno_dec0(uint32_t x1, uint32_t x2) {
    uint32_t    x;

    if(x2 ^ ((x1 >> 9) & 1)) {
        x = ((x1 >> 0x11) ^ (x1 >> 0x12) ^ (x1 >> 0x10) ^ (x1 >> 0x0d));
        x1 = (x1 << 1) & 0x7ffff;
        if(x & 1) return(x1 ^ 1);
    }
    return(x1);
}



uint32_t dunno_dec1(uint32_t x1, uint32_t x2) {
    uint32_t    x;

    if(x2 ^ ((x1 >> 11) & 1)) {
        x = ((x1 >> 0x14) ^ (x1 >> 0x15) ^ (x1 >> 0x10) ^ (x1 >> 0x0c));
        x1 = (x1 << 1) & 0x3fffff;
        if(x & 1) return(x1 ^ 1);
    }
    return(x1);
}



uint32_t dunno_dec2(uint32_t x1, uint32_t x2) {
    uint32_t    x;

    if(x2 ^ ((x1 >> 11) & 1)) {
        x = ((x1 >> 0x15) ^ (x1 >> 0x16) ^ (x1 >> 0x12) ^ (x1 >> 0x11));
        x1 = (x1 << 1) & 0x7fffff;
        if(x & 1) return(x1 ^ 1);
    }
    return(x1);
}



uint32_t dunno_decx(uint32_t *dunnoctx) {
    uint32_t    x;

    x = dunno_decy(dunnoctx[0], dunnoctx[1], dunnoctx[2]);
    dunnoctx[0] = dunno_dec0(dunnoctx[0], x);
    dunnoctx[1] = dunno_dec1(dunnoctx[1], x);
    dunnoctx[2] = dunno_dec2(dunnoctx[2], x);
    return((dunnoctx[0] ^ dunnoctx[1] ^ dunnoctx[2]) & 1);
}



void dunno_dec(uint32_t *dunnoctx, uint8_t *data, int datalen) {
    int     i,
            j;
    uint8_t c;

    dunnoctx[0] = dunnoctx[3];
    dunnoctx[1] = dunnoctx[4];
    dunnoctx[2] = dunnoctx[5];

    c = 0;
    for(i = 0; i < datalen; i++) {
        for(j = 0; j < 8; j++) {
            c = dunno_decx(dunnoctx) | (c << 1);
        }
        data[i] ^= c ^ dunno_sbox[i & 0x3f];
    }
}


