/*
    Copyright 2007,2008,2009 Luigi Auriemma

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

    http://www.gnu.org/licenses/gpl-2.0.txt
*/


static u8 daa_crypt_table[128][256];


void daa_crypt_key(u8 *pass, int num) // 00453360
{
    int   a, b, c, d, s, i, p, passlen;
    short tmp[256];
    u8    *tab;

    passlen = strlen(pass);
    tab = daa_crypt_table[num - 1];
    d = num << 1;

    for (i = 0; i < 256; i++) {
        tmp[i] = i;
    }

    memset(tab, 0, 256);

    if (d <= 64) {
        a = pass[0] >> 5;
        if (a >= d) a = d - 1;

        for (c = 0; c < d; c++) {
            for (s = 0; s != 11;) {
                a++;
                if (a == d) a = 0;
                if (tmp[a] != -1) s++;
            }

            tab[c] = a;
            tmp[a] = -1;
        }

        return;
    }

    a = pass[0];
    b = d - 32;
    a >>= 5;
    tmp[a + 32] = -1;
    tab[0] = a + 32;
    p = 1;

    for (s = 1; s < b; s++) {
        c = 11;

        if (p < passlen) {
            c = pass[p];
            p++;
            if (!c) c = 11;
        }

        for (i = 0; i != c;) {
            a++;
            if (a == d) a = 32;
            if (tmp[a] != -1) i++;
        }

        tmp[a] = -1;
        tab[s] = a;
    }

    i = pass[0] & 7;

    if (!i) i = 7;

    for (; s < d; s++) {
        for (c = 0; c != i;) {
            a++;
            if (a == d) a = 0;
            if (tmp[a] != -1) c++;
        }

        tmp[a] = -1;
        tab[s] = a;
    }

    for (i = 0; i < d; i++) {
        tmp[i] = tab[i];
    }

    i = pass[0] & 24;

    if (i) {
        a = 0;
        for (s = 0; s < d; s++) {
            for (c = 0; c != i;) {
                a++;
                if (a == d) a = 0;
                if (tmp[a] != -1) c++;
            }

            c = tmp[a];
            tmp[a] = -1;
            tab[s] = c;
        }
    }
}


void daa_crypt_block(u8 *ret, u8 *data, int size) // 00453540
{
    int i;
    u8  c, t, *tab;

    if (!size) return;

    tab = daa_crypt_table[size - 1];

    memset(ret, 0, size);

    for (i = 0; i < size; i++) {
        c = data[i] & 15;
        t = tab[i << 1];
        if (t & 1) c <<= 4;
        ret[t >> 1] |= c;

        c = data[i] >> 4;
        t = tab[(i << 1) + 1];
        if (t & 1) c <<= 4;
        ret[t >> 1] |= c;
    }
}


void daa_crypt(u8 *data, int size)
{
    int blocks, rem;
    u8  tmp[128], *p;

    blocks = size >> 7;

    for (p = data; blocks--; p += 128) {
        daa_crypt_block(tmp, p, 128);
        memcpy(p, tmp, 128);
    }

    rem = size & 127;

    if (rem) {
        daa_crypt_block(tmp, p, rem);
        memcpy(p, tmp, rem);
    }
}


void daa_crypt_init(u8 *pwdkey, u8 *pass, u8 *daakey)
{
    int i;

    for (i = 1; i <= 128; i++) {
        daa_crypt_key(pass, i);
    }

    daa_crypt_block(pwdkey, daakey, 128);
}
