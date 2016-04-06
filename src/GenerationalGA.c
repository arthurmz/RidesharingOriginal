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

/** Rota usada para a cópia em operações de mutação etc.*/
Rota *ROTA_CLONE;
Rota *ROTA_CLONE1;//Outros clones para não conflitar as cópias
Rota *ROTA_CLONE2;
Rota *ROTA_CLONE_PUSH;

/** Aloca a ROTA_CLONE global */
void malloc_rota_clone(){
	/*Criando uma rota para cópia e validação das rotas*/
	ROTA_CLONE = (Rota*) calloc(1, sizeof(Rota));
	ROTA_CLONE->list = calloc(MAX_SERVICES_MALLOC_ROUTE, sizeof(Service));

	ROTA_CLONE1 = (Rota*) calloc(1, sizeof(Rota));
	ROTA_CLONE1->list = calloc(MAX_SERVICES_MALLOC_ROUTE, sizeof(Service));

	ROTA_CLONE2 = (Rota*) calloc(1, sizeof(Rota));
	ROTA_CLONE2->list = calloc(MAX_SERVICES_MALLOC_ROUTE, sizeof(Service));

	ROTA_CLONE_PUSH = (Rota*) calloc(1, sizeof(Rota));
	ROTA_CLONE_PUSH->list = calloc(MAX_SERVICES_MALLOC_ROUTE, sizeof(Service));
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
 *
 * NÃO ESTÁ SENDO USADO.
 */
bool update_times(Rota *rota, int p){
	for (int i = p; i >= rota->length; i++){
		Service *anterior = &rota->list[i-1];
		Service *atual = &rota->list[i];
		double at = get_earliest_time_service(atual);
		double bt = get_latest_time_service(atual);

		double tbs = minimal_time_between_services(anterior, atual);

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
	if (posicao_insercao <= 0 || posicao_insercao >= rota->length || offset <= 0 || posicao_insercao + offset > rota->length) {
		printf("Parâmetros inválidos\n");
		return false;
	}

	clone_rota(rota, ROTA_CLONE);
	bool isRotaValida = false;
	Service * ant = NULL;
	Service * atual = NULL;
	Service * next = NULL;
	double PF;
	double nextTime;

	int ultimaPos = ROTA_CLONE->length-1;
	//Empurra todo mundo depois da posição de inserção
	for (int i = ultimaPos; i >= posicao_insercao; i--){
		ROTA_CLONE->list[i+1] = ROTA_CLONE->list[i];
	}

	ant = &ROTA_CLONE->list[posicao_insercao-1];
	atual = &ROTA_CLONE->list[posicao_insercao];
	next = &ROTA_CLONE->list[posicao_insercao+1];

	atual->r = carona;
	atual->is_source = true;
	atual->offset = offset;
	atual->service_time = calculate_service_time(atual, ant);

	nextTime = calculate_service_time(next, atual);
	PF = nextTime - next->service_time;
	if (PF > 0) {
		next->service_time+= PF;
		if (posicao_insercao+2 < ROTA_CLONE->length)
			push_forward(ROTA_CLONE, posicao_insercao+2, PF, true);
	}
	ROTA_CLONE->length++;

	//Empurra todo mundo depois da posição do offset
	for (int i = ultimaPos+1; i >= posicao_insercao + offset; i--){
		ROTA_CLONE->list[i+1] = ROTA_CLONE->list[i];
	}
	ant = &ROTA_CLONE->list[posicao_insercao+offset-1];
	atual = &ROTA_CLONE->list[posicao_insercao+offset];
	next = &ROTA_CLONE->list[posicao_insercao+offset+1];

	atual->r = carona;
	atual->is_source = false;
	atual->offset = 0;
	atual->service_time = calculate_service_time(atual, ant);

	nextTime = calculate_service_time(next, atual);
	PF = nextTime - next->service_time;
	if (PF > 0) {
		next->service_time+= PF;
		if (posicao_insercao+offset+2 < ROTA_CLONE->length){
			push_forward(ROTA_CLONE, posicao_insercao+offset+2, PF, true);
		}
	}
	ROTA_CLONE->length++;

	//Aumentar a capacidade se tiver chegando no limite
	if (ROTA_CLONE->length == ROTA_CLONE->capacity - 4){
		ROTA_CLONE->capacity += MAX_SERVICES_MALLOC_ROUTE;
		ROTA_CLONE->list = realloc(ROTA_CLONE->list, ROTA_CLONE->capacity * sizeof(Service));
	}
	//Aumentar a capacidade se tiver chegando no limite
	if (rota->length == rota->capacity - 4){
		rota->capacity += MAX_SERVICES_MALLOC_ROUTE;
		rota->list = realloc(rota->list, rota->capacity * sizeof(Service));
	}

	isRotaValida = is_rota_valida(ROTA_CLONE);

	if (isRotaValida && inserir_de_fato){
		carona->matched = true;
		carona->id_rota_match = ROTA_CLONE->id;
		clone_rota(ROTA_CLONE, rota);
	}

	return isRotaValida;
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
 * Utilizado na geração da população inicial, e na reparação dos indivíduos quebrados
 * try_all_offsets: se true então tenta inserir o destino em cada um dos pontos subsequentes à p na rota
 *
 * IMPORTANTE: Antes de chamar, os caronas devem estar determinados.*/
void insere_carona_aleatoria_rota(Rota* rota, bool try_all_offsets){
	Request * request = &g->request_list[rota->id];

	int qtd_caronas_inserir = request->matchable_riders;
	if (qtd_caronas_inserir == 0) return;

	/*Configurando o index_array usado na aleatorização
	 * da ordem de leitura dos caronas*/
	for (int l = 0; l < qtd_caronas_inserir; l++){
		index_array_caronas_inserir[l] = l;
	}

	shuffle(index_array_caronas_inserir, qtd_caronas_inserir);

	for (int z = 0; z < qtd_caronas_inserir; z++){
		int p = index_array_caronas_inserir[z];
		Request * carona = request->matchable_riders_list[p];
		int posicao_inicial = get_random_int(1, rota->length-1);

		if (!carona->matched){
			if (try_all_offsets){
				for (int offset = 1; offset <= rota->length - posicao_inicial; offset++){
					bool inseriu = insere_carona_rota(rota, carona, posicao_inicial, offset, true);
					if(inseriu) break;
				}
			}
			else{
				int offset = get_random_int(1, rota->length - posicao_inicial);
				bool inseriu = insere_carona_rota(rota, carona, posicao_inicial, offset, true);
				if(inseriu) break;
			}
		}
	}
}

/**
 * Insere caronas aleatórias para todas as caronas da rota
 * IMPORTANTE: Antes de chamar, todos os caronas já feito match devem estar no grafo
 */
void insere_carona_aleatoria_individuo(Individuo * ind){
	for (int i = 0; i < ind->size; i++){
		insere_carona_aleatoria_rota(&ind->cromossomo[i], true);
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

	int crossoverPoint = get_random_int(1, rotaSize-2);
	double accept = (double)rand() / RAND_MAX;

	if (accept < crossoverProbability){

		copy_rota(parent2, offspring1, 0, crossoverPoint);
		copy_rota(parent1, offspring1, crossoverPoint, rotaSize);
		copy_rota(parent1, offspring2, 0, crossoverPoint);
		copy_rota(parent2, offspring2, crossoverPoint, rotaSize);

		shuffle(index_array_riders, g->riders);
		repair(offspring1, g);
		evaluate_riders_matches(offspring1, g);

		insere_carona_aleatoria_individuo(offspring1);

		shuffle(index_array_riders, g->riders);
		repair(offspring2, g);

		insere_carona_aleatoria_individuo(offspring2);
	}
	else{
		copy_rota(parent1, offspring1, 0, rotaSize);
		copy_rota(parent2, offspring2, 0, rotaSize);
	}
}

/**
 * Atualiza o grafo com os matches do indivíduo atual*/
void evaluate_riders_matches(Individuo *ind, Graph * g){
	clean_riders_matches(g);
	for (int i = 0; i < ind->size; i++){//Pra cada rota do idv
		Rota *rota = &ind->cromossomo[i];
		for (int j = 1; j < rota->length-1; j++){
			if (rota->list[j].is_source && rota->list[j].r->matched){
				int rotaPositionBug = -12;
				bool achou = false;
				Rota * rotaBug;
				for (int g = 0; g < i; g++){
					rotaBug = &ind->cromossomo[g];
					for (int p = 1; p < rota->length -1; p++){
						if (rotaBug->list[p].r == rota->list[j].r){
							printf("local = %d, %d", g, p);
							rotaPositionBug = g;
							achou = true;
							break;
						}
					}
					if (achou)
						break;
				}
				if (achou)
					printf("bug e achou\n");
				else
					printf("bug e não achou\n");
			}
			if (rota->list[j].is_source)
				rota->list[j].r->matched = true;
		}
	}
}

/**
 * Repara o indivíduo, retirando todas as caronas repetidas.
 * No fim, as caronas com match são registradas no grafo
 */
void repair(Individuo *offspring, Graph *g){
	clean_riders_matches(g);
	//find_bug_cromossomo(offspring, g, 20);
	for (int i = 0; i < offspring->size; i++){//Pra cada rota do idv
		Rota *rota = &offspring->cromossomo[i];

		int j = 1;
		//pra cada um dos services SOURCES na rota
		while (j < (rota->length -1) ) {
			//Se é matched então algum SOURCE anterior já usou esse request
			//Então deve desfazer a rota de j até o offset
			if ((rota->list[j].is_source && rota->list[j].r->matched)){
				desfaz_insercao_carona_rota(rota, j);//Diminui length em duas unidades
			}
			else if (rota->list[j].is_source){//Somente "senão", pois o tamanho poderia ter diminuido aí em cima.
				rota->list[j].r->matched = true;
			}
			j++;
		}
	}
}


bool swap_rider(Rota * rota){
	if (rota->length < 6) return false;
	clone_rota(rota, ROTA_CLONE1);
	int ponto_swap = get_random_int(1, ROTA_CLONE1->length-3);
	Service service_temp = ROTA_CLONE1->list[ponto_swap];
	ROTA_CLONE->list[ponto_swap] = ROTA_CLONE1->list[ponto_swap+1];
	ROTA_CLONE->list[ponto_swap+1] = service_temp;

	Service *ant = &ROTA_CLONE1->list[ponto_swap-1];
	Service *atual = &ROTA_CLONE1->list[ponto_swap];
	Service *next = &ROTA_CLONE1->list[ponto_swap+1];

	atual->service_time = calculate_service_time(atual, ant);
	double nextTime = calculate_service_time(next, atual);

	double PF = nextTime - next->service_time;

	bool ordemValida = is_ordem_respeitada(ROTA_CLONE1);
	if (!ordemValida) return false;

	push_forward(ROTA_CLONE1, ponto_swap+1, PF, true);

	if(is_rota_valida(ROTA_CLONE1)){
		clone_rota(ROTA_CLONE1, rota);
		return true;
	}
	return false;

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

	//Invalida o carona
	caronaInserir->matched = false;

	bool conseguiu = false;
	//Buscando por uma rota que possa inserir
	for (int i = 0; i < g->drivers; i++){
		rotaInserir = &ind->cromossomo[i];
		if (rotaInserir == rotaRemover)
			continue;
		if (contains(rotaInserir->list[0].r, caronaInserir)){
			int posicaoInserir = get_random_int(1, rotaInserir->length-1);
			conseguiu = insere_carona_rota(rotaInserir, caronaInserir, posicaoInserir, 1, true);//TODO variar o offset
		}
		if (conseguiu)
			break;
	}

	//Se conseguiu inserir, remove o carona do rotaRemover
	if (conseguiu)
		desfaz_insercao_carona_rota(rotaRemover,pos);
	else
		caronaInserir->matched = true;//Revalida o carona
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

	clone_rota(rota, ROTA_CLONE1);
	if (ROTA_CLONE1->length < 4) return false;
	int positionSources[(ROTA_CLONE1->length-2)/2];
	//Procurando as posições dos sources
	int k = 0;
	for (int i = 1; i < ROTA_CLONE1->length-2; i++){
		if (ROTA_CLONE1->list[i].is_source)
			positionSources[k++] = i;
	}
	int position = positionSources[rand() % (ROTA_CLONE1->length-2)/2];
	Request * carona = ROTA_CLONE1->list[position].r;
	int offset = desfaz_insercao_carona_rota(ROTA_CLONE1, position);
	carona->matched = false;
	push_backward(ROTA_CLONE1, position, true);
	if (position+offset < ROTA_CLONE1->length)
		push_backward(ROTA_CLONE1, position+offset, true);
	insere_carona_aleatoria_rota(ROTA_CLONE1, true);
	if (is_rota_valida(ROTA_CLONE1)){
		clone_rota(ROTA_CLONE1, rota);
		return true;
	}
	else{
		carona->matched = true;
	}
	return false;
}

/*Tenta empurar os services uma certa quantidade de tempo
 * retorna true se conseguiu fazer algum push forward
 * manter_alteracoes:  Mantem as alterações mesmo se o push forward não for feito
 * ou a rota for considerada inválida.*/
bool push_forward(Rota * rota, int position, double pushf, bool manter_alteracoes){
	clone_rota(rota, ROTA_CLONE_PUSH);

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
		if (pushf == 0)
			break;
		atual = &ROTA_CLONE_PUSH->list[i];
		Service * ant = &ROTA_CLONE_PUSH->list[i-1];
		double bt = get_latest_time_service(atual);

		double waiting_time = atual->service_time - ant->service_time - haversine(ant, atual);
		pushf = fmax(0, pushf - waiting_time);
		pushf = fmin(pushf, bt - atual->service_time);

		atual->service_time+= pushf;
	}
	bool rotaValida = is_rota_valida(ROTA_CLONE_PUSH);
	if (rotaValida || manter_alteracoes){
		clone_rota(ROTA_CLONE_PUSH, rota);
	}
	return rotaValida;
}

/*Tenta empurar os services uma certa quantidade de tempo
 * Se position = -1, gera aleatoriamente a posição*/
bool push_backward(Rota * rota, int position, bool manter_alteracoes){
	clone_rota(rota, ROTA_CLONE_PUSH);

	if (position == -1)
		position = get_random_int(0, ROTA_CLONE_PUSH->length-1);
	Service * atual = &ROTA_CLONE_PUSH->list[position];
	double pushb = atual->service_time - get_earliest_time_service(atual);
	pushb = pushb * ((double)rand() / RAND_MAX);
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
	if (rotaValida || manter_alteracoes){
		clone_rota(ROTA_CLONE_PUSH, rota);
	}
	return rotaValida;
}

void mutation(Individuo *ind, Graph *g, double mutationProbability){

	evaluate_riders_matches(ind, g);

	for (int r = 0; r < ind->size; r++){
		double accept = (double)rand() / RAND_MAX;
		if (accept < mutationProbability){
			Rota * rota  = &ind->cromossomo[r];



			int op = rand() % 5;
			switch(op){
				case (0):{
					//push_backward(rota, -1, false);
					break;
				}
				case (1):{
					//push_forward(rota, -1, -1, false);
					break;
				}
				case (2):{
					//remove_insert(rota);
					break;
				}
				case (3):{
					//transfer_rider(rota,ind, g);
					break;
				}
				case (4):{
					//swap_rider(rota);
					break;
				}
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

		evaluate_riders_matches(offspring1, g);
		//evaluate_riders_matches(offspring2, g);

		mutation(offspring1, g, mutationProbability);
		mutation(offspring2, g, mutationProbability);
		offspring->size += 2;
	}
}

