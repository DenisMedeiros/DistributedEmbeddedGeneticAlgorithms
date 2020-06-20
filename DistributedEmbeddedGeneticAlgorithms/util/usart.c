#include "usart.h"
#include <avr/interrupt.h>

/* Configure the USART. */
void USART_init(void) 
{
	/* Set baud rate */
	UBRR0H = (unsigned char) (BAUD_PRESCALLER >> 8);
	UBRR0L = (unsigned char) (BAUD_PRESCALLER);
	
	/* Enable receiver and transmitter */
	UCSR0B |= (1 << RXEN0) | (1 << TXEN0);
	
	/* Set frame format: 8 bits for data, 1 bit for stop bit */
	UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00) | (1 << UPM00) | (1 << UPM01);	
}

/* Send one byte (with polling). */
void USART_send_byte(char data)
{
	/* Wait for empty transmit buffer. */
	while(!(UCSR0A & (1 << UDRE0)));
	
	/* Put data into buffer, then sends the data. */
	UDR0 = data;	
}

/* Send one string (with polling). */
void USART_send_string(char* string){
	while(*string != '\0')
	{
		USART_send_byte(*string);
		string++;
	}
}

/* Receive one byte (with polling). */
char USART_receive_byte(void)
{
	/* Wait for data to be received */
	while (!(UCSR0A & (1 << RXC0)));
	
	/* Get and return received data from buffer */
	return UDR0;
}

