/**
 * 
 * @see: https://gist.github.com/silv3rm00n/5821760
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h> 
#include "server.h"

int connection_handshake(Environment *, int);

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
            return consumer_service_new(env, client_sock);
        }
        else if( strcmp(recvBuff,"handshake:monitor") == 0 ) {
            // incoming connection is new monitor
            return monitor_service_new(env, client_sock);
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
