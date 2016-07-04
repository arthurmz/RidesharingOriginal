/*
 * helper.h
 *
 *  Created on: 21 de nov de 2015
 *      Author: arthur
 */

#ifndef HELPER_H_
#define HELPER_H_

#include "GenerationalGA.h"


Individuo * new_individuo(int drivers_qtd, int riders_qtd);
Population* new_empty_population(int max_capacity);
void clone_rota(Rota * rota, Rota **cloneRotaAddr);
Individuo *generate_random_individuo(Graph *g, bool insereCaronasAleatorias);
Population *generate_random_population(int size, Graph *g, bool insereCaronasAleatorias);
void copy_rota(Individuo * origin, Individuo * destiny, int start, int end);
//void clean_front_list(Fronts * f);
Graph * parse_file(char *filename);
void dealoc_graph(Graph*g);
//void dealoc_population(Population *p);
void shuffle(int *array, int n);
void free_population(Population *population);
void dealoc_full_population(Population *population);
void dealoc_empty_population(Population *population);
void dealoc_fronts(Fronts * f);
void print(Population *p);
Graph *new_graph(int drivers, int riders, int total_requests);
void print_to_file_decision_space(Population * p, Graph * g, unsigned int seed);
void print_objective_function_evolution(int n_gen, double *obj_f);
void fill_array(int * array, int size);
void fill_shuffle(int *array, int first, int n);
void increase_capacity(Rota *rota);
int get_random_carona_position(Rota * rota);
bool find_bug_rota(Rota * rota, int quemChama);
int find_bug_cromossomo(Individuo *offspring, Graph *g, int quemChama);
bool find_bug_population(Population * p, int quemChama);
bool fig_bug_rota2(Rota * rota);
bool find_bug_idv(Individuo * idv);
bool find_bug_pop2(Population * parents);
bool verifica_populacao(Population *p);
bool verifica_individuo(Individuo * offspring);

#endif /* HELPER_H_ */
