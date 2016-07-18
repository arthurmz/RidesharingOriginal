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

/** Rota usada para a cópia em operações de mutação etc.*/
Rota *ROTA_CLONE;
Rota *ROTA_CLONE_SWAP;//Outros clones para não conflitar as cópias
Rota *ROTA_CLONE_REMOVE_INSERT;//usado só no op de remove-insert
Rota *ROTA_CLONE_PUSH;

/** Aloca a ROTA_CLONE global */
void malloc_rota_clone(){
	/*Criando uma rota para cópia e validação das rotas*/
	ROTA_CLONE = (Rota*) calloc(1, sizeof(Rota));
	ROTA_CLONE->list = calloc(MAX_SERVICES_MALLOC_ROUTE, sizeof(Service));

	ROTA_CLONE_SWAP = (Rota*) calloc(1, sizeof(Rota));
	ROTA_CLONE_SWAP->list = calloc(MAX_SERVICES_MALLOC_ROUTE, sizeof(Service));

	ROTA_CLONE_REMOVE_INSERT = (Rota*) calloc(1, sizeof(Rota));
	ROTA_CLONE_REMOVE_INSERT->list = calloc(MAX_SERVICES_MALLOC_ROUTE, sizeof(Service));

	ROTA_CLONE_PUSH = (Rota*) calloc(1, sizeof(Rota));
	ROTA_CLONE_PUSH->list = calloc(MAX_SERVICES_MALLOC_ROUTE, sizeof(Service));
}

/**
 * Insere caronas aleatórias para todas as caronas da rota
 * IMPORTANTE: Antes de chamar, todos os caronas já feito match devem estar no grafo
 */
void insere_carona_aleatoria_individuo(Individuo * ind, bool full_search){
	shuffle(index_array_drivers,g->drivers);
	for (int i = 0; i < ind->size; i++){
		int j = index_array_drivers[i];
		insere_carona_aleatoria_rota(&ind->cromossomo[j], full_search);
	}
}

//Insere a carona na rota e empura os tempos de pickup e delivery.
//Completa a inserção mesmo se a rota ficar inválida
void insere_carona(Rota *rota, Request *carona, int posicao_insercao, int offset, bool is_source){
	Service * ant = NULL;
	Service * atual = NULL;
	Service * next = NULL;
	double PF;
	double nextTime;

	int ultimaPos = rota->length-1;
	//Empurra todo mundo depois da posição de inserção
	for (int i = ultimaPos; i >= posicao_insercao; i--){
		rota->list[i+1] = rota->list[i];
	}
	ant = &rota->list[posicao_insercao-1];
	atual = &rota->list[posicao_insercao];
	next = &rota->list[posicao_insercao+1];

	atual->r = carona;
	atual->is_source = is_source;
	atual->offset = offset;
	atual->service_time = calculate_service_time(atual, ant);

	nextTime = calculate_service_time(next, atual);
	PF = nextTime - next->service_time;
	rota->length++;//Deve aumentar o tamanho antes de fazer o PF
	if (PF > 0) {
		push_forward_hard(rota, posicao_insercao+1, PF);
	}
}


/**Minha dúvida sobre esse operador é:
 * ele pega as posições de inserção P e seu offset, coloca na rota e
 * recalcula todo mundo à partir do horário de saída do motorista?
 * Pensando bem, o recálculo é desnecessário. já que
 * A+A- segue a idéia do recalculo total
 * A+1+1-A- vai ser inserido no seus tempos mais cedo
 * A+1+1-2+2-A-, A rota vai ficar igual de A+ até 1-, o resto que vai sofrer push forward.
 * Então a resposta é SIM, a operação de recalcular os tempos de todo mundo é equivalente à
 * calcular a rota parcialmente do ponto P pra frente!
 *
 * A forma que está escrito dá a enteder que primeiro o ponto de inserção é adicionado,
 * é feito o push forward, então o ponto de offset é adicionado.
 * Esse é um bom ponto de melhoria, pois se o push forward de P+1 for maior que o necessário,
 * a carona nesse canto podia ser válida e o algoritmo não pegou
 * Ex: vou inserir o ponto Px, entre P0 e P1, e meu offset é 1
 * PIOR QUE NÃO!!
 * A menor distância entre Px e P1 é uma linha reta, exatamente o valor do harversine
 * Então quando eu colocar o offset 1 entre Px e P1, P1 só pode sofrer um push forward >= 0, nunca MENOR.
 *
 * -------------------------------------------------------------------
 * O método de inserção do algoritmo original é o seguinte:
 * Em uma rota aleatória, e com uma carona aleatória a adicionar, determinamos o ponto P de inserção.
 * A idéia do operador de inserção original é de que a rota construída até o momento de
 * 1 até p-1 não deve ser alterada.
 * A hora de atendimento desse novo ponto P
 *
 * Rota: A rota para inserir.
 * Carona: A carona para inserir
 * Posicao_insercao: uma posição que deve estar numa posição válida
 * offset: distância do source pro destino. posicao_insercao+offset < rota->length
 * Inserir_de_fato: útil para determinar se a rota acomoda o novo carona. se falso nada faz com o carona nem a rota.
 */
