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
#include <float.h>
#include <math.h>
#include "Helper.h"
#include "Calculations.h"
#include "GenerationalGA.h"

void initialize_mem(Graph * g);
void setup_matchable_riders(Graph * g);
void print_qtd_matches_minima(Graph * g);

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
	g = (Graph*)parse_file(filename);
	if (g == NULL) return 0;

	srand (seed);
	initialize_mem(g);
	setup_matchable_riders(g);
	print_qtd_matches_minima(g);
	printf("Seed: %u\n", seed);


	/*=====================Início do NSGA-II============================================*/
	clock_t tic = clock();
	
	Population * parents = generate_random_population(POPULATION_SIZE, g, true);
	Population * children = generate_random_population(POPULATION_SIZE, g, true);
	evaluate_objective_functions_pop(parents, g);

	//calculando o tempo mínimo de viagem dos motoristas
	/*double drivers_total_time = 0;
	double drivers_total_distance = 0;
	double tempo_total_que_deve_dar_igual_ao_artigo = 0;
	Individuo * idv = children->list[0];
	for (int i = 0; i < idv->size; i++){
		 Service *origem = &idv->cromossomo[i].list[0];
		 Service *destino = &idv->cromossomo[i].list[1];
		 double min_time = minimal_time_between_services(origem, destino);
		 drivers_total_time += min_time;
		 double hav_temp = haversine(origem, destino);
		 drivers_total_distance += hav_temp;
		 double diff_tempo = destino->r->delivery_earliest_time - origem->r->pickup_earliest_time;
		 double round_diff_tempo = round(diff_tempo * 10.0)/10.0;
		 tempo_total_que_deve_dar_igual_ao_artigo += ceil(diff_tempo);
		 char roundd = '+';
		 if (diff_tempo - hav_temp < 0.001)
			 roundd = ' ';
		 else
			 roundd = '+';
		 printf("diff: %f rounddiff: %f hav_temp: %f %c\n",diff_tempo, round_diff_tempo, hav_temp, roundd);
	}
	printf("tempo total dos motoristas: %f\n", drivers_total_time);
	printf("distância total dos motoristas: %f\n", drivers_total_distance);
	printf("TEMPO Total que deve dar igual ao artigo %f\n", tempo_total_que_deve_dar_igual_ao_artigo);
	//return 0;*/


	int i = 0;
	while(i < ITERATIONS){
		printf("Iteracao %d...\n", i);
		crossover_and_mutation(parents, children, g, crossoverProbability, mutationProbability);
		if (PRINT_ALL_GENERATIONS)
			print(children);

		evaluate_objective_functions_pop(children, g);

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
	printf("Tempo decorrido: %f segundos\n", (double)(toc - tic) / CLOCKS_PER_SEC);
	printf("Seed: %u\n", seed);
	
	if(!verifica_populacao(children)){
		printf("ERRO!");
	}

	print_to_file_decision_space(children,g,seed);

	dealoc_full_population(parents);
	dealoc_full_population(children);
	//dealoc_empty_population(big_population);
	//dealoc_fronts(frontsList); :(
	//dealoc_graph(g);
	return EXIT_SUCCESS;
}


//Inicializa vetores globais úteis
void initialize_mem(Graph * g){
	malloc_rota_clone();
	index_array_rotas = malloc(g->drivers * sizeof(Request*));
	index_array_drivers = malloc(g->drivers * sizeof(int));
	index_array_drivers_transfer_rider = malloc(g->drivers * sizeof(int));
	index_array_drivers_mutation = malloc(g->drivers * sizeof(int));
	index_array_caronas_inserir = malloc((g->riders + 2) * sizeof(int));
	fill_array(index_array_drivers, g->drivers);
	fill_array(index_array_drivers_transfer_rider, g->drivers);
	fill_array(index_array_drivers_mutation, g->drivers);
	fill_array(index_array_caronas_inserir, g->riders + 2);
}

void setup_matchable_riders(Graph * g){
	Individuo * individuoTeste = generate_random_individuo(g, false);
	for (int i = 0; i < g->drivers; i++){
		index_array_rotas[i] = &g->request_list[i];
	}

	for (int i = 0; i < g->drivers; i++){
		Request * motoristaGrafo = individuoTeste->cromossomo[i].list[0].r;
		Rota * rota = &individuoTeste->cromossomo[i];

		for (int j = g->drivers; j < g->total_requests; j++){
			Request * carona = &g->request_list[j];
			if (insere_carona_rota(rota, carona, 1, 1, false) ){
				motoristaGrafo->matchable_riders_list[motoristaGrafo->matchable_riders++] = carona;
				carona->matchable_riders_list[carona->matchable_riders++] = motoristaGrafo;
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
		motor_array[i] = 0;
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

