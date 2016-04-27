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

/** Rota usada para a c�pia em opera��es de muta��o etc.*/
Rota *ROTA_CLONE;
Rota *ROTA_CLONE1;//Outros clones para n�o conflitar as c�pias
Rota *ROTA_CLONE2;
Rota *ROTA_CLONE_PUSH;

/** Aloca a ROTA_CLONE global */
void malloc_rota_clone(){
	/*Criando uma rota para c�pia e valida��o das rotas*/
	ROTA_CLONE = (Rota*) calloc(1, sizeof(Rota));
	ROTA_CLONE->list = calloc(MAX_SERVICES_MALLOC_ROUTE, sizeof(Service));

	ROTA_CLONE1 = (Rota*) calloc(1, sizeof(Rota));
	ROTA_CLONE1->list = calloc(MAX_SERVICES_MALLOC_ROUTE, sizeof(Service));

	ROTA_CLONE2 = (Rota*) calloc(1, sizeof(Rota));
	ROTA_CLONE2->list = calloc(MAX_SERVICES_MALLOC_ROUTE, sizeof(Service));

	ROTA_CLONE_PUSH = (Rota*) calloc(1, sizeof(Rota));
	ROTA_CLONE_PUSH->list = calloc(MAX_SERVICES_MALLOC_ROUTE, sizeof(Service));
}

/*Pra poder usar a fun��o qsort com N objetivos,
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

/*Ordena a popula��o de acordo com o objetivo 0, 1, 2, 3*/
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


void insere_carona(Rota *rota, Request *carona, int posicao_insercao, int offset, bool is_source){
	Service * ant = NULL;
	Service * atual = NULL;
	Service * next = NULL;
	double PF;
	double nextTime;

	int ultimaPos = rota->length-1;
	//Empurra todo mundo depois da posi��o de inser��o
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
	if (PF > 0) {
		next->service_time+= PF;
		if (posicao_insercao+2 < rota->length)
			push_forward(rota, posicao_insercao+2, PF, true);
	}
	rota->length++;
}


/**Minha d�vida sobre esse operador �:
 * ele pega as posi��es de inser��o P e seu offset, coloca na rota e
 * recalcula todo mundo � partir do hor�rio de sa�da do motorista?
 * Pensando bem, o rec�lculo � desnecess�rio. j� que
 * A+A- segue a id�ia do recalculo total
 * A+1+1-A- vai ser inserido no seus tempos mais cedo
 * A+1+1-2+2-A-, A rota vai ficar igual de A+ at� 1-, o resto que vai sofrer push forward.
 * Ent�o a resposta � SIM, a opera��o de recalcular os tempos de todo mundo � equivalente �
 * calcular a rota parcialmente do ponto P pra frente!
 *
 * A forma que est� escrito d� a enteder que primeiro o ponto de inser��o � adicionado,
 * � feito o push forward, ent�o o ponto de offset � adicionado.
 * Esse � um bom ponto de melhoria, pois se o push forward de P+1 for maior que o necess�rio,
 * a carona nesse canto podia ser v�lida e o algoritmo n�o pegou
 * Ex: vou inserir o ponto Px, entre P0 e P1, e meu offset � 1
 * PIOR QUE N�O!!
 * A menor dist�ncia entre Px e P1 � uma linha reta, exatamente o valor do harversine
 * Ent�o quando eu colocar o offset 1 entre Px e P1, P1 s� pode sofrer um push forward >= 0, nunca MENOR.
 *
 * -------------------------------------------------------------------
 * O m�todo de inser��o do algoritmo original � o seguinte:
 * Em uma rota aleat�ria, e com uma carona aleat�ria a adicionar, determinamos o ponto P de inser��o.
 * A id�ia do operador de inser��o original � de que a rota constru�da at� o momento de
 * 1 at� p-1 n�o deve ser alterada.
 * A hora de atendimento desse novo ponto P
 *
 * Rota: A rota para inserir.
 * Carona: A carona para inserir
 * Posicao_insercao: uma posi��o que deve estar numa posi��o v�lida
 * offset: dist�ncia do source pro destino. posicao_insercao+offset < rota->length
 * Inserir_de_fato: �til para determinar se a rota acomoda o novo carona. se falso nada faz com o carona nem a rota.
 */
