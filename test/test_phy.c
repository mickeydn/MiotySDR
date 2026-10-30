
#include <stdio.h>

#include "phy.h"

int main(void)
{
    
    const uint8_t input[2] = {0x16, 0x0F};
    const uint8_t expected = 0xD0;

    uint8_t result = phy_crc8(input, 16);

    if (result == expected) {
        printf("PASS  CRC = 0x%02X\n", result);
        return 0;
    }

    printf("FAIL  CRC = 0x%02X, expected 0x%02X\n", result, expected);
    return 1;
}
