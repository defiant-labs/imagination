//
// EdcEcc.cpp
//
// EDC / ECC checking/patching code for cd sectors
//
// loser april 2001
// based on 'check' by crusador / kalisto
//
// supports the following sector modes:
//      MODE_1
//      MODE2_FORM1
//      MODE2_FORM2
//

#include "EdcEcc.h"
#include <string.h>
#include <stdio.h>

#define GF8_PRIM_POLY       0x11d           // x^8 + x^4 + x^3 + x^2 + 1

#define EDC_POLY            0x8001801B      // (x^16 + x^15 + x^2 + 1) (x^16 + x^2 + x + 1)
#define EDC_SIZE            4

#define ECC_P_DATA_OFFSET   12
#define ECC_P_DATA_SIZE     2064
#define ECC_P_OFFSET        2076
#define ECC_P_SIZE          172

#define ECC_Q_DATA_OFFSET   12
#define ECC_Q_DATA_SIZE     2236
#define ECC_Q_OFFSET        (ECC_P_OFFSET + ECC_P_SIZE)
#define ECC_Q_SIZE          104

#define ECC_OFFSET          ECC_P_OFFSET
#define ECC_SIZE            (ECC_P_SIZE + ECC_Q_SIZE)

// the offsets and sizes of data to compute the edc and ecc values over
// as well as the offsets and sizes of edc and ecc values themselves
const struct {
    s32 edcDataOffset;      // start of data to compute edc crc over
    s32 edcDataSize;        // size  of data to compute edc crc over
    s32 edcOffset;          // start of edc in sector
    s32 edcSize;            // size  of edc in sector

    s32 eccPDataOffset;     // start of data to compute ecc_p over
    s32 eccPDataSize;       // size  of data to compute ecc_p over
    s32 eccPOffset;         // start of ecc_p in sector
    s32 eccPSize;           // size  of ecc_p in sector

    s32 eccQDataOffset;     // start of data to compute ecc_q over
    s32 eccQDataSize;       // size  of data to compute ecc_q over
    s32 eccQOffset;         // start of ecc_q in sector
    s32 eccQSize;           // size  of ecc_q in sector
} SectorModeOffsets[] = {
    // Mode 1
    {  0, 2064, 2064,   4,
      12, 2064, 2076, 172,
      12, 2236, 2420, 104 },

    // Mode 2 Form 1
    { 16, 2056, 2072,   4,
      12, 2064, 2076, 172,
      12, 2236, 2248, 104 },

    // Mode 2 Form 2 (no ECC)
    { 16, 2332, 2348,   4,
      -1,   -1,   -1,  -1,
      -1,   -1,   -1,  -1 }
};


// sets up mod tables used in later calculations
EdcEcc::EdcEcc()
{
    if(objectNumber == 0) {
        create_EDC_table();
        create_ECC_table();
    }

    objectNumber++;
}

EdcEcc::~EdcEcc()
{
    objectNumber--;
    if(objectNumber == 0)
    {
        delete_EDC_table();
        delete_ECC_table();
    }
}

// current number of created edc-ecc objects
s32 EdcEcc::objectNumber = 0;


// this checks a sector's edc and ecc values
// but does not alter the sector in anyway
//
// args:    sector buffer (2352 bytes)
//          mode of data in sector buffer
// returns: true if sector is ok
//          false if sector needs patching
bool EdcEcc::isSectorOk(const u8* sector, SectorModeType sectorType)
{
    // make sure args are valid
    if(sector == NULL)
        return false;
    if( sectorType != MODE1 &&
        sectorType != MODE2_FORM1 &&
        sectorType != MODE2_FORM2)
        return false;

    u8 oldEdc[EDC_SIZE], newEdc[EDC_SIZE];
    memcpy(oldEdc, §or[ SectorModeOffsets[sectorType].edcOffset ], EDC_SIZE);
    CalcEDC(sector, sectorType, newEdc);
    if(memcmp(oldEdc, newEdc, EDC_SIZE))
        return false;

    u8 oldEcc[ECC_SIZE], newEcc[ECC_SIZE];
    memcpy(oldEdc, §or[ ECC_OFFSET ], ECC_SIZE);
    CalcECC(sector, sectorType, newEcc);
    if(memcmp(oldEcc, newEcc, ECC_SIZE))
        return false;

    return true;
}


