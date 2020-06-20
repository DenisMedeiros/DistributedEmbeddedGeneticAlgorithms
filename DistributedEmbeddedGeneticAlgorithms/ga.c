#include "ga.h"

/* Choose one of the random generators. */

#include "random/lfsr.h"
#include "util/usart.h"
#include "util/spi.h"
#include <math.h>
#include <stdio.h>
#include <util/delay.h>

/** 
 * This is the core function of the genetic algorithm. It runs all the modules
 * and finds the best possible solution.
 *
 * @param evaluation A vector that will store the fitness values for the individuals.
 * @param population A vector containing the individuals.
 * @return The index of the best individual in the population after the last generation.
 */
popsize_t geneticAlgorithmFM(fitness_t evaluation[], chromosome_t population[][DIMENSION])
{
	popsize_t iBest;
	generationsize_t k;
	
#if NODE_ID == 0
	/* These variables are used to store and verify what is the best individual. */
	chromosome_t bestIndividuals[NUM_NODES][DIMENSION];
	normalization_t normalizedChromosome[DIMENSION];
	chromosome_t bestIndividual[DIMENSION];
	fitness_t bestFitnessValue;
	fitness_t fitnessValue;
	dimensionsize_t j;
	
#endif 		

	/* Synchronize before begin. */
	spi_data_t data;
	
#if NODE_ID == 0
	
	while(1)
	{
		/* Send command and receive a response. */
		PORTB &= !(1 << 2);
		
		SPDR = CMD_SYNC;
		while(!(SPSR & (1 << SPIF)));
		data = SPDR;
		
		_delay_ms(1);

		SPDR = DUMMY;
		while(!(SPSR & (1 << SPIF)));
		data = SPDR;
		
		PORTB |= (1 << 2);
		
		_delay_ms(1);
		
		/* Check if it received a ACK. */
		if(data == ACK_SYNC)
		{
			_delay_ms(1);
			break;
			
		}
	}
	
#else

	while(1)
	{
		while(!(SPSR & (1 << SPIF)));
		data = SPDR;
		
		if(data == CMD_SYNC)
		{
			SPDR = ACK_SYNC ;
			while(!(SPSR & (1 << SPIF)));
			break;
		}
	}

#endif


	/* Initializes the population */
	initializationFM(population);
	
	/* Calculates the fitness for all individuals and save the best individual index */
	iBest = fitnessFM(evaluation, population);
		
	for(k = 0; k < NUM_GENERATIONS; k++)
	{
		
				/* Generates a new population */
		newPopulationFM(evaluation, population, iBest);
		
		/* Calculates the fitness again and saves the best individual */
		iBest = fitnessFM(evaluation, population);			
	}
	

	
	/* At this point, the GA already finished. Therefore, collect the best 
	individual of each salve to see what is the best one between all uCs. */
	
#if NODE_ID == 0 /* Master */

	/* Collect the best individuals of all microcontrollers. */
	collectBestIndividualsFM(bestIndividuals, population, iBest);
	
	/* Define the best individual (from master) as the best one. */
	for(j = 0; j < DIMENSION; j++)
	{
		bestIndividual[j] = bestIndividuals[0][j];
	}
	
	normalizationFM(bestIndividual, normalizedChromosome);
	bestFitnessValue = evaluationFM(normalizedChromosome);
	
	for(k = 1; k < NUM_NODES; k++)
	{
		normalizationFM(bestIndividuals[k], normalizedChromosome);
		fitnessValue = evaluationFM(normalizedChromosome);
		if(fitnessValue < bestFitnessValue)
		{
			for(j = 0; j < DIMENSION; j++)
			{	
				/* If a better individual was found, replace the best one in the master. */
				bestIndividual[j] = bestIndividuals[k][j];
			}

			/* Update the best individual aux variables. */
			normalizationFM(bestIndividual, normalizedChromosome);
			bestFitnessValue = evaluationFM(normalizedChromosome);
		}
	}
	
	/* Put the best individual in position iBest of the master. */
	for(j = 0; j < DIMENSION; j++)
	{
		population[iBest][j] = bestIndividual[j];	
	}
	
#else /* Slave */

	waitSendBestIndividuaFM(population, iBest);

#endif
	
	return iBest;
}

