/**
 * 
 * @see: https://gist.github.com/silv3rm00n/5821760
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h> 
#include "server.h"


// connection thread functions
void *connection_handler(void *);
int connection_handshake(Environment *, int);
void connection_await_and_handle_message(ConsumerService*);

int server_listen(Environment *env) {
    struct sockaddr_in server;
     
    //Create socket
    env->socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (env->socket_desc == -1) {
        if (debug.print) puts("Could not create socket");
    }
    if (debug.print) puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 60118 );
     
    //Bind
    if(bind(env->socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    if (debug.print) puts("bind done");
     
    //Listen
    listen(env->socket_desc, 3);
    if (debug.print) puts("Waiting for incoming connections...");

    //Accept incoming connection
    int client_sock;
    while((client_sock = accept(env->socket_desc, NULL, NULL))) {
        if (debug.print) printf("Connection accepted (%d)\n", client_sock);
        connection_handshake(env, client_sock);
    }
     
    // Identify failure to accept()
    if (client_sock < 0) {
        perror("accept failed");
        return 1;
    }
     
    return 0;
}

int server_close_connection(void) {
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
void *connection_handler(void *tp) {
    char *message;
    ConsumerService *t = (ConsumerService *)tp;
     
    // Notify client that a thread has taken the connection
    if (debug.print) printf("Write to sock %d\n",t->client_sock);
    message = "handshake:consumer\n";
    write(t->client_sock , message , strlen(message));

    // wait for client messages
    connection_await_and_handle_message(t);

    return NULL;
}

/**
 * Initial message from incoming connection, attempt to validate.
 */
int connection_handshake(Environment *env, int client_sock) {
    char *message;
    int recvSize;
    char recvBuff[1025];

    // read a message from the client
    if (debug.print) printf("Attempting to handshake w/ sock %d\n",client_sock);
    recvSize = read(client_sock, recvBuff, 1024);
    recvBuff[recvSize] = '\0';
    if (recvSize == 0) {
        // Client has disconnected
        if (debug.print) printf("Client disconnect\n");
        return -1;
    }
    else if (recvSize < 0) {
        // Error reading message
        if (debug.print) printf("ERROR reading from socket\n");
            return -1;
    }
    else {
        if( strcmp(recvBuff,"handshake:consumer") == 0 ) {
            // incoming connection is new consumer
            ConsumerService *t = malloc(sizeof(*t));
            t->client_sock = client_sock;
            t->env = env;
            if (debug.print) printf("consumer service struct ready\n");

            if( pthread_create(&(t->thread), NULL, connection_handler, (void*)t) < 0) {
                printf("could not create thread\n");
                return -1;
            }

            if (debug.print) puts("Handler assigned");
            return 0;
        }
        else if( strcmp(recvBuff,"handshake:monitor") == 0 ) {
            // incoming connection is new monitor
            return 0;
        }
        else {
            if (debug.print) printf("invalid request from client:\n%s\n", recvBuff);
            message = "invalid request\n";
            if (debug.print) printf("Sending message back to client:\n%s\n", message);
            write(client_sock, message, strlen(message));
            return -1;
        }
    }
    return 0;
}

/**
 * This handles incoming communications from a client socket for 
 * an individual client thread.
 */
void connection_await_and_handle_message(ConsumerService *t) {
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
                char message2[1024];
                if (debug.print) printf("consumed r%d\n", r->id);
                sprintf(message2, "here is %d\n", r->id);
                write(t->client_sock, message2, strlen(message2));
                sleep(3);
                connection_await_and_handle_message(t);
            }
        }
        else {
            message = "unrecognized client command.\n";
        }

        if (debug.print) printf("Sending message back to client:\n%s\n", message);
        write(t->client_sock, message, strlen(message));
        sleep(3);
        connection_await_and_handle_message(t);
    }
}
