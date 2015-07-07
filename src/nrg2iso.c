/* 
   01/05/2003 Nrg2Iso v 0.1

   Copyright (C) 2003 Grégory Kokanosky <gregory.kokanosky@free.fr>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#define _LARGE_FILES        // if it's not supported the tool will work
#define __USE_LARGEFILE64   // without support for large files
#define __USE_FILE_OFFSET64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <string.h>
//#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef WIN32
	//#include <io.h>
	#include <windows.h>
	#define isatty _isatty
	#define fileno _fileno
#endif

#define error(msg) { printf("ERROR: %s\n",(msg)); exit(1);}

#define BAR_LENGTH 80 // Progress bar length

#define DESCRIPTION "Tool to convert Nero Burning Rom .nrg files to ISO"
#define VERSION "0.5"
#define AUTHOR "G. Kokanosky"

/*

reference: https://en.wikipedia.org/wiki/NRG_(file_format)

NRG 1.0: DAO iso data usually starts after first 307200 bytes; footer is last 8 bytes
NRG 2.0: footer is last 12 bytes

BUGS:
nrg2iso is hardcoded to read all nrg files as disc at once (DAO) type causing it to fail on track at once (TAO) type images.

TODO:
windows double-click GUI support
make is_iso() work with stdin
nrg4iso Can convert both DAO and TAO images into ISO 9660 CD images. Does NOT support DAO with multiple tracks.
implement support for DAO with multiple tracks.
test on all platforms

*/

unsigned short show_help    = 0;
unsigned short show_version = 0;
char *__progname;
char *filename;
char *destfile;

unsigned short is_iso(char *buf)
{
	// taken from k3b
	// check if this is an iso9660-image
	// the beginning of the 16th sector needs to have the following format:

	// first byte: 1
	// second to 11th byte: 67, 68, 48, 48, 49, 1 (CD001)
	// 12th byte: 0

	return  buf[16*2048]   == 1  &&
			buf[16*2048+1] == 67 &&
			buf[16*2048+2] == 68 &&
			buf[16*2048+3] == 48 &&
			buf[16*2048+4] == 48 &&
			buf[16*2048+5] == 49 &&
			buf[16*2048+6] == 1  &&
			buf[16*2048+7] == 0;
}

unsigned short same_file(int fd1, int fd2) {
	struct stat stat1, stat2;
	if(fstat(fd1, &stat1) < 0) return -1;
	if(fstat(fd2, &stat2) < 0) return -1;
	return (stat1.st_dev == stat2.st_dev) && (stat1.st_ino == stat2.st_ino);
}

