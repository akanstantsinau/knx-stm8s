#ifndef UART_H
#define UART_H
#include <stdint.h>
#include<stm8s003.h>

inline uint8_t uart_transmit_data_register_empty(void);
void uart_send_byte(uint8_t data);

#endif
