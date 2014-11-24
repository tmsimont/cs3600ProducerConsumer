/* 
 * File:   server.h
 * Author: Trevor Simonton
 *
 * Created on November 17, 2014, 5:04 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define MAX_PRODUCERS 128

// Resource
typedef struct _Resource Resource;
struct _Resource {
    int id;
    int produced_by;
    int consumed_by;
    Resource *next;
};
Resource *resource_new(int);
int resourceID;

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
int pidx;
Producer *producer_new(ResourceBuffer*);

// Primary server function
typedef struct _environment Environment;
struct _environment {
    int socket_desc;
    ResourceBuffer *bufferp;
};

int consumer_service_new(Environment *, int);

int report_status(void);
int server_listen(Environment*);
int server_close_connection(void);


struct {
    unsigned int print : 1;
} debug;


// helper struct for individual consumer service threads
typedef struct {
    Environment* env;
    int client_sock;
    pthread_t thread;
} ConsumerService;
int consumer_service_new(Environment *, int);
int consumer_service_get_resource(Environment *, Resource **);


// helper struct for individual consumer service threads
typedef struct {
    Environment* env;
    int client_sock;
    pthread_t thread;
} MonitorService;
int monitor_service_new(Environment *, int);

int xml_write_message(int, char *);