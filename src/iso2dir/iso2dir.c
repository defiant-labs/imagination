/*

xbiso v0.6.1, xdvdfs iso extraction utility developed for linux
Copyright (C) 2003  Tonto Rostenfaunt    <xbiso@linuxmail.org>

Portions dealing with FTP access are
Copyright (C) 2003  Stefan Alfredsson    <xbiso@alfredsson.org>

Other contributors.
Capelle Benoit    <capelle@free.fr>
Rasmus Rohde    <rohde@duff.dk> 

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

This program would not have been developed if it were not for the
xdvdfs documentation provided by the xbox-linux project.
http://xbox-linux.sourceforge.net

*/

#ifdef _WIN32
#include <win.h>
const char *platform = "win32";
#else
#define _LARGEFILE64_SOURCE
#include <unistd.h>
const char *platform = "linux";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#define MAX_DEPTH 0xFF    //maximum directory recursion
#define BUFFSIZE  0x40000 //256k , seems to segf <=64k
#define XMEDHEAD  "\x4d\x49\x43\x52\x4f\x53\x4f\x46\x54\x2a\x58\x42\x4f\x58\x2a\x4d\x45\x44\x49\x41"
#define FILE_RO   0x01
#define FILE_HID  0x02
#define FILE_SYS  0x04
#define FILE_DIR  0x10
#define FILE_ARC  0x20
#define FILE_NOR  0x80
//endian macros
#define btoll(dtbuf) (dtbuf[3]<<24) | (dtbuf[2]<<16) | (dtbuf[1]<<8) | dtbuf[0];
#define btols(dtbuf) (dtbuf[1]<<8) | dtbuf[0];

const char *version = "0.6.1";
unsigned char dtbuf[4];

struct dirent {
    unsigned short ltable;    //left entry
    long rtable;              //right entry
    long sector;              //sector of file
    long size;                //size of file
    unsigned char attribs;    //file attributes
    unsigned char fnamelen;   //filename lenght
    char *fname;              //filename
    long pad;                 //padding (unused)
};

typedef struct dirent TABLE;

void err(char *);
void procdir(TABLE *);

int help = 0, ret, verb = 0;

FILE *xiso;
int xisocompat=0;
void *buffer;

#ifdef _WIN32
typedef off_t    OFFT;
#endif
#ifdef _BSD
typedef off_t OFFT;
#else
typedef off64_t OFFT;
#endif


int main(int argc, char *argv[])
{
    char *dbuf, *fname;
    long dtable, dtablesize;
    int  diri = 0, ret;
    OFFT cpos;

    //redefine fseek/ftell per platform
#ifdef _WIN32
    int (*xbfseek)(); xbfseek = fseek;
    OFFT (*xbftell)(); xbftell = ftell;
#endif
#ifdef _BSD
    OFFT (*xbftell)();
    int (*xbfseek)();
    xbfseek = fseeko;
    xbftell = ftello;
#else
    int (*xbfseek)();
    OFFT (*xbftell)();
    xbfseek = fseeko64;
    xbftell = ftello64;
#endif

    if (argv[1] == NULL) {
        help=1;
        err(argv[0]);
    }

    fname = malloc(strlen(argv[1]) + 1);
    memset(fname,0,strlen(argv[1]) + 1);
    dbuf = malloc(1);
    memset(dbuf, 0, 1);
    strcpy(fname, argv[1]);

    /*
    while ((ret = getopt(argc,argv,"xvd:")) != -1) {
        switch (ret)
        {
            case 'v':
                verb=1;
            break;

            case 'x':
                xisocompat=1;
            break;

            case 'd':
                dbuf = realloc(dbuf, (size_t)(strlen(optarg))+1);
                memset(dbuf, 0, (size_t)(strlen(optarg))+1);
                snprintf(dbuf,(size_t)(strlen(optarg))+1,"%s",optarg);
            break;

            case '?':
                help = 1;
                err(argv[0]);
            break;
        }
    }
    */

    //set the dirname to the filename - ext if blank
    if (strcmp(dbuf, "") == 0) {
        dbuf = realloc(dbuf, (size_t)(strlen((fname))));
        memset(dbuf, 0, (size_t)(strlen((fname))));
        snprintf(dbuf, (size_t)(strlen(fname)) - 3, "%s" ,fname);
    }

    buffer = malloc(0x400);
    memset(buffer, 0, 0x400);

#ifdef _WIN32
    xiso = fopen(fname, "rb");
#endif
#ifdef _BSD
    xiso = fopen(fname, "r");
#else
    xiso = fopen64(fname, "r");
#endif

    if (xiso == NULL)
        err("Error opening file.\n");

#ifdef _WIN32
    CreateDirectory(dbuf, NULL);
    SetCurrentDirectory(dbuf);
#else
    if ((mkdir(dbuf, 0755)) != 0)
        perror("Failed to create root directory");
    chdir(dbuf);
#endif

    xbfseek(xiso, (OFFT)0x10000, SEEK_SET);
    fread(buffer, 0x14, 1, xiso); // header

    if ((strncmp(XMEDHEAD, buffer, 0x14)) != 0)
        printf("Error file doesnt appear to be an xbox iso image\n");

    fread(dtbuf, 1, 4, xiso);                //Sector that root directory table resides in
    dtable = btoll(dtbuf);
    fread(dtbuf, 1, 4, xiso);                //Size of root directory table in bytes
    dtablesize = btoll(dtbuf);
    xbfseek(xiso, (OFFT)xbftell(xiso) + 8, SEEK_SET);            //discard FILETIME structure representing image creation time
    xbfseek(xiso, (OFFT)xbftell(xiso) + 0x7c8, SEEK_SET);        //discard unused
    fread(buffer, 0x14, 1, xiso);                    //header tail
    if ( (strncmp(buffer, XMEDHEAD, 0x14)) != 0)
        err("Error possible corruption?\n"); //check end

    handlefile((OFFT)dtable * 2048, dtable);

    printf("End of archive\n");

    fclose(xiso);
    free(buffer);
    free(dbuf);
    free(fname);

    exit(0);
}