// this recalculates a sector's edc and ecc values
//
// args:    sector buffer (2352 bytes)
//          mode of data in sector buffer
// returns: true if fixed sector is ok
//          false if error fixing sector
bool EdcEcc::fixSector(u8* sector, SectorModeType sectorType)
{
    // make sure args are valid
    if(sector == NULL)
        return false;
    if( sectorType != MODE1 &&
        sectorType != MODE2_FORM1 &&
        sectorType != MODE2_FORM2)
        return false;

    CalcEDC(sector, sectorType, §or[ SectorModeOffsets[sectorType].edcOffset  ]);
    CalcECC(sector, sectorType, §or[ SectorModeOffsets[sectorType].eccPOffset ]);

    return true;
}


// this calculates the EDC for a sector
// the data being crc'ed must be present in the sectors before calling this
//
// for mode1, the edc is calculated over the sync, header and data
// for mode2 form1, the edc is calculated over the data and subheader
// for mode2 form2, the edc is calculated over the data and subheader
//
// if sector and edc are in shared memspace, edc output will
// overwrite sector input, but they wont interfere with each other besides that
//
// note: it is valid to just use 0 for EDC,
// (this just means dont check for errors in this sector)
//
// args:    sector buffer (assumed to be 2352 bytes)
//          mode of data in sector buffer
//          output for calculated edc
// returns: true if successful
void EdcEcc::CalcEDC(const u8* sector, SectorModeType sectorType, u8* edc)
{
    u32 edcTemp = 0;

    // mode1
    if(     sectorType == MODE1)        edcTemp = calc_EDC(sector+ 0, 2048+16);
    // mode2 form1
    else if(sectorType == MODE2_FORM1)  edcTemp = calc_EDC(sector+16, 2048+ 8);
    // mode2 form2
    else if(sectorType == MODE2_FORM1)  edcTemp = calc_EDC(sector+16, 2324+ 8);

    // write out resultant edc
    memcpy(edc, &edcTemp, 4);
}

// calculates the edc for the given data
//
// args:    data to compute edc over
//          size of data to compute edc over
// returns: 32bit edc value
u32 EdcEcc::calc_EDC(const u8* data, s32 size)
{
    u32 edc = 0;

    while(size--)
    {
        u8 tableOffset = (edc ^ *data++) & 0xFF;
        edc = edcTable[tableOffset] ^ (edc >> 8);
    }

    return edc;
}


// create EDC table and fill it with values
void EdcEcc::create_EDC_table()
{
    edcTable = NULL;
    edcTable = new u32[256];

    // calculate each of the 256 values in the edc-table
    for (int i = 0; i < 256; i++)
    {
        // calculate edc value
        u32 r = mirrorBits(i, 8);
        r <<= 24;

        for (int j = 0; j < 8; j++)
        {
            if (r & 0x80000000)
            {
                r <<= 1;
                r ^= EDC_POLY;
            }
            else
            {
                r <<= 1;
            }
        }

        // set edc value in table
        edcTable[i] = mirrorBits(r, 32);
    }
}


// free EDC table
void EdcEcc::delete_EDC_table()
{
    delete[] edcTable;
    edcTable = NULL;
}


// table used for edc calculation
u32* EdcEcc::edcTable = NULL;


// reverses the bits in 'data'
//
// args:    data containing bits to reverse
//          number of bits in 'data' to reverse
// returns: data with reversed bits
u32 EdcEcc::mirrorBits(u32 data, int numBits)
{
    u32 reversedData = 0;

    for (int i = 0; i < numBits; i++)
    {
        reversedData <<= 1;
        if (data & 1)
            reversedData |= 1;
        data >>= 1;
    }

    return reversedData;
}


// this calculates the ECC for the sector.
//
// for mode1, the ecc is calculated over the sync, header and data
// for mode2 form1, ?
// for mode2 form2, there is no ecc present in sector
//
// if sector and ecc are in shared memspace, edc output will
// overwrite buffer input, but they wont interfere with each other besides that
//
// args:    sector buffer (assumed to be 2352 bytes)
//          mode of data in sector buffer
//          output for calculated ecc
// returns: true if successful
void EdcEcc::CalcECC(const u8* sector, SectorModeType sectorType, u8* ecc)
{
    // m2f2 doesnt have any ecc to correct!
    if(sectorType == MODE2_FORM2)
        return;
    // zero the header in m2f1 sectors
    u8 header[4];
    if(sectorType == MODE2_FORM1)
    {
        memcpy(header, §or[12], 4);
        memset((u8*)§or[12], 0, 4);
    }

    // calculate ECC_P part of ECC first
    calc_ECC_P(sector, &ecc[0]);
    // calculate ECC_Q part of ECC second
    calc_ECC_Q(sector, &ecc[ECC_P_SIZE]);

    // replace the header in m2f1 sectors
    if(sectorType == MODE2_FORM1)
    {
        memcpy((u8*)§or[12], header, 4);
    }
}


