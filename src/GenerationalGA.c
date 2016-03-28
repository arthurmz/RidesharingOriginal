/*
 * NSGAII.c
 *
 *  Created on: 21 de nov de 2015
 *      Author: arthur
 */

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "Helper.h"
#include "GenerationalGA.h"
#include "Calculations.h"


Rota *ROTA_CLONE;

void malloc_rota_clone(){
	/*Criando uma rota para cópia e validação das rotas*/
	ROTA_CLONE = (Rota*) calloc(1, sizeof(Rota));
	ROTA_CLONE->list = calloc(MAX_SERVICES_MALLOC_ROUTE, sizeof(Service));
}

/*Pra poder usar a função qsort com N objetivos,
 * precisamos implementar os n algoritmos de compare*/
int compare0(const void *p, const void *q) {
    int ret;
    Individuo * x = *(Individuo **)p;
    Individuo * y = *(Individuo **)q;
    if (x->objetivos[0] == y->objetivos[0])
        ret = 0;
    else if (x->objetivos[0] < y->objetivos[0])
        ret = -1;
    else
        ret = 1;
    return ret;
}

int compare1(const void *p, const void *q) {
    int ret;
    Individuo * x = *(Individuo **)p;
    Individuo * y = *(Individuo **)q;
    if (x->objetivos[1] == y->objetivos[1])
        ret = 0;
    else if (x->objetivos[1] < y->objetivos[1])
        ret = -1;
    else
        ret = 1;
    return ret;
}

int compare2(const void *p, const void *q) {
    int ret;
    Individuo * x = *(Individuo **)p;
    Individuo * y = *(Individuo **)q;
    if (x->objetivos[2] == y->objetivos[2])
        ret = 0;
    else if (x->objetivos[2] < y->objetivos[2])
        ret = -1;
    else
        ret = 1;
    return ret;
}

int compare3(const void *p, const void *q) {
    int ret;
    Individuo * x = *(Individuo **)p;
    Individuo * y = *(Individuo **)q;
    if (x->objetivos[3] == y->objetivos[3])
        ret = 0;
    else if (x->objetivos[3] < y->objetivos[3])
        ret = -1;
    else
        ret = 1;
    return ret;
}

int compare_rotas(const void *p, const void *q){
	int ret;
	Request * x = *(Request **)p;
	Request * y = *(Request **)q;
	if (x->matchable_riders == y->matchable_riders)
		ret = 0;
	else if (x->matchable_riders < y->matchable_riders)
		ret = -1;
	else
		ret = 1;
	return ret;
}

/*Ordena a população de acordo com o objetivo 0, 1, 2, 3*/
void sort_by_objective(Population *pop, int obj){
	switch(obj){
		case 0:
			qsort(pop->list, pop->size, sizeof(Individuo*), compare0 );
			break;
		case 1:
			qsort(pop->list, pop->size, sizeof(Individuo*), compare1 );
			break;
		case 2:
			qsort(pop->list, pop->size, sizeof(Individuo*), compare2 );
			break;
		case 3:
			qsort(pop->list, pop->size, sizeof(Individuo*), compare3 );
			break;
	}
}





/**
 * Atualiza os tempos de inserção no ponto de inserção até o fim da rota.
 * Ao mesmo tempo, se identificar uma situação onde não dá pra inserir, retorna false
 *
 * Diferentemente do update_times do nsga-ii. Este aqui atualiza sequencialmente
 * os tempos da rota à partir do ponto P inserido.
 *
 * é uma espécie de push_forward sem a aleatoriedade
 */
bool update_times(Rota *rota, int p){

	for (int i = p; i >= rota->length; i++){
		Service *anterior = &rota->list[i-1];
		Service *atual = &rota->list[i];
		double at = get_earliest_time_service(atual);
		double bt = get_latest_time_service(atual);

		double tbs = time_between_services(anterior, atual);

		atual->service_time = anterior->service_time - tbs;

		if (atual->service_time > bt){
			atual->service_time = bt;
			return false;
		}
		else if (atual->service_time < at){
			atual->service_time = at;
		}
	}

	return true;
}


