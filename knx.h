#ifndef KNX_H
#define KNX_H
#include <stdint.h>

void receive_knx_frame(uint8_t* data, uint8_t length);
void send_knx_frame(uint8_t* data, uint8_t length);

#endif // !KNX_H

