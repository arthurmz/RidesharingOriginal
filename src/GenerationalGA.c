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
#include "Calculations.h"
#include "GenerationalGA.h"


Rota *ROTA_CLONE;

void malloc_rota_clone(){
	/*Criando uma rota para cópia e validação das rotas*/
	ROTA_CLONE = (Rota*) calloc(1, sizeof(Rota));
	ROTA_CLONE->list = calloc(MAX_SERVICES_MALLOC_ROUTE, sizeof(Service));
}

/*Adiciona o indivíduo de rank k no front k de FRONTS
 * Atualiza o size de FRONTS caso o rank seja maior*/
void add_Individuo_front(Fronts * fronts, Individuo *p){
	Population *fronti = fronts->list[p->rank];
	if (fronts->size < p->rank + 1){
	  fronti->size = 0;
	  fronts->size++;
	}
	
	fronti->list[fronti->size] = p;
	fronti->size++;
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

/*Pra poder usar a função qsort com N objetivos,
 * precisamos implementar os n algoritmos de compare*/
int compareByCrowdingDistanceMax(const void *p, const void *q) {
    int ret;
    Individuo * x = *(Individuo **)p;
    Individuo * y = *(Individuo **)q;
    if (x->crowding_distance == y->crowding_distance)
        ret = 0;
    else if (x->crowding_distance > y->crowding_distance)
        ret = -1;
    else
        ret = 1;
    return ret;
}

bool crowded_comparison_operator(Individuo *a, Individuo *b){
	return (a->rank < b->rank || (a->rank == b->rank && a->crowding_distance > b->crowding_distance));
}


/*Tenta empurar os services uma certa quantidade de tempo*/
bool push_forward(Rota * rota, int position, double pf){
	for (int i = position; i < rota->length; i++){
		if (pf == 0) return true;
		Service * svc = &rota->list[i];
		Service * ant = &rota->list[i-1];
		double at = get_earliest_time_service(svc);
		double bt = get_latest_time_service(svc);

		double waiting_time = svc->service_time - ant->service_time - haversine(ant, svc);
		if (waiting_time > 0)
			pf -= waiting_time;
		if (pf < 0)
			pf = 0;
		double new_st = rota->list[i].service_time + pf;
		if (new_st < at || new_st > bt)
			return false;
	}
	return true;
}



/*
 *Atualiza os tempos de inserção no ponto de inserção até o fim da rota.
 *Ao mesmo tempo, se identificar uma situação onde não dá pra inserir, retorna false
 *
 *coloca o servicetime do delivery do motorista como o mais cedo
 *percorre a rota do fim pro início, setando o servicetime
 *st_i = st_i+1 - tempo(i, i+1);
 *se st_i < earliest_time
 *	push_forward(i+1);
 *se st_i > latest_time
 *	st_i = latest_time;
 *
 *Faz isso pra todo mundo, depois minimiza o tempo de espera.
 *
 * */
bool update_times(Rota *rota){
	Service * motoristaDelivery = &rota->list[rota->length-1];

	motoristaDelivery->service_time = motoristaDelivery->r->delivery_earliest_time;

	/**
	 * Calcula o service_time de i =
	 * service_time_i = service_time_i+1 - tempo(i, i+1)
	 *
	 * se o service_time_i < at então service_time_i = at;
	 * Isso acarreta que agora o service_time_i+1 precisa ser empurrado.
	 * Isso é feito depois.
	 *
	 * Se o service_time_i > bt, service_time_i = bt, e agora
	 * service_time_i+1 ganha um waiting_time;
	 */
	for (int i = rota->length-2; i >= 0; i--){
		Service *atual = &rota->list[i];
		Service *prox = &rota->list[i+1];
		double at = get_earliest_time_service(atual);
		double bt = get_latest_time_service(atual);

		double tbs = time_between_services(atual, prox);

		atual->service_time = prox->service_time - tbs;

		if (atual->service_time > bt){
			atual->service_time = bt;
		}
		else if (atual->service_time < at){
			double pf = at - atual->service_time;
			atual->service_time = at;
			bool conseguiu = push_forward(rota, i+1, pf);
			if (!conseguiu)
				return false;
		}
	}

	/**Empurra os service_times entre services que se aproximaram
	 * por causa do calculo anterior
	 */
	/*
	for (int i = 0; i < rota->length-1; i++){
		Service * atual = &rota->list[i];
		Service * next = &rota->list[i+1];
		double bt = get_latest_time_service(next);

		double tbs = time_between_services(atual, next);

		if (next->service_time < atual->service_time + tbs)
			next->service_time = atual->service_time + tbs;

		if (next->service_time > bt)
			return false;
	}
	*/
	return true;
}

/*
 * Atualiza os tempos de inserção, minimizando os tempos de espera
 * aumentando as chances da rota ser válida.
 *
 * o waiting_time é minimizado fazendo um push_foward dos elementos que
 * estão ANTES do ponto onde há waiting_time;
 *
 * idéia:
 * percorre sequencialmente enquanto não acha um waiting_time >0
 * > vai atualizando o máximo de push_foward no ponto anterior
 * > quando achar waiting time > 0
 * >> faz service_time = max do push forward possível.
 *
 * Ex:
 *
 * A+ 1+ 1- 3+ 3- 2+ 2+ A-
 *
 * Depois de inserir o 3+ no earliest time
 *
 */
void minimize_waiting_time(Rota * rota){
	for (int i = 0; i < rota->length-1; i++){
		Service *ant = &rota->list[i];
		Service *actual = &rota->list[i+1];


		actual->service_time = calculate_time_at(actual, ant);
	}
}


/*
 * A0101B = tamanho 6
 * 			de 0 a 5
 * 	Inserir na posiçao 0 não pode pq já tem o motorista
 * 	Inserir na posição 1, empurra os demais pra frente
 * 	Inserir o destino é contado à partir da origem (offset)
 * 	offset = 0 não pode pq é o proprio origem, 1 pode e é o próximo,
 * 	2 é o que pula um e insere.
 *
 * 	inserir_de_fato - Se deve inserir mesmo ou é apenas um teste
 * */
bool insere_carona_rota(Rota *rota, Request *carona, int posicao_insercao, int offset, bool inserir_de_fato){
	if (posicao_insercao <= 0 || offset <= 0) return false;

	clone_rota(rota, ROTA_CLONE);

	Service * anterior = &ROTA_CLONE->list[posicao_insercao-1];
	Service * proximo = &ROTA_CLONE->list[posicao_insercao];
	double pickup_result;
	double delivery_result;

	//if (!is_insercao_rota_valida_jt(anterior, proximo, carona, &pickup_result, &delivery_result))
		//return false;

	int ultimaPos = ROTA_CLONE->length-1;
	//Empurra todo mundo depois da posição de inserção
	for (int i = ultimaPos; i >= posicao_insercao; i--){
		ROTA_CLONE->list[i+1].is_source = ROTA_CLONE->list[i].is_source;
		ROTA_CLONE->list[i+1].offset = ROTA_CLONE->list[i].offset;
		ROTA_CLONE->list[i+1].r = ROTA_CLONE->list[i].r;
		ROTA_CLONE->list[i+1].service_time = ROTA_CLONE->list[i].service_time;
		//ROTA_CLONE->list[i+1].waiting_time = ROTA_CLONE->list[i].waiting_time;
	}
	//Empurra todo mundo depois da posição do offset
	for (int i = ultimaPos+1; i >= posicao_insercao + offset; i--){
		ROTA_CLONE->list[i+1].is_source = ROTA_CLONE->list[i].is_source;
		ROTA_CLONE->list[i+1].offset = ROTA_CLONE->list[i].offset;
		ROTA_CLONE->list[i+1].r = ROTA_CLONE->list[i].r;
		ROTA_CLONE->list[i+1].service_time = ROTA_CLONE->list[i].service_time;
		//ROTA_CLONE->list[i+1].waiting_time = ROTA_CLONE->list[i].waiting_time;
	}

	//Insere o conteúdo do novo carona
	ROTA_CLONE->list[posicao_insercao].r = carona;
	ROTA_CLONE->list[posicao_insercao].is_source = true;
	//ROTA_CLONE->list[posicao_insercao].service_time = pickup_result;
	//Insere o conteúdo do destino do carona
	ROTA_CLONE->list[posicao_insercao+offset].r = carona;
	ROTA_CLONE->list[posicao_insercao+offset].is_source = false;
	//ROTA_CLONE->list[posicao_insercao+offset].service_time = delivery_result;

	ROTA_CLONE->length += 2;

	if (ROTA_CLONE->length == ROTA_CLONE->capacity - 4){
		ROTA_CLONE->capacity += MAX_SERVICES_MALLOC_ROUTE;
		ROTA_CLONE->list = realloc(ROTA_CLONE->list, ROTA_CLONE->capacity * sizeof(Service));
	}

	bool rotaValida = update_times(ROTA_CLONE);

	if (rotaValida)
		rotaValida = is_rota_valida(ROTA_CLONE);

	if (rotaValida && inserir_de_fato){
		carona->matched = true;
		carona->id_rota_match = ROTA_CLONE->id;
		clone_rota(ROTA_CLONE, rota);
	}

	return rotaValida;

//	if (!rotaValida){
//		desfaz_insercao_carona_rota(ROTA_CLONE, posicao_insercao, offset);
//		carona->matched = false;
//		update_times(ROTA_CLONE);
//		return false;
//	}
//	clone_rota(ROTA_CLONE, rota);
//	return true;
}

void desfaz_insercao_carona_rota(Rota *rota, int posicao_insercao, int offset){
	if (posicao_insercao <= 0 || offset <= 0) return;

	if (rota->length == 2)
		printf("rota vai ficar vazia");

	for (int i = posicao_insercao; i < rota->length-1; i++){
		rota->list[i] = rota->list[i+1];
	}
	rota->length--;

	for (int i = posicao_insercao+offset-1; i < rota->length-1; i++){
		rota->list[i] = rota->list[i+1];
	}
	rota->length--;
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
		//clean_riders_matches(g);
	}
}


