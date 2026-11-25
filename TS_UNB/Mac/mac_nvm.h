#ifndef MAC_NVM_H
#define MAC_NVM_H

#include <stdint.h>

/*
 * Packet counter storage in flash.
 * The last two flash pages are reserved for it (see STM32WL33CCVX_FLASH.ld).
 * Counter values are appended one 32-bit word at a time, so a page is only
 * erased every 512 packets. The two pages are used alternately, so the last
 * stored value survives a power loss during an erase.
 */
#define MAC_NVM_PAGE_A_ADDR    0x1007F000u   // second-last 2 KB page
#define MAC_NVM_PAGE_B_ADDR    0x1007F800u   // last 2 KB page

/**
 * @brief  Read the stored packet counter.
 * @param  start_counter  value to use when nothing is stored (or when it is higher)
 * @retval counter of the next packet to send
 */
uint32_t mac_nvm_load_counter(uint32_t start_counter);

/**
 * @brief  Store the packet counter (counter of the next packet to send).
 *         May erase a page (a few ms); do not call while a frame is on air.
 * @retval 0 on success, 1 on flash error
 */
uint8_t mac_nvm_save_counter(uint32_t counter);

#endif /* MAC_NVM_H */