/**
 * O método de inserção do algoritmo original é o seguinte:
 * Em uma rota aleatória, e com uma carona aleatória a adicionar, determinamos o ponto P de inserção.
 * Então os tempos da rota são determinados sequencialmente à partir do ponto anterior.
 */
bool insere_carona_rota(Rota *rota, Request *carona, int posicao_insercao, int offset, bool inserir_de_fato){
	if (posicao_insercao <= 0 || offset <= 0) return false;

	clone_rota(rota, ROTA_CLONE);
	bool isRotaValida = false;
	Service * ant = NULL;
	Service * atual = NULL;
	double PF;

	int ultimaPos = ROTA_CLONE->length-1;
	//Empurra todo mundo depois da posição de inserção
	for (int i = ultimaPos; i >= posicao_insercao; i--){
		ROTA_CLONE->list[i+1].is_source = ROTA_CLONE->list[i].is_source;
		ROTA_CLONE->list[i+1].offset = ROTA_CLONE->list[i].offset;
		ROTA_CLONE->list[i+1].r = ROTA_CLONE->list[i].r;
		ROTA_CLONE->list[i+1].service_time = ROTA_CLONE->list[i].service_time;
	}
	ROTA_CLONE->length++;
	//Insere o conteúdo do novo carona
	ROTA_CLONE->list[posicao_insercao].r = carona;
	ROTA_CLONE->list[posicao_insercao].is_source = true;
	ant = &ROTA_CLONE->list[posicao_insercao-1];
	atual = &ROTA_CLONE->list[posicao_insercao];
	ROTA_CLONE->list[posicao_insercao].service_time = calculate_service_time(atual, ant);
	PF = ROTA_CLONE->list[posicao_insercao].service_time - ROTA_CLONE->list[posicao_insercao+1].service_time;
	isRotaValida = push_forward(ROTA_CLONE, posicao_insercao, PF);
	if (!isRotaValida)
		return false;

	//Empurra todo mundo depois da posição do offset
	for (int i = ultimaPos+1; i >= posicao_insercao + offset; i--){
		ROTA_CLONE->list[i+1].is_source = ROTA_CLONE->list[i].is_source;
		ROTA_CLONE->list[i+1].offset = ROTA_CLONE->list[i].offset;
		ROTA_CLONE->list[i+1].r = ROTA_CLONE->list[i].r;
		ROTA_CLONE->list[i+1].service_time = ROTA_CLONE->list[i].service_time;
	}
	ROTA_CLONE->length++;
	//Insere o conteúdo do destino do carona
	ROTA_CLONE->list[posicao_insercao+offset].r = carona;
	ROTA_CLONE->list[posicao_insercao+offset].is_source = true;
	ant = &ROTA_CLONE->list[posicao_insercao+offset-1];
	atual = &ROTA_CLONE->list[posicao_insercao+offset];
	ROTA_CLONE->list[posicao_insercao+offset].service_time = calculate_service_time(atual, ant);
	PF = ROTA_CLONE->list[posicao_insercao+offset].service_time - ROTA_CLONE->list[posicao_insercao+offset+1].service_time;
	isRotaValida = push_forward(ROTA_CLONE, posicao_insercao+offset, PF);
	if (!isRotaValida)
		return false;

	//Aumentar a capacidade se tiver chegando no limite
	if (ROTA_CLONE->length == ROTA_CLONE->capacity - 4){
		ROTA_CLONE->capacity += MAX_SERVICES_MALLOC_ROUTE;
		ROTA_CLONE->list = realloc(ROTA_CLONE->list, ROTA_CLONE->capacity * sizeof(Service));
	}

	isRotaValida = is_rota_valida(ROTA_CLONE);

	if (isRotaValida && inserir_de_fato){
		carona->matched = true;
		carona->id_rota_match = ROTA_CLONE->id;
		clone_rota(ROTA_CLONE, rota);
	}

	return isRotaValida;
}

int desfaz_insercao_carona_rota(Rota *rota, int posicao_insercao){
	int offset = 1;
	for (int k = posicao_insercao+1; k < rota->length; k++){//encontrando o offset
		if (rota->list[posicao_insercao].r == rota->list[k].r && !rota->list[k].is_source)
			break;
		offset++;
	}

	if (posicao_insercao <= 0 || offset <= 0) return offset;


	for (int i = posicao_insercao; i < rota->length-1; i++){
		rota->list[i] = rota->list[i+1];
	}
	rota->length--;


	for (int i = posicao_insercao+offset-1; i < rota->length-1; i++){
		rota->list[i] = rota->list[i+1];
	}
	rota->length--;


	return offset;
}