/** 
 * This function initializes the population with random individuals. It should be called once.
 *
 * @param population A vector containing the individuals.
 */
void initializationFM(chromosome_t population[][DIMENSION])
{	
	popsize_t i;
	dimensionsize_t j;
	
	for(i = 0; i < NODE_POPULATION_SIZE; i++)
	{
		for(j = 0; j < DIMENSION; j++)
		{
			/* Each individual is a random integer number */
#if CHROMOSOME_SIZE == 32
			population[i][j] = lfsr_rand32();		
#elif CHROMOSOME_SIZE == 16
			population[i][j] = lfsr_rand16();		
#else
			population[i][j] = lfsr_rand8();	
#endif
		}
	}
}

/** 
 * This function calculates the fitness value for the whole population. Before that, the
 * individuals are normalized to a range where the solution might be in.
 *
 * @param evaluation A vector that will store the fitness values for the individuals.
 * @param population A vector containing the individuals.
 * @return The index of the best individual in the population after the last generation.
 */
popsize_t fitnessFM(fitness_t evaluation[], chromosome_t population[][DIMENSION])
{
	popsize_t i;
	popsize_t iBest;
	normalization_t normalizedChromosome[DIMENSION];
	
	for (i = 0, iBest = 0; i < NODE_POPULATION_SIZE; i++)
	{
		/* First, normalize the individual */
		normalizationFM(population[i], normalizedChromosome);
				
		/* The evaluation function (evaluationFM) must be defined by the user */
		evaluation[i] = evaluationFM(normalizedChromosome); 
		
		if (evaluation[i] < evaluation[iBest])
		{
			iBest = i;
		}	
	}
	return iBest;
}

/** 
 * This function normalizes an individual to an specified range (between NORMALIZATION_MIN and NORMALIZATION_MAX).
 *
 * @param chromesome A individual of the population.
 * @param normalizedChromesome The normalized individual.
 */
void normalizationFM(chromosome_t chromesome[], normalization_t normalizedChromesome[])
{
	dimensionsize_t j;
	for(j = 0; j < DIMENSION; j++)
	{
	    /* Applies a normalized gain on the chromosome */
		normalizedChromesome[j] = NORMALIZATION_MIN + ((normalization_t) NORMALIZED_GAIN) * chromesome[j];
	}

}

/** 
 * This function generates a new population from a previous one. It does this by selecting and crossing
 * some individuals and applying mutation over some of them. In the and, it updates the current population
 * by the newer one.
 *
 * @param evaluation A vector that stores the fitness values of the individuals.
 * @param population A vector containing the individuals.
 * @param The index of the best individual in the population after the generation of the new population.
 */
void newPopulationFM(fitness_t evaluation[], chromosome_t population[][DIMENSION], popsize_t iBest)
{
	chromosome_t newPopulation[NODE_POPULATION_SIZE][DIMENSION];
	
	selectionCrossoverProcessing(evaluation, population, newPopulation);
	
	/* Applies the mutation over some individuals */
	mutationFM(newPopulation);
	
	/* Replaces the old population by the new one */
	updateFM(population, newPopulation, iBest);
}

/** 
 * This function mutates some individuals of the population.
 *
 * @param newPopulation A vector containing the individuals.
 */