bool insere_carona_rota(Rota *rota, Request *carona, int posicao_insercao, int offset, bool inserir_de_fato){
	//if (posicao_insercao <= 0 || posicao_insercao >= rota->length || offset <= 0 || posicao_insercao + offset > rota->length) {
	//	printf("Parâmetros inválidos\n");
	//	return false;
	//}

	clone_rota(rota, &ROTA_CLONE);
	bool isRotaValida = false;
	insere_carona(ROTA_CLONE, carona, posicao_insercao, offset, true);
	insere_carona(ROTA_CLONE, carona, posicao_insercao+offset, 0, false);

	isRotaValida = is_rota_valida(ROTA_CLONE);

	if (isRotaValida && inserir_de_fato){
		carona->matched = true;
		carona->id_rota_match = ROTA_CLONE->id;
		clone_rota(ROTA_CLONE, &rota);

		increase_capacity(ROTA_CLONE);
		increase_capacity(rota);
	}

	return isRotaValida;
}

/*Insere uma quantidade variável de caronas na rota informada
 * Utilizado na geração da população inicial, e na reparação dos indivíduos quebrados
 * IMPORTANTE: Antes de chamar, os caronas devem estar determinados.
 * full_search: não para de tentar inserir depois de conseguir o primeiro match*/
void insere_carona_aleatoria_rota(Rota* rota, bool full_search){
	Request * request = &g->request_list[rota->id];

	int qtd_caronas_inserir = request->matchable_riders;
	if (qtd_caronas_inserir == 0 || qtd_caronas_combinados(rota) == qtd_caronas_inserir) return;
	
	fill_shuffle(index_array_caronas_inserir, 0, qtd_caronas_inserir);

	if (full_search){
		for (int z = 0; z < qtd_caronas_inserir; z++){
			int p = index_array_caronas_inserir[z];
			Request * carona = request->matchable_riders_list[p];
			if (!carona->matched){
				if (qtd_caronas_combinados(rota) == qtd_caronas_inserir)
					return;
				bool inseriu = false;
				//fill_shuffle(index_array_posicao_inicial,1, rota->length-1);
				for (int posicao_inicial = 1; posicao_inicial < rota->length; posicao_inicial++){
					//int posicao_inicial = index_array_posicao_inicial[pi-1];
					//fill_shuffle(index_array_offset, 1, rota->length - posicao_inicial);
					for (int offset = 1; offset <= rota->length - posicao_inicial; offset++){
						//int offset = index_array_offset[ot-1];
						inseriu = insere_carona_rota(rota, carona, posicao_inicial, offset, true);
						if(inseriu) break;
					}
					if(inseriu) break;
				}
			}
		}
	}
	else{
		for (int z = 0; z < qtd_caronas_inserir; z++){
			int p = index_array_caronas_inserir[z];
			Request * carona = request->matchable_riders_list[p];
			if (!carona->matched){
				if (qtd_caronas_combinados(rota) == qtd_caronas_inserir)
					return;
				bool inseriu = false;
				fill_shuffle(index_array_posicao_inicial,1, rota->length-1);
				for (int pi = 1; pi < rota->length; pi++){
					int posicao_inicial = index_array_posicao_inicial[pi-1];
					fill_shuffle(index_array_offset, 1, rota->length - posicao_inicial);
					for (int ot = 1; ot <= rota->length - posicao_inicial; ot++){
						int offset = index_array_offset[ot-1];
						inseriu = insere_carona_rota(rota, carona, posicao_inicial, offset, true);
						if(inseriu) return;
					}
				}
			}
		}
	}
}


