/*
 * Solving the traveling salesman problem using a genetic algorithm.
 * Author: Theo Morales <theo.morales.fr@gmail.com>
 * 2017
*/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <zconf.h>


#define POPULATION_SIZE 1000
#define GENERATION_SIZE 50
#define NB_GENES 9
#define GENE_SIZE 3
#define CROSSOVER_MODE "SP" // SP:  Single Point, 2P: Two Point, U: Uniform
#define MUTATION_RATE 0.015


typedef struct {
    int x;
    int y;
    int binaryIndex[3];
    char *name;
} city;


const city cities[8] = {
    { .x = 4, .y = 2, .binaryIndex = { 0, 0, 0 }, .name = "Lille" },
    { .x = 4, .y = 6, .binaryIndex = { 0, 0, 1 }, .name = "Paris" },
    { .x = 7, .y = 7, .binaryIndex = { 0, 1, 0 }, .name = "Reims" },
    { .x = 6, .y = 14, .binaryIndex = { 0, 1, 1 }, .name =  "Lyon" },
    { .x = 10, .y = 22, .binaryIndex = { 1, 0, 0 }, .name = "Marseille" },
    { .x = 2, .y = 9, .binaryIndex = { 1, 0, 1 }, .name = "Nantes" },
    { .x = 1, .y = 11, .binaryIndex = { 1, 1, 0 }, .name = "La Rochelle" },
    { .x = 3, .y = 12, .binaryIndex = { 1, 1, 1 }, .name = "Bordeaux" }
};

/*
 * Describes the order to visit cities:
 * A gene is a 3-bit array, a chromosome contains 9 genes for the 8 cities + the first one
 * */
typedef struct {
    int genes[NB_GENES][GENE_SIZE];
    float fitness;
} chromosome;


float maxFitness = 0;

int bin_to_dec(int bin[GENE_SIZE]) {

    int dec = 0;

    for (int i = 0; i < GENE_SIZE; i ++)
        dec += bin[i] * pow(2, (GENE_SIZE - 1 - i));

    return dec;
}


/* Generate bits to fill a gene */
void make_gene(int *gene) {

    for (int i = 0; i < GENE_SIZE; i++)
        gene[i] = rand() % 2;
}


/*
 * Map a gene from its binary index to the corresponding city
 * and return its coordinates
 */
void get_gene_coord(int gene[], int coord[]) {

    int geneValue = bin_to_dec(gene);

    coord[0] = cities[geneValue].x;
    coord[1] = cities[geneValue].y;
}


int validate_chromosome(chromosome c) {

    int isThereTwice = 0;

    /* Check if a gene is present twice */
    for (int i = 0; i < NB_GENES; i++) {
        for (int j = 1; j < NB_GENES - 1; j++) { //Ignore the last one, which SHOULD be equal to the first one !
            if (i != j && c.genes[i] == c.genes[j]) {
                isThereTwice = 1;
                continue;
            }
        }
    }

    return !isThereTwice;
}

/* Objective function: total length of the trip */
float score(chromosome c) {

    float score = 0;

    for (int i =  1; i < NB_GENES; i++) {
        int geneCoord[2] = {0}, prevGeneCoord[2] = {0};

        get_gene_coord(c.genes[i], geneCoord);
        get_gene_coord(c.genes[i - 1], prevGeneCoord);

        score += sqrt(
                pow(fabs(geneCoord[0] - prevGeneCoord[0]), 2)
                + pow(fabs(geneCoord[1] - prevGeneCoord[1]), 2)
        );
    }

    if (!validate_chromosome(c))
        score = 0.1;

    return score;
}

/* Mean objective function value over population */
float mean(chromosome *generation, int size) {

    /* TODO: Optimize by caching the mean for the current generation */
    float mean = 0;

    for (int i = 0; i < size; i++)
        mean += score(generation[i]);

    mean /= size;

    return mean;
}

/* Objective score function: score / mean */
float fitness(chromosome c, chromosome *generation, int generationSize) {

    float fitness = score(c) / mean(generation, generationSize);

    /* Store the max fitness of current generation to adapt our random pick */
    if (fitness > maxFitness)
        maxFitness = fitness;

    return fitness;
}


float average_fitness(chromosome generation[], int size) {

    float average_fitness = 0;

    for (int i = 0; i < size; i++)
        average_fitness += generation[i].fitness;

    average_fitness /= size;

    return average_fitness;
}


float rand_a_b(float a, float b) {

    return (rand() / (float) RAND_MAX) * (b - a) + a;
}


void select_chromosomes(chromosome generation[], int toSize, chromosome population[], int fromSize) {

    chromosome c = { 0 };
    srand((unsigned)time(NULL));
    int picked = 0;
    float averageFitness = average_fitness(population, fromSize);

    for (int i = 0; i < toSize; i++) {
        picked = 0;

        while (!picked) {
            for (int j = i; j < fromSize; j++) {

                /* Only select the fittest */
                /*if (population[j].fitness < averageFitness)
                    continue;*/

                float wheel = rand_a_b(0, maxFitness);

                if (wheel <= population[j].fitness) {
                    generation[i] = population[j];
                    picked = 1;
                }
            }
        }
    }
}


/*
 * Crossover of two randomly selected chromosomes from the given population
 * Returns the offspring
 */