// calculate the P parities for the sector.
// the 43 P vectors of length 24 are combined with the eccTable values
//
// ecc_p gets generated for sector bytes: 12 to 2074
//
// args:    sector buffer (assumed to be 2352 bytes)
//          output for calculated ecc data
void EdcEcc::calc_ECC_P(const u8* sector, u8* ecc_p)
{
    // start of data to generate ecc_p for
    const u8* p_lsb_start = sector + ECC_P_DATA_OFFSET + 0;
    const u8* p_msb_start = sector + ECC_P_DATA_OFFSET + 1;

    // p1 = start of p-parity data, p0 = 2nd half of p-parity data
//  u8* p1 = (u8*)sector + ECC_P_OFFSET;
//  u8* p0 = (u8*)sector + ECC_P_OFFSET + 2*43;
    u8* p1 = ecc_p;
    u8* p0 = ecc_p + 2*43;

    // calculate all 43 P vectors
    for(int i = 0; i < 43; i++)
    {
        // p_lsb/p_msb = pointesr to data in sector to generate ecc-p for
        const u8* p_lsb   = p_lsb_start;
        const u8* p_msb   = p_msb_start;
        u16 p01_lsb = 0;
        u16 p01_msb = 0;

        // the last 24 q-coeffcients are actually the 24 p coeffcients
        for(int j = 19; j < 43; j++)
        {
            u8 d0 = *p_lsb;
            p01_lsb ^= eccTable[j][d0];
            p_lsb += 2 * 43;

            u8 d1 = *p_msb;
            p01_msb ^= eccTable[j][d1];
            p_msb += 2 * 43;
        }

        *(p0 + 0) = (u8)(p01_lsb>>0);
        *(p0 + 1) = (u8)(p01_msb>>0);
        p0 += 2;

        *(p1 + 0) = (u8)(p01_lsb>>8);
        *(p1 + 1) = (u8)(p01_msb>>8);
        p1 += 2;

        p_lsb_start += 2;
        p_msb_start += 2;
    }
}


// calculate the Q parities for the sector.
// the 26 Q vectors of length 43 are combined with the eccTable values
//
// args:    sector buffer (assumed to be 2352 bytes)
//          output for calculated ecc data
void EdcEcc::calc_ECC_Q(const u8* sector, u8* ecc_q)
{
    const u8* q_start = sector + ECC_Q_OFFSET;

    // start of data to generate ecc_q for
    const u8* q_lsb_start = sector + ECC_Q_DATA_OFFSET + 0;
    const u8* q_msb_start = sector + ECC_Q_DATA_OFFSET + 1;

    // q1 = start of q-parity data, q0 = 2nd half of q-parity data
//  u8* q1 = (u8*)sector + ECC_Q_OFFSET;
//  u8* q0 = (u8*)sector + ECC_Q_OFFSET + 2*26;
    u8* q1 = ecc_q;
    u8* q0 = ecc_q + 2*26;

    // calculate all 26 Q vectors
    for(int i = 0; i < 26; i++)
    {
        const u8* q_lsb = q_lsb_start;
        const u8* q_msb = q_msb_start;
        u16 q01_lsb = 0;
        u16 q01_msb = 0;

        for(int j = 0; j < 43; j++)
        {
            u8 d0 = *q_lsb;
            q01_lsb ^= eccTable[j][d0];
            q_lsb += 2 * 44;

            u8 d1 = *q_msb;
            q01_msb ^= eccTable[j][d1];
            q_msb += 2 * 44;

            if (q_lsb >= q_start)
            {
                q_lsb -= 2 * 1118;
                q_msb -= 2 * 1118;
            }
        }

        *(q0 + 0) = (u8)(q01_lsb>>0);
        *(q0 + 1) = (u8)(q01_msb>>0);
        q0 += 2;

        *(q1 + 0) = (u8)(q01_lsb>>8);
        *(q1 + 1) = (u8)(q01_msb>>8);
        q1 += 2;

        q_lsb_start += 2 * 43;
        q_msb_start += 2 * 43;
    }
}


