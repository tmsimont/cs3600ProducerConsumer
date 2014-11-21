/* 
 * File:   producer.c
 * Author: Trevor Simonton
 *
 * Created on November 17, 2014, 5:06 PM
 */

#include "server.h"

#define MEGEXTRA (1024*1024)

// index for thread creation/id assignment
int pidx = 0;

// pthread attribute for stack size
pthread_attr_t attr;

int bufferSim;

struct _Producer {
    int id;
    pthread_t thread;
    ResourceBuffer *bufferp;
    int status;
};


void *producer_produce(void *pi) {
    Producer *p = (Producer *)pi;

    while(1) {

        pthread_mutex_lock(&bufferMutex);

        // buffer is full 
        if (p->bufferp->count == p->bufferp->size) {
            if(debug.print) printf("producer %d is waiting...\n", p->id);
            // wait until there is room in buffer
            pthread_cond_wait(&bufferHasRoom, &bufferMutex);
        }

        if(debug.print) printf("producer %d produce to buffer\n", p->id);
        resource_buffer_enqueue(p->bufferp, resource_new(p->id));
        resource_buffer_print(p->bufferp);
        sleep(p->id * 2);

        pthread_mutex_unlock(&bufferMutex);

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

    // configure thread stack size based on Producer size
    size_t stacksize;
    pthread_attr_init(&attr);
    stacksize = sizeof(Producer)*1024+MEGEXTRA;
    pthread_attr_setstacksize (&attr, stacksize);

    // set Producer struct properties and record producer to global array
    p->id = pidx;
    p->bufferp = rb;
    producers[pidx] = p;
    pidx++;

    // create the producer thread
    p->thread = pthread_create(&(p->thread), &attr, producer_produce, (void *)p);

    // clean up
    pthread_attr_destroy(&attr);


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