void evaluate_objective_functions(Individuo *idv, Graph *g){
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

}



/*Insere uma quantidade variável de caronas na rota informada
 * Utilizado na geração da população inicial, e na reparação dos indivíduos quebrados*/
void insere_carona_aleatoria_rota(Graph *g, Rota* rota){
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
Individuo * tournamentSelection(Population * parents){
	Individuo * best = NULL;
	for (int i = 0; i < 2; i++){
		int pos = rand() % parents->size;
		Individuo * outro = parents->list[pos];
		if (best == NULL || crowded_comparison_operator(outro, best))
			best = outro;
	}
	return best;
}

void crossover(Individuo * parent1, Individuo *parent2, Individuo *offspring1, Individuo *offspring2, Graph *g, float crossoverProbability){
	int rotaSize = g->drivers;
	offspring1->size = rotaSize;
	offspring2->size = rotaSize;

	int crossoverPoint = 1 + (rand() % (rotaSize-1));
	float accept = (float)rand() / RAND_MAX;

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
				int offset = 1;
				for (int k = j+1; k < rota->length; k++){//encontrando o offset
					if (rota->list[j].r == rota->list[k].r && !rota->list[k].is_source)
						break;
					offset++;
				}
				desfaz_insercao_carona_rota(rota, j, offset);
			}
			else if (!rota->list[j].r->driver){
				rota->list[j].r->matched = true;
				rota->list[j].r->id_rota_match = rota->id;
			}
		}
		insere_carona_aleatoria_rota(g, rota);
	}
}

