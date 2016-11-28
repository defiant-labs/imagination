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

#define _LARGE_FILES        // if it's not supported the tool will work
#define __USE_LARGEFILE64   // without support for large files
#define __USE_FILE_OFFSET64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS   64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <zlib.h>
#include "des.h"
#include "LzmaDec.h"
#include "Bra.h"

#ifdef __DJGPP__    // thanx to Robert Riebisch http://www.bttr-software.de
    #define NOLFS
    char **__crt0_glob_function (char *arg) { return 0; }
    void   __crt0_load_environment_file (char *progname) { }
#endif

#ifdef WIN32
    #include <windows.h>
    HWND    mywnd;
    char *get_file(void);
    char *put_file(char *suggested);
#else
    #define stricmp strcasecmp
#endif

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef int32_t     i32;
typedef uint32_t    u32;
typedef int64_t     i64;
typedef uint64_t    u64;



#define VER         "0.1.7c"
#define BBIS_SIGN   0x73696262
#define BLHR_SIGN   0x72686c62
#define BSDR_SIGN   0x72647362
#define BLMS_SIGN   0x736d6c62
#define BLSS_SIGN   0x73736c62
#define OUT_ISO     0
#define OUT_CUE     1
#define OUT_MDS     2
#define OUT_CCD     3
#define OUT_NRG     4

#define CREATECUE   // creates an additional and "experimental" CUE file which can be used also on systems where are not available programs for the other formats
#define NRGFIX      // calls nrg_truncate at the end of the process

#define PRINTF64(x) (u32)(((x) >> 32) & 0xffffffff), (u32)((x) & 0xffffffff)    // I64x, llx? blah
#if defined(_LARGE_FILES)
    #if defined(__APPLE__)
        #define fseek   fseeko
        #define ftell   ftello
    #elif defined(__FreeBSD__)
    #elif !defined(NOLFS)       // use -DNOLFS if this tool can't be compiled on your OS!
        #define off_t   off64_t
        #define fopen   fopen64
        #define fseek   fseeko64
        #define ftell   ftello64
    #endif
#endif



#pragma pack(1)

typedef struct {
    u8      id[4];
    u32     size;
} nrg_chunk_t;

typedef struct {
    u32     sign;       // blhr
    u32     size;       // size of the data plus ver and num
    u32     compressed; // compressed data
    u32     num;        // number of blhr_data structures
} blhr_t;

typedef struct {
    u64     offset;     // input offset
    u32     zsize;      // block size
    u32     sector;     // where to place the output
    u32     size;       // size in sectors!
    u32     type;       // type
} blhr_data_t;

typedef struct {
    u32     sign;       // bbis
    u32     bbis_size;  // ignored, probably the size of the structure
    u16     ver;        // version, 1
    u16     image_type; // 8 for ISO, 9 for mixed
    u16     unknown1;   // ???
    u16     padding;    // ignored
    u32     sectors;    // number of sectors of the ISO
    u32     sectorsz;   // CD use sectors and this is the size of them (chunks)
    u32     lastdiff;   // almost ignored
    u64     blhr;       // where is located the blhr header
    u32     blhrbbissz; // total size of the blhr and bbis headers
    u8      hash[16];   // hash, used with passwords
    u8      key1;
    u8      key2;
    u8      key3;
    u8      key4;
    u32     unknown4;   // ignored
} bbis_t;

#pragma pack()



int     endian;



void myfree(u8 **mem);
u8 *path2fname(u8 *path);
u8 *frames2time(u64 num);
void nrg2cue(FILE *fd, u64 nrgoff, u8 *fileo);
void magiciso_is_invalid(FILE *fd, u64 nrgoff, u8 *fileo);
void nrg_truncate(u8 *fileo, int secsz);
u8 *blhr_unzip(FILE *fd, z_stream *z, CLzmaDec *lzma, des_context *des_ctx, u32 zsize, u32 unzsize, int compressed);
u8 *change_ext(u8 *fname, u8 *ext);
FILE *open_file(u8 *fname, int write);
int blms2cue(FILE *fd, u8 *fname, u8 *blms, int blms_len);
void uif_crypt_key(des_context **des_ctx, u8 *key, u8 *pwd);
void uif_crypt(des_context *des_ctx, u8 *data, int size);
u8 *show_hash(u8 *hash);
void myalloc(u8 **data, unsigned wantsize, unsigned *currsize);
void myfr(FILE *fd, void *data, unsigned size);
void myfw(FILE *fd, void *data, unsigned size);
u32 unlzma(CLzmaDec *lzma, u8 *in, u32 insz, u8 *out, u32 outsz);
u32 unzip(z_stream *z, u8 *in, u32 insz, u8 *out, u32 outsz);
void l2n_blhr(blhr_t *p);
void l2n_blhr_data(blhr_data_t *p);
void l2n_bbis(bbis_t *p);
void l2n_16(u16 *num);  // from little endian to number
void l2n_32(u32 *num);
void l2n_64(u64 *num);
void b2n_16(u16 *num);  // from big endian to number
void b2n_32(u32 *num);
void b2n_64(u64 *num);
int getxx(u8 *data, u64 *ret, int bits, int intnet);
int putxx(u8 *data, u64 num, int bits, int intnet);
void std_err(void);
int fgetz(u8 *data, int size, FILE *fd);
void myexit(void);
#include "magiciso_is_shit.h"

