#ifndef SPI_H_
#define SPI_H_

/*
SCK - Synchronous clock - PB5
MISO - Master Input Slave Output - PB4
MOSI - Master Output Slave Input - PB3
SS2 - Slave Select - PB2
SS1 - Slave Select - PB1
SS0 - Slave Select - PB0
*/
#define SCK 5
#define MISO 4
#define MOSI 3
#define SS2 2 /* It is the same for the slaves. */
#define SS1 1
#define SS0 0

#define DUMMY 0x00

#define CMD_SEND_BYTE 0xC0
#define ACK_SEND_BYTE 0xA0
#define CMD_RECEIVE_BYTE 0xC1
#define ACK_RECEIVE_BYTE 0xA1
#define CMD_SEND_FLOAT 0xC2
#define ACK_SEND_FLOAT 0xA2
#define CMD_RECEIVE_FLOAT 0xC3
#define ACK_RECEIVE_FLOAT 0xA3

#define CMD_COLLECT_EV 0xC4
#define ACK_COLLECT_EV 0xA4

#define CMD_COLLECT_IND 0xC5
#define ACK_COLLECT_IND 0xA5

#define CMD_SEND_IND 0xC6
#define ACK_SEND_IND 0xA6

#define CMD_COLLECT_BEST_IND 0xC7
#define ACK_COLLECT_BEST_IND 0xA7

#define CMD_CONTINUE_OPERATIONS 0xC8
#define ACK_CONTINUE_OPERATIONS 0xA7

#define CMD_SYNC 0xC9
#define ACK_SYNC 0xA9

/* This union is used to break a float in 4 individual bytes. */
typedef union {
	uint8_t bytes[sizeof(float)];
	float value;
} float_bytes;

typedef union {
	uint8_t bytes[1];
	uint8_t value;
} chromosome8_bytes;

typedef union {
	uint8_t bytes[2];
	uint16_t value;
} chromosome16_bytes;

typedef union {
	uint8_t bytes[4];
	uint32_t value;
} chromosome32_bytes;

/* Initialize the SPI master device. */
void SPI_master_init(void);

/* Initialize the SPI slave device. */
void SPI_slave_init(void);

/* These functions uses polling. */
void SPI_master_send_byte(uint8_t ss, uint8_t data);
uint8_t SPI_master_receive_byte(uint8_t ss);
void SPI_slave_send_byte(uint8_t data);
uint8_t SPI_slave_receive_byte(void);

void SPI_master_send_float(float data, uint8_t ss);
float SPI_master_receive_float(uint8_t ss);
void SPI_slave_send_float(float data);
float SPI_slave_receive_float(void);

#endif /* SPI_H_ */