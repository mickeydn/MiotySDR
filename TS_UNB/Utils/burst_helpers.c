#include "burst_helpers.h"
#include "phy.h"

uint16_t phy_ilv_subpacket(uint16_t bit_idx, uint16_t num_bursts) {
    //the first half of the frame is 288 bits, the second half is 288 bits
    if (bit_idx < TSUNBPHY_FRAME_FIRST_HALF) {
        //first half of the frame
        return bit_idx % TSUNBPHY_CORE_FRAME_BURSTS;
    }

    //second half of the frame bit_idx >= 288
    uint16_t second_half_idx = bit_idx - TSUNBPHY_FRAME_FIRST_HALF; //makes index start 0 for easier computation
    uint16_t group_len = num_bursts -(TSUNBPHY_CORE_FRAME_BURSTS / 2); 
    uint16_t group = second_half_idx / group_len; //group 0..23
    uint16_t group_idx = second_half_idx % group_len; //index within the group

    if(group_idx < (TSUNBPHY_CORE_FRAME_BURSTS / 2)) {
        return 2 * group_idx + (group & 1); //
    }

    return group_idx + (TSUNBPHY_CORE_FRAME_BURSTS / 2); //return the index in the second half of the frame (extension frame)
}

uint8_t phy_ilv_position(uint16_t subpacket, uint8_t rank) {

    uint8_t distance = rank / 2; //distance from the pilot

    if(((subpacket + rank) & 1) == 0 ){
        return (TSUNBPHY_SYMBOLS_PER_HALF_BURST - 1) - distance; //distance from pilot to start
    }

    return 2 * (TSUNBPHY_SYMBOLS_PER_HALF_BURST) + distance; //distance from pilot to end
}