/*Remove a marcação de matched dos riders*/
void clean_riders_matches(Graph *g){
	for (int i = g->drivers; i < g->total_requests; i++){
		g->request_list[i].matched = false;
	}
}

void evaluate_objective_functions_pop(Population* p, Graph *g){
	for (int i = 0; i < p->size; i++){//Pra cada um dos indivíduos
		evaluate_objective_functions(p->list[i], g);
	}
}


double evaluate_objective_functions(Individuo *idv, Graph *g){
	double distance = 0;
	double vehicle_time = 0;
	double rider_time = 0;
	double riders_unmatched = g->riders;
	for (int m = 0; m < idv->size; m++){//pra cada rota
		Rota *rota = &idv->cromossomo[m];

		vehicle_time += tempo_gasto_rota(rota, 0, rota->length-1);
		distance += distancia_percorrida(rota);

		for (int i = 0; i < rota->length-1; i++){//Pra cada um dos sources services
			Service *service = &rota->list[i];
			if (service->r->driver || !service->is_source)//só contabiliza os services source que não é o motorista
				continue;
			riders_unmatched--;
			//Repete o for até encontrar o destino
			//Ainda não considera o campo OFFSET contido no typedef SERVICE
			for (int j = i+1; j < rota->length; j++){
				Service *destiny = &rota->list[j];
				if(destiny->is_source || service->r != destiny->r)
					continue;

				rider_time += tempo_gasto_rota(rota, i, j);
				break;
			}
		}
	}

	idv->objetivos[TOTAL_DISTANCE_VEHICLE_TRIP] = distance;
	idv->objetivos[TOTAL_TIME_VEHICLE_TRIPS] = vehicle_time;
	idv->objetivos[TOTAL_TIME_RIDER_TRIPS] = rider_time;
	idv->objetivos[RIDERS_UNMATCHED] = riders_unmatched;

	double alfa = 0.7;
	double beta = 0.1;
	double epsilon = 0.1;
	double sigma = 0.1;
	idv->objective_function =
			alfa * idv->objetivos[TOTAL_DISTANCE_VEHICLE_TRIP]
			+beta * idv->objetivos[TOTAL_TIME_VEHICLE_TRIPS]
			+epsilon * idv->objetivos[TOTAL_TIME_RIDER_TRIPS]
			+sigma * idv->objetivos[RIDERS_UNMATCHED];
	return idv->objective_function;

}



/*Insere uma quantidade variável de caronas na rota informada
 * Utilizado na geração da população inicial, e na reparação dos indivíduos quebrados*/
void insere_carona_aleatoria_rota(Rota* rota){
	Request * request = &g->request_list[rota->id];

	int qtd_caronas_inserir = request->matchable_riders;
	if (qtd_caronas_inserir == 0) return;
	/*Configurando o index_array usado na aleatorização
	 * da ordem de leitura dos caronas*/
	for (int l = 0; l < qtd_caronas_inserir; l++){
		index_array_caronas_inserir[l] = l;
	}

	//int qtd_caronas_inserir = VEHICLE_CAPACITY;
	shuffle(index_array_caronas_inserir, qtd_caronas_inserir);

	for (int z = 0; z < qtd_caronas_inserir; z++){
		Request * carona = request->matchable_riders_list[index_array_caronas_inserir[z]];
		int posicao_inicial = 1 + (rand () % (rota->length-1));
		int offset = 1;//TODO, variar o offset
		if (!carona->matched)
			insere_carona_rota(rota, carona, posicao_inicial, offset, true);
	}
}


/*seleção por torneio, k = 2*/
Individuo * tournamentSelection(Population * parents, Graph * g){
	int pos = rand() % parents->size;
	Individuo * idv1 = parents->list[pos];
	pos = rand() % parents->size;
	Individuo * idv2 = parents->list[pos];

	evaluate_objective_functions(idv1, g);
	evaluate_objective_functions(idv2, g);

	if (idv1->objective_function < idv2->objective_function)
		return idv1;
	else return idv2;
}

