#include "Crc.h"

static uint32_t crc32_table[256];
static bool     crc_initialized = false;

static uint32_t Reflect(uint32_t ref, char ch)
{
    uint32_t value = 0;

    // Swap bit 0 for bit 7
    // bit 1 for bit 6, etc.
    for (int i = 1; i < (ch + 1); i++) {
        if (ref & 1) {
            value |= 1 << (ch - i);
        }
        ref >>= 1;
    }
    return value;
}

// Call this function only once to initialize the CRC table.
static void Init_CRC32_Table()
{
    // This is the official polynomial used by CRC-32
    // in PKZip, WinZip and Ethernet.
    uint32_t ulPolynomial = 0x04c11db7;

    // 256 values representing ASCII character codes.
    for (int i = 0; i <= 0xFF; i++) {
        crc32_table[i] = Reflect(i, 8) << 24;
        for (int j = 0; j < 8; j++) {
            crc32_table[i] = (crc32_table[i] << 1) ^ (crc32_table[i] & (1 << 31) ? ulPolynomial : 0);
        }
        crc32_table[i] = Reflect(crc32_table[i], 32);
    }
}

uint32_t calcCRC(const void* pBuf, int Count)
{
    if (!crc_initialized) {
        Init_CRC32_Table();
        crc_initialized = true;
    }
    // Pass a buffer to this function and it will return the CRC.

    // Once the lookup table has been filled in by the two functions above,
    // this function creates all CRCs using only the lookup table.

    // Be sure to use unsigned variables,
    // because negative values introduce high bits
    // where zero bits are required.

    // Start out with all bits set high.
    uint32_t ulCRC(0xffffffff);
    uint8_t* buffer;

    // Get the length.
    // Save the text in the buffer.
    buffer = (uint8_t*)pBuf;
    // Perform the algorithm on each character
    // in the string, using the lookup table values.
    while (Count--) {
        ulCRC = (ulCRC >> 8) ^ crc32_table[(ulCRC & 0xFF) ^ *buffer++];
    }

    // Exclusive OR the result with the beginning value.
    return ulCRC ^ 0xffffffff;
}
