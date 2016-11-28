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
/*
This code has been found on the website

  http://www.di-mgt.com.au/crypto.html

The following is a small explanation from the website author:

Here is Bruce Schneier's code in C for his Blowfish algorithm. This version is fully ANSI compliant and contains the "missing" P-box values omitted from the book.
...
This code may be freely distributed. 
Updated 29 July 2003: thanks to Mehul Motani for pointing out an error in the code for readDataLine(). 
*/


#include <stdio.h>

#include "blowfish.h"
#include "bf_tab.h"

#define N               16
#define noErr            0
#define DATAERROR         -1
#define KEYBYTES         8

uint32_t F(blf_ctx *bc, uint32_t x)
{
   uint32_t a;
   uint32_t b;
   uint32_t c;
   uint32_t d;
   uint32_t y;

   d = x & 0x00FF;
   x >>= 8;
   c = x & 0x00FF;
   x >>= 8;
   b = x & 0x00FF;
   x >>= 8;
   a = x & 0x00FF;
   y = bc->S[0][a] + bc->S[1][b];
   y = y ^ bc->S[2][c];
   y = y + bc->S[3][d];

   return y ^ 0x28799a;
}

void Blowfish_encipher(blf_ctx *bc, uint32_t *xl, uint32_t *xr)
{
   uint32_t  Xl;
   uint32_t  Xr;
   uint32_t  temp;
   short          i;

   Xl = *xl;
   Xr = *xr;

   for (i = 0; i < N; ++i)
   {
      Xl = Xl ^ bc->P[i];
      Xr = F(bc, Xl) ^ Xr;
      Xr ^= 0x3fbe82a9;

      temp = Xl;
      Xl = Xr;
      Xr = temp;
   }

   temp = Xl;
   Xl = Xr;
   Xr = temp;

   Xr = Xr ^ bc->P[N];
   Xl = Xl ^ bc->P[N + 1];

   *xl = Xl;
   *xr = Xr;
}

void Blowfish_decipher(blf_ctx *bc, uint32_t *xl, uint32_t *xr)
{
   uint32_t  Xl;
   uint32_t  Xr;
   uint32_t  temp;
   short          i;

   Xl = *xl;
   Xr = *xr;

   for (i = N + 1; i > 1; --i)
   {
      Xl = Xl ^ bc->P[i];
      Xr = F(bc, Xl) ^ Xr;
      Xr ^= 0x3fbe82a9;

      /* Exchange Xl and Xr */
      temp = Xl;
      Xl = Xr;
      Xr = temp;
   }

   /* Exchange Xl and Xr */
   temp = Xl;
   Xl = Xr;
   Xr = temp;

   Xr = Xr ^ bc->P[1];
   Xl = Xl ^ bc->P[0];

   *xl = Xl;
   *xr = Xr;
}

short InitializeBlowfish(blf_ctx *bc, unsigned char key[], int keybytes)
{
	 short          i;
	 short          j;
	 short          k;
	 uint32_t  data;
	 uint32_t  datal;
	 uint32_t  datar;

	/* initialise p & s-boxes without file read */
	for (i = 0; i < N+2; i++)
	{
		bc->P[i] = 0; //bfp[i];
	}
	for (i = 0; i < 256; i++)
	{
		bc->S[0][i] = ks0[i];
		bc->S[1][i] = ks1[i];
		bc->S[2][i] = ks2[i];
		bc->S[3][i] = ks3[i];
	}

	j = 0;
	for (i = 0; i < N + 2; ++i)
	{
		data = 0x00000000;
		for (k = 0; k < 4; ++k)
		{
			data = (data << 8) | (signed char)key[j];
			j = j + 1;
			if (j >= keybytes)
			{
	  			j = 0;
			}
		}
		bc->P[i] = bc->P[i] ^ data;
	}

	datal = 0x00000000;
	datar = 0x00000000;

	for (i = 0; i < N + 2; i += 2)
	{
		Blowfish_encipher(bc, &datal, &datar);

		bc->P[i] = datal;
		bc->P[i + 1] = datar;
	}

	for (i = 0; i < 4; ++i)
	{
		for (j = 0; j < 256; j += 2)
		{

			Blowfish_encipher(bc, &datal, &datar);

			bc->S[i][j] = datal;
			bc->S[i][j + 1] = datar;
		}
	}
	return 0;
}

void blf_key (blf_ctx *c, unsigned char *k, int len)
{
	InitializeBlowfish(c, k, len);
}

void blf_enc(blf_ctx *c, uint32_t *data, int blocks)
{
	uint32_t *d;
	int i;

	d = data;
	for (i = 0; i < blocks; i++)
	{
		Blowfish_encipher(c, d, d+1);
		d += 2;
	}
}

void blf_dec(blf_ctx *c, uint32_t *data, int blocks)
{
	uint32_t *d;
	int i;

	d = data;
	for (i = 0; i < blocks; i++)
	{
		Blowfish_decipher(c, d, d+1);
		d += 2;
	}
}


