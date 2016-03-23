/*
 * Calculations.h
 *
 *  Created on: 4 de mar de 2016
 *      Author: SIGAA-Des
 */

#ifndef CALCULATIONS_H_
#define CALCULATIONS_H_

#include "GenerationalGA.h"

double distancia_percorrida(Rota * rota);
double haversine(Service *a, Service *b);
double haversine_helper(double lat1, double lon1, double lat2, double lon2);
double time_between_services(Service *a, Service *b);
double tempo_gasto_rota(Rota *rota, int i, int j);
double calculate_time_at(Service * actual, Service *ant);
bool is_dentro_janela_tempo(Rota * rota);
bool is_carga_dentro_limite(Rota *rota);
bool is_distancia_motorista_respeitada(Rota * rota);
bool is_tempo_respeitado(Rota *rota, int i, int j);
bool is_tempos_respeitados(Rota *rota);
bool is_rota_valida(Rota *rota);
double get_earliest_time_service(Service * atual);
double get_latest_time_service(Service * atual);

#endif /* CALCULATIONS_H_ */
