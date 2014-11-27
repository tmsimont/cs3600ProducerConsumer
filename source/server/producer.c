/** 
 * @file
 * Author: Trevor Simonton
 *
 * The producer handles production of resources and places them into 
 * the ResourceBuffer.
 */

#include "server.h"

enum { SLEEP, PRODUCING, EXPORT, WAITING };

/**
 * Primary producer loop.
 * This will repeatedly acquire the bufferMutex, and then add a resource 
 * to the buffer. If the buffer is full, the thread will wait until
 * a ConsumerService thread signals that there is room in the buffer for new
 * resources. This waiting is handled by pthread_cond_wait()
 */
void *producer_produce(void *pi) {
    Producer *p = (Producer *)pi;
    while(1) {
        // time delay between productions
        p->status = SLEEP;
        sleep(producerRest);
        
        // acquire buffer mutex
        if(debug.print) printf("producer %d acquiring bufferMutex\n", p->id);
        pthread_mutex_lock(&bufferMutex);
        if(debug.print) printf("producer %d acquired bufferMutex\n", p->id);

        // CRITICAL SECTION-------------------------------------------
        // buffer is full 
        if (p->bufferp->count == p->bufferp->size) {
            if(debug.print) printf("producer %d is waiting (%d to %d)...\n", p->id, p->bufferp->count, p->bufferp->size);
            p->status = WAITING;
            // wait until there is room in buffer
            pthread_cond_wait(&bufferHasRoom, &bufferMutex);
        }

        // enqueue new resource to buffer
        if(debug.print) printf("producer %d produce to buffer\n", p->id);
        p->status = EXPORT;
        resource_buffer_enqueue(p->bufferp, resource_new(p->id));
        p->resources_produced++;
        if(debug.print) resource_buffer_print(p->bufferp);

        // signal producers that we have resources available
        pthread_cond_signal(&bufferNotEmpty);

        // tell monitors about update
        monitor_push_reports();

        // END CRITICAL SECTION---------------------------------------

        // release mutex
        pthread_mutex_unlock(&bufferMutex);
        if(debug.print) printf("producer %d released bufferMutex\n", p->id);
        
        // wait to produce more for produceDelay seconds
        p->status = PRODUCING;
        sleep(produceDelay);
    }
    free(p);
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
    p->status = PRODUCING;
    producers[pidx] = p;
    pidx++;

    // create the producer thread
    pthread_create(&(p->thread), NULL, producer_produce, (void *)p);

    // return Producer to caller
    return p;
}

/**
 * Create the given number of producers. Pass them the given ResourceBuffer
 */
int initialize_producers(ResourceBuffer *rb, int numProducers) {
    int i;
    for (i = 0; i < numProducers; i++) {
        producer_new(rb);
    }
    return 0;
}
