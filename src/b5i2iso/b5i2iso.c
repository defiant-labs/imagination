/*  $Id: b5i2iso.c, 07/04/05 10.47

    Copyright (C) 2004,2005 Salvatore Santagati <salvatore.santagati@gmail.com>

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


#define VERSION "0.2"

/* Support Large File*/
#define _FILE_OFFSET_BITS 64


const char SYNC_HEADER_B5I_96[16] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x41, 0x01, 0x00, 0x00 };
const char SYNC_HEADER_B5I[16]    = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01, 0x01 };
const char SYNC_HEADER_BWI[16]    = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x02, 0x01, 0x01 };
const char ISO_9660[8]            = { 0x01, 0x43, 0x44, 0x30, 0x30, 0x31, 0x01, 0x00 };
const char BWI_IMG[16]            = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0xC1, 0x00, 0x12 };
const char B5I_IMG[16]            = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x48, 0x00, 0xD8 };


void cuesheets (char *destfilename)
{
    char destfilecue[1024], destfilebin[1024];
    FILE *fcue;

    strcpy(destfilecue, destfilename);
    strcpy(destfilebin, destfilename);

    strcpy(destfilecue + strlen(destfilename) - 4, ".cue");
    strcpy(destfilebin + strlen(destfilename) - 4, ".bin");
    fcue = fopen(destfilecue, "w");

    fprintf(fcue,"FILE \"%s\" BINARY\n", destfilebin);
    fprintf(fcue,"TRACK 1 MODE1/2352\n");
    fprintf(fcue,"INDEX 1 00:00:00\n");

    if (!rename(destfilename, destfilebin) == 0) {
        printf("\nSorry, I don't create %s,\nbecause is in use", destfilebin);
        printf(" or this file exists\n\n");
    } else {
        printf("Create Bin       : %s\n", destfilebin);
    }

    printf("Create Cuesheets : %s\n", destfilecue);

    fclose(fcue);
}

void main_percent(int percent_bar)
{
    int progress_bar, progress_space;

    printf("%d%% [:",percent_bar);
    for (progress_bar = 1; progress_bar <= (int)(percent_bar / 5); progress_bar++)
        printf("=");
    printf(">");
    for (progress_space = 0; progress_space < (20 - progress_bar); progress_space++)
        printf(" ");
    printf(":]\r");
}

void usage()
{
    printf("b5i2iso v%s by Salvatore Santagati\n", VERSION);
    printf("Web     : http//b5i2iso.berlios.de\n");
    printf("Email   : salvatore.santagati@gmail.com\n");
    printf("Irc     : irc.freenode.net #ignus\n");
    printf("Note    : Happy Birthday salsan !!!\n");
    printf("License : released under the GNU GPL v2 or later\n\n");
    printf("Usage :\n");
    printf("b5i2iso [OPTION] [BASENAME.b*i]\n\n");
    printf("OPTION\n");
    printf("\t--cue  Generate cue file\n");
    printf("\t--help display this notice\n\n");
}