void crossover(Individuo * parent1, Individuo *parent2, Individuo *offspring1, Individuo *offspring2, Graph *g, double crossoverProbability){
	int rotaSize = g->drivers;
	offspring1->size = rotaSize;
	offspring2->size = rotaSize;

	int crossoverPoint = 1 + (rand() % (rotaSize-1));
	double accept = (double)rand() / RAND_MAX;

	if (accept < crossoverProbability){
		copy_rota(parent2, offspring1, 0, crossoverPoint);
		copy_rota(parent1, offspring1, crossoverPoint, rotaSize);
		copy_rota(parent1, offspring2, 0, crossoverPoint);
		copy_rota(parent2, offspring2, crossoverPoint, rotaSize);

		int index_array[g->riders];
		for (int l = 0; l < g->riders; l++){
			index_array[l] = l;
		}

		clean_riders_matches(g);
		shuffle(index_array, g->riders);
		repair(offspring1, g, 1);

		clean_riders_matches(g);
		shuffle(index_array, g->riders);
		repair(offspring2, g, 2);
	}
	else{
		copy_rota(parent1, offspring1, 0, rotaSize);
		copy_rota(parent2, offspring2, 0, rotaSize);
	}
}

/*Remove todas as caronas que quebram a validação
 * Tenta inserir novas
 * Utiliza graph pra saber quem já fez match.
 * */
void repair(Individuo *offspring, Graph *g, int position){

	for (int i = 0; i < offspring->size; i++){//Pra cada rota do idv
		Rota *rota = &offspring->cromossomo[i];

		for (int j = 0; j < rota->length; j++){//pra cada um dos services SOURCES na rota
			//Se é matched então algum SOURCE anterior já usou esse request
			//Então deve desfazer a rota de j até o offset
			if ((rota->list[j].is_source && rota->list[j].r->matched)){
				desfaz_insercao_carona_rota(rota, j);
			}
			else if (!rota->list[j].r->driver){
				rota->list[j].r->matched = true;
				rota->list[j].r->id_rota_match = rota->id;
			}
		}
		insere_carona_aleatoria_rota(rota);
	}
}


void swap_rider(){

}

/** Transfere o carona de uma rota com possivelmente mais
 * riders para outra com menos.
 */
void transfer_rider(Individuo * ind, Graph * g){
	Rota * rotaInserir = NULL;
	Rota * rotaRemover = NULL;
	Request * caronaInserir;
	Request * motoristaInserir;
	bool ok = false;

	for (int i = 0 ; i < g->drivers; i++){
		int k = index_array_drivers[i];
		rotaInserir = &ind->cromossomo[k];
		motoristaInserir = &g->request_list[k];

		if (rotaInserir->length/2 - 1 < motoristaInserir->matchable_riders){
			for (int p =0; p < motoristaInserir->matchable_riders; p++){
				Request * caronaTemp = motoristaInserir->matchable_riders_list[p];
				Rota * rotaTemp = &ind->cromossomo[caronaTemp->id_rota_match];

				if ( caronaTemp->matched && rotaTemp->id != rotaInserir->id && rotaInserir->length < rotaTemp->length){
					caronaInserir = caronaTemp;
					rotaRemover = rotaTemp;
					ok = true;
					break;
				}
			}
			if (ok) break;
		}
	}

	if (ok){
		int posicao_inserir = 1 + (rand () % (rotaInserir->length-1));
		caronaInserir->matched = false;
		bool inseriu = insere_carona_rota(rotaInserir, caronaInserir, posicao_inserir, 1, true );

		if (inseriu){
			int position = 0;
			for (int position = 0; position < rotaRemover->length; position ++){
				if ( rotaRemover->list[position].r == caronaInserir )
					break;
			}
			desfaz_insercao_carona_rota(rotaRemover,position);
		}
		caronaInserir->matched = true;
	}
}

bool remove_insert(Rota * rota){
	if (rota->length < 4) return false;
	int position = 1 + (rand() % (rota->length-1));
	int offset = desfaz_insercao_carona_rota(rota, position);
	//push_backward(rota, position);
	//push_backward(rota, offset);
	//insere_carona_aleatoria_rota(rota);
	return true;
}