chromosome make_random_offspring(chromosome *candidates, int size) {

    chromosome dad, mom, kid;
    kid.fitness = 0;

    int firstPick = random() % size;
    int secondPick = random() % size;

    dad = candidates[firstPick];

    while (secondPick == firstPick)
        secondPick = random() % size;

    mom = candidates[secondPick];


    if (strcmp(CROSSOVER_MODE, "SP") == 0) {
        /* Single point crossover */
        int crossover_point = 1 + rand() % (NB_GENES - 1);
        //int crossover_point = NB_GENES / 2; //TODO: Decide !!

        for (int i = 0; i < crossover_point; i++)
            for (int j = 0; j < GENE_SIZE; j++)
                kid.genes[i][j] = dad.genes[i][j];

        for (int i = crossover_point; i < NB_GENES; i++)
            for (int j = 0; j < GENE_SIZE; j++)
                kid.genes[i][j] = mom.genes[i][j];

    } else if (strcmp(CROSSOVER_MODE, "DP") == 0) {
        /* Double point crossover */
        //TODO

    } else if (strcmp(CROSSOVER_MODE, "U") == 0) {
        /* Uniform crossover */
        //TODO
    }

    return kid;
}


chromosome mutate(chromosome c) {

    chromosome mutated_c;
    mutated_c.fitness = 0;
    srand((unsigned)time(NULL));

    for (int i = 0; i < NB_GENES; i++) {
        for (int j = 0; j < GENE_SIZE; j++) {
            float wheel = rand_a_b(0, 1);

            if (wheel <= MUTATION_RATE && c.genes[i][j])
                mutated_c.genes[i][j] = 0;
            else if (wheel <= MUTATION_RATE && !c.genes[i][j])
                mutated_c.genes[i][j] = 1;
            else
                mutated_c.genes[i][j] = c.genes[i][j];
        }
    }

    return mutated_c;
}


int main() {

    chromosome population[POPULATION_SIZE];
    chromosome generation[GENERATION_SIZE];

    printf("* Generating %d candidates for base population...\n", POPULATION_SIZE);

    //TODO: Try to generate a valid population first, and allow invalid chromosomes then
    for (int p = 0; p < POPULATION_SIZE; p++) {
        chromosome c;
        int invalid = 1;

        /* Start from Lille, return to Lille */
        /* TODO: Make this scalable (if we add new cities) */
        c.genes[0][0] = c.genes[NB_GENES - 1][0] = 0;
        c.genes[0][1] = c.genes[NB_GENES - 1][1] = 0;
        c.genes[0][2] = c.genes[NB_GENES - 1][2] = 0;

        /* Reset C */
        for (int i = 1; i < NB_GENES - 1; i++)
            for (int j = 0; j < GENE_SIZE; j++)
                c.genes[i][j] = 0;

        c.fitness = 0;

        for (int i = 1; i < NB_GENES - 1; i++) {

            invalid = 1;

            while (invalid) {
                invalid = 0;
                make_gene(c.genes[i]);

                /* Check validity */
                for (int j = 0; j < i; j++) {
                    if (j != i
                        && (c.genes[j][0] == c.genes[i][0])
                        && (c.genes[j][1] == c.genes[i][1])
                        && (c.genes[j][2] == c.genes[i][2])) {
                        invalid = 1;
                        continue;
                    }
                }
            }
        }

        population[p] = c;
    }

    /* Calculate fitness for each candidate in the population */
    for (int i = 0; i < POPULATION_SIZE; i++)
        population[i].fitness = fitness(population[i], population, POPULATION_SIZE);

    /* Selecting original candidates */
    select_chromosomes(generation, GENERATION_SIZE, population, POPULATION_SIZE);


    int iteration = 0;

    while(1) {

        system("clear");
        printf("* Generating %d candidates for base population...\n", POPULATION_SIZE);
        printf("* Iteration: %d\n", iteration++);

        /* Select GENERATION_SIZE individuals from population, based on their fitness or randomly */
        printf("* Selecting %d individuals from generation %d...\n", GENERATION_SIZE, iteration);

        chromosome nextGeneration[GENERATION_SIZE] = {0}; //The offsprings of the (intermediate) generation

        maxFitness = 0;

        /* Crossover and mutate from the selection */
        for (int i = 0; i < GENERATION_SIZE; i++)
            nextGeneration[i] = mutate(make_random_offspring(generation, GENERATION_SIZE));

        /*
         * Update the fitness of the offsprings
         * Needs to be done after the crossover is complete because the fitness
         * is calculated depending on the mean of the whole generation
         */
        for (int i = 0; i < GENERATION_SIZE; i++)
            nextGeneration[i].fitness = fitness(nextGeneration[i], nextGeneration, GENERATION_SIZE);

        chromosome selection[GENERATION_SIZE] = {0}; //Selection of nextGeneration (offsprings) + base generation

        /*
         * Select new generation, based on the offsprings and the parent generation,
         * and the fitness of their chromosomes
         */
        select_chromosomes(selection, GENERATION_SIZE / 2, nextGeneration, GENERATION_SIZE);
        select_chromosomes(&selection[GENERATION_SIZE / 2], GENERATION_SIZE / 2, generation, GENERATION_SIZE);

        /* Replace generation by the selection -> cross-breed of old generation + next generation */
        for (int i = 0; i < GENERATION_SIZE; i++)
            generation[i] = selection[i];

        /* Update final mean score to give feedback */

        printf("* Generation average score: %.2f\n", mean(generation, GENERATION_SIZE));

    }

    return 0;
}