bool insere_carona_rota(Rota *rota, Request *carona, int posicao_insercao, int offset, bool inserir_de_fato){
	if (posicao_insercao <= 0 || posicao_insercao >= rota->length || offset <= 0 || posicao_insercao + offset > rota->length) {
		printf("Par�metros inv�lidos\n");
		return false;
	}
	if (rota->id == 3 && carona->id == 368 && rota->length == 2)
		printf("Condi��o especial");

	clone_rota(rota, ROTA_CLONE);
	bool isRotaValida = false;
	double min_time = minimal_time_between_services(&rota->list[0], &rota->list[1]);
	insere_carona(ROTA_CLONE, carona, posicao_insercao, offset, true);
	insere_carona(ROTA_CLONE, carona, posicao_insercao+offset, 0, false);

	increase_capacity(ROTA_CLONE);
	increase_capacity(rota);

	isRotaValida = is_rota_valida(ROTA_CLONE);

	if (isRotaValida && inserir_de_fato){
		carona->matched = true;
		carona->id_rota_match = ROTA_CLONE->id;
		clone_rota(ROTA_CLONE, rota);
	}

	return isRotaValida;
}


/** Remove o carona que tem source na posic��o Posicao_remocao
 * Retorna o valor do offset para encontrar o destino do carona removido
 * Retorna 0 caso n�o seja poss�vel fazer a remo��o do carona
 */
