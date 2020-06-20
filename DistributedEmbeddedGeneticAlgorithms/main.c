#include "ga.h"
#include "util/usart.h"
#include "util/spi.h"
#include "util/power.h"
#include "random/lfsr.h"

#include <stdio.h>
#include <stdint.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#ifndef PI
#define PI 3.14159265358979323846
#endif

/* Slow-down factor for evaluation function. */
#ifndef REPEAT
#define REPEAT 8000
#endif

fitness_t evaluationFM2(normalization_t xn[])
{
	//return 21.5 + xn[0]*(sin(40*pi*xn[0]) + cos(20*pi*xn[0]) );
	//return 20 + pow(xn[0],2) + pow(xn[1],2) - 10*(cos(2*pi*xn[0]) + cos(2*pi*xn[1]));
	//return pow(xn[0]*xn[0] + xn[1]*xn[1], 0.5);
	//return 2*xn[0] + 3*xn[1] + 5;
	//return MINIMO + gainNorm * xn[0];
	
	//return (xn[0]-2)*(xn[0]-4); // f1
	//return xn[0]*xn[0] + xn[1]*xn[1]; // f2
	//return 2*xn[0] + 3*xn[1] + 5; // f3
	return 21.5 + xn[0]*(sin(40*PI*xn[0]) + cos(20*PI*xn[0])); // f4
	//return 10 + xn[0]*xn[0] - 10*cos(2*PI*xn[0]) + xn[1]*xn[1] - 10*cos(2*PI*xn[1]);
	
}

/* Slower version of evaluationFM. */
fitness_t evaluationFM(normalization_t xn[])
{
	uint16_t s;
	fitness_t result;
	result = 0.0;
	for(s = 0; s < REPEAT; s++)
	{
		result += (1.0/REPEAT) * (21.5 + xn[0]*(sin(40*PI*xn[0]) + cos(20*PI*xn[0]))); // content of function.
	}
	return result;
}

int main(void)
{
	chromosome_t population[NODE_POPULATION_SIZE][DIMENSION];
	fitness_t evaluation[NODE_POPULATION_SIZE];
	normalization_t normalizedChromosome[DIMENSION];
	popsize_t iBest;
	char output[100];
	
	/* Set ups the CPU prescaller. */
	power_init();
	
	_delay_ms(1000);
	
	/* Enables interrupts */
	sei();
	
	/* Initialize the USART module */
	USART_init();
	
	/* Initialize PD7 as output - used by external timer. */
	DDRD |= (1 << DDD7);
	
	/* Use internal temperature of each node as seed for LFSR. */
#if NODE_ID == 0	
	lfsr_srand8(101);
	lfsr_srand16(19207);
	lfsr_srand32(96233);
#else
	lfsr_srand8(241);
	lfsr_srand16(55733);
	lfsr_srand32(104729);
#endif


	/* Enable master or slave SPI config */
#if NODE_ID == 0
	SPI_master_init();
	//USART_send_string("[master] system starting...\n");
#else
	SPI_slave_init();
	//USART_send_string("[slave] system starting...\n");
#endif

	_delay_ms(100);

	while(1)
	{	
		/* Set to high on PD7 so start external timer. */
		PORTD |= (1 << DDD7);
			
		/* Every node must run the GA */
		iBest = geneticAlgorithmFM(evaluation, population);
		
		/* Set to low on PD7 so start external timer. */
		PORTD &= ~(1 << DDD7);	
		
		/* Every node normalize their best result */
		normalizationFM(population[iBest], normalizedChromosome);
		
		/* Send result through USART connection */
#if NODE_ID == 0
		sprintf(output, "[master] index = %d, value = %f, \n", iBest, normalizedChromosome[0]);
		//sprintf(output, "[master] index = %d, value = (%f, %f)\n", iBest, normalizedChromosome[0], normalizedChromosome[1]);
#else
		//sprintf(output, "[slave] index = %d, value = %f\n", iBest, normalizedChromosome[0]);
		//sprintf(output, "[slave] index = %d, value = (%f, %f)\n", iBest, normalizedChromosome[0], normalizedChromosome[1]);		
#endif
		
		USART_send_string(output);
		
		//USART_send_string("\n---\n");
		
		/* Busy wait 500 ms */
		_delay_ms(2000);
		
	
		
	}
	
	return 0;
}