/** Transfere o carona de uma rota com possivelmente mais
 * riders para outra com menos.
 */
void transfer_rider(Individuo * ind, Graph * g){
	Rota * rotaInserir;
	Rota * rotaRemover;
	Request * caronaInserir;
	Request * motoristaInserir;
	bool ok = false;
	// busca entre a metade dos motoristas "menores" o primeiro sem match que pode ter um mach
	for (int i = 0 ; i < g->drivers/2; i++){
		int k = index_array_half_drivers[i];
		rotaInserir = &ind->cromossomo[k];
		motoristaInserir = &g->request_list[k];

		if (rotaInserir->length/2 - 1 < motoristaInserir->matchable_riders){

			for (int p =0; p < motoristaInserir->matchable_riders; p++){
				if ( motoristaInserir->matchable_riders_list[p]->matched
						&& motoristaInserir->matchable_riders_list[p]->id_rota_match != rotaInserir->id
						&& motoristaInserir->matchable_riders_list[p]->id_rota_match >= g->drivers/2){
					caronaInserir = motoristaInserir->matchable_riders_list[p];
					rotaRemover = &ind->cromossomo[caronaInserir->id_rota_match];
					ok = true;
					break;
				}
			}
			if (ok)
				break;
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
			int offset = 1;
			for (int k = position+1; k < rotaRemover->length; k++){//encontrando o offset
				if (rotaRemover->list[position].r == rotaRemover->list[k].r && !rotaRemover->list[k].is_source)
					break;
				offset++;
			}
			desfaz_insercao_carona_rota(rotaRemover,position,offset);
		}
		caronaInserir->matched = true;
	}
}

/** 1a mutação: remover o carona de uma rota e inserir em um motorista onde só cabe um carona*/
void mutation(Individuo *ind, Graph *g, float mutationProbability){
	float accept = (float)rand() / RAND_MAX;

	if (accept < mutationProbability){
		shuffle(index_array_half_drivers, g->drivers/2);
		transfer_rider(ind, g);
	}
}



/*Gera uma população de filhos, usando seleção, crossover e mutação*/
void crossover_and_mutation(Population *parents, Population *offspring,  Graph *g, float crossoverProbability, float mutationProbability){
	offspring->size = 0;//Tamanho = 0, mas considera todos já alocados
	int i = 0;
	while (offspring->size < parents->size){

		Individuo *parent1 = tournamentSelection(parents);
		Individuo *parent2 = tournamentSelection(parents);

		Individuo *offspring1 = offspring->list[i++];
		Individuo *offspring2 = offspring->list[i];

		crossover(parent1, parent2, offspring1, offspring2, g, crossoverProbability);

		mutation(offspring1, g, mutationProbability);
		mutation(offspring2, g, mutationProbability);
		offspring->size += 2;
	}
}

