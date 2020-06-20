#ifndef USART_H_
#define USART_H_

#include "power.h"

#define BAUD 9600
#define BAUD_PRESCALLER (((F_CPU / (BAUD * 16UL))) - 1)  /* The formula that does all the required maths. */

#include <avr/io.h>

/* Configure the USART. */
void USART_init(void);

/* All these functions works with polling (slow). */
void USART_send_byte(char data);
void USART_send_string(char* string);
char USART_receive_byte (void);

#endif /* USART_H_ */