#include "server.h"


void monitor_service_await_and_handle_message(ConsumerService*);
void *monitor_service_connection_handler(void *);

/**
 * Create a new ConsumerService struct, and begin the corresponding thread.
 */
int monitor_service_new(Environment *env, int client_sock) {
    MonitorService *t = malloc(sizeof(*t));
    t->client_sock = client_sock;
    t->env = env;
    if (debug.print) printf("monitor service struct ready\n");

    if( pthread_create(&(t->thread), NULL, monitor_service_connection_handler, (void*)t) < 0) {
        if (debug.print) printf("could not create monitor service thread\n");
        return -1;
    }

    if (debug.print) puts("monitor service handler assigned");
    return 0;
}

/**
 * This will notify the client that the connection is established 
 * between the client process and the server thread.
 */
void *monitor_service_connection_handler(void *tp) {
    char *message;
    MonitorService *t = (MonitorService *)tp;
     
    // Notify client that a thread has taken the connection
    if (debug.print) printf("Write to sock %d\n",t->client_sock);
    message = "handshake:monitor";
    write(t->client_sock , message , strlen(message));

    // wait for client messages
    monitor_service_await_and_handle_message(t);

    return NULL;
}

/**
 * This handles incoming communications from a client socket for 
 * an individual client thread.
 */
void monitor_service_await_and_handle_message(ConsumerService *t) {
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
        if( strcmp(recvBuff,"report") == 0 ) {
            sleep(5);
            message = "report data";
        }
        else {
            message = "unrecognized client command.";
        }

        if (debug.print) printf("Sending message back to client:\n%s\n", message);
        write(t->client_sock, message, strlen(message));
        sleep(3);
        monitor_service_await_and_handle_message(t);
    }
}
