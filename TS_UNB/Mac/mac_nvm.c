#include "mac_nvm.h"
#include "stm32wl3x_hal.h"

#define NVM_EMPTY            0xFFFFFFFFu                     // erased flash word
#define NVM_WORDS_PER_PAGE   (FLASH_PAGE_SIZE / 4u)          // 512

static const uint32_t nvm_pages[2] = { MAC_NVM_PAGE_A_ADDR, MAC_NVM_PAGE_B_ADDR };
static uint8_t  nvm_page;            // index (0/1) of the page currently written
static uint32_t nvm_next;            // number of words already used in that page

static uint32_t nvm_read(uint32_t addr){
    return *(volatile const uint32_t *)addr;
}

// number of used words in a page (values are written from the start without gaps)
static uint32_t nvm_used_words(uint32_t page_addr){
    uint32_t n = 0;
    while (n < NVM_WORDS_PER_PAGE && nvm_read(page_addr + 4u * n) != NVM_EMPTY){
        n++;
    }
    return n;
}

static uint8_t nvm_erase(uint32_t page_addr){
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error;
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Page      = (page_addr - FLASH_START_ADDR) / FLASH_PAGE_SIZE;
    erase.NbPages   = 1;
    return (HAL_FLASHEx_Erase(&erase, &page_error) == HAL_OK) ? 0 : 1;
}

uint32_t mac_nvm_load_counter(uint32_t start_counter){
    uint32_t best = 0;
    uint8_t  found = 0;
    nvm_page = 0;
    nvm_next = 0;

    // the newest value is the highest last word of the two pages
    for (uint8_t p = 0; p < 2; p++){
        uint32_t used = nvm_used_words(nvm_pages[p]);
        if (used > 0){
            uint32_t last = nvm_read(nvm_pages[p] + 4u * (used - 1u));
            if (!found || last > best){
                best = last;
                found = 1;
                nvm_page = p;
                nvm_next = used;
            }
        }
    }

    if (!found || start_counter > best){
        return start_counter;
    }
    return best;
}

uint8_t mac_nvm_save_counter(uint32_t counter){
    // current page full: continue on the other page (the full page keeps the old value meanwhile)
    if (nvm_next >= NVM_WORDS_PER_PAGE){
        nvm_page ^= 1u;
        nvm_next = 0;
    }
    // first word of a page: make sure the page is erased
    if (nvm_next == 0 && nvm_used_words(nvm_pages[nvm_page]) != 0){
        if (nvm_erase(nvm_pages[nvm_page]) != 0){
            return 1;
        }
    }
    uint32_t addr = nvm_pages[nvm_page] + 4u * nvm_next;
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, counter) != HAL_OK){
        return 1;
    }
    nvm_next++;
    return (nvm_read(addr) == counter) ? 0 : 1;
}