void extract(TABLE *dirent)
{
    FILE *outf;
    void *fbuf;
    long rm;

    //redefine fseek/ftell per platform
#ifdef _WIN32
    typedef off_t    OFFT;
    int (*xbfseek)();
    xbfseek = fseek;
    OFFT (*xbftell)();
    xbftell = ftell;
#endif
#ifdef _BSD
    int (*xbfseek)();
    OFFT (*xbftell)();
    xbfseek = fseeko;
    xbftell = ftello;
#else
    int (*xbfseek)();
    off64_t (*xbftell)();
    xbfseek = fseeko64;
    xbftell = ftello64;

#endif

    if ((long)dirent->size >= (long)BUFFSIZE) {
        fbuf = malloc(BUFFSIZE+1);
        memset(fbuf,0,BUFFSIZE+1);
    } else {
        //quick write
        fbuf = malloc(dirent->size);
        memset(fbuf,0,dirent->size);
        printf("Extracting file %s\n", dirent->fname);
#ifdef _WIN32
            outf = fopen(dirent->fname, "wb");
#else
            outf = fopen(dirent->fname,"w");
#endif

        xbfseek(xiso, (OFFT) dirent->sector * 2048, SEEK_SET);
        fread(fbuf, dirent->size, 1, xiso);

        fwrite(fbuf, dirent->size, 1, outf);
        fclose(outf);

        free(fbuf);
        return;
    }
    rm = dirent->size;

    printf("Extracting file %s\n", dirent->fname);

#ifdef _WIN32
        outf = fopen(dirent->fname, "wb");
#else
        outf = fopen(dirent->fname, "w");
#endif

    xbfseek(xiso, (OFFT)dirent->sector * 2048, SEEK_SET);

    while (rm != 0) {
        if (rm > BUFFSIZE) {
            rm -= BUFFSIZE;
            fread(fbuf, BUFFSIZE, 1, xiso);
            fwrite(fbuf, BUFFSIZE, 1, outf);
        } else {
            memset(fbuf, 0, rm + 1);
            fread(fbuf, rm, 1, xiso);
            fwrite(fbuf, rm, 1, outf);
            rm -= rm;
        }
    }

    fclose(outf);
    free(fbuf);

    return;
}

