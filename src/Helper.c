/*
 * helper.c
 *
 *  Created on: 21 de nov de 2015
 *      Author: arthur
 */

#include "Helper.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


/*Aloca um novo Fronts na memória, de tamanho max_capacity
 * Cada elemento de list é um ponteiro pra uma população que é alocada
 * Cada população guarda uma lista de ponteiros pros Individuos
 * Os indivíduos não são alocados*/
Fronts* new_front_list(int max_capacity){
	Fronts* f = (Fronts*) calloc(1, sizeof(Fronts));
	f->size = 0;
	f->max_capacity = max_capacity;
	f->list = (Population**) calloc(max_capacity, sizeof(Population*));
	for (int i = 0; i < max_capacity; i++){
		f->list[i] = calloc(1, sizeof(Population));
		Population *fronti = f->list[i];
		fronti->id_front = 0;
		fronti->size = 0;
		fronti->max_capacity = max_capacity;
		fronti->list = calloc(max_capacity, sizeof(Individuo));
	}
	return f;
}


/*TESTADO OK*/
Individuo * new_individuo(int drivers_qtd, int riders_qtd){
	Individuo *ind = calloc(1, sizeof(Individuo));
	ind->cromossomo = (Rota*) calloc(drivers_qtd, sizeof(Rota));
	ind->size = drivers_qtd;

	for (int i = 0; i < drivers_qtd; i++){
		Service * result = calloc(MAX_SERVICES_MALLOC_ROUTE, sizeof(Service));//Se quebrar, aumentar esse tamanho aqui
		ind->cromossomo[i].list = result;
		ind->cromossomo[i].length = 0;
		ind->cromossomo[i].capacity = MAX_SERVICES_MALLOC_ROUTE;
		ind->cromossomo[i].id = i;
	}
	return ind;
}

/*Gera um indivíduo preenchido com os motoristas e
 * caronas aleatórias, caso insereCaronasAleatorias seja true
 *
 * index_array[]: Aleatoriza a ORDEM em que as rotas serão preenchidas
 */
Individuo * generate_random_individuo(Graph *g, bool insereCaronasAleatorias){
	Individuo *idv = new_individuo(g->drivers, g->riders);

	for (int x = 0; x < g->drivers ; x++){//pra cada uma das rotas
		int j = index_array_drivers[x];
		Rota * rota = &idv->cromossomo[j];
		Request * driver = &g->request_list[j];

		//Insere o motorista na rota
		rota->list[0].r = driver;
		rota->list[0].is_source = true;
		rota->list[0].service_time = rota->list[0].r->pickup_earliest_time;//Sai na hora mais cedo
		rota->list[0].offset = 1;//Informa que o destino está logo à frente
		rota->list[1].r = driver;
		rota->list[1].is_source = false;
		rota->list[1].service_time = rota->list[0].r->delivery_earliest_time;//Chega na hora mais cedo
		rota->length = 2;

		if (insereCaronasAleatorias)
			insere_carona_aleatoria_rota(g, rota);
	}
	//Depois de inserir todas as rotas, limpa a lista de matches
	//Para que o próximo indivíduo possa usa-las
	if (insereCaronasAleatorias)
		clean_riders_matches(g);

	return idv;
}


/*Inicia a população na memória e então:
 * Pra cada um dos drivers, aleatoriza a lista de Riders, e lê sequencialmente
 * até conseguir fazer match de N caronas. Se até o fim não conseguiu, aleatoriza e segue pro próximo rider*/
Population *generate_random_population(int size, Graph *g, bool insereCaronasAleatorias){
	Population *p = (Population*) new_empty_population(size);

	for (int i = 0; i < size; i++){//Pra cada um dos indivíduos idv
		shuffle(index_array_drivers,g->drivers);
		Individuo *idv = generate_random_individuo(g, insereCaronasAleatorias);
		p->list[p->size++] = idv;
	}
	return p;
}


/** Copia as rotas do indivíduo origem pro indivíduo destino */
void copy_rota(Individuo * origin, Individuo * destiny, int start, int end){
	for (int i = start; i < end; i++){
		Rota * rotaOrigem = &origin->cromossomo[i];
		Rota * rotaDestino = &destiny->cromossomo[i];
		rotaDestino->length = rotaOrigem->length;
		for (int j = 0; j < rotaOrigem->length; j++){
			rotaDestino->list[j] = rotaOrigem->list[j];
			//rotaDestino->list[j].r = rotaOrigem->list[j].r;
			//rotaDestino->list[j].is_source = rotaOrigem->list[j].is_source;
			//rotaDestino->list[j].service_time = rotaOrigem->list[j].service_time;
			//destiny->cromossomo[i].list[j].waiting_time = rota->list[j].waiting_time;
		}
	}

}

/**Clona o conteúdo de uma rota em outra.
 * Para manter intacta a original, em caso da rota
 * clonada não servir*/
void clone_rota(Rota * rota, Rota *cloneRota){
	cloneRota->id = rota->id;
	cloneRota->length = rota->length;
	for (int i = 0; i < rota->length; i++){
		cloneRota->list[i] = rota->list[i];
	}
}

void complete_free_individuo(Individuo * idv){
	if (idv != NULL){
		for (int i = 0; i < idv->size; i++){
			if (idv->cromossomo != NULL && idv->cromossomo[i].list != NULL)
				free(idv->cromossomo[i].list);
		}
		if (idv->cromossomo != NULL)
			free(idv->cromossomo);
		free(idv);
	}
}

/*Aloca uma nova população de tamanho max_capacity
 * Cada elemento de list é um ponteiro pra indivíduo NÃO ALOCADO*/