void mutationFM(chromosome_t newPopulation[][DIMENSION])
{
	popsize_t i;
	dimensionsize_t j;
	chromosomesize_t bitPosition;
	
	/* The best individual will not be mutated */
	for(i = 1; i <=  NODE_MUTATED_INDIVIDUALS; i++)
	{
		
		for(j = 0; j < DIMENSION; j++)
		{
			bitPosition = lfsr_rand8() & (CHROMOSOME_SIZE-1);	
			
#if CHROMOSOME_SIZE == 32
			if(bitPosition < 8) /* First byte. */
			{
				newPopulation[i][j] = newPopulation[i][j] ^ (0x00000001 << bitPosition);
			}
			else if (bitPosition < 16) /* Second byte. */
			{
				newPopulation[i][j] = newPopulation[i][j] ^ (0x00000100 << (bitPosition - 8));
			}
			else if (bitPosition < 24) /* Third byte. */
			{
				newPopulation[i][j] = newPopulation[i][j] ^ (0x00010000 << (bitPosition - 16));
			}
			else /* Fourth byte. */
			{
				newPopulation[i][j] = newPopulation[i][j] ^ (0x01000000 << (bitPosition - 24));
			}
#elif CHROMOSOME_SIZE == 16
			if(bitPosition < 8) /* First byte. */
			{
				newPopulation[i][j] = newPopulation[i][j] ^ (0x0001 << bitPosition);
			}
			else /* Second byte. */
			{
				newPopulation[i][j] = newPopulation[i][j] ^ (0x0100 << (bitPosition - 8));
			}
#else	
			newPopulation[i][j] = newPopulation[i][j] ^ (0x01 << bitPosition);
#endif
		}
	}
}

/** 
 * This function replaced the old population by the new one.
 *
 * @param population A vector containing the old individuals.
 * @param newPopulation A vector containing the new individuals.
 * @param iBest The index of the best individual in the population.
 */
void updateFM(chromosome_t population[][DIMENSION], chromosome_t newPopulation[][DIMENSION], popsize_t iBest)
{
	popsize_t i;
	dimensionsize_t j;
	/* Put the best individual of the new population in the first position of the old population. */
	for(j = 0; j < DIMENSION; j++)
	{
		population[0][j] = population[iBest][j];
	}
	
	/* Replace the old population by the new one */
	for(i = 1; i < NODE_POPULATION_SIZE; i++)
	{
		for(j = 0; j < DIMENSION; j++)
		{
			population[i][j] = newPopulation[i][j];
		}
	}
}

/** 
 * This function transfer one individual from slave to master.
 *
 * @param x A vector representing a multi-dimension individual.
 * @param nodeId The id of the node to collect the individual from.
 * @param index The index of the individual to be collected.
 */
void collectIndividualFM(chromosome_t x[], slave_t  nodeId, popsize_t index)
{
	dimensionsize_t j;
	slave_select_t ss;
	spi_data_t data;
		
	/* Define ss based on nodeId. */
	ss = 3 - nodeId;
	
	/* Enable the selected slave */
	while(1)
	{
		PORTB &= !(1 << ss);
		
		/* Send command and receive a response. */
		SPDR = CMD_COLLECT_IND;
		while(!(SPSR & (1 << SPIF)));
		data = SPDR;
		
		_delay_ms(1);
		SPDR = DUMMY;
		while(!(SPSR & (1 << SPIF)));
		data = SPDR;
		
		PORTB |= (1 << ss);
		
		/* Check if it received a ACK. */
		if(data == ACK_COLLECT_IND)
		{
			break;
		}
	}
	
	/* Now, send index. */
	_delay_ms(1);
	PORTB &= !(1 << ss);
	
	SPDR = index;
	while(!(SPSR & (1 << SPIF)));
	/* Read dummy data. */
	data = SPDR;
	_delay_ms(1);
	/* Now receive the individual. */
	
#if CHROMOSOME_SIZE == 8

	for(j = 0; j < DIMENSION; j++)
	{
		SPDR = DUMMY;
		while(!(SPSR & (1 << SPIF)));
		x[j] = SPDR;
	}
	
#elif CHROMOSOME_SIZE == 16

	chromosome16_bytes temp;
	dimensionsize_t k;
	
	for(j = 0; j < DIMENSION; j++)
	{
		_delay_ms(1);
		for(k = 0; k < 2; k++)
		{
			SPDR = DUMMY;
			while(!(SPSR & (1 << SPIF)));
			temp.bytes[k] = SPDR;
		}
		
		x[j] = temp.value;
	}
	
#else
	chromosome32_bytes temp;
	dimensionsize_t k;
	
	for(j = 0; j < DIMENSION; j++)
	{
		_delay_ms(1);
		for(k = 0; k < 4; k++)
		{
			SPDR = DUMMY;
			while(!(SPSR & (1 << SPIF)));
			temp.bytes[k] = SPDR;
		}
		
		x[j] = temp.value;
	}

#endif	

	/* Disable the selected slave */
	PORTB |= (1 << ss);
}



