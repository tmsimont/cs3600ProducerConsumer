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

int initialize_producers(void) {
    int numProducers = 5;

    int i;
    for (i = 0; i < numProducers; i++) {
        producers[i] = producer_new();
    }


    for (i = 0; i < numProducers; i++) {
        producer_produce(producers[i]);
    }
    return 0;
}

int execute_producers(void) {
    return 0;
}
int report_status(void) {
    return 0;
}

/*
 * Initialize program and begin listening for client requests 
 */
int main(int argc, char** argv) {
    initialize_producers();
    server_listen();
    return (EXIT_SUCCESS);
}


