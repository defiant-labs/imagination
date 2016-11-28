//
// EdcEcc.h
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

#ifndef _EDC_ECC_H_
#define _EDC_ECC_H_

#include <types.h>

// sector mode/form types 
enum SectorModeType {
    MODE1 = 0,        // mode1 sector
    MODE2_FORM1,    // mode2 sector
    MODE2_FORM2,    // mode2 sector
    MODE_UNKNOWN,    // unknown sector mode
};

class EdcEcc
{
public:
    // sets up tables used in calculations
    EdcEcc();
    // frees all tables
    ~EdcEcc();

    // this checks a sector's edc and ecc values
    // but does not alter the sector in anyway
    //
    // args:    sector buffer (2352 bytes)
    //          mode of data in sector buffer
    // returns: true if sector is ok
    //          false if sector needs patching
    bool isSectorOk(const u8* sector, SectorModeType sectorType);

    // this recalculates a sector's edc and ecc values
    //
    // args:    sector buffer (2352 bytes)
    //          mode of data in sector buffer
    // returns: true if fixed sector is ok
    //          false if error fixing sector
    bool fixSector(u8* sector, SectorModeType sectorType);

private:
    // stores the current number of EdcEcc objects created
    // this is needed t know when to delete dynamically created static arrays
    static s32 objectNumber;

    // this calculates the EDC for a sector
    // the data being crc'ed must be present in the sectors before calling this
    //
    // for mode1, the edc is calculated over the sync, header and data
    // for mode2 form1, the edc is calculated over the data and subheader
    // for mode2 form2, the edc is calculated over the data and subheader
    //
    // if sector and edc are in shared memspace, edc output will
    // overwrite buffer input, but they wont interfere with each other besides that
    //
    // note: it is valid to just use 0 for EDC,
    // (this just means dont check for errors in this sector)
    //
    // args:    sector buffer (assumed to be 2352 bytes)
    //          mode of data in sector buffer
    //          output for calculated edc
    // returns: true if successful
    void CalcEDC(const u8* sector, SectorModeType sectorType, u8* edc);

    // calculates the edc for the given data
    //
    // args:    data to compute edc over
    //          size of data to compute edc over
    // returns: 32bit edc value
    u32 calc_EDC(const u8* data, s32 size);

    // create/delete edc table
    static void create_EDC_table();
    static void delete_EDC_table();
    static u32* edcTable;

    // reverses the bits in 'data'
    //
    // args:    data containing bits to reverse
    //          number of bits in 'data' to reverse
    // returns: data with reversed bits
    static u32 mirrorBits(u32 data, int numBits);

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
    void CalcECC(const u8* sector, SectorModeType sectorType, u8* ecc);

    // calculate part of ecc data
    //
    // args:    sector buffer (assumed to be 2352 bytes)
    //            output for calculated ecc data
    void calc_ECC_P(const u8* sector, u8* ecc_p);
    void calc_ECC_Q(const u8* sector, u8* ecc_p);

    // create/delete ecc table
    static void create_ECC_table();
    static void delete_ECC_table();
    static u16** eccTable;
    static u8 logAdd(u8 a, u8 b);
    static u8 logDiv(u8 a, u8 b);
    // create/delete log tables
    static void create_logTables();
    static void delete_logTables();
    static u8* logTable;
    static u8* ilogTable;
};

// Sector format (mode 1)
//    0          12        Synchronization data
//   12           4        Header (sector number)
//   16        2048        Data (2048 bytes)
// 2064           4        EDC (error detection code)
// 2068           8        Zero
// 2076         172        ECC-P (error correction code, layer P)
// 2248         104        ECC-Q (error correction code, layer Q)

// Sector format (mode 2 form 1)
//    0          12        Synchronization data
//   12           4        Header (sector number)
//   16           8        Sub-header (sector mode, XA header information)
//   24        2048        Data (2048 bytes)
// 2072           4        EDC (error detection code)
// 2076         172        ECC-P (error correction code, layer P)
// 2248         104        ECC-Q (error correction code, layer Q)

// Sector format (mode 2 form 2) no ECC present
//    0          12        Synchronization data
//   12           4        Header (sector number)
//   16           8        Sub-header (sector mode, XA header information)
//   24        2324        Data (2324 bytes)
// 2348           4        EDC (error detection code)

#endif // _EDC_ECC_H_
