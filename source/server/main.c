/**
 * @file
 * Author: Trevor Simonton
 *
 * The main function will simply initialize global variables
 * and begin the server_listen() process.
 */

#include "server.h"

int start() {
    // initialize buffer
    globalResourceBuffer = resource_buffer_new(bufferSize);
    env->bufferp = globalResourceBuffer;

    // initialize producers
    initialize_producers(env->bufferp, numProducers);
}

/**
 * Initialize program and begin listening for client requests 
 */
int main(int argc, char** argv) {
    debug.print = 0;

    // set behavioral variables
    bufferSize = 3;
    numProducers = 5;
    consumeDelay = 1;
    consumerRest = 1;
    produceDelay = 2;
    producerRest = 1;

    // allow behavior vars to be overridden with command line args
    if (argc > 1) {
        int i;
        for (i = 1; i < argc; i++) {
            switch (i) {
                case 1:
                    bufferSize = atoi(argv[i]);
                    break;
                case 2:
                    numProducers = atoi(argv[i]);
                    break;
                case 3:
                    consumeDelay = atoi(argv[i]);
                    break;
                case 4:
                    consumerRest = atoi(argv[i]);
                    break;
                case 5:
                    produceDelay = atoi(argv[i]);
                    break;
                case 6:
                    producerRest = atoi(argv[i]);
                    break;
                case 7:
                    debug.print = atoi(argv[i]);
                    break;
            }
        }
    }

    // initialize global producer and resource indices
    pidx = 0;
    ridx = 0;

    // initialize resource buffer and environment structures
    env = malloc(sizeof(*env));

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

    // begin producer/buffer threads
    start();

    // wait for connections
    server_listen(env);

    // exit when server_listen terminates
    pthread_exit(NULL);

    return (EXIT_SUCCESS);
}