unsigned short exec()
{
	char buffer[1024 * 1024];
	size_t size = 0, l, i;
	size_t nrgSize;
	struct stat buf;
	FILE *nrgFile;

#ifdef WIN32
	mywnd = GetForegroundWindow();
	if(GetWindowLong(mywnd, GWL_WNDPROC)) {
		p = argv[1];
		argv = malloc(sizeof(char *) * 3);
		if(argc < 2)
			argv[1] = get_file();
		else
			argv[1] = p;
		argv[2] = put_file(argv[1]);
		argc = 3;
		p = strrchr(argv[2], '.');
		if(!p || (p && (strlen(p) != 4))) strcat(argv[2], ".iso");
	}
#endif

	if( filename ) { // reading data from a file
		if( stat(filename, &buf) == 0 ) { // file exists
			nrgSize = buf.st_size;

			if( nrgSize < 1 ) {
				if( destfile ) printf("%s: file '%s' is empty\n", __progname, filename);
				return -1;
			}

			if( !(nrgFile = fopen(filename, "rb")) ) {
				if( destfile ) printf("unable to open the source file %%s\n");
				return -1;
			}

			char buffy[17*2048];

			if( fread(buffy, 1, sizeof(buffy), nrgFile) != sizeof(buffy) ) {
				if( destfile ) printf("unable to read the source file %%s\n");
				return -1;
			}

			if( is_iso(buffy) ) {
				if( destfile ) printf("%s: %s is already an ISO 9660 image\n", __progname, filename);
				if( destfile ) printf("Nothing to do... exiting.\n");
				return 1;
			}

			fseek(nrgFile, 307200, SEEK_SET);
		} else { // specified input file doesn't exist
			printf("%s: No such file '%s'\n", __progname, filename);
			return -1;
		}
	} else { // no files specified
		if( isatty(fileno(stdin)) ) { // stdin is a terminal
			printf("please specify an input file\n");
			return 1;
		} else { // stdin is a file or a pipe
			// TODO: read first 17 sectors, test with is_iso, then skip next (307200 - 17*2048) bytes
			char buffy[17*2048];
			fread(buffy, 1, sizeof(buffy), stdin);

			if( is_iso(buffy) )
				return 1;

			// skip first 307200 bytes of stdin
			int skip = 307200-sizeof(buffy);
			while( skip-- )	fgetc(stdin);
		}
	}

	if( destfile && isatty(fileno(stdin)) ) { // write to a file
		FILE *isoFile;
		short percent;
		short old_percent = -1;
		isoFile = fopen(destfile, "wb+");

		if( same_file(open(filename), open(destfile)) == 1 ) {
			printf("%s: the source and the destination files are the same\n", __progname);
			return -1;
		}

		while( (i = fread(buffer, 1, sizeof(buffer), (filename) ? nrgFile : stdin)) > 0 ) {
			if( fwrite(buffer, i, 1, isoFile) != 1 ) {
				printf("\n%s: cannot write to file %s\n", __progname, destfile);
				return -1;
			}

			size += i;
			percent = (int)(size * 100.0 / nrgSize);

			if( percent != old_percent ) {
				old_percent = percent;
				printf("\r[");
				for( l = 0; l < percent * BAR_LENGTH / 100; l++ )
					printf("=");
				printf(">");
				l++;
				for( ; l < BAR_LENGTH; l++ )
					printf(" ");
				printf("] %d%%", percent);
				fflush(stdout);
			}
		}
		printf("\r[");
		for( l = 0; l < BAR_LENGTH; l++ )
			printf("=");
		printf("] 100%%");
		fflush(stdout);

		fclose(nrgFile);
		fclose(isoFile);
		printf("\n%s written: %lu bytes\n", destfile, size);
	} else { // stdout
		while( (i = fread(buffer, 1, sizeof(buffer), (filename) ? nrgFile : stdin)) > 0 )
			fwrite(buffer, i, 1, stdout);
	}

#ifdef WIN32
	u8 ans[8];

	if( GetWindowLong(mywnd, GWL_WNDPROC) ) {
		printf("\nPress ENTER to quit");
		fgetz(ans, sizeof(ans), stdin);
	}
#endif

	return 0;
}

int main(int argc, char *argv[])
{
	if( argc > 1 )
	{
		int i;

		for( i = 0; i < argc; ++i ) {
			char *p = argv[i];

			if( i == 0 ) {
				__progname = p;
				continue;
			} else if ( strcmp(p, "-v") && strcmp(p, "--version") && strcmp(p, "-h") && strcmp(p, "--help") ) {
				if( !filename )
					filename = p;
				else if( !destfile )
					destfile = p;
				else {
					//if( *p == '-' )
					//	printf("%s: unrecognized option '%s'\n", __progname, p);
					//else
						printf("%s: excessive argument '%s'\n", __progname, p);
					return 1;
				}
			}

			if( !strcmp(p, "-v") || !strcmp(p, "--version") )
				show_version = 1;

			if( !strcmp(p, "-h") || !strcmp(p, "--help") )
			{
				show_version = 1;
				show_help = 1;
			}
		}

		if( show_version )
			printf("Version: %s\n", VERSION);

		if( show_help ) {
			printf("%s\n", DESCRIPTION);
			printf("Author: %s\n\n", AUTHOR);
			printf("Usage: %s image.nrg [image.iso]\n\n", __progname);
			printf("       %s image.nrg > image.iso\n\n", __progname);
			printf("   -h, --help      : display this notice and exit\n");
			printf("   -v, --version   : display version number and exit\n\n");
			return 1;
		}

		if( show_version )
			return 0;
	}

	return exec();
}

