#ifndef KNX_H
#define KNX_H
#include <stdint.h>

extern uint16_t knx_host_address;

void knx_set_address(uint16_t address);
void receive_knx_frame(uint8_t* data, uint8_t length);
void knx_send_frame(uint16_t destination_address, uint8_t priority, uint8_t* data, uint8_t length);

#endif // !KNX_H

