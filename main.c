#include <stdint.h>

#include<stm8s003.h>

#include "ncn5120.h"

typedef uint8_t timer;

#define F_CPU 2000000UL

// *********  LED settings *******************
#define LED_PIN     GPIO_ODR_5

// ********* UART settings *****
#define UART_BAUD_RATE 19200


// ******** KNX constants ********************
// Threshold should be 2.6ms
// We calculate it on timer with prescaler 128 and closk frequency 2000000 (F_CPU)
#define KNX_FRAME_SILENCE_THRESHOLD ((uint8_t)(0.0026 * (F_CPU >> 7)))

#define RECEIVE_BUFFER_LENGTH 100
#define SEND_BUFFER_LENGTH 100

volatile uint8_t receive_buffer[RECEIVE_BUFFER_LENGTH];
volatile uint8_t receive_buf_start_pos = 0;
volatile uint8_t receive_buf_end_pos = 0;
volatile uint8_t receive_knx_frame_in_progress = 0;

volatile uint8_t send_buffer[SEND_BUFFER_LENGTH];
volatile uint8_t send_buf_start_pos = 0;
volatile uint8_t send_buf_end_pos = 0;

inline timer get_time(void) {
    return TIM4->CNTR;
}

inline int uart_transmit_data_register_empty() {
	return UART1->SR & UART_SR_TXE;
}


uint8_t uart_transmit_byte() {
    uint8_t result = 0;
    {__asm__("SIM\n");}
    if(uart_transmit_data_register_empty() && send_buf_start_pos != send_buf_end_pos ){
       UART1->DR = send_buffer[send_buf_start_pos++];
       send_buf_start_pos %= SEND_BUFFER_LENGTH;
       result = 1;
    }
    {__asm__("RIM\n");}
    return result; 
}
void uart_send_bytes(uint8_t size, uint8_t* data) {
    for(uint8_t i = 0 ; i < size ; i++) {
        while(!uart_transmit_data_register_empty());
	UART1->DR = data[i];
    }
}

volatile timer last_b;

uint8_t is_data_start_ind(uint8_t data) {
    return data & (1 << 7 | 1 << 4) && ~data & (1 << 6 | 1 << 1 | 1);
}

void store_byte(uint8_t data) {
    receive_buffer[receive_buf_end_pos++] = data;
    if (receive_buf_end_pos == RECEIVE_BUFFER_LENGTH) {
    	receive_buf_end_pos = 0;
    }

    if(!receive_knx_frame_in_progress && is_data_start_ind(data)) {
        receive_knx_frame_in_progress = 1;
    }
    if(receive_knx_frame_in_progress && get_time() - last_b > KNX_FRAME_SILENCE_THRESHOLD) {
        receive_knx_frame_in_progress = 0;
        {__asm__("SIM\n");}
    }
    last_b = get_time();
}
uint8_t uart_data_available() {
    return receive_buf_end_pos != receive_buf_start_pos && !receive_knx_frame_in_progress;
}
uint8_t uart_get_byte() {
    uint8_t data = receive_buffer[receive_buf_start_pos++];
    if (receive_buf_start_pos == RECEIVE_BUFFER_LENGTH) {
    	receive_buf_start_pos = 0;
    }
    return data;
}

static inline void delay_ms(uint16_t ms) {
    uint32_t i;
    for (i = 0; i < ((F_CPU / 18000UL) * ms); i++)
        __asm__("nop");
}

static inline void uart_init() {
    // Set word length to 8 bit
    UART1->CR1 &= ~(1 << 4);
    // Number of stop bits
    UART1->CR3 &= ~(3 << 4);
    // Disable parity control
    UART1->CR1 &= ~(1 << 2);

    uint32_t brr;
    brr = F_CPU / UART_BAUD_RATE + 1;
    UART1->BRR2 = (brr & 0b1111) | ((brr >> 12) << 4);
    UART1->BRR1 = (brr >> 4) & 0xff;
    

    // Enable transmitter
    // Enable receiver
    // Enable receiver interrupts
    // Enable transmitter interrupts
    UART1->CR2 |= UART_CR2_TEN | UART_CR2_REN | UART_CR2_RIEN;
}

static inline void led_init() {
    GPIOB->DDR |= LED_PIN; // configure PD4 as output
    GPIOB->CR1 |= LED_PIN; // push-pull mode
}

static inline void led_off() {
    GPIOB->ODR |= LED_PIN;
}

static inline void led_on() {
    GPIOB->ODR &= ~LED_PIN;
}
//void uart_transmit_data_interrupt_handler(void) __interrupt(UART1_T_TXE_vector) {
    //if  (uart_transmit_data_register_empty()) {
    //    uart_transmit_byte();
    //}
//    UART_SR &= ~(UART_SR_TC | UART_SR_TXE);
//}

void uart_receive_data(void) __interrupt(UART1_RX_IRQn){
    if (UART1->SR & UART_SR_RXNE) {
    	store_byte(UART1->DR);
    }
}


void timer_init(void) {
    TIM4->CR1 = TIM_CR1_CEN;              
    TIM4->PSCR = 5; //prescaler set to 128
    TIM4->EGR = TIM_EGR_UG;
}

void process_knx_frame(uint8_t control_byte) {

}

void main() {
    uint8_t knx_ready = 0;
    
    uart_init();
    // Going to use 8-bit UART mode, frame end with silence, no CRC-CCITT
    // Need a timer to detect >=2.6ms silence
    // to indicate end of KNX frame received.
    timer_init();

    {__asm__("RIM\n");}

    uint8_t data = U_RESET_REQ;

    uart_send_bytes(1, &data);

    while (1) {
	if(uart_data_available() ){
	    uint8_t response = uart_get_byte();
            // process data from KNX module here.
	    if(response == U_RESET_IND) {
	        knx_ready = 1;
		led_on();
	    } else if(is_data_start_ind(response)) {
	        process_knx_frame(response);
                {__asm__("RIM\n");}
	    }
	} else {
            {__asm__("wfi\n");}
	}
    }
}

