#include <avr/io.h>
#include "spi.h"
#include <stdio.h>
#include "usart.h"
#include <util/delay.h>

/* Initialize the SPI master device. */
void SPI_master_init(void)
{
	/* Set SS's (PB0, PB1 and PB2), MOSI (PB3) and SCK (PB5) as output. */
	DDRB |= (1 << SS0) | (1 << SS1) | (1 << SS2) | (1 << MOSI) | (1 << SCK);
	
	/* Enable SPI, set up master, and set clock rate (divide by 128) */
	SPCR |= (1 << SPE) | (1 << MSTR) | (1 << SPR0) | (1 << SPR1);
	
	/* Disable all slaves by default (signal high = disable) */
	PORTB |= (1 << SS0) | (1 << SS1) | (1 << SS2);
}

/* Initialize the SPI slave device. */
void SPI_slave_init(void)
{
	/* Set MISO output */
	DDRB |= (1 << MISO);
	
	/* Enable SPI. */
	SPCR |= (1 << SPE);
}

void SPI_master_send_byte(uint8_t ss, uint8_t data)
{
	/* Enable the selected slave */
	PORTB &= !(1 << ss);
	
	while(1)
	{
		/* Send command and receive a response. */
		SPDR = CMD_SEND_BYTE;
		while(!(SPSR & (1 << SPIF)));
		
		SPDR = DUMMY;
		while(!(SPSR & (1 << SPIF)));
		
		/* Check if it received a ACK. */
		if(SPDR == ACK_SEND_BYTE)
		{
			break;
		}
		
		/* Delay necessary to not break the SPI (during polling). */
		_delay_ms(10);
	}
	
	/* Send the useful data */
	SPDR = data;
	while(!(SPSR & (1 << SPIF)));
	
	/* Disable the selected slave */
	PORTB |= (1 << ss);
}

uint8_t SPI_master_receive_byte(uint8_t ss)
{
	uint8_t data;
	
	/* Enable the selected slave */
	PORTB &= !(1 << ss);

	while(1)
	{
		/* Send command and receive a response. */
		SPDR = CMD_RECEIVE_BYTE;
		while(!(SPSR & (1 << SPIF)));
	
		SPDR = DUMMY;
		while(!(SPSR & (1 << SPIF)));
	
		/* Check if it received a ACK. */
		if(SPDR == ACK_RECEIVE_BYTE)
		{
			break;
		}
	
		/* Delay necessary to not break the SPI (during polling). */
		_delay_ms(10);
	}

	/* Send the useful data */
	SPDR = DUMMY;
	while(!(SPSR & (1 << SPIF)));
	data = SPDR;
	
	/* Disable the selected slave */
	PORTB |= (1 << ss);
	
	return data;
}
void SPI_slave_send_byte(uint8_t data)
{
	uint8_t command;
	
	command = 0x00;
	while(command != CMD_RECEIVE_BYTE)
	{
		while(!(SPSR & (1 << SPIF)));
		command = SPDR;
	}
	
	/* Send the ACK. */
	SPDR = ACK_RECEIVE_BYTE;
	while(!(SPSR & (1 << SPIF)));
	
	/* Read dummy byte (sent my master to receive the ack). */
	command = SPDR;
	
	SPDR = data;
	while(!(SPSR & (1 << SPIF)));
}

uint8_t SPI_slave_receive_byte(void)
{
	uint8_t command;
	
	command = 0x00;
	while(command != CMD_SEND_BYTE)
	{
		while(!(SPSR & (1 << SPIF)));
		command = SPDR;
	}
		
	/* Send the ACK. */
	SPDR = ACK_SEND_BYTE;
	while(!(SPSR & (1 << SPIF)));
		
	/* Read dummy byte (sent my master to receive the ack). */
	command = SPDR;
	
	while(!(SPSR & (1 << SPIF)));
	return SPDR;
}

void SPI_master_send_float(float data, uint8_t ss)
{
	float_bytes sent;
	uint8_t b;
	sent.value = data;
	
	/* Enable the selected slave */
	PORTB &= !(1 << ss);
	
	while(1)
	{		
		/* Send command and receive a response. */
		SPDR = CMD_SEND_FLOAT;
		while(!(SPSR & (1 << SPIF)));
		
		SPDR = DUMMY;
		while(!(SPSR & (1 << SPIF)));
		
		/* Check if it received a ACK. */
		if(SPDR == ACK_SEND_FLOAT)
		{
			break;
		}
		
		/* Delay necessary to not break the SPI (during polling). */
		_delay_ms(5);
	}
	
	/* Now send the 4 bytes. */
	for(b = 0; b < sizeof(float); b++)
	{
		SPDR = sent.bytes[b];
		while(!(SPSR & (1 << SPIF)));
	}
	
	/* Disable the selected slave */
	PORTB |= (1 << ss);
}

float SPI_master_receive_float(uint8_t ss)
{
	float_bytes received;
	uint8_t b;
		
	/* Enable the selected slave */
	PORTB &= !(1 << ss);
		
	while(1)
	{
		/* Send command and receive a response. */
		SPDR = CMD_RECEIVE_FLOAT;
		while(!(SPSR & (1 << SPIF)));
			
		SPDR = DUMMY;
		while(!(SPSR & (1 << SPIF)));
			
		/* Check if it received a ACK. */
		if(SPDR == ACK_RECEIVE_FLOAT)
		{
			break;
		}
			
		/* Delay necessary to not break the SPI (during polling). */
		_delay_ms(5);
			
	}
		
	/* Now receive the 4 bytes. */
	for(b = 0; b < sizeof(float); b++)
	{
		SPDR = DUMMY;
		while(!(SPSR & (1 << SPIF)));
		received.bytes[b] = SPDR;
	}
		
	/* Disable the selected slave */
	PORTB |= (1 << ss);
	
	return received.value;
}

void SPI_slave_send_float(float data)
{
	float_bytes sent;
	uint8_t b;
	uint8_t command;
	
	sent.value = data;
	
	command = 0x00;
	while(command != CMD_RECEIVE_FLOAT)
	{
		while(!(SPSR & (1 << SPIF)));
		command = SPDR;
	}
	
	/* Send the ACK. */
	SPDR = ACK_RECEIVE_FLOAT;
	while(!(SPSR & (1 << SPIF)));
	
	/* Read dummy byte (sent my master to receive the ack). */
	command = SPDR;
	
	/* Finally, send the 4 bytes. */
	for(b = 0; b < sizeof(float); b++)
	{
		/* Wait for transmission to complete */
		SPDR = sent.bytes[b];
		while(!(SPSR & (1 << SPIF)));
	}
}

float SPI_slave_receive_float(void)
{
	float_bytes received;
	uint8_t b;
	uint8_t command;
	
	command = 0x00;
	while(command != CMD_SEND_FLOAT)
	{
		while(!(SPSR & (1 << SPIF)));
		command = SPDR;
	}
	
	/* Send the ACK. */
	SPDR = ACK_SEND_FLOAT;
	while(!(SPSR & (1 << SPIF)));
	
	/* Read dummy byte (sent my master to receive the ack). */
	command = SPDR;
	
	/* Finally, receive the 4 bytes. */
	for(b = 0; b < sizeof(float); b++)
	{
		/* Wait for transmission to complete */
		while(!(SPSR & (1 << SPIF)));
		received.bytes[b] = SPDR;
	}
	
	/* Return the received float */
	return received.value;
}