#include <stdint.h>

#include<stm8s003.h>

#include "ncn5120.h"

typedef uint16_t timer;

#define F_CPU 2000000UL

// Need thins constant to adjust some addresses and timings to work
// in ucsim simulator.
// Should be undefined before flushing to target device.
#define UCSIM

// *********  LED settings *******************
#define LED_PIN     GPIO_ODR_5

// ********* UART settings *****
#define UART_BAUD_RATE 19200


// ******** KNX constants ********************
#ifdef UCSIM
#define KNX_FRAME_SILENCE_THRESHOLD 12
#define TIMER_PRESCALER 15
// ucsim does not support WFI at the moment of writing this code.
#define WAIT_INSTRUCTION "nop\n"
#else
// Threshold should be 2.6ms
// We calculate it on timer with prescaler 128 and closk frequency 2000000 (F_CPU)
#define KNX_FRAME_SILENCE_THRESHOLD (0.0026 * (F_CPU >> 7))
#define TIM2_TYPE2
#define TIMER_PRESCALER 7
#define WAIT_INSTRUCTION "wfi\n"
#endif //UCSIM

#define RECEIVE_BUFFER_LENGTH 100
#define SEND_BUFFER_LENGTH 100

volatile uint8_t receive_buffer[RECEIVE_BUFFER_LENGTH];
volatile uint8_t receive_buf_end_pos = 0;
volatile uint8_t receive_knx_frame_in_progress = 0;

volatile uint8_t send_buffer[SEND_BUFFER_LENGTH];
volatile uint8_t send_buf_start_pos = 0;
volatile uint8_t send_buf_end_pos = 0;

inline timer get_time(void) {
    return TIM2->CNTRH << 8 | TIM2->CNTRL;
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

void process_knx_frame(uint8_t* data, uint8_t length) {
    UART1->DR = length;
}

uint8_t knx_ready = 0;

void store_byte(uint8_t data) {
    
    if (receive_buf_end_pos == RECEIVE_BUFFER_LENGTH) {
    	receive_buf_end_pos = 0;
    }

    if(!receive_knx_frame_in_progress) {
	if(data == U_RESET_IND) {
	    knx_ready = 1;
	    led_on();
	} else if(is_data_start_ind(data)) {
            receive_buf_end_pos = 0;
	    receive_buffer[receive_buf_end_pos++] = data;
	    receive_knx_frame_in_progress = 1;
            last_b = get_time();
	}
    } else {
	receive_buffer[receive_buf_end_pos++] = data;
        if(get_time() - last_b > KNX_FRAME_SILENCE_THRESHOLD) {
            receive_knx_frame_in_progress = 0;
            process_knx_frame(receive_buffer, receive_buf_end_pos);
	    receive_buf_end_pos = 0;
        } else {
            last_b = get_time();
	}
    }
}

uint8_t uart_data_available() {
    return receive_buf_end_pos != 0 && !receive_knx_frame_in_progress;
}

uint8_t uart_get_byte() {
    return receive_buffer[0];
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

void uart_receive_data(void) __interrupt(UART1_RX_IRQn){
    if (UART1->SR & UART_SR_RXNE) {
    	store_byte(UART1->DR);
    }
}


void timer_init(void) {
    TIM2->PSCR = TIMER_PRESCALER; 
    TIM2->EGR |= TIM_EGR_UG;
    TIM2->CR1 |= TIM_CR1_CEN;              
}

void main() {
    
    uart_init();
    // Going to use 8-bit UART mode, frame end with silence, no CRC-CCITT
    // Need a timer to detect >=2.6ms silence
    // to indicate end of KNX frame received.
    timer_init();

    {__asm__("RIM\n");}

    uint8_t data = U_RESET_REQ;

    uart_send_bytes(1, &data);

    while (1) {
        {__asm__(WAIT_INSTRUCTION);}
    }
}

