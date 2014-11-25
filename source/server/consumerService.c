#include "server.h"


void consumer_service_await_and_handle_message(ConsumerService*);
void *consumer_service_connection_handler(void *);

/**
 * Create a new ConsumerService struct, and begin the corresponding thread.
 */
int consumer_service_new(Environment *env, int client_sock) {
    ConsumerService *cs = malloc(sizeof(*cs));
    cs->client_sock = client_sock;
    cs->env = env;
    cs->id = consumerList->idx++;
    cs->status = 0;
    cs->resources_consumed = 0;
    if (debug.print) printf("consumer service struct ready\n");

    if( pthread_create(&(cs->thread), NULL, consumer_service_connection_handler, (void*)cs) < 0) {
        if (debug.print) printf("could not create consumer service thread\n");
        free(cs);
        return -1;
    }

    pthread_mutex_lock(&consumerListMutex);

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

    pthread_mutex_unlock(&consumerListMutex);

    if (debug.print) puts("consumer service handler assigned");
    return 0;
}

int consumer_service_remove(ConsumerService *cs) {

    pthread_mutex_lock(&consumerListMutex);

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

    pthread_mutex_unlock(&consumerListMutex);

    
}

/**
 * This will pull a resource off of the buffer for a client process.
 * Any waiting producers are notified that the buffer now has room.
 */
int consumer_service_get_resource(Environment *env, Resource **r) {
    int dequeued = 0;
    pthread_mutex_lock(&bufferMutex);

    // buffer is full
    if (env->bufferp->count == env->bufferp->size) {
        pthread_cond_signal(&bufferHasRoom);
    }
    // dequeue resource from buffer
    dequeued = resource_buffer_dequeue(env->bufferp, r);

    pthread_mutex_unlock(&bufferMutex);
    return dequeued;
}

/**
 * This will notify the client that the connection is established 
 * between the client process and the server thread.
 */
void *consumer_service_connection_handler(void *tp) {
    char *message;
    ConsumerService *t = (ConsumerService *)tp;
     
    // Notify client that a thread has taken the connection
    if (debug.print) printf("Write to sock %d\n",t->client_sock);
    message = "handshake:consumer";
    write(t->client_sock , message , strlen(message));

    // wait for client messages
    consumer_service_await_and_handle_message(t);

    return NULL;
}

/**
 * This handles incoming communications from a client socket for 
 * an individual client thread.
 */
void consumer_service_await_and_handle_message(ConsumerService *t) {
    char *message;
    int recvSize;
    char recvBuff[1025];
    t->status = 0;

    // clear recvBuff
    memset(recvBuff, '\0', sizeof(recvBuff));

    // read a message from the client
    if (debug.print) printf("Attempt to read sock %d\n",t->client_sock);
    recvSize = read(t->client_sock, recvBuff, 1024);
    if (recvSize == 0) {
        // Client has disconnected
        if (debug.print) printf("Client disconnect\n");
        consumer_service_remove(t);
        fflush(stdout);
        pthread_exit(NULL);
    }
    else if (recvSize < 0) {
        // Error reading message
        if (debug.print) printf("ERROR reading from socket\n");
        pthread_exit(NULL);
    }
    else {
        // Valid message from client
        if (debug.print) printf("Message from client: %s\n",recvBuff);
        if( strcmp(recvBuff,"consume") == 0 ) {
            if (debug.print) printf("attempting to consume.\n");
            Resource *r;
            t->status = 1;
            if (consumer_service_get_resource(t->env, &r) == 0) {
                char resource_data[1024];
                if (debug.print) printf("consumed r%d\n", r->id);
                sprintf(resource_data, "rid:%d;produced_by:%d;", r->id, r->produced_by);
                write(t->client_sock, resource_data, strlen(resource_data));
                t->resources_consumed++;
                t->status = 2;
                sleep(3);
                consumer_service_await_and_handle_message(t);
            }
            else {
                char resource_data[1024];
                if (debug.print) printf("buffer underflow\n");
                sprintf(resource_data, "empty buffer");
                write(t->client_sock, resource_data, strlen(resource_data));
                sleep(3);
                consumer_service_await_and_handle_message(t);
            }
        }
        else {
            message = "unrecognized client command.\n";
        }

        if (debug.print) printf("Sending message back to client:\n%s\n", message);
        write(t->client_sock, message, strlen(message));
        sleep(3);
        consumer_service_await_and_handle_message(t);
    }
}