int desfaz_insercao_carona_rota(Rota *rota, int posicao_remocao){
	if (posicao_remocao > rota->length-2 || posicao_remocao <= 0 || rota->length < 4 || !rota->list[posicao_remocao].is_source) {
		printf("Erro ao desfazer a inser��o\n");
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

/*Remove a marca��o de matched dos riders*/
void clean_riders_matches(Graph *g){
	for (int i = g->drivers; i < g->total_requests; i++){
		g->request_list[i].matched = false;
	}
}

void evaluate_objective_functions_pop(Population* p, Graph *g){
	for (int i = 0; i < p->size; i++){//Pra cada um dos indiv�duos
		evaluate_objective_functions(p->list[i], g);
	}
}

/**Avalia as fun��es objetivo de um indiv�duo
 * sem verificar o grafo
 */
void evaluate_objective_functions(Individuo *idv, Graph *g){
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
			if (service->r->driver || !service->is_source)//s� contabiliza os services source que n�o � o motorista
				continue;
			riders_unmatched--;
			//Repete o for at� encontrar o destino
			//Ainda n�o considera o campo OFFSET contido no typedef SERVICE
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

}



/*Insere uma quantidade vari�vel de caronas na rota informada
 * Utilizado na gera��o da popula��o inicial, e na repara��o dos indiv�duos quebrados
 * IMPORTANTE: Antes de chamar, os caronas devem estar determinados.*/
void insere_carona_aleatoria_rota(Rota* rota){
	Request * request = &g->request_list[rota->id];

	int qtd_caronas_inserir = request->matchable_riders;
	if (qtd_caronas_inserir == 0) return;
	/*Configurando o index_array usado na aleatoriza��o
	 * da ordem de leitura dos caronas
	 * Precisa fazer por causa do tamanho vari�vel*/
	for (int l = 0; l < qtd_caronas_inserir; l++){
		index_array_caronas_inserir[l] = l;
	}
	shuffle(index_array_caronas_inserir, qtd_caronas_inserir);

	for (int z = 0; z < qtd_caronas_inserir; z++){
		int p = index_array_caronas_inserir[z];
		Request * carona = request->matchable_riders_list[p];
		int posicao_inicial = get_random_int(1, rota->length-1);
		if (!carona->matched){
			for (int offset = 1; offset <= rota->length - posicao_inicial; offset++){
				//Situa��o antes de inserir
				bool inseriu = insere_carona_rota(rota, carona, posicao_inicial, offset, true);
				if(inseriu) break;
			}
		}
	}
}

/**
 * Insere caronas aleat�rias para todas as caronas da rota
 * IMPORTANTE: Antes de chamar, todos os caronas j� feito match devem estar no grafo
 */
void insere_carona_aleatoria_individuo(Individuo * ind){
	shuffle(index_array_drivers,g->drivers);
	for (int i = 0; i < ind->size; i++){
		int j = index_array_drivers[i];
		insere_carona_aleatoria_rota(&ind->cromossomo[j]);
	}
}


/*sele��o por torneio, k = 2*/
Individuo * tournamentSelection(Population * parents){
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

		repair(offspring1, g);
		insere_carona_aleatoria_individuo(offspring1);
		repair(offspring2, g);
		insere_carona_aleatoria_individuo(offspring2);
	}
	else{
		copy_rota(parent1, offspring1, 0, rotaSize);
		copy_rota(parent2, offspring2, 0, rotaSize);
	}
}


/**
 * Repara o indiv�duo, retirando todas as caronas repetidas.
 * No fim, as caronas com match s�o registradas no grafo
 */
void repair(Individuo *offspring, Graph *g){
	clean_riders_matches(g);
	for (int i = 0; i < offspring->size; i++){//Pra cada rota do idv
		Rota *rota = &offspring->cromossomo[i];
		
		//pra cada um dos services SOURCES na rota
		for (int j = 1; j < rota->length-1; j++){
			//Se � matched ent�o algum SOURCE anterior j� usou esse request
			//Ent�o deve desfazer a rota de j at� o offset
			if (rota->list[j].is_source && rota->list[j].r->matched){
				desfaz_insercao_carona_rota(rota, j);//Diminui length em duas unidades
				j--;
			}
			else if (rota->list[j].is_source){//Somente "sen�o", pois o tamanho poderia ter diminuido a� em cima.
				rota->list[j].r->matched = true;
				rota->list[j].r->id_rota_match = rota->id;
			}
		}
	}
}


bool swap_rider(Rota * rota){
	if (rota->length < 6) return false;
	clone_rota(rota, ROTA_CLONE1);
	int ponto_swap = get_random_int(1, ROTA_CLONE1->length-4);
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
 * Escolhe uma rota aleat�ria com carona.
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
		return false;//Se n�o achou, retorna
	else
		caronaInserir = rotaRemover->list[pos].r;

	//Se 0 n�o tem nada pra fazer, se 1 ent�o s� pode na pr�pria rota
	if (caronaInserir->matchable_riders < 2)
		return false;

	int k = rand() % caronaInserir->matchable_riders;
	rotaInserir = &ind->cromossomo[caronaInserir->matchable_riders_list[k]->id];

	if (rotaInserir == rotaRemover){
		if (k < caronaInserir->matchable_riders-1)
			k++;
		else
			k = 0;

		rotaInserir = &ind->cromossomo[caronaInserir->matchable_riders_list[k]->id];
	}

	bool conseguiu = false;
	int posicaoInserir = get_random_int(1, rotaInserir->length-1);
	//Invalida o carona
	caronaInserir->matched = false;
	conseguiu = insere_carona_rota(rotaInserir, caronaInserir, posicaoInserir, 1, true);//TODO variar o offset
	//Se conseguiu inserir, remove o carona do rotaRemover
	if (conseguiu)
		desfaz_insercao_carona_rota(rotaRemover,pos);
	//Sempre vai ser v�lido em uma ou outra outra
	caronaInserir->matched = true;

	return conseguiu;
}


/** O bug mais dif�cil de descobrir:
 * get_random_int retornava qualquer coisa dentre o
 * primeiro carona e o ultimo source de carona.
 * mas se vc tem a seguinte rota
 * M C+ C+ C+ C+ M-
 *
 * ele poderia escolher o valor 3, que � um DESTINO do carona
 * A solu��o: al�m de adicionar uma verifica��o da posi��o de remo��o do
 * desfaz_insercao_carona_rota, � fazer um mecanismo de buscar n�meros aleat�rios
 * �mpares.
 *
 *
 * Outro bug complicado: Remove_insert necessita que os matches das
 * rotas estejam determinados. para poder saber de onde tirar e onde colocar.
 *
 *
 * Update final:
 * As posi��es podem sim ser �mpares.
 * Retorna false se o resultado for inv�lido
 * (Seria um erro pois o push_backward � esperado gerar altera��os v�lidas)
 *
 */
bool remove_insert(Rota * rota){
	//Criando um clone local(como backup!!)

	clone_rota(rota, ROTA_CLONE1);
	if (ROTA_CLONE1->length < 4) return false;
	int positionSources[(ROTA_CLONE1->length-2)/2];
	//Procurando as posi��es dos sources
	int k = 0;
	for (int i = 1; i < ROTA_CLONE1->length-2; i++){
		if (ROTA_CLONE1->list[i].is_source)
			positionSources[k++] = i;
	}
	int position = positionSources[rand() % (ROTA_CLONE1->length-2)/2];
	Request * carona = ROTA_CLONE1->list[position].r;
	int offset = desfaz_insercao_carona_rota(ROTA_CLONE1, position);

	//Calculando o push backward m�ximo
	double horaMaisCedo = calculate_service_time(&ROTA_CLONE1->list[position], &ROTA_CLONE1->list[position-1]);
	double PF = ROTA_CLONE1->list[position].service_time - horaMaisCedo;
	push_backward(ROTA_CLONE1, position,PF, true);
	if (position+offset < ROTA_CLONE1->length){
		//Calculando o push backward m�ximo
		double horaMaisCedoOffset = calculate_service_time(&ROTA_CLONE1->list[position+offset], &ROTA_CLONE1->list[position+offset-1]);
		double PFOffset = ROTA_CLONE1->list[position+offset].service_time - horaMaisCedoOffset;
		push_backward(ROTA_CLONE1, position+offset, PFOffset, true);
	}

	carona->matched = false;
	insere_carona_aleatoria_rota(ROTA_CLONE1);
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
 * forcar_clone:  Mantem as altera��es mesmo se o push forward n�o for feito
 * ou a rota for considerada inv�lida.*/
bool push_forward(Rota * rota, int position, double pushf, bool forcar_clone){
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

		double waiting_time = atual->service_time - ant->service_time -  minimal_time_between_services(ant, atual);
		pushf = fmax(0, pushf - waiting_time);
		pushf = fmin(pushf, bt - atual->service_time);

		atual->service_time+= pushf;
	}
	bool rotaValida = is_rota_valida(ROTA_CLONE_PUSH);
	if (rotaValida || forcar_clone){
		clone_rota(ROTA_CLONE_PUSH, rota);
	}
	return rotaValida;
}

/*Tenta empurar os services uma certa quantidade de tempo
 * Se position = -1, gera aleatoriamente a posi��o*/
bool push_backward(Rota * rota, int position, double pushb, bool forcar_clone){
	clone_rota(rota, ROTA_CLONE_PUSH);

	if (position == -1)
		position = get_random_int(0, ROTA_CLONE_PUSH->length-1);
	Service * atual = &ROTA_CLONE_PUSH->list[position];
	double maxPushb = atual->service_time - get_earliest_time_service(atual);

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
		clone_rota(ROTA_CLONE_PUSH, rota);
	}
	return rotaValida;
}

void mutation(Individuo *ind, Graph *g, double mutationProbability){
	repair(ind, g);
	shuffle(index_array_drivers_mutation, g->drivers);

	for (int r = 0; r < ind->size; r++){
		double accept = (double)rand() / RAND_MAX;
		if (accept < mutationProbability){
			int k = index_array_drivers_mutation[r];
			Rota * rota  = &ind->cromossomo[k];

			int op = rand() % 5;
			switch(op){
				case (0):{
					push_backward(rota, -1, -1, false);
					break;
				}
				case (1):{
					push_forward(rota, -1, -1, false);
					break;
				}
				case (2):{
					remove_insert(rota);
					break;
				}
				case (3):{
					transfer_rider(rota,ind, g);
					break;
				}
				case (4):{
					swap_rider(rota);
					break;
				}
			}
		}
	}
}



/*Gera uma popula��o de filhos, usando sele��o, crossover e muta��o*/
void crossover_and_mutation(Population *parents, Population *offspring,  Graph *g, double crossoverProbability, double mutationProbability){
	offspring->size = 0;//Tamanho = 0, mas considera todos j� alocados
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

