#include "server.h"


void consumer_service_await_and_handle_message(ConsumerService*);
void *consumer_service_connection_handler(void *);

/**
 * Create a new ConsumerService struct, and begin the corresponding thread.
 */
int consumer_service_new(Environment *env, int client_sock) {
    ConsumerService *t = malloc(sizeof(*t));
    t->client_sock = client_sock;
    t->env = env;
    if (debug.print) printf("consumer service struct ready\n");

    if( pthread_create(&(t->thread), NULL, consumer_service_connection_handler, (void*)t) < 0) {
        if (debug.print) printf("could not create consumer service thread\n");
        return -1;
    }

    if (debug.print) puts("consumer service handler assigned");
    return 0;
}

/**
 * This will pull a resource off of the buffer for a client process.
 * Any waiting producers are notified that the buffer now has room.
 */
int consumer_service_get_resource(Environment *env, Resource **r) {
    pthread_mutex_lock(&bufferMutex);

    // buffer is full
    if (env->bufferp->count == env->bufferp->size) {
        pthread_cond_signal(&bufferHasRoom);
    }
    // dequeue resource from buffer
    resource_buffer_dequeue(env->bufferp, r);

    pthread_mutex_unlock(&bufferMutex);
    return 0;
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

    // clear recvBuff
    memset(recvBuff, '\0', sizeof(recvBuff));

    // read a message from the client
    if (debug.print) printf("Attempt to read sock %d\n",t->client_sock);
    recvSize = read(t->client_sock, recvBuff, 1024);
    if (recvSize == 0) {
        // Client has disconnected
        if (debug.print) printf("Client disconnect\n");
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
            if (consumer_service_get_resource(t->env, &r) == 0) {
                char resource_data[1024];
                if (debug.print) printf("consumed r%d\n", r->id);
                sprintf(resource_data, "rid:%d;produced_by:%d;", r->id, r->produced_by);
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
