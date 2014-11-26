/**
 * File:   consumerService.c
 * Author: Trevor Simonton
 * 
 * The ConsumerService is a struct that tracks data that is used by 
 * individual Consumer-handling threads.
 */

#include "server.h"

enum { SLEEPING, HUNGRY, CONSUMING };

int consumer_service_await_and_handle_message(ConsumerService*);
void *consumer_service_connection_handler(void *);

/**
 * Create a new ConsumerService struct, and begin the corresponding thread.
 * This new struct is added to the global linked list of ConsumerService
 * structs.  This is a doubly-linked list with a tail. The list helps us
 * track any existing consumer connections.
 */
int consumer_service_new(Environment *env, int client_sock) {
    ConsumerService *cs = malloc(sizeof(*cs));
    cs->client_sock = client_sock;
    cs->env = env;
    cs->id = consumerList->idx++;
    cs->status = SLEEPING;
    cs->resources_consumed = 0;
    if (debug.print) printf("consumer service struct ready\n");

    if( pthread_create(&(cs->thread), NULL, consumer_service_connection_handler, (void*)cs) < 0) {
        if (debug.print) printf("could not create consumer service thread\n");
        free(cs);
        return -1;
    }
    
    // notify of new consumer
    monitor_push_reports();

    if (debug.print) puts("consumer service handler assigned");
    return 0;
}

/**
 * Remove a ConsumerService from the global linked list of ConsumerService
 * structs. This should be called when a Consumer disconnects
 */
int consumer_service_remove(ConsumerService *cs) {

    // acquire list mutex
    pthread_mutex_lock(&consumerListMutex);


    // CRITICAL SECTION-------------------------------------------

    // remove from linked list
    if (consumerList->count == 0) {
        if (debug.print) printf("unable to remove CS-%d from empty list (%d)\n", cs->id, consumerList->count);
    }
    else if (consumerList->count == 1) {
        consumerList->head = NULL;
        consumerList->tail = NULL;
        consumerList->count = 0;
        if (debug.print) printf("CS-%d removed from singleton list (%d)\n", cs->id, consumerList->count);
    }
    else {
        if (cs->id == consumerList->head->id) {
            consumerList->head = consumerList->head->next;
            consumerList->head->prev = NULL;
            consumerList->count--;
            if (debug.print) printf("CS-%d removed from head of large list (%d)\n", cs->id, consumerList->count);
        }
        else {
            ConsumerService *temp;
            temp = consumerList->head;
            while (temp->next->next != NULL && temp->next->id != cs->id) {
                temp = temp->next;
            }
            if (temp->next->id == cs->id) {
                if (temp->next->next != NULL) {
                    temp->next->next->prev = temp;
                    temp->next = temp->next->next;
                    consumerList->count--;
                    if (debug.print) printf("CS-%d removed large list (%d)\n", cs->id, consumerList->count);
                }
                else {
                    consumerList->tail = temp;
                    temp->next = NULL;
                    consumerList->count--;
                    if (debug.print) printf("CS-%d removed from end of large list (%d)\n", cs->id, consumerList->count);
                }
            }
            else {
                if (debug.print) printf("unable to remove CS-%d from non-empty list (%d)\n", cs->id, consumerList->count);
                // service not in list
            }
        }
    }
    free(cs);
    if (debug.print) printf("consumer struct freed from memory\n");


    // END CRITICAL SECTION---------------------------------------


    // release list mutex
    pthread_mutex_unlock(&consumerListMutex);

    
}

/**
 * This will pull a resource off of the buffer for a client process.
 * Any waiting producers are notified that the buffer now has room.
 */
int consumer_service_get_resource(Environment *env, Resource **r) {
    int dequeued = 0;
    
    // acquire bufferMutex
    if (debug.print) printf("consumer attempting buffer mutex\n");
    pthread_mutex_lock(&bufferMutex);
    if (debug.print) printf("consumer has buffer mutex\n");

    // CRITICAL SECTION-------------------------------------------

    // buffer is full
    if (debug.print) printf("checking env->bufferp->count\n");
    if (env->bufferp->count == env->bufferp->size) {
        // signal producers that we are making room in the buffer
        pthread_cond_signal(&bufferHasRoom);
    }
    if (env->bufferp->count == 0) {
        // make sure producers know there is room in the buffer
        pthread_cond_signal(&bufferHasRoom);
        
        // let monitors know about the condition
        monitor_push_reports();

        // wait until there are resources
        pthread_cond_wait(&bufferNotEmpty, &bufferMutex);
    }

    // dequeue resource from buffer
    if (debug.print) printf("calling dequeue\n");
    dequeued = resource_buffer_dequeue(env->bufferp, r);

    // END CRITICAL SECTION---------------------------------------
    
    // release bufferMutex
    pthread_mutex_unlock(&bufferMutex);

    return dequeued;
}

