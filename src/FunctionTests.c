/*
 * FunctionTests.c
 *
 *  Created on: 22 de nov de 2015
 *      Author: arthur
 */

#include "Helper.h"
#include "Calculations.h"
#include <stdio.h>
#include <stdlib.h>
#include "GenerationalGA.h"


typedef struct Simples{
	double t;
} Simples;


/*Pra poder usar a função qsort com N objetivos,
 * precisamos implementar os n algoritmos de compare*/
int compare(const void *p, const void *q) {
    int ret;
    Simples * x = *(Simples **)p;
    Simples * y = *(Simples **)q;
    if (x->t == y->t)
        ret = 0;
    else if (x->t < y->t)
        ret = -1;
    else
        ret = 1;
    return ret;
}


int mainbak(){
	Simples** list = calloc(sizeof(Simples*), 5);
	for (int i = 0; i < 5; i++){
		list[i] = calloc(sizeof(Simples),1);
	}

	list[0]->t = 12;
	list[1]->t = 6;
	list[2]->t = 0;
	list[3]->t = 2;
	list[4]->t = 3;
	printf("before\n");
	printf("%f\n", list[0]->t);
	printf("%f\n", list[1]->t);
	printf("%f\n", list[2]->t);
	printf("%f\n", list[3]->t);
	printf("%f\n", list[4]->t);

	qsort(list, 5, sizeof(Simples*), compare );

	printf("\nafter\n");
	printf("%f\n", list[0]->t);
	printf("%f\n", list[1]->t);
	printf("%f\n", list[2]->t);
	printf("%f\n", list[3]->t);
	printf("%f\n", list[4]->t);

	return 0;

}


int main_haversine(){

	double longitude_a = -88.13133;
	double latitude_a = 42.11068;

	double longitude_b = -88.07178;
	double latitude_b = 42.14213;

	double result = haversine_helper(latitude_a, longitude_a, latitude_b, longitude_b);

	printf("%f\n", result);
	return 0;
}