/** Remove o carona que tem source na posicção Posicao_remocao
 * Retorna o valor do offset para encontrar o destino do carona removido
 * Retorna 0 caso não seja possível fazer a remoção do carona
 */
int desfaz_insercao_carona_rota(Rota *rota, int posicao_remocao){
	if (posicao_remocao > rota->length-2 || posicao_remocao <= 0 || rota->length < 4 || !rota->list[posicao_remocao].is_source) {
		printf("Erro ao desfazer a inserção\n");
		return 0;
	}
	
	int offset = 1;
	for (int k = posicao_remocao+1; k < rota->length; k++){//encontrando o offset
		if (rota->list[posicao_remocao].r == rota->list[k].r && !rota->list[k].is_source)
			break;
		offset++;
	}

	for (int i = posicao_remocao; i < rota->length-1; i++){
		rota->list[i] = rota->list[i+1];
	}
	rota->length--;

	for (int i = posicao_remocao+offset-1; i < rota->length-1; i++){
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

/**Avalia as funções objetivo de um indivíduo
 * sem verificar o grafo
 */
void evaluate_objective_functions(Individuo *idv, Graph *g){
	double distance = 0;
	double vehicle_time = 0;
	double rider_time = 0;
	double riders_unmatched = g->riders;
	for (int m = 0; m < idv->size; m++){//pra cada rota
		Rota *rota = &idv->cromossomo[m];

		double vehicle_timeTemp = tempo_gasto_rota(rota, 0, rota->length-1);
		double distanceTemp = distancia_percorrida(rota);
		vehicle_time += vehicle_timeTemp;
		distance += distanceTemp;

		for (int i = 1; i < rota->length-2; i++){//Pra cada um dos sources services
			Service *service = &rota->list[i];
			if (service->r->driver || !service->is_source)//só contabiliza os services source que não é o motorista
				continue;
			riders_unmatched--;
			//Repete o for até encontrar o destino
			//Ainda não considera o campo OFFSET contido no typedef SERVICE
			for (int j = i+1; j < rota->length-1; j++){
				Service *destiny = &rota->list[j];
				if(destiny->is_source || service->r != destiny->r)
					continue;

				double rider_time_temp = tempo_gasto_rota(rota, i, j);
				if (rider_time_temp < 0){
					printf("negativo!");
				}
				rider_time += rider_time_temp;
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
			(alfa * (idv->objetivos[TOTAL_DISTANCE_VEHICLE_TRIP] - TOTAL_DISTANCE_VEHICLE_TRIP_LOWER_BOUND)  / (TOTAL_DISTANCE_VEHICLE_TRIP_UPPER_BOUND - TOTAL_DISTANCE_VEHICLE_TRIP_LOWER_BOUND))
			+(beta * (idv->objetivos[TOTAL_TIME_VEHICLE_TRIPS] - TOTAL_TIME_VEHICLE_TRIPS_LOWER_BOUND) / (TOTAL_TIME_VEHICLE_TRIPS_UPPER_BOUND - TOTAL_TIME_VEHICLE_TRIPS_LOWER_BOUND))
			+(epsilon * (idv->objetivos[TOTAL_TIME_RIDER_TRIPS] - TOTAL_TIME_RIDER_TRIPS_LOWER_BOUND) / (TOTAL_TIME_RIDER_TRIPS_UPPER_BOUND - TOTAL_TIME_RIDER_TRIPS_LOWER_BOUND))
			+(sigma * (idv->objetivos[RIDERS_UNMATCHED] - RIDERS_UNMATCHED_LOWER_BOUND) / (RIDERS_UNMATCHED_UPPER_BOUND - RIDERS_UNMATCHED_LOWER_BOUND));

}


void evaluate_objective_functions_pop(Population* p, Graph *g){
	for (int i = 0; i < p->size; i++){//Pra cada um dos indivíduos
		evaluate_objective_functions(p->list[i], g);
	}
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
		case 4 :
			qsort(pop->list, pop->size, sizeof(Individuo*), compare_obj_f );
			break;
	}
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

/*Empurra os tempos de pickup e delivery de forma fixa. Economizando nos waiting times
 * A rota pode ficar inválida no final*/
void push_forward_hard(Rota *rota, int position, double pushf){
	Service * atual = &rota->list[position];
	if (pushf <= 0) return;
	atual->service_time+= pushf;

	for (int i = position+1; i < rota->length; i++){
		if (pushf <= 0)
			break;
		atual = &rota->list[i];
		Service * ant = &rota->list[i-1];

		double waiting_time = atual->service_time - ant->service_time -  minimal_time_between_services(ant, atual);
		waiting_time = fmax(0, waiting_time);
		pushf = fmax(0, pushf - waiting_time);

		atual->service_time+= pushf;
	}
}

void push_forward_mutation_op(Rota * rota){
	clone_rota(rota, &ROTA_CLONE_PUSH);
	int position = get_random_int(0, ROTA_CLONE_PUSH->length-1);
	Service * atual = &ROTA_CLONE_PUSH->list[position];
	double maxPushf = get_latest_time_service(atual) -  atual->service_time;
	double pushf = maxPushf * ((double)rand() / RAND_MAX);

	push_forward_hard(ROTA_CLONE_PUSH, position, pushf);

	bool rotaValida = is_rota_valida(ROTA_CLONE_PUSH);
	if (rotaValida){
		clone_rota(ROTA_CLONE_PUSH, &rota);
	}
}


/*Tenta empurar os services uma certa quantidade de tempo
 * retorna true se conseguiu fazer algum push forward
 * forcar_clone:  Mantem as alterações mesmo se o push forward não for feito
 * ou a rota for considerada inválida.*/
bool push_forward(Rota * rota, int position, double pushf, bool forcar_clone){
	clone_rota(rota, &ROTA_CLONE_PUSH);

	if (position == -1)
		position = get_random_int(0, ROTA_CLONE_PUSH->length-1);
	Service * atual = &ROTA_CLONE_PUSH->list[position];
	double maxPushf = get_latest_time_service(atual) -  atual->service_time;

	if (pushf == -1){
		pushf = maxPushf * ((double)rand() / RAND_MAX);
	}
	else{
		pushf = fmin (pushf, maxPushf);
	}

	if (pushf <= 0) return false;

	atual->service_time+= pushf;

	for (int i = position+1; i < ROTA_CLONE_PUSH->length; i++){
		if (pushf <= 0)
			break;
		atual = &ROTA_CLONE_PUSH->list[i];
		Service * ant = &ROTA_CLONE_PUSH->list[i-1];
		double bt = get_latest_time_service(atual);

		double waiting_time = atual->service_time - ant->service_time -  minimal_time_between_services(ant, atual);
		waiting_time = fmax(0, waiting_time);
		pushf = fmax(0, pushf - waiting_time);
		pushf = fmin(pushf, bt - atual->service_time);

		atual->service_time+= pushf;
	}
	bool rotaValida = is_rota_valida(ROTA_CLONE_PUSH);
	if (rotaValida || forcar_clone){
		clone_rota(ROTA_CLONE_PUSH, &rota);
	}
	return rotaValida;
}

/*Puxa os services uma certa quantidade de tempo
 * "Soft" porque se em algum passo não puder fazer o pushb todo, faz o resto que dá pra fazer
 * e continua até o fim. embora a rota final ainda possa ser inválida.
 */
void push_backward_soft(Rota *rota, int position, int pushb){
	//bool rotaValidaAntes = is_rota_valida(rota);
	Service * atual = &rota->list[position];
	double maisCedoPossivel = get_earliest_time_service(atual);
	if (position > 0){
		Service * ant = &rota->list[position-1];
		double srvTime = calculate_service_time(atual, ant);
		if (srvTime > maisCedoPossivel)
			maisCedoPossivel = srvTime;
	}

	double maxPushb = atual->service_time - maisCedoPossivel;
	pushb = fmin (pushb, maxPushb);

	if (pushb <= 0)
		return;

	atual->service_time-= pushb;

	for (int i = position+1; i < rota->length; i++){
		if (pushb == 0)
			break;
		atual = &rota->list[i];
		double at = get_earliest_time_service(atual);

		pushb = fmin(pushb, atual->service_time - at);

		atual->service_time-= pushb;
	}
}

/*pode falhar, por isso faz clone*/
void push_backward_mutation_op(Rota * rota, int position){
	clone_rota(rota, &ROTA_CLONE_PUSH);
	if (position == -1)
		position = get_random_int(0, ROTA_CLONE_PUSH->length-1);
	Service * atual = &ROTA_CLONE_PUSH->list[position];
	double maisCedoPossivel = get_earliest_time_service(atual);
	if (position > 0){
		Service * ant = &ROTA_CLONE_PUSH->list[position-1];
		double srvTime = calculate_service_time(atual, ant);
		if (srvTime > maisCedoPossivel)
			maisCedoPossivel = srvTime;
	}
	double maxPushb = atual->service_time - maisCedoPossivel;
	double pushb = maxPushb * ((double)rand() / RAND_MAX);
	push_backward_soft(ROTA_CLONE_PUSH, position, pushb);
	bool rotaValida = is_rota_valida(ROTA_CLONE_PUSH);
	if (rotaValida){
		clone_rota(ROTA_CLONE_PUSH, &rota);
	}
}

/*Tenta puxar os services uma certa quantidade de tempo
 * Se position = -1, gera aleatoriamente a posição*/
bool push_backward(Rota * rota, int position, double pushb, bool forcar_clone){
	clone_rota(rota, &ROTA_CLONE_PUSH);

	if (position == -1)
		position = get_random_int(0, ROTA_CLONE_PUSH->length-1);
	Service * atual = &ROTA_CLONE_PUSH->list[position];
	double ets = get_earliest_time_service(atual);
	double maisCedoPossivel = ets;

	if (position > 0){
		Service * ant = &ROTA_CLONE_PUSH->list[position-1];
		double srvTime = calculate_service_time(atual, ant);
		if (srvTime > maisCedoPossivel)
			maisCedoPossivel = srvTime;
	}

	double maxPushb = atual->service_time - maisCedoPossivel;

	if (pushb == -1){
		pushb = maxPushb * ((double)rand() / RAND_MAX);
	}
	else{
		pushb = fmin (pushb, maxPushb);
	}

	if (pushb <= 0) return false;

	atual->service_time-= pushb;

	for (int i = position+1; i < ROTA_CLONE_PUSH->length; i++){
		if (pushb == 0)
			break;
		atual = &ROTA_CLONE_PUSH->list[i];
		double at = get_earliest_time_service(atual);

		pushb = fmin(pushb, atual->service_time - at);

		atual->service_time-= pushb;
	}
	bool rotaValida = is_rota_valida(ROTA_CLONE_PUSH);
	if (rotaValida || forcar_clone){
		clone_rota(ROTA_CLONE_PUSH, &rota);
	}
	return rotaValida;
}


/** Transfere o carona de uma rota para outra
 *
 * Escolhe uma rota aleatória com carona.
 * Invalida o match temporariamente
 * Escolhe uma rota que possa fazer match com o carona
 * Tenta inserir o carona
 * Se conseguiu, remove o carona da rota original
 */
bool transfer_rider(Rota * rotaRemover, Individuo *ind, Graph * g){
	Rota * rotaInserir = NULL;
	Request * caronaInserir;

	//Procurando um carona qualquer
	int pos = get_random_carona_position(rotaRemover);
	if (pos == -1)
		return false;//Se não achou, retorna
	else
		caronaInserir = rotaRemover->list[pos].r;

	//Se 0 não tem nada pra fazer, se 1 então só pode na própria rota
	if (caronaInserir->matchable_riders < 2)
		return false;

	int k = rand() % caronaInserir->matchable_riders;
	rotaInserir = &ind->cromossomo[caronaInserir->matchable_riders_list[k]->id];

	//Troca a rota, se a escolhida aleatoriamente foi a própria rotaRemover
	if (rotaInserir == rotaRemover){
		if (k < caronaInserir->matchable_riders-1)
			k++;
		else
			k = 0;
		rotaInserir = &ind->cromossomo[caronaInserir->matchable_riders_list[k]->id];
	}

	bool conseguiu = false;
	//Invalida o carona
	caronaInserir->matched = false;

	for (int posicaoInserir = 1; posicaoInserir < rotaInserir->length; posicaoInserir++){
		for (int offset = 1; offset <= rotaInserir->length - posicaoInserir; offset++){
			conseguiu = insere_carona_rota(rotaInserir, caronaInserir, posicaoInserir, offset, true);
			if(conseguiu) break;
		}
		if(conseguiu) break;
	}

	//Se conseguiu inserir, remove o carona do rotaRemover
	if (conseguiu)
		desfaz_insercao_carona_rota(rotaRemover,pos);
	//Sempre vai ser válido em uma ou outra outra
	caronaInserir->matched = true;

	return conseguiu;
}


/** O bug mais difícil de descobrir:
 * get_random_int retornava qualquer coisa dentre o
 * primeiro carona e o ultimo source de carona.
 * mas se vc tem a seguinte rota
 * M C+ C+ C+ C+ M-
 *
 * ele poderia escolher o valor 3, que é um DESTINO do carona
 * A solução: além de adicionar uma verificação da posição de remoção do
 * desfaz_insercao_carona_rota, é fazer um mecanismo de buscar números aleatórios
 * ímpares.
 *
 *
 * Outro bug complicado: Remove_insert necessita que os matches das
 * rotas estejam determinados. para poder saber de onde tirar e onde colocar.
 *
 *
 * Update final:
 * As posições podem sim ser ímpares.
 * Retorna false se o resultado for inválido
 * (Seria um erro pois o push_backward é esperado gerar alteraçãos válidas)
 *
 */
bool remove_insert(Rota * rota){
	//Criando um clone local(como backup!!)

	clone_rota(rota, &ROTA_CLONE_REMOVE_INSERT);
	if (ROTA_CLONE_REMOVE_INSERT->length < 4) return false;
	int positionSources[(ROTA_CLONE_REMOVE_INSERT->length-2)/2];
	//Procurando as posições dos sources
	int k = 0;
	for (int i = 1; i < ROTA_CLONE_REMOVE_INSERT->length-2; i++){
		if (ROTA_CLONE_REMOVE_INSERT->list[i].is_source)
			positionSources[k++] = i;
	}
	int position = positionSources[rand() % (ROTA_CLONE_REMOVE_INSERT->length-2)/2];
	Request * carona = ROTA_CLONE_REMOVE_INSERT->list[position].r;
	int offset = desfaz_insercao_carona_rota(ROTA_CLONE_REMOVE_INSERT, position);

	//Calculando o push backward máximo
	//double horaMaisCedo = calculate_service_time(&ROTA_CLONE_REMOVE_INSERT->list[position], &ROTA_CLONE_REMOVE_INSERT->list[position-1]);
	//double PB = ROTA_CLONE_REMOVE_INSERT->list[position].service_time - horaMaisCedo;
	//if (PB > 0){
	//}

	//O push backward pode falhar, por isso a preferência é tentar um aleatório.
	push_backward_mutation_op(ROTA_CLONE_REMOVE_INSERT, position);

	if (offset > 1 && position+offset < ROTA_CLONE_REMOVE_INSERT->length-1){
		//Calculando o push backward máximo
		//double horaMaisCedoOffset = calculate_service_time(&ROTA_CLONE_REMOVE_INSERT->list[position+offset], &ROTA_CLONE_REMOVE_INSERT->list[position+offset-1]);
		//double PBOffset = ROTA_CLONE_REMOVE_INSERT->list[position+offset].service_time - horaMaisCedoOffset;
		//if (PBOffset > 0){
			//push_backward_soft(ROTA_CLONE_REMOVE_INSERT, position+offset, PBOffset);
		//}

		push_backward_mutation_op(ROTA_CLONE_REMOVE_INSERT, position+offset);
	}

	carona->matched = false;
	insere_carona_aleatoria_rota(ROTA_CLONE_REMOVE_INSERT, false);
	if (is_rota_valida(ROTA_CLONE_REMOVE_INSERT)){
		clone_rota(ROTA_CLONE_REMOVE_INSERT, &rota);
		return true;
	}
	else{
		carona->matched = true;
	}
	return false;
}

/*Escolhe um ponto aleatório e então troca o service de posição com o próximo */
bool swap_rider(Rota * rota){
	if (rota->length < 6) return false;
	clone_rota(rota, &ROTA_CLONE_SWAP);
	int ponto_swap = get_random_int(1, ROTA_CLONE_SWAP->length-4);
	Service service_temp = ROTA_CLONE_SWAP->list[ponto_swap];
	ROTA_CLONE_SWAP->list[ponto_swap] = ROTA_CLONE_SWAP->list[ponto_swap+1];
	ROTA_CLONE_SWAP->list[ponto_swap+1] = service_temp;

	Service *ant = &ROTA_CLONE_SWAP->list[ponto_swap-1];
	Service *atual = &ROTA_CLONE_SWAP->list[ponto_swap];
	Service *next = &ROTA_CLONE_SWAP->list[ponto_swap+1];

	if (atual->r == next->r)
		return false;

	atual->service_time = calculate_service_time(atual, ant);
	double nextTime = calculate_service_time(next, atual);

	double PF = nextTime - next->service_time;

	/*bool ordemValida = is_ordem_respeitada(ROTA_CLONE_SWAP);
	if (!ordemValida) {
		printf("muito improvável\n");
		return false;
	}*/

	push_forward_hard(ROTA_CLONE_SWAP, ponto_swap+1, PF);

	if(is_rota_valida(ROTA_CLONE_SWAP)){
		clone_rota(ROTA_CLONE_SWAP, &rota);
		return true;
	}
	return false;

}

/**
 * Repara o indivíduo, retirando todas as caronas repetidas.
 * No fim, as caronas com match são registradas no grafo
 */
void repair(Individuo *offspring, Graph *g){
	clean_riders_matches(g);
	for (int i = 0; i < offspring->size; i++){//Pra cada rota do idv
		Rota *rota = &offspring->cromossomo[i];

		//pra cada um dos services SOURCES na rota
		for (int j = 1; j < rota->length-1; j++){
			//Se é matched então algum SOURCE anterior já usou esse request
			//Então deve desfazer a rota de j até o offset
			if (rota->list[j].is_source && rota->list[j].r->matched){
				desfaz_insercao_carona_rota(rota, j);//Diminui length em duas unidades
				j--;
			}
			else if (rota->list[j].is_source){//Somente "senão", pois o tamanho poderia ter diminuido aí em cima.
				rota->list[j].r->matched = true;
				rota->list[j].r->id_rota_match = rota->id;
			}
		}
	}
}

void mutation(Individuo *ind, Graph *g, double mutationProbability){
	repair(ind, g);
	//shuffle(index_array_drivers_mutation, g->drivers);

	for (int r = 0; r < ind->size; r++){
		double accept = (double)rand() / RAND_MAX;
		if (accept <0.4){
			//int k = index_array_drivers_mutation[r];
			Rota * rota  = &ind->cromossomo[r];

			int op = rand() % 5;
			switch(op){
				case (0):{
					remove_insert(rota);
					break;
				}
				case (1):{
					transfer_rider(rota,ind, g);
					break;
				}
				case (2):{
					swap_rider(rota);
					break;
				}
				case (3):{
					push_backward_mutation_op(rota,-1);
					break;
				}
				case (4):{
					push_forward_mutation_op(rota);
					break;
				}
			}
		}
	}
}



void crossover(Individuo * parent1, Individuo *parent2, Individuo *offspring1, Individuo *offspring2, Graph *g, double crossoverProbability){
	int rotaSize = g->drivers;
	offspring1->size = rotaSize;
	offspring2->size = rotaSize;

	int crossoverPoint = get_random_int(1, rotaSize-2);
	double accept = (double)rand() / RAND_MAX;

	if (accept < crossoverProbability){

		copy_rota(parent2, offspring1, 0, crossoverPoint);
		copy_rota(parent1, offspring1, crossoverPoint, rotaSize);
		copy_rota(parent1, offspring2, 0, crossoverPoint);
		copy_rota(parent2, offspring2, crossoverPoint, rotaSize);

		repair(offspring1, g);
		insere_carona_aleatoria_individuo(offspring1, false);
		repair(offspring2, g);
		insere_carona_aleatoria_individuo(offspring2, false);
	}
	else{
		copy_rota(parent1, offspring1, 0, rotaSize);
		copy_rota(parent2, offspring2, 0, rotaSize);
	}
}


/*Gera uma população de filhos, usando seleção, crossover e mutação*/
void crossover_and_mutation(Population *parents, Population *offspring,  Graph *g, double crossoverProbability, double mutationProbability){
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


/*seleção por torneio, k = 2*/
Individuo * tournamentSelection(Population * parents){
	Individuo * best = NULL;
	for (int i = 0; i < 2; i++){
		int pos = rand() % parents->size;
		Individuo * outro = parents->list[pos];
		if (best == NULL || outro->objective_function < best->objective_function)
			best = outro;
	}
	return best;
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

int compare_obj_f(const void *p, const void *q) {
    int ret;
    Individuo * x = *(Individuo **)p;
    Individuo * y = *(Individuo **)q;
    if (x->objective_function == y->objective_function)
        ret = 0;
    else if (x->objective_function < y->objective_function)
        ret = -1;
    else
        ret = 1;
    return ret;
}














