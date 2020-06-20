#ifndef TEMPERATURE_H_
#define TEMPERATURE_H_

#include <stdint.h>

/* Initializes the temperature module */
void temperature_init(void);

/* Gets the internal temperature of the microcontroller */
uint32_t temperature_get(void);

#endif /* TEMPERATURE_H_ */