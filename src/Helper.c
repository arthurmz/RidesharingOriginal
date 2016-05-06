/*
 * helper.c
 *
 *  Created on: 21 de nov de 2015
 *      Author: arthur
 */

#include "Helper.h"
#include "Calculations.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

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

/*Gera um indiv�duo preenchido com os motoristas e
 * caronas aleat�rias, caso insereCaronasAleatorias seja true
 *
 * index_array[]: Aleatoriza a ORDEM em que as rotas ser�o preenchidas
 */
Individuo * generate_random_individuo(Graph *g, bool insereCaronasAleatorias){
	Individuo *idv = new_individuo(g->drivers, g->riders);
	clean_riders_matches(g);

	for (int x = 0; x < g->drivers ; x++){//pra cada uma das rotas
		Rota * rota = &idv->cromossomo[x];
		Request * driver = &g->request_list[x];
		//Insere o motorista na rota
		rota->list[0].r = driver;
		rota->list[0].is_source = true;
		rota->list[0].service_time = rota->list[0].r->pickup_earliest_time;//Sai na hora mais cedo
		rota->list[0].offset = 1;//Informa que o destino est� logo � frente
		rota->list[1].r = driver;
		rota->list[1].is_source = false;
		rota->list[1].service_time = rota->list[0].r->delivery_earliest_time;//Chega na hora mais cedo
		rota->length = 2;
	}

	if (insereCaronasAleatorias)
		insere_carona_aleatoria_individuo(idv);

	return idv;
}


/*Inicia a popula��o na mem�ria e ent�o:
 * Pra cada um dos drivers, aleatoriza a lista de Riders, e l� sequencialmente
 * at� conseguir fazer match de N caronas. Se at� o fim n�o conseguiu, aleatoriza e segue pro pr�ximo rider*/
Population *generate_random_population(int size, Graph *g, bool insereCaronasAleatorias){
	Population *p = (Population*) new_empty_population(size);
	for (int i = 0; i < size; i++){//Pra cada um dos indiv�duos idv
		Individuo *idv = generate_random_individuo(g, insereCaronasAleatorias);
		idv->id = p->size;
		p->list[p->size++] = idv;
	}
	return p;
}


/** Copia as rotas do indiv�duo origem pro indiv�duo destino */
void copy_rota(Individuo * origin, Individuo * destiny, int start, int end){
	for (int i = start; i < end; i++){
		Rota * rotaOrigem = &origin->cromossomo[i];
		Rota * rotaDestino = &destiny->cromossomo[i];
		rotaDestino->length = rotaOrigem->length;
		for (int j = 0; j < rotaOrigem->length; j++){
			rotaDestino->list[j] = rotaOrigem->list[j];
		}
	}

}

/**Clona o conte�do de uma rota em outra.
 * Para manter intacta a original, em caso da rota
 * clonada n�o servir*/
void clone_rota(Rota * rota, Rota *cloneRota){
	if (rota == cloneRota) {
		printf("bug\n");
		return;
	}

	cloneRota->id = rota->id;
	cloneRota->capacity = rota->capacity;
	cloneRota->length = rota->length;
	for (int i = 0; i < rota->length; i++){
		cloneRota->list[i] = rota->list[i];
	}
}

/** Retorna uma posi��o de carona aleat�ria
 * da rota informada
 */
inline int get_random_carona_position(Rota * rota){
	if (rota->length < 4) return -1;
	int positionSources[(rota->length-2)/2];
	//Procurando as posi��es dos sources
	int k = 0;
	for (int i = 1; i < rota->length-2; i++){
		if (rota->list[i].is_source)
			positionSources[k++] = i;
	}
	int position = positionSources[rand() % (rota->length-2)/2];
	return position;
}

/*Aloca uma nova popula��o de tamanho max_capacity
 * Cada elemento de list � um ponteiro pra indiv�duo N�O ALOCADO*/
Population* new_empty_population(int max_capacity){
	Population *p = (Population*) calloc(1, sizeof(Population));
	p->max_capacity = max_capacity;
	p->size = 0;
	p->list = calloc(max_capacity, sizeof(Individuo*));
	return p;
}



/*Constroi um novo grafo em mem�ria*/
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
		else
			g->request_list[i].matchable_riders_list = calloc(drivers, sizeof(Request*));
	}
	return g;
}

/** Obtem a inst�ncia do problema do arquivo e preenche no "Grafo" em mem�ria
 * Observar que os tempos de delivery no txt n�o est�o corretos,
 * j� que o tempo computado originalmente no artigo � sempre o ceil do tempo real
 * (que por sua vez � determinado pela dist�ncia de haversine).
 */