static void *SzAlloc(void *p, size_t size) { return(malloc(size)); }
static void SzFree(void *p, void *address) { free(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };



magiciso_is_shit_ctx_t  *shit_key_blhr  = NULL,
                        *shit_key_data  = NULL;
static const u8 *fixedkeys[] = {
        "FAngS)snOt32",  "AglEy29TarsI;",     "bum87OrYx*THeCa", "ARMy67sabot&",
        "FoOTs:blaZe70", "panIc+elD%self79",  "Usnea*hest98",    "apex(TUft!BLOKE70",
        "sword18rEpP}",  "ARb10naVY=Rouse",   "PAIR18gAG:swAYs", "gums}Box73yANg",
        "naVal45drain]", "Cams42hEt83faiLs)", "rIaNt$Notch5",    "ExaCT&art*MEteS47",
        NULL
};



int main(int argc, char *argv[]) {
    des_context *des_ctx = NULL;
    CLzmaDec    *lzma   = NULL;
    z_stream    *z      = NULL;
    blhr_data_t *blhr_data;
    blhr_t  blhr,
            blms,       // both blms and blss have a structure very similar to blhr so I have decided
            blss;       // to use the same type for avoiding to create other functions
    bbis_t  bbis;
    FILE    *fdi        = NULL,
            *fdo        = NULL,
            *fdx        = NULL;
    u64     file_size,
            tmp,
            tot;
    u32     i,
            insz,
            outsz,
            lastdiff    = 0;
    int     outtype     = OUT_ISO;
    u8      ans[130],   // password is max 31 chars
            pwdkey[32], // password is max 31 chars
            tmphash[16],
            *blms_data  = NULL,
            *blss_data  = NULL,
            *outext     = NULL,
            *filei      = NULL,
            *fileo      = NULL,
            *filec      = NULL,
            *in         = NULL,
            *out        = NULL,
            *p;

    setbuf(stdout, NULL);

    fputs("\n"
        "UIF2ISO "VER"\n"
        "by Luigi Auriemma\n"
        "e-mail: aluigi@autistici.org\n"
        "web:    aluigi.org\n"
        "\n", stdout);

    endian = 1;                         // big endian
    if(*(char *)&endian) endian = 0;    // little endian

#ifdef WIN32
    mywnd = GetForegroundWindow();
    if(GetWindowLong(mywnd, GWL_WNDPROC)) {
        p = argv[1];
        argv = malloc(sizeof(char *) * 3);
        if(argc < 2) {
            argv[1] = get_file();
        } else {
            argv[1] = p;
        }
        argv[2] = put_file(argv[1]);
        argc = 3;
    }
#endif

    if(argc < 3) {
        printf("\n"
            "Usage: %s <input.UIF> <output>\n"
            "\n"
            "The output ISO,CUE/BIN,MDS/MDS,CCD,NRG extension is selected by this tool\n"
            "\n", argv[0]);
        myexit();
    }

    filei   = argv[1];
    fileo   = argv[2];

    fdi = open_file(filei, 0);

    fseek(fdi, 0, SEEK_END);
    file_size = ftell(fdi);
    if(fseek(fdi, file_size - sizeof(bbis), SEEK_SET)) {
        if(((file_size - sizeof(bbis)) > 0x7fffffff) && ((file_size - sizeof(bbis)) < file_size)) printf("  an error here means that your exe has no full LARGE_FILES 64 bit support!\n");
        std_err();
    }
    myfr(fdi, &bbis, sizeof(bbis));
redo_bbis:
    l2n_bbis(&bbis);
    if(bbis.sign != BBIS_SIGN) {
        printf("\nError: wrong bbis signature (%08x)\n", bbis.sign);
        myexit();
    }
    if(des_ctx) bbis.blhr += sizeof(bbis) + 8;

    printf("\n"
        "  file size    %08x%08x\n"
        "  version      %hu\n"
        "  image type   %hu\n"
        "  padding      %hu\n"
        "  sectors      %u\n"
        "  sectors size %u\n"
        "  blhr offset  %08x%08x\n"
        "  blhr size    %u\n"
        "  hash         %s\n"
        "  others       %08x %08x %02x %02x %02x %02x %08x\n"
        "\n",
        PRINTF64(file_size),
        bbis.ver,
        bbis.image_type,
        bbis.padding,
        bbis.sectors,
        bbis.sectorsz,
        PRINTF64(bbis.blhr),
        bbis.blhrbbissz,
        show_hash(bbis.hash),
        bbis.bbis_size, bbis.lastdiff, bbis.key1, bbis.key2, bbis.key3, bbis.key4, bbis.unknown4);

    if((bbis.ver < 2) || ((bbis.key2 != 2) && (bbis.key1 > 0x10))) {
        printf("- disable any encryption\n");
        bbis.key1 = 0;
    }
    if(bbis.key2 == 2) {
        printf("- enable magiciso_is_shit encryption\n");
        shit_key_blhr = calloc(1, sizeof(magiciso_is_shit_ctx_t));
        shit_key_data = calloc(1, sizeof(magiciso_is_shit_ctx_t));
        if(!shit_key_blhr || !shit_key_data) std_err();

        shit_key_blhr->flag = (bbis.key3 != 1) ? 0xff : bbis.key1;
        magiciso_is_shit_key(shit_key_blhr, bbis.key3);

        shit_key_data->flag = (bbis.key3 != 0x0f) ? bbis.key1 : 0;
        magiciso_is_shit_key(shit_key_data, bbis.key3);
    } else if(bbis.key1) {
        printf("- enable fixedkey encryption\n");
        uif_crypt_key(&des_ctx, pwdkey, (u8 *)fixedkeys[(bbis.key1 - 1) & 0x0f]);
        des_setkey_dec(des_ctx, pwdkey);
    }

    if(fseek(fdi, bbis.blhr, SEEK_SET)) std_err();
    myfr(fdi, &blhr, sizeof(blhr));
    l2n_blhr(&blhr);
    if(blhr.sign != BLHR_SIGN) {
        if(blhr.sign == BSDR_SIGN) {
            printf("- the input file is protected by password, insert it: ");
            fgetz(ans, sizeof(ans), stdin);
            if(strlen(ans) > 31) ans[31] = 0;

            uif_crypt_key(&des_ctx, pwdkey, ans);
            des_setkey_dec(des_ctx, pwdkey);

            if(blhr.size != sizeof(bbis)) {
                printf("- Alert: the size of the bbis struct and the one specified by bsdr don't match\n");
            }
            fseek(fdi, -8, SEEK_CUR);
            memcpy(tmphash, bbis.hash, sizeof(bbis.hash));
            myfr(fdi, &bbis, sizeof(bbis));
            uif_crypt(des_ctx, (u8 *)&bbis, sizeof(bbis));
            memcpy(bbis.hash, tmphash, sizeof(bbis.hash));
            goto redo_bbis;
        } else {
            printf("\nError: wrong blhr signature (%08x)\n", blhr.sign);
        }
        myexit();
    }

    if(bbis.ver < 3) {
        z = malloc(sizeof(z_stream));
        if(!z) std_err();
        z->zalloc = (alloc_func)0;
        z->zfree  = (free_func)0;
        z->opaque = (voidpf)0;
        if(inflateInit2(z, 15)) {
            printf("\nError: zlib initialization error\n");
            myexit();
        }
    } else {
        lzma = malloc(sizeof(CLzmaDec));
        if(!lzma) std_err();
    }
    blhr_data = (blhr_data_t *)blhr_unzip(fdi, z, lzma, des_ctx, blhr.size - 8, sizeof(blhr_data_t) * blhr.num, blhr.compressed);

    if(bbis.image_type == 8) {
        // nothing to do
    } else if(bbis.image_type == 9) {
        printf("- raw or mixed type image\n");

        myfr(fdi, &blms, sizeof(blms));
        l2n_blhr(&blms);
        if(blms.sign != BLMS_SIGN) {
            printf("- Alert: wrong blms signature (%08x)\n", blms.sign);
        } else {
            blms_data = blhr_unzip(fdi, z, lzma, des_ctx, blms.size - 8, blms.num, blms.compressed);

            myfr(fdi, &blss, sizeof(blss));
            myfr(fdi, &outtype, 4);
            l2n_blhr(&blss);
            l2n_32(&outtype);
            if(blss.sign != BLSS_SIGN) {
                printf("- Alert: wrong blss signature (%08x)\n", blss.sign);
            } else {
                if(blss.num) {
                    blss_data = blhr_unzip(fdi, z, lzma, des_ctx, blss.size - 12, blss.num, blss.compressed);
                }
            }
        }
    } else {
        printf("- Alert: this type of image (%hu) is not supported by this tool, I try as ISO\n", bbis.image_type);
    }

    in   = out   = NULL;
    insz = outsz = 0;
    tot  = 0;

    switch(outtype) {
        case OUT_ISO: {
            printf("- ISO output image format\n");
            outext = ".iso";
            break;
        }
        case OUT_CUE: {
            printf("- BIN/CUE output image format\n");
            outext = ".bin";
            break;
        }
        case OUT_MDS: {
            printf("- MDS output image format\n");
            outext = ".mdf";
            break;
        }
        case OUT_CCD: {
            printf("- CCD (CloneCD) output image format\n");
            outext = ".img";
            break;
        }
        case OUT_NRG: {
            printf("- NRG (Nero v2) output image format\n");
            outext = ".nrg";
            break;
        }
        default: {
            printf("\nError: the output image %u is not supported by this tool, contact me\n", outtype);
            myexit();
            break;
        }
    }
    fileo = change_ext(fileo, outext);
    fdo = open_file(fileo, 1);

    #ifdef CREATECUE
    if(outtype != OUT_ISO) {    // I generate a CUE file in ANY case for maximum compatibility
        printf("- generate an \"experimental\" CUE file for more compatibility on various systems\n");
        filec = change_ext(fileo, "_uif2iso.cue");
        fdx = open_file(filec, 1);
        blms2cue(fdx, path2fname(fileo), blms_data, blms.num);
        fclose(fdx);
        myfree(&filec);
    }
    #endif

    if(blss_data) {
        switch(outtype) {
            case OUT_ISO: {
                break;
            }
            case OUT_CUE: {
                filec = change_ext(fileo, ".cue");
                fdx = open_file(filec, 1);
                myfw(fdx, blss_data, blss.num);
                fclose(fdx);
                myfree(&filec);
                break;
            }
            case OUT_MDS: {
                filec = change_ext(fileo, ".mds");
                fdx = open_file(filec, 1);
                myfw(fdx, blss_data, blss.num);
                fclose(fdx);
                myfree(&filec);
                break;
            }
            case OUT_CCD: {
                p = blss_data + 8;
                getxx(blss_data, &tmp, 32, 0);

                filec = change_ext(fileo, ".ccd");
                fdx = open_file(filec, 1);
                myfw(fdx, p, tmp);
                fclose(fdx);
                myfree(&filec);

                p += tmp;
                getxx(blss_data + 4, &tmp, 32, 0);

                filec = change_ext(fileo, ".sub");
                fdx = open_file(filec, 1);
                myfw(fdx, p, tmp);
                fclose(fdx);
                myfree(&filec);
                break;
            }
            case OUT_NRG: {
                break;
            }
            default: break; // handled previously
        }
    }

    printf("- start unpacking:\n");
    for(i = 0; i < blhr.num; i++) {
        l2n_blhr_data(&blhr_data[i]);

        printf("  %03d%%\r", (i * 100) / blhr.num);

        #ifdef VERBOSE
        printf("\n"
            "offset        %08x%08x\n"
            "input size    %08x\n"
            "output sector %08x\n"
            "sectors       %08x\n"
            "type          %08x\n",
            PRINTF64(blhr_data[i].offset),
            blhr_data[i].zsize,
            blhr_data[i].sector,
            blhr_data[i].size,
            blhr_data[i].type);
        #endif

        myalloc(&in, blhr_data[i].zsize, &insz);

        if(blhr_data[i].zsize) {
            if(fseek(fdi, blhr_data[i].offset, SEEK_SET)) std_err();
            myfr(fdi, in, blhr_data[i].zsize);
            magiciso_is_shit_dec(shit_key_data, in, blhr_data[i].zsize);
            uif_crypt(des_ctx, in, blhr_data[i].zsize);
        }

        if(bbis.lastdiff && ((blhr_data[i].sector + blhr_data[i].size) >= bbis.sectors)) {
            lastdiff = bbis.sectorsz - bbis.lastdiff;
        }

        blhr_data[i].size *= bbis.sectorsz;
        myalloc(&out, blhr_data[i].size, &outsz);

        switch(blhr_data[i].type) {
            case 1: {   // non compressed
                if(blhr_data[i].zsize > blhr_data[i].size) {
                    printf("\nError: input size is bigger than output\n");
                    myexit();
                }
                memcpy(out, in, blhr_data[i].zsize);
                if(blhr_data[i].zsize < blhr_data[i].size) {
                    memset(out + blhr_data[i].zsize, 0, blhr_data[i].size - blhr_data[i].zsize); // needed?
                }
                break;
            }
            case 3: {   // multi byte
                memset(out, 0, blhr_data[i].size);
                break;
            }
            case 5: {   // compressed
                unlzma(lzma, in, blhr_data[i].zsize, out, blhr_data[i].size);
                unzip(z,     in, blhr_data[i].zsize, out, blhr_data[i].size);
                break;
            }
            default: {
                printf("\nError: unknown type (%d)\n", blhr_data[i].type);
                myexit();
            }
        }

        if(fseek(fdo, (u64)blhr_data[i].sector * (u64)bbis.sectorsz, SEEK_SET)) std_err();
        if(lastdiff) {
            blhr_data[i].size -= lastdiff;  // remove the superflous bytes at the end
            lastdiff = 0;
        }
        myfw(fdo, out, blhr_data[i].size);
        tot += blhr_data[i].size;
    }

    printf("  100%%\n"
        "- 0x%08x%08x bytes written\n", PRINTF64(tot));

    fclose(fdi);
    fclose(fdo);
    if(z) {
        inflateEnd(z);
        myfree((void *)&z);
    }
    if(lzma) {
        myfree((void *)&lzma);
    }
    magiciso_is_shit_free(shit_key_blhr);
    magiciso_is_shit_free(shit_key_data);
    if(des_ctx) myfree((void *)&des_ctx);

    if(outtype == OUT_NRG) {
        printf(
            "\n"
            "  Please keep in mind that MagicISO creates INVALID NRG files which not only\n"
            "  are unreadable by the various burners/mounters/converters for this type of\n"
            "  image but also by the same Nero which owns the original NRG format, so if the\n"
            "  output NRG file doesn't work is enough \"normal\".\n"
            "\n");
        #ifdef NRGFIX
        printf(
            "  This is the reason why this tool will create an additional CUE file which can\n"
            "  be used in case the NRG one doesn't work. If you are trying to mount the CUE\n"
            "  file but it gives errors or you see no data try to enable all the emulation\n"
            "  options of your mounting program and it will work perfectly.\n"
            "\n");
        nrg_truncate(fileo, bbis.sectorsz * 2); // max 1 sector plus another one if NER5 is not in the last one
        #endif
    }

    printf("- finished\n");
    myexit();
    return(0);
}



#ifdef WIN32
char *get_file(void) {
    OPENFILENAME    ofn;
    static char     filename[4096];
    static const char   filter[] =
                    "UIF file\0"    "*.uif\0"
                    "(*.*)\0"       "*.*\0"
                    "\0"            "\0";

    filename[0] = 0;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize     = sizeof(ofn);
    ofn.lpstrFilter     = filter;
    ofn.nFilterIndex    = 1;
    ofn.lpstrFile       = filename;
    ofn.nMaxFile        = sizeof(filename);
    ofn.lpstrTitle      = "select the input UIF file to convert";
    ofn.Flags           = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_EXPLORER | OFN_HIDEREADONLY;

    printf("- %s\n", ofn.lpstrTitle);
    if(!GetOpenFileName(&ofn)) exit(1);
    return(filename);
}

char *put_file(char *suggested) {
    OPENFILENAME    ofn;
    char            *p;
    static char     filename[4096 + 10];
    static const char   filter[] =
                    "image file\0"  "*.iso;*.cue;*.bin;*.mds;*.mdf;*.ccd;*.img;*.sub;*.nrg\0"
                    "(*.*)\0"       "*.*\0"
                    "\0"            "\0";

    if(!suggested) {
        filename[0] = 0;
    } else {
        p = strrchr(suggested, '.');
        if(!p) p = suggested + strlen(suggested);
        if((p - suggested) >= 4096) p = suggested + 4096 - 1;
        memcpy(filename, suggested, p - suggested);
        filename[p - suggested] = 0;
    }
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize     = sizeof(ofn);
    ofn.lpstrFilter     = filter;
    ofn.nFilterIndex    = 1;
    ofn.lpstrFile       = filename;
    ofn.nMaxFile        = sizeof(filename);
    ofn.lpstrTitle      = "choose the name of the output file to create (the extension is automatic)";
    ofn.Flags           = OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_EXPLORER | OFN_HIDEREADONLY;

    printf("- %s\n", ofn.lpstrTitle);
    if(!GetSaveFileName(&ofn)) exit(1);
    return(filename);
}
#endif



void myfree(u8 **mem) {
    if(*mem) {
        free(*mem);
        *mem = NULL;
    }
}



u8 *path2fname(u8 *path) {
    u8      *p;

    p = strrchr(path, '\\');
    if(!p) p = strrchr(path, '/');
    if(p) return(p + 1);
    return(path);
}



u8 *frames2time(u64 num) {
    int     mm,
            ss,
            ff;
    static u8   ret_time[32];

    num /= 2352;    // default sector size
    ff = num % 75;
    ss = (num / 75) % 60;
    mm = (num / 75) / 60;
    sprintf(ret_time, "%02d:%02d:%02d", mm, ss, ff);
    return(ret_time);
}



void nrg2cue(FILE *fd, u64 nrgoff, u8 *fileo) {
    nrg_chunk_t chunk;
    FILE    *fdcue      = NULL,
            *fdcdt      = NULL;
    u64     index0,
            index1,
            index2;
    u32     sectsz,
            mode;
    int     i,
            numsz,
            track,
            firstindex  = 1;
    u8      *filec      = NULL,
            *filecdt    = NULL,
            *buff       = NULL,
            *p,
            *l;

    if(fseek(fd, nrgoff, SEEK_SET)) {
        printf("- Alert: wrong NRG header offset\n");
        return;
    }

    printf("- generate a new CUE file derived from the NRG one\n");
    filec = change_ext(fileo, "_nrg.cue");
    fdcue = open_file(filec, 1);
    fprintf(fdcue, "FILE \"%s\" BINARY\r\n", path2fname(fileo));

    track = 1;
    for(;;) {   // get tracks and do more things
        myfr(fd, &chunk, sizeof(chunk));
        b2n_32(&chunk.size);
        if(!memcmp(chunk.id, "NER5", 4) || !memcmp(chunk.id, "NERO", 4)) break;
        if(!memcmp(chunk.id, "DAOX", 4) || !memcmp(chunk.id, "DAOI", 4)) {
            if(chunk.size >= 22) {
                buff = malloc(chunk.size);
                if(!buff) std_err();
                myfr(fd, buff, chunk.size);

                numsz = (!memcmp(chunk.id, "DAOX", 4)) ? 8 : 4;

                p = buff + 22;
                l = buff + chunk.size - (4 + 4 + (numsz * 3));
                for(i = 0; p <= l; i++) {
                    p += 10;
                    sectsz = *(u32 *)p; p += 4;
                    b2n_32(&sectsz);
                    mode   = p[0];      p += 4;
                    p += getxx(p, &index0, numsz << 3, 1);
                    p += getxx(p, &index1, numsz << 3, 1);
                    p += getxx(p, &index2, numsz << 3, 1);
                    #ifdef VERBOSE
                    printf("  %08x %02x %08x%08x %08x%08x %08x%08x\n", sectsz, mode, PRINTF64(index0), PRINTF64(index1), PRINTF64(index2));
                    #endif
                    switch(mode) {
                        // case 2: yes, this is data mode 2 BUT the CUE will give an error if you use this mode!
                        case 3: fprintf(fdcue, "    TRACK %02d MODE2/%u\r\n", track, sectsz); break;
                        case 7: fprintf(fdcue, "    TRACK %02d AUDIO\r\n",    track);         break;
                        case 0:
                        default:fprintf(fdcue, "    TRACK %02d MODE1/%u\r\n", track, sectsz); break;
                    }
                    if(firstindex) {
                        fprintf(fdcue, "        INDEX 00 00:00:00\r\n");
                        firstindex = 0;
                    } else if(index1 > index0) {
                        fprintf(fdcue, "        INDEX 00 %s\r\n", frames2time(index0));
                    }
                    fprintf(fdcue, "        INDEX 01 %s\r\n", frames2time(index1));
                    track++;
                }

                myfree(&buff);
                continue;
            }
        }
        if(!memcmp(chunk.id, "ETN2", 4) || !memcmp(chunk.id, "ETNF", 4)) {
            if(chunk.size >= 22) {
                buff = malloc(chunk.size);
                if(!buff) std_err();
                myfr(fd, buff, chunk.size);

                numsz = (!memcmp(chunk.id, "ETN2", 4)) ? 8 : 4;

                sectsz = 2352;  // right???
                p = buff;
                l = buff + chunk.size - ((numsz * 2) + 4 + 4 + 4);
                for(i = 0; p <= l; i++) {
                    p += getxx(p, &index1, numsz << 3, 1);
                    p += getxx(p, &index2, numsz << 3, 1);
                    mode   = p[0];      p += 4;
                    p += 4 + 4;
                    #ifdef VERBOSE
                    printf("  %02x %08x%08x %08x%08x\n", mode, PRINTF64(index1), PRINTF64(index2));
                    #endif
                    switch(mode) {
                        case 3: fprintf(fdcue, "    TRACK %02d MODE2/%u\r\n", track, sectsz); break;
                        case 7: fprintf(fdcue, "    TRACK %02d AUDIO\r\n",    track);         break;
                        case 0:
                        default:fprintf(fdcue, "    TRACK %02d MODE1/%u\r\n", track, sectsz); break;
                    }
                    if(!i) fprintf(fdcue, "        INDEX 00 00:00:00\r\n");
                    fprintf(fdcue, "        INDEX 01 %s\r\n", frames2time(index1));
                    track++;
                }

                myfree(&buff);
                continue;
            }
        }
        if(!memcmp(chunk.id, "CDTX", 4)) {
            buff = malloc(chunk.size);
            if(!buff) std_err();
            myfr(fd, buff, chunk.size);

            filecdt = change_ext(fileo, ".cdt");
            fdcdt = open_file(filecdt, 1);
            myfw(fdcdt, buff, chunk.size);
            fclose(fdcdt);

            fprintf(fdcue, "CDTEXTFILE \"%s\"\r\n", path2fname(filecdt));
            myfree(&buff);
            continue;
        }
        if(fseek(fd, chunk.size, SEEK_CUR)) break;
    }
    fclose(fdcue);
}



void magiciso_is_invalid(FILE *fd, u64 nrgoff, u8 *fileo) {
    nrg_chunk_t chunk;
    u64     index2;
    int     numsz,
            track;
    u8      tracks, // can't be more than 8bit
            *buff   = NULL;

    if(fseek(fd, nrgoff, SEEK_SET)) {
        printf("- Alert: wrong NRG header offset\n");
        return;
    }

    track = 1;
    tracks = 1;
    for(;;) {   // get tracks and do more things
        myfr(fd, &chunk, sizeof(chunk));
        b2n_32(&chunk.size);
        if(!memcmp(chunk.id, "NER5", 4) || !memcmp(chunk.id, "NERO", 4)) break;
        if(!memcmp(chunk.id, "DAOX", 4) || !memcmp(chunk.id, "DAOI", 4)) {
            if(chunk.size >= 22) {
                buff = malloc(chunk.size);
                if(!buff) std_err();
                myfr(fd, buff, chunk.size);

                tracks = (buff[21] - buff[20]) + 1;
                numsz = (!memcmp(chunk.id, "DAOX", 4)) ? 8 : 4;
                getxx(buff + chunk.size - numsz, &index2, numsz << 3, 1);
                if(index2 > nrgoff) {
                    putxx(buff + chunk.size - numsz, nrgoff, numsz << 3, 1);
                    printf("- correcting last DAO index2\n");
                    fseek(fd, -numsz, SEEK_CUR);
                    myfw(fd, buff + chunk.size - numsz, numsz);
                    fflush(fd); // you can't imagine how much required is this fflush...
                }

                myfree(&buff);
                continue;   // skip the fseek chunk.size stuff made at each cycle
            }
        }
        if(!memcmp(chunk.id, "SINF", 4)) {  // usually located after DAO
            if(chunk.size >= 4) {
                if(fseek(fd, 3, SEEK_CUR)) break;
                printf("- correcting SINF to %u tracks\n", tracks);
                myfw(fd, &tracks, 1);
                fflush(fd); // you can't imagine how much required is this fflush...
                fseek(fd, -4, SEEK_CUR);    // restore
            }
        }
        if(fseek(fd, chunk.size, SEEK_CUR)) break;
    }
}



void nrg_truncate(u8 *fileo, int secsz) {
    FILE    *fd     = NULL;
    u64     truncsize,
            realsize,
            nrgoff;
    int     truncseek;
    u8      *buff   = NULL,
            *p;

    fd = fopen(fileo, "r+b");
    if(!fd) return;

    fflush(fd);
    fseek(fd, 0, SEEK_END);
    realsize = ftell(fd);

    if(!fseek(fd, -secsz, SEEK_END)) {
        buff = malloc(secsz);
        if(!buff) std_err();
        myfr(fd, buff, secsz);
        for(p = buff + secsz - 12; p >= buff; p--) {
            if(!memcmp(p, "NER5", 4)) {
                nrgoff = *(u64 *)(p + 4);
                p += 12;
                break;
            }
            if(!memcmp(p, "NERO", 4)) {
                nrgoff = *(u32 *)(p + 4);
                p += 8;
                break;
            }
        }
        if(p >= buff) {
            truncseek = -(secsz - (p - buff));

            b2n_64(&nrgoff);
            magiciso_is_invalid(fd, nrgoff, fileo);
            nrg2cue(fd, nrgoff, fileo);

            fseek(fd, truncseek, SEEK_END);
            fflush(fd);
            truncsize = ftell(fd);
            if(realsize != truncsize) {
                printf("- found NRG end of file at offset 0x%08x%08x\n", PRINTF64(truncsize));
                ftruncate(fileno(fd), truncsize);   // trick to spawn errors or warnings if there is no large file support
                fflush(fd);
                fclose(fd);

                fd = fopen(fileo, "rb");    // verify if the truncation was correct
                if(!fd) return;
                fseek(fd, 0, SEEK_END);
                realsize = ftell(fd);
                if(realsize < truncsize) {
                    printf("\n"
                        "Error: the truncated file is smaller than how much I requested\n"
                        "       is possible that ftruncate() doesn't support large files\n"
                        "       Please contact me reporting also the sizeo of the UIF and your platform\n");
                } else if(realsize != truncsize) {
                    printf("- Alert: seems that the file has not been truncated to the correct NRG size\n");
                }
            }
        }
        myfree(&buff);
    }
    fclose(fd);
}



u8 *blhr_unzip(FILE *fd, z_stream *z, CLzmaDec *lzma, des_context *des_ctx, u32 zsize, u32 unzsize, int compressed) {
    static int  insz = 0;
    static u8   *in  = NULL;
    u8          *data;

    if(compressed) {
        myalloc(&in, zsize, &insz);
        myfr(fd, in, zsize);
        magiciso_is_shit_dec(shit_key_blhr, in, zsize);
        if(des_ctx) uif_crypt(des_ctx, in, zsize);
        data = malloc(unzsize);
        if(!data) std_err();
        unlzma(lzma, in, zsize, (u8 *)data, unzsize);
        unzip(z, in, zsize, (u8 *)data, unzsize);
    } else {
        data = malloc(unzsize);
        if(!data) std_err();
        myfr(fd, data, unzsize);
        magiciso_is_shit_dec(shit_key_blhr, data, unzsize);
    }
    return(data);
}



u8 *change_ext(u8 *fname, u8 *ext) {
    u8      *p;

    p = malloc(strlen(fname) + strlen(ext) + 1);
    if(!p) std_err();
    strcpy(p, fname);
    fname = p;
    p = strrchr(fname, '.');
    if(!p || (p && (strlen(p) != 4))) p = fname + strlen(fname);
    strcpy(p, ext);
    return(fname);
}



FILE *open_file(u8 *fname, int write) {
    FILE    *fd     = NULL;
    u8      ans[16];

    if(write) {
        printf("- create %s\n", fname);
        fd = fopen(fname, "rb");
        if(fd) {
            fclose(fd);
            printf("- the output file already exists, do you want to overwrite it (y/N)? ");
            fgetz(ans, sizeof(ans), stdin);
            if((ans[0] != 'y') && (ans[0] != 'Y')) myexit();
        }
        fd = fopen(fname, "wb");
        if(!fd) std_err();
    } else {
        printf("- open %s\n", fname);
        fd = fopen(fname, "rb");
        if(!fd) std_err();
    }
    return(fd);
}



int blms2cue(FILE *fd, u8 *fname, u8 *blms, int blms_len) {
    u32     bin,
            cue,
            type;
    int     track,
            mode,
            tot;
    u8      mm,
            ss,
            ff,
            *p;

    if(blms_len < 0x40) return(-1);

    bin = *(u32 *)(blms + 0x04);
    l2n_32(&bin);
    if(bin > blms_len) return(-1);

    cue = *(u32 *)(blms + 0x10);
    l2n_32(&cue);
    if(cue > blms_len) return(-1);

    p = blms + 0x40;
    if(bin) {
        p = blms + bin;
        printf("- BIN name stored in the UIF file: %s\n", p);
        p += strlen(p) + 1;
    }
    if(cue) {
        p = blms + cue;
        printf("- CUE name stored in the UIF file: %s\n", p);
        p += strlen(p) + 1;
    }

    fprintf(fd, "FILE \"%s\" BINARY\r\n", fname);

    for(tot = 0; (p - blms) < blms_len; p += 68) {
        if(p[3] & 0xa0) continue;
        track = p[3];
        mode  = p[11];
        mm    = p[8];
        ss    = p[9] - 2;   // these are the 2 seconds located at the beginning of the NRG file
        ff    = p[10];
        type = *(u32 *)(p + 24);
        l2n_32(&type);
        switch(p[1]) {
            case 0x10:
            case 0x12: {
                fprintf(fd, "    TRACK %02d AUDIO\r\n", track);
                } break;
            case 0x14:
            default: {
                fprintf(fd, "    TRACK %02d MODE%d/%d\r\n", track, mode, type);
                } break;
        }
        fprintf(fd, "        INDEX %02d %02d:%02d:%02d\r\n", 1, mm, ss, ff);
        tot++;
    }

    return(tot);
}



void uif_crypt_key(des_context **des_ctx, u8 *key, u8 *pwd) {
    i64     *k,
            a,
            b;
    int     i;

    if(!des_ctx) return;
    if(!*des_ctx) {
        *des_ctx = malloc(sizeof(des_context));
        if(!*des_ctx) std_err();
    }

    strncpy(key, pwd, 32);
    k = (i64 *)key;

    printf("- set DES encryption key: %.32s\n", key);

    for(i = 1; i < 4; i++) {
        if(!endian) {   // this solution is required for little/big endian compatibility and speed
            k[0] += k[i];
            continue;
        }
        a = k[0];
        b = k[i];
        l2n_64(&a);
        l2n_64(&b);
        a += b;
        l2n_64(&a);
        k[0] = a;
    }

    printf("- DES password: %02x %02x %02x %02x %02x %02x %02x %02x\n",
        key[0], key[1], key[2], key[3], key[4], key[5], key[6], key[7]);
}



void uif_crypt(des_context *des_ctx, u8 *data, int size) {
    u8      *p,
            *l;

    if(!des_ctx) return;
    l = data + size - (size & 7);
    for(p = data; p < l; p += 8) {
        des_crypt_ecb(des_ctx, p, p);
    }
}



u8 *show_hash(u8 *hash) {
    int     i;
    static u8   vis[33];
    static const char hex[16] = "0123456789abcdef";
    u8      *p;

    p = vis;
    for(i = 0; i < 16; i++) {
        *p++ = hex[hash[i] >> 4];
        *p++ = hex[hash[i] & 15];
    }
    *p = 0;

    return(vis);
}



void myalloc(u8 **data, unsigned wantsize, unsigned *currsize) {
    if(wantsize <= *currsize) return;
    *data = realloc(*data, wantsize);
    if(!*data) std_err();
    *currsize = wantsize;
}



void myfr(FILE *fd, void *data, unsigned size) {
    if(fread(data, 1, size, fd) == size) return;
    printf("\nError: incomplete input file, can't read %u bytes\n", size);
    myexit();
}



void myfw(FILE *fd, void *data, unsigned size) {
    if(fwrite(data, 1, size, fd) == size) return;
    printf("\nError: problems during the writing of the output file\n");
    myexit();
}



u32 unlzma(CLzmaDec *lzma, u8 *in, u32 insz, u8 *out, u32 outsz) {
    ELzmaStatus status;
    SizeT   inlen,
            outlen;
    UInt32  x86State;
    int     filter;

    if(!lzma) return(0);
    if(insz < (1 + LZMA_PROPS_SIZE + 8)) {
        printf("\nError: the input lzma block is too short (%u)\n", insz);
        myexit();
    }

    filter = in[0];
    in++;
    insz--;

    LzmaDec_Construct(lzma);
    LzmaDec_Allocate(lzma, in, LZMA_PROPS_SIZE, &g_Alloc);
    LzmaDec_Init(lzma);

    in    += (LZMA_PROPS_SIZE + 8); // the uncompressed size is already known so it must be not read from this 64 bit field
    inlen  = insz - (LZMA_PROPS_SIZE + 8);
    outlen = outsz;

    if(
        (LzmaDec_DecodeToBuf(lzma, out, &outlen, in, &inlen, LZMA_FINISH_END, &status) != SZ_OK)
     || ((status != LZMA_STATUS_FINISHED_WITH_MARK) && (status != LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK))) {
        printf("\nError: the compressed LZMA input is wrong or incomplete (%d)\n", status);
        myexit();
    }
    if(filter) {
        x86_Convert_Init(x86State);
        x86_Convert(out, outlen, 0, &x86State, 0);
    }
    LzmaDec_Free(lzma, &g_Alloc);
    return(outlen);
}



u32 unzip(z_stream *z, u8 *in, u32 insz, u8 *out, u32 outsz) {
    if(!z) return(0);
    inflateReset(z);

    z->next_in   = in;
    z->avail_in  = insz;
    z->next_out  = out;
    z->avail_out = outsz;
    if(inflate(z, Z_SYNC_FLUSH) != Z_STREAM_END) {
        printf("\nError: the compressed input is wrong or incomplete\n");
        myexit();
    }
    return(z->total_out);
}



void l2n_blhr(blhr_t *p) {
    if(!endian) return;
    l2n_32(&p->sign);
    l2n_32(&p->size);
    l2n_32(&p->compressed);
    l2n_32(&p->num);
}



void l2n_blhr_data(blhr_data_t *p) {
    if(!endian) return;
    l2n_64(&p->offset);
    l2n_32(&p->zsize);
    l2n_32(&p->sector);
    l2n_32(&p->size);
    l2n_32(&p->type);
}



void l2n_bbis(bbis_t *p) {
    if(!endian) return;
    l2n_32(&p->sign);
    l2n_32(&p->bbis_size);
    l2n_16(&p->ver);
    l2n_16(&p->image_type);
    l2n_16(&p->unknown1);
    l2n_16(&p->padding);
    l2n_32(&p->sectors);
    l2n_32(&p->sectorsz);
    l2n_32(&p->lastdiff);
    l2n_64(&p->blhr);
    l2n_32(&p->blhrbbissz);
    l2n_32(&p->unknown4);
}



void l2n_16(u16 *num) {
    u16     tmp;

    if(!endian) return;

    tmp = *num;
    *num = ((tmp & 0xff00) >> 8) |
           ((tmp & 0x00ff) << 8);
}



void l2n_32(u32 *num) {
    u32     tmp;

    if(!endian) return;

    tmp = *num;
    *num = ((tmp & 0xff000000) >> 24) |
           ((tmp & 0x00ff0000) >>  8) |
           ((tmp & 0x0000ff00) <<  8) |
           ((tmp & 0x000000ff) << 24);
}



void l2n_64(u64 *num) {
    u64     tmp;

    if(!endian) return;

    tmp = *num;
    *num = (u64)((u64)(tmp & (u64)0xff00000000000000ULL) >> (u64)56) |
           (u64)((u64)(tmp & (u64)0x00ff000000000000ULL) >> (u64)40) |
           (u64)((u64)(tmp & (u64)0x0000ff0000000000ULL) >> (u64)24) |
           (u64)((u64)(tmp & (u64)0x000000ff00000000ULL) >> (u64)8)  |
           (u64)((u64)(tmp & (u64)0x00000000ff000000ULL) << (u64)8)  |
           (u64)((u64)(tmp & (u64)0x0000000000ff0000ULL) << (u64)24) |
           (u64)((u64)(tmp & (u64)0x000000000000ff00ULL) << (u64)40) |
           (u64)((u64)(tmp & (u64)0x00000000000000ffULL) << (u64)56);
}



void b2n_16(u16 *num) {
    u16     tmp;

    if(endian) return;

    tmp = *num;
    *num = ((tmp & 0xff00) >> 8) |
           ((tmp & 0x00ff) << 8);
}



void b2n_32(u32 *num) {
    u32     tmp;

    if(endian) return;

    tmp = *num;
    *num = ((tmp & 0xff000000) >> 24) |
           ((tmp & 0x00ff0000) >>  8) |
           ((tmp & 0x0000ff00) <<  8) |
           ((tmp & 0x000000ff) << 24);
}



void b2n_64(u64 *num) {
    u64     tmp;

    if(endian) return;

    tmp = *num;
    *num = (u64)((u64)(tmp & (u64)0xff00000000000000ULL) >> (u64)56) |
           (u64)((u64)(tmp & (u64)0x00ff000000000000ULL) >> (u64)40) |
           (u64)((u64)(tmp & (u64)0x0000ff0000000000ULL) >> (u64)24) |
           (u64)((u64)(tmp & (u64)0x000000ff00000000ULL) >> (u64)8)  |
           (u64)((u64)(tmp & (u64)0x00000000ff000000ULL) << (u64)8)  |
           (u64)((u64)(tmp & (u64)0x0000000000ff0000ULL) << (u64)24) |
           (u64)((u64)(tmp & (u64)0x000000000000ff00ULL) << (u64)40) |
           (u64)((u64)(tmp & (u64)0x00000000000000ffULL) << (u64)56);
}



int getxx(u8 *data, u64 *ret, int bits, int intnet) {
    u64     num;
    int     i,
            bytes;

    num = 0;
    bytes = bits >> 3;
    for(i = 0; i < bytes; i++) {
        if(!intnet) {   // intel/little endian
            num |= (data[i] << (i << 3));
        } else {        // network/big endian
            num |= (data[i] << ((bytes - 1 - i) << 3));
        }
    }
    *ret = num;
    return(bytes);
}



int putxx(u8 *data, u64 num, int bits, int intnet) {
    int     i,
            bytes;

    bytes = bits >> 3;
    for(i = 0; i < bytes; i++) {
        if(!intnet) {
            data[i] = (num >> (i << 3)) & 0xff;
        } else {
            data[i] = (num >> ((bytes - 1 - i) << 3)) & 0xff;
        }
    }
    return(bytes);
}



void std_err(void) {
    perror("\nError");
    myexit();
}



int fgetz(u8 *data, int size, FILE *fd) {
    u8      *p;

    fflush(fd);
    if(!fgets(data, size, fd)) {
        data[0] = 0;
        return(0);
    }
    for(p = data; *p && (*p != '\n') && (*p != '\r'); p++);
    *p = 0;
    return(p - data);
}



void myexit(void) {
#ifdef WIN32
    u8      ans[8];

    if(GetWindowLong(mywnd, GWL_WNDPROC)) {
        printf("\nPress RETURN to quit");
        fgetz(ans, sizeof(ans), stdin);
    }
#endif
    exit(1);
}


