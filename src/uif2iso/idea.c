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
/* idea.c  -  IDEA function
 *	Copyright (c) 1997,1998,1999 by Werner Koch (dd9jn)
 ************************************************************************
 * ATTENTION: The used algorithm is patented and may need a license
	      for any use.  See below for more information.
 ************************************************************************
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * WERNER KOCH BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Werner Koch shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Werner Koch.
 */

/*--------------------------------------------------------------
 The code herein has been taken from:
   Bruce Schneier: Applied Cryptography. John Wiley & Sons, 1996.
   ISBN 0-471-11709-9. .

 The IDEA algorithm is patented by Ascom Systec Ltd. of CH-5506 Maegenwil,
 Switzerland, who allow it to be used on a royalty-free basis for certain
 non-profit applications.  Commercial users must obtain a license from the
 company in order to use IDEA.	IDEA may be used on a royalty-free basis under
 the following conditions:

 Free use for private purposes:

 The free use of software containing the algorithm is strictly limited to non
 revenue generating data transfer between private individuals, ie not serving
 commercial purposes.  Requests by freeware developers to obtain a
 royalty-free license to spread an application program containing the
 algorithm for non-commercial purposes must be directed to Ascom.

 Special offer for shareware developers:

 There is a special waiver for shareware developers.  Such waiver eliminates
 the upfront fees as well as royalties for the first US$10,000 gross sales of
 a product containing the algorithm if and only if:

 1. The product is being sold for a minimum of US$10 and a maximum of US$50.
 2. The source code for the shareware is available to the public.

 Special conditions for research projects:

 The use of the algorithm in research projects is free provided that it serves
 the purpose of such project and within the project duration.  Any use of the
 algorithm after the termination of a project including activities resulting
 from a project and for purposes not directly related to the project requires
 a license.

 Ascom Tech requires the following notice to be included for freeware
 products:

 This software product contains the IDEA algorithm as described and claimed in
 US patent 5,214,703, EPO patent 0482154 (covering Austria, France, Germany,
 Italy, the Netherlands, Spain, Sweden, Switzerland, and the UK), and Japanese
 patent application 508119/1991, "Device for the conversion of a digital block
 and use of same" (hereinafter referred to as "the algorithm").  Any use of
 the algorithm for commercial purposes is thus subject to a license from Ascom
 Systec Ltd. of CH-5506 Maegenwil (Switzerland), being the patentee and sole
 owner of all rights, including the trademark IDEA.

 Commercial purposes shall mean any revenue generating purpose including but
 not limited to:

 i) Using the algorithm for company internal purposes (subject to a site
    license).

 ii) Incorporating the algorithm into any software and distributing such
     software and/or providing services relating thereto to others (subject to
     a product license).

 iii) Using a product containing the algorithm not covered by an IDEA license
      (subject to an end user license).

 All such end user license agreements are available exclusively from Ascom
 Systec Ltd and may be requested via the WWW at http://www.ascom.ch/systec or
 by email to idea@ascom.ch.

 Use other than for commercial purposes is strictly limited to non-revenue
 generating data transfer between private individuals.	The use by government
 agencies, non-profit organizations, etc is considered as use for commercial
 purposes but may be subject to special conditions.  Any misuse will be
 prosecuted.
-------------------------------------------------------------------*/

/* How to compile:
 *
       gcc -Wall -O2 -shared -fPIC -o idea idea.c
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* configuration stuff */
#ifdef __alpha__
  #define SIZEOF_UNSIGNED_LONG 8
#else
  #define SIZEOF_UNSIGNED_LONG 4
#endif



typedef unsigned char  byte;
typedef uint16_t u16;
typedef uint32_t  u32;

/* end configurable stuff */

#ifndef DIM
  #define DIM(v) (sizeof(v)/sizeof((v)[0]))
  #define DIMof(type,member)   DIM(((type *)0)->member)
#endif

/* imports */
void g10_log_fatal( const char *fmt, ... );


/* local stuff */

#define FNCCAST_SETKEY(f)  ((int(*)(void*, byte*, unsigned))(f))
#define FNCCAST_CRYPT(f)   ((void(*)(void*, byte*, byte*))(f))

#define IDEA_KEYSIZE 16
#define IDEA_BLOCKSIZE 8
#define IDEA_ROUNDS 8
#define IDEA_KEYLEN (6*IDEA_ROUNDS+4)

typedef struct {
    u16 ek[IDEA_KEYLEN];
    u16 dk[IDEA_KEYLEN];
    int have_dk;
} IDEA_context;


int do_setkey( IDEA_context *c, byte *key, unsigned keylen );
void encrypt_block( IDEA_context *bc, byte *outbuf, byte *inbuf );
void decrypt_block( IDEA_context *bc, byte *outbuf, byte *inbuf );