/** 
 * This function transfer the evaluation of a individual from slave to master.
 *
 * @param nodeId The id of the node to collect the fitness value.
 * @param index The index of the fitness value to be collected.
 * @returns The fitness value collected.
 */
float collectEvaluationFM(slave_t nodeId, popsize_t index)
{
	slave_select_t ss;
	dimensionsize_t b;
	float_bytes received;
	spi_data_t data;
	
	/* Define ss based on nodeId. */
	ss = 3 - nodeId;
	
	while(1)
	{
		
		/* Enable the selected slave */
		PORTB &= !(1 << ss);
		
		/* Send command and receive a response. */
		SPDR = CMD_COLLECT_EV;
		while(!(SPSR & (1 << SPIF)));		
		data = SPDR;
		
		_delay_ms(1);
		SPDR = DUMMY;
		while(!(SPSR & (1 << SPIF)));
		data = SPDR;
		
		/* Disable the selected slave */
		PORTB |= (1 << ss);
		
		/* Check if it received a ACK. */
		if(data == ACK_COLLECT_EV)
		{
			break;
		}
		
	}
	
	/* Now, send index. */
	_delay_ms(1);
	/* Enable the selected slave */
	PORTB &= !(1 << ss);
	SPDR = index;
	while(!(SPSR & (1 << SPIF)));
	/* Read dummy data. */
	data = SPDR;
	
	_delay_ms(1);
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

/** 
 * This function transfer one individual from master to slave.
 * It will be stored in the next available position.
 * @param x A vector representing a multi-dimension individual.
 * @param nodeId The id of the node to collect the individual from.
 */
void sendIndividualFM(chromosome_t x[], slave_t nodeId)
{
	dimensionsize_t j;
	slave_select_t ss;
	spi_data_t data;
	
	/* Define ss based on nodeId. */
	ss = 3 - nodeId;
	
	/* Enable the selected slave */
	
	while(1)
	{
		
		PORTB &= !(1 << ss);
		
		/* Send command and receive a response. */
		SPDR = CMD_SEND_IND;
		while(!(SPSR & (1 << SPIF)));
		data = SPDR;
		
		_delay_ms(1);
		SPDR = DUMMY;
		while(!(SPSR & (1 << SPIF)));
		data = SPDR;
		
		PORTB |= (1 << ss);
		
		/* Check if it received a ACK. */
		if(data == ACK_SEND_IND)
		{
			break;
		}
		
	}
	
	_delay_ms(1);
	/* Now send the individual. */
	PORTB &= !(1 << ss);
	
#if CHROMOSOME_SIZE == 8

	for(j = 0; j < DIMENSION; j++)
	{
		SPDR = x[j];
		while(!(SPSR & (1 << SPIF)));
		data = SPDR;
	}

#elif CHROMOSOME_SIZE == 16

	chromosome16_bytes temp;
	dimensionsize_t k;
	
	for(j = 0; j < DIMENSION; j++)
	{
		temp.value = x[j];
		_delay_ms(1);
		for(k = 0; k < 2; k++)
		{
			SPDR = temp.bytes[k];
			while(!(SPSR & (1 << SPIF)));
			data = SPDR; // Clean up the buffer.
		}
	}	
	
#else

	chromosome32_bytes temp;
	dimensionsize_t k;
	
	for(j = 0; j < DIMENSION; j++)
	{
		temp.value = x[j];
		_delay_ms(1);
		for(k = 0; k < 4; k++)
		{
			SPDR = temp.bytes[k];
			while(!(SPSR & (1 << SPIF)));
			data = SPDR; // Clean up the buffer.
		}
	}

#endif

	
	/* Disable the selected slave */
	PORTB |= (1 << ss);
	
}

#if NODE_ID == 0

void collectBestIndividualsFM(chromosome_t bestIndividuals[][DIMENSION], chromosome_t population[][DIMENSION], popsize_t iBest)
{
	popsize_t i;
	dimensionsize_t j;
	slave_select_t ss;
	spi_data_t data;
	
	/* Grab the best from the master. */
	for(j = 0; j < DIMENSION; j++)
	{
		bestIndividuals[iBest][j] = population[0][j];
	}
	
	/* Collect the best from the other slaves. */
	for(i = 1; i < NUM_NODES; i++)
	{
		//collectIndividualFM(bestIndividuals[i], i, 0);
		
		/* Define ss based on nodeId. */
		ss = 3 - i;
		
		/* Enable the selected slave */
		while(1)
		{					
			/* Send command and receive a response. */
			PORTB &= !(1 << ss);
			
			SPDR = CMD_COLLECT_BEST_IND;
			while(!(SPSR & (1 << SPIF)));
			data = SPDR;
			
			_delay_ms(1);

			SPDR = DUMMY;
			while(!(SPSR & (1 << SPIF)));
			data = SPDR;
			
			PORTB |= (1 << ss);	
		
			_delay_ms(1);
						
			/* Check if it received a ACK. */
			if(data == ACK_COLLECT_BEST_IND)
			{
				break;
			}
		}
		
		/* Now receive the individual. */
#if CHROMOSOME_SIZE == 8

		for(j = 0; j < DIMENSION; j++)
		{
			PORTB &= !(1 << ss);
			SPDR = DUMMY;
			while(!(SPSR & (1 << SPIF)));
			PORTB |= (1 << ss);	
			bestIndividuals[i][j] = SPDR;
		}
	
#elif CHROMOSOME_SIZE == 16

		chromosome16_bytes temp;
		dimensionsize_t k;
	
		for(j = 0; j < DIMENSION; j++)
		{
			_delay_ms(1);
			for(k = 0; k < 2; k++)
			{
				PORTB &= !(1 << ss);
				SPDR = DUMMY;
				while(!(SPSR & (1 << SPIF)));
				PORTB |= (1 << ss);	
				temp.bytes[k] = SPDR;
			}
		
			bestIndividuals[i][j] = temp.value;
		}
	
#else
		chromosome32_bytes temp;
		dimensionsize_t k;
	
		for(j = 0; j < DIMENSION; j++)
		{
			_delay_ms(1);
			for(k = 0; k < 4; k++)
			{
				PORTB &= !(1 << ss);
				SPDR = DUMMY;
				while(!(SPSR & (1 << SPIF)));
				PORTB |= (1 << ss);	
				temp.bytes[k] = SPDR;
			}
		
			bestIndividuals[i][j] = temp.value;
		}

#endif
	}
}


/* This function is run only by the master. It controls the selection and crossover of all nodes. */
void selectionCrossoverProcessing(fitness_t evaluation[], chromosome_t population[][DIMENSION], chromosome_t newPopulation[][DIMENSION])
{
	// popsize_t internalCounter = 0;
	popsize_t iChromosomeX1, iChromosomeX2, iChromosomeY1, iChromosomeY2, iWinnerX, iWinnerY;
	fitness_t fitnessX1, fitnessX2, fitnessY1, fitnessY2;
	slave_t nodeChromosomeX1, nodeChromosomeX2, nodeChromosomeY1, nodeChromosomeY2, nodeWinnerX, nodeWinnerY;
	chromosome_t winnerX[DIMENSION], winnerY[DIMENSION], newIndX[DIMENSION], newIndY[DIMENSION];
	popsize_t i;
	dimensionsize_t j;
	
	for(i = 0; i < POPULATION_SIZE; i += 2) /* Process the whole population. */
	{
		/* Randomly pick 4 individuals (2 winners to generate 2 new individuals). */
		
#if POPULATION_SIZE < 256
		iChromosomeX1 = lfsr_rand8() & (POPULATION_SIZE - 1);
		iChromosomeX2 = lfsr_rand8() & (POPULATION_SIZE - 1);
		iChromosomeY1 = lfsr_rand8() & (POPULATION_SIZE - 1);
		iChromosomeY2 = lfsr_rand8() & (POPULATION_SIZE - 1);
#else
		iChromosomeX1 = lfsr_rand16() & (POPULATION_SIZE - 1);
		iChromosomeX2 = lfsr_rand16() & (POPULATION_SIZE - 1);
		iChromosomeY1 = lfsr_rand16() & (POPULATION_SIZE - 1);
		iChromosomeY2 = lfsr_rand16() & (POPULATION_SIZE - 1);
#endif		

		/* Depending on the index, grab the individual from the respective microcontroller. */
#if NUM_NODES == 2
		if(iChromosomeX1 < POPULATION_SIZE/2)
		{
			nodeChromosomeX1 = 0;
			/* iChromosomeX remains the same value. */
		}
		else
		{
			nodeChromosomeX1 = 1;
			iChromosomeX1 -= POPULATION_SIZE/2; /* Calculate the relative index. */
		}
		if(iChromosomeX2 < POPULATION_SIZE/2)
		{
			nodeChromosomeX2 = 0;
			/* iChromosomeX remains the same value. */
		}
		else
		{
			nodeChromosomeX2 = 1;
			iChromosomeX2 -= POPULATION_SIZE/2; /* Calculate the relative index. */
		}
		if(iChromosomeY1 < POPULATION_SIZE/2)
		{
			nodeChromosomeY1 = 0;
			/* iChromosomeY remains the same value. */
		}
		else
		{
			nodeChromosomeY1 = 1;
			iChromosomeY1 -= POPULATION_SIZE/2; /* Calculate the relative index. */
		}
		if(iChromosomeY2 < POPULATION_SIZE/2)
		{
			nodeChromosomeY2 = 0;
			/* iChromosomeY remains the same value. */
		}
		else
		{
			nodeChromosomeY2 = 1;
			iChromosomeY2 -= POPULATION_SIZE/2; /* Calculate the relative index. */
		}		
		
		/* At this point, nodes and indexes were identified. Then, grab the evaluation values for each one. */

		if (nodeChromosomeX1 == 0)
		{
			fitnessX1 = evaluation[iChromosomeX1];
		}
		else
		{
			fitnessX1 = collectEvaluationFM(nodeChromosomeX1, iChromosomeX1);
		}
		
		if (nodeChromosomeX2 == 0)
		{
			fitnessX2 = evaluation[iChromosomeX2];
		}
		else
		{
			fitnessX2 = collectEvaluationFM(nodeChromosomeX2, iChromosomeX2);
		}
		
		if (nodeChromosomeY1 == 0)
		{
			fitnessY1 = evaluation[iChromosomeY1];
		}
		else
		{
			fitnessY1 = collectEvaluationFM(nodeChromosomeY1, iChromosomeY1);
		}
		
		if (nodeChromosomeY2 == 0)
		{
			fitnessY2 = evaluation[iChromosomeY2];
		}
		else
		{
			fitnessY2 = collectEvaluationFM(nodeChromosomeY2, iChromosomeY2);
		}

		/* Now, do the tournament method. */
		if (fitnessX1 < fitnessX2) {
			iWinnerX = iChromosomeX1;
			nodeWinnerX = nodeChromosomeX1;
		}
		else
		{
			iWinnerX = iChromosomeX2;
			nodeWinnerX = nodeChromosomeX2;
		}
		
		if (fitnessY1 < fitnessY2) {
			iWinnerY = iChromosomeY1;
			nodeWinnerY = nodeChromosomeY1;
		}
		else
		{
			iWinnerY = iChromosomeY2;
			nodeWinnerY = nodeChromosomeY2;
		}
		
		/* If the node is the master, grab the individual directly. */
		
		if (nodeWinnerX == 0)
		{
			for(j = 0; j < DIMENSION; j++)
			{
				winnerX[j] = population[iWinnerX][j];
			}
		}
		else
		{
			collectIndividualFM(winnerX, nodeWinnerX, iWinnerX);
		}		
		
		if (nodeWinnerY == 0)
		{
			for(j = 0; j < DIMENSION; j++)
			{
				winnerY[j] = population[iWinnerY][j];
			}
		}
		else
		{
			collectIndividualFM(winnerY, nodeWinnerY, iWinnerY);
		}
	
		/* Do the crossover of the individuals. */
		for(j = 0; j < DIMENSION; j++)
		{
			/* Two chromosomes exchanges their genes between each other */
			newIndX[j] = (winnerX[j] & MASK) | (winnerY[j] & ~MASK);
			newIndY[j] = (winnerX[j] & ~MASK) | (winnerY[j] & MASK);
		}
				
		/* Store the new individuals. */
		if (i < POPULATION_SIZE/2) /* If replacing the first half (= master). */
		{
			for(j = 0; j < DIMENSION; j++)
			{
				newPopulation[i][j] = newIndX[j];
				newPopulation[i+1][j] = newIndY[j];
			}
		}
		else /* Remote uC. */
		{
			sendIndividualFM(newIndX, 1);
			sendIndividualFM(newIndY, 1);
		}
				
#elif NUM_NODES == 4
	/* TODO */
#endif

	}
	
	/* Continue operation in all slaves. */
	for(i = 1; i < NUM_NODES; i++)
	{
		continueOperationsFM(i);
	}
}

void continueOperationsFM(slave_t nodeId)
{	
	slave_select_t ss;
	spi_data_t data;
	
	ss = 3 - nodeId;
	
	while(1)
	{
			
		/* Enable the selected slave */
		PORTB &= !(1 << ss);
			
		/* Send command and receive a response. */
		SPDR = CMD_CONTINUE_OPERATIONS;
		while(!(SPSR & (1 << SPIF)));
		data = SPDR;
			
		_delay_ms(1);
		SPDR = DUMMY;
		while(!(SPSR & (1 << SPIF)));
		data = SPDR;
			
		/* Disable the selected slave */
		PORTB |= (1 << ss);
			
		/* Check if it received a ACK. */
		if(data == ACK_CONTINUE_OPERATIONS)
		{
			break;
		}
			
	}
}

#else

/* This function is run only by the slave. It takes decisions based on the 
command received in the first byte. */

void selectionCrossoverProcessing(fitness_t evaluation[], chromosome_t population[][DIMENSION], chromosome_t newPopulation[][DIMENSION])
{
	
	popsize_t counter; /* To count the number of individuals replaced. */
	command_t command;
	popsize_t index;
	dimensionsize_t j;
	float_bytes sent;
	dimensionsize_t b;
	
	counter = 0;
	
	while(1) 
	{
		while(!(SPSR & (1 << SPIF)));
		command = SPDR;
		
		/* Identify the command and take an action. */
		if (command == CMD_COLLECT_EV)
		{			
			/* Send the ACK. */
			SPDR = ACK_COLLECT_EV;
			while(!(SPSR & (1 << SPIF)));
				
			/* Read dummy byte (0x00) .*/
			command = SPDR; // DUMMY
				
			/* Receive the index. */
			while(!(SPSR & (1 << SPIF)));
			index = SPDR;
		
			sent.value = evaluation[index];
			
			/* Finally, send the 4 bytes. */
			for(b = 0; b < sizeof(float); b++)
			{
				/* Wait for transmission to complete */
				SPDR = sent.bytes[b];
				while(!(SPSR & (1 << SPIF)));
				command = SPDR;
			}
		} 
		else if (command == CMD_COLLECT_IND)
		{
			/* Send the ACK. */
			SPDR = ACK_COLLECT_IND;
			while(!(SPSR & (1 << SPIF)));
							
			/* Read dummy byte (sent my master to receive the ack). */
			command = SPDR;
							
			/* Read index. */
			while(!(SPSR & (1 << SPIF)));
			index = SPDR;
			
			
#if CHROMOSOME_SIZE == 8

			/* Finally, send the individual. */
			for(j = 0; j < DIMENSION; j++)
			{
				/* Wait for transmission to complete */
				SPDR = population[index][j];
				while(!(SPSR & (1 << SPIF)));
			}
			
#elif CHROMOSOME_SIZE == 16

			chromosome16_bytes temp;
			dimensionsize_t k;

			/* Finally, send the individual. */
			for(j = 0; j < DIMENSION; j++)
			{
				temp.value = population[index][j];
				
				for(k = 0; k < 2; k++)
				{
					/* Wait for transmission to complete */
					SPDR = temp.bytes[k];
					while(!(SPSR & (1 << SPIF)));
				}
			}
#else 
			chromosome32_bytes temp;
			dimensionsize_t k;
			
			/* Finally, send the individual. */
			for(j = 0; j < DIMENSION; j++)
			{
				temp.value = population[index][j];
				
				for(k = 0; k < 4; k++)
				{
					/* Wait for transmission to complete */
					SPDR = temp.bytes[k];
					while(!(SPSR & (1 << SPIF)));
				}
			}

#endif					

		}		
		else if (command == CMD_SEND_IND)
		{

			/* Send the ACK. */
			SPDR = ACK_SEND_IND;
			while(!(SPSR & (1 << SPIF)));
				
			/* Read dummy byte (sent my master to receive the ack). */
			command = SPDR;
			
#if CHROMOSOME_SIZE == 8			
		
			/* Finally, receive the 4 bytes. */
			for(j = 0; j < DIMENSION; j++)
			{
				/* Wait for transmission to complete */
				SPDR = DUMMY;
				while(!(SPSR & (1 << SPIF)));
				newPopulation[counter][j] = SPDR;
			}
			
#elif CHROMOSOME_SIZE == 16
			
			chromosome16_bytes temp;
			dimensionsize_t k;
			
			/* Finally, receive the 4 bytes. */
			for(j = 0; j < DIMENSION; j++)
			{
				for(k = 0; k < 2; k++)
				{
					SPDR = DUMMY;
					while(!(SPSR & (1 << SPIF)));
					temp.bytes[k] = SPDR;
				}
				
				newPopulation[counter][j] = temp.value;
			}

#else
			chromosome32_bytes temp;
			dimensionsize_t k;
			
			/* Finally, receive the 4 bytes. */
			for(j = 0; j < DIMENSION; j++)
			{
				for(k = 0; k < 4; k++)
				{
					SPDR = DUMMY;
					while(!(SPSR & (1 << SPIF)));
					temp.bytes[k] = SPDR;
				}
				
				newPopulation[counter][j] = temp.value;
			}
#endif	
			counter++;
		
		}
		else if (command == CMD_CONTINUE_OPERATIONS)
		{
			/* Send the ACK. */
			SPDR = ACK_CONTINUE_OPERATIONS;
			while(!(SPSR & (1 << SPIF)));
						
			/* Read dummy byte (sent my master to receive the ack). */
			command = SPDR;
			
			return;
		}
	}
}

void waitSendBestIndividuaFM(chromosome_t population[][DIMENSION], popsize_t iBest)
{
	
	command_t command;
	dimensionsize_t j;
		
	while(1)
	{	
		SPDR = DUMMY;
		while(!(SPSR & (1 << SPIF)));
		command = SPDR;
				
		if (command == CMD_COLLECT_BEST_IND)
		{

			SPDR = ACK_COLLECT_BEST_IND;
			while(!(SPSR & (1 << SPIF)));
			command = SPDR;
			break;
		}
	}
							
#if CHROMOSOME_SIZE == 8

	/* Finally, send the individual. */
	for(j = 0; j < DIMENSION; j++)
	{
		/* Wait for transmission to complete */
		SPDR = population[iBest][j];
		while(!(SPSR & (1 << SPIF)));
	}
			
#elif CHROMOSOME_SIZE == 16

	chromosome16_bytes temp;
	dimensionsize_t k;

	/* Finally, send the individual. */
	for(j = 0; j < DIMENSION; j++)
	{
		temp.value = population[iBest][j];
								
		for(k = 0; k < 2; k++)
		{
			/* Wait for transmission to complete */
			SPDR = temp.bytes[k];
			while(!(SPSR & (1 << SPIF)));
		}
	}
#else
	chromosome32_bytes temp;
	dimensionsize_t k;
			
	/* Finally, send the individual. */
	for(j = 0; j < DIMENSION; j++)
	{
		temp.value = population[iBest][j];
				
		for(k = 0; k < 4; k++)
		{
			/* Wait for transmission to complete */
			SPDR = temp.bytes[k];
			while(!(SPSR & (1 << SPIF)));
		}
	}

#endif
		
}

#endif