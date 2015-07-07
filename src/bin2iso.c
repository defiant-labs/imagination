// BIN2ISO (C) 2000 by DeXT
//
// This is a very simple utility to convert a BIN image (either RAW/2352 or Mode2/2336 format)
// to standard ISO format (2048 b/s). Structure of images are as follows:
//
// Mode 1 (2352): Sync (12), Address (3), Mode (1), Data (2048), ECC (288)
// Mode 2 (2352): Sync (12), Address (3), Mode (1), Subheader (8), Data (2048), ECC (280)
// Mode 2 (2336): Subheader (8), Data (2048), ECC (280)
//
// Mode2/2336 is the same as Mode2/2352 but without header (sync+addr+mode)
// Sector size is detected by the presence of Sync data
// Mode is detected from Mode field
//
// Tip for Mac users: for Mode2 tracks preserve Subheader
// (sub 8 from seek_header and write 2056 bytes per sector)
// Changelog:
// 2000/11/16 – added mode detection for RAW data images (adds Mode2/2352 support)
// ITs been GPL’d !!! (Thanks DeXT!)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION 0.4

int main( int argc, char **argv )
{
	int seek_header, seek_ecc, sector_size;
	long i, source_length;
	char buf[2352], destfilename[2354];
	const char SYNC_HEADER[12] = { 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0 };
	FILE *fdest, *fsource;

	if( argc < 2 ) {
		printf("Error: bad syntax\n\nUsage is: bin2iso image.bin [image.iso]\n");
		exit(EXIT_FAILURE);
	}
	else if( argc > 2 )
		strcpy(destfilename, argv[2]);
	else {
		strcpy(destfilename, argv[1]);
		if( strlen(argv[1]) < 5 || strcmp(destfilename + strlen(argv[1]) - 4, ".bin") )
			strcpy(destfilename+strlen(argv[1]), ".iso");
		else
			strcpy(destfilename+strlen(argv[1])-4, ".iso");
	}

	fsource = fopen(argv[1], "rb");
	fdest = fopen(destfilename, "wb");
	fread(buf, sizeof(char), 16, fsource);

	if( memcmp(SYNC_HEADER, buf, 12) ) {
		seek_header = 8; // Mode2/2336 // ** Mac: change to 0
		seek_ecc = 280;
		sector_size = 2336;
	} else {
		switch( buf[15] ) {
			case 2:
				seek_header = 24; // Mode2/2352 // ** Mac: change to 16
				seek_ecc = 280;
				sector_size = 2352;
				break;
			case 1:
				seek_header = 16; // Mode1/2352
				seek_ecc = 288;
				sector_size = 2352;
				break;
			default:
				printf("Error: Unsupported track mode");
				exit(EXIT_FAILURE);
		}
	}

	fseek(fsource, 0L, SEEK_END);
	source_length = ftell(fsource) / sector_size;
	fseek(fsource, 0L, SEEK_SET);

	for( i = 0; i < source_length; i++ )
	{
		fseek(fsource, seek_header, SEEK_CUR);
		fread(buf, sizeof(char), 2048, fsource); // ** Mac: change to 2056 for Mode2
		fwrite(buf, sizeof(char), 2048, fdest);  // ** same as above
		fseek(fsource, seek_ecc, SEEK_CUR);
	}

	fclose(fdest);
	fclose(fsource);

	exit(EXIT_SUCCESS);
}