// create/delete ecc table
void EdcEcc::create_ECC_table()
{
    // create the log tables
    create_logTables();

    // generate coefficients table
    u8 eccHelpTable[2][45];
    u8 eccCoeffsTable[2][45];

    // build matrix H:
    //  1    1   ...  1   1
    // a^44 a^43 ... a^1 a^0
    for (int i = 0; i < 45; i++)
    {
        eccHelpTable[0][i] = 1;                 // e0
        eccHelpTable[1][i] = ilogTable[44-i];   // e1
    }

    // resolve equation system for parity byte 0 and 1

    // e1' = e1 + e0
    for(int i = 0; i < 45; i++)
        eccCoeffsTable[1][i] = logAdd(eccHelpTable[1][i], eccHelpTable[0][i]);

    // e1'' = e1' / (a^1 + 1)
    for(int i = 0; i < 45; i++)
        eccCoeffsTable[1][i] = logDiv(eccCoeffsTable[1][i], eccCoeffsTable[1][43]);

    // e0' = e0 + e1 / a^1
    for(int i = 0; i < 45; i++)
        eccCoeffsTable[0][i] = logAdd( eccHelpTable[0][i], logDiv(eccHelpTable[1][i], ilogTable[1]) );

    // e0'' = e0' / (1 + 1 / a^1)
    for(int i = 0; i < 45; i++)
        eccCoeffsTable[0][i] = logDiv(eccCoeffsTable[0][i], eccCoeffsTable[0][44]);

    // compare
/*  for(int i=0; i<2; i++)
    {
        for(int j=0; j<43; j++)
        {
            printf("%02X ", eccCoeffsTable[i][j]);
        }
        printf("\n");
    }
*/
    // Compute the products of 0..255 with all of the Q coefficients in
    // advance. When building the scalar product between the data vectors
    // and the P/Q vectors the individual products can be looked up in
    // this table
    //
    // The P parity coefficients are just a subset of the Q coefficients so
    // that we do not need to create a separate table for them.
    eccTable  = NULL;
    eccTable  = new u16*[43];
    for(int i=0; i<43; i++)  eccTable[i] = new u16[256];

    u8 loIdx, hiIdx;
    for(int j = 0; j < 43; j++)
    {
        eccTable[j][0] = 0;

        for (i = 1; i < 256; i++)
        {
            loIdx = (logTable[i] + logTable[eccCoeffsTable[0][j]]) % 255;
            hiIdx = (logTable[i] + logTable[eccCoeffsTable[1][j]]) % 255;
            eccTable[j][i]= (ilogTable[loIdx]<<0) |
                            (ilogTable[hiIdx]<<8);
        }
    }
}


void EdcEcc::delete_ECC_table()
{
    delete_logTables();

    for(int i=0; i<43; i++)  { delete[] eccTable[i]; eccTable[i] = NULL; }
    delete[] eccTable;      eccTable = NULL;
}


u16** EdcEcc::eccTable = NULL;


// Addition in the GF(8) domain: just the XOR of the values.
u8 EdcEcc::logAdd(u8 a, u8 b)
{
    return a ^ b;
}


// Division in the GF(8) domain
// Like multiplication but logarithms are subtracted.
u8 EdcEcc::logDiv(u8 a, u8 b)
{
    if (a == 0)
        return 0;

    s16 sum = logTable[a] - logTable[b];
    if (sum < 0)
        sum += 255;
    return ilogTable[sum];
}


// creates the logarithm and inverse logarithm tables that are required
// for performing multiplication in the GF(8) domain
void EdcEcc::create_logTables()
{
    logTable  = NULL;
    logTable  = new u8[256];
    ilogTable = NULL;
    ilogTable = new u8[256];
    memset(logTable, 0, 256);
    memset(ilogTable, 0, 256);

    u16 b = 1;
    for (u16 log = 0; log < 255; log++)
    {
        logTable[( u8)b  ] = (u8)log;
        ilogTable[(u8)log] = (u8)b;

        b <<= 1;
        if (b & 0x0100)
            b ^= GF8_PRIM_POLY;
    }
}


void EdcEcc::delete_logTables()
{
    delete[] logTable;
    logTable  = NULL;

    delete[] ilogTable;
    ilogTable = NULL;
}


u8* EdcEcc::logTable  = NULL;
u8* EdcEcc::ilogTable = NULL;