u16
mul_inv( u16 x )
{
    u16 t0, t1;
    u16 q, y;

    if( x < 2 )
	return x;
    t1 = 0x10001L / x;
    y =  0x10001L % x;
    if( y == 1 )
	return (1-t1) & 0xffff;

    t0 = 1;
    do {
	q = x / y;
	x = x % y;
	t0 += q * t1;
	if( x == 1 )
	    return t0;
	q = y / x;
	y = y % x;
	t1 += q * t0;
    } while( y != 1 );
    return (1-t1) & 0xffff;
}



void
expand_key( byte *userkey, u16 *ek )
{
    int i,j;

    for(j=0; j < 8; j++ ) {
	ek[j] = (*userkey << 8) + userkey[1];
	userkey += 2;
    }
    for(i=0; j < IDEA_KEYLEN; j++ ) {
	i++;
	ek[i+7] = ek[i&7] << 9 | ek[(i+1)&7] >> 7;
    ek[i+7] ^= 0x28fc;
	ek += i & 8;
	i &= 7;
    }
}


void
invert_key( u16 *ek, u16 dk[IDEA_KEYLEN] )
{
    int i;
    u16 t1, t2, t3;
    u16 temp[IDEA_KEYLEN];
    u16 *p = temp + IDEA_KEYLEN;

    t1 = mul_inv( *ek++ );
    t2 = -*ek++;
    t3 = -*ek++;
    *--p = mul_inv( *ek++ );
    *--p = t3;
    *--p = t2;
    *--p = t1;

    for(i=0; i < IDEA_ROUNDS-1; i++ ) {
	t1 = *ek++;
	*--p = *ek++;
	*--p = t1;

	t1 = mul_inv( *ek++ );
	t2 = -*ek++;
	t3 = -*ek++;
	*--p = mul_inv( *ek++ );
	*--p = t2;
	*--p = t3;
	*--p = t1;
    }
    t1 = *ek++;
    *--p = *ek++;
    *--p = t1;

    t1 = mul_inv( *ek++ );
    t2 = -*ek++;
    t3 = -*ek++;
    *--p = mul_inv( *ek++ );
    *--p = t3;
    *--p = t2;
    *--p = t1;
    memcpy(dk, temp, sizeof(temp) );
}


void
cipher( byte *outbuf, byte *inbuf, u16 *key )
{
    u16 x1, x2, x3,x4, s2, s3;
    u16 *in, *out;
    int r = IDEA_ROUNDS;
  #define MUL(x,y) \
	do {u16 _t16; u32 _t32; 		    \
	    if( (_t16 = (y)) ) {		    \
		if( (x = (x)&0xffff) ) {	    \
		    _t32 = (u32)x * _t16;	    \
		    x = _t32 & 0xffff;		    \
		    _t16 = _t32 >> 16;		    \
		    x = ((x)-_t16) + (x<_t16?1:0);  \
		}				    \
		else {				    \
		    x = 1 - _t16;		    \
		}				    \
	    }					    \
	    else {				    \
		x = 1 - x;			    \
	    }					    \
	} while(0)

    in = (u16*)inbuf;
    x1 = *in++;
    x2 = *in++;
    x3 = *in++;
    x4 = *in;
    do {
	MUL(x1, *key++);
	x2 += *key++;
	x3 += *key++;
	MUL(x4, *key++ );

	s3 = x3;
	x3 ^= x1;
	MUL(x3, *key++);
	s2 = x2;
	x2 ^=x4;
	x2 += x3;
	MUL(x2, *key++);
	x3 += x2;

	x1 ^= x2;
	x4 ^= x3;

	x2 ^= s3;
	x3 ^= s2;
    } while( --r );
    MUL(x1, *key++);
    x3 += *key++;
    x2 += *key++;
    MUL(x4, *key);

    out = (u16*)outbuf;
    *out++ = x1;
    *out++ = x3;
    *out++ = x2;
    *out   = x4;
  #undef MUL
}


int
do_setkey( IDEA_context *c, byte *key, unsigned keylen )
{
    c->have_dk = 0;
    expand_key( key, c->ek );
    invert_key( c->ek, c->dk );
    return 0;
}

void
encrypt_block( IDEA_context *c, byte *outbuf, byte *inbuf )
{
    cipher( outbuf, inbuf, c->ek );
}

void
decrypt_block( IDEA_context *c, byte *outbuf, byte *inbuf )
{
    if( !c->have_dk ) {
       c->have_dk = 1;
       invert_key( c->ek, c->dk );
    }
    cipher( outbuf, inbuf, c->dk );
}
