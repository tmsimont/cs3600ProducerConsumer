/**
 * 
 * @see: https://gist.github.com/silv3rm00n/5821760
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h> 
#include <pthread.h>

#include "server.h"


//the thread function
void *connection_handler(void *);

int server_listen(void) {
    int socket_desc, client_sock, c;
    struct sockaddr_in server, client;
    pthread_t thread_id;
    c = sizeof(struct sockaddr_in);
     
    //Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 60118 );
     
    //Bind
    if(bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc, 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
	
    while((client_sock = accept(
        socket_desc, 
        (struct sockaddr *)&client, 
        (socklen_t*)&c)
    )) {
        puts("Connection accepted");
         
        if( pthread_create(
            &thread_id, 
            NULL, 
            connection_handler, 
            (void*)&client_sock
        ) < 0) {
            perror("could not create thread");
            return 1;
        }
         
        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( thread_id , NULL);
        puts("Handler assigned");
    }
     
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
 * This will handle connection for each client
 */
void *connection_handler(void *socket_desc) {
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int recvSize;
    char recvBuff[1025];
    char *message, client_message[2000];

    // clear recvBuff
    memset(recvBuff, '\0', sizeof(recvBuff));
     
    //Send some messages to the client
    message = "Thread has taken connection.\n";
    write(sock , message , strlen(message));

    sleep(2);

    message = "and then slept for 2 seconds and sent this.\n";
    write(sock , message , strlen(message));

    // read a message from the client
    recvSize = read(sock, recvBuff, 1024);
    if (recvSize == 0) {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if (recvSize < 0) {
        printf("ERROR reading from socket");
        return NULL;
    }

    printf("Here is the message: %s\n",recvBuff);
         
    return NULL;
} 

