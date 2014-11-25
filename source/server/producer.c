/* 
 * File:   producer.c
 * Author: Trevor Simonton
 *
 * Created on November 17, 2014, 5:06 PM
 */

#include "server.h"

#define MEGEXTRA (1024*1024)

// pthread attribute for stack size
pthread_attr_t attr;

int bufferSim;



void *producer_produce(void *pi) {
    Producer *p = (Producer *)pi;

    while(1) {
        p->status = 1;

        if(debug.print) printf("producer %d acquiring bufferMutex\n", p->id);
        pthread_mutex_lock(&bufferMutex);
        if(debug.print) printf("producer %d acquired bufferMutex\n", p->id);

        // buffer is full 
        if (p->bufferp->count == p->bufferp->size) {
            if(debug.print) printf("producer %d is waiting (%d to %d)...\n", p->id, p->bufferp->count, p->bufferp->size);
            // wait until there is room in buffer
            pthread_cond_wait(&bufferHasRoom, &bufferMutex);
        }

        if(debug.print) printf("producer %d produce to buffer\n", p->id);
        p->status = 2;
        resource_buffer_enqueue(p->bufferp, resource_new(p->id));
        p->resources_produced++;
        resource_buffer_print(p->bufferp);
        p->status = 0;
        pthread_mutex_unlock(&bufferMutex);

        sleep(produceDelay);

        if(debug.print) printf("producer %d released bufferMutex\n", p->id);

    }

    pthread_exit(NULL);
}

/**
 * Create a new Producer struct to track a new producer thread.
 * Memory will be allocated for the struct and a thread will be
 * created.
 */
Producer *producer_new(ResourceBuffer *rb) {
    // allocate memory for Producer
    Producer *p = malloc(sizeof(*p));

    // set Producer struct properties and record producer to global array
    p->id = pidx;
    p->bufferp = rb;
    p->resources_produced = 0;
    p->status = 0;
    producers[pidx] = p;
    pidx++;

    // create the producer thread
    pthread_create(&(p->thread), NULL, producer_produce, (void *)p);

    // return Producer to caller
    return p;
}

int initialize_producers(ResourceBuffer *rb, int numProducers) {
    int i;
    for (i = 0; i < numProducers; i++) {
        producer_new(rb);
    }
    return 0;
}
