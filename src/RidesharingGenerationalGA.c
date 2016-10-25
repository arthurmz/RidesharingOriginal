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
void evaluate_bounds(Population *pop);

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
	int EVAL_EACH_GENERATION = 0;
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
	double objective_function_per_gen[ITERATIONS];
	printf("Seed: %u\n", seed);


	/*=====================Início do NSGA-II============================================*/
	clock_t tic = clock();

	Population * parents = generate_random_population(POPULATION_SIZE, g, false);
	Population * children = generate_random_population(POPULATION_SIZE, g, true);
	evaluate_bounds(parents);

	int i = 0;
	while(i < ITERATIONS){
		printf("Iteracao %d...\n", i);

		evaluate_objective_functions_pop(children, g);
		if (EVAL_EACH_GENERATION){
			sort_by_objective(children, OBJECTIVE_FUNCTION);
			objective_function_per_gen[i] = children->list[0]->objective_function;
		}
		Population * temp = parents;
		parents = children;
		children = temp;

		crossover_and_mutation(parents, children, g, crossoverProbability, mutationProbability);
		if (PRINT_ALL_GENERATIONS){
			print(children);
		}

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

	if (EVAL_EACH_GENERATION){
		print_objective_function_evolution(ITERATIONS, objective_function_per_gen);
	}

	dealoc_full_population(parents);
	dealoc_full_population(children);
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
	index_array_posicao_inicial = malloc((g->riders + 2) * sizeof(int));
	index_array_offset = malloc((g->riders + 2) * sizeof(int));
	fill(index_array_drivers,0, g->drivers);
	fill(index_array_drivers_transfer_rider,0, g->drivers);
	fill(index_array_drivers_mutation,0, g->drivers);
	fill(index_array_caronas_inserir,0, g->riders + 2);
	fill(index_array_posicao_inicial,0, g->riders + 2);
	fill(index_array_offset,0, g->riders + 2);
}

void setup_matchable_riders(Graph * g){
	Individuo * individuoTeste = generate_random_individuo(g, false);
	for (int i = 0; i < g->drivers; i++){
		index_array_rotas[i] = &g->request_list[i];
	}
	int dummy = 0;//Apenas por segurança, não usar
	for (int i = 0; i < g->drivers; i++){
		Request * motoristaGrafo = individuoTeste->cromossomo[i].list[0].r;
		Rota * rota = &individuoTeste->cromossomo[i];

		for (int j = g->drivers; j < g->total_requests; j++){
			Request * carona = &g->request_list[j];
			if (insere_carona_unica_rota(rota, carona, 1, 1, false, &dummy)){
				motoristaGrafo->matchable_riders_list[motoristaGrafo->matchable_riders++] = carona;
				carona->matchable_riders_list[carona->matchable_riders++] = motoristaGrafo;
			}
		}
	}
	//Ordenando o array de indices das rotas (por matchable_riders)
	qsort(index_array_rotas, g->drivers, sizeof(Request*), compare_rotas );
}

/*
 * Avalia os limites superiores e inferiores de:
 * TOTAL_DISTANCE_VEHICLE_TRIP
 * TOTAL_TIME_VEHICLE_TRIPS
 * TOTAL_TIME_RIDER_TRIPS
 * RIDERS_UNMATCHED
 *
 * Pop: Deve ser uma população sem caronas formadas
 */
void evaluate_bounds(Population * pop){
	Individuo * idv = pop->list[0];
	for (int i = 0; i < idv->size; i++){
		 Service *origem = &idv->cromossomo[i].list[0];
		 Service *destino = &idv->cromossomo[i].list[1];
		 double haversineTemp = haversine(origem, destino);
		 TOTAL_DISTANCE_VEHICLE_TRIP_UPPER_BOUND += AD + (BD * haversineTemp);
		 TOTAL_DISTANCE_VEHICLE_TRIP_LOWER_BOUND += haversineTemp;

		 double tempoMinimo = minimal_time_between_services(origem, destino);
		 TOTAL_TIME_VEHICLE_TRIPS_UPPER_BOUND += ceil(AT + (BT * tempoMinimo));
		 TOTAL_TIME_VEHICLE_TRIPS_LOWER_BOUND += tempoMinimo;
	}

	for (int i = g->drivers; i < g->riders; i++){
		Service x, y;
		x.is_source = true;
		x.offset = 1;
		x.r = &g->request_list[i];
		y.is_source = false;
		y.offset = 0;
		y.r = &g->request_list[i];
		TOTAL_TIME_RIDER_TRIPS_UPPER_BOUND += ceil(AT + (BT * minimal_time_between_services(&x, &y)));
	}

	TOTAL_TIME_RIDER_TRIPS_LOWER_BOUND = 0;
	RIDERS_UNMATCHED_UPPER_BOUND = g->riders;
	RIDERS_UNMATCHED_LOWER_BOUND = 0;
}


void print_qtd_matches_minima(Graph * g){
	char buf[123];
	sprintf(buf, "../Run/qtd_minima_matches_%d.txt", g->total_requests);
	FILE *fp=fopen(buf, "w");
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

