/*
 * Validations.c
 *
 *  Created on: 4 de mar de 2016
 *      Author: SIGAA-Des
 */

#include "Calculations.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

inline int qtd_caronas_combinados(Rota *rota){
	if (rota->length <= 2)
		return 0;
	return (rota->length - 2)/2;

}

/** Arredonda o número para 1 casa decimal */
inline double round_2_decimal(double n){
	return round(n * 10.0) / 10.0;
}

//True se a <= b, com diferença < epsilon
bool leq(double a, double b){
	return a <= b || (a - b) < EPSILON;
}

/**Retorna um número inteiro entre minimum_number e maximum_number, inclusive */
int get_random_int(int minimum_number, int max_number){
	int modulo = max_number + 1 - minimum_number;
	return minimum_number + (rand() % modulo);
}

/** Recebe uma rota e calcula a distância percorrida pelo motorista */
double distancia_percorrida(Rota * rota){
	double accDistance = 0;
	for (int i = 0; i < rota->length -1; i++){
		Service *a = &rota->list[i];
		Service *b = &rota->list[i+1];
		double distancia = haversine(a,b);
		double tempoGastoRota = tempo_gasto_rota(rota, i, i+1);
		if (distancia > tempoGastoRota)
			distancia = tempoGastoRota;//Evita que os arredontamentos pra cima do haversine ultrapassem o tempo gasto da rota.
		accDistance += distancia;
	}

	return accDistance;
}

/*Distância em km ORIGINAL*/
double haversine_helper_or(double lat1, double lon1, double lat2, double lon2){
	const double R = 6371;//Recomendado. não mudar
	const double to_rad = 3.1415926536 / 180;
	double dLat = to_rad * (lat2 - lat1);
	double dLon = to_rad * (lon2 - lon1);

	lat1 = to_rad * lat1;
	lat2 = to_rad * lat2;

	double a = pow (sin(dLat/2),2) + pow(sin(dLon/2),2) * cos(lat1) * cos(lat2);
	double c = 2 * asin(sqrt(a));
	return R * c;
}

/** rosetacode */
double haversine_helper_ros(double th1, double ph1, double th2, double ph2){
	double TO_RAD = 3.1415926536 / 180;
	double R = 6371;//Valor que chega mais perto do artigo
	double dx, dy, dz;
	ph1 -= ph2;
	ph1 *= TO_RAD, th1 *= TO_RAD, th2 *= TO_RAD;

	dz = sin(th1) - sin(th2);
	dx = cos(ph1) * cos(th1) - cos(th2);
	dy = sin(ph1) * cos(th1);
	double result = asin(sqrt(dx * dx + dy * dy + dz * dz) / 2) * 2 * R;
	return round(result * 10.0)/10.0;
}

//Versão aproximada e mais rápida
double haversine_helper(double lat1, double lon1, double lat2, double lon2){
	const double R = 6371;
	const double to_rad = 3.1415926536 / 180;

	lat1 = lat1 * to_rad;
	lon1 = lon1 * to_rad;
	lat2 = lat2 * to_rad;
	lon2 = lon2 * to_rad;

	double x = (lon2 - lon1) * cos((lat1 + lat2) / 2);
	double y = (lat2 - lat1);
	double result = sqrt(x * x + y * y) * R;
	return result;
}

//versão do excel
double haversine_helper_excel(double lat1, double lon1, double lat2, double lon2){
	const double to_rad = 3.1415926536 / 180;
	double x = cos((90-lat1)*to_rad);
	double y = cos((90-lat2)*to_rad);
	double z = sin((90-lat1)*to_rad);
	double w = sin((90-lat2)*to_rad);
	double p = cos((lon1-lon2)*to_rad);

	return acos(x * y + z * w * p) *6371;
}

/** Distância entre os services */
double haversine(Service *a, Service *b){
	double lat1, long1, lat2, long2;
	if (a->is_source){
		lat1 = a->r->pickup_location_latitude;
		long1 = a->r->pickup_location_longitude;
	}
	else{
		lat1 = a->r->delivery_location_latitude;
		long1 = a->r->delivery_location_longitude;
	}

	if (b->is_source){
		lat2 = b->r->pickup_location_latitude;
		long2 = b->r->pickup_location_longitude;
	}
	else {
		lat2 = b->r->delivery_location_latitude;
		long2 = b->r->delivery_location_longitude;
	}
	return haversine_helper(lat1, long1, lat2, long2);
}

