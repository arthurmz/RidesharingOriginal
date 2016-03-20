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
#include "Calculations.h"
#include "GenerationalGA.h"


//Inicializa vetores globais úteis
void initialize_mem(Graph * g){
	malloc_rota_clone();
	index_array_rotas = malloc(g->drivers * sizeof(Request*));
	index_array_riders = malloc(g->riders * sizeof(int));
	index_array_drivers = malloc(g->drivers * sizeof(int));
	index_array_half_drivers = malloc(g->drivers/2 * sizeof(int));
	index_array_caronas_inserir = malloc(MAX_SERVICES_MALLOC_ROUTE * 10 * sizeof(int));
	for (int i = 0; i < g->riders; i++){
		index_array_riders[i] = i;
	}
	for (int i = 0; i < g->drivers; i++){
		index_array_drivers[i] = i;
	}
	for (int i = 0; i < g->drivers/2; i++){
		index_array_half_drivers[i] = i;
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
	/*Imprimindo quantos caronas cada motorista consegue fazer match*/
	int qtd = 0;
	int qtd_array[g->total_requests];
	memset(qtd_array,0,g->total_requests);
	printf("quantos matches cada motorista consegue\n");
	for (int i = 0; i < g->drivers; i++){
		//if (g->request_list[i].matchable_riders > 0)
			//qtd++;
		printf("%d: ",g->request_list[i].matchable_riders);
		for (int j = 0; j < g->request_list[i].matchable_riders; j++){
			if (!qtd_array[g->request_list[i].matchable_riders_list[j]->id]){
				qtd_array[g->request_list[i].matchable_riders_list[j]->id] = 1;
				qtd++;
			}
			printf("%d ", g->request_list[i].matchable_riders_list[j]->id);
		}
		printf("\n");
	}

	printf("qtd mínima que deveria conseguir: %d\n", qtd);
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
	srand (seed);
	//srand (3);
	//Parametros (variáveis)
	int POPULATION_SIZE;
	int ITERATIONS;
	int PRINT_ALL_GENERATIONS = 0;
	float crossoverProbability = 0.75;
	float mutationProbability = 0.1;
	char *filename = argv[1];

	sscanf(argv[2], "%d", &POPULATION_SIZE);
	sscanf(argv[3], "%d", &ITERATIONS);
	if (argc >= 5)
		sscanf(argv[4], "%f", &crossoverProbability);
	if (argc >= 6)
			sscanf(argv[5], "%f", &mutationProbability);
	if (argc >= 7)
		sscanf(argv[6], "%d", &PRINT_ALL_GENERATIONS);
	Graph * g = (Graph*)parse_file(filename);
	if (g == NULL) return 0;

	initialize_mem(g);
	setup_matchable_riders(g);
	print_qtd_matches_minima(g);


	/*=====================Início do NSGA-II============================================*/
	clock_t tic = clock();
	
	//Population *big_population = (Population*) new_empty_population(POPULATION_SIZE*2);
	//Fronts *frontsList = new_front_list(POPULATION_SIZE * 2);
	
	Population * parents = generate_random_population(POPULATION_SIZE, g, true);
	Population * children = generate_random_population(POPULATION_SIZE, g, true);
	evaluate_objective_functions_pop(parents, g);

	int i = 0;
	while(i < ITERATIONS){
		printf("Iteracao %d...\n", i);
		Population * temp = parents;
		parents = children;
		children = temp;
		crossover_and_mutation(parents, children, g, crossoverProbability, mutationProbability);
		if (PRINT_ALL_GENERATIONS)
			print(children);
		i++;
	}
	
	//Ao sair do loop, verificamos uma ultima vez o melhor gerado entre os pais e filhos
	evaluate_objective_functions_pop(parents, g);
	evaluate_objective_functions_pop(children, g);
	//merge(parents, children, big_population);
	//fast_nondominated_sort(big_population, frontsList);

	evaluate_objective_functions_pop(children, g);

	clock_t toc = clock();
	printf("Imprimindo o ultimo front obtido:\n");
	sort_by_objective(children, RIDERS_UNMATCHED);
	print(children);
	printf("Número de riders combinados: %f\n", g->riders - children->list[0]->objetivos[3]);

	printf("Tempo decorrido: %f segundos\n", (double)(toc - tic) / CLOCKS_PER_SEC);


	print_to_file_decision_space(children,g,seed);

	dealoc_full_population(parents);
	dealoc_full_population(children);
	//dealoc_empty_population(big_population);
	//dealoc_fronts(frontsList); :(
	//dealoc_graph(g);
	return EXIT_SUCCESS;
}