Population* new_empty_population(int max_capacity){
	Population *p = (Population*) calloc(1, sizeof(Population));
	p->max_capacity = max_capacity;
	p->size = 0;
	p->list = calloc(max_capacity, sizeof(Individuo*));
	return p;
}



/*Constroi um novo grafo em memória*/
Graph *new_graph(int drivers, int riders, int total_requests){
	Graph * g = calloc(1, sizeof(Graph));
	g->request_list = calloc(total_requests, sizeof(Request));
	g->drivers = drivers;
	g->riders = riders;
	g->total_requests = total_requests;

	for (int i = 0; i < total_requests; i++){
		g->request_list[i].matchable_riders = 0;
		if (i < drivers)
			g->request_list[i].matchable_riders_list = calloc(riders, sizeof(Request*));
	}
	return g;
}

Graph * parse_file(char *filename){
	FILE *fp=fopen(filename, "r");

	if (fp == NULL){
		printf("Não foi possível abrir o arquivo %s\n", filename);
		return NULL;
	}
	else{
		printf("Arquivo aberto corretamente! %s\n", filename);
	}

	int total_requests;
	int drivers;
	int riders;

	char linha_temp[1000];
	fgets(linha_temp, 1000, fp);
	sscanf(linha_temp, "%i", &total_requests);
	fgets(linha_temp, 1000, fp);
	sscanf(linha_temp, "%i", &drivers);
	fgets(linha_temp, 1000, fp);
	sscanf(linha_temp, "%i", &riders);
	Graph * g = new_graph(drivers, riders, total_requests);

	int index_request = 0;

	while (!feof(fp) && index_request < total_requests){
		char linha[10000];
		fgets(linha, 10000, fp);

		Request *rq = &g->request_list[index_request++];

		sscanf(linha, "%i %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf ",
				&rq->id,
				&rq->request_arrival_time,
				&rq->service_time_at_source,
				&rq->pickup_location_longitude,
				&rq->pickup_location_latitude,
				&rq->pickup_earliest_time,
				&rq->pickup_latest_time,
				&rq->service_time_at_delivery,
				&rq->delivery_location_longitude,
				&rq->delivery_location_latitude,
				&rq->delivery_earliest_time,
				&rq->delivery_latest_time);
		if(rq->id < drivers)
			rq->driver = true;
		else
			rq->driver = false;
	}

	fclose(fp);
	return g;
}


/*aleatoriza o vetor informado*/
void shuffle(int *array, int n) {
    if (n > 1) {
    	int i;
		for (i = 0; i < n - 1; i++) {
		  int j = i + rand() / (RAND_MAX / (n - i) + 1);
		  int t = array[j];
		  array[j] = array[i];
		  array[i] = t;
		}
    }
}


/*Desaloca a população, mantendo os indivíduos alocados*/
void free_population(Population *population){
	if (population != NULL){
		if (population->list != NULL)
			free(population->list);
		free(population);
	}
}

/*Desaloca a população, desalocando também os indivíduos*/
void dealoc_full_population(Population *population){
	if (population != NULL){
		for (int i = 0; i < population->size; i++){
			if (population->list[i] != NULL)
				free(population->list[i]);
		}
		free(population->list);
		free(population);
	}
}

/*Desaloca a população cujos indivíduos não estão alocados aqui*/
void dealoc_empty_population(Population *population){
	if (population != NULL){
		free(population->list);
		free(population);
	}
}

/*Desaloca a população da memória, sem desalocar o front!!!*/
void dealoc_fronts(Fronts * front){
	for (int i = 0; i < front->max_capacity; front++){
		if (front->list[i] != NULL)
			dealoc_full_population(front->list[i]);
	}
	free(front->list);
	free(front);
}


void print(Population *p){
	for (int i = 0; i < p->size; i++){
		Individuo *id = p->list[i];
		printf("%f %f %f %f\n",id->objetivos[0], id->objetivos[1], id->objetivos[2], id->objetivos[3]);
	}
}


/** Imprime para um arquivo o conteúdo do espaço de decisão dos indivíduos da população*/
void print_to_file_decision_space(Population * p, Graph * g){
	FILE *fp=fopen("espaco_decisao.txt", "w");

	for (int i = 0; i < p->size; i++){
		Individuo * individuo = p->list[i];
		fprintf(fp, "Indivíduo %d\n", i);

		for (int j = 0; j < g->drivers; j++){
			for (int k = 0; k < individuo->cromossomo[j].length; k++){
				fprintf(fp, "%d:%c ", individuo->cromossomo[j].list[k].r->id, individuo->cromossomo[j].list[k].is_source ? '+' : '-');
			}
			fprintf(fp, " | ");
			for (int k = 0; k < individuo->cromossomo[j].length; k++){
				fprintf(fp, "%.2f: ", individuo->cromossomo[j].list[k].service_time);
			}
			fprintf(fp, " | ");
			for (int k = 0; k < individuo->cromossomo[j].length; k++){
				double a,b;
				if ( individuo->cromossomo[j].list[k].is_source){
					a = individuo->cromossomo[j].list[k].r->pickup_earliest_time;
					b = individuo->cromossomo[j].list[k].r->pickup_latest_time;
				}
				else {
					a = individuo->cromossomo[j].list[k].r->delivery_earliest_time;
					b = individuo->cromossomo[j].list[k].r->delivery_latest_time;
				}
				fprintf(fp, "[%.2f;%.2f] ", a, b);
			}

			fprintf(fp, "\n");
		}
	}

	fclose(fp);
}


