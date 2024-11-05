#include "knx.h"
#include "uart.h"
#include "ncn5120.h"

uint16_t knx_host_address;

void knx_set_address(uint16_t address) {
    uart_send_byte(U_SET_ADDRESS_REQ);
    uart_send_byte((uint8_t)(address >> 8));
    uart_send_byte((uint8_t)address);
    uart_send_byte((uint8_t)0xFF);
    knx_host_address = address;
}
void receive_knx_frame(uint8_t* data, uint8_t length) {
}

// TODO: current max frame length is 64 bytes. Implement U_L_DataOffset.req
void knx_send_frame(uint16_t destination_address, uint8_t priority, uint8_t* data, uint8_t length) {
    uint8_t check_field = 0;
    uint8_t pos = 0;

    uart_send_byte(U_L_DATA_START_REQ | pos++);
    // prepare control field
    // 10R1PP00
    // R is repeat flag. Lets assume it always 0 at this iteration
    // PP are 2 bits of priority.
    // According to KNX spec:
    // 00 - System functions
    // 01 - Alarm functions
    // 10 - Normal mode high priority
    // 11 - Normal mode low priority
    uint8_t control_field =0x90 | (priority & 3) << 2; 
    uart_send_byte(control_field);
    check_field ^= control_field;
    

    uart_send_byte(U_L_DATA_START_REQ | pos++);
    uart_send_byte(*((uint8_t*)&knx_host_address));
    check_field ^= *((uint8_t*)&knx_host_address);

    uart_send_byte(U_L_DATA_START_REQ | pos++);
    uart_send_byte(*(((uint8_t*)&knx_host_address)+1));
    check_field ^= *(((uint8_t*)&knx_host_address)+1);
    
    uart_send_byte(U_L_DATA_START_REQ | pos++);
    uart_send_byte(*((uint8_t*)&destination_address));
    check_field ^= *((uint8_t*)&destination_address);

    uart_send_byte(U_L_DATA_START_REQ | pos++);
    uart_send_byte(*(((uint8_t*)&destination_address)+1));
    check_field ^= *(((uint8_t*)&destination_address)+1);

    for(uint8_t i = 0 ; i < length ; i++){
        uart_send_byte(U_L_DATA_START_REQ | pos++);
        uart_send_byte(data[i]);
	check_field ^= data[i];
    }

    check_field = ~check_field;
    uart_send_byte(0x40 | (pos & 0b111111));
    uart_send_byte(check_field);
}

