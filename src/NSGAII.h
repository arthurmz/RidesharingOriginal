/*
 * NSGAII.h
 *
 *  Created on: 21 de nov de 2015
 *      Author: arthur
 */

#ifndef NSGAII_H_
#define NSGAII_H_

#include <stdbool.h>
#include "StaticVariables.h"

/*Driver or Rider*/
typedef struct Request{
	bool driver;//true -driver, false -rider
	bool matched;//true se for um carona j� combinado
	int id_rota_match;//se for um carona, informa o id da rota que faz parte
	int id;
	double request_arrival_time;//A hora em que esse request foi descoberto (desconsiderado pq o problema � est�tico)
	double service_time_at_source;//Tempo gasto para atender o source (Diferente da HORA em que chega no source)
	double service_time_at_delivery;//Tempo gasto para atender o destino (Diferente da HORA em que chega no destino)
	double pickup_location_longitude;
	double pickup_location_latitude;
	double delivery_location_longitude;
	double delivery_location_latitude;
	double pickup_earliest_time;
	double pickup_latest_time;
	double delivery_earliest_time;
	double delivery_latest_time;


	int matchable_riders;//Somente para o caso do motorista: O n�mero de riders que podem fazer match
	struct Request ** matchable_riders_list;//S� � preenchida se este for um Driver
}Request;

/*Gene of a solution*/
typedef struct Service{
	Request *r;
	/*O service time � a hora e minuto que este ponto come�ou a ser atendido.
	 * O WAITING_TIME � um per�odo de tempo e acontece ANTES do service_time*/
	double service_time;
	//Tempo de espera nos pontos de source do rider
	//Atualizado ao inserir uma nova carona
	//double waiting_time; //Para manter as coisas simples. o waiting time ficar� implicito.
	bool is_source;//1-est� saindo da origem, 0-est� chegando no destino
	int offset;//Dist�ncia desse source at� o service destino;
}Service;

/*A rota guarda o servi�o, podendo sobrescrever
 * sem se preocupar com a mem�ria*/
typedef struct Rota{
	Service *list;
	int capacity;
	int length;
	int id;//Identifica o motorista da rota na estrutura GRAPH
}Rota;

typedef struct Individuo{
	//Cromossomo � uma lista de rotas
	//Onde cada rota � uma lista de Services
	//Cada Service � uma coleta ou entrega(tanto de motorista como de carona)
	Rota * cromossomo;
	int size;//tamanho da lista de rotas
	float objetivos[4];

	int dominated_by_count;//N�mero de solu��es que dominam ind
	struct Individuo **dominates_list;//Conjunto de solu��es dominadas por this
	int dominates_list_capacity;
	int dominates_list_count;//n�mero de solu��es dominadas por ind
	int rank;//Qual front este indiv�duo est�
	float crowding_distance;
}Individuo;

/*Os indiv�duos s�o armazenados no heap pra
 * facilitar em alguns aspectos a manipula��o do dado.*/
typedef struct Population{
	//posi��o do front (caso essa popula��o seja um front)
	int id_front;
	Individuo **list;
	int size;
	int max_capacity;
}Population;


typedef struct Fronts{
	int size;//quantidade de fronts (pode ser diferente da capacidade max)
	int max_capacity;
	Population **list;//Cada popula��o � um front
}Fronts;



/*====================Graph=====================================================*/
/*Os primeiros DRIVERS requests s�o requisi��es de motoristas,
 * o restante s�o requisi��es de caronas*/
typedef struct Graph{
	Request *request_list;//Lista de requests
	int drivers;
	int riders;
	int total_requests;
}Graph;


//Graph *new_graph(int drivers, int riders, int total_requests);
void add_Individuo_front(Fronts * fronts, Individuo *p);
void malloc_rota_clone();
bool update_times(Rota *rota);
bool dominates(Individuo *a, Individuo *b);
void add_dominated(Individuo *b, Individuo *a);
void fast_nondominated_sort(Population *population, Fronts * fronts);
void crowding_distance_assignment(Population *pop);
bool crowded_comparison_operator(Individuo *a, Individuo *b);
bool insere_carona_rota(Rota *rota, Request *carona, int posicao_insercao, int offset, bool inserir_de_fato);
void insere_carona_aleatoria_rota(Graph *g, Rota* rota);
void desfaz_insercao_carona_rota(Rota *rota, int posicao_insercao, int offset);
void clean_riders_matches(Graph *g);
void evaluate_objective_functions(Individuo *idv, Graph *g);
void evaluate_objective_functions_pop(Population* p, Graph *g);
void free_population(Population *population);
void crossover_and_mutation(Population *parents, Population *offspring,  Graph *g, float crossoverProbability, float mutationProbability );
void empty_front_list(Fronts * f);
void sort_by_crowding_distance_assignment(Population *front);
void sort_by_objective(Population *pop, int obj);
int compare_rotas(const void *p, const void *q);
void select_parents_by_rank(Fronts *frontsList, Population *parents, Population *offsprings, Graph *g);
void merge(Population *p1, Population *p2, Population *big_population);
void complete_free_individuo(Individuo * idv);
void repair(Individuo *offspring, Graph *g, int position);
void mutation(Individuo *ind, Graph *g, float mutationProbability);


int * index_array_riders;
int * index_array_drivers;
int * index_array_half_drivers;//metade dos motoristas
int * index_array_caronas_inserir;


Request ** index_array_rotas;//Array com os �ndices ordenados das rotas, da menor pra maior qtd de matchable_riders


#endif /* NSGAII_H_ */