/** Retorna o tempo necessário para ir do ponto
 * de pickup ao ponto de delivery de um mesmo request
 * Usado para determinar a janela de tempo corretamente.
 */
double minimal_time_request(Request *rq){
	double distance = haversine_helper(rq->pickup_location_latitude, rq->pickup_location_longitude,
			rq->delivery_location_latitude, rq->delivery_location_longitude);
	return distance / VEHICLE_SPEED * 60.0;
}

/*Tempo de viagem em minutos entre o ponto A e o ponto B*/
double minimal_time_between_services(Service *a, Service *b){
	double distance = haversine(a, b);
	return distance / VEHICLE_SPEED * 60;
}

/*Calcula o tempo gasto para ir do ponto i ao ponto j, através de cada
 * request da rota.
 * Os tempos deve estar configurados corretamente nos services*/
double tempo_gasto_rota(Rota *rota, int i, int j){
	return rota->list[j].service_time - rota->list[i].service_time;
}

/**Calcula o service_time mais cedo possível para actual. Baseado
 * no service_time de ant */
double calculate_service_time(Service * actual, Service *ant){
	double next_time = 0;

	double st;
	if (ant->is_source)
		st = ant->r->service_time_at_source;
	else
		st = ant->r->service_time_at_delivery;

	double at = get_earliest_time_service(actual);

	next_time = ant->service_time + st + minimal_time_between_services(ant, actual);
	next_time = fmax(next_time, at);

	return next_time;
}

inline double get_earliest_time_service(Service * atual){
	if (atual->is_source)
		return atual->r->pickup_earliest_time;
	return atual->r->delivery_earliest_time;
}

inline double get_latest_time_service(Service * atual){
	if (atual->is_source)
		return atual->r->pickup_latest_time;
	return atual->r->delivery_latest_time;
}

bool is_dentro_janela_tempo_is_tempos_respeitados(Rota * rota){

	for (int i = 0; i < rota->length-1; i++){
		if (rota->list[i+1].service_time < rota->list[i].service_time)
			return false;
		Service *source = &rota->list[i];
		if (!source->is_source) continue;
		for (int j = i+1; j < rota->length; j++){
			Service *destiny = &rota->list[j];
			if(destiny->is_source || source->r != destiny->r) continue;

			//Janela de tempo
			if ( ! ((leq(source->r->pickup_earliest_time, source->service_time) && leq(source->service_time, source->r->pickup_latest_time))
				&& (leq(destiny->r->delivery_earliest_time, destiny->service_time) && leq(destiny->service_time, destiny->r->delivery_latest_time))))
				return false;

			/*Verifica se os tempos de todos os requests nessa rota estão sendo respeitados*/
			if (!is_tempo_respeitado(rota, i, j)){
				return false;
			}
			break;
		}
	}
	return true;

}
/*Verifica se durante toda a rota a carga permanece dentro do limite
 * e se todos os passageiros embarcados são desembarcados, terminando com carga 0*/
bool is_carga_dentro_limite(Rota *rota){
	int temp_load = 0;
	for (int i = 1; i < rota->length-1; i++){
		Service *a = &rota->list[i];

		if(a->is_source){
			temp_load += RIDER_DEMAND;
			if (temp_load > VEHICLE_CAPACITY) return false;
		}
		else{
			temp_load -= RIDER_DEMAND;
			if (temp_load < 0) return false;
		}
	}

	if (temp_load != 0) return false;
	return true;
}


/*Verifica se durante toda a rota a carga permanece dentro do limite
 * PODE SER QUE UM PASSAGEIRO EMBARQUE MAS NÃO DESEMBARQUE - ÚTIL PARA AS ROTAS PARCIALMENTE CONSTRUÍDAS.*/
