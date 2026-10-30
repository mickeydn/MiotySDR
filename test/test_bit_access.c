/**
 * @file    test_bit_access.c
 * @brief   PC unit tests for bit_read() and bit_write().
 *
 * Build and run from the project root (on the PC, not the STM32):
 *   gcc -std=c99 -Wall -Wextra -ITS_UNB/Utils test/test_bit_access.c TS_UNB/Utils/bit_access.c -o test/test_bit_access.exe
 *   ./test/test_bit_access.exe
 */
#include <stdio.h>

#include "bit_access.h"

static int failures = 0;

/* Print PASS/FAIL for one check and count failures. */
static void check(const char *name, int ok)
{
    printf("%s  %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok)
    {
        failures++;
    }
}

/* 0xA0 = 1010 0000, 0x2C = 0010 1100 */
static const uint8_t test_data[2] = {0xA0, 0x2C};
static const uint8_t test_bits[16] = {1, 0, 1, 0, 0, 0, 0, 0,
                                      0, 0, 1, 0, 1, 1, 0, 0};

/* 1. Every bit is read in MSB-first order. */
static void test_read_all_bits(void)
{
    int ok = 1;

    for (uint16_t i = 0; i < 16; i++)
    {
        if (bit_read(test_data, i) != test_bits[i])
        {
            printf("      bit %u: got %u, expected %u\n", i, bit_read(test_data, i), test_bits[i]);
            ok = 0;
        }
    }
    check("read all 16 bits in MSB-first order", ok);
}

/* 2. Index 0 is the MSB and index 7 is the LSB of the first byte. */
static void test_read_byte_edges(void)
{
    const uint8_t data[1] = {0x81};  /* 1000 0001 */
    const uint8_t only_msb[1] = {0x80};

    check("index 0 is the MSB", bit_read(data, 0) == 1 && bit_read(only_msb, 0) == 1);
    check("index 7 is the LSB", bit_read(data, 7) == 1 && bit_read(only_msb, 7) == 0);
}

/* 3. Index 8 moves on to the MSB of the next byte. */
static void test_read_byte_boundary(void)
{
    const uint8_t data[2] = {0x00, 0x80};  /* bit 7 is 0, bit 8 is 1 */

    check("index 7 -> 8 crosses into the next byte", bit_read(data, 7) == 0 && bit_read(data, 8) == 1);
}

/* 4. Writing every bit rebuilds the data, starting from the opposite values. */
static void test_write_rebuilds_data(void)
{
    uint8_t buf[2] = {0x00, 0xFF};

    for (uint16_t i = 0; i < 16; i++)
    {
        bit_write(buf, i, test_bits[i]);
    }

    if (buf[0] != test_data[0] || buf[1] != test_data[1])
    {
        printf("      got %02x %02x, expected %02x %02x\n", buf[0], buf[1], test_data[0], test_data[1]);
    }
    check("write sets and clears bits to rebuild the data",
          buf[0] == test_data[0] && buf[1] == test_data[1]);
}

/* 5. Writing one bit leaves all other bits unchanged. */
static void test_write_preserves_other_bits(void)
{
    uint8_t buf[2] = {0xA0, 0x2C};

    bit_write(buf, 10, 0);  /* 0010 1100 -> 0000 1100 */
    check("clearing one bit leaves the others unchanged", buf[0] == 0xA0 && buf[1] == 0x0C);

    bit_write(buf, 9, 1);   /* 0000 1100 -> 0100 1100 */
    check("setting one bit leaves the others unchanged", buf[0] == 0xA0 && buf[1] == 0x4C);
}

/* 6. Any nonzero value counts as 1. */
static void test_write_nonzero_is_one(void)
{
    uint8_t buf[1] = {0x00};

    bit_write(buf, 2, 7);
    check("writing a nonzero value sets the bit", buf[0] == 0x20);
}

/* 7. A bit written at any position reads back the same. */
static void test_round_trip(void)
{
    uint8_t buf[4] = {0};
    int ok = 1;

    for (uint16_t i = 0; i < 32; i++)
    {
        bit_write(buf, i, 1);
        if (bit_read(buf, i) != 1)
        {
            ok = 0;
        }
        bit_write(buf, i, 0);
        if (bit_read(buf, i) != 0)
        {
            ok = 0;
        }
    }
    check("write then read gives the same bit at every position", ok);
}

int main(void)
{
    test_read_all_bits();
    test_read_byte_edges();
    test_read_byte_boundary();
    test_write_rebuilds_data();
    test_write_preserves_other_bits();
    test_write_nonzero_is_one();
    test_round_trip();

    printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASSED", failures, failures == 1 ? "" : "s");
    return failures;
}
