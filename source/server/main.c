/* 
 * File:   main.c
 * Author: Trevor
 *
 * The main function will simply initialize global variables
 * and begin the server_listen() process.
 */

#include "server.h"

/**
 * Initialize program and begin listening for client requests 
 */
int main(int argc, char** argv) {
    debug.print = 1;

    // set behavioral variables
    consumeDelay = 1;
    produceDelay = 2;
    monitorPullDelay = 1;

    // initialize global producer and resource indices
    pidx = 0;
    ridx = 0;

    // initialize resource buffer and environment structures
    ResourceBuffer *rb;
    Environment *env = malloc(sizeof(*env));

    // initialize global mutexes
    pthread_mutex_init(&bufferMutex, NULL);
    pthread_mutex_init(&consumerListMutex, NULL);
    pthread_mutex_init(&monitorListMutex, NULL);
    pthread_cond_init (&bufferHasRoom, NULL);
    pthread_cond_init (&bufferNotEmpty, NULL);

    // initialize consumersList
    consumerList = malloc(sizeof(*consumerList));
    consumerList->count = 0;

    // initialize monitorList
    monitorList = malloc(sizeof(*monitorList));
    monitorList->count = 0;

    // initialize buffer
    env->bufferp = resource_buffer_new(3);

    // initialize producers
    initialize_producers(env->bufferp, 5);

    // wait for connections
    server_listen(env);

    // exit when server_listen terminates
    pthread_exit(NULL);

    return (EXIT_SUCCESS);
}


