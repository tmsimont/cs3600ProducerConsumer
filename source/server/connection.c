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


typedef struct {
    Environment* env;
    int client_sock;
} ConnectionThread;

//the thread function
void *connection_handler(void *);
void connection_recurse(ConnectionThread*);

int recvSize;
char recvBuff[1025];
char *message, client_message[2000];


int server_listen(Environment *env) {
    struct sockaddr_in server, client;
    int c;
    pthread_t thread_id;
    c = sizeof(struct sockaddr_in);
     
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
     
    //Accept and incoming connection
    if (debug.print) puts("Waiting for incoming connections...");

    int client_sock;
    while((client_sock = accept(
        env->socket_desc, 
        (struct sockaddr *)&client, 
        (socklen_t*)&c)
    )) {
        if (debug.print) printf("Connection accepted (%d)\n", client_sock);
        
        ConnectionThread *t = malloc(sizeof(*t));
        t->client_sock = client_sock;
        t->env = env;

        if (debug.print) printf("Connection struct ready\n");
 
        if( pthread_create(
            &thread_id, 
            NULL, 
            connection_handler, 
            (void*)t
        ) < 0) {
            perror("could not create thread");
            return 1;
        }
         
        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( thread_id , NULL);
        if (debug.print) puts("Handler assigned");
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
 * This will handle connection for each client
 */
void *connection_handler(void *tp) {
    ConnectionThread *t = (ConnectionThread *)tp;
     
    if (debug.print) printf("Tryna write to sock %d\n",t->client_sock);
    //Send some messages to the client
    message = "Thread has taken connection.\n";
    write(t->client_sock , message , strlen(message));

    sleep(5);
    connection_recurse(t);

    return NULL;
} 

void connection_recurse(ConnectionThread *t) {
    // clear recvBuff
    memset(recvBuff, '\0', sizeof(recvBuff));

    if (debug.print) printf("Tryna read sock %d\n",t->client_sock);

    // read a message from the client
    recvSize = read(t->client_sock, recvBuff, 1024);
    if (recvSize == 0) {
        if (debug.print) printf("Client disconnect\n");
        fflush(stdout);
    }
    else if (recvSize < 0) {
        if (debug.print) printf("ERROR reading from socket");
    }
    else {
        if (debug.print) printf("Message from client: %s\n",recvBuff);
        message = "unrecognized client command.\n";

        if( strcmp(recvBuff,"consume") == 0 ) {
            if (debug.print) printf("attempting to consume.\n");
            Resource *r;
            if (consumer_service_get_resource(t->env, &r) == 0) {
                char message2[1024];
                if (debug.print) printf("consumed r%d\n", r->id);
                sprintf(message2, "here is %d\n", r->id);
                write(t->client_sock, message2, strlen(message2));
                sleep(3);
                connection_recurse(t);
            }
        }
        else {

        }

        if (debug.print) printf("Sending message back to client:\n%s\n", message);
        write(t->client_sock, message, strlen(message));
        sleep(3);
        connection_recurse(t);
    }
}
