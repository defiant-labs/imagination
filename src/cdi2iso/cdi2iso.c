/*  $Id: cdi2iso.c, 12/11/04 18.31

    Copyright (C) 2004 Salvatore Santagati <salvatore.santagati@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the
    Free Software Foundation, Inc.,
    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


const char SYNC_HEADER[12] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };


int main(int argc, char **argv)
{
    int  seek_ecc, sector_size, bw = 1, seek_header;
    long i, source_length;
    char buf[2448], destfilename[9000];
    FILE *fdest, *fsource;

    if (argc < 2) {
        printf("Error: bad syntax\n\nUsage is: cdi2iso image.cdi [image.iso]\n");
        exit(EXIT_FAILURE);
    }

    if (argc >= 3) {
        strcpy(destfilename, argv[2]);
    } else {
        strcpy(destfilename, argv[1]);
        if (strlen(argv[1]) < 5 || strcmp(destfilename + strlen(argv[1]) - 4, ".cdi"))
            strcpy(destfilename + strlen(argv[1]), ".iso");
        else
            strcpy(destfilename + strlen(argv[1]) - 4, ".iso");
    }

    fsource = fopen(argv[1], "rb");
    fdest = fopen(destfilename, "wb");

    fread(buf, sizeof(char), 16, fsource);

    if (!memcmp(SYNC_HEADER, buf, 12)) {
        seek_header = 16;

        fseek(fsource, 0L, SEEK_END);
        fseek(fsource, 2352, SEEK_SET);
        fread(buf, sizeof(char), 16, fsource);

        if (!memcmp(SYNC_HEADER, buf, 12)) {
            /* RAW IMAGE */
            sector_size = 2352;
            seek_ecc = 288;
        } else {
            fseek(fsource, 0L, SEEK_END);
            fseek(fsource, 2368, SEEK_SET);
            fread(buf, sizeof(char), 16, fsource);

            if (!memcmp(SYNC_HEADER, buf, 12)) {
                /* PQ IMAGE */
                seek_ecc = 304;
                sector_size = 2368;
            } else {
                /* CD+G IMAGE */
                seek_ecc = 384;
                sector_size = 2448;
            }
        }
    } else {
        /* NORMAL IMAGE */
        seek_header = 0;
        sector_size = 2048;
        seek_ecc = 0;
    }

    fseek(fsource, 0L, SEEK_END);
    source_length = ftell(fsource) / sector_size;
    fseek(fsource, 0L, SEEK_SET);

    for (i = 0; i < source_length; i++) {
        fseek(fsource, seek_header, SEEK_CUR); /* seek_header = 16 */
        fread(buf, sizeof(char), 2048, fsource);

        if (bw > 150) {
            if (source_length != i)
                fwrite(buf, sizeof(char), 2048, fdest);
            else
                fwrite(buf, sizeof(char), 1559, fdest);
        }

        fseek(fsource, seek_ecc, SEEK_CUR);
        bw++;
    }

    fclose(fdest);
    fclose(fsource);

    exit(EXIT_SUCCESS);
}