Graph * parse_file(char *filename){
	FILE *fp=fopen(filename, "r");

	if (fp == NULL){
		printf("N�o foi poss�vel abrir o arquivo %s\n", filename);
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

		double minimal_time = minimal_time_request(rq);
		rq->delivery_earliest_time = rq->pickup_earliest_time + minimal_time;
		rq->delivery_latest_time = rq->pickup_latest_time + minimal_time;
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


/*Desaloca a popula��o, desalocando tamb�m os indiv�duos*/
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

/*Desaloca a popula��o cujos indiv�duos n�o est�o alocados aqui*/
void dealoc_empty_population(Population *population){
	if (population != NULL){
		free(population->list);
		free(population);
	}
}

/*Desaloca a popula��o da mem�ria, sem desalocar o front!!!*/
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


/** Imprime para um arquivo o conte�do do espa�o de decis�o dos indiv�duos da popula��o*/
void print_to_file_decision_space(Population * p, Graph * g, unsigned int seed){
	FILE *fp=fopen("espaco_decisao.txt", "w");
	fprintf(fp, "Seed %u\n", seed);

	for (int i = 0; i < p->size; i++){
		Individuo * individuo = p->list[i];
		fprintf(fp, "Indiv�duo %d\n", i);

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

/**Preenche o array sequencialmente*/
void fill_array(int * array, int size){
	for (int i = 0; i < size; i++){
		array[i] = i;
	}
}

/** Verifica se a rota est� chegando no limite e aumente sua capacidade */
void increase_capacity(Rota *rota){
	//Aumentar a capacidade se tiver chegando no limite
	if (rota->length == rota->capacity - 4){
		rota->capacity += MAX_SERVICES_MALLOC_ROUTE;
		rota->list = realloc(rota->list, rota->capacity * sizeof(Service));
		printf("Aumentando a capacidade da rota clone\n");
	}
}

bool verifica_individuo(Individuo * offspring){

	bool valido = true;

	for (int q = 0; q < offspring->size; q++) {
		Rota *rota = &offspring->cromossomo[q];

		if (rota->length > rota->capacity){
			printf("Erro no length da rota %d\n", q);
			return false;
		}

		int index = 0;
		Request * rqts[100] = {0};
		//Verifica se cada carona inserido � retirado
		//Verifica se o carona � feito match s� UMA VEZ
		//Verifica se o carona � feito match na rota que est�.
		for (int k = 0; k < rota->length; k++){
			Service * srv = &rota->list[k];

			if (srv->is_source){
				Service * destino = NULL;
				for (int g = k; g < rota->length; g++){
					if (rota->list[g].r == srv->r && !rota->list[g].is_source){
						destino = &rota->list[g];
						break;
					}
				}
				if (destino == NULL){
					printf("Erro na remo��o de um carona ou driver da rota %d\n", q);
					return false;
				}

				if(!srv->r->driver){
					bool contem = false;
					for (int r = 0; r < index; r++){
						if (rqts[r] == srv->r){
							contem = true;
							break;
						}
					}
					if (contem){
						printf("Erro na rota %d, carona duplicada\n", q);
						valido = false;
					}
					else{
						rqts[index++] = srv->r;
					}
				}
			}
		}
	}
	return valido;
}

/** Verifica se a popula��o � v�lida
 * Retorna true se a popula��o � v�lida.
 *
 */
bool verifica_populacao(Population *p){
	for (int x = 0; x < p->size; x++) {
		Individuo * offspring = p->list[x];
		if (offspring->size != g->drivers){
			printf("Erro no tamanho do indiv�duo %d\n", x);
			return false;
		}
		if(!verifica_individuo(offspring))
			return false;
	}
	return true;
}


bool find_bug_population(Population * p, int quemChama){
	for (int x = 0; x < p->size; x++) {
		Individuo * offspring = p->list[x];

		for (int q = 0; q < offspring->size; q++) {
			Rota *rota = &offspring->cromossomo[q];

			Request * rq = NULL;
			int acc = 0;
			for (int k = 0; k < rota->length; k++){
				if (rq == NULL){
					rq = rota->list[k].r;
					continue;
				}
				if (rota->list[k].r != rq){
					rq = rota->list[k].r;
					acc = 0;
				}
				else if (rota->list[k].r == rq){
					acc++;
				}

				if (acc >= 2){
					printf("ENCONTROU ERRO NA ROTA: %d, quemChama: %d\n",rota->id, quemChama);
					return true;
				}
			}

		}

	}
	return false;
}

int find_bug_cromossomo(Individuo *offspring, Graph *g, int quemChama){
	for (int i = 0; i < offspring->size; i++){//Pra cada rota do idv
		Rota *rota = &offspring->cromossomo[i];
		bool achou = find_bug_rota(rota, quemChama);
		if (achou) return i;
	}
	return -200;
}


bool find_bug_rota(Rota * rota, int quemChama){
	Request * rq = NULL;
	int acc = 0;
	for (int i = 0; i < rota->length; i++){
		if (rq == NULL){
			rq = rota->list[i].r;
			continue;
		}
		if (rota->list[i].r != rq){
			rq = rota->list[i].r;
			acc = 0;
		}
		else if (rota->list[i].r == rq){
			acc++;
		}

		if (acc >= 2){
			printf("ENCONTROU ERRO NA ROTA: %d, quemChama: %d\n",rota->id, quemChama);
			return true;
		}
		if (i > 0 && i < rota->length-2 && !rota->list[i].r->matched){
			printf("matched eh false\n");
			return true;
		}
	}
	return false;
}



bool find_bug_pop2(Population * parents){
	for (int i = 0; i < parents->size; i++){
		if (find_bug_idv(parents->list[i]))
			return true;
	}
	return false;
}


bool find_bug_idv(Individuo * idv){
	for (int i = 0; i < idv->size; i++){
		if (fig_bug_rota2(&idv->cromossomo[i]))
			return true;
	}
	return false;
}


bool fig_bug_rota2(Rota * rota){
	int src = 0;
	int dest = 0;
	Service * listSources[rota->length];
	Service * listDestinations[rota->length];
	bool * matches[rota->length];
	for (int i = 0; i < rota->length; i++){
		listSources[i] = NULL;
		listDestinations[i] = NULL;
		matches[i] = false;
	}

	for (int i = 0; i < rota->length; i++){
		Service * sv = &rota->list[i];

		if (sv->is_source){
			listSources[src++] = sv;
		}
		else{
			listDestinations[dest++] = sv;
		}
	}

	if (src != dest)
		return true;
	return find_bug_rota(rota, 969696);
}

