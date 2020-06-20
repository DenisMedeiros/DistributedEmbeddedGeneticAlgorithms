#ifndef GA_H_
#define GA_H_

#include <stdint.h>

/* Configuration of the Genetic Algorithm */

#define CHROMOSOME_SIZE 16 /* 8, 16 or 32 */
#define DIMENSION 1 /* Dimension of the chromosome */
#define POPULATION_SIZE 32 /* 16, 32, 64, 128 or 256 */
#define NUM_GENERATIONS 64 /* Between 1 and 256 */
#define MUTATED_INDIVIDUALS 2 /* Between 0 and POPULATION_SIZE. */
#define NORMALIZATION_MIN 0.0 /* Min limit for the result */
#define NORMALIZATION_MAX 1.0 /* Max limit for the result */

/* Configuration of the distributed system. */
#define NUM_NODES 2 /* Number of microcontrollers, including the master */
#define NODE_ID 1 /* 0 means it's the master */

/*  DO NOT EDIT ANYTHING BELLOW THIS POINT */

/* Cluster configuration */
#if NUM_NODES != 2 && NUM_NODES != 4
	#error "NUM_NODES must be 2 or 4"
#endif

#define NODE_POPULATION_SIZE (POPULATION_SIZE/NUM_NODES)
#define NODE_MUTATED_INDIVIDUALS (MUTATED_INDIVIDUALS/NUM_NODES)

typedef uint8_t slave_t;
typedef uint8_t command_t;
typedef uint8_t slave_select_t;
typedef uint8_t spi_data_t;

/* Chromosome configuration */

#define MASK_SIZE CHROMOSOME_SIZE

#if CHROMOSOME_SIZE == 8

	typedef uint8_t chromosome_t;
	#define MASK 0xF0
	#define NORMALIZED_GAIN ((NORMALIZATION_MAX - NORMALIZATION_MIN)/255.0)

#elif CHROMOSOME_SIZE == 16

	typedef uint16_t chromosome_t;
	#define MASK 0xFF00
	#define NORMALIZED_GAIN ((NORMALIZATION_MAX - NORMALIZATION_MIN)/65535.0)

#elif CHROMOSOME_SIZE == 32

	typedef uint32_t chromosome_t;
	#define MASK 0xFFFF0000
	#define NORMALIZED_GAIN ((NORMALIZATION_MAX - NORMALIZATION_MIN)/4294967295.0) 
	
#else
	#error "CHROMOSOME_SIZE must be 8, 16 or 32"
#endif

/* Population configuration */
typedef uint8_t dimensionsize_t;
typedef uint8_t chromosomesize_t;

#if POPULATION_SIZE < 256
typedef uint8_t popsize_t;
#else
typedef uint16_t popsize_t;
#endif

#if NUM_GENERATIONS < 256
typedef uint8_t generationsize_t;
#else
typedef uint16_t generationsize_t;
#endif

/* Fitness configuration */
typedef float fitness_t;
typedef float normalization_t;

/* Functions definitions */

/** 
 * This is the core function of the genetic algorithm. It runs all the modules
 * and finds the best possible solution.
 *
 * @param evaluation A vector that will store the fitness values for the individuals.
 * @param population A vector containing the individuals.
 * @return The index of the best individual in the population after the last generation.
 */
popsize_t geneticAlgorithmFM(fitness_t evaluation[], chromosome_t population[][DIMENSION]);

/** 
 * This function initializes the population with random individuals. It should be called once.
 *
 * @param population A vector containing the individuals.
 */
void initializationFM(chromosome_t population[][DIMENSION]);

/** 
 * This function calculates the fitness value for the whole population. Before that, the
 * individuals are normalized to a range where the solution might be in.
 *
 * @param evaluation A vector that will store the fitness values for the individuals.
 * @param population A vector containing the individuals.
 * @return The index of the best individual in the population after the last generation.
 */