/*Tenta empurar os services uma certa quantidade de tempo
 * retorna true se conseguiu fazer algum push forward*/
bool push_forward(Rota * rota, int position, double pushf){
	clone_rota(rota, ROTA_CLONE);

	if (position == -1)
		position = rand() % ROTA_CLONE->length;
	Service * atual = &ROTA_CLONE->list[position];
	double maxPushf = get_latest_time_service(atual) -  atual->service_time;

	if (pushf == -1){
		pushf = maxPushf * ((double)rand() / RAND_MAX);
	}
	else{
		pushf = fmin (pushf, maxPushf);
	}

	if (pushf <= 0) return false;

	atual->service_time+= pushf;

	for (int i = position+1; i < ROTA_CLONE->length; i++){
		if (pushf == 0)
			break;
		atual = &ROTA_CLONE->list[i];
		Service * ant = &ROTA_CLONE->list[i-1];
		double bt = get_latest_time_service(atual);

		double waiting_time = atual->service_time - ant->service_time - haversine(ant, atual);
		pushf = fmax(0, pushf - waiting_time);
		pushf = fmin(pushf, bt - atual->service_time);

		atual->service_time+= pushf;
	}
	bool rotaValida = is_rota_valida(ROTA_CLONE);
	if (rotaValida){
		clone_rota(ROTA_CLONE, rota);
		return true;
	}
	return false;
}

/*Tenta empurar os services uma certa quantidade de tempo
 * Se position = -1, gera aleatoriamente a posição*/
bool push_backward(Rota * rota, int position){
	clone_rota(rota, ROTA_CLONE);

	if (position == -1)
		position = rand() % ROTA_CLONE->length;
	Service * atual = &ROTA_CLONE->list[position];
	double pushb = atual->service_time - get_earliest_time_service(atual);
	pushb = pushb * ((double)rand() / RAND_MAX);
	if (pushb <= 0) return false;

	atual->service_time-= pushb;

	for (int i = position+1; i < ROTA_CLONE->length; i++){
		if (pushb == 0)
			break;
		atual = &ROTA_CLONE->list[i];
		double at = get_earliest_time_service(atual);

		pushb = fmin(pushb, atual->service_time - at);

		atual->service_time-= pushb;
	}
	bool rotaValida = is_rota_valida(ROTA_CLONE);
	if (rotaValida){
		clone_rota(ROTA_CLONE, rota);
		return true;
	}
	return false;
}

void mutation(Individuo *ind, Graph *g, double mutationProbability){
	for (int r = 0; r < ind->size; r++){
		double accept = (double)rand() / RAND_MAX;

		if (accept < mutationProbability){
			Rota * rota  = &ind->cromossomo[r];

			int operators = 5;
			int index = 0;
			int mutation_array[operators];
			fill_array(mutation_array, operators);
			shuffle(mutation_array, operators);
			bool ok = false;

			while (!ok && index < operators){
				switch(mutation_array[index]){
				case 0 :
					ok = push_backward(rota, -1);
					break;
				case 1 :
					ok = push_forward(rota, -1, -1);
					break;
				case 2 :
					//ok = remove_insert(rota);
					break;
				case 3 :
					//transfer_rider(ind, g);
					ok = true;
					break;
				case 4 :
					//swap_rider();
					ok = true;
					break;
				}
				index++;
			}
		}
	}
}



/*Gera uma população de filhos, usando seleção, crossover e mutação*/
void crossover_and_mutation(Population *parents, Population *offspring,  Graph *g, double crossoverProbability, double mutationProbability){
	offspring->size = 0;//Tamanho = 0, mas considera todos já alocados
	int i = 0;
	while (offspring->size < parents->size){

		Individuo *parent1 = tournamentSelection(parents, g);
		Individuo *parent2 = tournamentSelection(parents, g);

		Individuo *offspring1 = offspring->list[i++];
		Individuo *offspring2 = offspring->list[i];

		crossover(parent1, parent2, offspring1, offspring2, g, crossoverProbability);

		mutation(offspring1, g, mutationProbability);
		mutation(offspring2, g, mutationProbability);
		offspring->size += 2;
	}
}

