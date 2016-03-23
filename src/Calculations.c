/*
 * Validations.c
 *
 *  Created on: 4 de mar de 2016
 *      Author: SIGAA-Des
 */

#include "Calculations.h"
#include <math.h>

double distancia_percorrida(Rota * rota){
	double accDistance =0;
	for (int i = 0; i < rota->length -1; i++){
		Service *a = &rota->list[i];
		Service *b = &rota->list[i+1];
		accDistance += haversine(a,b);
	}

	return accDistance;
}

/*Distância em km*/
double haversine_helper(double lat1, double lon1, double lat2, double lon2){
	double R = 6372.8;
	double to_rad = 3.1415926536 / 180;
	double dLat = to_rad * (lat2 - lat1);
	double dLon = to_rad * (lon2 - lon1);

	lat1 = to_rad * lat1;
	lat2 = to_rad * lat2;

	double a = pow (sin(dLat/2),2) + pow(sin(dLon/2),2) * cos(lat1) * cos(lat2);
	double c = 2 * asin(sqrt(a));
	return R * c;
}

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

/*Tempo em minutos*/
double time_between_services(Service *a, Service *b){
	double distance = haversine(a, b);
	return distance / VEHICLE_SPEED * 60;
}

/*Calcula o tempo gasto para ir do ponto i ao ponto j, através de cada
 * request da rota.*/
double tempo_gasto_rota(Rota *rota, int i, int j){
	double accTime =0;
	for (int k = i; k < j; k++){
		Service *a = &rota->list[k];
		Service *b = &rota->list[k+1];
		accTime += time_between_services(a,b);
	}
	return accTime;
}

/**Calcula o service_time mais cedo possível para actual. Baseado
 * no service_time de ant */
double calculate_time_at(Service * actual, Service *ant){
	double next_time = 0;

	double st;
	if (ant->is_source)
		st = ant->r->service_time_at_source;
	else
		st = ant->r->service_time_at_delivery;

	double janela_tempo_a;
	if (actual->is_source)
		janela_tempo_a = actual->r->pickup_earliest_time;
	else
		janela_tempo_a = actual->r->delivery_earliest_time;


	next_time = ant->service_time + st + time_between_services(ant, actual);
	next_time = fmax(next_time, janela_tempo_a);


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

bool is_dentro_janela_tempo(Rota * rota){

	for (int i = 0; i < rota->length-1; i++){
		Service *source = &rota->list[i];
		if (!source->is_source) continue;
		for (int j = i+1; j < rota->length; j++){
			Service *destiny = &rota->list[j];
			if(destiny->is_source || source->r != destiny->r) continue;

			if ( ! ((source->r->pickup_earliest_time <= source->service_time && source->service_time <= source->r->pickup_latest_time)
				&& (destiny->r->delivery_earliest_time <= destiny->service_time && destiny->service_time <= destiny->r->delivery_latest_time)))
				return false;
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

/*Verifica se a distancia percorrida pelo motorista é respeitada*/
bool is_distancia_motorista_respeitada(Rota * rota){
	Service * source = &rota->list[0];
	Service * destiny = &rota->list[rota->length-1];
	double MTD = AD + BD * haversine(source, destiny);//Maximum Travel Distance
	double accDistance = distancia_percorrida(rota);
	return accDistance <= MTD;
}

/*Verifica se o tempo do request partindo do índice I e chegando no índice J é respeitado*/
bool is_tempo_respeitado(Rota *rota, int i, int j){
	Service * source = &rota->list[i];
	Service * destiny = &rota->list[j];
	double MTT = AT + BT * time_between_services(source, destiny);
	double accTime = tempo_gasto_rota(rota, i, j);
	return accTime <= MTT;
}

/*Verifica se os tempos de todos os requests nessa rota estão sendo respeitados*/
bool is_tempos_respeitados(Rota *rota){
	for (int i = 0; i < rota->length-1; i++){//Pra cada um dos sources
		Service *source = &rota->list[i];
		if (!source->is_source) continue;
		for (int j = i+1; j < rota->length; j++){//Repete o for até encontrar o destino
			Service *destiny = &rota->list[j];
			if(destiny->is_source || source->r != destiny->r) continue;

			if (!is_tempo_respeitado(rota, i, j)) return false;
		}
	}
	return true;
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
	if ( !is_dentro_janela_tempo(rota) || !is_carga_dentro_limite(rota) || !is_tempos_respeitados(rota) || !is_distancia_motorista_respeitada(rota) )
		return false;
	return true;
}
