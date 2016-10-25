/*
 * NSGAII.h
 *
 *  Created on: 21 de nov de 2015
 *      Author: arthur
 */

#ifndef GENERATIONALGA_H_
#define GENERATIONALGA_H_

#include <stdbool.h>
#include "StaticVariables.h"


typedef struct {
	double a;
	double b;
} TIMEWINDOW;


/*Driver or Rider*/
typedef struct Request{
	bool driver;//true -driver, false -rider
	bool matched;//true se for um carona já combinado
	int id_rota_match;//se for um carona, informa o id da rota que faz parte
	int id;
	double request_arrival_time;//A hora em que esse request foi descoberto (desconsiderado pq o problema é estático)
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

	TIMEWINDOW tw1;//pickup
	TIMEWINDOW tw2;//delivery

	/**Para o motorista: O número de riders que podem fazer match
	 * Para o carona: O número de motoristas que podem fazer match
	 */
	int matchable_riders;//Somente para o caso do motorista: O número de riders que podem fazer match
	/**
	 * Se esse request for um motorista, a lista contém os caronas que podem fazer match, e
	 * vice versa.
	 */
	struct Request ** matchable_riders_list;
}Request;

/*Gene of a solution*/
typedef struct Service{
	Request *r;
	/*O service time é a hora e minuto que este ponto começou a ser atendido.
	 * O WAITING_TIME é um período de tempo e acontece ANTES do service_time*/
	double service_time;
	//Tempo de espera nos pontos de source do rider
	//Atualizado ao inserir uma nova carona
	//double waiting_time; //Para manter as coisas simples. o waiting time ficará implicito.
	bool is_source;//1-está saindo da origem, 0-está chegando no destino
	int offset;//Distância desse source até o service destino;
}Service;

/*A rota guarda o serviço, podendo sobrescrever
 * sem se preocupar com a memória*/
typedef struct Rota{
	Service *list;
	int capacity;
	int length;
	int id;//Identifica o motorista da rota na estrutura GRAPH
}Rota;

typedef struct Individuo{
	//Cromossomo é uma lista de rotas
	//Onde cada rota é uma lista de Services
	//Cada Service é uma coleta ou entrega(tanto de motorista como de carona)
	Rota * cromossomo;
	int size;//tamanho da lista de rotas
	double objetivos[4];
	double objective_function;
	int id;
}Individuo;

/*Os indivíduos são armazenados no heap pra
 * facilitar em alguns aspectos a manipulação do dado.*/
typedef struct Population{
	//posição do front (caso essa população seja um front)
	int id_front;
	Individuo **list;
	int size;
	int max_capacity;
}Population;


typedef struct Fronts{
	int size;//quantidade de fronts (pode ser diferente da capacidade max)
	int max_capacity;
	Population **list;//Cada população é um front
}Fronts;



/*====================Graph=====================================================*/
/*Os primeiros DRIVERS requests são requisições de motoristas,
 * o restante são requisições de caronas*/
typedef struct Graph{
	Request *request_list;//Lista de requests
	int drivers;
	int riders;
	int total_requests;
}Graph;


void malloc_rota_clone();
void insere_carona_aleatoria_individuo(Individuo * ind, bool full_search);
bool insere_carona(Rota *rota, Request *carona, int posicao_insercao, int offset, bool is_source, int * insertFalso);
bool insere_carona_rota(Rota *rota, Request *carona, int posicao_insercao, int offset, bool inserir_de_fato, int *insercaoFalso);
bool insere_carona_aleatoria_rota(Rota* rota, bool full_search);
bool insere_carona_unica_rota(Rota *rota, Request *carona, int posicao_insercao, int offset, bool inserir_de_fato, int *insercaoFalso);
void encaixando_carona(Rota *rota);
int desfaz_insercao_carona_rota(Rota *rota, int posicao_insercao);
void clean_riders_matches(Graph *g);
void evaluate_objective_functions(Individuo *idv, Graph *g);
void evaluate_objective_functions_pop(Population* p, Graph *g);
void sort_by_objective(Population *pop, int obj);
int compare_rotas(const void *p, const void *q);
void push_forward_hard(Rota *rota, int position, double pushf);
void push_forward_mutation_op(Rota * rota);
bool push_forward(Rota * rota, int position, double pushf, bool forcar_clone);
void push_backward_soft(Rota *rota, int position, double pushb);
void push_backward_mutation_op(Rota * rota, int position);
bool push_backward(Rota * rota, int position, double pushb, bool forcar_clone);
bool transfer_rider(Rota * rotaRemover, Individuo *ind, Graph * g);
bool remove_insert(Rota * rota);
bool swap_rider(Rota * rota);
void repair(Individuo *offspring, Graph *g);
void mutation(Individuo *ind, Graph *g, double mutationProbability);
void crossover(Individuo * parent1, Individuo *parent2, Individuo *offspring1, Individuo *offspring2, Graph *g, double crossoverProbability);
void crossover_and_mutation(Population *parents, Population *offspring,  Graph *g, double crossoverProbability, double mutationProbability );
Individuo * tournamentSelection(Population * parents);
int compare0(const void *p, const void *q);
int compare1(const void *p, const void *q);
int compare2(const void *p, const void *q);
int compare3(const void *p, const void *q);
int compare_obj_f(const void *p, const void *q);

int * index_array_drivers;
int * index_array_drivers_transfer_rider;
int * index_array_drivers_mutation;
int * index_array_caronas_inserir;
int * index_array_posicao_inicial;
int * index_array_offset;


Request ** index_array_rotas;//Array com os índices ordenados das rotas, da menor pra maior qtd de matchable_riders

Graph * g;

#endif /* GENERATIONALGA_H_ */
