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
// Code modified by Luigi Auriemma. for emulating the shameful customizations of MagicISO
// Code derived from the reference implementation of J.S.A.Kapp with deep modifications.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

typedef uint32_t word32;

	/* rotate functions */
#define ROTL(x,s) ((x)<<(s) | (x)>>(32-(s)))
#define ROTR(x,s) ((x)>>(s) | (x)<<(32-(s)))

	/* Magic Contants */
#define P 0xb7e15163
#define Q 0x9e3779b9

#define ANDVAL 31

#define KR(x) (2*((x)+1))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

void RC5decrypt(word32 *in, int len, int b, word32 *key) {
	int i;
    int t;

    for(t = b << 2; len; len--) {
        key += t + 1;
        for (i = 0; i < t; i += 2) {
            in[1] -= *key--;
            in[1] = ROTR(in[1], in[0]&ANDVAL) ^ in[0];
            in[0] -= *key--;
            in[0] ^= 0x63a72efb;
            in[0] = ROTR(in[0], in[1]&ANDVAL) ^ in[1];
        }
        in[1] -= *key--;
        in[0] -= *key;
        in += 2;
    }
}

void RC5key(unsigned char *key, int b, word32 **ctx) {
    int t = (b << 2) + 2;
	int i;
	int x = ((b-1)/sizeof(word32))+1;
	int mix = 3 * (MAX(t,x));
	word32 *L, *ky;
	word32 A = 0, B = 0;
    unsigned char *Lcmp;

	L = (word32 *) calloc((x+1), sizeof(word32));
	memcpy((unsigned char *)L, key, b);

    if(b & 3) { // endian compatibility
        Lcmp = (unsigned char *)(L + (b >> 2));
        L[b >> 2] = (Lcmp[3] << 24) | (Lcmp[2] << 16) | (Lcmp[1] << 8) | Lcmp[0];
    }

	ky = (word32 *) calloc((t+1), sizeof(word32));
	*ky = P;
		/* Prep subkeys with magic Constants */
	for(i = 1; i < t; i++)
		ky[i] = ky[i-1] + Q;

	for(i = 0; i < mix; i++) {
		A = ky[i%t] = ROTL((ky[i%t] + A + B), 3&ANDVAL);
		B = L[i%x] = ROTL((L[i%x] + A + B), ((A + B)&ANDVAL));
	}
    free(L);
    *ctx = ky;
}
