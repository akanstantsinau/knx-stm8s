#include "uart.h"

inline uint8_t uart_transmit_data_register_empty(void) {
	return UART1->SR & UART_SR_TXE;
}

void uart_send_byte(uint8_t data) {
    while(!uart_transmit_data_register_empty());
    UART1->DR = data;
}