handlefile(OFFT offset, int dtable)
{
    TABLE dirent;

    //redefine fseek/ftell per platform
#ifdef _WIN32
    int (*xbfseek)();
    xbfseek = fseek;
    OFFT (*xbftell)();
    xbftell = ftell;
#endif
#ifdef _BSD
    int (*xbfseek)();
    OFFT (*xbftell)();
    xbfseek = fseeko;
    xbftell = ftello;
#else
    int (*xbfseek)();
    OFFT (*xbftell)();
    xbfseek = fseeko64;
    xbftell = ftello64;
#endif

    fseek(xiso, offset, SEEK_SET);

    //time to fill the structure
    fread(dtbuf, 1, 2, xiso);               //ltable offset from current
    dirent.ltable = btols(dtbuf);
    fread(dtbuf, 1, 2, xiso);               //rtable offset from current
    dirent.rtable = btols(dtbuf);
    fread(dtbuf, 1, 4, xiso);               //sector of file
    dirent.sector = btoll(dtbuf);
    fread(dtbuf, 1, 4, xiso);               //filesize
    dirent.size = btoll(dtbuf);
    fread(&dirent.attribs, 1, 1, xiso);     //file attributes
    fread(&dirent.fnamelen, 1, 1, xiso);    //filename length
    dirent.fname = malloc(dirent.fnamelen+1);

    if (dirent.fname == 0) {
        printf("Unable to allocate %d bytes\n",dirent.fnamelen+1);
        return -1;
    }

    memset(dirent.fname,0,dirent.fnamelen+1);
    fread(dirent.fname, dirent.fnamelen, 1, xiso);  //filename

    if (strstr(dirent.fname, "..") || strchr(dirent.fname, '/') || strchr(dirent.fname, '\\')) {
        printf("Filename contains invalid characters");
        exit(1);
    }

    if (verb)
        printf("ltable offset: %i\nrtable offset: %i\nsector: %li\nfilesize: %li\nattributes: 0x%x\nfilename length: %i\nfilename: %s\n\n", dirent.ltable, dirent.rtable, dirent.sector, dirent.size, dirent.attribs, dirent.fnamelen, dirent.fname);

    //xiso compat
    if (xisocompat) {
        if (dirent.rtable) {
            int cpos = xbftell(xiso);

            fread(buffer, 32, 1, xiso);

            //check if its the last record in block
            if ((memcmp(buffer, "\xff\xff\xff\xff\xff\xff", 6)) == 0) {
                //calc our own rtable
                double b1 = (cpos-(dtable * 2048));
                int block = ceil(b1 / 4 / 512);
                dirent.rtable = block * 512;
            }

            memset(buffer, 0, 32);
            xbfseek(xiso, (OFFT)cpos,SEEK_SET);
        }
    }
    //end xiso compat

    //check for dirs with no files
    if (dirent.fnamelen) {
        if ((dirent.attribs & FILE_DIR)) {
            procdir(&dirent);

            handlefile((OFFT)dirent.sector * 2048, dirent.sector);

#ifdef _WIN32
            SetCurrentDirectory("..");
#else
            chdir("..");
#endif
        } else {
            // Not a directory - just extract it
            extract(&dirent);
        }
        free(dirent.fname);

        if (dirent.rtable)
            handlefile((OFFT)dtable * 2048 + (dirent.rtable * 4), dtable);
        if (dirent.ltable)
            handlefile((OFFT)dtable * 2048 + (dirent.ltable * 4), dtable);
    }
}


void err(char *error)
{
    if (help == 1) {
        printf("\n \
        xbiso ver %s %s, Copyright (C) 2003 Tonto Rostenfaunt \n \
        xbiso comes with ABSOLUTELY NO WARRANTY; \n \
        This is free software, and you are welcome to redistribute \n \
        it under certain conditions; See the GNU General Public \n \
        License for more details. \n \
        \n\n", version,platform);

        printf("%s filename [options]\n",error);
        printf("        -v    verbose\n");
        printf("        -x    enable xiso <=1.10 compat\n");
        printf("        -d    (dirname)   directory to extract to\n");
        exit(0);
    }

    printf("%s", error);
    exit(1);
}

void procdir(TABLE *dirent)
{
    printf("Creating directory %s\n", dirent->fname);
#ifdef _WIN32
        CreateDirectory(dirent->fname, NULL);
        SetCurrentDirectory(dirent->fname);
#else
        if (mkdir(dirent->fname, 0755))
            err("Failed to create directory.\n");
        chdir(dirent->fname);
#endif
}