popsize_t fitnessFM(fitness_t evaluation[], chromosome_t population[][DIMENSION]);

/** 
 * This function normalizes an individual to an specified range (between NORMALIZATION_MIN and NORMALIZATION_MAX).
 *
 * @param chromesome A individual of the population.
 * @param normalizedChromesome The normalized individual.
 */
void normalizationFM(chromosome_t chromesome[], normalization_t normalizedChromesome[]);

/** 
 * This function generates a new population from a previous one. It does this by selecting and crossing
 * some individuals and applying mutation over some of them. In the and, it updates the current population
 * by the newer one.
 *
 * @param evaluation A vector that stores the fitness values of the individuals.
 * @param population A vector containing the individuals.
 * @param The index of the best individual in the population after the generation of the new population.
 */
void newPopulationFM(fitness_t evaluation[], chromosome_t population[][DIMENSION], popsize_t iBest);

/** 
 * This function mutates some individuals of the population.
 *
 * @param newPopulation A vector containing the individuals.
 */
void mutationFM(chromosome_t newPopulation[][DIMENSION]);

/** 
 * This function replaced the old population by the new one.
 *
 * @param population A vector containing the old individuals.
 * @param newPopulation A vector containing the new individuals.
 * @param iBest The index of the best individual in the population.
 */
void updateFM(chromosome_t population[][DIMENSION], chromosome_t newPopulation[][DIMENSION], popsize_t iBest);

/*
 * This function evaluates and generates a fitness value of the invididual, that has to
 * be already normalized. The user must implement it in his program.
 *
 * @param xn A individual of the population.
 * @return The fitness value of this individual.
 */
fitness_t evaluationFM(normalization_t xn[]);


/** 
 * This function transfer one individual from slave to master.
 *
 * @param x A vector representing a multi-dimension individual.
 * @param nodeId The id of the node to collect the individual from.
 * @param index The index of the individual to be collected.
 */
void collectIndividualFM(chromosome_t x[], slave_t  nodeId, popsize_t index);


/** 
 * This function transfer one evaluation value from slave to master.
 *
 * @param nodeId The id of the node to collect the individual from.
 * @param index The index of the individual to be collected.
 * @return value The evaluation value for the individual in position index.
 */
float collectEvaluationFM(slave_t nodeId, popsize_t index);

/** 
 * This function transfer one individual from master to slave.
 *
 * @param x A vector representing a multi-dimension individual.
 * @param nodeId The id of the node to send the individual to.
 */
void sendIndividualFM(chromosome_t x[], slave_t nodeId);

/** 
 * This function processes the selection and crossover (it has different implementations for master and slaves).
 *
 * @param evaluation A vector that stores the fitness values for the individuals.
 * @param population A vector containing the individuals.
 * @param newPopulation A vector containing the individuals of the new population. 
 */
void selectionCrossoverProcessing(fitness_t evaluation[], chromosome_t population[][DIMENSION], chromosome_t newPopulation[][DIMENSION]);


#if NODE_ID == 0
/** 
 * This function is run by the master and collects the best individuals of all slaves.
 *
 * @param bestIndividuals A vector that stores the best individuals of all microcontrollers.
 * @param population A vector containing the individuals.
 * @param iBest Index of the best individual in master node.
 */
void collectBestIndividualsFM(chromosome_t bestIndividuals[][DIMENSION], chromosome_t population[][DIMENSION], popsize_t iBest);


/** 
 * This function is run by the master tell all slaves to continue their operations.
 *
 * @param nodeId The node id.
 */
void continueOperationsFM(slave_t nodeId);

#else
/** 
 * This function is run by the slaves and busy wait to send the best individual to master.
 *
 * @param population A vector containing the individuals.
 * @param iBest Index of the best individual in slave node.
 */
void waitSendBestIndividuaFM(chromosome_t population[][DIMENSION], popsize_t iBest);

#endif

#endif /* GA_H_ */