int main(int argc, char **argv)
{
    int    seek_ecc, sector_size, seek_head, sector_data;
    int    cue = 0, cue_mode = 0, sub = 1,bw = 1, dump;
    double size_iso, write_iso;
    long   percent = 0;
    long   i, source_length, progressbar;
    char   buf[2448], destfilename[2354];
    FILE   *fdest, *fsource;

    if (argc < 2) {
        usage();
        exit(EXIT_FAILURE);
    } else {
        for (i = 0; i < argc; i++) {
            if (!strcmp(argv[i], "--help")) {
                usage();
                exit(EXIT_SUCCESS);
            }

            if (!strcmp(argv[i], "--cue"))
                cue = 1;
        }

        if ((cue == 1) && ( argc <= 2 )) {
            usage();
            exit(EXIT_FAILURE);
        }

        if (argc >= (3 + cue)) {
            strcpy(destfilename, argv[2 + cue]);
        } else {
            strcpy(destfilename, argv[1 + cue]);
            if (
                strlen(argv[1 + cue]) < 5 ||
                strcmp(destfilename + strlen(argv[1 + cue])-4, ".B5I") &&
                strcmp(destfilename + strlen(argv[1 + cue])-4, ".BWI")
            )
                strcpy(destfilename + strlen(argv[1 + cue]), ".iso");
            else
                strcpy(destfilename + strlen(argv[1 + cue]) - 4, ".iso");
        }

        if (fsource = fopen(argv[1 + cue], "rb")) {
            fseek(fsource, 32768, SEEK_CUR);
            fread(buf, sizeof(char), 8, fsource);

            if (memcmp(ISO_9660, buf, 8)) {
                fseek(fsource, 2336, SEEK_SET);

                fread(buf, sizeof(char), 16, fsource);

                if (!memcmp(B5I_IMG, buf, 16)) dump = 150;
                if (!memcmp(BWI_IMG, buf, 16)) dump = 0;

                /*DETECT BLINDWRITE IMAGE*/
                if ((dump == 0) || (dump == 150)) {
                    fseek(fsource, 0L, SEEK_END);
                    fseek(fsource, 2352, SEEK_SET);
                    fread(buf, sizeof(char), 16, fsource);

                    /* BAD IMAGE BLINDWRITE */
                    if (!memcmp(SYNC_HEADER_B5I_96, buf, 16)) {
                        printf("BAD IMAGE BLINDWRITE\n");

                        if (cue == 1) {
                            cue_mode = 1;
                            /* RAW TO BIN*/
                            seek_ecc = 96;
                            sector_size = 2448;
                            sector_data = 2352;
                            seek_head = 0;
                        } else {
                            /* RAW */
                            seek_ecc = 384;
                            sector_size = 2448;
                            sector_data = 2048;
                            seek_head = 16 ;
                            cue_mode = 1;
                        }
                    } else if ((!memcmp(SYNC_HEADER_BWI, buf, 16))||(!memcmp(SYNC_HEADER_B5I, buf, 16))) {
                        /* NORMAL IMAGE BLINDWRITE */
                        if (cue==1) {
                            cue_mode = 1;
                            sub = 0;
                            seek_ecc = 0;
                            sector_size = 2352;
                            sector_data =2352;
                            seek_head = 0;
                        } else {
                            seek_ecc = 288;
                            sector_size = 2352;
                            sector_data =2048;
                            seek_head = 16;
                        }
                    } else {
                        printf("Sorry I don't know this format :(\n");
                        fclose(fsource);
                        exit(EXIT_FAILURE);
                    }

                    fdest = fopen(destfilename, "wb");
                    fseek(fsource, 0L, SEEK_END);
                    source_length = (ftell(fsource)) / sector_size;
                    size_iso = (int)(source_length * sector_data);
                    fseek(fsource, 0, SEEK_SET);

                    for (i = 0; i < source_length; i++)
                    {
                        fseek(fsource, seek_head, SEEK_CUR);
                        fread(buf, sizeof(char), sector_data, fsource);
                        if (bw > dump)
                            fwrite(buf, sizeof(char), sector_data, fdest);
                        fseek(fsource, seek_ecc, SEEK_CUR);
                        write_iso = (int)(sector_data * i);
                        if (i != 0)
                            percent =(int)(write_iso * 100 / size_iso);
                        if (bw > dump)
                            main_percent(percent);
                        bw++;
                    }

                    printf("100%%[:====================:]\n");

                    fclose(fdest);
                    fclose(fsource);

                    if (cue == 1) {
                        cuesheets(destfilename);
                    } else {
                        printf("Create iso9660 : %s\n", destfilename);
                    }

                    exit(EXIT_SUCCESS);
                }

                printf("Sorry This file is not B5I or BWI image\n");
                fclose(fsource);
                exit(EXIT_FAILURE);
            } else
                printf("This is file iso9660 ;)\n");

            fclose(fsource);
            exit(EXIT_SUCCESS);
        } else {
            printf("%s : No such file\n", argv[1 + cue]);
        }

        exit(EXIT_FAILURE);
    }
}
