#ifndef MAC_KEYS_H
#define MAC_KEYS_H

/*
 * Device credentials. Must match the endpoint registered in the base station.
 * Currently the public ETSI TS 103 357-2 Annex D.3 test vector values.
 * Replace with your own EUI64 / network key before real use, and keep real keys out of git.
 */
#define MAC_KEY_EUI64        { 0x70, 0xb3, 0xd5, 0xc1, 0xf0, 0x01, 0x50, 0x3a }
#define MAC_KEY_NWK_KEY      { 0xe6, 0xad, 0xe6, 0x60, 0x5a, 0x92, 0xdd, 0xd2, \
                               0xa2, 0xb2, 0xcf, 0x39, 0x5e, 0xb1, 0x26, 0x62 }
#define MAC_KEY_SHORT_ADDR   0xF001
#define MAC_KEY_USE_SHORT    1
#define MAC_KEY_START_COUNTER 0x000B8DE1u

#endif /* MAC_KEYS_H */