bool is_carga_dentro_limite2(Rota *rota){
	int temp_load = 0;
	for (int i = 1; i < rota->length-1; i++){
		Service *a = &rota->list[i];

		if(a->is_source){
			temp_load += RIDER_DEMAND;
			if (temp_load > VEHICLE_CAPACITY) return false;
		}
		else{
			temp_load -= RIDER_DEMAND;
			if (temp_load < 0) return false;
		}
	}
	return true;
}

/*Verifica se a distancia percorrida pelo motorista é respeitada*/
bool is_distancia_motorista_respeitada(Rota * rota){
	Service * source = &rota->list[0];
	Service * destiny = &rota->list[rota->length-1];
	double MTD = ceil(AD + (BD * haversine(source, destiny)));//Maximum Travel Distance
	double accDistance = distancia_percorrida(rota);
	bool ok = leq(accDistance, MTD);
	return ok;
}

/*Verifica se o tempo do request partindo do índice I e chegando no índice J é respeitado*/
bool is_tempo_respeitado(Rota *rota, int i, int j){
	Service * source = &rota->list[i];
	Service * destiny = &rota->list[j];
	double MTT = ceil(AT + (BT * minimal_time_between_services(source, destiny)));
	double accTime = tempo_gasto_rota(rota, i, j);
	return leq(accTime, MTT) && accTime >= 0;//Não é válido se o tempo acumulado for negativo
}

/*Verifica se a ordem de inserção e remoção dos riders é respeitada
 * Usar apenas no swap, pois só lá essa restrição pode ser quebrada*/
bool is_ordem_respeitada(Rota * rota){
	int existe(Request ** p, int alt, Request * content){
		for (int i = 0; i < alt; i++){
			if (p[i] == content)
				return i;
		}
		return -1;
	}

	void remove(Request ** p, int alt, int pos){
		for (int i = pos; i < alt; i++){
			p[i] = p[i+1];
		}
	}

	Request * pilha[rota->length];
	int altura = 0;
	for (int i = 0; i < rota->length; i++){//Pra cada um dos sources
		Service *source = &rota->list[i];
		if (source->is_source)
			pilha[altura++] = source->r;
		else{
			int pos = existe(pilha, altura, source->r);
			if (pos != -1){
				remove(pilha, altura, pos);
				altura--;
			}
		}
	}
	return altura == 0;
}



/*Restrição 2 e 3 do artigo é garantida pois sempre que um carona é adicionado
 * seu destino tbm é adicionado
 * Restrição 4 é garantida pois ao fazer o match o carona não pode ser usado pra outras inserções
 * Restrição 5, uma vez que só pode ser feito match uma vez, tá garantido
 * Restrição 6 tem que garantir que a hora que eu chego no ponto J não pode ser maior do que a
 * soma da hora de chegada no ponto anterior com o tempo de viajgem entre IJ.
 * Restrição 7 tem que garantir que a janela de tempo está sendo satisfeitaa TODO
 * Restrição 8 já é atendida pela forma de inserção do carona
 * Restirção 9 e 10 A carga da rota tem que ser verificada a cada inserção
 * OU seja, tem que verificar que a todo instante a carga está dentro do limite
 * Restrição 11 é garantida pela forma como é feita a inserção
 * Restrição 12 e 13 é a verificação da distancia e tempo do motorista
 * Restrição 14 é a verificação de tempo do carona*/
bool is_rota_valida(Rota *rota){

	/*Verificando se os tempos de chegada em cada ponto atende às janelas de tempo de cada request (Driver e Rider)*/
	if (!is_carga_dentro_limite(rota) || !is_dentro_janela_tempo_is_tempos_respeitados(rota) || !is_distancia_motorista_respeitada(rota) )
		return false;
	return true;
}


bool is_rota_parcialmente_valida(Rota *rota){

	/*Verificando se os tempos de chegada em cada ponto atende às janelas de tempo de cada request (Driver e Rider)*/
	if (!is_carga_dentro_limite2(rota) || !is_dentro_janela_tempo_is_tempos_respeitados(rota) || !is_distancia_motorista_respeitada(rota) )
		return false;
	return true;
}