/**
 * This will notify the client that the connection is established 
 * between the client process and the server thread. Note that 
 * access to the global consumerList is protected by mutex in this function.
 */
void *consumer_service_connection_handler(void *tp) {
    char *message;
    ConsumerService *cs = (ConsumerService *)tp;
    
    // acquire consumerListMutex
    pthread_mutex_lock(&consumerListMutex);

    // CRITICAL SECTION-------------------------------------------

    // add to linked list
    if (consumerList->count == 0) {
        consumerList->head = cs;
        consumerList->tail = cs;
        consumerList->count++;
        if (debug.print) printf("CS-%d added to list at head (%d)\n", cs->id, consumerList->count);
    }
    else {
        consumerList->tail->next = cs;
        cs->prev = consumerList->tail;
        consumerList->tail = cs;
        consumerList->count++;
        if (debug.print) printf("CS-%d added to list (%d)\n", cs->id, consumerList->count);
    }

    // END CRITICAL SECTION---------------------------------------

    // release consumerListMutex
    pthread_mutex_unlock(&consumerListMutex);

    // Notify client that a thread has taken the connection
    if (debug.print) printf("Write to sock %d\n",cs->client_sock);
    message = "handshake:consumer";
    write(cs->client_sock , message , strlen(message));

    // wait for client messages
    while(consumer_service_await_and_handle_message(cs) == 0) {
       ;
    }

    // remove service from global list when connection fails
    consumer_service_remove(cs);

    pthread_exit(NULL);

    return NULL;
}

/**
 * This handles incoming communications from a client socket for 
 * an individual client thread.
 */
int consumer_service_await_and_handle_message(ConsumerService *t) {
    int recvSize;
    char recvBuff[1025];

    t->status = SLEEPING;

    // clear recvBuff
    memset(recvBuff, '\0', sizeof(recvBuff));

    // read a message from the client
    if (debug.print) printf("Attempt to read sock %d\n",t->client_sock);
    recvSize = read(t->client_sock, recvBuff, 1024);
    if (recvSize == 0) {
        // Client has disconnected
        if (debug.print) printf("Client disconnect\n");
        fflush(stdout);
        return -1;
    }
    else if (recvSize < 0) {
        // Error reading message
        if (debug.print) printf("ERROR reading from socket\n");
        return -1;
    }
    else {
        // Valid message from client
        if (debug.print) printf("Message from client: %s\n",recvBuff);

        // limit recvBuff size to 7, to eliminate duplicate "consumeconsume" commands
        // TODO: why do some messages come through duplicated? (need message framing...)
        strncpy(recvBuff, recvBuff, 6);
        recvBuff[7] = '\0';
        if (debug.print) printf("Message from client cleaned: %s\n",recvBuff);

        // consume message from the client
        if( strcmp(recvBuff,"consume") == 0 ) {
            if (debug.print) printf("attempting to consume.\n");
            Resource *r;
            t->status = HUNGRY;
            
            // try to get a resource for the client
            // NOTE: consumer_service_get_resource() will wait until resources are ready
            if (consumer_service_get_resource(t->env, &r) == 0) {
                // construct a message for the client now that we have a resource
                char resource_data[1024];
                if (debug.print) printf("about to write about dequeued resource\n");
                if (debug.print) printf("consumed r%d\n", r->id);
                sprintf(resource_data, "rid:%d;produced_by:%d;", r->id, r->produced_by);

                // send the message to the client
                write(t->client_sock, resource_data, strlen(resource_data));

                // update this service's thread data
                t->resources_consumed++;
                t->status = CONSUMING;
                
                // free the resource memory
                free(r);

                // push reports out to listening monitors
                monitor_push_reports();

                // sleep for given consumer delay
                sleep(consumeDelay);
            }
            else {
                /** 
                 * consumer_service_get_resource() should have waited until
                 * it got a resource from the buffer, this shouldn't be a 
                 * reachable code block.
                 */
                if (debug.print) printf("ERROR: no resource after wait for client.\n");
                pthread_exit(NULL);
                return -1;
            }
        }
        else {
            if (debug.print) printf("unrecognized client command.\n");
        }
    }
    return 0;
}
