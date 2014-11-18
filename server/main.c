/* 
 * File:   main.c
 * Author: Trevor
 *
 * Created on November 9, 2014, 5:58 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "server.h"

/*
 * 
 */
int main(int argc, char** argv) {
    int numProducers = 5;

    int i;
    for (i = 0; i < numProducers; i++) {
        producers[i] = producer_new();
    }


    for (i = 0; i < numProducers; i++) {
        producer_produce(producers[i]);
    }


    return (EXIT_SUCCESS);
}


