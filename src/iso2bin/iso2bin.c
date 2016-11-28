//
// iso2bin
//
// converts an iso file to a bin file
// (2048 bytes per sector to 2352 bytes per sector)
//
// loser march 2005
//


#include <stdio.h>
#include <string.h>
#include <types.h>
#include "EdcEcc.h"


#define ISO_SECTOR_SIZE     2048
#define BIN_SECTOR_SIZE     2352

#define SYNC_SIZE           12
#define SUB_HEADER_SIZE     8


const u8 syncData[SYNC_SIZE] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };
const u8 subHeader[SUB_HEADER_SIZE] = { 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00 };

const char title[] = "iso2bin beta 0.1 - loser 2005";
const char usage[] = "usage:  iso2bin <input iso file> <output bin file>";

void setSector(u8* sectorBuffer, s32 sectorNum);
void testit();


int main(int argc, const char* argv[])
{
    // check args
    printf("%s\n", title);

    if (argc != 3) {
        printf("%s\n", usage);
        return 1;
    }

    // open files
    char isoFilename[260];
    char binFilename[260];
    strcpy(isoFilename, argv[1]);
    strcpy(binFilename, argv[2]);

    FILE* iso_fd = fopen(isoFilename, "rb");

    if (iso_fd == NULL) {
        printf("error opening iso file: %s\n", isoFilename);
        return 2;
    }

    FILE* bin_fd = fopen(binFilename, "w+b");

    if (bin_fd == NULL) {
        printf("error creating bin file: %s\n", binFilename);
        fclose(iso_fd);
        return 2;
    }

    // convert iso to bin
    fseek(iso_fd, 0, SEEK_END);
    s32 sectorMax = ftell(iso_fd) / ISO_SECTOR_SIZE;
    fseek(iso_fd, 0, SEEK_SET);
    u8 sectorBuffer[BIN_SECTOR_SIZE];
    EdcEcc edcecc;

    for (s32 sectorNum = 0; sectorNum<sectorMax; sectorNum++) {
        // convert 1 sector from iso to bin
        setSector(sectorBuffer, sectorNum);
        if (fread(§orBuffer[24], ISO_SECTOR_SIZE, 1, iso_fd) != 1)
            printf("error reading sector %d\n", sectorNum);

        if (!edcecc.fixSector(sectorBuffer, MODE2_FORM1))
            printf("error adding edc/ecc data for sector %d\n", sectorNum);

        if (fwrite(§orBuffer[0], BIN_SECTOR_SIZE, 1, bin_fd) != 1)
            printf("error writing sector %d\n", sectorNum);
    }

    printf("Successfully converted %s to %s\n", isoFilename, binFilename);

    fclose(iso_fd);
    fclose(bin_fd);

    return 0;
}

u8 int2bcd(u8 i)
{
    return (((i/10) << 4) & 0xF0) | ((i%10) & 0x0F);
}

void setSector(u8* sectorBuffer, s32 sectorNum)
{
    memcpy(§orBuffer[0], syncData, SYNC_SIZE);
    sectorBuffer[12] = int2bcd(sectorNum / (75 * 60)); // minutes
    sectorBuffer[13] = int2bcd(sectorNum / 75);        // seconds
    sectorBuffer[14] = int2bcd(sectorNum % 75);        // frames
    sectorBuffer[15] = 0x02;                           // mode2
    memcpy(§orBuffer[16], subHeader, SUB_HEADER_SIZE);
}
