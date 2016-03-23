/*
 ============================================================================
 Name        : RidesharingNSGAII-Clean.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "Helper.h"
#include "GenerationalGA.h"
#include "Calculations.h"


//Inicializa vetores globais úteis
void initialize_mem(Graph * g){
	malloc_rota_clone();
	index_array_rotas = malloc(g->drivers * sizeof(Request*));
	index_array_riders = malloc(g->riders * sizeof(int));
	index_array_drivers = malloc(g->drivers * sizeof(int));
	index_array_caronas_inserir = malloc(MAX_SERVICES_MALLOC_ROUTE * 10 * sizeof(int));
	for (int i = 0; i < g->riders; i++){
		index_array_riders[i] = i;
	}
	for (int i = 0; i < g->drivers; i++){
		index_array_drivers[i] = i;
	}
	for (int i = 0; i < (MAX_SERVICES_MALLOC_ROUTE * 10); i++){
		index_array_caronas_inserir[i] = i;
	}
}

void setup_matchable_riders(Graph * g){
	Individuo * individuoTeste = generate_random_individuo(g, false);
	for (int i = 0; i < individuoTeste->size; i++){
		index_array_rotas[i] = individuoTeste->cromossomo[i].list[0].r;
	}

	for (int i = 0; i < g->drivers; i++){
		Request * motoristaGrafo = &g->request_list[i];

		Rota * rota = &individuoTeste->cromossomo[i];

		for (int j = g->drivers; j < g->total_requests; j++){

			Request * carona = &g->request_list[j];

			if (insere_carona_rota(rota, carona, 1, 1, false) ){
				motoristaGrafo->matchable_riders_list[motoristaGrafo->matchable_riders++] = carona;
			}
		}
	}
	//Ordenando o array de indices das rotas (por matchable_riders)
	qsort(index_array_rotas, g->drivers, sizeof(Request*), compare_rotas );
}


void print_qtd_matches_minima(Graph * g){
	FILE *fp=fopen("qtd_matches_minima.txt", "w");
	/*Imprimindo quantos caronas cada motorista consegue fazer match*/
	int qtd = 0;
	int motor_array[g->drivers];
	int qtd_array[g->total_requests];
	for (int i = 0; i < g->drivers; i++){
		motor_array[0] = 0;
	}
	for (int i = 0; i < g->total_requests; i++){
		qtd_array[i] = 0;
	}
	fprintf(fp,"quantos matches cada motorista consegue\n");
	for (int i = 0; i < g->drivers; i++){
		fprintf(fp,"%d: ",g->request_list[i].matchable_riders);
		for (int j = 0; j < g->request_list[i].matchable_riders; j++){
			if (!qtd_array[g->request_list[i].matchable_riders_list[j]->id] && !motor_array[i]){
				qtd_array[g->request_list[i].matchable_riders_list[j]->id] = 1;
				motor_array[i] = 1;
				qtd++;
			}
			fprintf(fp,"%d ", g->request_list[i].matchable_riders_list[j]->id);
		}
		fprintf(fp,"\n");
	}

	fprintf(fp,"qtd mínima que deveria conseguir: %d\n", qtd);
	fclose(fp);
}

/*Parametros: nome do arquivo
 *
 * Inicia com 3 populações
 * Pais - Indivíduos alocados
 * Filhos -Indivíduos alocados
 * Big_population - indivíduos NÃO alocados*/
int main(int argc,  char** argv){

	/*Setup======================================*/
	if (argc < 4) {
		printf("Argumentos insuficientes\n");
		return 0;
	}
	unsigned int seed = time(NULL);
	//srand (3);
	//Parametros (variáveis)
	int POPULATION_SIZE;
	int ITERATIONS;
	int PRINT_ALL_GENERATIONS = 0;
	double crossoverProbability = 0.95;
	double mutationProbability = 0.1;
	char *filename = argv[1];

	sscanf(argv[2], "%d", &POPULATION_SIZE);
	sscanf(argv[3], "%d", &ITERATIONS);
	if (argc >= 5)
		sscanf(argv[4], "%lf", &crossoverProbability);
	if (argc >= 6)
		sscanf(argv[5], "%lf", &mutationProbability);
	if (argc >= 7)
		sscanf(argv[6], "%d", &PRINT_ALL_GENERATIONS);
	if (argc >= 8)
		sscanf(argv[7], "%u", &seed);
	Graph * g = (Graph*)parse_file(filename);
	if (g == NULL) return 0;

	srand (seed);
	initialize_mem(g);
	setup_matchable_riders(g);
	print_qtd_matches_minima(g);


	/*=====================Início do NSGA-II============================================*/
	clock_t tic = clock();
	
	Population * parents = generate_random_population(POPULATION_SIZE, g, true);
	Population * children = generate_random_population(POPULATION_SIZE, g, false);
	evaluate_objective_functions_pop(parents, g);

	int i = 0;
	while(i < ITERATIONS){
		printf("Iteracao %d...\n", i);
		crossover_and_mutation(parents, children, g, crossoverProbability, mutationProbability);
		if (PRINT_ALL_GENERATIONS)
			print(children);
		Population * temp = parents;
		parents = children;
		children = temp;
		i++;
	}
	
	//Ao sair do loop, verificamos uma ultima vez o melhor gerado entre os pais e filhos
	evaluate_objective_functions_pop(children, g);

	clock_t toc = clock();
	printf("Imprimindo o ultimo front obtido:\n");
	sort_by_objective(children, RIDERS_UNMATCHED);
	print(children);
	printf("Número de riders combinados: %f\n", g->riders - children->list[0]->objetivos[3]);

	printf("Tempo decorrido: %lf segundos\n", (double)(toc - tic) / CLOCKS_PER_SEC);
	printf("Seed: %u\n", seed);


	print_to_file_decision_space(children,g,seed);

	dealoc_full_population(parents);
	dealoc_full_population(children);
	//dealoc_empty_population(big_population);
	//dealoc_fronts(frontsList); :(
	//dealoc_graph(g);
	return EXIT_SUCCESS;
}

