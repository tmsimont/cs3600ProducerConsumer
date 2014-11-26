/**
 * File:   connection.c
 * Author: Trevor Simonton
 * 
 * This is the primary connection handler for the main
 * thread loop. 
 * 
 * The socket creation and thread dispatcher code in server_listen()
 * was based on code found in an example on github:
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

/**
 * Primary server listener loop.
 * Wait infinitely for connections to the server (until an error occurs)
 * on the given application port.
 */
int server_listen(Environment *env) {
    struct sockaddr_in server;
     
    // create the socket
    env->socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (env->socket_desc == -1) {
        if (debug.print) puts("Could not create socket");
    }
    if (debug.print) puts("Socket created");
     
    // prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( APPLICATION_PORT );
     
    // bind the socket
    if(bind(env->socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    if (debug.print) puts("bind done");
     
    // listen
    listen(env->socket_desc, 3);
    if (debug.print) puts("Waiting for incoming connections...");

    // accept incoming connections forever (until error occurs)
    int client_sock;
    while((client_sock = accept(env->socket_desc, NULL, NULL))) {
        if (debug.print) printf("Connection accepted (%d)\n", client_sock);
        // handle the connection
        connection_handshake(env, client_sock);
    }
     
    // identify failure to accept()
    if (client_sock < 0) {
        perror("accept failed");
        return 1;
    }

    return 0;
}


/**
 * Handle the initial message from incoming connection.
 * This will attempt to validate the "handshake" message and respond
 * by creating a thread of the requested connection type.
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
