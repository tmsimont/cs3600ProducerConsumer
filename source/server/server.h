/* 
 * File:   server.h
 * Author: Trevor Simonton
 *
 * Created on November 17, 2014, 5:04 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_PRODUCERS 128

// Resource
typedef struct _Resource Resource;
struct _Resource {
    int id;
    Resource *next;
};
Resource *resource_new(int);

// ResourceBuffer
typedef struct _ResourceBuffer ResourceBuffer;
struct _ResourceBuffer {
    int size;
    int count;
    Resource *head;
};
ResourceBuffer *resource_buffer_new(int);
void resource_buffer_test(ResourceBuffer*);
void resource_buffer_print(ResourceBuffer*);
int initialize_producers(ResourceBuffer*, int);

// bufferMutex
pthread_mutex_t bufferMutex;
pthread_cond_t bufferHasRoom;

// Producer
typedef struct _Producer Producer;
Producer *producers[MAX_PRODUCERS];
Producer *producer_new(ResourceBuffer*);

// Primary server function
typedef struct _environment Environment;
struct _environment {
    int socket_desc;
    int client_sock;
    ResourceBuffer *bufferp;
};


int report_status(void);
int server_listen(Environment*);
int server_close_connection(void);


struct {
    unsigned int print : 1;
} debug;
