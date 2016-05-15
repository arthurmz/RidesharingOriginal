/*
 * staticVariables.h
 *
 *  Created on: 21 de nov de 2015
 *      Author: arthur
 */

#ifndef STATICVARIABLES_H_
#define STATICVARIABLES_H_

/*Variáveis estáticas do problema*/
//const static double MPs = 16;//16 metros por segundo
const static double VEHICLE_SPEED = 60;//Em km/h
const static double SERVICE_TIME = 0;//Tempo gasto para servir um Rider
const static double AT = 0;//Parte A da equação de tempo máximo (para driver e rider)
const static double BT = 1.3;//Parte B da equação de tempo máximo (para driver e rider)
const static double AD = 0;//Parte A da função de distância máxima do driver
const static double BD = 1.3;//parte B da função de distância máxima do driver

const static double BD_FRAC = 0.3;//parte fracionária do BD;

const static int VEHICLE_CAPACITY = 5;//Capacidade dos carros
const static double RIDER_DEMAND = 1;//Todos os caronas tem apenas uma pessoa

const static int QTD_OBJECTIVES = 4;

const static int TOTAL_DISTANCE_VEHICLE_TRIP = 0;
const static int TOTAL_TIME_VEHICLE_TRIPS = 1;
const static int TOTAL_TIME_RIDER_TRIPS = 2;
const static int RIDERS_UNMATCHED = 3;

const static int MAX_SERVICES_MALLOC_ROUTE = 60; //A quantidade de services alocados por rota

const static int EPSILON = 1;

#endif /* STATICVARIABLES_H_ */
